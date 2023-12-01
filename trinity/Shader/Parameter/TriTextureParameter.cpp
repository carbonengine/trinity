#include "StdAfx.h"
#include "TriTextureParameter.h"
#include "ITr2TextureProvider.h"
#include "Shader/Tr2Shader.h"
#include "Tr2Renderer.h"
#include "Resources/TriTextureRes.h"


TriTextureParameter::TriTextureParameter(IRoot* lockobj):
	m_resourceType( Tr2EffectResource::TEXTURE_TYPELESS ),
	m_uavMipLevel( 0 ),
	m_isUsedByEffect( false ),
	m_textureLodEnabled( false )
{
	std::fill( std::begin( m_uvDensityScale ), std::end( m_uvDensityScale ), 0.f );
}


TriTextureParameter::~TriTextureParameter()
{
}

const char* TriTextureParameter::GetParameterName() const
{
	return m_name.c_str();
}

void TriTextureParameter::SetParameterName( const BlueSharedString& name )
{
	m_name = name;
}

unsigned TriTextureParameter::GetHashValue( unsigned startingHash ) const
{
	if( IBlueResourcePtr currentRes = BlueCastPtr( m_resource ) )
	{
		startingHash = CcpHashFNV1( currentRes->GetPath(), wcslen( currentRes->GetPath() ) * sizeof( wchar_t ), startingHash );
	}
	auto name = m_name.c_str();
	return CcpHashFNV1( &name, sizeof( name ), startingHash );
}

void TriTextureParameter::UsedWithScreenSize( float screenSize, float worldRadius, const std::vector<float>& uvDensities )
{
	if( m_textureRes )
	{
		if( m_textureLodEnabled && !uvDensities.empty() )
		{
			size_t i = 0;
			float resolution = 0;

			{
				auto density = worldRadius * m_uvDensityScale[0];
				if( density > 0 )
				{
					resolution = std::max( resolution, screenSize / density );
				}
			}

			for( auto uv : uvDensities )
			{
				auto scale = m_uvDensityScale[1 + i++];
				auto density = uv * scale;
				if( density > 0 )
				{
					resolution = std::max( resolution, screenSize / density );
				}
			}
			if( resolution == 0 )
			{
				resolution = std::numeric_limits<float>::max();
			}
			m_textureRes->RequestResolution( resolution );
		}
		else
		{
			m_textureRes->RequestResolution( std::numeric_limits<float>::max() );
		}
	}
}

// -------------------------------------------------------------
// Description:
//   Returns the respath to the currently used texture. Might
//   be LOD based.
// -------------------------------------------------------------
const wchar_t* TriTextureParameter::GetResourcePath() const
{
	IBlueResourcePtr currentRes = BlueCastPtr( GetResource() );
	if( !currentRes )
	{
		return L"";
	}
	return currentRes->GetPath();
}

void TriTextureParameter::SetResourcePath( const char* resourcePath )
{
	m_resourcePath = resourcePath;
	OnModified( (Be::Var*)&m_resourcePath );
}

void TriTextureParameter::EnableTextureLoding( const std::array<float, UV_SET_MAX_COUNT>& uvDensityScale )
{
	m_textureLodEnabled = true;
	std::copy( begin( uvDensityScale ), end( uvDensityScale ), begin( m_uvDensityScale ) );
}

void TriTextureParameter::DisableTextureLoding()
{
	m_textureLodEnabled = false;
}

bool TriTextureParameter::OnModified(	Be::Var* val )
{
	m_resource.Unlock();
	m_lowResResource = nullptr;
	m_textureRes = nullptr;

	Initialize();

	RebuildEffectHandles( m_cachedEffect );

	return true;
}

