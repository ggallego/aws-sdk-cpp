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

#pragma once
#include <aws/sqs/SQS_EXPORTS.h>
#include <aws/sqs/extendedlib/SQSExtendedClientConfiguration.h>
#include <aws/sqs/model/MessageAttributeValue.h>
#include <aws/s3/S3Client.h>
#include <aws/sqs/SQSClient.h>

namespace Aws
{
  namespace SQS
  {
    namespace ExtendedLib
    {

      class AWS_SQS_API SQSExtendedClient : public SQSClient
      {

      private:
        std::shared_ptr<SQS::SQSClient> sqsClient;
        std::shared_ptr<SQSExtendedClientConfiguration> sqsConfig;

        virtual bool IsLargeMessage (const Model::SendMessageRequest& request) const;
        virtual unsigned GetMsgAttributesSize(const Aws::Map<Aws::String, Model::MessageAttributeValue>& messageAttributes) const;
        virtual Model::SendMessageRequest StoreMessageInS3 (const Model::SendMessageRequest& request) const;

      public:
        SQSExtendedClient (const std::shared_ptr<SQSClient>& client,
            const std::shared_ptr<SQSExtendedClientConfiguration>& config);

        virtual Model::SendMessageOutcome SendMessage (const Model::SendMessageRequest& request) const;
        virtual Model::ReceiveMessageOutcome ReceiveMessage(const Model::ReceiveMessageRequest& request) const;
        virtual Model::DeleteMessageOutcome DeleteMessage(const Model::DeleteMessageRequest& request) const;

      };

    } // namespace extendedLib
  } // namespace SQS
} // namespace Aws

