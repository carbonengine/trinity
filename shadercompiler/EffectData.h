// Copyright © 2023 CCP ehf.

#pragma once
#ifndef EffectData_H
#define EffectData_H

#include "StringTable.h"
#include <vector>

const uint32_t DATA_VERSION = 15;

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
	CONSTANT_TYPE_UINT = 2,
	CONSTANT_TYPE_BOOL = 3,
	CONSTANT_TYPE_OTHER = 4,
};

const char* ToString( ConstantType constType );

enum AnnotationType
{
	ANNOTATION_TYPE_BOOL = 0,
	ANNOTATION_TYPE_INT = 1,
	ANNOTATION_TYPE_FLOAT = 2,
	ANNOTATION_TYPE_STRING = 3,
};

enum TextureType
{
	TEX_TYPE_1D = 1,
	TEX_TYPE_2D,
	TEX_TYPE_3D,
	TEX_TYPE_CUBE,

	TEX_TYPE_TYPELESS, // valid but unknown dimensions

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

	// raytracing
	TEX_TYPE_RAYTRACING_ACCELERATION_STRUCTURE,

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

const char* GetStringForUsageCode( int usageCode );

enum RegisterInputType
{
	RT_CONSTANT_BUFFER,
	RT_SAMPLER,

	RT_SRV_BUFFER = 1 << 5,
	RT_SRV_STRUCTURED_BUFFER,
	RT_SRV_TEXTURE1D,
	RT_SRV_TEXTURE1DARRAY,
	RT_SRV_TEXTURE2D,
	RT_SRV_TEXTURE2DARRAY,
	RT_SRV_TEXTURE2DMS,
	RT_SRV_TEXTURE2DMSARRAY,
	RT_SRV_TEXTURE3D,
	RT_SRV_TEXTURECUBE,
	RT_SRV_TEXTURECUBEARRAY,

	RT_UAV_BUFFER = 1 << 6,
	RT_UAV_STRUCTURED_BUFFER,
	RT_UAV_TEXTURE1D,
	RT_UAV_TEXTURE1DARRAY,
	RT_UAV_TEXTURE2D,
	RT_UAV_TEXTURE2DARRAY,
	RT_UAV_TEXTURE2DMS,
	RT_UAV_TEXTURE2DMSARRAY,
	RT_UAV_TEXTURE3D,
	RT_UAV_TEXTURECUBE,
	RT_UAV_TEXTURECUBEARRAY,
};

enum class RtShaderType : uint8_t
{
	RAY_GEN,
	MISS,
	CLOSEST_HIT,
	ANY_HIT,
	INTERSECTION,
};

template <typename T, typename P>
class PackAs
{
public:
	PackAs() = default;

	PackAs( T v ) :
		t( v )
	{
	}

	operator T() const
	{
		return t;
	}
	operator T&()
	{
		return t;
	}
	PackAs& operator=( T o )
	{
		t = o;
		return *this;
	}
	P Packed() const
	{
		return static_cast<P>( t );
	}

private:
	T t;
};




class PackedStream
{
public:
	explicit PackedStream( void* data, size_t size ) :
		m_size( size )
	{
		m_data = static_cast<uint8_t*>( data );
		m_start = m_data;
	}

	~PackedStream()
	{
	}

	size_t GetSize() const
	{
		return m_size;
	}

	const void* GetData() const
	{
		return m_start;
	}

	void Save( StringReference value )
	{
		extern StringTable g_stringTable;

		*reinterpret_cast<uint32_t*>( m_data ) = g_stringTable.GetOffset( value );
		m_data += sizeof( uint32_t );
	}
	void Save( float value )
	{
		*reinterpret_cast<float*>( m_data ) = value;
		m_data += sizeof( float );
	}
	void Save( uint32_t value )
	{
		*reinterpret_cast<uint32_t*>( m_data ) = value;
		m_data += sizeof( uint32_t );
	}
	void Save( uint16_t value )
	{
		*reinterpret_cast<uint16_t*>( m_data ) = value;
		m_data += sizeof( uint16_t );
	}
	void Save( uint8_t value )
	{
		*reinterpret_cast<uint8_t*>( m_data ) = value;
		m_data += sizeof( uint8_t );
	}
	void Save( bool value )
	{
		*reinterpret_cast<uint8_t*>( m_data ) = value ? 1 : 0;
		m_data += sizeof( uint8_t );
	}
	template <typename T, typename P>
	void Save( PackAs<T, P> value )
	{
		*reinterpret_cast<P*>( m_data ) = value.Packed();
		m_data += sizeof( P );
	}

private:
	uint8_t* m_data;
	void* m_start;
	size_t m_size;
};


class SizeCountStream
{
public:
	SizeCountStream() :
		m_size( 0 )
	{
	}

