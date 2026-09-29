find_package(Python3 REQUIRED COMPONENTS Interpreter)
find_package(Java 1.8 REQUIRED COMPONENTS Development)
include(UseJava)
if(NOT ANDROID)
    if(CMAKE_VERSION VERSION_LESS 3.24)
        find_package(JNI REQUIRED)
    else()
        # The JNI adapter needs headers only; headless JDKs need not provide AWT.
        find_package(JNI REQUIRED COMPONENTS JVM)
    endif()
endif()

set(java_generated "${CMAKE_CURRENT_BINARY_DIR}/generated")
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/generate.py"
    "${PROJECT_SOURCE_DIR}/cpp/inspireface/c_api/inspireface.h"
    "${PROJECT_SOURCE_DIR}/cpp/inspireface/c_api/intypedef.h"
    "${PROJECT_SOURCE_DIR}/cpp/inspireface/include/inspireface/herror.h")
execute_process(COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/generate.py"
    --root "${PROJECT_SOURCE_DIR}" --output "${java_generated}"
    RESULT_VARIABLE generation_status)
if(NOT generation_status EQUAL 0)
    message(FATAL_ERROR "Portable JNI generation failed; review C API mapping")
endif()

add_library(InspireFaceJNI SHARED "${java_generated}/bindings.cpp" streams.cpp features.cpp)
target_include_directories(InspireFaceJNI PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}"
    "${PROJECT_SOURCE_DIR}/cpp/inspireface" "${PROJECT_SOURCE_DIR}/cpp/inspireface/include/inspireface"
    ${JNI_INCLUDE_DIRS})
target_link_libraries(InspireFaceJNI PRIVATE InspireFace)
set_property(TARGET InspireFaceJNI APPEND PROPERTY LINK_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/verify_api.py"
    "${PROJECT_SOURCE_DIR}/java/src/test/java/com/insightface/sdk/inspireface/jni/ContractTest.java")
add_custom_command(TARGET InspireFaceJNI POST_BUILD
    COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/verify_api.py"
        --root "${PROJECT_SOURCE_DIR}" --generated "${java_generated}"
        --library "$<TARGET_FILE:InspireFaceJNI>" --nm "${CMAKE_NM}"
    COMMENT "Checking complete C API / Java / JNI parity" VERBATIM)
set_target_properties(InspireFaceJNI PROPERTIES
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/java/native"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/java/native")
if(APPLE)
    set_target_properties(InspireFaceJNI PROPERTIES INSTALL_RPATH "@loader_path")
    set_target_properties(InspireFace PROPERTIES INSTALL_NAME_DIR "@rpath" BUILD_WITH_INSTALL_NAME_DIR TRUE)
elseif(UNIX)
    set_target_properties(InspireFaceJNI PROPERTIES INSTALL_RPATH "$ORIGIN")
    target_link_options(InspireFaceJNI PRIVATE "-Wl,-z,noexecstack")
endif()

file(GLOB_RECURSE java_sources CONFIGURE_DEPENDS
    "${PROJECT_SOURCE_DIR}/java/src/main/java/*.java" "${java_generated}/java/*.java")
if(Java_VERSION VERSION_LESS 9)
    set(CMAKE_JAVA_COMPILE_FLAGS -source 8 -target 8 -encoding UTF-8)
else()
    set(CMAKE_JAVA_COMPILE_FLAGS --release 8 -encoding UTF-8)
endif()
add_jar(InspireFaceJava SOURCES ${java_sources}
    OUTPUT_NAME inspireface OUTPUT_DIR "${CMAKE_BINARY_DIR}/java")
add_dependencies(InspireFaceJava InspireFaceJNI)

string(TOLOWER "${CMAKE_SYSTEM_NAME}" java_os)
if(APPLE)
    set(java_os macos)
endif()
set(java_arch "${CMAKE_SYSTEM_PROCESSOR}")
if(CMAKE_OSX_ARCHITECTURES)
    set(java_arch "${CMAKE_OSX_ARCHITECTURES}")
endif()
if(java_arch MATCHES "^(aarch64|arm64|ARM64)$")
    set(java_arch arm64)
elseif(java_arch MATCHES "^(x86_64|AMD64|amd64)$")
    set(java_arch x86_64)
endif()
set(java_native_destination "Java/native/${java_os}-${java_arch}")
install_jar(InspireFaceJava DESTINATION Java)
install(TARGETS InspireFaceJNI LIBRARY DESTINATION "${java_native_destination}"
    RUNTIME DESTINATION "${java_native_destination}")
get_target_property(java_core_type InspireFace TYPE)
if(java_core_type STREQUAL "SHARED_LIBRARY")
    install(TARGETS InspireFace LIBRARY DESTINATION "${java_native_destination}"
        RUNTIME DESTINATION "${java_native_destination}")
endif()
install(DIRECTORY "${java_generated}/java/" "${PROJECT_SOURCE_DIR}/java/src/main/java/"
    DESTINATION Java/sources FILES_MATCHING PATTERN "*.java")
install(FILES "${java_generated}/api-manifest.json" DESTINATION Java)
install(DIRECTORY "${PROJECT_SOURCE_DIR}/java/examples/" DESTINATION Java/examples)

if(ISF_BUILD_JAVA_TESTS)
    if(CMAKE_CROSSCOMPILING OR ANDROID)
        message(FATAL_ERROR "JVM runtime tests require a native host build")
    endif()
    file(GLOB_RECURSE java_test_sources CONFIGURE_DEPENDS "${PROJECT_SOURCE_DIR}/java/src/test/java/*.java")
    add_jar(InspireFaceJavaTests SOURCES ${java_test_sources} INCLUDE_JARS InspireFaceJava
        OUTPUT_NAME inspireface-tests OUTPUT_DIR "${CMAKE_BINARY_DIR}/java")
    if(WIN32)
        set(java_classpath_separator "\;")
    else()
        set(java_classpath_separator ":")
    endif()
    add_test(NAME InspireFace.Java.Contract COMMAND "${Java_JAVA_EXECUTABLE}" -Xcheck:jni
        "-Dinspireface.native.path=$<TARGET_FILE:InspireFaceJNI>"
        -cp "${CMAKE_BINARY_DIR}/java/inspireface.jar${java_classpath_separator}${CMAKE_BINARY_DIR}/java/inspireface-tests.jar"
        com.insightface.sdk.inspireface.jni.ContractTest
        "${PROJECT_SOURCE_DIR}/test_res/pack/Pikachu" "${PROJECT_SOURCE_DIR}/test_res/data/bulk/kun.jpg")
    set_tests_properties(InspireFace.Java.Contract PROPERTIES TIMEOUT 180 LABELS "java;contract")
endif()
