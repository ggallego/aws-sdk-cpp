cmake_minimum_required(VERSION 2.6)
project(aws-cpp-sdk-support)

  file(GLOB AWS_SUPPORT_HEADERS
    "include/aws/support/*.h"
  )

  file(GLOB AWS_SUPPORT_MODEL_HEADERS
    "include/aws/support/model/*.h"
  )

  file(GLOB AWS_SUPPORT_SOURCE
    "source/*.cpp"
  )

  file(GLOB AWS_SUPPORT_MODEL_SOURCE
    "source/model/*.cpp"
  )

  file(GLOB SUPPORT_UNIFIED_HEADERS
    ${AWS_SUPPORT_HEADERS}
    ${AWS_SUPPORT_MODEL_HEADERS}
  )

  file(GLOB SUPPORT_UNITY_SRC
    ${AWS_SUPPORT_SOURCE}
    ${AWS_SUPPORT_MODEL_SOURCE}
  )

  if(ENABLE_UNITY_BUILD)
    enable_unity_build("SUPPORT" SUPPORT_UNITY_SRC)
  endif()

  file(GLOB SUPPORT_SRC
    ${SUPPORT_UNIFIED_HEADERS}
    ${SUPPORT_UNITY_SRC}
  )

  if(WIN32)
    #if we are compiling for visual studio, create a sane directory tree.
    if(MSVC)
      source_group("Header Files\\aws\\support" FILES ${AWS_SUPPORT_HEADERS})
      source_group("Header Files\\aws\\support\\model" FILES ${AWS_SUPPORT_MODEL_HEADERS})
      source_group("Source Files" FILES ${AWS_SUPPORT_SOURCE})
      source_group("Source Files\\model" FILES ${AWS_SUPPORT_MODEL_SOURCE})
    endif(MSVC)
  endif()

  set(SUPPORT_INCLUDES
    "${CMAKE_CURRENT_SOURCE_DIR}/include/"
    "${CORE_DIR}/include/"
  )

  include_directories(${SUPPORT_INCLUDES})

  if(MSVC AND BUILD_SHARED_LIBS)
    add_definitions("-DAWS_SUPPORT_EXPORTS")
  endif()

  add_library(aws-cpp-sdk-support ${LIBTYPE} ${SUPPORT_SRC})

  target_include_directories(aws-cpp-sdk-support PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>)
  
  target_link_libraries(aws-cpp-sdk-support aws-cpp-sdk-core)

  if(SIMPLE_INSTALL)
      install (TARGETS aws-cpp-sdk-support
               ARCHIVE DESTINATION ${ARCHIVE_DIRECTORY}
               LIBRARY DESTINATION lib
               RUNTIME DESTINATION bin)
  else()
      install (TARGETS aws-cpp-sdk-support
               ARCHIVE DESTINATION ${ARCHIVE_DIRECTORY}/${SDK_INSTALL_BINARY_PREFIX}/${PLATFORM_INSTALL_QUALIFIER}/\${CMAKE_INSTALL_CONFIG_NAME}
               LIBRARY DESTINATION lib/${SDK_INSTALL_BINARY_PREFIX}/${PLATFORM_INSTALL_QUALIFIER}/\${CMAKE_INSTALL_CONFIG_NAME}
               RUNTIME DESTINATION bin/${SDK_INSTALL_BINARY_PREFIX}/${PLATFORM_INSTALL_QUALIFIER}/\${CMAKE_INSTALL_CONFIG_NAME})
  endif()

  install (FILES ${AWS_SUPPORT_HEADERS} DESTINATION include/aws/support)
  install (FILES ${AWS_SUPPORT_MODEL_HEADERS} DESTINATION include/aws/support/model)

  if(MSVC)
    install (FILES nuget/aws-cpp-sdk-support.autopkg DESTINATION nuget)
  endif()



