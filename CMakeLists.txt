cmake_minimum_required(VERSION 3.20)

# Standalone KServe V2 / Open Inference Protocol client. Pure wire-protocol
# client (HTTP + optional gRPC), with no dependency on any inference backend:
# tensor payloads are raw little-endian bytes, so this library is a drop-in peer
# of e.g. Triton's client library. It builds standalone (with its own tests) and
# as a FetchContent subproject of a consumer such as neuriplo-infer.

file(STRINGS "${CMAKE_CURRENT_SOURCE_DIR}/VERSION" _KSERVE_CLIENT_VERSION)
project(neuriplo-kserve-client VERSION ${_KSERVE_CLIENT_VERSION} LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

option(KSERVE_CLIENT_ENABLE_GRPC "Build the gRPC transport" ON)
option(KSERVE_CLIENT_ENABLE_TLS "Enable HTTPS (OpenSSL) for the HTTP client" ON)
option(KSERVE_CLIENT_BUILD_TESTS "Build the unit tests" ${PROJECT_IS_TOP_LEVEL})

# gRPC proto profile: OIP (strict KServe/OVMS) or OIP_REPOSITORY (Triton + repo).
set(KSERVE_CLIENT_PROTO_PROFILE "OIP_REPOSITORY" CACHE STRING
    "gRPC proto profile: OIP or OIP_REPOSITORY (see proto/README.md)")
set_property(CACHE KSERVE_CLIENT_PROTO_PROFILE PROPERTY STRINGS OIP OIP_REPOSITORY)
set(KSERVE_CLIENT_PROTO_FILE "" CACHE FILEPATH
    "Optional override: path to a .proto file (takes precedence over the profile)")

include(FetchContent)

# nlohmann/json: reuse the consumer's target when embedded; otherwise fetch it.
if(NOT TARGET nlohmann_json::nlohmann_json)
    find_package(nlohmann_json QUIET)
    if(NOT nlohmann_json_FOUND)
        FetchContent_Declare(
            nlohmann_json
            URL https://github.com/nlohmann/json/releases/download/v3.12.0/json.tar.xz
            DOWNLOAD_EXTRACT_TIMESTAMP TRUE
        )
        FetchContent_MakeAvailable(nlohmann_json)
    endif()
endif()

set(KSERVE_CLIENT_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/src/KserveProtocol.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/KserveRetry.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/KserveHttpClient.cpp
)

# --- Optional gRPC transport: generate stubs from the proto -------------------
set(_KSERVE_CLIENT_WITH_GRPC FALSE)
if(KSERVE_CLIENT_ENABLE_GRPC)
    find_package(Protobuf QUIET)
    find_package(gRPC CONFIG QUIET)
    if(Protobuf_FOUND AND gRPC_FOUND)
        set(_KSERVE_CLIENT_WITH_GRPC TRUE)
        message(STATUS "neuriplo-kserve-client: gRPC transport enabled")

        set(_KSERVE_PROTO_REPOSITORY FALSE)
        if(KSERVE_CLIENT_PROTO_FILE)
            set(_KSERVE_PROTO_SRC "${KSERVE_CLIENT_PROTO_FILE}")
        elseif(KSERVE_CLIENT_PROTO_PROFILE STREQUAL "OIP")
            set(_KSERVE_PROTO_SRC
                "${CMAKE_CURRENT_SOURCE_DIR}/proto/oip/grpc_predict_v2.proto")
        else()
            set(_KSERVE_PROTO_SRC
                "${CMAKE_CURRENT_SOURCE_DIR}/proto/kserve_grpc.proto")
            set(_KSERVE_PROTO_REPOSITORY TRUE)
        endif()

        set(PROTO_GEN_DIR "${CMAKE_CURRENT_BINARY_DIR}/proto")
        file(MAKE_DIRECTORY "${PROTO_GEN_DIR}")
        set(PROTO_FILE "${PROTO_GEN_DIR}/kserve_grpc.proto")
        configure_file("${_KSERVE_PROTO_SRC}" "${PROTO_FILE}" COPYONLY)

        if(NOT _KSERVE_PROTO_REPOSITORY AND KSERVE_CLIENT_PROTO_FILE)
            file(READ "${PROTO_FILE}" _KSERVE_PROTO_TEXT)
            if(_KSERVE_PROTO_TEXT MATCHES "rpc RepositoryIndex")
                set(_KSERVE_PROTO_REPOSITORY TRUE)
            endif()
        endif()

        get_filename_component(PROTO_DIR "${PROTO_FILE}" DIRECTORY)
        message(STATUS "neuriplo-kserve-client: gRPC proto profile "
                       "${KSERVE_CLIENT_PROTO_PROFILE} (${_KSERVE_PROTO_SRC})")

        set(PROTO_SRCS "${PROTO_GEN_DIR}/kserve_grpc.pb.cc")
        set(GRPC_SRCS "${PROTO_GEN_DIR}/kserve_grpc.grpc.pb.cc")

        get_target_property(_GRPC_PLUGIN_SUPPORT_LOC gRPC::grpc_plugin_support IMPORTED_LOCATION_NONE)
        if(NOT _GRPC_PLUGIN_SUPPORT_LOC)
            get_target_property(_GRPC_PLUGIN_SUPPORT_LOC gRPC::grpc_plugin_support IMPORTED_LOCATION)
        endif()
        if(_GRPC_PLUGIN_SUPPORT_LOC)
            get_filename_component(_GRPC_LIB_DIR "${_GRPC_PLUGIN_SUPPORT_LOC}" DIRECTORY)
        else()
            set(_GRPC_LIB_DIR "")
        endif()

        if(_GRPC_LIB_DIR)
            add_custom_command(
                OUTPUT "${PROTO_SRCS}" "${GRPC_SRCS}"
                COMMAND ${CMAKE_COMMAND} -E env "LD_LIBRARY_PATH=${_GRPC_LIB_DIR}"
                    "${PROTOBUF_PROTOC_EXECUTABLE}"
                    "--proto_path=${PROTO_DIR}"
                    "--cpp_out=${PROTO_GEN_DIR}"
                    "--grpc_out=${PROTO_GEN_DIR}"
                    "--plugin=protoc-gen-grpc=$<TARGET_FILE:gRPC::grpc_cpp_plugin>"
                    "${PROTO_FILE}"
                DEPENDS "${PROTO_FILE}"
                COMMENT "Generating gRPC sources"
            )
        else()
            add_custom_command(
                OUTPUT "${PROTO_SRCS}" "${GRPC_SRCS}"
                COMMAND "${PROTOBUF_PROTOC_EXECUTABLE}"
                    "--proto_path=${PROTO_DIR}"
                    "--cpp_out=${PROTO_GEN_DIR}"
                    "--grpc_out=${PROTO_GEN_DIR}"
                    "--plugin=protoc-gen-grpc=$<TARGET_FILE:gRPC::grpc_cpp_plugin>"
                    "${PROTO_FILE}"
                DEPENDS "${PROTO_FILE}"
                COMMENT "Generating gRPC sources"
            )
        endif()

        list(APPEND KSERVE_CLIENT_SOURCES
            "${PROTO_SRCS}"
            "${GRPC_SRCS}"
            ${CMAKE_CURRENT_SOURCE_DIR}/src/KserveGrpcClient.cpp)
    else()
        message(WARNING "neuriplo-kserve-client: gRPC/Protobuf not found; "
                        "building the HTTP client only.")
    endif()
endif()

add_library(neuriplo-kserve-client STATIC ${KSERVE_CLIENT_SOURCES})
add_library(neuriplo::kserve-client ALIAS neuriplo-kserve-client)

target_include_directories(neuriplo-kserve-client PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(neuriplo-kserve-client PUBLIC nlohmann_json::nlohmann_json)

# --- Optional HTTPS (OpenSSL) ------------------------------------------------
set(_KSERVE_CLIENT_WITH_TLS FALSE)
if(KSERVE_CLIENT_ENABLE_TLS)
    find_package(OpenSSL QUIET)
    if(OpenSSL_FOUND)
        set(_KSERVE_CLIENT_WITH_TLS TRUE)
        message(STATUS "neuriplo-kserve-client: HTTPS enabled (OpenSSL ${OPENSSL_VERSION})")
        target_compile_definitions(neuriplo-kserve-client PRIVATE KSERVE_CLIENT_WITH_TLS)
        target_link_libraries(neuriplo-kserve-client PRIVATE OpenSSL::SSL OpenSSL::Crypto)
    else()
        message(WARNING "neuriplo-kserve-client: OpenSSL not found; HTTP client "
                        "built without TLS. https:// endpoints error at runtime.")
    endif()
endif()

if(_KSERVE_CLIENT_WITH_GRPC)
    # PUBLIC so consumers can #ifdef on gRPC availability.
    target_compile_definitions(neuriplo-kserve-client PUBLIC KSERVE_CLIENT_WITH_GRPC)
    if(_KSERVE_PROTO_REPOSITORY)
        target_compile_definitions(neuriplo-kserve-client PUBLIC
            KSERVE_CLIENT_PROTO_REPOSITORY)
    endif()
    target_include_directories(neuriplo-kserve-client PRIVATE "${PROTO_GEN_DIR}")
    target_link_libraries(neuriplo-kserve-client PRIVATE protobuf::libprotobuf gRPC::grpc++)
endif()

if(KSERVE_CLIENT_BUILD_TESTS)
    enable_testing()
    add_subdirectory(test)
endif()
