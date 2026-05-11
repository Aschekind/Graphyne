# GraphyneShaders.cmake
#
# Compiles GLSL shader sources to SPIR-V at build time. Looks for the
# shader compiler in (in order):
#
#   1. CMake's FindVulkan results (Vulkan_GLSLANG_VALIDATOR_EXECUTABLE /
#      Vulkan_GLSLC_EXECUTABLE) — set when find_package(Vulkan) is called.
#   2. PATH (`glslangValidator`, `glslang`, `glslc`).
#   3. $VULKAN_SDK/Bin and $VULKAN_SDK/bin.
#   4. Default LunarG install location on Windows: C:/VulkanSDK/*/Bin
#      (newest version wins).
#
# Usage:
#   graphyne_compile_shaders(
#       TARGET     graphyne_shaders
#       OUTPUT_DIR ${CMAKE_BINARY_DIR}/bin/shaders
#       SOURCES    shaders/sprite.vert shaders/sprite.frag
#   )

include_guard(GLOBAL)

# Probe well-known Vulkan SDK install locations. Returns a list of bin dirs.
function(_graphyne_find_vulkan_bin_dirs out_var)
    set(result "")

    if(DEFINED ENV{VULKAN_SDK})
        list(APPEND result "$ENV{VULKAN_SDK}/Bin" "$ENV{VULKAN_SDK}/bin")
    endif()

    if(WIN32)
        # The LunarG Windows installer drops SDKs under C:\VulkanSDK\<version>\.
        file(GLOB _candidates "C:/VulkanSDK/*/Bin")
        if(_candidates)
            list(SORT _candidates ORDER DESCENDING)  # newest version first
            list(APPEND result ${_candidates})
        endif()
        # Some installs use lowercase 'bin'.
        file(GLOB _candidates_lc "C:/VulkanSDK/*/bin")
        if(_candidates_lc)
            list(SORT _candidates_lc ORDER DESCENDING)
            list(APPEND result ${_candidates_lc})
        endif()
    endif()

    if(APPLE)
        file(GLOB _candidates "$ENV{HOME}/VulkanSDK/*/macOS/bin")
        if(_candidates)
            list(SORT _candidates ORDER DESCENDING)
            list(APPEND result ${_candidates})
        endif()
    endif()

    set(${out_var} "${result}" PARENT_SCOPE)
endfunction()


