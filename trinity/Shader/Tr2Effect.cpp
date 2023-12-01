#include "StdAfx.h"

#include "ccpparser.h"

#include "Tr2Effect.h"
#include "Tr2Shader.h"
#include "Tr2Renderer.h"
#include "Shader/Parameter/Tr2FloatParameter.h"
#include "Shader/Parameter/Tr2Vector2Parameter.h"
#include "Shader/Parameter/Tr2Vector3Parameter.h"
#include "Shader/Parameter/Tr2Vector4Parameter.h"
#include "Shader/Parameter/Tr2Matrix4Parameter.h"
#include "Shader/Parameter/TriFloatArrayParameter.h"
#include "Shader/Parameter/TriTextureParameter.h"
#include "Shader/Parameter/Tr2GeometryBufferParameter.h"
#include "Shader/Parameter/TriVariableParameter.h"
#include "Shader/Parameter/Tr2RuntimeTextureParameter.h"
#include "Shader/Parameter/Tr2Matrix4Parameter.h"

BLUE_DEFINE_INTERFACE( ITr2EffectValue );
BLUE_DEFINE_INTERFACE( ITr2ScreenSizeAwareValue );

#define INVALID_PARAMETER_HASH (~0)

using namespace Tr2RenderContextEnum;

bool GetBool( const Tr2Shader* shaderState, const char* paramName, const char* annotationName, bool defaultValue = false );

static BlueStructureDefinition Tr2EffectParameterStructureDef[] =
{ 
	{ "name", Be::SHAREDSTRING_1, 0 }, 
	{ "value", Be::FLOAT32_4, 8 }, 
	{0} 
};

Be::VarChooser SamplerStateChooser_AddressMode[] =
{
	{
		"Wrap",
		BeCast( Tr2RenderContextEnum::TA_WRAP ),
		"Texture is wrapped"
	},
	{
		"Clamp",
		BeCast( Tr2RenderContextEnum::TA_CLAMP ),
		"Texture is clamped"
	},
	{
		"Mirror",
		BeCast( Tr2RenderContextEnum::TA_MIRROR ),
		"Texture is mirrored"
	},
	{ 0 }
};

Be::VarChooser SamplerStateChooser_FilterMode[] =
{
	{
		"Point",
		BeCast( Tr2RenderContextEnum::TF_POINT ),
		"Texture is not filtered"
	},
	{
		"Linear",
		BeCast( Tr2RenderContextEnum::TF_LINEAR ),
		"Texture is filtered: linear"
	},
	{
		"Anisotropic",
		BeCast( Tr2RenderContextEnum::TF_ANISOTROPIC ),
		"Texture is filtered: anisotropic"
	},
	{ 0 }
};

Be::VarChooser SamplerStateChooser_MipFilterMode[] =
{
	{
		"None",
		BeCast( Tr2RenderContextEnum::TF_NONE ),
		"Disable Mipmaps"
	},
	{
		"Point",
		BeCast( Tr2RenderContextEnum::TF_POINT ),
		"Mipmaps are not filtered"
	},
	{
		"Linear",
		BeCast( Tr2RenderContextEnum::TF_LINEAR ),
		"Mipmaps are filtered: linear"
	},
	{
		"Anisotropic",
		BeCast( Tr2RenderContextEnum::TF_ANISOTROPIC ),
		"Mipmaps are filtered: anisotropic"
	},
	{ 0 }
};

namespace
{

BlueStructureDefinition Tr2SamplerOverrideStructureDef[] =
{
	{ "name", Be::SHAREDSTRING_1, offsetof( Tr2SamplerOverride, name ) },
	{ "addressU", Be::UINT32_1, offsetof( Tr2SamplerOverride, addressU ), SamplerStateChooser_AddressMode },
	{ "addressV", Be::UINT32_1, offsetof( Tr2SamplerOverride, addressV ), SamplerStateChooser_AddressMode },
	{ "addressW", Be::UINT32_1, offsetof( Tr2SamplerOverride, addressW ), SamplerStateChooser_AddressMode },
	{ "filter", Be::UINT32_1, offsetof( Tr2SamplerOverride, filter ), SamplerStateChooser_FilterMode },
	{ "mipFilter", Be::UINT32_1, offsetof( Tr2SamplerOverride, mipFilter ), SamplerStateChooser_MipFilterMode },
	{ "lodBias", Be::FLOAT32_1, offsetof( Tr2SamplerOverride, lodBias ) },
	{ "maxMipLevel", Be::UINT32_1, offsetof( Tr2SamplerOverride, maxMipLevel ) },
	{ "maxAnisotropy", Be::UINT32_1, offsetof( Tr2SamplerOverride, maxAnisotropy ) },
	{0}
};

Tr2SamplerOverride s_defaultValue = {
	BlueSharedString(),
	Tr2RenderContextEnum::TA_WRAP,
	Tr2RenderContextEnum::TA_WRAP,
	Tr2RenderContextEnum::TA_WRAP,
	Tr2RenderContextEnum::TF_LINEAR,
	Tr2RenderContextEnum::TF_LINEAR,
	0.0f,
	0,
	4
};


BlueStructureDefinition Tr2ShaderOptionStructureDef[] =
{
	{ "name", Be::SHAREDSTRING_1, offsetof( Tr2ShaderOption, name ) },
	{ "value", Be::SHAREDSTRING_1, offsetof( Tr2ShaderOption, value ) },
	{0}
};


// --------------------------------------------------------------------------------------
// Description:
//   Look at the constant and build an ITriEffectParameter out of it
// Arguments:
//   constant - constant being analyzed
//   constantValues - blob with the default value for the constant
//   adder - callback function that's in charge of actually adding the newly created parameter
// --------------------------------------------------------------------------------------
void ConvertEffectConstant( const Tr2EffectConstant& constant,
	const char* constantValues,
	std::function<void( ITriEffectParameter* )> adder )
{
	if( constant.type != Tr2EffectConstant::FLOAT )
	{
		return;
	}
	if( constant.elements > 1 )
	{
		// create new paramater
		OTriFloatArrayParameter* newFloatArray = new OTriFloatArrayParameter();  // Creates object with 1 lock
		newFloatArray->m_name = BlueSharedString( constant.name );
		// create an instance for each vector4
		for( unsigned int e = 0; e < constant.elements; ++e )
		{
			TriVector4Ptr newVector4;
			newVector4.CreateInstance();
			newVector4->m_data = *reinterpret_cast<const Vector4*>( constantValues + constant.offset + e * sizeof( Vector4 ) );
			newFloatArray->m_value.Insert( -1, newVector4 );
		}
		adder( newFloatArray );
		newFloatArray->Unlock();
	}
	else if( constant.dimension == 16 )
	{
		if( strstr( constant.name.c_str(), "Reflection" ) )
		{
			OTriVariableParameter* p = new OTriVariableParameter();
			p->m_name = BlueSharedString( constant.name );
			p->m_variableName = BlueSharedString( "EnvMapTransform" );
			p->Initialize();
			adder( p );
			p->Unlock(); // Remove the original lock created by 'new'.
		}
		else
		{
			OTr2Matrix4Parameter* p = new OTr2Matrix4Parameter();
			p->m_name = BlueSharedString( constant.name );
			p->m_value = *reinterpret_cast<const Matrix*>( constantValues + constant.offset );
			adder( p );
			p->Unlock();
		}
	}
	else if( constant.dimension == 2 )
	{
		OTr2Vector2Parameter* newVector2 = new OTr2Vector2Parameter();  // Creates object with 1 lock
		newVector2->m_name = BlueSharedString( constant.name );
		newVector2->m_value = *reinterpret_cast<const Vector2*>( constantValues + constant.offset );
		adder( newVector2 );
		newVector2->Unlock();
	}
	else if( constant.dimension == 3 )
	{
		OTr2Vector3Parameter* newVector3 = new OTr2Vector3Parameter();  // Creates object with 1 lock
		newVector3->m_name = BlueSharedString( constant.name );
		newVector3->m_value = *reinterpret_cast<const Vector3*>( constantValues + constant.offset );
		adder( newVector3 );
		newVector3->Unlock();
	}
	else if( constant.dimension > 1 )
	{
		OTr2Vector4Parameter* newVector4 = new OTr2Vector4Parameter();  // Creates object with 1 lock
		newVector4->m_name = BlueSharedString( constant.name );
		newVector4->m_value = *reinterpret_cast<const Vector4*>( constantValues + constant.offset );
		adder( newVector4 );
		newVector4->Unlock();
	}
	else
	{
		OTr2FloatParameter* newFloat = new OTr2FloatParameter();
		newFloat->m_name = BlueSharedString( constant.name );
		newFloat->m_value = *reinterpret_cast<const float*>( static_cast<const void*>( constantValues + constant.offset ) );
		adder( newFloat );
		newFloat->Unlock(); // Remove the original lock created by 'new'.
	}
}

// --------------------------------------------------------------------------------------
// Description:
//   Look at the constant and build an ITriEffectParameter and/or ITriEffectResourceParameter out of it
// Arguments:
//   texture - texture being analyzed
//   paramAdder - callback function that's in charge of actually adding any newly created parameters
//   resourceAdder - callback function that's in charge of actually adding any newly created resources
// --------------------------------------------------------------------------------------
void ConvertEffectResource( const Tr2EffectResource& resource,
	std::function<void( ITriEffectParameter* )> paramAdder,
	std::function<void( ITriEffectResourceParameter* )> resourceAdder )
{
	switch( resource.type )
	{
	case Tr2EffectResource::TEXTURE_CUBE:
	case Tr2EffectResource::TEXTURE_1D:
	case Tr2EffectResource::TEXTURE_2D:
	case Tr2EffectResource::TEXTURE_3D:
	case Tr2EffectResource::TEXTURE_TYPELESS:
	{
		OTriTextureParameter* newTex2D = new OTriTextureParameter();
		newTex2D->SetParameterName( BlueSharedString( resource.name ) );
		resourceAdder( newTex2D );
		newTex2D->Unlock(); // Remove the original lock created by 'new'.
	}
	break;
	default:
	{
		OTr2GeometryBufferParameter* newBuffer = new OTr2GeometryBufferParameter();
		newBuffer->m_name = BlueSharedString( resource.name );
		resourceAdder( newBuffer );
		newBuffer->Unlock(); // Remove the original lock created by 'new'.
	}
	break;
	}
}

}

