#ifndef TR2EFFECT_H
#define TR2EFFECT_H

#include "ITr2ShaderMaterial.h"
#include "Resources/Tr2EffectRes.h"
#include "IRenderCallback.h"

BLUE_DECLARE( TriTextureParameter );
BLUE_DECLARE( Tr2VariableStore );
BLUE_DECLARE( Tr2LodResource );
BLUE_DECLARE_INTERFACE( ITriEffectParameter );
BLUE_DECLARE_IVECTOR( ITriEffectParameter );
BLUE_DECLARE_INTERFACE( ITriEffectResourceParameter );
BLUE_DECLARE_IVECTOR( ITriEffectResourceParameter );
BLUE_DECLARE_INTERFACE( ITr2ShaderState );

BLUE_CLASS_ALLOW_DELAYED_DELETE( Tr2Effect );


struct Tr2SamplerOverride
{
	BlueSharedString name;

	Tr2RenderContextEnum::TextureAddressMode addressU;
	Tr2RenderContextEnum::TextureAddressMode addressV;
	Tr2RenderContextEnum::TextureAddressMode addressW;
	Tr2RenderContextEnum::TextureFilter filter;
	Tr2RenderContextEnum::TextureFilter mipFilter;
	float lodBias;
	uint32_t maxMipLevel;
	uint32_t maxAnisotropy;
};

BLUE_DECLARE_STRUCTURE_LIST( Tr2SamplerOverride );


BLUE_CLASS( Tr2Effect ) :
	public IInitialize,
	public INotify,
	public IListNotify,
	public IBlueAsyncResNotifyTarget,
	public ITr2ShaderMaterial,
	public Tr2DeviceResource
{
public:    		
	using IRoot::Lock;
	using IRoot::Unlock;

	EXPOSE_TO_BLUE();

	Tr2Effect(IRoot* lockobj = NULL);	
	virtual ~Tr2Effect();

	bool IsEqual( Tr2Effect* other );

	// Utility Functions
	bool PopulateParameters();
	bool PruneParameters();

	// Suppress notification to changed lists
	void StartUpdate();
	void EndUpdate();
	
	// gets
	virtual Tr2EffectRes* GetEffectRes() const;
	const char* GetName() const;

	// sets & adds
	void SetEffectPathName( const char* path );
	bool AddResourceTexture2D( const BlueSharedString& name, const char* resPath );
	bool AddResourceTexture2DLod( const BlueSharedString& name, Tr2LodResourcePtr lodResource );
	bool AddSamplerOverride( const BlueSharedString& name, Tr2RenderContextEnum::TextureAddressMode addressModeU, Tr2RenderContextEnum::TextureAddressMode addressModeV );
	bool AddParameterVector4( const BlueSharedString& name, const Vector4* value );
	bool AddParameterFloat( const BlueSharedString& name, float value );
	bool AddParameterColor( const BlueSharedString& name, const Color* value );

	// access parameters and resources
	const char* GetEffectPathName() const;
	ITriEffectParameter* GetResourceByName( const char* name ) const;
	bool HasSamplerOverride( const char* name ) const;
	bool HasParameter( const char* name ) const;

    // This function is called by Tr2Renderer to update Tr2Materials
    // with any changes to the parameters
	virtual void UpdateMaterial() {};

	void Render( IRenderCallback* cb, Tr2RenderContext& renderContext );
	void RenderForPicking( IRenderCallback* cb, int objId, Tr2RenderContext& renderContext );

	unsigned GetHashValue() const;

	////////////////////////////////////////////
	// IShaderMaterial
	///////////////
	uint32_t ApplyMaterialDataForPass( unsigned int passIndex, Tr2RenderContext& renderContext );
	void ApplyShaderInputs( unsigned int passIndex, Tr2RenderContextEnum::ShaderType shaderType, Tr2RenderContext& renderContext );
	unsigned int GetSortValue() const;
	ITr2ShaderState* GetShaderStateInterface() const;
	void SetVariableStore( Tr2VariableStore* variableStore );
	const Tr2ConstantEffectParameter* GetConstParameters( size_t& count ) const;
	void UnloadResources();
	bool LoadResources();
	ITriEffectParameter* GetParameterByName( const char* name ) const;

	/////////////////////////////////////////////////////////////////////////////////////
	// INotify
	/////////////////////////////////////////////////////////////////////////////////////
	bool OnModified( Be::Var* val );

	/////////////////////////////////////////////////////////////////////////////////////
	// IInitialize
	/////////////////////////////////////////////////////////////////////////////////////
	virtual bool Initialize();

	/////////////////////////////////////////////////////////////////////////////////////
	// IListNotify
	/////////////////////////////////////////////////////////////////////////////////////
	void OnListModified(
		long event,
		ssize_t key,
		ssize_t key2,
		IRoot* value,
		const IList* theList
		);

	/////////////////////////////////////////////////////////////////////////////////////
	// IBlueAsyncResNotifyTarget
	/////////////////////////////////////////////////////////////////////////////////////
	void ReleaseCachedData( BlueAsyncRes* p );
	void RebuildCachedData( BlueAsyncRes* p );

protected:
	void RebuildCachedDataInternal();

	std::string m_name;
	std::string m_effectFilePath;			// Path to the effect file as set by user
	std::string m_actualEffectFilePath;		// Path to effect file, adjusted for shader model

	unsigned int m_parameterHash;
	bool m_display;

	unsigned int m_sortValue;

	Tr2EffectResPtr m_effectResource;

	virtual void ReleaseResources( TriStorage s );
	virtual bool OnPrepareResources();
private:
	void RebuildSamplerOverrides();
public: // TODO: make this private - need to change EveBoosterSet2...
	// Our list of ITr2EffectParameters
	typedef PITriEffectParameterVector EffectParameterList;
	EffectParameterList m_parameters;	

	// Effect Resources. These need some more care than normal parameters.
	typedef PITriEffectResourceParameterVector EffectResourceList;
	EffectResourceList m_resources;

	PTr2ConstantEffectParameterStructureList m_constParameters;
	PTr2SamplerOverrideStructureList m_samplerOverrides;

private:
#if TRINITYDEV
	bool m_insideBegin;
	bool m_insideBeginPass;
#endif

	bool m_insideStartUpdate;

protected:
	Tr2EffectPassParametersVector m_parametersForPasses;

	virtual void MapPassResources( 
		const Tr2EffectResourceMap& resources, 
		Tr2EffectParamVector &pv, 
		uint32_t resourceFlags );

	// Python
	bool IsParameterUsedByTechnique( const std::string& parameterName );

	// Utility
	virtual ITriEffectParameter* FindParameterByName( const char* name ) const;

	// Variable store to use for binding variables
	Tr2VariableStorePtr m_variableStore;
	Tr2VariableStore& GetVariableStore();

};
TYPEDEF_BLUECLASS(Tr2Effect);
BLUE_DECLARE_VECTOR( Tr2Effect );

