# Install script for directory: /Users/tunm/work/InspireFace/cpp/sample

# Set the install prefix
if(NOT DEFINED CMAKE_INSTALL_PREFIX)
  set(CMAKE_INSTALL_PREFIX "/Users/tunm/work/InspireFace/build_test/install")
endif()
string(REGEX REPLACE "/$" "" CMAKE_INSTALL_PREFIX "${CMAKE_INSTALL_PREFIX}")

# Set the install configuration name.
if(NOT DEFINED CMAKE_INSTALL_CONFIG_NAME)
  if(BUILD_TYPE)
    string(REGEX REPLACE "^[^A-Za-z0-9_]+" ""
           CMAKE_INSTALL_CONFIG_NAME "${BUILD_TYPE}")
  else()
    set(CMAKE_INSTALL_CONFIG_NAME "")
  endif()
  message(STATUS "Install configuration: \"${CMAKE_INSTALL_CONFIG_NAME}\"")
endif()

# Set the component getting installed.
if(NOT CMAKE_INSTALL_COMPONENT)
  if(COMPONENT)
    message(STATUS "Install component: \"${COMPONENT}\"")
    set(CMAKE_INSTALL_COMPONENT "${COMPONENT}")
  else()
    set(CMAKE_INSTALL_COMPONENT)
  endif()
endif()

# Is this installation the result of a crosscompile?
if(NOT DEFINED CMAKE_CROSSCOMPILING)
  set(CMAKE_CROSSCOMPILING "FALSE")
endif()

