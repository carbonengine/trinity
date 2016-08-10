#pragma once
#ifndef EffectData_H
#define EffectData_H

#include "StringTable.h"
#include <vector>

struct IUnknown;

// These next enums must have the same values as corresponding Trinity enums

enum InputStageType
{
	VERTEX_STAGE,
	PIXEL_STAGE,
	COMPUTE_STAGE,
	GEOMETRY_STAGE,
	HULL_STAGE,
	DOMAIN_STAGE,
};

enum ConstantType
{
	CONSTANT_TYPE_FLOAT = 0,
	CONSTANT_TYPE_INT = 1,
	CONSTANT_TYPE_BOOL = 2,
	CONSTANT_TYPE_OTHER = 3,
};

enum AnnotationType
{
	ANNOTATION_TYPE_BOOL = 0,
	ANNOTATION_TYPE_INT = 1,
	ANNOTATION_TYPE_FLOAT = 2,
	ANNOTATION_TYPE_STRING = 3,
};

enum TextureType
{
	TEX_TYPE_1D		= 1,
	TEX_TYPE_2D,
	TEX_TYPE_3D,
	TEX_TYPE_CUBE,

	TEX_TYPE_TYPELESS,	// valid but unknown dimensions

	// buffers
	TEX_TYPE_BUFFER,
	TEX_TYPE_STRUCTURED_BUFFER,
	TEX_TYPE_TBUFFER,
	TEX_TYPE_BYTEADDRESS_BUFFER,

	// uavs
	TEX_TYPE_UAV_RWTYPED,
	TEX_TYPE_UAV_RWSTRUCTURED,
	TEX_TYPE_UAV_RWBYTEADDRESS,
	TEX_TYPE_UAV_APPEND_STRUCTURED,
	TEX_TYPE_UAV_CONSUME_STRUCTURED,
	TEX_TYPE_UAV_RWSTRUCTURED_WITH_COUNTER,

	TEX_TYPE_INVALID
};

enum UsageCode
{
	UC_POSITION,
	UC_COLOR,
	UC_NORMAL,
	UC_TANGENT,
	UC_BITANGENT,
	UC_TEXCOORD,
	UC_BLENDINDICES,
	UC_BLENDWEIGHTS,

	UC_NUM_USAGE_CODE
};


// Saved as:
//	name: StringReference
//	cbIndex: BYTE
//  index: DWORD
//  size: DWORD
//  constantType: BYTE
//  dimension: BYTE
//  elements: DWORD
struct Constant
{
	size_t GetPackedSize()
	{
		return sizeof( DWORD ) + sizeof( DWORD ) + sizeof( DWORD ) + sizeof( BYTE ) + sizeof( BYTE ) + sizeof( DWORD ) + sizeof( BYTE ) + sizeof( BYTE );
	}
	void Save( BYTE*& buffer )
	{
		*reinterpret_cast<DWORD*>( buffer ) = name;
		buffer += sizeof( DWORD );
		*reinterpret_cast<DWORD*>( buffer ) = offset;
		buffer += sizeof( DWORD );
		*reinterpret_cast<DWORD*>( buffer ) = size;
		buffer += sizeof( DWORD );
		*reinterpret_cast<BYTE*>( buffer ) = type;
		buffer += sizeof( BYTE );
		*reinterpret_cast<BYTE*>( buffer ) = dimension;
		buffer += sizeof( BYTE );
		*reinterpret_cast<DWORD*>( buffer ) = elements;
		buffer += sizeof( DWORD );
		*reinterpret_cast<BYTE*>( buffer ) = isSRGB ? 1 : 0;
		buffer += sizeof( BYTE );
		*reinterpret_cast<BYTE*>( buffer ) = isAutoregister ? 1 : 0;
		buffer += sizeof( BYTE );
	}

	StringReference name;
	unsigned offset;
	unsigned size;
	ConstantType type;
	unsigned dimension;
	unsigned elements;
	bool isSRGB;
	bool isAutoregister;
};

struct Vector4
{
	float x, y, z, w;
};

