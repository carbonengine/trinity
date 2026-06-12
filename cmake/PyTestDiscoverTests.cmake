# Copyright © 2025 CCP ehf.
#[[
  Automatically add tests with CTest by querying the compiled test executable
  for available tests.

    pytest_discover_tests(target
                         [EXTRA_ARGS arg1...]
                         [WORKING_DIRECTORY dir]
                         [TEST_PREFIX prefix]
                         [TEST_SUFFIX suffix]
                         [TEST_FILTER expr]
                         [NO_PRETTY_TYPES] [NO_PRETTY_VALUES]
                         [PROPERTIES name1 value1...]
                         [TEST_LIST var]
                         [DISCOVERY_TIMEOUT seconds]
                         [XML_OUTPUT_DIR dir]
                         [DISCOVERY_MODE <POST_BUILD|PRE_TEST>]
    )
]]

function(pytest_discover_tests TARGET)
    cmake_parse_arguments(
        ""
        "NO_PRETTY_TYPES;NO_PRETTY_VALUES"
        "TEST_PREFIX;TEST_SUFFIX;WORKING_DIRECTORY;TEST_LIST;DISCOVERY_TIMEOUT;XML_OUTPUT_DIR;DISCOVERY_MODE"
        "EXTRA_ARGS;PROPERTIES"
        ${ARGN}
    )

    if(NOT Python_EXECUTABLE)
        message(FATAL_ERROR "Failed discovering pytest tests because the Python_EXECUTABLE variable is not set.")
    endif()

    if(NOT _WORKING_DIRECTORY)
        set(_WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}")
    endif()
    if(NOT _TEST_LIST)
        set(_TEST_LIST ${TARGET}_TESTS)
    endif()
    if(NOT _DISCOVERY_TIMEOUT)
        set(_DISCOVERY_TIMEOUT 5)
    endif()
    if(NOT _DISCOVERY_MODE)
        if(NOT CMAKE_GTEST_DISCOVER_TESTS_DISCOVERY_MODE)
            set(CMAKE_GTEST_DISCOVER_TESTS_DISCOVERY_MODE "POST_BUILD")
        endif()
        set(_DISCOVERY_MODE ${CMAKE_GTEST_DISCOVER_TESTS_DISCOVERY_MODE})
    endif()

    get_property(
        has_counter
        TARGET ${TARGET}
        PROPERTY CTEST_DISCOVERED_TEST_COUNTER
        SET
    )
    if(has_counter)
        get_property(
            counter
            TARGET ${TARGET}
            PROPERTY CTEST_DISCOVERED_TEST_COUNTER
        )
        math(EXPR counter "${counter} + 1")
    else()
        set(counter 1)
    endif()
    set_property(
        TARGET ${TARGET}
        PROPERTY CTEST_DISCOVERED_TEST_COUNTER
        ${counter}
    )

    # Define rule to generate test list for aforementioned test executable
    set(ctest_file_base "${CMAKE_CURRENT_BINARY_DIR}/${TARGET}[${counter}]")
    set(ctest_include_file "${ctest_file_base}_include.cmake")
    set(ctest_tests_file "${ctest_file_base}_tests.cmake")
    get_property(crosscompiling_emulator
        TARGET ${TARGET}
        PROPERTY CROSSCOMPILING_EMULATOR
        )

    if(_DISCOVERY_MODE STREQUAL "POST_BUILD")
        add_custom_command(
            TARGET ${TARGET} POST_BUILD
            BYPRODUCTS "${ctest_tests_file}"
            COMMAND "${CMAKE_COMMAND}"
            -D "TEST_TARGET=${TARGET}"
            -D "TEST_EXECUTABLE=${Python_EXECUTABLE}"
            -D "TEST_EXECUTOR=${crosscompiling_emulator}"
            -D "TEST_WORKING_DIR=${_WORKING_DIRECTORY}"
            -D "TEST_EXTRA_ARGS=${_EXTRA_ARGS}"
            -D "TEST_PROPERTIES=${_PROPERTIES}"
            -D "TEST_PREFIX=${_TEST_PREFIX}"
            -D "TEST_SUFFIX=${_TEST_SUFFIX}"
            -D "NO_PRETTY_TYPES=${_NO_PRETTY_TYPES}"
            -D "NO_PRETTY_VALUES=${_NO_PRETTY_VALUES}"
            -D "TEST_LIST=${_TEST_LIST}"
            -D "CTEST_FILE=${ctest_tests_file}"
            -D "TEST_DISCOVERY_TIMEOUT=${_DISCOVERY_TIMEOUT}"
            -D "TEST_XML_OUTPUT_DIR=${_XML_OUTPUT_DIR}"
            -P "${_PYTEST_DISCOVER_TESTS_SCRIPT}"
            VERBATIM
        )

        file(WRITE "${ctest_include_file}"
            "if(EXISTS \"${ctest_tests_file}\")\n"
            "  include(\"${ctest_tests_file}\")\n"
            "else()\n"
            "  add_test(${TARGET}_NOT_BUILT ${TARGET}_NOT_BUILT)\n"
            "endif()\n"
            )
    elseif(_DISCOVERY_MODE STREQUAL "PRE_TEST")

        get_property(GENERATOR_IS_MULTI_CONFIG GLOBAL
            PROPERTY GENERATOR_IS_MULTI_CONFIG
            )

        if(GENERATOR_IS_MULTI_CONFIG)
            set(ctest_tests_file "${ctest_file_base}_tests-$<CONFIG>.cmake")
        endif()

        string(CONCAT ctest_include_content
            "if(EXISTS \"$<TARGET_FILE:${TARGET}>\")"                                    "\n"
            "  if(NOT EXISTS \"${ctest_tests_file}\" OR"                                 "\n"
            "     NOT \"${ctest_tests_file}\" IS_NEWER_THAN \"$<TARGET_FILE:${TARGET}>\")" "\n"
            "    pytest_discover_tests_impl("                                             "\n"
            "      TEST_EXECUTABLE"        " [==[" "$<TARGET_FILE:${TARGET}>"   "]==]"   "\n"
            "      TEST_EXECUTOR"          " [==[" "${crosscompiling_emulator}" "]==]"   "\n"
            "      TEST_WORKING_DIR"       " [==[" "${_WORKING_DIRECTORY}"      "]==]"   "\n"
            "      TEST_EXTRA_ARGS"        " [==[" "${_EXTRA_ARGS}"             "]==]"   "\n"
            "      TEST_PROPERTIES"        " [==[" "${_PROPERTIES}"             "]==]"   "\n"
            "      TEST_PREFIX"            " [==[" "${_TEST_PREFIX}"            "]==]"   "\n"
            "      TEST_SUFFIX"            " [==[" "${_TEST_SUFFIX}"            "]==]"   "\n"
            "      NO_PRETTY_TYPES"        " [==[" "${_NO_PRETTY_TYPES}"        "]==]"   "\n"
            "      NO_PRETTY_VALUES"       " [==[" "${_NO_PRETTY_VALUES}"       "]==]"   "\n"
            "      TEST_LIST"              " [==[" "${_TEST_LIST}"              "]==]"   "\n"
            "      CTEST_FILE"             " [==[" "${ctest_tests_file}"        "]==]"   "\n"
            "      TEST_DISCOVERY_TIMEOUT" " [==[" "${_DISCOVERY_TIMEOUT}"      "]==]"   "\n"
            "      TEST_XML_OUTPUT_DIR"    " [==[" "${_XML_OUTPUT_DIR}"         "]==]"   "\n"
            "    )"                                                                      "\n"
            "  endif()"                                                                  "\n"
            "  include(\"${ctest_tests_file}\")"                                         "\n"
            "else()"                                                                     "\n"
            "  add_test(${TARGET}_NOT_BUILT ${TARGET}_NOT_BUILT)"                        "\n"
            "endif()"                                                                    "\n"
            )

        if(GENERATOR_IS_MULTI_CONFIG)
            foreach(_config ${CMAKE_CONFIGURATION_TYPES})
                file(GENERATE OUTPUT "${ctest_file_base}_include-${_config}.cmake" CONTENT "${ctest_include_content}" CONDITION $<CONFIG:${_config}>)
            endforeach()
            file(WRITE "${ctest_include_file}" "include(\"${ctest_file_base}_include-\${CTEST_CONFIGURATION_TYPE}.cmake\")")
        else()
            file(GENERATE OUTPUT "${ctest_file_base}_include.cmake" CONTENT "${ctest_include_content}")
            file(WRITE "${ctest_include_file}" "include(\"${ctest_file_base}_include.cmake\")")
        endif()

    else()
        message(FATAL_ERROR "Unknown DISCOVERY_MODE: ${_DISCOVERY_MODE}")
    endif()

    # Add discovered tests to directory TEST_INCLUDE_FILES
    set_property(DIRECTORY
        APPEND PROPERTY TEST_INCLUDE_FILES "${ctest_include_file}"
    )
endfunction()

set(_PYTEST_DISCOVER_TESTS_SCRIPT
    ${CMAKE_CURRENT_LIST_DIR}/internal/PytestDiscoverTestsImpl.cmake
)
