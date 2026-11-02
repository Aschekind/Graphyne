# graphyne_add_shaders(<target> <glsl files...>)
# Compiles each GLSL file (relative to the repo root, e.g. shaders/a.vert) to SPIR-V next to the executables:
# bin/shaders/a.vert.spv (bin/<Config>/shaders on multi-config generators). Uses glslang from vcpkg.
# Installed to share/graphyne/shaders; apps using an installed Graphyne copy them next to their executable.
set(GRAPHYNE_SHADER_SOURCE_ROOT "${CMAKE_CURRENT_LIST_DIR}/..")
find_program(GRAPHYNE_GLSLANG NAMES glslang glslangValidator
    HINTS "${VCPKG_INSTALLED_DIR}/${VCPKG_HOST_TRIPLET}/tools/glslang"
    NO_DEFAULT_PATH)
find_program(GRAPHYNE_GLSLANG NAMES glslang glslangValidator)

function(graphyne_add_shaders target)
    if(NOT GRAPHYNE_GLSLANG)
        message(FATAL_ERROR "glslang not found: build with the vcpkg toolchain (vcpkg.json host dependency)")
    endif()
    get_property(multi GLOBAL PROPERTY GENERATOR_IS_MULTI_CONFIG)
    if(multi)
        set(out_dir "${CMAKE_BINARY_DIR}/bin/$<CONFIG>/shaders")
    else()
        set(out_dir "${CMAKE_BINARY_DIR}/bin/shaders")
    endif()
    set(outputs)
    foreach(src IN LISTS ARGN)
        get_filename_component(name "${src}" NAME)
        set(spv "${out_dir}/${name}.spv")
        add_custom_command(
            OUTPUT "${spv}"
            COMMAND ${CMAKE_COMMAND} -E make_directory "${out_dir}"
            COMMAND ${GRAPHYNE_GLSLANG} --target-env vulkan1.3 -o "${spv}" "${GRAPHYNE_SHADER_SOURCE_ROOT}/${src}"
            DEPENDS "${GRAPHYNE_SHADER_SOURCE_ROOT}/${src}"
            COMMENT "Compiling shader ${name}"
            VERBATIM)
        list(APPEND outputs "${spv}")
    endforeach()
    add_custom_target(${target}_shaders DEPENDS ${outputs})
    add_dependencies(${target} ${target}_shaders)
    install(FILES ${outputs} DESTINATION share/graphyne/shaders)
endfunction()
