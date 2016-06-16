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
  largePayloadSupport(true),
  alwaysThroughS3(false),
  messageSizeThreshold(262144)
{
}

SQSExtendedClientConfiguration::~SQSExtendedClientConfiguration ()
{
}

void SQSExtendedClientConfiguration::SetLargePayloadSupportEnabled () const
{
}
void SQSExtendedClientConfiguration::SetLargePayloadSupportDisabled () const
{
}

void SQSExtendedClientConfiguration::SetAlwaysThroughS3Enabled () const
{
}

void SQSExtendedClientConfiguration::SetAlwaysThroughS3Disabled () const
{
}
