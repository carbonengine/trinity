// Copyright © 2023 CCP ehf.

#pragma once

#ifndef Tr2EffectDescription_H
#define Tr2EffectDescription_H

#include "../Tr2IndirectDrawBuffer.h"

extern const BlueSharedString DEFAULT_TECHNIQUE;
extern const BlueSharedString ANY_TECHNIQUE;

// --------------------------------------------------------------------------------------
// Description:
//   Structure to describe effect compile #define. Used for compiling effects from
//   source.
// --------------------------------------------------------------------------------------
struct Tr2EffectDefine
{
	const char* name;
	const char* value;
};

// --------------------------------------------------------------------------------------
// Description:
//   Contains constant/parameter/uniform extracted from an effect.
// --------------------------------------------------------------------------------------
struct Tr2EffectConstant
{
	// ----------------------------------------------------------------------------------
	// Description:
	//   Constant type. Currently Trinity supports only FLOAT constants.
	// ----------------------------------------------------------------------------------
	enum Type
	{
		FLOAT,
		INT,
		UINT,
		BOOL,

		OTHER,
	};

	// Constant name (string is managed by the owner of string table: Tr2EffectRes or
	// Tr2HighLevelShader)
	BlueSharedString name;
	// Constant data offset in constant buffer (or offset/4 = register number for DX9)
	unsigned offset;
	// Constant size in bytes
	unsigned size;
	// Constant type
	Type type;
	// Vector dimensions (from 1 to 4)
	unsigned dimension;
	// Number of elements (>1 if constant is an array)
	unsigned elements;
	// Tr2SRGB annotation value
	bool isSRGB;
	// Autoregister annotation value
	bool isAutoregister;
};

typedef TrackableStdVector<Tr2EffectConstant> Tr2EffectConstantVector;

// --------------------------------------------------------------------------------------
// Description:
// Tr2EffectTexture stores effect texture input information: texture name and type.
// --------------------------------------------------------------------------------------
struct Tr2EffectResource
{
	enum Type
	{
		TEXTURE_1D = Tr2RenderContextEnum::TEX_TYPE_1D,
		TEXTURE_2D = Tr2RenderContextEnum::TEX_TYPE_2D,
		TEXTURE_3D = Tr2RenderContextEnum::TEX_TYPE_3D,
		TEXTURE_CUBE = Tr2RenderContextEnum::TEX_TYPE_CUBE,
		TEXTURE_TYPELESS = Tr2RenderContextEnum::TEX_TYPE_TYPELESS, // valid but unknown dimensions

		BUFFER,
		STRUCTURED_BUFFER,
		TBUFFER,
		BYTEADDRESS_BUFFER,

		UAV_RWTYPED,
		UAV_RWSTRUCTURED,
		UAV_RWBYTEADDRESS,
		UAV_APPEND_STRUCTURED,
		UAV_CONSUME_STRUCTURED,
		UAV_RWSTRUCTURED_WITH_COUNTER,
	};

	static const int BINDLESS_SAMPLER = 100;

	// Texture name (string is managed by the owner of string table: Tr2EffectRes or
	// Tr2HighLevelShader)
	const char* name;
	// Texture type
	Type type;
	uint32_t arrayElements;
	// Is texture requested to be converted from sRGB to linear
	bool isSRGB;
	// Autoregister annotation value
	bool isAutoregister;
};

typedef TrackableStdMap<int, Tr2EffectResource> Tr2EffectResourceMap;

// --------------------------------------------------------------------------------------
// Description:
// Tr2SamplerSetup stores sampler name and registered sampler state handle.
// --------------------------------------------------------------------------------------
struct Tr2SamplerSetup
{
	const char* name;
	Tr2SamplerStateAL sampler;
};

typedef TrackableStdMap<int, Tr2SamplerSetup> Tr2SamplerSetupMap;

// --------------------------------------------------------------------------------------
// Description:
// Tr2EffectParameterAnnotation represents HLSL effect annotation value (along with its
// type).
// --------------------------------------------------------------------------------------
struct Tr2EffectParameterAnnotation
{
	// ----------------------------------------------------------------------------------
	// Description:
	//   Annotation type.
	// ----------------------------------------------------------------------------------
	enum Type
	{
		BOOL,
		INT,
		FLOAT,
		STRING,
	};