void HashSamplers( Tr2EffectPassParameters::StageInput& stageInput );

void RebuildCachedDataForEffect(	
						ITr2ShaderState &effectResource, 
						ITr2ShaderMaterial &owner,
						Tr2EffectPassParametersVector& parametersForPasses );

uint32_t ApplyMaterialDataForPass( 
						Tr2EffectPassParametersVector& vec, 
						ITr2ShaderState* resource, 
						unsigned passIndex, 
						Tr2RenderContext& renderContext );

void ApplyShaderInputs( Tr2EffectPassParameters& parametersForPass, 
						Tr2RenderContextEnum::ShaderType shaderType,
						bool& samplersChanged,
						Tr2RenderContext& renderContext );

void ConvertEffectConstant( 
						const Tr2EffectConstant& constant, 
						const char* constantValues,
						std::function<void(ITriEffectParameter*)> adder );

void ConvertEffectResource(	
						const Tr2EffectResource& resource, 
						std::function<void(ITriEffectParameter*)> paramAdder,
						std::function<void(ITriEffectResourceParameter*)> resourceAdder );

void MapPassParameters( 
						unsigned passIx,
						Tr2EffectPassParameters& pp,
						Tr2RenderContextEnum::ShaderType stage,
						const Tr2EffectConstantVector& constants, 
						ITr2ShaderState& resource, 
						const Tr2EffectDescription& desc,
						ITr2ShaderMaterial& owner,
						Tr2RenderContext& renderContext );

#endif
