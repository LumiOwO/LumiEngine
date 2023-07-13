set(LUMI_ENGINE_TARGET ${CMAKE_PROJECT_NAME})

# ============ Create executable ============
file(GLOB_RECURSE HEADERS CONFIGURE_DEPENDS  *.h *.inl)
file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS  *.cpp)
source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}" FILES ${HEADERS} ${SOURCES})

add_executable(${LUMI_ENGINE_TARGET} ${HEADERS} ${SOURCES})

set_target_properties(
    ${LUMI_ENGINE_TARGET} PROPERTIES 
    FOLDER ${CMAKE_PROJECT_NAME})

if(MSVC)
    set_target_properties(
        ${LUMI_ENGINE_TARGET} PROPERTIES
        VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/$(Configuration)")
endif()

# ============ Compile flags ============ 
target_compile_options(${LUMI_ENGINE_TARGET} PUBLIC 
    $<$<CXX_COMPILER_ID:MSVC>:/W3 >
    $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-Wall>
    $<$<C_COMPILER_ID:MSVC>:/W3 >
    $<$<NOT:$<C_COMPILER_ID:MSVC>>:-Wall>
    )

# ============ Macros for target ============
# target_compile_definitions(
# ${LUMI_ENGINE_TARGET} PUBLIC
# LUMI_VULKAN_SHADERS_DIR="${SHADERS_BINARY_DIR}/vulkan/")

# ============  Includes  ============
target_include_directories(${LUMI_ENGINE_TARGET}
    PUBLIC $<BUILD_INTERFACE:${LUMI_SOURCE_DIR}>
    PUBLIC $<BUILD_INTERFACE:${LUMI_THIRDPARTY_DIR}>
    PUBLIC $<BUILD_INTERFACE:${LUMI_THIRDPARTY_DIR}/glfw/include>
    PUBLIC $<BUILD_INTERFACE:${LUMI_THIRDPARTY_DIR}/glm>
    PUBLIC $<BUILD_INTERFACE:${LUMI_THIRDPARTY_DIR}/nlohmann>
    PUBLIC $<BUILD_INTERFACE:${LUMI_THIRDPARTY_DIR}/stb_image>
    PUBLIC $<BUILD_INTERFACE:${LUMI_THIRDPARTY_DIR}/rttr/src>
    PUBLIC $<BUILD_INTERFACE:${Vulkan_INCLUDE_DIRS}>
)


# ============ Link dependencies ============ 
add_dependencies(${LUMI_ENGINE_TARGET}
    ${LUMI_SHADER_COMPILE})

target_link_libraries(${LUMI_ENGINE_TARGET}
    ${Vulkan_LIBRARIES}
    glfw
    glm
    vkbootstrap
    vma
    imgui
    stb_image
    spdlog
    nlohmann
    tiny_obj_loader
    tinygltf
    rttr_core_lib
)
    