# Set path to fallback-tool for dependency-resolution.
if(NOT DEFINED CMAKE_OBJDUMP)
  set(CMAKE_OBJDUMP "/Applications/Xcode.app/Contents/Developer/Toolchains/XcodeDefault.xctoolchain/usr/bin/objdump")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  list(APPEND CMAKE_ABSOLUTE_DESTINATION_FILES
   "/Users/tunm/work/InspireFace/build_test/install/sample/Leak")
  if(CMAKE_WARN_ON_ABSOLUTE_INSTALL_DESTINATION)
    message(WARNING "ABSOLUTE path INSTALL DESTINATION : ${CMAKE_ABSOLUTE_DESTINATION_FILES}")
  endif()
  if(CMAKE_ERROR_ON_ABSOLUTE_INSTALL_DESTINATION)
    message(FATAL_ERROR "ABSOLUTE path INSTALL DESTINATION forbidden (by caller): ${CMAKE_ABSOLUTE_DESTINATION_FILES}")
  endif()
  file(INSTALL DESTINATION "/Users/tunm/work/InspireFace/build_test/install/sample" TYPE EXECUTABLE FILES "/Users/tunm/work/InspireFace/build_test/sample/Leak")
  if(EXISTS "$ENV{DESTDIR}/Users/tunm/work/InspireFace/build_test/install/sample/Leak" AND
     NOT IS_SYMLINK "$ENV{DESTDIR}/Users/tunm/work/InspireFace/build_test/install/sample/Leak")
    execute_process(COMMAND /usr/bin/install_name_tool
      -delete_rpath "/Users/tunm/work/InspireFace/3rdparty/inspireface-precompile-lite/rknn/rknpu2/runtime/RV1106/Linux/librknn_api"
      -delete_rpath "/Users/tunm/work/InspireFace/build_test/lib"
      "$ENV{DESTDIR}/Users/tunm/work/InspireFace/build_test/install/sample/Leak")
    if(CMAKE_INSTALL_DO_STRIP)
      execute_process(COMMAND "/Applications/Xcode.app/Contents/Developer/Toolchains/XcodeDefault.xctoolchain/usr/bin/strip" -u -r "$ENV{DESTDIR}/Users/tunm/work/InspireFace/build_test/install/sample/Leak")
    endif()
  endif()
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  list(APPEND CMAKE_ABSOLUTE_DESTINATION_FILES
   "/Users/tunm/work/InspireFace/build_test/install/sample/FaceTrackSample")
  if(CMAKE_WARN_ON_ABSOLUTE_INSTALL_DESTINATION)
    message(WARNING "ABSOLUTE path INSTALL DESTINATION : ${CMAKE_ABSOLUTE_DESTINATION_FILES}")
  endif()
  if(CMAKE_ERROR_ON_ABSOLUTE_INSTALL_DESTINATION)
    message(FATAL_ERROR "ABSOLUTE path INSTALL DESTINATION forbidden (by caller): ${CMAKE_ABSOLUTE_DESTINATION_FILES}")
  endif()
  file(INSTALL DESTINATION "/Users/tunm/work/InspireFace/build_test/install/sample" TYPE EXECUTABLE FILES "/Users/tunm/work/InspireFace/build_test/sample/FaceTrackSample")
  if(EXISTS "$ENV{DESTDIR}/Users/tunm/work/InspireFace/build_test/install/sample/FaceTrackSample" AND
     NOT IS_SYMLINK "$ENV{DESTDIR}/Users/tunm/work/InspireFace/build_test/install/sample/FaceTrackSample")
    execute_process(COMMAND /usr/bin/install_name_tool
      -delete_rpath "/Users/tunm/work/InspireFace/3rdparty/inspireface-precompile-lite/rknn/rknpu2/runtime/RV1106/Linux/librknn_api"
      -delete_rpath "/Users/tunm/work/InspireFace/build_test/lib"
      "$ENV{DESTDIR}/Users/tunm/work/InspireFace/build_test/install/sample/FaceTrackSample")
    if(CMAKE_INSTALL_DO_STRIP)
      execute_process(COMMAND "/Applications/Xcode.app/Contents/Developer/Toolchains/XcodeDefault.xctoolchain/usr/bin/strip" -u -r "$ENV{DESTDIR}/Users/tunm/work/InspireFace/build_test/install/sample/FaceTrackSample")
    endif()
  endif()
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  list(APPEND CMAKE_ABSOLUTE_DESTINATION_FILES
   "/Users/tunm/work/InspireFace/build_test/install/sample/FaceComparisonSample")
  if(CMAKE_WARN_ON_ABSOLUTE_INSTALL_DESTINATION)
    message(WARNING "ABSOLUTE path INSTALL DESTINATION : ${CMAKE_ABSOLUTE_DESTINATION_FILES}")
  endif()
  if(CMAKE_ERROR_ON_ABSOLUTE_INSTALL_DESTINATION)
    message(FATAL_ERROR "ABSOLUTE path INSTALL DESTINATION forbidden (by caller): ${CMAKE_ABSOLUTE_DESTINATION_FILES}")
  endif()
  file(INSTALL DESTINATION "/Users/tunm/work/InspireFace/build_test/install/sample" TYPE EXECUTABLE FILES "/Users/tunm/work/InspireFace/build_test/sample/FaceComparisonSample")
  if(EXISTS "$ENV{DESTDIR}/Users/tunm/work/InspireFace/build_test/install/sample/FaceComparisonSample" AND
     NOT IS_SYMLINK "$ENV{DESTDIR}/Users/tunm/work/InspireFace/build_test/install/sample/FaceComparisonSample")
    execute_process(COMMAND /usr/bin/install_name_tool
      -delete_rpath "/Users/tunm/work/InspireFace/3rdparty/inspireface-precompile-lite/rknn/rknpu2/runtime/RV1106/Linux/librknn_api"
      -delete_rpath "/Users/tunm/work/InspireFace/build_test/lib"
      "$ENV{DESTDIR}/Users/tunm/work/InspireFace/build_test/install/sample/FaceComparisonSample")
    if(CMAKE_INSTALL_DO_STRIP)
      execute_process(COMMAND "/Applications/Xcode.app/Contents/Developer/Toolchains/XcodeDefault.xctoolchain/usr/bin/strip" -u -r "$ENV{DESTDIR}/Users/tunm/work/InspireFace/build_test/install/sample/FaceComparisonSample")
    endif()
  endif()
endif()

