////////////////////////////////////////////////////////////
//
//    Created:   June 2017
//    Copyright: CCP 2017
//

#pragma once

#include "Tr2EffectDescription.h"

BLUE_DECLARE( Tr2Shader );
BLUE_DECLARE_INTERFACE( ITriEffectTextureParameter );
BLUE_DECLARE_INTERFACE( ITr2EffectValue );
BLUE_DECLARE_INTERFACE( ITriReroutable );

// Tr2EffectParam describes one parameter for an effect. It provides a mapping from
// the source data.
class Tr2EffectParam
{
public:
	// Name of variable
	std::string m_sourceName;

	ITr2EffectValuePtr m_sourceValue;

	// Offset of parameter value in constant buffer in bytes
	unsigned int m_registerIndex;
	// Size of parameter value in bytes
	unsigned int m_registerCount;
};

typedef std::vector<Tr2EffectParam>		Tr2EffectParamVector;

struct Tr2SamplerOverrideData
{
	uint32_t registerIndex;
	Tr2SamplerStateAL sampler;
};

typedef std::vector<Tr2SamplerOverrideData> Tr2SamplerOverrideDataVector;


class Tr2SharedConstantBuffers
{
public:
	struct Key
	{
		Key() :
			size( 0 ),
			hash( 0 ),
			contents( nullptr )
		{
		}

		uint32_t size;
		uint32_t hash;
		const void* contents;
	};

	std::pair<Key, Tr2ConstantBufferAL> GetBuffer( const void* contents, uint32_t size );
	void ReleaseBuffer( const Key& key );

private:
	struct Value
	{
		Tr2ConstantBufferAL buffer;
		uint32_t refCount;
	};

	struct KeyHash
	{
		size_t operator()( const Key& cb ) const
		{
			return cb.hash;
		}
	};

	struct KeyEquals
	{
		size_t operator()( const Key& key1, const Key& key2 ) const
		{
			return key1.size == key2.size && key1.hash == key2.hash && memcmp( key1.contents, key2.contents, key1.size ) == 0;
		}
	};

	std::unordered_map<Key, Value, KeyHash, KeyEquals> m_buffers;
};


// Tr2EffectPassParameters holds information on parameters for the effect pass.
// * For each vertex/pixel shader parameter there is a Tr2EffectParam instance
//   describing where the value comes from.
// * For each sampler used there is a Tr2SamplerSetup structure.
// * Optionally, there is a block of Vector4 values with the default values
//   for any vertex/pixel shader constants. This is needed when constants are
//   given values in the .fx file.
class Tr2EffectPassParameters
{
public:
	Tr2EffectPassParameters();
	~Tr2EffectPassParameters();

	void AllocateConstantMirror( Tr2RenderContextEnum::ShaderType type, unsigned int size );
	void GetSharedConstantBuffer( Tr2RenderContextEnum::ShaderType type, const void* contents, unsigned int size );

	// ----------------------------------------------------------------------------------
	// Description:
	//   Represents per-render-stage inputs.
	// ----------------------------------------------------------------------------------
	struct StageInput
	{
		StageInput();
		~StageInput();

		void AllocateConstants( uint32_t size );
		void GetSharedConstantBuffer( const void* contents, uint32_t size );

		Tr2EffectParamVector m_shaderParameters;
		Tr2EffectParamVector m_textures;
		Tr2EffectParamVector m_uavs;
		Tr2SamplerOverrideDataVector m_samplers;
		Tr2ConstantBufferAL m_constantBuffer;
		Tr2SharedConstantBuffers::Key m_sharedBufferKey;
		CcpMallocBuffer m_constantMirror;
		bool m_constantBufferDirty;
	};

	StageInput m_stageInput[Tr2RenderContextEnum::SHADER_TYPE_COUNT];
	std::vector<ITriReroutable*> m_reroutedParameters;
	Tr2ResourceSetDescriptionAL m_resourceSetDesc;
	Tr2ResourceSetAL m_resourceSet;
	uint32_t m_resourceSetHash;
	bool m_resourceSetDirty;
};

typedef std::vector<std::unique_ptr<Tr2EffectPassParameters>> Tr2EffectPassParametersVector;
typedef std::vector<Tr2EffectPassParametersVector> Tr2EffectTechniqueParametersVector;

BLUE_CLASS( Tr2Material ): 
	public IRoot
{
public:
	Tr2Material( IRoot* lockobj = nullptr );
	~Tr2Material();

	EXPOSE_TO_BLUE();

	void ApplyMaterialDataForPass( uint32_t techniqueIndex, unsigned int passIndex, Tr2RenderContext& renderContext ) const;
	uint64_t GetSortValue() const;
	Tr2Shader* GetShaderStateInterface() const;

	virtual void SetOption( const BlueSharedString& name, const BlueSharedString& value ) {}

	Tr2EffectPassParameters* GetPassDescription( uint32_t techniqueIndex, uint32_t passIndex );

	void InvalidateResourceSets();

	void UsedWithScreenSize( float screenSize, float worldRadius, const std::vector<float>& uvDensities );

protected:
	bool ApplyShaderInputs( uint32_t techniqueIndex, unsigned int passIndex, Tr2RenderContextEnum::ShaderType shaderType, Tr2RenderContext& renderContext ) const;

	Tr2ShaderPtr m_shader;
	Tr2EffectTechniqueParametersVector m_parametersForPasses;
	std::vector<ITriEffectTextureParameterPtr> m_lodTextureParameters;
	mutable uint32_t m_resourceSetHash;
};

TYPEDEF_BLUECLASS( Tr2Material );