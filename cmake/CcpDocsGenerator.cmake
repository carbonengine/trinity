# Copyright © 2025 CCP ehf.
function(_create_venv_and_install_packages)
    set(options "")
    set(single_value_keywords "PYTHON_EXE;VENV_NAME")
    set(multi_value_keywords "PIP_PACKAGES")
    cmake_parse_arguments(PARSE_ARGV 0 "arg"
            "${options}" "${single_value_keywords}" "${multi_value_keywords}"
    )

    execute_process(
            COMMAND ${arg_PYTHON_EXE} -m venv ${arg_VENV_NAME}
            WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    )

    if(WIN32)
        set(PIP_EXE ${CMAKE_CURRENT_BINARY_DIR}/${arg_VENV_NAME}/Scripts/pip)
    elseif(APPLE)
        set(PIP_EXE ${CMAKE_CURRENT_BINARY_DIR}/${arg_VENV_NAME}/bin/pip)
    endif()

    execute_process(
            COMMAND ${PIP_EXE} install ${arg_PIP_PACKAGES}
            WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    )
endfunction()

function(create_carbon_docs_sphinx_target)
    set(options "")
    set(single_value_keywords "SPHINX_SOURCE;SPHINX_BUILD;SPHINX_TARGET_NAME;INSTALL_DESTINATION;DOXYGEN_TARGET_NAME;PYTHON_EXE;VENV_NAME")
    set(multi_value_keywords "DOXYGEN_SRC_FILES;PYTHONPATH_ENV")
    cmake_parse_arguments(PARSE_ARGV 0 "arg"
            "${options}" "${single_value_keywords}" "${multi_value_keywords}"
    )

    find_package(Doxygen REQUIRED)

    set(PIP_PACKAGES sphinx breathe myst_parser sphinx_rtd_theme)

    _create_venv_and_install_packages(
            PYTHON_EXE ${arg_PYTHON_EXE}
            VENV_NAME ${arg_VENV_NAME}
            PIP_PACKAGES ${PIP_PACKAGES}
    )

    # Evaluate config file for Doxygen to input project values
    set(DOXYFILE_IN ${CMAKE_CURRENT_SOURCE_DIR}/doc/Doxyfile.in)
    set(DOXYFILE_OUT ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile)
    set(DOXYGEN_INDEX_FILE ${CMAKE_CURRENT_BINARY_DIR}/xml/index.xml)
    configure_file(${DOXYFILE_IN} ${DOXYFILE_OUT} @ONLY)

    # Regenerate with source changes
    add_custom_command(OUTPUT ${DOXYGEN_INDEX_FILE}
            DEPENDS ${arg_DOXYGEN_SRC_FILES}
            COMMAND ${DOXYGEN_EXECUTABLE} ${DOXYFILE_OUT}
            MAIN_DEPENDENCY ${DOXYFILE_OUT} ${DOXYFILE_IN}
            COMMENT "Running Doxygen"
            VERBATIM)

    add_custom_target(${arg_DOXYGEN_TARGET_NAME} ALL DEPENDS ${DOXYGEN_INDEX_FILE})

    if (WIN32)
        set(SPHINX_COMMAND Scripts/sphinx-build -E -b html -D breathe_projects.doxygen=${CMAKE_CURRENT_BINARY_DIR}/docs/xml -c ${arg_SPHINX_SOURCE} ${arg_SPHINX_SOURCE} ${arg_SPHINX_BUILD})
    elseif(APPLE)
        set(SPHINX_COMMAND bin/sphinx-build -E -b html -D breathe_projects.doxygen=${CMAKE_CURRENT_BINARY_DIR}/docs/xml -c ${arg_SPHINX_SOURCE} ${arg_SPHINX_SOURCE} ${arg_SPHINX_BUILD})
    endif()

    message(STATUS "Working directory is ${CMAKE_CURRENT_BINARY_DIR}")
    message(STATUS "Command: ${SPHINX_COMMAND}")

    add_custom_target(${arg_SPHINX_TARGET_NAME} ALL
            COMMAND ${CMAKE_COMMAND} -E env PYTHONPATH=${arg_PYTHONPATH_ENV} ${SPHINX_COMMAND}
            WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/${arg_VENV_NAME}
            DEPENDS ${arg_DOXYGEN_TARGET_NAME}
            COMMENT "Generating documentation with Sphinx")

    # Install rule for documentation
    install(DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/doc/build/ DESTINATION ${arg_INSTALL_DESTINATION})
endfunction()