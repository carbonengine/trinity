cmake_minimum_required(VERSION 3.16)

if(APPLE)
    set(CMAKE_OSX_DEPLOYMENT_TARGET 10.15 CACHE STRING "The minimum macOS version we target." FORCE)
endif()

project(ShaderCompiler VERSION 1.1.1)

if (APPLE)
    set(TELEMETRY_DEFAULT OFF)
else()
    set(TELEMETRY_DEFAULT ON)
endif()

option(WITH_TELEMETRY "Enable integration with RAD Game tools Telemetry SDK" ${TELEMETRY_DEFAULT})

set(GENERATED_SOURCES_DIR ${CMAKE_CURRENT_BINARY_DIR}/GeneratedSources)

add_executable(ShaderCompiler
    ASTNode.cpp
    ASTNode.h
    CachingIncludeHandler.cpp
    CachingIncludeHandler.h
    CompileMessageQueue.cpp
    CompileMessageQueue.h
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
    ${GENERATED_SOURCES_DIR}/HLSLParser.c
    ${GENERATED_SOURCES_DIR}/HLSLParser.h
    HLSLParser.y
    ${GENERATED_SOURCES_DIR}/HLSLScanner.cpp
    ${GENERATED_SOURCES_DIR}/ShaderCompilerConfig.h
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

if (WIN32)
    set(LemonPath ${Lemon_ROOT}/${CCP_VENDOR_BIN_PATH}lemon.bat)
    set(Re2cPath ${re2c_ROOT}/${CCP_VENDOR_BIN_PATH}re2c.exe)
elseif (APPLE)
    set(LemonPath ${Lemon_ROOT}/${CCP_VENDOR_BIN_PATH}lemon.sh)
    set(Re2cPath ${re2c_ROOT}/${CCP_VENDOR_BIN_PATH}re2c)
endif()


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

target_precompile_headers(ShaderCompiler PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/stdafx.h)
target_compile_definitions(ShaderCompiler PRIVATE GLEW_MX GLEW_STATIC)

target_include_directories(ShaderCompiler PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${GENERATED_SOURCES_DIR}
)
set_target_properties(ShaderCompiler PROPERTIES 
    OUTPUT_NAME "ShaderCompiler$<$<CONFIG:DEBUG>:d>"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/../../tools/ShaderCompiler/${CCP_VENDOR_PLATFORM}$<$<CONFIG:DEBUG>:>"
)

if (WIN32)
    target_link_libraries(ShaderCompiler PRIVATE
        d3dcompiler.lib
        d3d11.lib
        dxguid.lib
    )
    target_compile_options(ShaderCompiler PRIVATE /W4)
else()
    target_compile_options(ShaderCompiler PRIVATE -Wall -Wextra -pedantic -Wno-gnu-zero-variadic-macro-arguments)
    set_source_files_properties(${GENERATED_SOURCES_DIR}/HLSLParser.c PROPERTIES COMPILE_FLAGS "-Wno-unused-parameter -Wno-sign-compare -Wno-unused-variable")
    set_source_files_properties(${GENERATED_SOURCES_DIR}/Preprocessor.c PROPERTIES COMPILE_FLAGS "-Wno-unused-parameter -Wno-sign-compare -Wno-unused-variable")
endif()


if(WITH_TELEMETRY)
    find_package(RadGameToolsTelemetry REQUIRED NO_CMAKE_PATH)
    target_link_libraries(ShaderCompiler PUBLIC RadGameTools::Telemetry)
    target_compile_definitions(ShaderCompiler PUBLIC CCP_TELEMETRY_ENABLED=1)
else()
    target_compile_definitions(ShaderCompiler PUBLIC CCP_TELEMETRY_ENABLED=0)
endif()

get_filename_component(SHADER_COMPILER_BRANCH_PREFIX $ENV{CCP_EVE_PERFORCE_BRANCH_PATH} ABSOLUTE)

# Install rule to ensure that our runtime and linker files are in the expected, platform-specific folders
install(
    TARGETS ShaderCompiler
    EXPORT ShaderCompilerTargets
    CONFIGURATIONS ${CMAKE_CONFIGURATION_TYPES}
    LIBRARY DESTINATION shadercompiler/lib/${CCP_PLATFORM}/${CCP_ARCHITECTURE}/${CCP_TOOLSET}/
    ARCHIVE DESTINATION shadercompiler/lib/${CCP_PLATFORM}/${CCP_ARCHITECTURE}/${CCP_TOOLSET}/
    RUNTIME DESTINATION ${SHADER_COMPILER_BRANCH_PREFIX}/carbon/tools/ShaderCompiler/${CCP_PLATFORM}/
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
