set(LUMI_SHADER_COMPILE LumiShaderCompile)
set(LUMI_SHADER_COMPILE ${LUMI_SHADER_COMPILE} PARENT_SCOPE)
find_program(GLSL_VALIDATOR glslangValidator 
            HINTS /usr/bin /usr/local/bin $ENV{VULKAN_SDK}/Bin/ $ENV{VULKAN_SDK}/Bin32/)

## find all the shader files under the shaders folder
file(GLOB_RECURSE GLSL_SOURCE_FILES CONFIGURE_DEPENDS
    *.vert
    *.frag
    *.comp
    *.geom
    *.tesc
    *.tese
    *.mesh
    *.task
    *.rgen
    *.rchit
    *.rmiss
    )
source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}"
             PREFIX "shaders"
             FILES ${GLSL_SOURCE_FILES})

## iterate each shader
foreach(GLSL_SOURCE ${GLSL_SOURCE_FILES})
  string(REPLACE "${CMAKE_CURRENT_SOURCE_DIR}" "${LUMI_SHADERS_COMPILED_DIR}" 
                 GLSL_COMPILED ${GLSL_SOURCE})
  set(GLSL_COMPILED ${GLSL_COMPILED}.spv)
  # message(STATUS "${GLSL_COMPILED}")

  ## execute glslang command to compile that specific shader
  add_custom_command(
    OUTPUT ${GLSL_COMPILED}
    COMMAND ${CMAKE_COMMAND} -E make_directory ${LUMI_SHADERS_COMPILED_DIR}
    COMMAND ${GLSL_VALIDATOR} -V ${GLSL_SOURCE} -o ${GLSL_COMPILED}
    DEPENDS ${GLSL_SOURCE}
    )
  list(APPEND GLSL_BINARY_FILES ${GLSL_COMPILED})
endforeach()

add_custom_target(
    ${LUMI_SHADER_COMPILE} 
    DEPENDS ${GLSL_BINARY_FILES}
    SOURCES ${GLSL_SOURCE_FILES}
    )
set_target_properties(${LUMI_SHADER_COMPILE} PROPERTIES FOLDER ${CMAKE_PROJECT_NAME})
