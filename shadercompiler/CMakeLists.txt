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


if (WIN32)
    set(LemonPath ${Lemon_ROOT}/${CCP_VENDOR_BIN_PATH}lemon.bat)
    set(Re2cPath ${re2c_ROOT}/${CCP_VENDOR_BIN_PATH}re2c.exe)
elseif (APPLE)
    set(LemonPath ${Lemon_ROOT}/${CCP_VENDOR_BIN_PATH}lemon.sh)
    set(Re2cPath ${re2c_ROOT}/${CCP_VENDOR_BIN_PATH}re2c)
endif()

get_filename_component(SHADER_COMPILER_BRANCH_PREFIX $ENV{CCP_EVE_PERFORCE_BRANCH_PATH} ABSOLUTE)

if (WIN32)
    if(WITH_DXC)
        set(dxc_ROOT "${SHADER_COMPILER_BRANCH_PREFIX}/vendor/dxc/v1.7.2212.1")
        find_package(dxc REQUIRED)
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


    add_custom_command(
        OUTPUT ${GENERATED_SOURCES_DIR}/HLSLParser.c ${GENERATED_SOURCES_DIR}/HLSLParser.h
        COMMAND ${CMAKE_COMMAND} -E make_directory ${GENERATED_SOURCES_DIR}
        COMMAND ${LemonPath} -d${GENERATED_SOURCES_DIR} -T${Lemon_ROOT}/include/lempar.c ${CMAKE_CURRENT_SOURCE_DIR}/HLSLParser.y
        DEPENDS lempar.c
        MAIN_DEPENDENCY HLSLParser.y
    )

    add_custom_command(
        OUTPUT ${GENERATED_SOURCES_DIR}/Preprocessor.c ${GENERATED_SOURCES_DIR}/Preprocessor.h
        COMMAND ${CMAKE_COMMAND} -E make_directory ${GENERATED_SOURCES_DIR}
        COMMAND ${LemonPath} -d${GENERATED_SOURCES_DIR} -T${CMAKE_CURRENT_SOURCE_DIR}/preprocessorLempar.c ${CMAKE_CURRENT_SOURCE_DIR}/Preprocessor.y
        DEPENDS preprocessorLempar.c
        MAIN_DEPENDENCY Preprocessor.y
    )

    add_custom_command(
        OUTPUT ${GENERATED_SOURCES_DIR}/HLSLScanner.cpp
        COMMAND ${CMAKE_COMMAND} -E make_directory ${GENERATED_SOURCES_DIR}
        COMMAND ${Re2cPath} -o ${GENERATED_SOURCES_DIR}/HLSLScanner.cpp ${CMAKE_CURRENT_SOURCE_DIR}/HLSLScanner.re
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
            dxc
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
set_target_properties(ShaderCompiler PROPERTIES 
    OUTPUT_NAME "ShaderCompiler$<$<CONFIG:DEBUG>:d>"
    RUNTIME_OUTPUT_DIRECTORY "$ENV{CCP_EVE_PERFORCE_BRANCH_PATH}/carbon/tools/ShaderCompiler/${CCP_VENDOR_PLATFORM}$<$<CONFIG:DEBUG>:>"
)
configure_shader_compiler(ShaderCompiler)


# Install rule to ensure that our runtime and linker files are in the expected, platform-specific folders
install(
    TARGETS ShaderCompiler
    EXPORT ShaderCompilerTargets
    CONFIGURATIONS ${CMAKE_CONFIGURATION_TYPES}
    LIBRARY DESTINATION shadercompiler/lib/${CCP_PLATFORM}/${CCP_ARCHITECTURE}/${CCP_TOOLSET}/
    ARCHIVE DESTINATION shadercompiler/lib/${CCP_PLATFORM}/${CCP_ARCHITECTURE}/${CCP_TOOLSET}/
    RUNTIME DESTINATION shadercompiler/bin/${CCP_PLATFORM}/${CCP_ARCHITECTURE}/${CCP_TOOLSET}/
)

# Install rule for available public headers
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