	// Annotation name (string is managed by the owner of string table: Tr2EffectRes or
	// Tr2HighLevelShader)
	const char* name;
	Type type;
	union
	{
		bool boolValue;
		int intValue;
		float floatValue;
		// String value is managed by the owner of string table: Tr2EffectRes or
		// Tr2HighLevelShader
		const char* stringValue;
	};
};

typedef TrackableStdVector<Tr2EffectParameterAnnotation> Tr2EffectParameterAnnotationMap;

// Keys are parameter names (string is managed by the owner of string table: Tr2EffectRes or
// Tr2HighLevelShader)
typedef TrackableStdMap<const char*, Tr2EffectParameterAnnotationMap> Tr2EffectAnnotationMap;

const int SHADER_CONSTANTS_MAX = 256 * sizeof( Vector4 );

// ----------------------------------------------------------------------------------
// Description:
//   Represents per-render-stage inputs (shader, shader inputs).
// ----------------------------------------------------------------------------------
struct Tr2EffectStageInput
{
	Tr2EffectStageInput();

	enum
	{
		INVALID = 0xffFFffFFu
	};

	// If the stage was loaded from effect file
	bool m_exists;
	// Textures used in this stage
	Tr2EffectResourceMap resources;
	// UAVs used in this stage
	Tr2EffectResourceMap uavs;
	// Samplers used in this stage
	Tr2SamplerSetupMap samplers;
	// Shader handle (see Tr2EffectStateManager)
	unsigned m_shader;
	// Constants used by the shader
	Tr2EffectConstantVector constants;
	// Size of default constant buffer (in bytes)
	unsigned m_constantValueSize;
	// Default constant buffer contents
	char constantValues[SHADER_CONSTANTS_MAX];
	// Shader pipeline inputs
	Tr2ShaderSignatureAL signature;
	Tr2EffectParameterAnnotationMap annotation;
};

// --------------------------------------------------------------------------------------
// Description:
//   Effect pass description: pipeline stage inputs and a handle to render states.
// --------------------------------------------------------------------------------------
struct Tr2Pass
{
	// Input stages
	Tr2EffectStageInput stageInputs[Tr2RenderContextEnum::SHADER_TYPE_COUNT];
	// Handle to render state setup
	unsigned int renderStates;
	unsigned int shaderTypeMask;
	unsigned int shaderProgram;
	Tr2ResourceSetDescriptionAL resourceSetDesc;

#if TRINITY_PLATFORM == TRINITY_DIRECTX12 || TRINITY_PLATFORM == TRINITY_METAL
	Tr2IndirectDrawBufferLayout indirectLayout;
#endif
};

struct Tr2EffectLibrary
{
	uint32_t payloadSize;
	uint32_t libraryHandle;

	BlueSharedStringW rayGenName;
	BlueSharedStringW missName;
	BlueSharedStringW closestHitName;
	BlueSharedStringW anyHitName;
	BlueSharedStringW intersectionName;
	BlueSharedStringW hitGroupName;

	Tr2EffectStageInput globalInput;
	Tr2EffectStageInput localInput;
	Tr2ResourceSetDescriptionAL globalResourceSetDesc;
};


struct Tr2EffectTechnique
{
	Tr2EffectTechnique() :
		passes( "Tr2EffectTechnique::passes" ),
		shaderTypeMask( 0 )
	{
	}

	BlueSharedString name;
	TrackableStdVector<Tr2Pass> passes;
	std::vector<Tr2EffectLibrary> libraries;
	unsigned int shaderTypeMask;
};

// --------------------------------------------------------------------------------------
// Description:
//   Effect description/reflection: a vector of passes and parameter annotations.
// --------------------------------------------------------------------------------------
struct Tr2EffectDescription
{
	Tr2EffectDescription();

	bool Read( const void* data,
			   size_t dataSize,
			   unsigned version,
			   const char* stringTable,
			   size_t stringTableSize,
			   const char* effectName );

	TrackableStdVector<Tr2EffectTechnique> techniques;
	// Mapping from parameter name to its annotations
	Tr2EffectAnnotationMap annotations;
};


// --------------------------------------------------------------------------------------
// Description:
//   Shader permutation option: options to select from different shader permutations.
// --------------------------------------------------------------------------------------
struct Tr2ShaderOption
{
	BlueSharedString name;
	BlueSharedString value;
};


const Tr2SamplerSetupMap::value_type* FindSamplerByName( const Tr2SamplerSetupMap& samplerMap, const char* name );


#endif //Tr2EffectDescription_H