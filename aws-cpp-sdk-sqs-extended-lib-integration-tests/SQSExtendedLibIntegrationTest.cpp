/*
 * Copyright 2010-2015 Amazon.com, Inc. or its affiliates. All Rights Reserved.
 * 
 * Licensed under the Apache License, Version 2.0 (the "License").
 * You may not use this file except in compliance with the License.
 * A copy of the License is located at
 * 
 *  http://aws.amazon.com/apache2.0
 * 
 * or in the "license" file accompanying this file. This file is distributed
 * on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either
 * express or implied. See the License for the specific language governing
 * permissions and limitations under the License.
 */

#include <aws/external/gtest.h>
#include <aws/testing/ProxyConfig.h>
#include <aws/core/client/ClientConfiguration.h>
#include <aws/core/client/CoreErrors.h>
#include <aws/core/auth/AWSCredentialsProviderChain.h>
#include <aws/core/utils/json/JsonSerializer.h>
#include <aws/core/utils/xml/XmlSerializer.h>
#include <aws/sqs/SQSClient.h>
#include <aws/sqs/model/CreateQueueRequest.h>
#include <aws/sqs/model/ListQueuesRequest.h>
#include <aws/sqs/model/DeleteQueueRequest.h>
#include <aws/sqs/model/SendMessageRequest.h>
#include <aws/sqs/model/ReceiveMessageRequest.h>
#include <aws/sqs/model/DeleteMessageRequest.h>
#include <aws/sqs/model/GetQueueAttributesRequest.h>
#include <aws/sqs/model/SetQueueAttributesRequest.h>
#include <aws/sqs/model/AddPermissionRequest.h>
#include <aws/sqs/model/RemovePermissionRequest.h>
#include <aws/sqs/model/ListDeadLetterSourceQueuesRequest.h>
#include <aws/s3/model/CreateBucketRequest.h>
#include <aws/s3/model/HeadBucketRequest.h>
#include <aws/s3/model/HeadObjectRequest.h>
#include <aws/core/utils/Outcome.h>
#include <aws/testing/ProxyConfig.h>
#include <aws/sqs/model/ChangeMessageVisibilityBatchRequestEntry.h>
#include <aws/sqs/model/ChangeMessageVisibilityBatchRequest.h>
#include <aws/sqs/model/SendMessageBatchRequestEntry.h>
#include <aws/sqs/model/SendMessageBatchRequest.h>
#include <aws/access-management/AccessManagementClient.h>
#include <aws/iam/IAMClient.h>
#include <aws/cognito-identity/CognitoIdentityClient.h>
#include <aws/core/utils/memory/stl/AWSStringStream.h>
#include <aws/core/utils/DateTime.h>

#include <aws/sqs/extendedlib/SQSExtendedClient.h>
#include <aws/sqs/extendedlib/SQSExtendedClientConfiguration.h>

using namespace Aws;
using namespace Aws::Http;
using namespace Aws::Auth;
using namespace Aws::Client;
using namespace Aws::S3;
using namespace Aws::S3::Model;
using namespace Aws::SQS;
using namespace Aws::SQS::Model;
using namespace Aws::SQS::ExtendedLib;
using namespace Aws::Utils::Json;

static const char* ALLOCATION_TAG = "ExtendedQueueOperationTest";

static const unsigned QUEUE_SIZE_LIMIT = 262144;
static const char* S3_BUCKET_NAME_MARKER = "-..s3BucketName..-";
static const char* S3_KEY_MARKER = "-..s3Key..-";

static const char* SMALLMESSAGE_WITHLARGEPAYLOADSUPPORTENABLED_BUCKET = "SmallMessageWithLargePayloadSupportEnabled";
static const char* SMALLMESSAGE_WITHLARGEPAYLOADSUPPORTENABLED_QUEUENAME = "SmallMessageWithLargePayloadSupportEnabled";
static const char* LARGEMESSAGE_WITHLARGEPAYLOADSUPPORTENABLED_BUCKET = "LargeMessageWithLargePayloadSupportEnabled";
static const char* LARGEMESSAGE_WITHLARGEPAYLOADSUPPORTENABLED_QUEUENAME = "LargeMessageWithLargePayloadSupportEnabled";

namespace
{

  class ExtendedQueueOperationTest : public ::testing::Test
  {

  public:
    std::shared_ptr<SQSClient> sqsStdClient;
    std::shared_ptr<S3Client> s3Client;
    Aws::String accountId;
    Aws::String timeStamp;

  protected:

