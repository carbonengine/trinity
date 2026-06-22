# Copyright © 2023 CCP ehf.

cmake_minimum_required(VERSION 3.16)

project(ShaderCompiler VERSION 1.2.6)

set(GENERATED_SOURCES_DIR ${CMAKE_CURRENT_BINARY_DIR}/GeneratedSources)


set(_SOURCES
    ASTNode.cpp
    ASTNode.h
    CachingIncludeHandler.cpp
    CachingIncludeHandler.h
    CompileMessageQueue.cpp
    CompileMessageQueue.h
    DxReflection.cpp
    DxReflection.h
    EffectCompilerBase.h
    EffectCompilerDX11.cpp
    EffectCompilerDX11.h
    EffectCompilerDX12.cpp
    EffectCompilerDX12.h
    EffectCompilerMetal.cpp
    EffectCompilerMetal.h
    EffectData.cpp
    EffectData.h
    FXAnalyzer.cpp
    FXAnalyzer.h
    HLSLParser.y
    ShaderCompilerConfig.h.in
    HLSLScanner.re
    InlineString.h
    IntrinsicTypes.h
    Macro.h
    md5.cpp
    md5.h
    ModifiedTime.cpp
    ModifiedTime.h
    MSLInclude.h
    OutputHLSL.cpp
    OutputHLSL.h
    ParserState.cpp
    ParserState.h
    ParserUtils.cpp
    ParserUtils.h
    Platforms.h
    ${GENERATED_SOURCES_DIR}/HLSLParser.c
    ${GENERATED_SOURCES_DIR}/HLSLParser.h
    ${GENERATED_SOURCES_DIR}/HLSLScanner.cpp
    ${GENERATED_SOURCES_DIR}/ShaderCompilerConfig.h
    ${GENERATED_SOURCES_DIR}/Preprocessor.c
    ${GENERATED_SOURCES_DIR}/Preprocessor.h
    Preprocessor.y
    ShaderCompiler.cpp
    stdafx.cpp
    stdafx.h
    StringTable.cpp
    StringTable.h
    SymbolTable.cpp
    SymbolTable.h
    TextureFunctionConversionDX11.cpp
    TextureFunctionConversionDX11.h
    Type.cpp
    Type.h
    WorkQueue.h
    YamlOutput.h
)

set(_TEST_SOURCES
    tests/Bindless.cpp
    tests/GlobalInputTest.cpp
    tests/MetalConversionTest.cpp
    tests/RayTracingTest.cpp
    tests/ShaderCompilerTest.cpp
    tests/TesingUtils.h
)


if (APPLE)
    set(TELEMETRY_DEFAULT OFF)
    set(DXC_DEFAULT OFF)
else()
    set(TELEMETRY_DEFAULT OFF)
    set(DXC_DEFAULT ON)
endif()

option(WITH_TELEMETRY "Enable integration with Tracy profiler" ${TELEMETRY_DEFAULT})

option(WITH_DXC "Enable DXC Compiler" ${DXC_DEFAULT})



if(BUILD_FOR_PYTHON_2)
    find_package(re2c CONFIG REQUIRED)
    find_package(Lemon CONFIG REQUIRED)
    set(RE2C ${RE2C_PATH})
    set(LEMON ${LEMON_PATH})
    set(LEMPAR_ROOT "${Lemon_ROOT}/include/")
else()
    find_program(RE2C re2c REQUIRED)
    find_program(LEMON lemon REQUIRED)
    get_filename_component(LEMPAR_ROOT "${LEMON}" DIRECTORY)
endif()

if (WIN32)
    if(WITH_DXC)
        if(BUILD_FOR_PYTHON_2)
            find_package(dxc REQUIRED)
            add_library(Microsoft::DirectXShaderCompiler ALIAS dxc)
        else()
            find_package(directx-dxc CONFIG REQUIRED)
        endif()
    endif()
endif()

