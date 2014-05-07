#ifndef TR2EFFECTRES_H
#define TR2EFFECTRES_H


#include "ITr2EffectValue.h"
#include "Tr2DeviceResource.h"
#include "ITriReroutable.h"
#include "ITr2ShaderState.h"
#include "ITr2ShaderMaterial.h"

class TriVariable;
struct ITriEffectResourceParameter;

BLUE_DECLARE( Tr2EffectRes );

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
	Tr2EffectPassParameters()
	{
	}
	~Tr2EffectPassParameters()
	{
		for( ITriReroutableVector::iterator it = m_reroutedParameters.begin(); it != m_reroutedParameters.end(); ++it )
		{
			(*it)->SetDestination( NULL, 0 );
			(*it)->Unlock();
		}
	}

	void AllocateConstantMirror( Tr2RenderContextEnum::ShaderType type, unsigned int size )
	{
		if( size )
		{
			// fix the size to be multiple of Vector4s
			if( size % sizeof( Vector4 ) )
			{
				size += sizeof( Vector4 ) - size % sizeof( Vector4 );
			}

			USE_MAIN_THREAD_RENDER_CONTEXT();
			if( !m_stageInput[type].m_constantBuffer )
			{
				m_stageInput[type].m_constantBuffer.reset( CCP_NEW( "Tr2EffectPassParameters::m_stageInput::m_constantBuffer" ) Tr2ConstantBufferAL );
			}
			m_stageInput[type].m_constantBuffer->Create( 
				size, 
				Tr2RenderContextEnum::USAGE_CPU_WRITE | Tr2RenderContextEnum::USAGE_LOCK_FREQUENTLY, 
				nullptr, 
				renderContext );
		}
	}

	// ----------------------------------------------------------------------------------
	// Description:
	//   Represents per-render-stage inputs.
	// ----------------------------------------------------------------------------------
	struct StageInput
	{
		Tr2EffectParamVector m_shaderParameters;
		Tr2EffectParamVector m_samplers;
		Tr2EffectParamVector m_uavs;
		std::unique_ptr<Tr2ConstantBufferAL> m_constantBuffer;
	};

	StageInput m_stageInput[Tr2RenderContextEnum::SHADER_TYPE_COUNT];
	ITriReroutableVector m_reroutedParameters;
};

typedef std::vector<std::unique_ptr<Tr2EffectPassParameters>> Tr2EffectPassParametersVector;

BLUE_CLASS( Tr2EffectRes ): 
	public BlueAsyncRes,
	public ICacheable,
	public Tr2DeviceResource,
	public ITr2ShaderState
{
public:
	EXPOSE_TO_BLUE();

	Tr2EffectRes( IRoot* lockobj = NULL );
	~Tr2EffectRes();

	//////////////////////////////////////////////////////////////////////////
	// ICacheable
	bool IsMemoryUsageKnown();
	size_t GetMemoryUsage();

	unsigned int GetSortValue() const { return m_sortValue; }

	/////////////////
	// ITr2ShaderState
	uint32_t GetPassCount() const;
	uint32_t GetShaderTypeMask();
	void ApplyAllStateForPass(	uint32_t passIx,
								Tr2RenderContext &renderContext ) const;
	void ApplyShader(			uint32_t passIx, 
								Tr2RenderContextEnum::ShaderType type,
								Tr2RenderContext &renderContext ) const;
	void ApplyRenderStates(		uint32_t passIx, 
								Tr2RenderContext &renderContext ) const;
	void ApplySamplerStates(	uint32_t passIx, 
								Tr2RenderContextEnum::ShaderType type,
								Tr2RenderContext &renderContext ) const;

	const Tr2EffectConstant* GetConstant( const char* name ) const;
	const Tr2EffectResource* GetResource( const char* name ) const;
	const Tr2EffectParameterAnnotationMap* GetParameterAnnotations( const char* parameterName ) const;
	///////////////

	const Tr2Pass& GetPass( unsigned int passIx ) const;

	const Tr2EffectDescription& GetEffectDescription() const;

	const Tr2ShaderInputDefinition* GetShaderInputDefinition( 
								uint32_t passIx, 
								Tr2RenderContextEnum::ShaderType type );

	/////////////////////////////////////////////////////////////////////////////////////
	// ITriDeviceResource
	void ReleaseResources( TriStorage s );
#if TRINITYDEV
	virtual void GetDescription( std::string& desc );
#endif
private:
	bool OnPrepareResources();

	// Provide the functions that do the actual work of loading and preparing.
	// The async management itself is done in BlueAsyncRes.
	virtual bool DoOpenStream();
	virtual LoadingResult DoLoad();
	virtual bool DoPrepare();
	virtual void DoCloseStream();

protected:
	IBlueStreamPtr m_dataStream;
	uint8_t* m_data;
	uint32_t m_dataSize;

	unsigned int m_shaderTypeMask;

	char* m_stringTable;
	Tr2EffectDescription m_effect;

	unsigned int m_sortValue;

	friend class Tr2EffectStateManager;
};

TYPEDEF_BLUECLASS_WR_SHUTDOWN( Tr2EffectRes );

#endif
