cmake_minimum_required(VERSION 3.10 FATAL_ERROR)
project(ETS-MTI870)

set (CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
set (CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})

set (CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
set (CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})

# ============================================================================
# Set a default build configuration (Release)
# ============================================================================
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  message(STATUS "Setting build type to 'Release' as none was specified.")
  set(CMAKE_BUILD_TYPE
      Release
      CACHE STRING "Choose the type of build." FORCE
  )
  set_property(
    CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Debug" "Release" "MinSizeRel" "RelWithDebInfo"
  )
endif()
string(TOUPPER "${CMAKE_BUILD_TYPE}" U_CMAKE_BUILD_TYPE)

# ============================================================================
# Sanitizer 
# ============================================================================
if(USE_SANITIZER OR USE_STATIC_ANALYZER)
  if(USE_SANITIZER)
    include(${cmake-scripts_SOURCE_DIR}/sanitizers.cmake)
  endif()

  if(USE_STATIC_ANALYZER)
    if("clang-tidy" IN_LIST USE_STATIC_ANALYZER)
      set(CLANG_TIDY
          ON
          CACHE INTERNAL ""
      )
    else()
      set(CLANG_TIDY
          OFF
          CACHE INTERNAL ""
      )
    endif()
    if("iwyu" IN_LIST USE_STATIC_ANALYZER)
      set(IWYU
          ON
          CACHE INTERNAL ""
      )
    else()
      set(IWYU
          OFF
          CACHE INTERNAL ""
      )
    endif()
    if("cppcheck" IN_LIST USE_STATIC_ANALYZER)
      set(CPPCHECK
          ON
          CACHE INTERNAL ""
      )
    else()
      set(CPPCHECK
          OFF
          CACHE INTERNAL ""
      )
    endif()

    clang_tidy(${CLANG_TIDY_ARGS})
    include_what_you_use(${IWYU_ARGS})
    cppcheck(${CPPCHECK_ARGS})
  endif()
endif()

# ============================================================================
# LOOK for the packages that we need! 
# ============================================================================
# - JSON reader
add_library(nlohmann_json INTERFACE IMPORTED)
target_include_directories(nlohmann_json INTERFACE "3rdparty/json/single_include")
# - Logging 
add_subdirectory(3rdparty/spdlog)
# - HDR image (OpenEXR)
SET(TINYEXR_BUILD_SAMPLE off)
add_subdirectory(3rdparty/tinyexr)
include_directories(3rdparty/tinyexr)
# - LDR images
include_directories(3rdparty/stb)
# - Obj model format
add_subdirectory(3rdparty/tinyobjloader)
# - Very simple math library
include_directories(3rdparty/linalg)
# - Command line parsing
include_directories(3rdparty/argparse/include)
# - Multithreading
add_subdirectory(3rdparty/nanothread)
include_directories(3rdparty/nanothread/include)
# - Random number generator
include_directories(3rdparty/pcg32)
# - Custom threads
set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)
# - Pybind
# add_subdirectory(3rdparty/pybind11)

# ============================================================================
# 
# ============================================================================
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include_directories(include)
add_library(render_lib OBJECT
  # Headers
  include/render/common.h
  include/render/fwd.h  
  include/render/vec.h
  include/render/array2d.h
  include/render/image.h
  include/render/camera.h
  include/render/json.h
  include/render/ray.h
  include/render/transform.h
  include/render/progress.h
  include/render/scene.h
  include/render/filesystem.h
  include/render/texture.h
  include/render/aabb.h

  include/render/materials/material.h
  include/render/materials/diffuse.h
  include/render/materials/vMF_diffuse.h
  include/render/materials/metal.h
  include/render/materials/dielectric.h
  include/render/materials/diffuse_emitter.h
  include/render/materials/diffuse_transparent.h
  include/render/materials/blend.h
  include/render/materials/fresnel_blend.h
  include/render/materials/phong.h
  include/render/materials/mirage.h
  
  include/render/shapes/shape.h
  include/render/shapes/sphere.h
  include/render/shapes/quad.h
  include/render/shapes/shape_group.h
  include/render/shapes/envmap.h
  include/render/shapes/instance.h
  
  include/render/samplers/sampler.h
  include/render/samplers/independent.h

  include/render/integrators/direct.h
  include/render/integrators/integrator.h 
  include/render/integrators/path.h
  include/render/integrators/path_mis.h
  include/render/integrators/normal.h
  include/render/integrators/tangent.h
  include/render/integrators/uv.h

  # Sources
  src/image.cpp
  src/materials/material.cpp
  src/shapes/shape.cpp
  src/shapes/mesh.cpp
  src/samplers/sampler.cpp
  src/integrators/integrator.cpp
  src/scene.cpp
  src/progress.cpp
  src/texture.cpp
)

target_compile_options(render_lib PUBLIC "$<$<COMPILE_LANG_AND_ID:CXX,MSVC>:/permissive->")
target_link_libraries(
    render_lib
  PRIVATE tinyobjloader tinyexr nanothread Threads::Threads
  PUBLIC nlohmann_json spdlog::spdlog 
)

# add_executable(devoir_0 devoirs/devoir_0.cpp)
# target_link_libraries(devoir_0 PRIVATE render_lib)

# add_executable(devoir_1 devoirs/devoir_1.cpp)
# target_link_libraries(devoir_1 PRIVATE render_lib)

# add_executable(devoir_3 devoirs/devoir_3.cpp devoirs/utils.h)
# target_link_libraries(devoir_3 PRIVATE render_lib)

# add_executable(test devoirs/test.cpp devoirs/utils.h)
# target_link_libraries(test PRIVATE render_lib)

add_executable(render src/render.cpp)
target_link_libraries(render PRIVATE render_lib)