	size_t GetSize() const
	{
		return m_size;
	}

	void Save( StringReference )
	{
		m_size += sizeof( uint32_t );
	}
	void Save( float )
	{
		m_size += sizeof( float );
	}
	void Save( uint32_t )
	{
		m_size += sizeof( uint32_t );
	}
	void Save( uint16_t )
	{
		m_size += sizeof( uint16_t );
	}
	void Save( uint8_t )
	{
		m_size += sizeof( uint8_t );
	}
	void Save( bool )
	{
		m_size += sizeof( uint8_t );
	}
	template <typename T, typename P>
	void Save( PackAs<T, P> )
	{
		m_size += sizeof( P );
	}

private:
	size_t m_size;
};

struct Constant
{

	template <typename T>
	void Save( T& stream )
	{
		stream.Save( name );
		stream.Save( offset );
		stream.Save( size );
		stream.Save( type );
		stream.Save( dimension );
		stream.Save( elements );
		stream.Save( isSRGB );
		stream.Save( isAutoregister );
	}

	bool operator==( const Constant& other ) const
	{
		return name == other.name && offset == other.offset && size == other.size && type == other.type &&
			dimension == other.dimension && elements == other.elements && isSRGB == other.isSRGB && isAutoregister == other.isAutoregister;
	}

	StringReference name;
	uint32_t offset;
	uint32_t size;
	PackAs<ConstantType, uint8_t> type;
	uint8_t dimension;
	uint32_t elements;
	bool isSRGB;
	bool isAutoregister;
};

struct Vector4
{
	float x, y, z, w;
};


struct Texture
{
	template <typename T>
	void Save( T& stream )
	{
		stream.Save( name );
		stream.Save( type );
		stream.Save( count );
		stream.Save( isSRGB );
		stream.Save( isAutoregister );
	}

	bool operator==( const Texture& other ) const
	{
		return name == other.name && type == other.type && count == other.count && isSRGB == other.isSRGB && isAutoregister == other.isAutoregister;
	}

	StringReference name;
	PackAs<TextureType, uint8_t> type;
	uint32_t count = 1;
	bool isSRGB;
	bool isAutoregister;
};


struct Uav
{
	template <typename T>
	void Save( T& stream )
	{
		stream.Save( name );
		stream.Save( type );
		stream.Save( count );
		stream.Save( isAutoregister );
	}

	bool operator==( const Texture& other ) const
	{
		return name == other.name && type == other.type && count == other.count && isAutoregister == other.isAutoregister;
	}

	StringReference name;
	PackAs<TextureType, uint8_t> type;
	uint32_t count = 1;
	bool isAutoregister;
};


struct Sampler
{
	template <typename T>
	void Save( T& stream )
	{
		stream.Save( name );
		stream.Save( comparison );
		stream.Save( minFilter );
		stream.Save( magFilter );
		stream.Save( mipFilter );
		stream.Save( addressU );
		stream.Save( addressV );
		stream.Save( addressW );
		stream.Save( mipLODBias );
		stream.Save( maxAnisotropy );
		stream.Save( comparisonFunc );

		stream.Save( borderColor.x );
		stream.Save( borderColor.y );
		stream.Save( borderColor.z );
		stream.Save( borderColor.w );
		stream.Save( minLOD );
		stream.Save( maxLOD );
		stream.Save( isDynamic );
	}

	bool operator==( const Sampler& sampler ) const
	{
		return comparison == sampler.comparison &&
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
			maxLOD == sampler.maxLOD &&
			isDynamic == sampler.isDynamic;
	}

	bool operator<( const Sampler& sampler ) const
	{
#define COMPARE( field )        \
	if( field < sampler.field ) \
		return true;            \
	if( field > sampler.field ) \
		return false;
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
		COMPARE( isDynamic );
#undef COMPARE
		return false;
	}

	StringReference name;
	uint8_t filter; // unsaved ?
	uint8_t comparison;
	uint8_t minFilter;
	uint8_t magFilter;
	uint8_t mipFilter;
	uint8_t addressU;
	uint8_t addressV;
	uint8_t addressW;
	float mipLODBias;
	uint8_t maxAnisotropy;
	uint8_t comparisonFunc;
	Vector4 borderColor;
	float minLOD;
	float maxLOD;
	uint8_t srgbTexture; // unsaved ?
	uint8_t isDynamic;
};


struct PipelineInputDescription
{
	template <typename T>
	void Save( T& stream )
	{
		stream.Save( name );
		stream.Save( registerIndex );
		stream.Save( index );
		stream.Save( usedMask );
		stream.Save( type );
		stream.Save( dimension );
	}