// Saved as:
//	textureName: StringReference
//  textureType: BYTE
//  isSRG: BYTE
//  isAutoregister: BYTE
struct Texture
{
	size_t GetPackedSize()
	{
		return sizeof( DWORD ) + sizeof( BYTE ) + sizeof( BYTE ) + sizeof( BYTE );
	}
	void Save( BYTE*& buffer )
	{
		*reinterpret_cast<DWORD*>( buffer ) = name;
		buffer += sizeof( DWORD );
		*reinterpret_cast<BYTE*>( buffer ) = type;
		buffer += sizeof( BYTE );
		*reinterpret_cast<BYTE*>( buffer ) = isSRGB ? 1 : 0;
		buffer += sizeof( BYTE );
		*reinterpret_cast<BYTE*>( buffer ) = isAutoregister ? 1 : 0;
		buffer += sizeof( BYTE );
	}

	StringReference name;
	TextureType type;
	bool isSRGB;
	bool isAutoregister;
};

// Saved as:
//	name: StringReference
//  type: BYTE
//  isAutoregister: BYTE
struct Uav
{
	size_t GetPackedSize()
	{
		return sizeof( DWORD ) + sizeof( BYTE ) + sizeof( BYTE );
	}
	void Save( BYTE*& buffer )
	{
		*reinterpret_cast<DWORD*>( buffer ) = name;
		buffer += sizeof( DWORD );
		*reinterpret_cast<BYTE*>( buffer ) = type;
		buffer += sizeof( BYTE );
		*reinterpret_cast<BYTE*>( buffer ) = isAutoregister ? 1 : 0;
		buffer += sizeof( BYTE );
	}

	StringReference name;
	TextureType type;
	bool isAutoregister;
};

// Saved as:
//	name: StringReference
//  samplerStateCount: BYTE
//  (samplerState: BYTE samplerStateValue: BYTE) x samplerStateCount
struct Sampler
{
	size_t GetPackedSize()
	{
		return sizeof( DWORD ) + sizeof( BYTE ) * 9 + sizeof( FLOAT ) * 7;
	}
	void Save( BYTE*& buffer )
	{
		*reinterpret_cast<DWORD*>( buffer ) = name;
		buffer += sizeof( DWORD );
		*reinterpret_cast<BYTE*>( buffer ) = comparison;
		buffer += sizeof( BYTE );
		*reinterpret_cast<BYTE*>( buffer ) = minFilter;
		buffer += sizeof( BYTE );
		*reinterpret_cast<BYTE*>( buffer ) = magFilter;
		buffer += sizeof( BYTE );
		*reinterpret_cast<BYTE*>( buffer ) = mipFilter;
		buffer += sizeof( BYTE );
		*reinterpret_cast<BYTE*>( buffer ) = addressU;
		buffer += sizeof( BYTE );
		*reinterpret_cast<BYTE*>( buffer ) = addressV;
		buffer += sizeof( BYTE );
		*reinterpret_cast<BYTE*>( buffer ) = addressW;
		buffer += sizeof( BYTE );
		*reinterpret_cast<float*>( buffer ) = mipLODBias;
		buffer += sizeof( float );
		*reinterpret_cast<BYTE*>( buffer ) = maxAnisotropy;
		buffer += sizeof( BYTE );
		*reinterpret_cast<BYTE*>( buffer ) = comparisonFunc;
		buffer += sizeof( BYTE );
		*reinterpret_cast<float*>( buffer ) = borderColor.x;
		buffer += sizeof( float );
		*reinterpret_cast<float*>( buffer ) = borderColor.y;
		buffer += sizeof( float );
		*reinterpret_cast<float*>( buffer ) = borderColor.z;
		buffer += sizeof( float );
		*reinterpret_cast<float*>( buffer ) = borderColor.w;
		buffer += sizeof( float );
		*reinterpret_cast<float*>( buffer ) = minLOD;
		buffer += sizeof( float );
		*reinterpret_cast<float*>( buffer ) = maxLOD;
		buffer += sizeof( float );
	}