    virtual void SetUp ()
    {
      ClientConfiguration stdConfig;
      stdConfig.scheme = Scheme::HTTPS;
      stdConfig.region = Region::US_EAST_1;

#if USE_PROXY_FOR_TESTS
      stdConfig.scheme = Scheme::HTTP;
      stdConfig.proxyHost = PROXY_HOST;
      stdConfig.proxyPort = PROXY_PORT;
#endif

      // get account_id
      auto cognitoClient = Aws::MakeShared<Aws::CognitoIdentity::CognitoIdentityClient>(ALLOCATION_TAG, stdConfig);
      auto iamClient = Aws::MakeShared<Aws::IAM::IAMClient>(ALLOCATION_TAG, stdConfig);
      Aws::AccessManagement::AccessManagementClient accessManagementClient(iamClient, cognitoClient);
      accountId = accessManagementClient.GetAccountId();

      // build standard sqsClient
      sqsStdClient = Aws::MakeShared<SQSClient> (ALLOCATION_TAG, Aws::MakeShared<DefaultAWSCredentialsProviderChain> (ALLOCATION_TAG), stdConfig);

      // build s3Client
      s3Client = Aws::MakeShared<S3Client> (
          ALLOCATION_TAG, Aws::MakeShared<DefaultAWSCredentialsProviderChain> (ALLOCATION_TAG), stdConfig, false);

      // TimeStamp to use in bucket names
      timeStamp = Utils::DateTime::Now().CalculateLocalTimestampAsString("%Y%m%dt%H%M%Sz");
    }

    virtual void TearDown ()
    {
      //DeleteAllTestQueues ();
      //DeleteAllBuckets();
      sqsStdClient = nullptr;
      s3Client = nullptr;
    }

    bool
    CreateBucket (const std::shared_ptr<S3Client> s3Client, const Aws::String bucketName)
    {
      CreateBucketRequest createBucketRequest;
      createBucketRequest.SetBucket(bucketName);
      createBucketRequest.SetACL(BucketCannedACL::private_);

      CreateBucketOutcome createBucketOutcome = s3Client->CreateBucket(createBucketRequest);
      if (!createBucketOutcome.IsSuccess())
        return false;
      const CreateBucketResult& createBucketResult = createBucketOutcome.GetResult();
      if (createBucketResult.GetLocation().empty())
        return false;

      return WaitForBucketToPropagate(s3Client, bucketName);
    }

    Aws::String
    CreateQueue (const std::shared_ptr<SQSClient> sqsClient, const Aws::String queueName)
    {
      CreateQueueRequest request;
      request.SetQueueName (queueName);
      request.AddAttributes (QueueAttributeName::MaximumMessageSize, std::to_string (QUEUE_SIZE_LIMIT).c_str ());

      bool shouldContinue = true;
      while (shouldContinue)
        {
          CreateQueueOutcome outcome = sqsClient->CreateQueue (request);

          if (outcome.IsSuccess ())
            {
              return outcome.GetResult ().GetQueueUrl ();
            }
          if (outcome.GetError ().GetErrorType () != SQSErrors::QUEUE_DELETED_RECENTLY)
            {
              return "";
            }
          std::this_thread::sleep_for (std::chrono::seconds (10));
        }

      return "";
    }

    DeleteQueueOutcome
    DeleteQueue (const std::shared_ptr<SQSClient> sqsClient, const Aws::String queueUrl)
    {
      DeleteQueueRequest deleteQueueRequest;
      deleteQueueRequest.SetQueueUrl (queueUrl);
      DeleteQueueOutcome deleteQueueOutcome = sqsClient->DeleteQueue (deleteQueueRequest);

      bool done = false;
      while (!done)
        {
          if (deleteQueueOutcome.IsSuccess ())
            {
              break;
            }
          std::this_thread::sleep_for (std::chrono::seconds (1));
        }
      return deleteQueueOutcome;
    }

    SendMessageOutcome
    SendMessage (const std::shared_ptr<SQSClient> sqsClient, const Aws::String queueUrl, const Aws::String messageBody)
    {
      SendMessageRequest sendMessageRequest;
      sendMessageRequest.SetQueueUrl (queueUrl);
      sendMessageRequest.SetMessageBody (messageBody);
      return sqsClient->SendMessage (sendMessageRequest);
    }

    ReceiveMessageOutcome
    ReceiveMessage (const std::shared_ptr<SQSClient> sqsClient, const Aws::String queueUrl)
    {
      ReceiveMessageRequest receiveMessageRequest;
      receiveMessageRequest.SetMaxNumberOfMessages (1);
      receiveMessageRequest.SetQueueUrl (queueUrl);
      return sqsClient->ReceiveMessage (receiveMessageRequest);
    }

