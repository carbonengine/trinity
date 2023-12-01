#ifndef TR2EFFECT_H
#define TR2EFFECT_H

#include "Resources/Tr2EffectRes.h"
#include "IRenderCallback.h"
#include "Tr2Material.h"

BLUE_DECLARE( TriTextureParameter );
BLUE_DECLARE( Tr2VariableStore );
BLUE_DECLARE_INTERFACE( ITriEffectParameter );
BLUE_DECLARE_IVECTOR( ITriEffectParameter );
BLUE_DECLARE_INTERFACE( ITriEffectResourceParameter );
BLUE_DECLARE_IVECTOR( ITriEffectResourceParameter );
BLUE_DECLARE( Tr2Shader );
BLUE_DECLARE_INTERFACE( ITr2GpuBuffer );
BLUE_DECLARE_INTERFACE( ITr2TextureProvider );

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

struct Tr2ConstantEffectParameter
{
	BlueSharedString name;
	Vector4 value;
};

BLUE_DECLARE_STRUCTURE_LIST( Tr2ConstantEffectParameter );


BLUE_DECLARE_STRUCTURE_LIST( Tr2ShaderOption );


BLUE_CLASS( Tr2Effect ) :
	public Tr2Material,
	public IInitialize,
	public INotify,
	public IListNotify,
	public IBlueAsyncResNotifyTarget,
	public Tr2DeviceResource
{
public:    		
	using IRoot::Lock;
	using IRoot::Unlock;

	EXPOSE_TO_BLUE();

	Tr2Effect(IRoot* lockobj = NULL);	
	virtual ~Tr2Effect();

	// Utility Functions
	bool PopulateParameters();
	bool PruneParameters();

	// Suppress notification to changed lists
	void StartUpdate();
	void EndUpdate();
	
	// gets
	Tr2EffectRes* GetEffectRes() const;
	const char* GetName() const;

	// sets & adds & clears
	void SetEffectPathName( const char* path );
	bool AddResource( ITriEffectParameter* param );
	bool AddResourceTexture2D( const BlueSharedString& name, const char* resPath );
	bool AddSamplerOverride( const BlueSharedString& name, Tr2RenderContextEnum::TextureAddressMode addressModeU, Tr2RenderContextEnum::TextureAddressMode addressModeV );
	bool AddParameterVector4( const BlueSharedString& name, const Vector4* value );
	bool AddParameterFloat( const BlueSharedString& name, float value );
	bool AddParameterColor( const BlueSharedString& name, const Color* value );
	void ClearAllParameters();
	void ClearAllResources();

	void SetParameter( const BlueSharedString& name, ITr2GpuBuffer* buffer );
	void SetParameter( const BlueSharedString& name, ITr2TextureProvider* texture );
	void SetParameter( const BlueSharedString& name, uint32_t value );
	void SetParameter( const BlueSharedString& name, float value );
	void SetParameter( const BlueSharedString& name, const Vector2& value ); 
	void SetParameter( const BlueSharedString& name, const Vector3& value ); 
	void SetParameter( const BlueSharedString& name, const Vector4& value );
	void SetParameter( const BlueSharedString& name, const Matrix& matrix );

	void SetOption( const BlueSharedString& name, const BlueSharedString& value );
	void ResetOption( const BlueSharedString& name );
	BlueSharedString GetOption( const BlueSharedString& name ) const;


	// access parameters and resources
	const char* GetEffectPathName() const;
	ITriEffectParameter* GetResourceByName( const char* name ) const;
	bool HasSamplerOverride( const char* name ) const;
	bool HasParameter( const char* name ) const;

	void Render( IRenderCallback* cb, Tr2RenderContext& renderContext );

	unsigned GetHashValue() const;

	const Tr2ConstantEffectParameter* GetConstParameters( size_t& count ) const;
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

	void RebuildCachedData();

	void SetVariableStore( Tr2VariableStore* variableStore );
	Tr2VariableStore& GetVariableStore();

protected:
	void RebuildCachedDataInternal();

	std::string m_name;
	std::string m_effectFilePath;			// Path to the effect file as set by user
	std::string m_actualEffectFilePath;		// Path to effect file, adjusted for shader model

	unsigned int m_parameterHash;
	bool m_display;

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
	PTr2ShaderOptionStructureList m_options;
	Tr2VariableStorePtr m_variableStore;


#if TRINITYDEV
	bool m_insideBegin;
	bool m_insideBeginPass;
#endif

	bool m_insideStartUpdate;

protected:
	void MapPassResources( 
		const Tr2EffectResourceMap& resources, 
		Tr2EffectParamVector &pv );
	void MapPassParameters(
		size_t technique,
		unsigned passIx,
		Tr2EffectPassParameters& pp,
		Tr2RenderContextEnum::ShaderType stage,
		const Tr2EffectConstantVector& constants,
		const Tr2EffectDescription& desc,
		Tr2RenderContext& renderContext );

	// Python
	bool IsParameterUsedByTechnique( const std::string& parameterName );

	// Utility
	ITriEffectParameter* FindParameterByName( const char* name ) const;
};
TYPEDEF_BLUECLASS(Tr2Effect);
BLUE_DECLARE_VECTOR( Tr2Effect );

#endif
