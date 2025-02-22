# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

# The function generates the Qt module header structure in build directory and creates install
# rules. Apart the lists of header files the function takes into account
# QT_REPO_PUBLIC_NAMESPACE_REGEX cache variable, that can be set by repository in .cmake.conf file.
# The variable tells the syncqt program, what namespaces are treated as public. Symbols in public
# namespaces are considered when generating CaMeL case header files.
function(qt_internal_target_sync_headers target module_headers module_headers_generated)
    if(NOT TARGET ${QT_CMAKE_EXPORT_NAMESPACE}::syncqt)
        message(FATAL_ERROR "${QT_CMAKE_EXPORT_NAMESPACE}::syncqt is not a target.")
    endif()
    get_target_property(has_headers ${target} _qt_module_has_headers)
    if(NOT has_headers)
        return()
    endif()

    qt_internal_module_info(module "${target}")

    get_target_property(sync_source_directory ${target} _qt_sync_source_directory)
    set(syncqt_timestamp "${CMAKE_CURRENT_BINARY_DIR}/${target}_syncqt_timestamp")
    set(syncqt_outputs "${syncqt_timestamp}")

    set(is_interface_lib FALSE)
    get_target_property(type ${target} TYPE)
    if(type STREQUAL "INTERFACE_LIBRARY")
        set(is_interface_lib TRUE)
    endif()

    set(version_script_private_content_file "")
    if(NOT is_interface_lib)
        list(APPEND syncqt_outputs
            "${module_build_interface_include_dir}/${module}Version"
            "${module_build_interface_include_dir}/qt${module_lower}version.h")
        if(TEST_ld_version_script)
            set(version_script_private_content_file
                "${CMAKE_CURRENT_BINARY_DIR}/${target}.version.private_content")
            set(version_script_args
                "-versionScript" "${version_script_private_content_file}")
            list(APPEND syncqt_outputs "${version_script_private_content_file}")
            qt_internal_add_linker_version_script(${target}
                PRIVATE_CONTENT_FILE "${version_script_private_content_file}")
        endif()
    endif()

    # Check for _qt_module_is_3rdparty_header_library flag to detect non-Qt modules and
    # indicate this to syncqt.
    get_target_property(is_3rd_party_library ${target} _qt_module_is_3rdparty_header_library)
    set(non_qt_module_argument "")
    if(is_3rd_party_library)
        set(non_qt_module_argument "-nonQt")
    else()
        list(APPEND syncqt_outputs "${module_build_interface_include_dir}/${module}")
        get_target_property(no_headersclean_check ${target} _qt_no_headersclean_check)
        if(NOT no_headersclean_check)
            list(APPEND syncqt_outputs
                "${CMAKE_CURRENT_BINARY_DIR}/${module}_header_check_exceptions")
        endif()
    endif()

    set(is_framework FALSE)
    if(NOT is_interface_lib)
        get_target_property(is_framework ${target} FRAMEWORK)
        if(is_framework)
            qt_internal_get_framework_info(fw ${target})
            get_target_property(fw_output_base_dir ${target} LIBRARY_OUTPUT_DIRECTORY)
            set(framework_args "-framework"
                "-frameworkIncludeDir" "${fw_output_base_dir}/${fw_versioned_header_dir}"
            )
        endif()
    endif()

    qt_internal_get_qt_all_known_modules(known_modules)

    get_target_property(is_internal_module ${target} _qt_is_internal_module)
    set(internal_module_argument "")
    if(is_internal_module)
        set(internal_module_argument "-internal")
    endif()

    get_target_property(qpa_filter_regex ${target} _qt_module_qpa_headers_filter_regex)
    get_target_property(private_filter_regex ${target} _qt_module_private_headers_filter_regex)

    # We need to use the real paths since otherwise it may lead to the invalid work of the
    # std::filesystem API
    get_filename_component(source_dir_real "${sync_source_directory}" REALPATH)
    get_filename_component(binary_dir_real "${CMAKE_CURRENT_BINARY_DIR}" REALPATH)

    if(QT_REPO_PUBLIC_NAMESPACE_REGEX)
        set(public_namespaces_filter -publicNamespaceFilter "${QT_REPO_PUBLIC_NAMESPACE_REGEX}")
    endif()

    if(qpa_filter_regex)
        set(qpa_filter_argument
            -qpaHeadersFilter "${qpa_filter_regex}"
        )
    endif()

    set(common_syncqt_arguments
        -module "${module}"
        -sourceDir "${source_dir_real}"
        -binaryDir "${binary_dir_real}"
        -privateHeadersFilter "${private_filter_regex}"
        -includeDir "${module_build_interface_include_dir}"
        -privateIncludeDir "${module_build_interface_private_include_dir}"
        -qpaIncludeDir "${module_build_interface_qpa_include_dir}"
        ${qpa_filter_argument}
        ${public_namespaces_filter}
        ${non_qt_module_argument}
        ${internal_module_argument}
    )

    if(QT_INTERNAL_ENABLE_SYNCQT_DEBUG_OUTPUT)
        list(APPEND common_syncqt_arguments -debug)
    endif()

    set(build_time_syncqt_arguments "")
    if(WARNINGS_ARE_ERRORS)
        if(is_interface_lib)
            set(warnings_are_errors_enabled_genex 1)
        else()
            set(warnings_are_errors_enabled_genex
                "$<NOT:$<BOOL:$<TARGET_PROPERTY:${target},QT_SKIP_WARNINGS_ARE_ERRORS>>>")
        endif()
        list(APPEND build_time_syncqt_arguments
            "$<${warnings_are_errors_enabled_genex}:-warningsAreErrors>")
    endif()

    if(is_framework)
        list(REMOVE_ITEM module_headers "${CMAKE_CURRENT_BINARY_DIR}/${target}_fake_header.h")
    endif()

    # Filter the generated ui_ header files and header files located in the 'doc/' subdirectory.
    list(FILTER module_headers EXCLUDE REGEX
        "(.+/(ui_)[^/]+\\.h|${CMAKE_CURRENT_SOURCE_DIR}(/.+)?/doc/+\\.h)")

    set(module_headers_rsp "${binary_dir_real}/${target}_module_headers")
    list(JOIN module_headers "\n" module_headers_string)
    qt_configure_file_v2(OUTPUT "${module_headers_rsp}" CONTENT "${module_headers_string}")

    set(module_headers_generated_rsp "${binary_dir_real}/${target}_module_headers_generated")
    list(JOIN module_headers_generated "\n" module_headers_generated_string)
    qt_configure_file_v2(OUTPUT "${module_headers_generated_rsp}" CONTENT
        "${module_headers_generated_string}")

    set(syncqt_staging_dir "${module_build_interface_include_dir}/.syncqt_staging")
    add_custom_command(
        OUTPUT
            ${syncqt_outputs}
        COMMAND
            ${QT_CMAKE_EXPORT_NAMESPACE}::syncqt
            ${common_syncqt_arguments}
            ${build_time_syncqt_arguments}
            -headers "@${module_headers_rsp}"
            -generatedHeaders "@${module_headers_generated_rsp}"
            -stagingDir "${syncqt_staging_dir}"
            -knownModules ${known_modules}
            ${framework_args}
            ${version_script_args}
        COMMAND
            ${CMAKE_COMMAND} -E touch "${syncqt_timestamp}"
        DEPENDS
            ${module_headers_rsp}
            ${module_headers_generated_rsp}
            ${module_headers}
            ${QT_CMAKE_EXPORT_NAMESPACE}::syncqt
        COMMENT
            "Running syncqt.cpp for module: ${module}"
        VERBATIM
    )
    add_custom_target(${target}_sync_headers
        DEPENDS
            ${syncqt_outputs}
    )
    add_dependencies(sync_headers ${target}_sync_headers)

    # This target is required when building docs, to make all header files and their aliases
    # available for qdoc.
    # ${target}_sync_headers is added as dependency to make sure that
    # ${target}_sync_all_public_headers is running after ${target}_sync_headers, when building docs.
    add_custom_target(${target}_sync_all_public_headers
        COMMAND
            ${QT_CMAKE_EXPORT_NAMESPACE}::syncqt
            ${common_syncqt_arguments}
            -all
        DEPENDS
            ${module_headers}
            ${QT_CMAKE_EXPORT_NAMESPACE}::syncqt
            ${target}_sync_headers
        VERBATIM
    )

    if(NOT TARGET sync_all_public_headers)
        add_custom_target(sync_all_public_headers)
    endif()
    add_dependencies(sync_all_public_headers ${target}_sync_all_public_headers)

    if(NOT is_3rd_party_library AND NOT is_framework)
        # Install all the CaMeL style aliases of header files from the staging directory in one rule
        qt_install(DIRECTORY "${syncqt_staging_dir}/"
            DESTINATION "${module_install_interface_include_dir}"
        )
    endif()

    if(NOT is_interface_lib)
        set_property(TARGET ${target}
            APPEND PROPERTY AUTOGEN_TARGET_DEPENDS "${target}_sync_headers")
    endif()
    add_dependencies(${target} "${target}_sync_headers")


    get_target_property(private_module_target ${target} _qt_private_module_target_name)
    if(private_module_target)
        add_dependencies(${private_module_target} "${target}_sync_headers")
    endif()

    # Run sync Qt first time at configure step to make all header files available for the code model
    # of IDEs.
    get_property(synced_modules GLOBAL PROPERTY _qt_synced_modules)
    if(NOT "${module}" IN_LIST synced_modules)
        message(STATUS "Running syncqt.cpp for module: ${module}")
        get_target_property(syncqt_location ${QT_CMAKE_EXPORT_NAMESPACE}::syncqt LOCATION)
        execute_process(
            COMMAND
                ${syncqt_location}
                ${common_syncqt_arguments}
                -headers "@${module_headers_rsp}"
                -generatedHeaders "@${module_headers_generated_rsp}"
                -stagingDir "${syncqt_staging_dir}"
                -knownModules ${known_modules}
                ${framework_args}
            RESULT_VARIABLE syncqt_result
            OUTPUT_VARIABLE syncqt_output
            ERROR_VARIABLE syncqt_output
        )
        if(NOT syncqt_result EQUAL 0)
            message(FATAL_ERROR
                "syncqt.cpp failed for module ${module}:\n${syncqt_output}")
        endif()
        if(syncqt_output)
            message(WARNING "${syncqt_output}")
        endif()
        set_property(GLOBAL APPEND PROPERTY _qt_synced_modules ${module})
    endif()