// ---------------------------------------------------------------
Tr2Effect::Tr2Effect(IRoot* lockobj) :
	#if TRINITYDEV
		m_insideBegin( false ),
		m_insideBeginPass( false ),
	#endif
	m_insideStartUpdate( false ),
	PARENTLOCK(m_parameters),
	PARENTLOCK(m_resources),
	PARENTLOCK( m_constParameters ),
	PARENTLOCK( m_samplerOverrides ),
	PARENTLOCK( m_options ),
	m_display( true ),
	m_parameterHash( INVALID_PARAMETER_HASH )
{
	m_parameters.SetNotify( this );
	m_resources.SetNotify( this );
	m_constParameters.SetStructureDefinition( Tr2EffectParameterStructureDef );
	m_samplerOverrides.SetStructureDefinition( Tr2SamplerOverrideStructureDef );
	m_samplerOverrides.SetDefaultValue( &s_defaultValue );
	m_options.SetStructureDefinition( Tr2ShaderOptionStructureDef );

	Tr2Renderer::RegisterEffect( this );
}

// ---------------------------------------------------------------
Tr2Effect::~Tr2Effect()
{
	Tr2Renderer::UnregisterEffect( this );

	for( auto it = begin( m_resources ); it != end( m_resources ); ++it )
	{
		( *it )->OnRemovedFromMaterial( this );
	}

	if( m_effectResource )
	{
		ReleaseCachedData( m_effectResource );
		m_effectResource->RemoveNotifyTarget( this );
	}
}

void Tr2Effect::ReleaseResources( TriStorage s )
{
	if( ( s & TRISTORAGE_ALL ) == TRISTORAGE_ALL )
	{
		for( auto it = m_parametersForPasses.begin(); it != m_parametersForPasses.end(); ++it )
		{
			for( auto jt = it->begin(); jt != it->end(); ++jt )
			{
				for( unsigned i = 0; i != SHADER_TYPE_COUNT; ++i )
				{
					( *jt )->m_stageInput[i].m_constantBuffer = Tr2ConstantBufferAL();
				}
			}
		}
	}
}

bool Tr2Effect::OnPrepareResources()
{
	return true;
}

static bool ConvertEffectPath( const std::string& path, std::string& actualPath )
{
	CCP_STATS_ZONE( __FUNCTION__ );

	actualPath = std::string( path.size() + 8 + 10, 0 );
	const char* str = actualPath.c_str();
	size_t i = 0;
	size_t dot = -1;
	bool foundEffect = false;
	for( auto it = std::begin( path ); it != std::end( path ); ++it )
	{
		char c = actualPath[i] = tolower( *it );
		if( c == '/' )
		{
			if( i > 8 && strcmp( str + i - 7, "/effect/" ) == 0 )
			{
				--i;
				static const char* s_platformName = "."  TRINITY_PLATFORM_NAME "/";
				for( const char* src = s_platformName; *src; ++src )
				{
					actualPath[++i] = *src;
				}
				foundEffect = true;
			}
			dot = -1;
		}
		else if( c == '.' )
		{
			dot = i;
		}
		++i;
	}
	if( !foundEffect )
	{
		CCP_LOGERR( "Effect '%s' needs to be under /effect/", path.c_str() );
		return false;
	}
	if( dot == -1 )
	{
		CCP_LOGWARN( "Invalid effect path '%s', no extension", path.c_str() );
		dot = i;
	}
	actualPath[dot++] = '.';
	const char* sm;
	switch( Tr2Renderer::GetShaderModel() )
	{
	case TR2SM_3_0_DEPTH:
		sm = "sm_depth";
		break;
	case TR2SM_3_0_HI:
		sm = "sm_hi";
		break;
	default:
		sm = "sm_lo";
	}
	while( *sm )
	{
		actualPath[dot++] = *sm++;
	}
	return true;
}

// ---------------------------------------------------------------
bool Tr2Effect::Initialize()
{
	CCP_STATS_ZONE( __FUNCTION__ );

	for( auto it = begin( m_resources ); it != end( m_resources ); ++it )
	{
		( *it )->OnAddedToMaterial( this );
	}

	if( m_effectResource )
	{
		ReleaseCachedData( m_effectResource );
		m_effectResource->RemoveNotifyTarget( this );
		m_effectResource.Unlock();
	}

	if( m_effectFilePath.size() > 0 )
	{
		if( !ConvertEffectPath( m_effectFilePath, m_actualEffectFilePath ) )
		{
			m_actualEffectFilePath = "";
			return true;
		}
		BeResMan->GetResource( m_actualEffectFilePath.c_str(), "", BlueInterfaceIID<Tr2EffectRes>(), (void**)&m_effectResource );

		if( m_effectResource )
		{
			m_effectResource->AddNotifyTarget( this );
		}
	}
	else
	{
		m_actualEffectFilePath = "";
	}

	return true;
}

// ---------------------------------------------------------------
void Tr2Effect::SetEffectPathName( const char* path )
{
	m_effectFilePath = path ? path : "";
	Initialize();
}

