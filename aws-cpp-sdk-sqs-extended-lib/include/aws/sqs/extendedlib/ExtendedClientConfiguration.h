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

#include <aws/core/utils/memory/stl/AWSString.h>
#include <aws/s3/S3Client.h>

namespace Aws
{
namespace SQS
{
namespace ExtendedLib
{
	struct AWS_CORE_API ExtendedClientConfiguration
	{
		ExtendedClientConfiguration();

		Aws::S3::S3Client s3;

		Aws::String s3BucketName;

		bool largePayloadSupport;

		bool alwaysThroughS3;

		int messageSizeThreshold;

	};

} // namespace extendedLib
} // namespace SQS
} // namespace Aws


