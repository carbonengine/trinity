// Copyright © 2017 CCP ehf.

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

typedef std::vector<Tr2EffectParam> Tr2EffectParamVector;

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

// ----------------------------------------------------------------------------------
// Description:
//   Represents per-render-stage inputs.
// ----------------------------------------------------------------------------------
struct Tr2MaterialStageInput
{
	Tr2MaterialStageInput();
	~Tr2MaterialStageInput();

	Tr2EffectParamVector m_shaderParameters;
	Tr2EffectParamVector m_shaderParametersWithNotification;
	Tr2EffectParamVector m_textures;
	Tr2EffectParamVector m_uavs;
	Tr2ConstantBufferAL m_constantBuffer;
	CcpMallocBuffer m_constantMirror;
	bool m_constantBufferDirty;
	Tr2SharedConstantBuffers::Key m_sharedBufferKey;
	//Tr2SamplerOverrideDataVector m_samplers;

	// previously AllocateConstantMirror
	//bool AllocateConstantBuffer( uint32_t size );

	// new AllocateConstantMirror
	void AllocateConstants( uint32_t size );
	void GetSharedConstantBuffer( const void* contents, uint32_t size );
};

class PassParametersOwner
{

public:
	virtual ~PassParametersOwner() {};
	virtual void AddUsedResource( ITr2EffectValuePtr resource ) = 0;
	virtual void AddReroutable( ITriReroutable* reroutable ) = 0;
};

// Tr2EffectPassParameters holds information on parameters for the effect pass.
// * For each vertex/pixel shader parameter there is a Tr2EffectParam instance
//   describing where the value comes from.
// * For each sampler used there is a Tr2SamplerSetup structure.
// * Optionally, there is a block of Vector4 values with the default values
//   for any vertex/pixel shader constants. This is needed when constants are
//   given values in the .fx file.
class Tr2EffectPassParameters : public PassParametersOwner
{
public:
	Tr2EffectPassParameters();
	~Tr2EffectPassParameters();

	void AllocateConstantMirror( Tr2RenderContextEnum::ShaderType type, unsigned int size );
	void GetSharedConstantBuffer( Tr2RenderContextEnum::ShaderType type, const void* contents, unsigned int size );

	Tr2MaterialStageInput m_stageInput[Tr2RenderContextEnum::SHADER_TYPE_COUNT];


	std::vector<ITriReroutable*> m_reroutedParameters;
	Tr2ResourceSetDescriptionAL m_resourceSetDesc;
	Tr2ResourceSetAL m_resourceSet;
	std::vector<ITr2EffectValuePtr> m_usedResources;
	Tr2BindlessResourcesAL m_usedTextures;
	uint32_t m_resourceSetHash;
	bool m_resourceSetDirty;
	bool m_compatibleWithGdr;
	bool m_usedTexturesDirty;

	void AddUsedResource( ITr2EffectValuePtr resource ) override;
	void AddReroutable( ITriReroutable* reroutable ) override;
};

struct Tr2EffectLibraryParameters : public PassParametersOwner
{
	Tr2EffectLibraryParameters();

	Tr2MaterialStageInput m_localInput;

	Tr2MaterialStageInput m_globalInput;
	Tr2ResourceSetDescriptionAL m_globalResourceSetDesc;
	Tr2ResourceSetAL m_globalResourceSet;
	std::vector<ITriReroutable*> m_reroutedParameters;
	std::vector<ITr2EffectValuePtr> m_usedResources;
	Tr2BindlessResourcesAL m_usedTextures;
	bool m_globalResourceSetDirty;
	bool m_usedTexturesDirty;

	void AddUsedResource( ITr2EffectValuePtr resource ) override;
	void AddReroutable( ITriReroutable* reroutable ) override;
};

struct Tr2EffectTechniqueInputs
{
	Tr2EffectTechniqueInputs()
	{
	}
	Tr2EffectTechniqueInputs( Tr2EffectTechniqueInputs&& other )
	{
		std::swap( passes, other.passes );
		std::swap( libraries, other.libraries );
	}
	~Tr2EffectTechniqueInputs()
	{
	}

