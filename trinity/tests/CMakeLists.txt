project(TrinityEngineTest VERSION 1.0.0)

# ---------------------------------------------------------------------------------------------
# Engine-level unit tests (no GPU / scene required) — e.g. EveComponentRegistry bookkeeping.
#
# WHY THIS COMPILES ENGINE SOURCES DIRECTLY (instead of linking trinity_dx12):
#   The trinity engine is a SHARED library and its BLUE_CLASS types are NOT dllexported
#   (CXX_VISIBILITY_PRESET hidden + no __declspec(dllexport)). So EveComponentRegistry /
#   EveEntity symbols can't be called across the DLL boundary from a separate test exe.
#   Instead we compile the small set of engine .cpp files under test directly into the test
#   binary, with the same include dirs / compile defs the engine uses.
#
# BUILD-ITERATION PLAYBOOK (this target was authored without a local build; expect to tweak):
#   * Unresolved external "g_eveReflectionMode": provided as a stub in EveComponentRegistryTest.cpp.
#   * Unresolved CCP_LOGERR / CCP_STATS_ZONE / CCP_ASSERT symbols: add the providing target to
#     target_link_libraries (CcpCore is the usual one; Blue may pull it transitively).
#   * We build with TRINITY_AL_WITH_BLUE_EXPOSURE=1 (same as the engine). This is REQUIRED:
#     with =0, ALResult.h redefines Be helpers that BlueTypes.h already provides, and
#     EXPOSE_TO_BLUE() does not implement IRoot's pure virtuals so EveEntity/EveComponentRegistry
#     stay abstract and can't be instantiated. If the linker then misses Blue type/reflection
#     symbols, add ../Eve/EveEntity_Blue.cpp and ../Eve/EveComponentRegistry_Blue.cpp to _SOURCES.
#   * A header included transitively (e.g. Tr2Renderer.h via EveEntity.cpp) fails to parse:
#     add the missing include dir / compile define here to match trinity/CMakeLists.txt
#     (set_shared_trinity_properties).
#   * "duplicate symbol main": we link GTest::GTest only (NOT GTest::Main) and provide our own
#     main() in TrinityEngineTest.cpp.
#   * Cannot instantiate TestComponentEntity (abstract / Blue pure virtuals): EveEntity may
#     need its BLUE_CLASS declaration or Blue init even with exposure off — see test file.
# ---------------------------------------------------------------------------------------------

# Engine sources compiled directly into the test exe. We can't link the trinity_dx12 SHARED
# lib (its BLUE_CLASS symbols aren't dllexported), so the classes under test are built in here.
# Blue exposure glue (*_Blue.cpp) defines EveEntity::ExposeToBlue / EveComponentRegistry::
# ExposeToBlue, which the classes' vtables reference via the EXPOSE_TO_BLUE-generated ClassType().
set(_ENGINE_SOURCES_UNDER_TEST
    ../Eve/EveComponentRegistry.cpp
    ../Eve/EveEntity.cpp
    ../Eve/EveComponentRegistry_Blue.cpp
    ../Eve/EveEntity_Blue.cpp
)

set(_SOURCES
    TrinityEngineTest.cpp
    EveComponentRegistryTest.cpp
    ${_ENGINE_SOURCES_UNDER_TEST}
)

# These engine files show up in this target's IDE project because they're compiled into it
# (VS reflects a target's full source list). Group them under a clearly-labeled filter so it's
# obvious they're borrowed engine code, not test-owned files.
source_group("Engine sources (compiled-in)" FILES ${_ENGINE_SOURCES_UNDER_TEST})

find_package(GTest REQUIRED)
include(GoogleTest)   # provides gtest_discover_tests()
enable_testing()      # so the gtest cases register with CTest (VS Test Explorer reads CTest)

