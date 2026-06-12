# Copyright © 2025 CCP ehf.
# Overwrite possibly existing ${_CTEST_FILE} with empty file
set(flush_tests_MODE WRITE)

# Flushes script to ${_CTEST_FILE}
macro(flush_script)
    file(${flush_tests_MODE} "${_CTEST_FILE}" "${script}")
    set(flush_tests_MODE APPEND)

    set(script "")
endmacro()

# Flushes tests_buffer to tests
macro(flush_tests_buffer)
    list(APPEND tests "${tests_buffer}")
    set(tests_buffer "")
endmacro()

macro(add_command NAME)
    set(_args "")
    foreach(_arg ${ARGN})
        if(_arg MATCHES "[^-./:a-zA-Z0-9_]")
            string(APPEND _args " [==[${_arg}]==]")
        else()
            string(APPEND _args " ${_arg}")
        endif()
    endforeach()
    string(APPEND script "${NAME}(${_args})\n")
    string(LENGTH "${script}" _script_len)
    if(${_script_len} GREATER "50000")
        flush_script()
    endif()
    # Unsets macro local variables to prevent leakage outside of this macro.
    unset(_args)
    unset(_script_len)
endmacro()

function(pytest_discover_tests_impl)

    cmake_parse_arguments(
        ""
        ""
        "NO_PRETTY_TYPES;NO_PRETTY_VALUES;TEST_EXECUTABLE;TEST_WORKING_DIR;TEST_PREFIX;TEST_SUFFIX;TEST_LIST;CTEST_FILE;TEST_DISCOVERY_TIMEOUT;TEST_XML_OUTPUT_DIR"
        "TEST_EXTRA_ARGS;TEST_PROPERTIES;TEST_EXECUTOR"
        ${ARGN}
    )

    set(prefix "${_TEST_PREFIX}")
    set(suffix "${_TEST_SUFFIX}")
    set(extra_args ${_TEST_EXTRA_ARGS})
    set(properties ${_TEST_PROPERTIES})
    set(script)
    set(suite)
    set(tests)
    set(tests_buffer)

    # Run test executable to get list of available tests
    if(NOT EXISTS "${_TEST_EXECUTABLE}")
        message(FATAL_ERROR
            "Specified test executable does not exist.\n"
            "  Path: '${_TEST_EXECUTABLE}'"
            )
    endif()
    message(DEBUG "TEST_WORKING_DIR=${_TEST_WORKING_DIR}")
    execute_process(
        COMMAND ${_TEST_EXECUTOR} "${_TEST_EXECUTABLE}" -m pytest --collect-only -p no:cacheprovider -m "not skip"
        WORKING_DIRECTORY "${_TEST_WORKING_DIR}"
        #TIMEOUT ${_TEST_DISCOVERY_TIMEOUT}
        OUTPUT_VARIABLE output
        RESULT_VARIABLE result
    )
    if(NOT ${result} EQUAL 0)
        string(REPLACE "\n" "\n    " output "${output}")
        message(FATAL_ERROR
            "Error running test executable.\n"
            "  Path: '${_TEST_EXECUTABLE}'\n"
            "  Result: ${result}\n"
            "  Output:\n"
            "    ${output}\n"
            )
    endif()
    message(DEBUG "Ran pytest collect. Got:")
    message(DEBUG ${output})
    message(DEBUG "---")

    # Preserve semicolon in test-parameters
    string(REPLACE [[;]] [[\;]] output "${output}")
    string(REPLACE "\n" ";" output "${output}")

    # Parse output
    foreach(line ${output})

        # Suite name
        if(line MATCHES "<UnitTestCase (.*)>")
            set(suite ${CMAKE_MATCH_1})
            message(DEBUG "Got suite ${suite}")
            set(pretty_suite "${suite}")

        # Test name
        elseif(line MATCHES "<TestCaseFunction (.*)>")
            set(test ${CMAKE_MATCH_1})
            message(DEBUG "Got test ${test}")
            set(pretty_test "${test}")

            if(NOT "${_TEST_XML_OUTPUT_DIR}" STREQUAL "")
                set(TEST_XML_OUTPUT_PARAM "--junitxml=${_TEST_XML_OUTPUT_DIR}/${prefix}.${suite}.${test}${suffix}.xml")
            else()
                unset(TEST_XML_OUTPUT_PARAM)
            endif()

            # sanitize test name for further processing downstream
            set(testname "${prefix}.${pretty_suite}.${pretty_test}${suffix}")
            # escape \
            string(REPLACE [[\]] [[\\]] testname "${testname}")
            # escape ;
            string(REPLACE [[;]] [[\;]] testname "${testname}")
            # escape $
            string(REPLACE [[$]] [[\$]] testname "${testname}")

            # ...and add to script
            add_command(add_test
                "${testname}"
                ${_TEST_EXECUTOR}
                "${_TEST_EXECUTABLE}"
                "-m"
                "pytest"
                "-p"
                "no:cacheprovider"
                "-k"
                "${suite} and ${test}"
                ${TEST_XML_OUTPUT_PARAM}
                ${extra_args}
                )
            add_command(set_tests_properties
                "${testname}"
                PROPERTIES
                WORKING_DIRECTORY "${_TEST_WORKING_DIR}"
                SKIP_REGULAR_EXPRESSION "\\\\[  SKIPPED \\\\]"
                ${properties}
                )
            list(APPEND tests_buffer "${testname}")
            list(LENGTH tests_buffer tests_buffer_length)
            if(${tests_buffer_length} GREATER "250")
                flush_tests_buffer()
            endif()
        endif()
    endforeach()


    # Create a list of all discovered tests, which users may use to e.g. set
    # properties on the tests
    flush_tests_buffer()
    add_command(set ${_TEST_LIST} ${tests})

    # Write CTest script
    flush_script()

endfunction()

if(CMAKE_SCRIPT_MODE_FILE)
    pytest_discover_tests_impl(
        NO_PRETTY_TYPES ${NO_PRETTY_TYPES}
        NO_PRETTY_VALUES ${NO_PRETTY_VALUES}
        TEST_EXECUTABLE ${TEST_EXECUTABLE}
        TEST_EXECUTOR ${TEST_EXECUTOR}
        TEST_WORKING_DIR ${TEST_WORKING_DIR}
        TEST_PREFIX ${TEST_PREFIX}
        TEST_SUFFIX ${TEST_SUFFIX}
        TEST_LIST ${TEST_LIST}
        CTEST_FILE ${CTEST_FILE}
        TEST_DISCOVERY_TIMEOUT ${TEST_DISCOVERY_TIMEOUT}
        TEST_XML_OUTPUT_DIR ${TEST_XML_OUTPUT_DIR}
        TEST_EXTRA_ARGS ${TEST_EXTRA_ARGS}
        TEST_PROPERTIES ${TEST_PROPERTIES}
    )
endif()