// --------------------------------------------------------------------------------
// Description:
//   Manually adding a texture 2d resource to this effect's list with creating
//   it. This can fail if the texture name already exists
// --------------------------------------------------------------------------------
bool Tr2Effect::AddResourceTexture2D( const BlueSharedString& name, const char* resPath )
{
	// check if have that name already!
	if( GetResourceByName( name.c_str() ) )
	{
		return false;
	}
	// alloc and init the texture parameter
	TriTextureParameterPtr texture2d;
	texture2d.CreateInstance();
	texture2d->SetParameterName( name );
	texture2d->SetResourcePath( resPath );
	// add it to this effect's resources
	m_resources.Append( texture2d->GetRawRoot() );
	return true;
}

// --------------------------------------------------------------------------------
bool Tr2Effect::AddResource( ITriEffectParameter* param )
{
	m_resources.Append( param );
	return true;
}

// --------------------------------------------------------------------------------
// Description:
//   Manually adding a sampler override to change texture lookups
// --------------------------------------------------------------------------------
bool Tr2Effect::AddSamplerOverride( const BlueSharedString& name, Tr2RenderContextEnum::TextureAddressMode addressModeU, Tr2RenderContextEnum::TextureAddressMode addressModeV )
{
	// check if we have that name already!
	if( HasSamplerOverride( name.c_str() ) )
	{
		return false;
	}

	// set up the const stuff
	Tr2SamplerOverride o;
	o.name = name;
	o.mipFilter = Tr2RenderContextEnum::TF_LINEAR;
	o.maxAnisotropy = 4;
	o.maxMipLevel = 0;
	o.lodBias = 0.f;
	o.filter = Tr2RenderContextEnum::TF_ANISOTROPIC;
	o.addressW = Tr2RenderContextEnum::TA_WRAP;

	// setup the non-const stuff
	o.addressU = addressModeU;
	o.addressV = addressModeV;

	m_samplerOverrides.Append( &o );

	return true;
}

// --------------------------------------------------------------------------------
// Description:
//   Manually adding a vector4 parameter to this effect's list with creating
//   it
// --------------------------------------------------------------------------------
bool Tr2Effect::AddParameterVector4( const BlueSharedString& name, const Vector4* value )
{
	// check if we have that name already!
	if( HasParameter( name.c_str() ) )
	{
		return false;
	}

	Tr2ConstantEffectParameter param;
	param.name = name;
	param.value = *value;
	m_constParameters.Append( &param );
	return true;
}

// --------------------------------------------------------------------------------
// Description:
//   Manually adding a float parameter to this effect's list with creating
//   it
// --------------------------------------------------------------------------------
bool Tr2Effect::AddParameterFloat( const BlueSharedString& name, float value )
{
	// turn float vlaue into a vector4, cause that's what we put into constant parameters
	Vector4 vec4( value, value, value, value );
	return AddParameterVector4( name, &vec4 );
}

// --------------------------------------------------------------------------------
// Description:
//   Manually adding a color parameter to this effect's list with creating
//   it
// --------------------------------------------------------------------------------
bool Tr2Effect::AddParameterColor( const BlueSharedString& name, const Color* value )
{
	// check if we have that name already!
	if( HasParameter( name.c_str() ) )
	{
		return false;
	}

	Tr2ConstantEffectParameter param;
	param.name = BlueSharedString( name );
	param.value = *reinterpret_cast<const Vector4*>( value );
	m_constParameters.Append( &param );
	return true;
}

// --------------------------------------------------------------------------------
void Tr2Effect::ClearAllParameters()
{
	// clear out all const and non-const parameters
	StartUpdate();
	m_constParameters.Clear();
	m_parameters.Clear();
	EndUpdate();
}

// --------------------------------------------------------------------------------
void Tr2Effect::ClearAllResources()
{
	m_resources.Clear();
}

// --------------------------------------------------------------------------------
void Tr2Effect::SetOption( const BlueSharedString& name, const BlueSharedString& value )
{
	for( auto it = m_options.begin(); it != m_options.end(); ++it )
	{
		if( it->name == name )
		{
			if( !(it->value == value ) )
			{
				it->value = value;
				RebuildCachedDataInternal();
			}
			return;
		}
	}
	Tr2ShaderOption option;
	option.name = name;
	option.value = value;

	m_options.Append( &option );
	RebuildCachedDataInternal();
}

void Tr2Effect::ResetOption( const BlueSharedString& name )
{
	for( size_t i = 0; i < m_options.size(); ++i )
	{
		if( m_options[i].name == name )
		{
			m_options.Remove( i );
			RebuildCachedDataInternal();
			return;
		}
	}
}

BlueSharedString Tr2Effect::GetOption( const BlueSharedString& name ) const
{
	for( auto it = m_options.begin(); it != m_options.end(); ++it )
	{
		if( it->name == name )
		{
			return it->value;
		}
	}
	return BlueSharedString();
}

static Tr2SamplerDescription CreateSamplerDescription( const Tr2SamplerOverride& samplerOverride )
{
	Color color( 0.f, 0.f, 0.f, 0.f );
	return Tr2SamplerDescription(
		samplerOverride.filter,
		samplerOverride.filter,
		samplerOverride.mipFilter,
		false,
		samplerOverride.addressU,
		samplerOverride.addressV,
		samplerOverride.addressW,
		samplerOverride.lodBias,
		samplerOverride.maxAnisotropy,
		Tr2RenderContextEnum::CMP_NEVER,
		&color.r,
		float( samplerOverride.maxMipLevel ),
		FLT_MAX
		);
}

void Tr2Effect::RebuildSamplerOverrides()
{
	if( m_samplerOverrides.empty() )
	{
		return;
	}

	USE_MAIN_THREAD_RENDER_CONTEXT();

	auto& desc = m_shader->GetEffectDescription();
	for( size_t technique = 0; technique < desc.techniques.size(); ++technique )
	{
		const unsigned passCount = unsigned( desc.techniques[technique].passes.size() );
		for( unsigned passIx = 0; passIx != passCount; ++passIx )
		{
			Tr2EffectPassParameters& pp = *m_parametersForPasses[technique][passIx];

			for( unsigned i = 0; i != Tr2RenderContextEnum::SHADER_TYPE_COUNT; ++i )
			{
				auto& stage = desc.techniques[technique].passes[passIx].stageInputs[i];
				if( !stage.m_exists )
				{
					continue;
				}

				for( auto jt = m_samplerOverrides.begin(); jt != m_samplerOverrides.end(); ++jt )
				{
					for( auto it = stage.samplers.begin(); it != stage.samplers.end(); ++it )
					{
						if( it->second.name && strcmp( jt->name.c_str(), it->second.name ) == 0 )
						{
							Tr2SamplerOverrideData d;
							d.sampler.Create( CreateSamplerDescription( *jt ), renderContext );
							d.registerIndex = it->first;

							pp.m_stageInput[i].m_samplers.push_back( d );
							pp.m_resourceSetDesc.SetSampler( ShaderType( i ), it->first, d.sampler );
							break;
						}
					}
				}
			}
		}
	}
}