	bool operator==( const PipelineInputDescription& other ) const
	{
		return name == other.name && registerIndex == other.registerIndex && index == other.index && usedMask == other.usedMask && type == other.type && dimension == other.dimension;
	}

	uint8_t name;
	uint8_t registerIndex;
	uint8_t index;
	uint8_t usedMask;
	PackAs<ConstantType, uint8_t> type;
	uint8_t dimension;
};


struct RegisterInputDescription
{
	template <typename T>
	void Save( T& stream )
	{
		stream.Save( registerType );
		stream.Save( registerIndex );
		stream.Save( registerCount );
		stream.Save( registerSpace );
	}

	bool operator==( const RegisterInputDescription& other ) const
	{
		return registerType == other.registerType && registerIndex == other.registerIndex && registerCount == other.registerCount && registerSpace == other.registerSpace;
	}

	PackAs<RegisterInputType, uint8_t> registerType;
	uint32_t registerIndex;
	uint32_t registerCount;
	uint8_t registerSpace;
};

struct StaticSampler
{
	template <typename T>
	void Save( T& stream )
	{
		stream.Save( registerIndex );
		stream.Save( registerSpace );
		stream.Save( comparison );
		stream.Save( minFilter );
		stream.Save( magFilter );
		stream.Save( mipFilter );
		stream.Save( addressU );
		stream.Save( addressV );
		stream.Save( addressW );
		stream.Save( mipLODBias );
		stream.Save( maxAnisotropy );
		stream.Save( comparisonFunc );

		stream.Save( borderColor );
		stream.Save( minLOD );
		stream.Save( maxLOD );
	}

	bool operator==( const StaticSampler& sampler ) const
	{
		return registerIndex == sampler.registerIndex &&
			registerSpace == sampler.registerSpace &&
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
			borderColor == sampler.borderColor &&
			minLOD == sampler.minLOD &&
			maxLOD == sampler.maxLOD;
	}

	uint32_t registerIndex;
	uint8_t registerSpace;

	uint8_t comparison;
	uint8_t minFilter;
	uint8_t magFilter;
	uint8_t mipFilter;
	uint8_t addressU;
	uint8_t addressV;
	uint8_t addressW;
	float mipLODBias;
	uint8_t maxAnisotropy;
	uint8_t comparisonFunc;
	uint8_t borderColor;
	float minLOD;
	float maxLOD;
};



struct Annotation
{
	template <typename T>
	void Save( T& stream )
	{
		stream.Save( type );
		if( type == ANNOTATION_TYPE_STRING )
		{
			stream.Save( stringValue );
		}
		else
		{
			stream.Save( floatValue );
		}
	}

	PackAs<AnnotationType, uint8_t> type;
	union
	{
		float floatValue;
		int32_t intValue;
	};
	StringReference stringValue;
};



struct ParameterAnnotation
{
	template <typename T>
	void Save( T& stream )
	{
		stream.Save( uint8_t( annotations.size() ) );

		std::vector<StringReference> keys;
		keys.reserve( annotations.size() );
		for( auto& k : annotations )
		{
			keys.push_back( k.first );
		}

		std::sort( begin( keys ), end( keys ), []( StringReference a, StringReference b ) {
			extern StringTable g_stringTable;
			return strcmp( g_stringTable.GetString( a ), g_stringTable.GetString( b ) ) < 0;
		} );

		for( auto key : keys )
		{
			stream.Save( key );
			annotations[key].Save( stream );
		}
	}

	std::map<StringReference, Annotation> annotations;
};


struct StageData
{
	template <typename T>
	void Save( T& stream )
	{
		stream.Save( uint8_t( registerInputs.size() ) );
		for( auto it = registerInputs.begin(); it != registerInputs.end(); ++it )
		{
			it->Save( stream );
		}

		stream.Save( uint8_t( staticSamplers.size() ) );
		for( auto& it : staticSamplers )
		{
			it.Save( stream );
		}

		stream.Save( uint32_t( constants.size() ) );
		for( auto it = constants.begin(); it != constants.end(); ++it )
		{
			it->Save( stream );
		}

		stream.Save( uint32_t( defaultValues.size() ) );
		stream.Save( defaultValuesStr );

		stream.Save( uint8_t( textures.size() ) );
		for( auto it = textures.begin(); it != textures.end(); ++it )
		{
			stream.Save( it->first );
			it->second.Save( stream );
		}

		stream.Save( uint8_t( samplers.size() ) );
		for( auto it = samplers.begin(); it != samplers.end(); ++it )
		{
			stream.Save( it->first );
			it->second.Save( stream );
		}

		stream.Save( uint8_t( uavs.size() ) );
		for( auto it = uavs.begin(); it != uavs.end(); ++it )
		{
			stream.Save( it->first );
			it->second.Save( stream );
		}
		annotations.Save( stream );
	}

