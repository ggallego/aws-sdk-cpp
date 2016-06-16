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
#include <aws/core/utils/Outcome.h>
#include <aws/testing/ProxyConfig.h>
#include <aws/sqs/model/ChangeMessageVisibilityBatchRequestEntry.h>
#include <aws/sqs/model/ChangeMessageVisibilityBatchRequest.h>
#include <aws/sqs/model/SendMessageBatchRequestEntry.h>
#include <aws/sqs/model/SendMessageBatchRequest.h>
#include <aws/core/utils/memory/stl/AWSStringStream.h>

#include <aws/sqs/extendedlib/SQSExtendedClient.h>
#include "../aws-cpp-sdk-sqs-extended-lib/include/aws/sqs/extendedlib/SQSExtendedClientConfiguration.h"

using namespace Aws::Http;
using namespace Aws;
using namespace Aws::Auth;
using namespace Aws::Client;
using namespace Aws::SQS;
using namespace Aws::SQS::Model;
using namespace Aws::SQS::ExtendedLib;
using namespace Aws::Utils::Json;

static const char* QUEUE_NAME = "EQ_IntegrationTest_Queue";
static const int QUEUE_SIZE_LIMIT = 262144;
static const char* BUCKET_NAME = "EQ_IntegrationTest_Bucket";
static const char* ALLOCATION_TAG = "ExtendedQueueOperationTest";

namespace
{

  class ExtendedQueueOperationTest : public ::testing::Test
  {

  public:
    std::shared_ptr<SQSExtendedClient> sqsClient;
    Aws::String queueUrl;

  protected:
    virtual void
    SetUp ()
    {
      ClientConfiguration stdConfig;
      stdConfig.scheme = Scheme::HTTPS;
      stdConfig.region = Region::US_EAST_1;
#if USE_PROXY_FOR_TESTS
      stdConfig.scheme = Scheme::HTTP;
      stdConfig.proxyHost = PROXY_HOST;
      stdConfig.proxyPort = PROXY_PORT;
#endif
      std::shared_ptr<SQSClient> sqsStdClient = Aws::MakeShared<SQSClient> (
	  ALLOCATION_TAG,
	  Aws::MakeShared<DefaultAWSCredentialsProviderChain> (ALLOCATION_TAG),
	  stdConfig);

      std::shared_ptr<SQSExtendedClientConfiguration> sqsConfig =
	  Aws::MakeShared<SQSExtendedClientConfiguration> (ALLOCATION_TAG);
      sqsConfig->s3BucketName = BUCKET_NAME;
      sqsClient = Aws::MakeShared<SQSExtendedClient> (ALLOCATION_TAG,
						      sqsStdClient, sqsConfig);

      queueUrl = CreateQueue (QUEUE_NAME);
    }

    virtual void
    TearDown ()
    {
      DeleteQueue ();
      sqsClient = nullptr;
    }

    Aws::String
    CreateQueue (Aws::String name)
    {
      CreateQueueRequest request;
      request.SetQueueName (name);
      request.AddAttributes (QueueAttributeName::MaximumMessageSize,
			     std::to_string (QUEUE_SIZE_LIMIT).c_str ());

      bool shouldContinue = true;
      while (shouldContinue)
	{
	  CreateQueueOutcome outcome = sqsClient->CreateQueue (request);

	  if (outcome.IsSuccess ())
	    {
	      return outcome.GetResult ().GetQueueUrl ();
	    }
	  if (outcome.GetError ().GetErrorType ()
	      != SQSErrors::QUEUE_DELETED_RECENTLY)
	    {
	      return "";
	    }
	  std::this_thread::sleep_for (std::chrono::seconds (10));
	}

      return "";
    }

