cmake_minimum_required(VERSION 3.20 FATAL_ERROR)

project(LumiEngine VERSION 0.1.2)
message(STATUS "${CMAKE_PROJECT_NAME} v${CMAKE_PROJECT_VERSION}")

# =============== Prevent in-source builds ====================
if(PROJECT_SOURCE_DIR STREQUAL PROJECT_BINARY_DIR)
    message(
        FATAL_ERROR
        "In-source builds not allowed. Please make a new directory (called a build directory) and run CMake from there."
    )
endif()

# ==================== Path Variables ====================
set(LUMI_ROOT_DIR              ${CMAKE_CURRENT_SOURCE_DIR})
set(LUMI_THIRDPARTY_DIR        ${LUMI_ROOT_DIR}/thirdparty)
set(LUMI_ASSETS_DIR            ${LUMI_ROOT_DIR}/assets)
set(LUMI_SOURCE_DIR            ${LUMI_ROOT_DIR}/src)
set(LUMI_SHADERS_SRC_DIR       ${LUMI_ROOT_DIR}/shaders)
set(LUMI_SHADERS_COMPILED_DIR  ${PROJECT_BINARY_DIR}/shaders)

# ==================== Options =========================
# build test option
option(LUMI_FORCE_ASSERT       "Force assert in release mode"  OFF)
option(LUMI_ENABLE_DEBUG_LOG   "Enable debug level logging"    OFF)

# ==================== Global settings ====================
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
add_compile_options($<$<CXX_COMPILER_ID:MSVC>:/MP>)
add_compile_options($<$<CXX_COMPILER_ID:MSVC>:/utf-8>)
add_compile_options($<$<CXX_COMPILER_ID:MSVC>:/wd26812>)

set(BUILD_SHARED_LIBS OFF)
set(CMAKE_INSTALL_PREFIX ${LUMI_ROOT_DIR}/install)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# Global macros
add_compile_definitions(LUMI_VERSION="${CMAKE_PROJECT_VERSION}")
add_compile_definitions(LUMI_ENGINE_NAME="${CMAKE_PROJECT_NAME}")
add_compile_definitions(LUMI_ROOT_DIR="${LUMI_ROOT_DIR}")
add_compile_definitions(LUMI_ASSETS_DIR="${LUMI_ASSETS_DIR}")
add_compile_definitions(LUMI_SHADERS_DIR="${LUMI_SHADERS_COMPILED_DIR}")
add_compile_definitions(NOMINMAX)

if(LUMI_FORCE_ASSERT)
    add_compile_definitions(LUMI_FORCE_ASSERT)
endif()
if(LUMI_ENABLE_DEBUG_LOG)
    add_compile_definitions(LUMI_ENABLE_DEBUG_LOG)
endif()

# Group targets by folders in IDE
set_property(GLOBAL PROPERTY USE_FOLDERS ON)
define_property(
    TARGET
    PROPERTY FOLDER
    INHERITED
    BRIEF_DOCS "Set the folder name."
    FULL_DOCS  "Use to organize targets in an IDE."
)

# ==================== sub directories ====================
add_subdirectory(thirdparty)

add_subdirectory(shaders)
add_subdirectory(src)