function(graphyne_compile_shaders)
    set(oneValueArgs TARGET OUTPUT_DIR)
    set(multiValueArgs SOURCES)
    cmake_parse_arguments(ARG "" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    if(NOT ARG_TARGET OR NOT ARG_OUTPUT_DIR OR NOT ARG_SOURCES)
        message(FATAL_ERROR "graphyne_compile_shaders: TARGET, OUTPUT_DIR and SOURCES are required")
    endif()

    _graphyne_find_vulkan_bin_dirs(_vk_bin_dirs)

    # --- Locate glslangValidator ---------------------------------------------
    set(_glslang_path "")
    if(Vulkan_GLSLANG_VALIDATOR_EXECUTABLE)
        set(_glslang_path "${Vulkan_GLSLANG_VALIDATOR_EXECUTABLE}")
    endif()
    if(NOT _glslang_path)
        find_program(GRAPHYNE_GLSLANG_EXE
            NAMES glslangValidator glslang
            HINTS ${_vk_bin_dirs})
        if(GRAPHYNE_GLSLANG_EXE)
            set(_glslang_path "${GRAPHYNE_GLSLANG_EXE}")
        endif()
    endif()

    # --- Locate glslc (fallback) ---------------------------------------------
    set(_glslc_path "")
    if(Vulkan_GLSLC_EXECUTABLE)
        set(_glslc_path "${Vulkan_GLSLC_EXECUTABLE}")
    endif()
    if(NOT _glslc_path)
        find_program(GRAPHYNE_GLSLC_EXE
            NAMES glslc
            HINTS ${_vk_bin_dirs})
        if(GRAPHYNE_GLSLC_EXE)
            set(_glslc_path "${GRAPHYNE_GLSLC_EXE}")
        endif()
    endif()

    # --- Decide which to use -------------------------------------------------
    set(_compiler "")
    set(_flags    "")
    set(_label    "")
    if(_glslang_path)
        set(_compiler "${_glslang_path}")
        set(_flags    -V)
        set(_label    "glslangValidator")
        message(STATUS "Shader compiler: ${_compiler}")
    elseif(_glslc_path)
        set(_compiler "${_glslc_path}")
        set(_flags    "")
        set(_label    "glslc")
        message(STATUS "Shader compiler: ${_compiler}")
    else()
        # Tell the user where we looked and how to fix it.
        set(_searched_paths "")
        foreach(p IN LISTS _vk_bin_dirs)
            string(APPEND _searched_paths "    - ${p}\n")
        endforeach()
        if(NOT _searched_paths)
            set(_searched_paths "    (no Vulkan SDK paths detected)\n")
        endif()

        set(_env_sdk "$ENV{VULKAN_SDK}")
        if(NOT _env_sdk)
            set(_env_sdk "<not set>")
        endif()

        message(FATAL_ERROR
            "\n"
            "============================================================\n"
            "  Shader compiler not found\n"
            "============================================================\n"
            "  Neither glslangValidator nor glslc is on PATH or in any\n"
            "  detected Vulkan SDK location.\n"
            "\n"
            "  VULKAN_SDK env var: ${_env_sdk}\n"
            "  Paths searched:\n"
            "${_searched_paths}"
            "\n"
            "  Fix one of these:\n"
            "\n"
            "  1. If you ran scripts/setup.ps1, the Vulkan SDK installer\n"
            "     set VULKAN_SDK machine-wide. Close this shell and open\n"
            "     a new one so the variable propagates, then retry.\n"
            "\n"
            "  2. Install the Vulkan SDK manually:\n"
            "       https://vulkan.lunarg.com/sdk/home\n"
            "     The installer drops glslangValidator.exe in\n"
            "     C:\\VulkanSDK\\<version>\\Bin.\n"
            "\n"
            "  3. Already installed but at a non-default path? Set\n"
            "     VULKAN_SDK explicitly before configuring:\n"
            "       \$env:VULKAN_SDK = \"C:\\path\\to\\sdk\"\n"
            "\n"
            "  4. Bypass shader compilation entirely (compile them\n"
            "     yourself or pre-bundle the .spv files):\n"
            "       cmake -DGRAPHYNE_COMPILE_SHADERS=OFF ...\n"
            "============================================================\n")
    endif()

    file(MAKE_DIRECTORY ${ARG_OUTPUT_DIR})

    set(_spirv_outputs "")
    foreach(_src IN LISTS ARG_SOURCES)
        if(NOT IS_ABSOLUTE ${_src})
            set(_src ${CMAKE_CURRENT_SOURCE_DIR}/${_src})
        endif()
        get_filename_component(_name ${_src} NAME)
        set(_out ${ARG_OUTPUT_DIR}/${_name}.spv)

        if(_label STREQUAL "glslangValidator")
            add_custom_command(
                OUTPUT  ${_out}
                COMMAND ${_compiler} ${_flags} -o ${_out} ${_src}
                DEPENDS ${_src}
                COMMENT "GLSL -> SPIR-V: ${_name}"
                VERBATIM
            )
        else() # glslc
            add_custom_command(
                OUTPUT  ${_out}
                COMMAND ${_compiler} ${_src} -o ${_out}
                DEPENDS ${_src}
                COMMENT "GLSL -> SPIR-V: ${_name}"
                VERBATIM
            )
        endif()
        list(APPEND _spirv_outputs ${_out})
    endforeach()

    add_custom_target(${ARG_TARGET} ALL DEPENDS ${_spirv_outputs})
    set_target_properties(${ARG_TARGET} PROPERTIES SHADER_OUTPUT_DIR ${ARG_OUTPUT_DIR})
endfunction()
