cmake_minimum_required(VERSION 2.6)
project(aws-cpp-sdk-sqs-extended-lib-integration-tests)

# Headers are included in the source so that they show up in Visual Studio.
# They are included elsewhere for consistency.

  file(GLOB AWS_SQS_EXTENDED_LIB_SRC
  "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp"
  )

  file(GLOB AWS_SQS_EXTENDED_LIB_INTEGRATION_TESTS_SRC
    ${AWS_SQS_EXTENDED_LIB_SRC}
  )

set(AWS_SQS_EXTENDED_LIB_INTEGRATION_TEST_APPLICATION_INCLUDES
  "${AWS_NATIVE_SDK_ROOT}/aws-cpp-sdk-core/include/"
  "${AWS_NATIVE_SDK_ROOT}/aws-cpp-sdk-sqs-extended-lib/include/"
  "${AWS_NATIVE_SDK_ROOT}/testing-resources/include/"
)

include_directories(${AWS_SQS_EXTENDED_LIB_INTEGRATION_TEST_APPLICATION_INCLUDES})

if(MSVC AND BUILD_SHARED_LIBS)
    add_definitions(-DGTEST_LINKED_AS_SHARED_LIBRARY=1)
endif()

enable_testing()

if(PLATFORM_ANDROID AND BUILD_SHARED_LIBS)
    add_library(runSqsExtendedLibIntegrationTests ${LIBTYPE} ${AWS_SQS_EXTENDED_LIB_INTEGRATION_TESTS_SRC})
else()
    add_executable(runSqsExtendedLibIntegrationTests ${AWS_SQS_EXTENDED_LIB_INTEGRATION_TESTS_SRC})
endif()

target_link_libraries(runSqsExtendedLibIntegrationTests aws-cpp-sdk-core aws-cpp-sdk-sqs-extended-lib testing-resources)
copyDlls(runSQSExtendedLibIntegrationTests aws-cpp-sdk-core aws-cpp-sdk-sqs-extended-lib testing-resources)