    DeleteMessageOutcome
    DeleteMessage (const std::shared_ptr<SQSClient> sqsClient, const Aws::String queueUrl, const Aws::String receiptHandle)
    {
      DeleteMessageRequest deleteMessageRequest;
      deleteMessageRequest.SetQueueUrl (queueUrl);
      deleteMessageRequest.SetReceiptHandle (receiptHandle);
      return sqsClient->DeleteMessage (deleteMessageRequest);
    }

    bool WaitForBucketToPropagate(const std::shared_ptr<S3Client> s3Client, const Aws::String& bucketName)
    {
      unsigned timeoutCount = 0;
      while (timeoutCount++ < 10)
      {
          HeadBucketRequest headBucketRequest;
          headBucketRequest.SetBucket(bucketName);
          HeadBucketOutcome headBucketOutcome = s3Client->HeadBucket(headBucketRequest);
          if (headBucketOutcome.IsSuccess())
          {
              return true;
          }
          std::this_thread::sleep_for(std::chrono::seconds(1));
      }
      return false;
    }

    Aws::String
    GenerateMessageBody (const unsigned messageLength)
    {
      return Aws::String (messageLength, 'x');
    }

    Aws::String
    GetFromReceiptHandleByMarker(const Aws::String receiptHandle, const Aws::String marker) {
      int firstOccurence = receiptHandle.find(marker);
      int secondOccurence = receiptHandle.find(marker, firstOccurence + 1);
      return receiptHandle.substr(firstOccurence + marker.length(), secondOccurence);
    }

  };
} // anonymous namespace

TEST_F(ExtendedQueueOperationTest, TestSmallMessageWithLargePayloadSupportEnabled)
{
  // build a bucket, an extended sqs config, an extended sqs client a queue and message body
  Aws::String s3BucketName = accountId + "_" + SMALLMESSAGE_WITHLARGEPAYLOADSUPPORTENABLED_BUCKET + "_" + timeStamp;
  CreateBucket(s3Client, s3BucketName);

  auto sqsConfig = Aws::MakeShared<SQSExtendedClientConfiguration> (ALLOCATION_TAG);
  sqsConfig->SetLargePayloadSupportEnabled(s3Client, s3BucketName);

  std::shared_ptr<SQSClient> sqsClient = Aws::MakeShared<SQSExtendedClient> (ALLOCATION_TAG, sqsStdClient, sqsConfig);

  Aws::String queueUrl = CreateQueue (sqsClient, SMALLMESSAGE_WITHLARGEPAYLOADSUPPORTENABLED_QUEUENAME);

  // prepare a small payload
  Aws::String messageBody = ExtendedQueueOperationTest::GenerateMessageBody (QUEUE_SIZE_LIMIT - 1000);

  // send message
  SendMessageOutcome sendM = ExtendedQueueOperationTest::SendMessage (sqsClient, queueUrl, messageBody);
  ASSERT_TRUE(sendM.IsSuccess ());
  EXPECT_TRUE(sendM.GetResult ().GetMessageId ().length () > 0);

  // receive message
  ReceiveMessageOutcome receiveM = ExtendedQueueOperationTest::ReceiveMessage (sqsClient, queueUrl);
  ASSERT_TRUE(receiveM.IsSuccess ());
  ASSERT_EQ(1uL, receiveM.GetResult ().GetMessages ().size ());
  EXPECT_EQ(messageBody, receiveM.GetResult ().GetMessages ()[0].GetBody ());

  // check if s3 was used. It must not!
  Aws::String receiptHandle = receiveM.GetResult ().GetMessages ()[0].GetReceiptHandle ();
  ASSERT_FALSE(receiptHandle.find (S3_BUCKET_NAME_MARKER) != std::string::npos);
  ASSERT_FALSE(receiptHandle.find (S3_KEY_MARKER) != std::string::npos);

  // delete message
  DeleteMessageOutcome deleteM = ExtendedQueueOperationTest::DeleteMessage (sqsClient, queueUrl, receiptHandle);
  ASSERT_TRUE(deleteM.IsSuccess ());
  receiveM = ExtendedQueueOperationTest::ReceiveMessage (sqsClient, queueUrl);
  EXPECT_EQ(0uL, receiveM.GetResult ().GetMessages ().size ());

  // delete queue
  DeleteQueueOutcome deleteQ = DeleteQueue(sqsClient, queueUrl);
  ASSERT_TRUE(deleteQ.IsSuccess());
}