	bool operator==( const Sampler& sampler ) const
	{
		return 
			comparison == sampler.comparison &&
			minFilter == sampler.minFilter &&
			magFilter == sampler.magFilter &&
			mipFilter == sampler.mipFilter &&
			addressU == sampler.addressU &&
			addressV == sampler.addressV &&
			addressW == sampler.addressW &&
			mipLODBias == sampler.mipLODBias &&
			maxAnisotropy == sampler.maxAnisotropy &&
			comparisonFunc == sampler.comparisonFunc &&
			borderColor.x == sampler.borderColor.x &&
			borderColor.y == sampler.borderColor.y &&
			borderColor.z == sampler.borderColor.z &&
			borderColor.w == sampler.borderColor.w &&
			minLOD == sampler.minLOD &&
			maxLOD == sampler.maxLOD;
	}

	bool operator<( const Sampler& sampler ) const
	{
#define COMPARE( field ) if( field < sampler.field ) return true; if( field > sampler.field ) return false;
		COMPARE( comparison );
		COMPARE( minFilter );
		COMPARE( magFilter );
		COMPARE( mipFilter );
		COMPARE( addressU );
		COMPARE( addressV );
		COMPARE( addressW );
		COMPARE( mipLODBias );
		COMPARE( maxAnisotropy );
		COMPARE( comparisonFunc );
		COMPARE( borderColor.x );
		COMPARE( borderColor.y );
		COMPARE( borderColor.z );
		COMPARE( borderColor.w );
		COMPARE( minLOD );
		COMPARE( maxLOD );
#undef COMPARE
		return false;
	}

	StringReference name;
	BYTE filter;
	BYTE comparison;
	BYTE minFilter;
	BYTE magFilter;
	BYTE mipFilter;
	BYTE addressU;
	BYTE addressV;
	BYTE addressW;
	float mipLODBias;
	BYTE maxAnisotropy;
	BYTE comparisonFunc;
	Vector4 borderColor;
	float minLOD;
	float maxLOD;
	BYTE srgbTexture;
};

// Saved as:
//	name: StringReference
//  index: BYTE
//  usedMask: BYTE
struct InputDescription
{
	size_t GetPackedSize()
	{
		return sizeof( BYTE ) + sizeof( BYTE ) + sizeof( BYTE ) + sizeof( BYTE );
	}
	void Save( BYTE*& buffer )
	{
		*reinterpret_cast<BYTE*>( buffer ) = name;
		buffer += sizeof( BYTE );
		*reinterpret_cast<BYTE*>( buffer ) = registerIndex;
		buffer += sizeof( BYTE );
		*reinterpret_cast<BYTE*>( buffer ) = index;
		buffer += sizeof( BYTE );
		*reinterpret_cast<BYTE*>( buffer ) = usedMask;
		buffer += sizeof( BYTE );
	}

	BYTE name;
	BYTE registerIndex;
	BYTE index;
	BYTE usedMask;
	BYTE componentType;
};