// ---------------------------------------------------------------
void Tr2Effect::RebuildCachedDataInternal()
{
	if( m_insideStartUpdate )
	{
		return;
	}
	m_parameterHash = INVALID_PARAMETER_HASH;
	auto bk = m_shader;
	m_shader = nullptr;
	m_lodTextureParameters.clear();

	if( m_effectResource )
	{		
		m_shader = m_effectResource->GetShader( m_options.empty() ? nullptr : &m_options[0], m_options.size() );
		if( m_shader )
		{
			CCP_STATS_ZONE( __FUNCTION__ );
			USE_MAIN_THREAD_RENDER_CONTEXT();

			m_parametersForPasses.clear();

			auto& desc = m_shader->GetEffectDescription();

			m_parametersForPasses.resize( desc.techniques.size() );

			for( size_t technique = 0; technique < desc.techniques.size(); ++technique )
			{

				const unsigned passCount = unsigned( desc.techniques[technique].passes.size() );

				m_parametersForPasses[technique].resize( passCount );

				for( unsigned passIx = 0; passIx != passCount; ++passIx )
				{
					m_parametersForPasses[technique][passIx].reset( CCP_NEW( "Tr2EffectPassParameters" ) Tr2EffectPassParameters() );
					Tr2EffectPassParameters& pp = *m_parametersForPasses[technique][passIx];
					pp.m_resourceSetDesc = desc.techniques[technique].passes[passIx].resourceSetDesc;
					pp.m_resourceSetHash = 0;
					pp.m_resourceSetDirty = true;

					for( unsigned i = 0; i != Tr2RenderContextEnum::SHADER_TYPE_COUNT; ++i )
					{
						auto& stage = desc.techniques[technique].passes[passIx].stageInputs[i];
						if( !stage.m_exists )
						{
							continue;
						}

						MapPassParameters( technique, passIx, pp, Tr2RenderContextEnum::ShaderType( i ), stage.constants, desc, renderContext );

						auto& input = pp.m_stageInput[i];
						if( !stage.resources.empty() )
						{
							MapPassResources( stage.resources, input.m_textures );
						}
						if( !stage.uavs.empty() )
						{
							MapPassResources( stage.uavs, input.m_uavs );
						}
					}
				}
			}

			RebuildSamplerOverrides();
		}
		else
		{
			m_parametersForPasses.clear();
		}
	}
	else
	{
		m_parametersForPasses.clear();
	}

	// It's ok to pass in NULL values to these functions so that the parameters
	// realize that they're not in use by a NULL technique etc

	for( size_t i = 0; i != m_parameters.size(); ++i )
	{
		m_parameters[i]->RebuildEffectHandles( m_shader );
	}

	for( size_t i = 0; i != m_resources.size(); ++i)
	{
		m_resources[i]->RebuildEffectHandles( m_shader );
	}

}

void Tr2Effect::RebuildCachedData( BlueAsyncRes* p )
{
	CCP_STATS_ZONE( __FUNCTION__ );

	CCP_ASSERT( p == m_effectResource );
	if( p == m_effectResource )
	{
		auto bk = m_insideStartUpdate;
		m_insideStartUpdate = false;
		RebuildCachedDataInternal();
		m_insideStartUpdate = bk;
	}
}

void Tr2Effect::RebuildCachedData()
{
	RebuildCachedDataInternal();
}

void Tr2Effect::ReleaseCachedData( BlueAsyncRes* p )
{
	CCP_STATS_ZONE( __FUNCTION__ );
	CCP_ASSERT( p == m_effectResource );
	if( p == m_effectResource )
	{
		m_shader = nullptr;

		const int numResources = (unsigned int)m_resources.size();
		const int numParameters = (unsigned int)m_parameters.size();

		// It's ok to pass in NULL values to these functions so that the parameters
		// realize that they're not in use by a NULL technique etc.
		// It's important to notify parameters of the fact that the effect
		// has been reset
		for (int i = 0; i < numParameters; i++)
		{
			m_parameters[i]->RebuildEffectHandles( nullptr );
		}

		for (int i = 0; i < numResources; i++)
		{
			m_resources[i]->RebuildEffectHandles( nullptr );
		}
		m_lodTextureParameters.clear();
	}
}

// ---------------------------------------------------------------
bool Tr2Effect::OnModified( Be::Var* value )
{
	CCP_STATS_ZONE( __FUNCTION__ );
	// m_effectFilePath is the only attribute with a notify flag

	Initialize();

	// Immediately invalidate the cached information
	RebuildCachedDataInternal();

	return true;
}

// ---------------------------------------------------------------
void Tr2Effect::StartUpdate()
{
	m_insideStartUpdate = true;
}

// ---------------------------------------------------------------
void Tr2Effect::EndUpdate()
{
	if( m_insideStartUpdate )
	{
		m_insideStartUpdate = false;
		RebuildCachedDataInternal();
	}
}

// ---------------------------------------------------------------
void Tr2Effect::OnListModified( long event, ssize_t key, ssize_t key2, IRoot* currvalue, const IList* theList )
{
	CCP_STATS_ZONE( __FUNCTION__ );

	if( (event & BELIST_LOADING) == 0  )
	{
		switch( event & BELIST_EVENTMASK )
		{
			case BELIST_REMOVED:
				if( ITriReroutablePtr rp = BlueCastPtr( currvalue ) )
				{
					rp->SetDestination( NULL, 0 );
				}
				if( ITriEffectResourceParameterPtr res = BlueCastPtr( currvalue ) )
				{
					res->OnRemovedFromMaterial( this );
				}
				RebuildCachedDataInternal();
				break;

			case BELIST_INSERTED:
				if( ITriEffectResourceParameterPtr res = BlueCastPtr( currvalue ) )
				{
					res->OnAddedToMaterial( this );
				}
			case BELIST_SWAPPED:
			case BELIST_MOVED:
				RebuildCachedDataInternal();
				break;
		}
	}
}

// ---------------------------------------------------------------
// PopulateParameters
// 
// Populates the list of parameters on the mesh using the effect file
// Visibility annotations default to false, so you need to specifically
// expose stuff.
// ---------------------------------------------------------------
bool Tr2Effect::PopulateParameters()
{
	CCP_STATS_ZONE( __FUNCTION__ );

	if (!m_shader)
	{
		CCP_LOGERR( "Tr2Effect::PopulateParameters: no effect resource loaded." );
		return false;
	}

	auto paramAdder = [&]( ITriEffectParameter* p )
						{
							m_parameters.Insert( -1, p );
						};

	auto resourceAdder = [&]( ITriEffectResourceParameter* p )
						{
							m_resources.Insert( -1, p );
						};

	auto hasParameter = [&]( const char* name ) -> bool
	{
		if( GetParameterByName( name ) )
		{
			return true;
		}
		for( auto it = m_constParameters.begin(); it != m_constParameters.end(); ++it )
		{
			if( strcmp( it->name.c_str(), name ) == 0 )
			{
				return true;
			}
		}
		return false;
	};

	auto& desc = m_shader->GetEffectDescription();
	for( auto technique = desc.techniques.begin(); technique != desc.techniques.end(); ++technique )
	{
		for( auto pass = technique->passes.begin(); pass != technique->passes.end(); ++pass )
		{
			for( unsigned i = 0; i != Tr2RenderContextEnum::SHADER_TYPE_COUNT; ++i )
			{
				const auto& input = pass->stageInputs[i];

				for( auto constant = input.constants.cbegin(); constant != input.constants.cend(); ++constant )
				{
					if( !GetBool( m_shader, constant->name.c_str(), "SasUiVisible" ) )
					{
						continue;
					}

					if( hasParameter( constant->name.c_str() ) )
					{
						continue;
					}

					ConvertEffectConstant( *constant, input.constantValues, paramAdder );
				}

				for( auto sampler = input.resources.begin(); sampler != input.resources.end(); ++sampler )
				{
					if( !GetBool( m_shader, sampler->second.name, "SasUiVisible" ) )
					{
						continue;
					}

					if( hasParameter( sampler->second.name ) )
					{
						continue;
					}

					ConvertEffectResource( sampler->second, paramAdder, resourceAdder );
				}

				for( auto uav = input.uavs.begin(); uav != input.uavs.end(); ++uav )
				{
					if( !GetBool( m_shader, uav->second.name, "SasUiVisible" ) )
					{
						continue;
					}

					if( hasParameter( uav->second.name ) )
					{
						continue;
					}

					ConvertEffectResource( uav->second, paramAdder, resourceAdder );
				}
			}
		}
	}

	RebuildCachedDataInternal();
	return true;
}


