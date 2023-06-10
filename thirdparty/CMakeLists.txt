set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)

# functions for target properties
define_property(
    TARGET
    PROPERTY FOLDER
    INHERITED
    BRIEF_DOCS "Set the folder name."
    FULL_DOCS  "Use to organize targets in an IDE."
)

function(get_all_targets _result _dir)
    get_property(_subdirs DIRECTORY "${_dir}" PROPERTY SUBDIRECTORIES)
    foreach(_subdir IN LISTS _subdirs)
        get_all_targets(${_result} "${_subdir}")
    endforeach()
    get_property(_sub_targets DIRECTORY "${_dir}" PROPERTY BUILDSYSTEM_TARGETS)
    set(${_result} ${${_result}} ${_sub_targets} PARENT_SCOPE)
endfunction()

function(add_subdirectory_with_folder _folder_name _folder)
    add_subdirectory(${_folder} ${ARGN})
    get_all_targets(_targets "${_folder}")
    foreach(_target IN LISTS _targets)
        set_target_properties(
            ${_target}
            PROPERTIES FOLDER "${_folder_name}"
        )
    endforeach()
endfunction()

# Vulkan
find_package(Vulkan REQUIRED)
set( Vulkan_INCLUDE_DIRS ${Vulkan_INCLUDE_DIRS} PARENT_SCOPE )
set( Vulkan_LIBRARIES ${Vulkan_LIBRARIES} PARENT_SCOPE )

# glfw
if(NOT TARGET glfw)
    set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
    set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
    set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
    set(GLFW_INSTALL OFF CACHE BOOL "" FORCE)

    add_subdirectory_with_folder("thirdparty/glfw" glfw)
endif()

# glm
if(NOT TARGET glm)
    add_subdirectory_with_folder("thirdparty/glm" glm)
    # add_subdirectory(glm)
    # set_property(DIRECTORY glm 
    #     PROPERTY FOLDER thirdparty/glm)
endif()

# spdlog
if(NOT TARGET spdlog)
    add_subdirectory(spdlog)
    set_property(TARGET spdlog 
        PROPERTY FOLDER thirdparty/spdlog)
endif()

# nlohmann json
if(NOT TARGET nlohmann)
    add_subdirectory(nlohmann)
    set_property(TARGET nlohmann
        PROPERTY FOLDER thirdparty/nlohmann-json)
endif()

# imgui
if(NOT TARGET imgui)
    add_subdirectory(imgui)
    set_property(TARGET imgui 
        PROPERTY FOLDER thirdparty/imgui)
endif()

# stb
if(NOT TARGET stb_image)
    add_subdirectory(stb_image)
    set_property(TARGET stb_image
        PROPERTY FOLDER thirdparty/stb_image)
endif()

# vkbootstrap
if(NOT TARGET vkbootstrap)
    add_subdirectory(vkbootstrap)
    set_property(TARGET vkbootstrap 
        PROPERTY FOLDER thirdparty/vkbootstrap)
endif()

# vma
if(NOT TARGET vma)
    add_subdirectory(vma)
    set_property(TARGET vma 
        PROPERTY FOLDER thirdparty/vma)
endif()


# # Assimp
# if(NOT TARGET assimp)
#     set(ASSIMP_BUILD_TESTS OFF CACHE BOOL "" FORCE)
#     set(ASSIMP_INSTALL OFF CACHE BOOL "" FORCE) 
#     set(ASSIMP_BUILD_ASSIMP_TOOLS OFF CACHE BOOL "" FORCE) 
#     # importer format
#     set(ASSIMP_BUILD_ALL_IMPORTERS_BY_DEFAULT OFF CACHE BOOL "" FORCE) 
#     set(ASSIMP_BUILD_OBJ_IMPORTER ON CACHE BOOL "" FORCE) 
#     # exporter format
#     set(ASSIMP_BUILD_ALL_EXPORTERS_BY_DEFAULT OFF CACHE BOOL "" FORCE) 

#     add_subdirectory(assimp)
#     set_property(DIRECTORY assimp 
#         PROPERTY FOLDER thirdparty/assimp)
# endif()




