/*
 * Copyright 2010-2016 Amazon.com, Inc. or its affiliates. All Rights Reserved.
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

#include <aws/core/utils/Outcome.h>
#include <aws/core/auth/AWSAuthSigner.h>
#include <aws/core/client/CoreErrors.h>
#include <aws/core/client/RetryStrategy.h>
#include <aws/core/http/HttpClient.h>
#include <aws/core/http/HttpResponse.h>
#include <aws/core/http/HttpClientFactory.h>
#include <aws/core/auth/AWSCredentialsProviderChain.h>
#include <aws/core/utils/xml/XmlSerializer.h>
#include <aws/core/utils/memory/stl/AWSStringStream.h>
#include <aws/core/utils/threading/Executor.h>
#include <aws/sqs/SQSClient.h>
#include <aws/sqs/SQSEndpoint.h>
#include <aws/sqs/SQSErrorMarshaller.h>
#include <aws/sqs/model/AddPermissionRequest.h>
#include <aws/sqs/model/ChangeMessageVisibilityRequest.h>
#include <aws/sqs/model/ChangeMessageVisibilityBatchRequest.h>
#include <aws/sqs/model/CreateQueueRequest.h>
#include <aws/sqs/model/DeleteMessageRequest.h>
#include <aws/sqs/model/DeleteMessageBatchRequest.h>
#include <aws/sqs/model/DeleteQueueRequest.h>
#include <aws/sqs/model/GetQueueAttributesRequest.h>
#include <aws/sqs/model/GetQueueUrlRequest.h>
#include <aws/sqs/model/ListDeadLetterSourceQueuesRequest.h>
#include <aws/sqs/model/ListQueuesRequest.h>
#include <aws/sqs/model/PurgeQueueRequest.h>
#include <aws/sqs/model/ReceiveMessageRequest.h>
#include <aws/sqs/model/RemovePermissionRequest.h>
#include <aws/sqs/model/SendMessageRequest.h>
#include <aws/sqs/model/SendMessageBatchRequest.h>
#include <aws/sqs/model/SetQueueAttributesRequest.h>
#include <aws/sqs/model/MessageAttributeValue.h>
#include <aws/core/utils/DateTime.h>

#include <aws/sqs/extendedlib/SQSExtendedClient.h>
#include <aws/sqs/extendedlib/SQSExtendedClientConfiguration.h>
#include <aws/sqs/extendedlib/SQSLargeMessageS3Pointer.h>
#include <aws/s3/model/PutObjectRequest.h>
#include <aws/core/utils/json/JsonSerializer.h>

using namespace Aws;
using namespace Aws::Auth;
using namespace Aws::Client;
using namespace Aws::S3;
using namespace Aws::S3::Model;
using namespace Aws::SQS;
using namespace Aws::SQS::Model;
using namespace Aws::SQS::ExtendedLib;
using namespace Aws::Http;
using namespace Aws::Utils::Xml;
using namespace Aws::Utils::Json;

//static const char* SERVICE_NAME = "sqs";
static const char* ALLOCATION_TAG = "SQSExtendedClient";
static const char* RESERVED_ATTRIBUTE_NAME = "SQSLargePayloadSize";

SQSExtendedClient::SQSExtendedClient (const std::shared_ptr<SQSClient>& client,
    const std::shared_ptr<SQSExtendedClientConfiguration>& config) :
    sqsClient (client), sqsConfig (config)
{
}

SendMessageOutcome
SQSExtendedClient::SendMessage (const SendMessageRequest& request) const
{
  //request.GetRequestClientOptions ().AppendUserAgent (USER_AGENT_HEADER);

  if (!sqsConfig->IsLargePayloadSupportEnabled ())
    return SQSClient::SendMessage (request);

  if (sqsConfig->IsAlwaysThroughS3 () || SQSExtendedClient::IsLargeMessage (request)) {
    SendMessageRequest reqWithS3Support = SQSExtendedClient::StoreMessageInS3 (request);
    return SQSClient::SendMessage (reqWithS3Support);
  }

  return SQSClient::SendMessage (request);
}

bool
SQSExtendedClient::IsLargeMessage (const SendMessageRequest& request) const
{
  unsigned msgAttributesSize = SQSExtendedClient::GetMsgAttributesSize (request.GetMessageAttributes ());
  unsigned msgBodySize = request.GetMessageBody ().size ();
  unsigned totalMsgSize = msgAttributesSize + msgBodySize;
  return (totalMsgSize > sqsConfig->GetMessageSizeThreshold ());
}

SendMessageRequest SQSExtendedClient::StoreMessageInS3 (const SendMessageRequest& request) const
{
  SendMessageRequest reqWithS3Support = request;

  // UUID.randomUUID ().toString ();
  Aws::String s3Key = RESERVED_ATTRIBUTE_NAME + Utils::DateTime::Now().CalculateLocalTimestampAsString("%Y%m%dt%H%M%Sz");
  Aws::String body = reqWithS3Support.GetMessageBody ();
  unsigned size = body.size ();
  std::shared_ptr<Aws::IOStream> bodyAsStream = Aws::MakeShared<Aws::StringStream>(ALLOCATION_TAG);
  *bodyAsStream << body;
  bodyAsStream->flush();

  // Add message attribute as a flag
  MessageAttributeValue messageAttributeValue;
  messageAttributeValue.SetDataType ("Number");
  messageAttributeValue.SetStringValue (std::to_string (size).c_str ());
  reqWithS3Support.AddMessageAttributes (RESERVED_ATTRIBUTE_NAME, messageAttributeValue);

  // Upload payload to S3
  PutObjectRequest putObjectRequest;
  putObjectRequest.SetBucket(sqsConfig->GetS3BucketName());
  putObjectRequest.SetKey(s3Key);
  putObjectRequest.SetBody(bodyAsStream);
  putObjectRequest.SetContentLength(static_cast<long>(putObjectRequest.GetBody()->tellp()));
  PutObjectOutcome putObjectOutcome = sqsConfig->GetS3Client()->PutObject(putObjectRequest);

  // Get S3 Handler/Pointer
  SQSLargeMessageS3Pointer s3Pointer;
  s3Pointer.SetS3BucketName(sqsConfig->GetS3BucketName());
  s3Pointer.SetS3Key(s3Key);
  JsonValue json = s3Pointer.Jsonize();
  reqWithS3Support.SetMessageBody (json.WriteReadable());

  return reqWithS3Support;
}

unsigned
SQSExtendedClient::GetMsgAttributesSize (
    const Aws::Map<Aws::String, Model::MessageAttributeValue>& messageAttributes) const
{
  unsigned size = 0;

  std::cout << "I haz attritubes ???" << std::endl;

  for (auto each : messageAttributes)
    {
      Aws::String key = each.first;
      Model::MessageAttributeValue value = each.second;

      std::cout << "I haz attritubes: " << key << std::endl;

      // TODO: Test this with care
      size += key.size ();
      size += value.GetDataType ().size ();
      size += value.GetStringValue ().size ();
      size += value.GetBinaryValue ().GetLength ();
    }

  return size;
}