// ---------------------------------------------------------------
// PruneParameters
// 
// Removes any parameters from the object that do not exist in the
// effect file
// ---------------------------------------------------------------
bool Tr2Effect::PruneParameters()
{
	CCP_STATS_ZONE( __FUNCTION__ );

	if (!m_shader)
	{
		CCP_LOGERR( "Tr2Effect::PruneParameters: no effect resource loaded." );
		return false;
	}

	// Prune Effects Parameters
	int y = 0;
	while (y < m_parameters.GetSize())
	{
		std::string pName = m_parameters[y]->GetParameterName();
		bool removeParameter = !GetBool( m_shader, pName.c_str(), "SasUiVisible" );
		const Tr2EffectConstant *constant = m_shader->GetConstant( pName.c_str() );
		if( constant == nullptr && !removeParameter )
		{
			removeParameter = true;
			auto& desc = m_shader->GetEffectDescription();
			for( auto technique = desc.techniques.begin(); technique != desc.techniques.end(); ++technique )
			{
				for( auto pass = technique->passes.begin(); pass != technique->passes.end(); ++pass )
				{
					for( unsigned i = 0; i < Tr2RenderContextEnum::SHADER_TYPE_COUNT; ++i )
					{
						for( auto sampler = pass->stageInputs[i].resources.begin(); sampler != pass->stageInputs[i].resources.end(); ++sampler )
						{
							if( strcmp( sampler->second.name, pName.c_str() ) == 0 )
							{
								removeParameter = false;
								break;
							}
						}
					}
				}
			}
		}
		if (removeParameter)
		{
			m_parameters.Remove(y);
		}
		else
		{
			++y;
		}
	}

	// Prune const parameters
	size_t it = 0;
	while( it < m_constParameters.size() )
	{
		const char* pName = m_constParameters[it].name.c_str();
		bool removeParameter = !GetBool( m_shader, pName, "SasUiVisible" );
		const Tr2EffectConstant *constant = m_shader->GetConstant( pName );
		if( constant == nullptr && !removeParameter )
		{
			removeParameter = true;
		}
		if (removeParameter)
		{
			m_constParameters.Remove( it );
		}
		else
		{
			++it;
		}
	}

	// Prune Effects Resources
	y = 0;
	while (y < m_resources.GetSize())
	{
		std::string pName = m_resources[y]->GetParameterName();
		bool removeParameter = !GetBool( m_shader, pName.c_str(), "SasUiVisible" );
		const Tr2EffectConstant *constant = m_shader->GetConstant( pName.c_str() );
		if( constant == nullptr && !removeParameter )
		{
			removeParameter = true;
			auto& desc = m_shader->GetEffectDescription();
			for( auto technique = desc.techniques.begin(); technique != desc.techniques.end(); ++technique )
			{
				for( auto pass = technique->passes.begin(); pass != technique->passes.end(); ++pass )
				{
					for( unsigned i = 0; i < Tr2RenderContextEnum::SHADER_TYPE_COUNT; ++i )
					{
						for( auto sampler = pass->stageInputs[i].resources.begin(); sampler != pass->stageInputs[i].resources.end(); ++sampler )
						{
							if( strcmp( sampler->second.name, pName.c_str() ) == 0 )
							{
								removeParameter = false;
								break;
							}
						}
					}
				}
			}
		}
		if (removeParameter)
		{
			m_resources.Remove(y);
		}
		else
		{
			++y;
		}
	}

	return true;
}

bool Tr2Effect::IsParameterUsedByTechnique( const std::string& parameterName )
{
	if (!GetShaderStateInterface())
	{
		CCP_LOGERR( "Tr2Effect::IsParameterUsedByTechnique: no effect resource loaded." );
		return false;
	}

	return GetShaderStateInterface()->GetConstant( parameterName.c_str() ) != nullptr;
}

Tr2EffectRes* Tr2Effect::GetEffectRes() const
{
	return m_effectResource;
}

// --------------------------------------------------------------------------------------
// Description:
//   Just return the name
// --------------------------------------------------------------------------------------
const char* Tr2Effect::GetName() const
{
	return m_name.c_str();
}

// --------------------------------------------------------------------------------------
// Description:
//   Simple getter for the shader file name
// --------------------------------------------------------------------------------------
const char* Tr2Effect::GetEffectPathName() const
{
	return m_effectFilePath.c_str();
}

ITriEffectParameter* Tr2Effect::GetParameterByName( const char* name ) const
{
	CCP_STATS_ZONE( __FUNCTION__ );

	ITriEffectParameter* result = FindParameterByName( name );

	if ( !result )
	{
		result = GetResourceByName( name );
	}

	return result;
}

// --------------------------------------------------------------------------------------
// Description:
//   Implements ITr2ShaderMaterial interface. Returns an array of constant effect 
//   parameters.
// Arguments:
//   count - (out) Number of constant parameters
// Return Value:
//   Pointer to the first element in constant parameter array
// --------------------------------------------------------------------------------------
const Tr2ConstantEffectParameter* Tr2Effect::GetConstParameters( size_t& count ) const
{
	count = m_constParameters.size();
	if( count )
	{
		return &m_constParameters[0];
	}
	else
	{
		return nullptr;
	}
}