function(set_shared_trinityenginetest_properties target)

    target_include_directories(${target}
        PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR}
            ${CMAKE_CURRENT_SOURCE_DIR}/..          # trinity/        -> "StdAfx.h", "Eve/..."
            ${CMAKE_CURRENT_SOURCE_DIR}/../include   # trinity/include
            ${CMAKE_CURRENT_SOURCE_DIR}/../..        # repo root       -> cross-component includes
            # The engine StdAfx.h transitively includes headers from several third-party libs
            # that the engine pulls in only for their include dirs. We don't call into them, so
            # borrow just their INTERFACE_INCLUDE_DIRECTORIES (no link). Mirrors the deps in
            # set_shared_trinity_properties. If another header turns up missing, add its target
            # here the same way (or link it if you also need its symbols).
            "$<TARGET_PROPERTY:PNG::PNG,INTERFACE_INCLUDE_DIRECTORIES>"               # png.h
            "$<TARGET_PROPERTY:RadGameTools::Granny,INTERFACE_INCLUDE_DIRECTORIES>"   # granny.h
            "$<TARGET_PROPERTY:TrinityAL_dx12,INTERFACE_INCLUDE_DIRECTORIES>"         # TrinityAL.h
            "$<TARGET_PROPERTY:ImageIO,INTERFACE_INCLUDE_DIRECTORIES>"                # Tr2ImageHandler.h / HostBitmap.h
            "$<TARGET_PROPERTY:CarbonMesh,INTERFACE_INCLUDE_DIRECTORIES>"             # cmf/*.h  (added for main)
    )

    # Match the engine's compile environment (see set_shared_trinity_properties in
    # trinity/CMakeLists.txt). Blue exposure MUST be ON (=1): the engine compiles these files
    # with it on, and it's what makes EveEntity/EveComponentRegistry concrete (IRoot impls) and
    # avoids the ALResult.h/BlueTypes.h Be-helper redefinition.
    target_compile_definitions(${target}
        PRIVATE
            TRINITY_PLATFORM=TRINITY_DIRECTX12
            BLUE_OVERRIDE_VECTOR_TYPES=1
            TRINITYBUILD
            TRINITY_AL_WITH_BLUE_EXPOSURE=1
            BLUEBUILD
    )

    target_link_libraries(${target}
        PRIVATE
            Blue
            CcpMath
            GTest::GTest
    )

    if(WIN32)
        # The Release blue.dll (carbon-blue) bundles and re-exports the carbon-core /
        # carbon-blueexposure symbols, which are ALSO pulled in as static libs through the
        # Blue / CcpMath usage requirements. That double definition is harmless here (same
        # code, and the duplicated functions aren't executed by these tests — we never call
        # ClassType()/ExposeToBlue() at runtime), so take the first (blue.dll) definition
        # instead of failing with LNK2005.
        target_link_options(${target} PRIVATE
            "/IGNORE:4049"
            "/FORCE:MULTIPLE"
            "/IGNORE:4006"   # "symbol already defined; second definition ignored"
            "/IGNORE:4088"   # "image may not run correctly due to /FORCE"
        )
    endif()

    # Keep all test outputs in one folder, differentiated by OUTPUT_NAME (mirrors TrinityALTest).
    set(output_directory "${CMAKE_SOURCE_DIR}/carbon/autobuild/TrinityEngineTest/${CCP_PLATFORM}/${CCP_ARCHITECTURE}/${CCP_TOOLSET}/$<$<CONFIG:DEBUG>:>")
    message(STATUS "Overriding ${target} output directory to ${output_directory}")
    set_target_properties(${target}
        PROPERTIES
            RUNTIME_OUTPUT_DIRECTORY ${output_directory}
            PDB_OUTPUT_DIRECTORY ${output_directory}
            FOLDER Trinity/Tests
    )

endfunction()

if(WIN32)
    ccp_add_executable(TrinityEngineTest_dx12 ${_SOURCES})
    set_shared_trinityenginetest_properties(TrinityEngineTest_dx12)

    # Copy runtime DLL dependencies (Blue.dll, etc.) next to the test exe so it can launch.
    add_custom_command(
        TARGET TrinityEngineTest_dx12 POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_RUNTIME_DLLS:TrinityEngineTest_dx12> $<TARGET_FILE_DIR:TrinityEngineTest_dx12>
        COMMAND_EXPAND_LISTS
    )

    # blue.dll imports CCP Stackless-Python exports (PyStackless_*/PyTasklet_*/PyChannel_*).
    # $<TARGET_RUNTIME_DLLS> does NOT include python27.dll, and the vanilla CPython in System32
    # lacks those exports (-> STATUS_ENTRYPOINT_NOT_FOUND on launch). Deploy the matching
    # Stackless python27.dll from the vendor tree next to the exe.
    file(GLOB _stackless_python27
        "${BRANCH_ROOT_DIR}/vendor/python/*ccp-stackless*/bin/${CCP_PLATFORM}/${CCP_ARCHITECTURE}/${CCP_TOOLSET}/python27.dll")
    if(_stackless_python27)
        list(GET _stackless_python27 0 _stackless_python27)
        add_custom_command(
            TARGET TrinityEngineTest_dx12 POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different "${_stackless_python27}" "$<TARGET_FILE_DIR:TrinityEngineTest_dx12>"
        )
    else()
        message(WARNING "Stackless python27.dll not found under ${BRANCH_ROOT_DIR}/vendor/python; "
                        "TrinityEngineTest_dx12 will fail to launch (vanilla python27.dll lacks PyStackless_* exports).")
    endif()

    set_target_properties(TrinityEngineTest_dx12 PROPERTIES FOLDER "Tests")

    # Register each gtest case with CTest so it shows up in Visual Studio's Test Explorer.
    # PRE_TEST discovery runs the exe at test time (not build time), so a launch/DLL issue
    # can't break the build; WORKING_DIRECTORY points at the exe folder where its dependent
    # DLLs were copied above (needed for the discovery run to launch).
    gtest_discover_tests(TrinityEngineTest_dx12
        DISCOVERY_MODE PRE_TEST
        WORKING_DIRECTORY "$<TARGET_FILE_DIR:TrinityEngineTest_dx12>"
    )
endif()