TEST_F(ExtendedQueueOperationTest, TestLargeMessageWithLargePayloadSupportEnabled)
{
  // build a bucket, an extended sqs config, an extended sqs client a queue and message body
  Aws::String s3BucketName = accountId + "_" + LARGEMESSAGE_WITHLARGEPAYLOADSUPPORTENABLED_BUCKET + "_" + timeStamp;
  CreateBucket(s3Client, s3BucketName);

  auto sqsConfig = Aws::MakeShared<SQSExtendedClientConfiguration> (ALLOCATION_TAG);
  sqsConfig->SetLargePayloadSupportEnabled(s3Client, s3BucketName);

  std::shared_ptr<SQSClient> sqsClient = Aws::MakeShared<SQSExtendedClient> (ALLOCATION_TAG, sqsStdClient, sqsConfig);

  Aws::String queueUrl = CreateQueue (sqsClient, LARGEMESSAGE_WITHLARGEPAYLOADSUPPORTENABLED_QUEUENAME);

  // prepare a small payload
  Aws::String messageBody = ExtendedQueueOperationTest::GenerateMessageBody (QUEUE_SIZE_LIMIT + 1000);

  // send message
  SendMessageOutcome sendM = ExtendedQueueOperationTest::SendMessage (sqsClient, queueUrl, messageBody);
  ASSERT_TRUE(sendM.IsSuccess ());
  EXPECT_TRUE(sendM.GetResult ().GetMessageId ().length () > 0);

  // receive message
  ReceiveMessageOutcome receiveM = ExtendedQueueOperationTest::ReceiveMessage (sqsClient, queueUrl);
  ASSERT_TRUE(receiveM.IsSuccess ());
  ASSERT_EQ(1uL, receiveM.GetResult ().GetMessages ().size ());
  EXPECT_EQ(messageBody, receiveM.GetResult ().GetMessages ()[0].GetBody ());

  // check if s3 was used. It must!
  Aws::String receiptHandle = receiveM.GetResult ().GetMessages ()[0].GetReceiptHandle ();
  ASSERT_TRUE(receiptHandle.find (S3_BUCKET_NAME_MARKER) != std::string::npos);
  ASSERT_TRUE(receiptHandle.find (S3_KEY_MARKER) != std::string::npos);

  // delete message
  DeleteMessageOutcome deleteM = ExtendedQueueOperationTest::DeleteMessage (sqsClient, queueUrl, receiptHandle);
  ASSERT_TRUE(deleteM.IsSuccess ());
  receiveM = ExtendedQueueOperationTest::ReceiveMessage (sqsClient, queueUrl);
  EXPECT_EQ(0uL, receiveM.GetResult ().GetMessages ().size ());

  // check if s3key was removed
  Aws::String s3BucketNameToTest = ExtendedQueueOperationTest::GetFromReceiptHandleByMarker(receiptHandle, S3_BUCKET_NAME_MARKER);
  Aws::String s3KeyToTest = ExtendedQueueOperationTest::GetFromReceiptHandleByMarker(receiptHandle, S3_KEY_MARKER);
  HeadObjectRequest headObjectRequest;
  headObjectRequest.SetBucket(s3BucketNameToTest);
  headObjectRequest.SetKey(s3KeyToTest);
  HeadObjectOutcome headObjectOutcome = s3Client->HeadObject(headObjectRequest);
  ASSERT_FALSE(headObjectOutcome.IsSuccess());

  // delete queue
  DeleteQueueOutcome deleteQ = DeleteQueue(sqsClient, queueUrl);
  ASSERT_TRUE(deleteQ.IsSuccess());
}

/*
TEST_F(ExtendedQueueOperationTest, TestSmallMessageWithLargePayloadSupportDisabled)
{
}

TEST_F(ExtendedQueueOperationTest, TestLargeMessageWithLargePayloadSupportDisabled)
{
  Aws::String messageBody = ExtendedQueueOperationTest::GenerateMessageBody (QUEUE_SIZE_LIMIT + 1000);

  // TODO: turnoff payload support

  SendMessageOutcome send = ExtendedQueueOperationTest::SendMessage (messageBody);
  SQSErrors error = send.GetError ().GetErrorType ();
  EXPECT_TRUE(SQSErrors::INVALID_PARAMETER_VALUE == error);

  // TODO: see if s3bucket was used, it must not!

  Aws::String errorMessage = send.GetError ().GetMessage ();
  EXPECT_TRUE(errorMessage.find (std::to_string (QUEUE_SIZE_LIMIT).c_str ()) != std::string::npos);
  // GetMessage(): One or more parameters are invalid. Reason: Message must be shorter than 262144 bytes.
}

TEST_F(ExtendedQueueOperationTest, TestSendReceiveSmallMessageWithAlwaysThroughS3Enabled)
{
}

TEST_F(ExtendedQueueOperationTest, TestSendReceiveLargeMessageWithAlwaysThroughS3Enabled)
{
}

TEST_F(ExtendedQueueOperationTest, TestSendReceiveBatchMessagesWithRamdomPayloadSizeAndLargePayloadSupportEnabled)
{
}
*/