namespace
{

struct VariableFactoryArguments
{
	const Tr2ConstantEffectParameterStructureList* constParams;
	bool hasInvalidVariable;
	std::vector<std::unique_ptr<CcpParser::Variable>> variables;
};

const CcpParser::Variable* GetParserVariable( void* ctx, const CcpParser::StringView& name )
{
	auto arguments = static_cast<VariableFactoryArguments*>( ctx );
	for( auto& param : *arguments->constParams )
	{
		auto len = strlen( param.name.c_str() );
		auto nlen = name.end - name.begin;
		if( ( nlen == len || nlen == len + 2 ) && strncmp( param.name.c_str(), name.begin, len ) == 0 )
		{
			size_t swizzle = 0;
			if( nlen == len + 2 && name.begin[len] == '_' )
			{
				switch( name.begin[len + 1] )
				{
				case 'x':
				case 'r':
					swizzle = 0;
					break;
				case 'y':
				case 'g':
					swizzle = 1;
					break;
				case 'z':
				case 'b':
					swizzle = 2;
					break;
				case 'w':
				case 'a':
					swizzle = 3;
					break;
				default:
					continue;
				}
			}
			else if( nlen != len )
			{
				continue;
			}
			std::unique_ptr<CcpParser::Variable> var( new CcpParser::Variable() );
			var->name = param.name.c_str();
			var->buffer = 0;
			var->offset = uint32_t( reinterpret_cast<const uint8_t*>( &param.value ) - reinterpret_cast<const uint8_t*>( &( *arguments->constParams )[0] ) + swizzle * sizeof( float ) );
			auto result = var.get();
			arguments->variables.push_back( std::move( var ) );
			return result;
		}
	}
	arguments->hasInvalidVariable = true;
	return nullptr;
}

std::optional<float> GetUvScaleFromAnnotation( const char* paramName, const Tr2EffectParameterAnnotation& annotation, const Tr2ConstantEffectParameterStructureList& constParams )
{
	if( annotation.type == Tr2EffectParameterAnnotation::FLOAT )
	{
		return { annotation.floatValue };
	}
	else if( annotation.type == Tr2EffectParameterAnnotation::STRING )
	{
		std::string expr( annotation.stringValue );
		for( size_t i = 0; i < expr.length(); ++i )
		{
			auto& c = expr[i];
			if( c == '.' )
			{
				bool isId = false;
				for( size_t j = i; j > 0; --j)
				{
					auto pc = expr[j - 1];
					if( isdigit( pc ) )
					{
						continue;
					}
					if( isalpha( pc ) )
					{
						isId = true;
						break;
					}
					break;
				}
				if( isId )
				{
					c = '_';
				}
			}
		}

		VariableFactoryArguments factoryArgs = { &constParams, false };
		CcpParser::Externals externals;
		externals.variableFactory = { &GetParserVariable, &factoryArgs };
		CcpParser::Program program;
		auto parsed = CcpParser::Parse( expr.c_str(), externals, program );
		if( !parsed )
		{
			if( !factoryArgs.hasInvalidVariable )
			{
				CCP_LOGWARN( "Invalid LodUvScale annotation for effect parameter \"%s\": %s", paramName, ToString( parsed, expr.c_str() ).c_str() );
			}
			return std::nullopt;
		}
		void* buffers[] = { (void*)&constParams[0] };
		std::unique_ptr<uint8_t[]> tempArena( new uint8_t[program.GetTempArenaSize()] );
		return program.Eval( buffers, tempArena.get() );
	}
	return std::nullopt;
}

bool ExtractLodingAnnotations( std::array<float, ITriEffectTextureParameter::UV_SET_MAX_COUNT>& densityScale, const char* name, const Tr2EffectAnnotationMap& effectAnnotations, const Tr2ConstantEffectParameterStructureList& constParams )
{
	std::fill( begin( densityScale ), end( densityScale ), 0.f );

	auto annotations = effectAnnotations.find( name );
	if( annotations == end( effectAnnotations ) )
	{
		return false;
	}
	bool enabled = false;
	uint32_t uvSet = 0;
	float scale = 1;
	const char* prefix = "LodUvScale";
	auto length = strlen( prefix );
	for( auto& annotation : annotations->second )
	{
		if( strncmp( annotation.name, prefix, length ) == 0 )
		{
			char* strEnd;
			auto uvIndex = strtol( annotation.name + length, &strEnd, 10 );
			if( *strEnd != 0 || uvIndex < 0 || uvIndex + 1 >= ITriEffectTextureParameter::UV_SET_MAX_COUNT )
			{
				continue;
			}
			
			if( auto paramScale = GetUvScaleFromAnnotation( name, annotation, constParams ) )
			{
				densityScale[uvIndex + 1] = *paramScale;
				if( *paramScale > 0 )
				{
					enabled = true;
				}
			}
			else
			{
				return false;
			}
		}
		else if( strcmp( annotation.name, "LodPositionScale" ) == 0 )
		{
			if( auto paramScale = GetUvScaleFromAnnotation( name, annotation, constParams ) )
			{
				densityScale[0] = *paramScale;
				if( *paramScale > 0 )
				{
					enabled = true;
				}
			}
			else
			{
				return false;
			}
		}
	}
	return enabled;
}

}


void Tr2Effect::MapPassResources( const Tr2EffectResourceMap& resources, Tr2EffectParamVector &pv )
{
	CCP_STATS_ZONE( __FUNCTION__ );

	for( auto it = resources.begin(); it != resources.end(); ++it )
	{
		const Tr2EffectResource& ss = it->second;
		const char* name = ss.name;

		Tr2EffectParam param;

		// First search in effect resource list 
		if( ITriEffectParameter* p = GetResourceByName( name ) )
		{
			param.m_sourceValue = p;

			if( ITriEffectTextureParameterPtr loddable = BlueCastPtr( p ) )
			{
				std::array<float, ITriEffectTextureParameter::UV_SET_MAX_COUNT> uvScale;
				if( ExtractLodingAnnotations( uvScale, name, m_shader->GetEffectDescription().annotations, m_constParameters ) )
				{
					auto found = find( begin( m_lodTextureParameters ), end( m_lodTextureParameters ), loddable );
					if( found == end( m_lodTextureParameters ) )
					{
						loddable->EnableTextureLoding( uvScale );
						m_lodTextureParameters.push_back( loddable );
					}
				}
				else
				{
					auto found = find( begin( m_lodTextureParameters ), end( m_lodTextureParameters ), loddable );
					if( found == end( m_lodTextureParameters ) )
					{
						loddable->DisableTextureLoding();
						m_lodTextureParameters.push_back( loddable );
					}					
				}
			}
		}
		// Secondly search in effect parameter list
		else if( ITriEffectParameter* p = FindParameterByName( name ) )
		{
			// The only parameter that can point to a resource is the TriVariableParameter
			TriVariableParameter* vp = dynamic_cast<TriVariableParameter*>(p);
			if( vp 
				&& vp->m_variable
				&& ( vp->m_variable->GetType() == TRIVARIABLE_TEXTURE_RES || 
					 vp->m_variable->GetType() == TRIVARIABLE_GPUBUFFER ) )
			{
				param.m_sourceValue = p;
			}

		}
		// Fallback to variable store
		else if( TriVariable* v = GetVariableStore().FindVariable( name ) )
		{
			if( v->GetType() == TRIVARIABLE_TEXTURE_RES || 
				v->GetType() == TRIVARIABLE_GPUBUFFER )
			{
				param.m_sourceValue = v;
			}
		}
		else if( ss.isAutoregister )
		{
			param.m_sourceValue = GetVariableStore().GetVariable( name );
		}
		
		if( param.m_sourceValue )
		{
			param.m_sourceName = ss.name;
			param.m_registerIndex = it->first;
			param.m_registerCount = it->second.isSRGB ? ITr2EffectValue::RESOURCE_FLAG_SRGB : 0;

			pv.push_back( param );
		}
	}
}

void Tr2Effect::Render( IRenderCallback* cb, Tr2RenderContext& renderContext )
{
	CCP_STATS_ZONE( __FUNCTION__ );

	auto effectResource = GetShaderStateInterface();

	if( !effectResource )
	{
		return;
	}

	CCP_ASSERT( cb );

	unsigned int passCount = effectResource->GetPassCount( 0 );

	for( uint32_t passIx = 0; passIx < passCount; ++passIx )
	{
		effectResource->ApplyAllStateForPass( 0, passIx, renderContext );
		ApplyMaterialDataForPass( 0, passIx, renderContext );
		cb->SubmitGeometry( renderContext );
	}

}

// --------------------------------------------------------------------------------------
// Description:
//   This method computes a hash value for the effect based on its path name and 
//   parameters. It is not without faults: the method doen't check which parameters are
//   actually used by the effect, doesn't do anything with values coming from a variable
//   store. The method is not light-weight and should not be called every frame.
// Return Value:
//   Hash value for the effect
// --------------------------------------------------------------------------------------
unsigned Tr2Effect::GetHashValue() const
{
	unsigned hash = 0;
	if( m_effectResource )
	{
		hash = CcpHashFNV1( m_effectResource->GetPath(), wcslen( m_effectResource->GetPath() ) * sizeof( wchar_t ) );
	}
	for( auto& option : m_options )
	{
		hash = CcpHashFNV1( &option, sizeof( option ), hash );
	}
	for( auto it = m_constParameters.begin(); it != m_constParameters.end(); ++it )
	{
		auto name = it->name.c_str();
		// looks scary, but it's a constant string, so the pointer is unique
		hash = CcpHashFNV1( &name, sizeof( name ), hash );
		hash = CcpHashFNV1( &it->value, sizeof( it->value ), hash );
	}
	for( auto it = m_parameters.begin(); it != m_parameters.end(); ++it )
	{
		hash = ( *it )->GetHashValue( hash );
	}
	for( auto it = m_resources.begin(); it != m_resources.end(); ++it )
	{
		hash = ( *it )->GetHashValue( hash );
	}
	return hash;
}