endfunction()

function(qt_internal_collect_sync_header_dependencies out_var skip_non_existing)
    list(LENGTH ARGN sync_headers_target_count)
    if(sync_headers_target_count EQUAL 0)
        message(FATAL_ERROR "Invalid use of qt_internal_collect_sync_header_dependencies,"
            " dependencies are not specified")
    endif()

    set(${out_var} "")
    foreach(sync_headers_target IN LISTS ARGN)
        set(sync_headers_target "${sync_headers_target}_sync_headers")
        if(NOT skip_non_existing OR TARGET ${sync_headers_target})
            list(APPEND ${out_var} ${sync_headers_target})
        endif()
    endforeach()
    list(REMOVE_DUPLICATES ${out_var})

    set(${out_var} "${${out_var}}" PARENT_SCOPE)
endfunction()

function(qt_internal_add_sync_header_dependencies target)
    qt_internal_collect_sync_header_dependencies(sync_headers_targets FALSE ${ARGN})
    if(sync_headers_targets)
        add_dependencies(${target} ${sync_headers_targets})
    endif()
endfunction()

function(qt_internal_add_autogen_sync_header_dependencies target)
    qt_internal_collect_sync_header_dependencies(sync_headers_targets TRUE ${ARGN})
    foreach(sync_headers_target IN LISTS sync_headers_targets)
        set_property(TARGET ${target} APPEND PROPERTY AUTOGEN_TARGET_DEPENDS
            "${sync_headers_target}")
    endforeach()
endfunction()
