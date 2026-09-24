// Copyright © 2026 CCP ehf.

#ifndef CARBON_SYSTEM_FXH
#define CARBON_SYSTEM_FXH

// This file contains definitions for backend API, including capability flags, and shader quality setting defines.

// Trinity graphics API backend
#define PLATFORM_DX11 2
#define PLATFORM_DX12 6
#define PLATFORM_METAL 10

#ifndef PLATFORM
#error Missing value for PLATFORM macro
#endif

// API supports compute shaders
#define PLATFORM_SUPPORTS_COMPUTE 1

// API supports geometry shaders
#if PLATFORM==PLATFORM_DX11 || PLATFORM==PLATFORM_DX12
#define PLATFORM_SUPPORTS_GEOMETRY_SHADERS 1
#else
#define PLATFORM_SUPPORTS_GEOMETRY_SHADERS 0
#endif

// API supports reading MSAA textures
#define PLATFORM_SUPPORTS_MSAA_READS 1
// API supports GetDimensions method for textures
#define PLATFORM_SUPPORTS_TEXTURE_SIZE_QUERIES 1
// API supports .Load methods for textures
#define PLATFORM_SUPPORTS_TEXTURE_LOAD 1

// API supports SRV/UAV/Sampler heap views
#if PLATFORM==PLATFORM_METAL || PLATFORM==PLATFORM_DX12
#define PLATFORM_SUPPORTS_SUPPORTS_HEAP_VIEW 1
#else
#define PLATFORM_SUPPORTS_SUPPORTS_HEAP_VIEW 0
#endif

// Maximum size of constant buffer in bytes
#if PLATFORM==PLATFORM_DX11 || PLATFORM==PLATFORM_DX12
#define PLATFORM_MAX_CONSTANT_BUFFER_SIZE 65536
#else
#define PLATFORM_MAX_CONSTANT_BUFFER_SIZE 4096
#endif

// API supports ray tracing
#if PLATFORM==PLATFORM_DX12 || PLATFORM==PLATFORM_METAL
#define PLATFORM_SUPPORTS_RAY_TRACING 1
#else
#define PLATFORM_SUPPORTS_RAY_TRACING 0
#endif

// Support for "precise" keyword is for DirectX only
#if PLATFORM==PLATFORM_DX11 || PLATFORM==PLATFORM_DX12
#define DX11_PRECISE precise
#else
#define DX11_PRECISE
#endif


// Shader quality setting. Comes from project user settings via Trinity
#define SHADER_QUALITY_LOW 3
#define SHADER_QUALITY_MEDIUM 4
#define SHADER_QUALITY_HIGH 5

// Legacy quality defines
#define SM_3_0_LO SHADER_QUALITY_LOW
#define SM_3_0_HI SHADER_QUALITY_MEDIUM
#define SM_3_0_DEPTH SHADER_QUALITY_HIGH

#ifndef SHADERMODEL
#define SHADERMODEL SHADER_QUALITY_HIGH
#endif

#ifndef SHADER_QUALITY
#define SHADER_QUALITY SHADERMODEL
#endif


// Hits for the shader compiler signifying per-object and per-frame constant buffers.
// Used in global variable declarations. The actual register numbers specified in macros are
// just hints to the compiler, and have nothing to do with the actual binding. Declaration example:
// PerFrameStructType PerFrameVS : register( PERFRAME_VS_STARTREGISTER );
#define PERFRAME_VS_STARTREGISTER vs, c220
#define PERFRAME_PS_STARTREGISTER ps, c200
#define PEROBJECT_VS_STARTREGISTER vs, c16
#define PEROBJECT_PS_STARTREGISTER ps, c40

#endif