ITriEffectParameter* Tr2Effect::FindParameterByName( const char* name ) const
{
	CCP_STATS_ZONE( __FUNCTION__ );

	for( auto it = m_parameters.cbegin(); it != m_parameters.cend(); ++it )
	{
		ITriEffectParameter* p = *it;
		const char* candidate = p->GetParameterName();
		if( candidate && strcmp( candidate, name ) == 0 )
		{
			return p;
		}
	}

	return nullptr;
}

ITriEffectParameter* Tr2Effect::GetResourceByName( const char* name ) const
{
	CCP_STATS_ZONE( __FUNCTION__ );

	for( auto it = m_resources.cbegin(); it != m_resources.cend(); ++it )
	{
		ITriEffectParameter* p = *it;
		const char* candidate = p->GetParameterName();
		if( candidate && strcmp( candidate, name ) == 0 )
		{
			return p;
		}
	}
	return nullptr;
}

// --------------------------------------------------------------------------------------
// Description:
//	Run through all the existing sampler overrides and check if we already have it, by it's name
// Return Value:
//   True if already exists on this effect
// -------------------------------------------------------------------------------------
bool Tr2Effect::HasSamplerOverride( const char* name ) const
{
	CCP_STATS_ZONE( __FUNCTION__ );

	for( auto it = m_samplerOverrides.begin(); it != m_samplerOverrides.end(); ++it )
	{
		if( strcmp( it->name.c_str(), name ) == 0 )
		{
			return true;
		}
	}
	return false;
}

// --------------------------------------------------------------------------------------
// Description:
//	Run through all the existing const(!) parameters and check if we already have it, by it's name
// Return Value:
//   True if already exists on this effect
// -------------------------------------------------------------------------------------
bool Tr2Effect::HasParameter( const char* name ) const
{
	CCP_STATS_ZONE( __FUNCTION__ );

	for( auto it = m_constParameters.begin(); it != m_constParameters.end(); ++it )
	{
		if( strcmp( it->name.c_str(), name ) == 0 )
		{
			return true;
		}
	}
	return false;
}



// --------------------------------------------------------------------------------------
// Description:
//	Find an annotation called annotationName in the map, and return its value. If not found, or it's not a
//	bool, or map is nullptr, return defaultValue instead.
// Arguments:
//   map - annotation map, may be nullptr
//   annotationName - name of the annotation, may be nullptr
//	 defaultValue - what to return if not found or the other arguments are nullptr
// Return Value:
//   the value of this annotation, if it can be found and is of type BOOL.
// -------------------------------------------------------------------------------------
bool GetBool( const Tr2EffectParameterAnnotationMap* map, const char* annotationName, bool defaultValue )
{
	CCP_STATS_ZONE( __FUNCTION__ );

	if( !map || !annotationName )
	{
		return defaultValue;
	}

	auto value = std::find_if( map->begin(), map->end(), [&]( const Tr2EffectParameterAnnotation& a ) { return strcmp( a.name, annotationName ) == 0; } );
	if( value != map->end() && value->type == Tr2EffectParameterAnnotation::BOOL )
	{
		return value->boolValue;
	}

	return defaultValue;
}

// --------------------------------------------------------------------------------------
// Description:
//	Find an annotation called annotationName in the annotation map of shaderState, belonging to a
//  parameter called paramName.  Return its value if found and bool.
// Arguments:
//   shaderState - shader state, may be nullptr
//   paramName - name of the parameters of which the annotations need to be scanned, may be nullptr
//   annotationName - name of the annotation, may be nullptr
//	 defaultValue - what to return if not found or the other arguments are nullptr
// Return Value:
//   the value of this annotation on this parameter, if both can be found and the annotation is of type BOOL.
// -------------------------------------------------------------------------------------
bool GetBool( const Tr2Shader* shaderState, const char* paramName, const char* annotationName, bool defaultValue )
{
	CCP_STATS_ZONE( __FUNCTION__ );

	return	( shaderState && paramName )
			? GetBool( shaderState->GetParameterAnnotations( paramName ), annotationName, defaultValue )
			: defaultValue;
}

