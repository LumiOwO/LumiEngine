set(LUMI_TARGET ${CMAKE_PROJECT_NAME} CACHE INTERNAL "")

# ============ Create executable ============
file(GLOB_RECURSE HEADERS CONFIGURE_DEPENDS  *.h)
file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS  *.cpp)
source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}" FILES ${HEADERS} ${SOURCES})
# message(STATUS "${HEADERS} ${SOURCES}")

add_executable(${LUMI_TARGET} ${HEADERS} ${SOURCES})
set_target_properties(${LUMI_TARGET} PROPERTIES FOLDER ${CMAKE_PROJECT_NAME})

# ============ Compile flags ============ 
target_compile_options(${LUMI_TARGET} PUBLIC 
    $<$<CXX_COMPILER_ID:MSVC>:/W3 >
    $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-Wall>
    $<$<C_COMPILER_ID:MSVC>:/W3 >
    $<$<NOT:$<C_COMPILER_ID:MSVC>>:-Wall>
    )

# ============ Macros for target ============ 
# target_compile_definitions(
#     ${LUMI_TARGET} PUBLIC 
#     LUMI_VULKAN_SHADERS_DIR="${SHADERS_BINARY_DIR}/vulkan/")

# ============  Includes  ============ 
target_include_directories( ${LUMI_TARGET}
    PUBLIC $<BUILD_INTERFACE:${LUMI_SOURCE_DIR}>
    PUBLIC $<BUILD_INTERFACE:${LUMI_THIRDPARTY_DIR}>
    PUBLIC $<BUILD_INTERFACE:${LUMI_THIRDPARTY_DIR}/glfw/include>
    PUBLIC $<BUILD_INTERFACE:${LUMI_THIRDPARTY_DIR}/glm>
    PUBLIC $<BUILD_INTERFACE:${Vulkan_INCLUDE_DIRS}>
    )


# ============ Link dependencies ============ 
# add_dependencies(${LUMI_TARGET}
#     ${SHADERS_COMPILE_TARGET})

target_link_libraries( ${LUMI_TARGET} 
    ${Vulkan_LIBRARIES}
    glfw
    glm 
    vkbootstrap 
    vma
    imgui
    stb_image
    spdlog
    nlohmann
    # tinyobjloader 
    )
    