function(configure_shader_compiler target)

    target_sources(${target} PRIVATE ${_SOURCES})

    source_group(Grammar FILES 
        ASTNode.cpp
        ASTNode.h
        HLSLParser.c
        HLSLParser.h
        HLSLParser.y
        HLSLScanner.cpp
        HLSLScanner.re
        InlineString.h
        IntrinsicTypes.h
        ParserState.cpp
        ParserState.h
        ParserUtils.cpp
        ParserUtils.h
        Preprocessor.c
        Preprocessor.h
        Preprocessor.y
        SymbolTable.cpp
        SymbolTable.h
        TextureFunctionConversionDX11.cpp
        TextureFunctionConversionDX11.h
        Type.cpp
        Type.h
    )

    configure_file("${PROJECT_SOURCE_DIR}/ShaderCompilerConfig.h.in" "${GENERATED_SOURCES_DIR}/ShaderCompilerConfig.h") 

    set(PUBLIC_HEADER_FILES "")

    set_source_files_properties(${GENERATED_SOURCES_DIR}/HLSLParser.c ${GENERATED_SOURCES_DIR}/Preprocessor.c PROPERTIES LANGUAGE CXX )

    # Matching historical setup that previously used .bat and .sh files to run Lemon that would
    # Force the return value to always be 0 and therefore pass even if Lemon returned non-zero result
    # At time of writing, omitting this will fail the build as lemon returns an issue.
    # It would be best if the issue was addressed so that issues are not masked by this mechanism.
    # Once issue has been resolved then this can all be simplified
    if(WIN32)
        set(FORCE_ZERO_RETURN_FROM_LEMON "exit /b 0")
        set(CMD "cmd")
        set(CMD_ARG "/c")
    elseif(APPLE)
        set(FORCE_ZERO_RETURN_FROM_LEMON "true")
        set(CMD "bash")
        set(CMD_ARG "-c")
    else()
        message(FATAL "Unsupported Platform")
    endif()

    add_custom_command(
        COMMENT "Lemon: Generate HLSLParser.c & HLSLParser.h"
        OUTPUT ${GENERATED_SOURCES_DIR}/HLSLParser.c ${GENERATED_SOURCES_DIR}/HLSLParser.h
        COMMAND ${CMAKE_COMMAND} -E make_directory ${GENERATED_SOURCES_DIR}
        COMMAND ${CMD} ${CMD_ARG} "${LEMON} -d${GENERATED_SOURCES_DIR} -T${LEMPAR_ROOT}/lempar.c ${CMAKE_CURRENT_SOURCE_DIR}/HLSLParser.y||${FORCE_ZERO_RETURN_FROM_LEMON}"
        DEPENDS lempar.c
        MAIN_DEPENDENCY HLSLParser.y
    )

    add_custom_command(
        COMMENT "Lemon: Generate Preprocessor.c & Preprocessor.h"
        OUTPUT ${GENERATED_SOURCES_DIR}/Preprocessor.c ${GENERATED_SOURCES_DIR}/Preprocessor.h
        COMMAND ${CMAKE_COMMAND} -E make_directory ${GENERATED_SOURCES_DIR}
        COMMAND ${CMD} ${CMD_ARG} "${LEMON} -d${GENERATED_SOURCES_DIR} -T${CMAKE_CURRENT_SOURCE_DIR}/preprocessorLempar.c ${CMAKE_CURRENT_SOURCE_DIR}/Preprocessor.y||${FORCE_ZERO_RETURN_FROM_LEMON}"
        DEPENDS preprocessorLempar.c
        MAIN_DEPENDENCY Preprocessor.y
    )

    add_custom_command(
        COMMENT "Re2c: Generate HLSLScanner.cpp"
        OUTPUT ${GENERATED_SOURCES_DIR}/HLSLScanner.cpp
        COMMAND ${CMAKE_COMMAND} -E make_directory ${GENERATED_SOURCES_DIR}
        COMMAND ${RE2C} -o ${GENERATED_SOURCES_DIR}/HLSLScanner.cpp ${CMAKE_CURRENT_SOURCE_DIR}/HLSLScanner.re
        MAIN_DEPENDENCY HLSLScanner.re
    )

    target_precompile_headers(${target} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/stdafx.h)
    target_compile_definitions(${target} PRIVATE GLEW_MX GLEW_STATIC)

    target_include_directories(${target} PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${GENERATED_SOURCES_DIR}
    )


    if (WIN32)
        target_link_libraries(${target} PRIVATE
            d3dcompiler.lib
            d3d11.lib
            dxguid.lib
            Microsoft::DirectXShaderCompiler
        )
        target_compile_options(${target} PRIVATE /W4)

        # Copy DLL dependencies
        add_custom_command(
            TARGET ${target} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_RUNTIME_DLLS:${target}> $<TARGET_FILE_DIR:${target}>
            COMMAND_EXPAND_LISTS
        )
    else()
        target_compile_options(${target} PRIVATE -Wall -Wextra -pedantic -Wno-gnu-zero-variadic-macro-arguments -Wno-unused-command-line-argument)
        set_source_files_properties(${GENERATED_SOURCES_DIR}/HLSLParser.c PROPERTIES COMPILE_FLAGS "-x c++ -Wno-unused-parameter -Wno-sign-compare -Wno-unused-variable")
        set_source_files_properties(${GENERATED_SOURCES_DIR}/Preprocessor.c PROPERTIES COMPILE_FLAGS "-x c++ -Wno-unused-parameter -Wno-sign-compare -Wno-unused-variable")
    endif()

    if(WITH_TELEMETRY)
        #find_package(Tracy REQUIRED NO_CMAKE_PATH)
        #target_link_libraries(${target} PUBLIC Tracy::TracyClient)
        #target_compile_definitions(${target} PUBLIC CCP_TELEMETRY_ENABLED=1 TRACY_IMPORTS TRACY_ENABLE TRACY_FIBERS TRACY_ON_DEMAND TRACY_DELAYED_INIT TRACY_MANUAL_LIFETIME)
        target_compile_definitions(${target} PUBLIC CCP_TELEMETRY_ENABLED=0)
    else()
        target_compile_definitions(${target} PUBLIC CCP_TELEMETRY_ENABLED=0)
    endif()