// --------------------------------------------------------------------------------------
// Description:
//   Maps the parameters for a pass to indices in the constant mirror.
// Arguments:
//   passIx - Pass index
//   pp - Pass parameters
//   stage - Stage type
//   constants - The constant table
//	 resource - effect being mapped
//   owner - shaderState being mapped
//	 renderContext - render context
// --------------------------------------------------------------------------------------
void Tr2Effect::MapPassParameters( 
	size_t technique,
	unsigned passIx,
	Tr2EffectPassParameters& pp,
	Tr2RenderContextEnum::ShaderType stage,
	const Tr2EffectConstantVector& constants, 
	const Tr2EffectDescription& desc,
	Tr2RenderContext& renderContext )
{
	CCP_STATS_ZONE( __FUNCTION__ );

	static const size_t MAX_PARAMS = 64;

	Tr2EffectParamVector &pv = pp.m_stageInput[stage].m_shaderParameters;
	auto& reroutables = pp.m_reroutedParameters;
	Tr2VariableStore& variableStore = GetVariableStore();

	unsigned int perObjectStart = 0xffffffff;

	CCP_ASSERT( constants.size() < MAX_PARAMS );
	ITr2EffectValue* foundParams[MAX_PARAMS];
	unsigned constantSize = 0;
	unsigned index = 0;
	bool hasConstantParams = false;
	bool hasVariableParams = false;

	size_t constParamCount;
	auto constParams = GetConstParameters( constParamCount );
	uint32_t constIndexes[MAX_PARAMS];

	// Fist pass: determine the size of the constant buffer
	for( auto constantIx = constants.begin(); constantIx != constants.end(); ++constantIx )
	{
		constIndexes[index] = -1;
		bool foundConstant = false;
		for( size_t i = 0; i < constParamCount; ++i )
		{
			if( constantIx->name == constParams[i].name )
			{
				constIndexes[index] = uint32_t( i );
				foundConstant = true;
				hasConstantParams = true;
				break;
			}
		}
		ITr2EffectValue* paramAsEffectValue = NULL;
		if( !foundConstant )
		{

			// First search in effect parameter list and see if we have a match
			if( ITriEffectParameter* p = FindParameterByName( constantIx->name.c_str() ) )
			{
				paramAsEffectValue = p;
			}
			// Fallback to variable store
			else if( TriVariable* v = variableStore.FindVariable( constantIx->name.c_str() ) )
			{
				paramAsEffectValue = v;
			}
			else
			{
				if( constantIx->isAutoregister )
				{
					paramAsEffectValue = GlobalStore().GetVariable( constantIx->name.c_str() );
				}
			}
			hasVariableParams |= paramAsEffectValue != nullptr;
			foundConstant = paramAsEffectValue != nullptr;
		}
		foundParams[index++] = paramAsEffectValue;

		if( foundConstant )
		{
			// It's illegal to pass the perObjectStart if the effect has the block
			if( constantIx->offset >= perObjectStart )
			{
				CCP_ASSERT_M( false, "Register is mapped beyond valid range" );
				CCP_LOGERR( "Effect maps parameter '%s' beyond limit (target is c%d, limit is c%d)", 
					constantIx->name.c_str(), constantIx->offset, perObjectStart );

				// We must ignore this parameter!
				constIndexes[index - 1] = -1;
				foundParams[index - 1] = nullptr;
				continue;
			}

			unsigned int constantIndex = constantIx->offset + constantIx->size;
			if( constantIndex > constantSize )
			{
				constantSize = constantIndex;
			}
		}
	}

	unsigned int constantDefaultValueSize = desc.techniques[technique].passes[passIx].stageInputs[stage].m_constantValueSize;
	const void* constantDefaultValues = desc.techniques[technique].passes[passIx].stageInputs[stage].constantValues;

	constantSize = std::max( constantSize, constantDefaultValueSize );

	if( constantSize > 0 && !hasVariableParams )
	{
		std::unique_ptr<uint8_t[]> mirror( new uint8_t[constantSize] );
		if( !mirror )
		{
			return;
		}

		memcpy( mirror.get(), constantDefaultValues, constantDefaultValueSize );
		if( constantSize > constantDefaultValueSize )
		{
			memset( mirror.get() + constantDefaultValueSize, 0, constantSize - constantDefaultValueSize );
		}

		if( hasConstantParams )
		{
			for( size_t i = 0; i < constants.size(); ++i )
			{
				if( constIndexes[i] != -1 )
				{
					auto& c = constants[i];
					memcpy( mirror.get() + c.offset, &constParams[constIndexes[i]].value, c.size );
				}
			}
		}

		pp.GetSharedConstantBuffer( stage, mirror.get(), constantSize );
	}
	else
	{
		// Allocate constant buffer
		pp.AllocateConstantMirror( stage, constantSize );
		if( constantSize == 0 || !pp.m_stageInput[stage].m_constantBuffer.IsValid() )
		{
			return;
		}

		void* mirror = pp.m_stageInput[stage].m_constantMirror.get();
		if( !mirror )
		{
			return;
		}

		memcpy( mirror, constantDefaultValues, constantDefaultValueSize );

		if( hasConstantParams )
		{
			for( size_t i = 0; i < constants.size(); ++i )
			{
				if( constIndexes[i] != -1 )
				{
					auto& c = constants[i];
					memcpy( static_cast<uint8_t*>( mirror ) + c.offset, &constParams[constIndexes[i]].value, c.size );
				}
			}
		}

		if( hasVariableParams )
		{
			index = 0;
			// Now iterate again over the list and process the parameters:
			for( auto constantIx = constants.begin(); constantIx != constants.end(); ++constantIx )
			{
				ITriReroutablePtr paramAsReroutable;
				ITr2EffectValuePtr paramAsEffectValue = foundParams[index++];

				ITriEffectParameterPtr param = ITriEffectParameterPtr( BlueCastPtr( paramAsEffectValue ) );
				if( param )
				{
					// Notify a parameter of its future binding. Here
					// the parameter might check for sRGB flags, etc.
					param->RebuildEffectHandles( m_shader );

					paramAsReroutable = ITriReroutablePtr( BlueCastPtr( param ) );
					if( paramAsReroutable )
					{
						paramAsEffectValue.Unlock();
					}
				}

				if( paramAsEffectValue || paramAsReroutable )
				{
					// It's illegal to pass the perObjectStart if the effect has the block
					if( constantIx->offset >= perObjectStart )
					{
						// We must ignore this parameter! ValuePassParameters will have
						// spit out an error message.
						continue;
					}

					if( paramAsReroutable )
					{
						// Parameter is reroutable - this means we can hook it up directly
						// and won't have to copy its value every frame

						if( paramAsReroutable->IsRerouted() )
						{
							// Parameter is already rerouted - likely used in a previous pass
							paramAsEffectValue = param;
						}
						else
						{
							CCP_ASSERT( constantIx->offset + constantIx->size <= constantSize );

							void* dest = (void*)( (uint8_t*)mirror + constantIx->offset );
							paramAsReroutable->SetDestination( dest, constantIx->size );

							// check that it actually worked; ie the param may be reroutable in theory, but not for
							// this specific instance.
							if( !paramAsReroutable->IsRerouted() )
							{
								paramAsEffectValue = param;
							}
							else
							{
								reroutables.push_back( paramAsReroutable.Detach() );
							}
						}
					}

					if( paramAsEffectValue )
					{

						Tr2EffectParam param;
						param.m_sourceValue = paramAsEffectValue;

						// Size is now passed down when copying value from parameter so
						// we don't need to check for the size here. In fact, it is normal
						// for the destination to be smaller than the value, in particular for
						// TriTransformParameter where a portion of a 4x4 matrix is used.

						param.m_sourceName = constantIx->name.c_str();
						param.m_registerIndex = constantIx->offset;
						param.m_registerCount = constantIx->size;

						pv.push_back( param );
					}
				}
			}
		}
	}
}

void Tr2Effect::SetVariableStore( Tr2VariableStore* variableStore )
{
	m_variableStore = variableStore;
}

Tr2VariableStore& Tr2Effect::GetVariableStore()
{
	if( m_variableStore )
	{
		return *m_variableStore;
	}
	return GlobalStore();
}

// --------------------------------------------------------------------------------------
void Tr2Effect::SetParameter( const BlueSharedString& name, ITr2GpuBuffer* buffer )
{
	auto existing = GetResourceByName( name.c_str() );
	Tr2GeometryBufferParameterPtr param = BlueCastPtr( existing );
	if( param )
	{
		param->SetGpuBuffer( buffer );
	}
	else
	{
		param.CreateInstance();
		param->m_name = name;
		param->SetGpuBuffer( buffer );
		AddResource( param );
	}
}

// --------------------------------------------------------------------------------------
void Tr2Effect::SetParameter( const BlueSharedString& name, ITr2TextureProvider* texture )
{
	auto existing = GetResourceByName( name.c_str() );
	Tr2RuntimeTextureParameterPtr param = BlueCastPtr( existing );

	if( param )
	{
		param->SetTextureProvider( texture );
	}
	else
	{
		param.CreateInstance();
		param->Create( name, texture );
		AddResource( param );
	}
}

// --------------------------------------------------------------------------------------
void Tr2Effect::SetParameter( const BlueSharedString& name, uint32_t value )
{
	auto existing = GetParameterByName( name.c_str() );
	Tr2FloatParameterPtr param = BlueCastPtr( existing );
	float cast = *reinterpret_cast<float*>( &value );

	if( param )
	{
		param->SetValue( cast );
	}
	else
	{
		param.CreateInstance();
		param->m_name = name;
		param->SetValue( cast );
		m_parameters.Append( param->GetRawRoot() );
	}
}

// --------------------------------------------------------------------------------------
void Tr2Effect::SetParameter( const BlueSharedString& name, float value )
{
	auto existing = GetParameterByName( name.c_str() );
	Tr2FloatParameterPtr param = BlueCastPtr( existing );

	if( param )
	{
		param->SetValue( value );
	}
	else
	{
		param.CreateInstance();
		param->m_name = name;
		param->SetValue( value );
		m_parameters.Append( param->GetRawRoot() );
	}
}

// --------------------------------------------------------------------------------------
void Tr2Effect::SetParameter( const BlueSharedString& name, const Vector2& value )
{
	SetParameter( name, Vector4( value.x, value.y, 0.0, 0.0 ) );
}

// --------------------------------------------------------------------------------------
void Tr2Effect::SetParameter( const BlueSharedString& name, const Vector3& value )
{
	SetParameter( name, Vector4( value, 0.0 ) );
}

// --------------------------------------------------------------------------------------
void Tr2Effect::SetParameter( const BlueSharedString& name, const Vector4& value )
{
	auto existing = GetParameterByName( name.c_str() );
	Tr2Vector4ParameterPtr param = BlueCastPtr( existing );

	if( param )
	{
		param->SetValue( value );
	}
	else
	{
		param.CreateInstance();
		param->m_name = name;
		param->SetValue( value );
		m_parameters.Append( param->GetRawRoot() );
	}
}


// --------------------------------------------------------------------------------------
void Tr2Effect::SetParameter( const BlueSharedString& name, const Matrix& matrix )
{
	auto existing = GetParameterByName( name.c_str() );
	Tr2Matrix4ParameterPtr param = BlueCastPtr( existing );
	if( param )
	{
		param->SetValue( matrix );
	}
	else
	{
		param.CreateInstance();
		param->m_name = name;
		param->SetValue( matrix );
		m_parameters.Append( param->GetRawRoot() );
	}
}