    void
    DeleteQueue ()
    {
      DeleteQueueRequest deleteQueueRequest;
      deleteQueueRequest.WithQueueUrl (queueUrl);
      DeleteQueueOutcome deleteQueueOutcome = sqsClient->DeleteQueue (
	  deleteQueueRequest);

      bool done = false;
      while (!done)
	{
	  if (deleteQueueOutcome.IsSuccess ())
	    {
	      break;
	    }
	  std::this_thread::sleep_for (std::chrono::seconds (1));
	}

    }

    static Aws::String
    GenerateMessageBody (const int messageLength)
    {
      return Aws::String (messageLength, 'x');
    }

  };
} // anonymous namespace

TEST_F(ExtendedQueueOperationTest, TestSendReceiveSmallMessageWithLargePayloadSupportEnabled)
{
}

TEST_F(ExtendedQueueOperationTest, TestSendReceiveLargeMessageWithLargePayloadSupportEnabled)
{
}

TEST_F(ExtendedQueueOperationTest, TestSendReceiveSmallMessageWithLargePayloadSupportDisabled)
{
  Aws::String messageBody = ExtendedQueueOperationTest::GenerateMessageBody (
      QUEUE_SIZE_LIMIT - 10);

  SendMessageRequest sendMessageRequest;
  sendMessageRequest.SetMessageBody (messageBody);
  sendMessageRequest.SetQueueUrl (queueUrl);

  SendMessageOutcome sendMessageOutcome = sqsClient->SendMessage (
      sendMessageRequest);

  ASSERT_TRUE(sendMessageOutcome.IsSuccess ());
  EXPECT_TRUE(sendMessageOutcome.GetResult ().GetMessageId ().length () > 0);

  ReceiveMessageRequest receiveMessageRequest;
  receiveMessageRequest.SetMaxNumberOfMessages (1);
  receiveMessageRequest.SetQueueUrl (queueUrl);

  ReceiveMessageOutcome receiveMessageOutcome = sqsClient->ReceiveMessage (
      receiveMessageRequest);
  ASSERT_TRUE(receiveMessageOutcome.IsSuccess ());
  ReceiveMessageResult receiveMessageResult =
      receiveMessageOutcome.GetResult ();
  ASSERT_EQ(1uL, receiveMessageResult.GetMessages ().size ());
  EXPECT_EQ(messageBody, receiveMessageResult.GetMessages ()[0].GetBody ());

  DeleteMessageRequest deleteMessageRequest;
  deleteMessageRequest.SetQueueUrl (queueUrl);
  deleteMessageRequest.SetReceiptHandle (
      receiveMessageResult.GetMessages ()[0].GetReceiptHandle ());

  DeleteMessageOutcome deleteMessageOutcome = sqsClient->DeleteMessage (
      deleteMessageRequest);
  ASSERT_TRUE(deleteMessageOutcome.IsSuccess ());

  receiveMessageOutcome = sqsClient->ReceiveMessage (receiveMessageRequest);
  EXPECT_EQ(0uL, receiveMessageOutcome.GetResult ().GetMessages ().size ());
}

TEST_F(ExtendedQueueOperationTest, TestSendReceiveLargeMessageWithLargePayloadSupportDisabled)
{
  Aws::String messageBody = ExtendedQueueOperationTest::GenerateMessageBody (
      QUEUE_SIZE_LIMIT + 10);

  SendMessageRequest sendMessageRequest;
  sendMessageRequest.SetMessageBody (messageBody);
  sendMessageRequest.SetQueueUrl (queueUrl);

  SendMessageOutcome sendMessageOutcome = sqsClient->SendMessage (
      sendMessageRequest);

  SQSErrors error = sendMessageOutcome.GetError ().GetErrorType ();
  EXPECT_TRUE(SQSErrors::INVALID_PARAMETER_VALUE == error);

  Aws::String errorMessage = sendMessageOutcome.GetError ().GetMessage ();
  EXPECT_TRUE(
      errorMessage.find (std::to_string (QUEUE_SIZE_LIMIT).c_str ())
	  != std::string::npos);
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

