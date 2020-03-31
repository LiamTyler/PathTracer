cmake_minimum_required(VERSION 3.9)

project(PathTracer LANGUAGES CXX C)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(OpenMP)
if(NOT OpenMP_CXX_FOUND)
    message("OpenMP not found! Rendering will be single threaded")
endif()

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_DEBUG ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_DEBUG ${CMAKE_BINARY_DIR}/lib)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR}/lib)

add_subdirectory(ext)

if (UNIX)
    #set(CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} -Wall -Werror -Wextra -Wno-implicit-fallthrough -Wshadow -Wno-unused-variable -Wno-unused-function")
    set(CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} -Wall -Wno-unused-function")
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -g")
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O3")
elseif(MSVC)
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /Od")
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /O2")
endif()


configure_file(cmake/configuration.hpp.in ${CMAKE_SOURCE_DIR}/src/configuration.hpp)

include_directories( SYSTEM
    ext/
    ext/assimp/include
)
include_directories(
    src/
)

set(
    SRC_FILES
    src/aabb.cpp
    src/aabb.hpp
    src/anti_aliasing.cpp
    src/anti_aliasing.hpp
    src/bvh.cpp
    src/bvh.hpp
    src/camera.cpp
    src/camera.hpp
    src/configuration.hpp
    src/core_defines.hpp
    src/image.cpp
    src/image.hpp
    src/intersection_tests.cpp
    src/intersection_tests.hpp
    src/lights.cpp
    src/lights.hpp
    src/math.cpp
    src/math.hpp
    src/path_tracer.cpp
    src/path_tracer.hpp
    src/scene.cpp
    src/scene.hpp
    src/shapes.hpp
    src/shapes.cpp
    src/transform.cpp
    src/transform.hpp
    
    src/resource/material.hpp
    src/resource/model.cpp
    src/resource/model.hpp
    src/resource/resource.hpp
    src/resource/resource_manager.cpp
    src/resource/resource_manager.hpp
    src/resource/skybox.cpp
    src/resource/skybox.hpp
    src/resource/texture.cpp
    src/resource/texture.hpp
    
    src/utils/json_parsing.cpp
    src/utils/json_parsing.hpp
    src/utils/random.cpp
    src/utils/random.hpp
    src/utils/time.cpp
    src/utils/time.hpp
    
    src/main.cpp
)

add_executable(pathTracer ${SRC_FILES})
set_target_properties(
    pathTracer
    PROPERTIES
    DEBUG_POSTFIX _debug
)

if(OpenMP_CXX_FOUND)
    target_link_libraries(pathTracer PUBLIC OpenMP::OpenMP_CXX assimp)
endif()

if(MSVC)
    # Enable object level parallelism during build
    target_compile_options(pathTracer PRIVATE "/MP")
    
    # Tell msvc to keep directory structure in it's list of files
    SET(listVar "")
    FOREACH(f ${SRC_FILES})
        LIST(APPEND listVar "${CMAKE_CURRENT_SOURCE_DIR}/${f}")
    ENDFOREACH(f)
    source_group(TREE ${CMAKE_SOURCE_DIR} FILES ${listVar})
    
    # Set this executable as the startup project in msvc
    set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT pathTracer)
    
    set_property(
        DIRECTORY
        PROPERTY
        COMPILE_DEFINITIONS
        _CRT_SECURE_NO_WARNINGS
    )
endif()