// Saved as:
//	type: BYTE
//  shaderSize: DWORD
//  shaderData: BYTE x shaderSize
//  constantCount: DWORD
//  Constant x constantCount
//  defaultValueCount: DWORD
//  Vector4 x defaultValueCount
//  samplerCount: BYTE
//  (stage: BYTE Sampler) x samplerCount
struct StageInput
{
	size_t GetPackedSize()
	{
		size_t size = sizeof( BYTE ) + sizeof( DWORD ) + shaderSize + sizeof( DWORD ) + shadowShaderSize + sizeof( DWORD ) + 3 * sizeof( DWORD );
		for( auto it = constants.begin(); it != constants.end(); ++it )
		{
			size += it->GetPackedSize();
		}
		size += sizeof( DWORD ) + defaultValues.size() + sizeof( BYTE ) + sizeof( BYTE );
		for( auto it = textures.begin(); it != textures.end(); ++it )
		{
			size += sizeof( BYTE );
			size += it->second.GetPackedSize();
		}
		for( auto it = samplers.begin(); it != samplers.end(); ++it )
		{
			size += sizeof( BYTE );
			size += it->second.GetPackedSize();
		}
		size += sizeof( BYTE );
		for( auto it = inputs.begin(); it != inputs.end(); ++it )
		{
			size += it->GetPackedSize();
		}
		size += sizeof( BYTE );
		for( auto it = uavs.begin(); it != uavs.end(); ++it )
		{
			size += sizeof( BYTE );
			size += it->second.GetPackedSize();
		}
		return size;
	}
	void Save( BYTE*& buffer )
	{
		*reinterpret_cast<BYTE*>( buffer ) = type;
		buffer += sizeof( BYTE );
		*reinterpret_cast<BYTE*>( buffer ) = inputs.size();
		buffer += sizeof( BYTE );
		for( auto it = inputs.begin(); it != inputs.end(); ++it )
		{
			BYTE* start = buffer;
			it->Save( buffer );
			assert( buffer - start == it->GetPackedSize() );
		}
		*reinterpret_cast<DWORD*>( buffer ) = shaderSize;
		buffer += sizeof( DWORD );
		memcpy( buffer, shaderData, shaderSize );
		buffer += shaderSize;
		*reinterpret_cast<DWORD*>( buffer ) = shadowShaderSize;
		buffer += sizeof( DWORD );
		memcpy( buffer, shadowShaderData, shadowShaderSize );
		buffer += shadowShaderSize;
		*reinterpret_cast<DWORD*>( buffer ) = threadGroupSize[0];
		buffer += sizeof( DWORD );
		*reinterpret_cast<DWORD*>( buffer ) = threadGroupSize[1];
		buffer += sizeof( DWORD );
		*reinterpret_cast<DWORD*>( buffer ) = threadGroupSize[2];
		buffer += sizeof( DWORD );
		*reinterpret_cast<DWORD*>( buffer ) = constants.size();
		buffer += sizeof( DWORD );
		for( auto it = constants.begin(); it != constants.end(); ++it )
		{
			BYTE* start = buffer;
			it->Save( buffer );
			assert( buffer - start == it->GetPackedSize() );
		}
		*reinterpret_cast<DWORD*>( buffer ) = defaultValues.size();
		buffer += sizeof( DWORD );
		if( !defaultValues.empty() )
		{
			memcpy( buffer, &defaultValues[0], defaultValues.size() );
			buffer += defaultValues.size();
		}
		*reinterpret_cast<BYTE*>( buffer ) = textures.size();
		buffer += sizeof( BYTE );
		for( auto it = textures.begin(); it != textures.end(); ++it )
		{
			*reinterpret_cast<BYTE*>( buffer ) = it->first;
			buffer += sizeof( BYTE );
			BYTE* start = buffer;
			it->second.Save( buffer );
			assert( buffer - start == it->second.GetPackedSize() );
		}
		*reinterpret_cast<BYTE*>( buffer ) = samplers.size();
		buffer += sizeof( BYTE );
		for( auto it = samplers.begin(); it != samplers.end(); ++it )
		{
			*reinterpret_cast<BYTE*>( buffer ) = it->first;
			buffer += sizeof( BYTE );
			BYTE* start = buffer;
			it->second.Save( buffer );
			assert( buffer - start == it->second.GetPackedSize() );
		}
		*reinterpret_cast<BYTE*>( buffer ) = uavs.size();
		buffer += sizeof( BYTE );
		for( auto it = uavs.begin(); it != uavs.end(); ++it )
		{
			*reinterpret_cast<BYTE*>( buffer ) = it->first;
			buffer += sizeof( BYTE );
			BYTE* start = buffer;
			it->second.Save( buffer );
			assert( buffer - start == it->second.GetPackedSize() );
		}
	}

	InputStageType type;
	std::vector<InputDescription> inputs;
	unsigned shaderSize;
	void* shaderData;
	unsigned shadowShaderSize;
	void* shadowShaderData;
	unsigned threadGroupSize[3];
	std::vector<Constant> constants;
	std::vector<BYTE> defaultValues;
	std::map<unsigned, Texture> textures;
	std::map<unsigned, Sampler> samplers;
	std::map<unsigned, Uav> uavs;
};

typedef std::map<unsigned, unsigned> RenderStates;

