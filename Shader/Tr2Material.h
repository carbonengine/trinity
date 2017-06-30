////////////////////////////////////////////////////////////
//
//    Created:   June 2017
//    Copyright: CCP 2017
//

#pragma once

#include "Tr2EffectDescription.h"

BLUE_DECLARE( Tr2Shader );
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
	// Initial count for buffer list UAVs (default -1 means the counter is not reset)
	uint32_t m_initialCount;
};

typedef std::vector<Tr2EffectParam>		Tr2EffectParamVector;

struct Tr2SamplerOverrideData
{
	uint32_t registerIndex;
	uint32_t handle;
};

typedef std::vector<Tr2SamplerOverrideData> Tr2SamplerOverrideDataVector;

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

	// ----------------------------------------------------------------------------------
	// Description:
	//   Represents per-render-stage inputs.
	// ----------------------------------------------------------------------------------
	struct StageInput
	{
		Tr2EffectParamVector m_shaderParameters;
		Tr2EffectParamVector m_textures;
		Tr2EffectParamVector m_uavs;
		Tr2SamplerOverrideDataVector m_samplers;
		std::unique_ptr<Tr2ConstantBufferAL> m_constantBuffer;
	};

	StageInput m_stageInput[Tr2RenderContextEnum::SHADER_TYPE_COUNT];
	std::vector<ITriReroutable*> m_reroutedParameters;
};

typedef std::vector<std::unique_ptr<Tr2EffectPassParameters>> Tr2EffectPassParametersVector;


BLUE_CLASS( Tr2Material ): public IRoot
{
public:
	Tr2Material( IRoot* lockobj = nullptr );
	~Tr2Material();

	EXPOSE_TO_BLUE();

	uint32_t ApplyMaterialDataForPass( unsigned int passIndex, Tr2RenderContext& renderContext ) const;
	void ApplyShaderInputs( unsigned int passIndex, Tr2RenderContextEnum::ShaderType shaderType, Tr2RenderContext& renderContext ) const;
	uint32_t GetSortValue() const;
	Tr2Shader* GetShaderStateInterface() const;

	virtual void UnloadResources();
	virtual bool LoadResources();
protected:
	Tr2ShaderPtr m_shader;
	Tr2EffectPassParametersVector m_parametersForPasses;
	uint32_t m_sortValue;
private:
	void ApplyShaderInputs( unsigned int passIndex, Tr2RenderContextEnum::ShaderType shaderType, bool& samplersChanged, Tr2RenderContext& renderContext ) const;
};

TYPEDEF_BLUECLASS( Tr2Material );