	Tr2EffectTechniqueInputs& operator=( Tr2EffectTechniqueInputs&& other )
	{
		std::swap( passes, other.passes );
		std::swap( libraries, other.libraries );
		return *this;
	}

	std::vector<std::unique_ptr<Tr2EffectPassParameters>> passes;
	std::vector<std::unique_ptr<Tr2EffectLibraryParameters>> libraries;
};

typedef std::vector<Tr2EffectTechniqueInputs> Tr2EffectTechniqueParametersVector;

//typedef std::vector<std::unique_ptr<Tr2EffectPassParameters>> Tr2EffectPassParametersVector;
//typedef std::vector<Tr2EffectPassParametersVector> Tr2EffectTechniqueParametersVector;

BLUE_CLASS( Tr2Material ) :
	public IRoot
{
public:
	Tr2Material( IRoot* lockobj = nullptr );
	~Tr2Material();

	EXPOSE_TO_BLUE();

	void ApplyMaterialDataForPass( uint32_t techniqueIndex, unsigned int passIndex, Tr2RenderContext& renderContext ) const;
	void ApplyMaterialDataForPassWithOverride( uint32_t techniqueIndex, unsigned int passIndex, uint32_t overrideProgram, Tr2RenderContext& renderContext ) const;
	void ApplyMaterialDataForRtState( uint32_t techniqueIndex, const Tr2RtPipelineStateAL& rtPipelineState, Tr2RenderContext& renderContext ) const;
	void ApplyMaterialDataForRtMaterial( uint32_t techniqueIndex, Tr2RtLocalMaterialDescriptionAL& localMaterial, Tr2RenderContext& renderContext ) const;
	uint64_t GetSortValue() const;
	Tr2Shader* GetShaderStateInterface() const;


	virtual void SetOption( const BlueSharedString& name, const BlueSharedString& value )
	{
	}

	Tr2EffectPassParameters* GetPassDescription( uint32_t techniqueIndex, uint32_t passIndex );

	void InvalidateResourceSets();
	void ResourceChanged();
	void MarkConstantBuffersDirty();

	void UsedWithScreenSize( float screenSize, float worldRadius, const std::vector<float>& uvDensities );

	void GetUsedBindlessTextures( uint32_t techniqueIndex, Tr2BindlessResourcesAL& usedTextures );

	bool CompatibleWithGdr() const;
	bool CompatibleWithGdr( const BlueSharedString& techniqueName ) const;
	void ApplyConstantBuffers( uint32_t techniqueIndex, unsigned int passIndex, Tr2IndirectDrawBufferWriter& indirectBuffer, Tr2RenderContext& renderContext );

protected:
	bool ApplyShaderInputs( uint32_t techniqueIndex, unsigned int passIndex, Tr2RenderContextEnum::ShaderType shaderType, Tr2RenderContext& renderContext ) const;
	bool ApplyShaderInputs( Tr2EffectPassParameters & pp, Tr2RenderContextEnum::ShaderType shaderType, Tr2RenderContext & renderContext ) const;

	void ApplyConstants( Tr2RenderContextEnum::ShaderType shaderType, Tr2MaterialStageInput & input, bool hasReroutables, Tr2RenderContext& renderContext ) const;
	void UpdateConstants( Tr2RenderContextEnum::ShaderType shaderType, Tr2MaterialStageInput & input, bool hasReroutables, Tr2RenderContext& renderContext ) const;
	bool UpdateResourceSetDesc( Tr2RenderContextEnum::ShaderType shaderType, Tr2MaterialStageInput & input, Tr2ResourceSetDescriptionAL & desc ) const;


	Tr2ShaderPtr m_shader;
	Tr2EffectTechniqueParametersVector m_parametersForPasses;
	Tr2EffectTechniqueParametersVector m_parametersForLibraries;
	std::vector<ITriEffectTextureParameterPtr> m_lodTextureParameters;
	mutable uint32_t m_resourceSetHash;
	bool m_compatibleWithGdr;
};

TYPEDEF_BLUECLASS( Tr2Material );