	std::vector<RegisterInputDescription> registerInputs;
	std::vector<StaticSampler> staticSamplers;
	std::vector<Constant> constants;
	std::vector<BYTE> defaultValues;
	StringReference defaultValuesStr;
	std::map<uint8_t, Texture> textures;
	std::map<uint8_t, Sampler> samplers;
	std::map<uint8_t, Uav> uavs;
	ParameterAnnotation annotations;
};

struct StageInput : public StageData
{
	template <typename T>
	void Save( T& stream )
	{
		stream.Save( type );

		stream.Save( shaderSize );
		stream.Save( shaderDataStr );
		stream.Save( threadGroupSize[0] );
		stream.Save( threadGroupSize[1] );
		stream.Save( threadGroupSize[2] );

		stream.Save( uint8_t( pipelineInputs.size() ) );
		for( auto it = pipelineInputs.begin(); it != pipelineInputs.end(); ++it )
		{
			it->Save( stream );
		}

		StageData::Save( stream );
	}

	PackAs<InputStageType, uint8_t> type;
	uint32_t shaderSize;
	StringReference shaderDataStr;
	uint32_t threadGroupSize[3];
	std::vector<PipelineInputDescription> pipelineInputs;
	std::string source; // not persisted: for testing/debugging only
};

typedef std::map<uint32_t, uint32_t> RenderStates;


struct Pass
{
	template <typename T>
	void Save( T& stream )
	{
		stream.Save( uint8_t( stages.size() ) );
		for( auto it = stages.begin(); it != stages.end(); ++it )
		{
			it->Save( stream );
		}
		stream.Save( uint8_t( states.size() ) );
		for( auto it = states.begin(); it != states.end(); ++it )
		{
			stream.Save( it->first );
			stream.Save( it->second );
		}
	}

	std::vector<StageInput> stages;
	RenderStates states;
};

struct ShaderExport
{
	template <typename T>
	void Save( T& stream )
	{
		stream.Save( type );
		stream.Save( name );
	}

	PackAs<RtShaderType, uint8_t> type;
	StringReference name;
};


struct Library
{
	template <typename T>
	void Save( T& stream )
	{
		stream.Save( payloadSize );
		stream.Save( shaderSize );
		stream.Save( shaderDataStr );
		stream.Save( uint32_t( exports.size() ) );
		for( auto it : exports )
		{
			it.Save( stream );
		}
		stream.Save( hitGroupName );

		globalInputs.Save( stream );
		localInputs.Save( stream );
	}

	uint32_t payloadSize;
	uint32_t shaderSize;
	StringReference shaderDataStr;
	std::vector<ShaderExport> exports;
	StringReference hitGroupName;
	StageData globalInputs;
	StageData localInputs;
	std::string source; // not persisted: for testing/debugging only
};

struct Technique
{
	template <typename T>
	void Save( T& stream )
	{
		stream.Save( name );
		stream.Save( uint8_t( passes.size() ) );
		for( auto it = passes.begin(); it != passes.end(); ++it )
		{
			it->Save( stream );
		}

		stream.Save( uint8_t( libraries.size() ) );
		for( auto& it : libraries )
		{
			it.Save( stream );
		}
	}

	StringReference name;
	std::vector<Pass> passes;
	std::vector<Library> libraries;
};

struct PDB
{
	std::string name;
#if _WIN32
	CComPtr<IDxcBlob> pdbBlob; // this is for dx11/dx12. metal will require another field.
#endif
};

struct EffectData
{
	template <typename T>
	void Save( T& stream )
	{
		stream.Save( uint8_t( techniques.size() ) );
		for( auto it = techniques.begin(); it != techniques.end(); ++it )
		{
			it->Save( stream );
		}
		stream.Save( uint16_t( annotations.size() ) );


		std::vector<StringReference> keys;
		keys.reserve( annotations.size() );
		for( auto& k : annotations )
		{
			keys.push_back( k.first );
		}

		std::sort( begin( keys ), end( keys ), []( StringReference a, StringReference b ) {
			extern StringTable g_stringTable;
			return strcmp( g_stringTable.GetString( a ), g_stringTable.GetString( b ) ) < 0;
		} );

		for( auto key : keys )
		{
			stream.Save( key );
			annotations[key].Save( stream );
		}
	}

	std::vector<PDB> pdbs;
	std::vector<Technique> techniques;
	std::map<StringReference, ParameterAnnotation> annotations;
};

#endif // EffectData_H