endfunction()

add_executable(ShaderCompiler)

configure_shader_compiler(ShaderCompiler)

# Install rules
if (INSTALL_TO_MONOLITH)

    # Install rule to ensure that our runtime and linker files are in the expected, platform-specific folders
    install(
        TARGETS ShaderCompiler
        EXPORT ShaderCompilerTargets
        CONFIGURATIONS ${CMAKE_CONFIGURATION_TYPES}
        LIBRARY DESTINATION shadercompiler/lib/${CCP_PLATFORM}/${CCP_ARCHITECTURE}/${CCP_TOOLSET}/
        ARCHIVE DESTINATION shadercompiler/lib/${CCP_PLATFORM}/${CCP_ARCHITECTURE}/${CCP_TOOLSET}/
        RUNTIME DESTINATION shadercompiler/bin/${CCP_PLATFORM}/${CCP_ARCHITECTURE}/${CCP_TOOLSET}/
    )
    if (WIN32)
        install(FILES $<TARGET_PDB_FILE:ShaderCompiler> DESTINATION shadercompiler/bin/${CCP_PLATFORM}/${CCP_ARCHITECTURE}/${CCP_TOOLSET}/ OPTIONAL)
    endif()

    install(FILES ${PUBLIC_HEADER_FILES} DESTINATION shadercompiler/include)

    message(STATUS "lib/${CCP_PLATFORM}/${CCP_ARCHITECTURE}/${CCP_TOOLSET}/")

    configure_ccp_vendor_config_file(
        TARGET ShaderCompiler
        DESTINATION "${CMAKE_CURRENT_BINARY_DIR}/ShaderCompilerConfig.cmake"
        DEPENDENCIES ""
    )
    install(FILES ${CMAKE_CURRENT_BINARY_DIR}/ShaderCompilerConfig.cmake DESTINATION shadercompiler/.)

    # Install python package
    install(DIRECTORY python/ DESTINATION shadercompiler/python
    FILES_MATCHING
    PATTERN "*.py")

    # Install python build script
    install(DIRECTORY pythonBuildScript/ DESTINATION shadercompiler/pythonBuildScript
    FILES_MATCHING
    PATTERN "*.py")

else() # VCPKG install mode

    install(
        TARGETS ShaderCompiler
        EXPORT ShaderCompilerTargets
        CONFIGURATIONS ${CMAKE_CONFIGURATION_TYPES}
        LIBRARY DESTINATION lib
        RUNTIME DESTINATION bin
    )

    install(EXPORT ShaderCompilerTargets
            DESTINATION share/carbon-shadercompiler
            FILE carbon-shadercompilerConfig.cmake
        )

    if (WIN32)
        install(FILES $<TARGET_PDB_FILE:ShaderCompiler> DESTINATION bin OPTIONAL)
    endif()

    install(FILES ${PUBLIC_HEADER_FILES} DESTINATION include)

    # Install python package
    install(DIRECTORY python/ DESTINATION shadercompiler/python
    FILES_MATCHING
    PATTERN "*.py")

    # Install python build script
    install(DIRECTORY pythonBuildScript/ DESTINATION shadercompiler/pythonBuildScript
    FILES_MATCHING
    PATTERN "*.py")

endif()
 
option(BUILD_TESTING "Build and run tests. Enabled by default." ON)
message(STATUS " BUILD_TESTING VALUE IS ${BUILD_TESTING}")
if(BUILD_TESTING)

    find_package(GTest REQUIRED)

    add_executable(ShaderCompilerTest)
    target_compile_definitions(ShaderCompilerTest PRIVATE SHADER_COMPILER_TEST=1)
    target_link_libraries(ShaderCompilerTest PRIVATE GTest::GTest GTest::Main)
    target_sources(ShaderCompilerTest PRIVATE ${_TEST_SOURCES})
    source_group(Tests FILES ${_TEST_SOURCES})
    configure_shader_compiler(ShaderCompilerTest)
    set_target_properties(ShaderCompilerTest PROPERTIES FOLDER "Tests")
    add_dependencies(ShaderCompilerTest ShaderCompiler)
endif()