// --------------------------------------------------------------------------------------
bool TriTextureParameter::CopyToResourceSet(
	Tr2ResourceSetDescriptionAL& resourceDesc,
	Tr2RenderContextEnum::ShaderType stage,
	uint32_t registerIndex,
	ResourceFlags flags ) const
{
	auto resource = GetResource();
	bool isSrgb = ( flags & RESOURCE_FLAG_SRGB ) != 0;
	auto colorSpace = isSrgb ? Tr2RenderContextEnum::COLOR_SPACE_SRGB : Tr2RenderContextEnum::COLOR_SPACE_LINEAR;
	if( const Tr2TextureAL* tex = ( resource ? resource->GetTexture() : nullptr ) )
	{
		return resourceDesc.SetSrv( stage, registerIndex, *tex, colorSpace );
	}
	else
	{
		return resourceDesc.SetSrv( stage, registerIndex, Tr2Renderer::GetFallbackTexture( m_resourceType, m_name.c_str() ), colorSpace );
	}
}

// --------------------------------------------------------------------------------------
bool TriTextureParameter::ApplyUav(
	Tr2ResourceSetDescriptionAL& resourceDesc,
	Tr2RenderContextEnum::ShaderType stage,
	uint32_t registerIndex ) const
{
	auto resource = GetResource();
	if( Tr2TextureAL* tex = ( resource ? resource->GetTexture() : nullptr ) )
	{
		return resourceDesc.SetUav( stage, registerIndex, *tex, m_uavMipLevel );
	}
	else
	{
		return resourceDesc.SetUav( stage, registerIndex, Tr2TextureAL() );
	}
}

// ---------------------------------------------------------------
bool TriTextureParameter::Initialize()
{
	m_resource = nullptr;
	m_lowResResource = nullptr;
	m_textureRes = nullptr;

	if( !m_resourcePath.empty() )
	{
		if( !BePaths->FileExistsLocally( CA2W( m_resourcePath.c_str() ) ) )
		{
			auto dot = m_resourcePath.rfind( '.' );
			if( dot != std::string::npos )
			{
				auto lowResPath = m_resourcePath.substr( 0, dot ) + "_lowdetail" + m_resourcePath.substr( dot );
				if( BePaths->FileExistsLocally( CA2W( lowResPath.c_str() ) ) )
				{
					BeResMan->GetResource( lowResPath.c_str(), "", BlueInterfaceIID<ITr2TextureProvider>(), (void**)&m_lowResResource );
				}
			}
		}


		BeResMan->GetResource( m_resourcePath.c_str(), "", BlueInterfaceIID<ITr2TextureProvider>(), (void**)&m_resource );
		m_textureRes = BlueCastPtr( m_resource );
	}
	return true;
}

// --------------------------------------------------------------------------------------
// Description:
//   Let go of our current resource, and take ownership of newRes instead.
// --------------------------------------------------------------------------------------
void TriTextureParameter::SetResource( ITr2TextureProvider* newRes )
{
	m_resource = newRes;
	m_lowResResource = nullptr;
	m_textureRes = BlueCastPtr( m_resource );
	RebuildEffectHandles( m_cachedEffect );
}

ITr2TextureProvider* TriTextureParameter::GetResource() const
{
	if( m_lowResResource )
	{
		if( m_resource->GetTexture() )
		{
			m_lowResResource = nullptr;
		}
		else
		{
			return m_lowResResource;
		}
	}
	return m_resource;
}

// --------------------------------------------------------------------------------------
//  Description:
//    Copies any resource that was dynamically assigned to the parameter to the new copy
// --------------------------------------------------------------------------------------
bool TriTextureParameter::AssignTo( ICopierCustomAssignment* other, 
											  ICopier* copier )
{
	if( m_resourcePath.empty() && m_resource )
	{
		// texture that was dynamically assigned
		TriTextureParameter* dest = static_cast<TriTextureParameter*>( other );
		dest->SetResource( m_resource );
	}
	return true;
}

void TriTextureParameter::RebuildEffectHandles( Tr2Shader* effectRes )
{
	m_cachedEffect = effectRes;

	m_isUsedByEffect = false;
	if ( m_name.empty() || !effectRes )
	{
		return;
	}

	auto resource = effectRes->GetResource( m_name.c_str() );
	if( !resource )
	{
		return;
	}
	m_resourceType = resource->type;
	m_isUsedByEffect = true;
}