// Saved as:
//	stageCount: BYTE
//  InputStage x stageCount
//	inputCount: BYTE
//  InputDescription x inputCount
//	stateCount: BYTE
//  (state: DWORD value: DWORD) x stateCount
struct Pass
{
	size_t GetPackedSize()
	{
		size_t size = sizeof( BYTE );
		for( auto it = stages.begin(); it != stages.end(); ++it )
		{
			size += it->GetPackedSize();
		}
		size += sizeof( BYTE ) + states.size() * 2 * sizeof( DWORD );
		return size;
	}
	void Save( BYTE*& buffer )
	{
		*reinterpret_cast<BYTE*>( buffer ) = stages.size();
		buffer += sizeof( BYTE );
		for( auto it = stages.begin(); it != stages.end(); ++it )
		{
			BYTE* start = buffer;
			it->Save( buffer );
			assert( buffer - start == it->GetPackedSize() );
		}
		*reinterpret_cast<BYTE*>( buffer ) = states.size();
		buffer += sizeof( BYTE );
		for( auto it = states.begin(); it != states.end(); ++it )
		{
			*reinterpret_cast<DWORD*>( buffer ) = it->first;
			buffer += sizeof( DWORD );
			*reinterpret_cast<DWORD*>( buffer ) = it->second;
			buffer += sizeof( DWORD );
		}
	}

	std::vector<StageInput> stages;
	RenderStates states;
};

// Saved as:
//	type: BYTE
//	value: DWORD
struct Annotation
{
	size_t GetPackedSize()
	{
		return sizeof( BYTE ) + sizeof( DWORD );
	}
	void Save( BYTE*& buffer )
	{
		*reinterpret_cast<BYTE*>( buffer ) = type;
		buffer += sizeof( BYTE );
		*reinterpret_cast<DWORD*>( buffer ) = intValue;
		buffer += sizeof( DWORD );
	}

	AnnotationType type;
	union
	{
		float floatValue;
		int intValue;
		StringReference stringValue;
	};
};

// Saved as:
//	annotationCount: BYTE
//	(annotationName:StringReference Annotation) x annotationCount
struct ParameterAnnotation
{
	size_t GetPackedSize()
	{
		size_t size = sizeof( BYTE );
		for( auto it = annotations.begin(); it != annotations.end(); ++it )
		{
			size += sizeof( DWORD );
			size += it->second.GetPackedSize();
		}
		return size;
	}
	void Save( BYTE*& buffer )
	{
		*reinterpret_cast<BYTE*>( buffer ) = annotations.size();
		buffer += sizeof( BYTE );
		for( auto it = annotations.begin(); it != annotations.end(); ++it )
		{
			*reinterpret_cast<DWORD*>( buffer ) = it->first;
			buffer += sizeof( DWORD );

			BYTE* start = buffer;
			it->second.Save( buffer );
			assert( buffer - start == it->second.GetPackedSize() );
		}
	}

	std::map<StringReference, Annotation> annotations;
};

// Saved as:
//	passCount: BYTE
//	Pass x passCount
//	parameterAnnotationCount: WORD
//	(paramName:StringReference ParameterAnnotation) x parameterAnnotationCount
struct EffectData
{
	size_t GetPackedSize()
	{
		size_t size = sizeof( BYTE );
		for( auto it = passes.begin(); it != passes.end(); ++it )
		{
			size += it->GetPackedSize();
		}
		size += sizeof( WORD );
		for( auto it = annotations.begin(); it != annotations.end(); ++it )
		{
			size += sizeof( DWORD );
			size += it->second.GetPackedSize();
		}
		return size;
	}
	void Save( BYTE*& buffer )
	{
		*reinterpret_cast<BYTE*>( buffer ) = passes.size();
		buffer += sizeof( BYTE );
		for( auto it = passes.begin(); it != passes.end(); ++it )
		{
			BYTE* start = buffer;
			it->Save( buffer );
			assert( buffer - start == it->GetPackedSize() );
		}
		*reinterpret_cast<WORD*>( buffer ) = annotations.size();
		buffer += sizeof( WORD );
		for( auto it = annotations.begin(); it != annotations.end(); ++it )
		{
			*reinterpret_cast<DWORD*>( buffer ) = it->first;
			buffer += sizeof( DWORD );
			BYTE* start = buffer;
			it->second.Save( buffer );
			assert( buffer - start == it->second.GetPackedSize() );
		}
	}

	std::vector<Pass> passes;
	std::map<StringReference, ParameterAnnotation> annotations;
};

extern bool SaveEffectData( EffectData& data, ID3DXBuffer** buffer );

#endif // EffectData_H