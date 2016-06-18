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

#include <aws/core/utils/memory/stl/AWSStringStream.h>
#include <aws/sqs/extendedlib/SQSExtendedClientConfiguration.h>

using namespace Aws::SQS::ExtendedLib;

SQSExtendedClientConfiguration::SQSExtendedClientConfiguration () :
  s3Client(nullptr),
  s3BucketName("bucket"),
  messageSizeThreshold(262144),
  largePayloadSupport(true),
  alwaysThroughS3(false)
{
}


void SQSExtendedClientConfiguration::SetLargePayloadSupportEnabled (const std::shared_ptr<Aws::S3::S3Client> _s3Client, const Aws::String _s3BucketName)
{
  s3Client = _s3Client;
  s3BucketName = _s3BucketName;
  largePayloadSupport = true;
}

void SQSExtendedClientConfiguration::SetLargePayloadSupportDisabled ()
{
  s3Client = nullptr;
  s3BucketName = "";
  largePayloadSupport = false;
}

bool SQSExtendedClientConfiguration::IsLargePayloadSupportEnabled() const { return largePayloadSupport; }

// ---

void SQSExtendedClientConfiguration::SetAlwaysThroughS3Enabled () { alwaysThroughS3 = true; }

void SQSExtendedClientConfiguration::SetAlwaysThroughS3Disabled () { alwaysThroughS3 = false; }

bool SQSExtendedClientConfiguration::IsAlwaysThroughS3 () const { return alwaysThroughS3; }

// ---

std::shared_ptr<Aws::S3::S3Client> SQSExtendedClientConfiguration::GetS3Client () const { return s3Client; }

Aws::String SQSExtendedClientConfiguration::GetS3BucketName () const { return s3BucketName; }

unsigned SQSExtendedClientConfiguration::GetMessageSizeThreshold () const { return messageSizeThreshold; }

