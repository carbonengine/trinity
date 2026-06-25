// Copyright © 2023 CCP ehf.

#include "StdAfx.h"
#include "TriTextureParameter.h"
#include "ITr2TextureProvider.h"
#include "Shader/Tr2Shader.h"
#include "Shader/Tr2Material.h"
#include "Tr2Renderer.h"
#include "Resources/TriTextureRes.h"


TriTextureParameter::TriTextureParameter( IRoot* lockobj ) :
	m_resourceType( Tr2EffectResource::TEXTURE_TYPELESS ),
	m_uavMipLevel( 0 ),
	m_isUsedByEffect( false ),
	m_textureLodEnabled( false ),
	m_bindlessColorSpace( Tr2RenderContextEnum::COLOR_SPACE_LINEAR )
{
	std::fill( std::begin( m_uvDensityScale ), std::end( m_uvDensityScale ), 0.f );
	std::fill( std::begin( m_cachedSrvIndex ), std::end( m_cachedSrvIndex ), 0xffffffff );
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

bool TriTextureParameter::SupportsDirtyNotification() const
{
	return true;
}

void TriTextureParameter::UsedWithScreenSize( float screenSize, float worldRadius, const std::vector<float>& uvDensities )
{
	if( m_textureRes )
	{
		uint32_t requestedLod = 0;
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
			if( resolution != 0 )
			{
				auto requestedResolution = std::max( 1.f, resolution );
				auto resolutionChange = (uint32_t)( m_textureRes->GetOriginalResolutionAsFloat() / requestedResolution );
				if( resolutionChange > 0 )
				{
					// quickly calculate log2 of resolutionChange, which gives us the required LOD
#if __APPLE__
					requestedLod = 31 - (uint32_t)__builtin_clz( resolutionChange );
#else
					unsigned long reverse;
					_BitScanReverse( &reverse, resolutionChange );
					requestedLod = reverse;
#endif
				}
			}
			m_textureRes->RequestResolution( requestedLod );
		}
		else
		{
			m_textureRes->RequestResolution( requestedLod );
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
	if( m_resourcePath.empty() || strcmp( m_resourcePath.c_str(), resourcePath ) != 0 )
	{
		m_resourcePath = resourcePath;
		OnModified( (Be::Var*)&m_resourcePath );
	}
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

bool TriTextureParameter::OnModified( Be::Var* val )
{
	if( m_resource )
	{
		m_resource->OnTextureChange().UnregisterListener( this );
		m_resource = nullptr;
	}
	if( m_lowResResource )
	{
		m_lowResResource->OnTextureChange().UnregisterListener( this );
		m_lowResResource = nullptr;
	}
	m_textureRes = nullptr;

	Initialize();

	RebuildEffectHandles( m_cachedEffect );

	return true;
}

void TriTextureParameter::CopyValueToEffect(
	Tr2RenderContextEnum::ShaderType inputType,
	unsigned char* destHandle,
	size_t size,
	Tr2RenderContext& renderContext ) const
{
	uint32_t value = m_cachedSrvIndex[m_bindlessColorSpace];
	memcpy( destHandle, &value, std::min( size, sizeof( value ) ) );
}

void TriTextureParameter::AddUsedTexture( Tr2BindlessResourcesAL& usedTextures ) const
{
	usedTextures.Add( m_cachedTexture );
}

// --------------------------------------------------------------------------------------
bool TriTextureParameter::CopyToResourceSet(
	Tr2ResourceSetDescriptionAL& resourceDesc,
	Tr2RenderContextEnum::ShaderType stage,
	uint32_t registerIndex,
	ResourceFlags flags ) const
{
	bool isSrgb = ( flags & RESOURCE_FLAG_SRGB ) != 0;
	auto colorSpace = isSrgb ? Tr2RenderContextEnum::COLOR_SPACE_SRGB : Tr2RenderContextEnum::COLOR_SPACE_LINEAR;
	return resourceDesc.SetSrv( stage, registerIndex, m_cachedTexture, colorSpace );
}

// --------------------------------------------------------------------------------------
bool TriTextureParameter::ApplyUav(
	Tr2ResourceSetDescriptionAL& resourceDesc,
	Tr2RenderContextEnum::ShaderType stage,
	uint32_t registerIndex ) const
{
	return resourceDesc.SetUav( stage, registerIndex, m_cachedTexture, m_uavMipLevel );
}

// ---------------------------------------------------------------
bool TriTextureParameter::Initialize()
{
	if( m_resource )
	{
		m_resource->OnTextureChange().UnregisterListener( this );
		m_resource = nullptr;
	}
	if( m_lowResResource )
	{
		m_lowResResource->OnTextureChange().UnregisterListener( this );
		m_lowResResource = nullptr;
	}
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
					if( m_lowResResource )
					{
						m_lowResResource->OnTextureChange().RegisterListener<TriTextureParameter, &TriTextureParameter::OnTextureChanged>( this );
					}
				}
			}
		}


		BeResMan->GetResource( m_resourcePath.c_str(), "", BlueInterfaceIID<ITr2TextureProvider>(), (void**)&m_resource );
		if( m_resource )
		{
			m_resource->OnTextureChange().RegisterListener<TriTextureParameter, &TriTextureParameter::OnTextureChanged>( this );
		}
		m_textureRes = BlueCastPtr( m_resource );
	}
	OnTextureChanged();
	return true;
}

// --------------------------------------------------------------------------------------
// Description:
//   Let go of our current resource, and take ownership of newRes instead.
// --------------------------------------------------------------------------------------
void TriTextureParameter::SetResource( ITr2TextureProvider* newRes )
{
	if( m_resource != newRes )
	{
		if( m_resource )
		{
			m_resource->OnTextureChange().UnregisterListener( this );
		}
		m_resource = newRes;
		if( m_resource )
		{
			m_resource->OnTextureChange().RegisterListener<TriTextureParameter, &TriTextureParameter::OnTextureChanged>( this );
		}
	}
	if( m_lowResResource )
	{
		m_lowResResource->OnTextureChange().UnregisterListener( this );
		m_lowResResource = nullptr;
	}
	m_textureRes = BlueCastPtr( m_resource );
	RebuildEffectHandles( m_cachedEffect );
	OnTextureChanged();
}

ITr2TextureProvider* TriTextureParameter::GetResource() const
{
	if( m_lowResResource )
	{
		if( m_resource->GetTexture() )
		{
			if( m_lowResResource )
			{
				m_lowResResource->OnTextureChange().UnregisterListener( this );
				m_lowResResource = nullptr;
			}
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

void TriTextureParameter::OnAddedToMaterial( Tr2Material* material )
{
	m_materials.push_back( material );
}

void TriTextureParameter::OnRemovedFromMaterial( Tr2Material* material )
{
	m_materials.erase( find( begin( m_materials ), end( m_materials ), material ), end( m_materials ) );
}

void TriTextureParameter::RebuildEffectHandles( Tr2Shader* effectRes )
{
	m_cachedEffect = effectRes;

	m_isUsedByEffect = false;
	if( m_name.empty() || !effectRes )
	{
		return;
	}

	auto resource = effectRes->GetResource( m_name.c_str() );
	if( resource )
	{
		m_resourceType = resource->type;
		m_isUsedByEffect = true;
	}
	else
	{
		auto c = effectRes->GetConstant( m_name.c_str() );
		if( c )
		{
			m_isUsedByEffect = true;
			m_resourceType = Tr2EffectResource::TEXTURE_2D;
			m_bindlessColorSpace = Tr2RenderContextEnum::COLOR_SPACE_LINEAR;

			if( auto annotations = effectRes->GetParameterAnnotations( m_name.c_str() ) )
			{
				auto found = std::find_if( begin( *annotations ), end( *annotations ), []( auto& x ) {
					return strcmp( x.name, "BindlessHandleType" ) == 0 && x.type == Tr2EffectParameterAnnotation::INT;
				} );
				if( found != end( *annotations ) )
				{
					m_resourceType = Tr2EffectResource::Type( found->intValue );
					//CCP_LOGERR( "Got resource type." );
				}
				else
				{
					CCP_ASSERT_M( false, "Bindless texture input is defined as uint, but does not have the BindlessHandleType annotation set. Either change the type to BindlessTexture??? or add the annotation" );
				}
				found = std::find_if( begin( *annotations ), end( *annotations ), [&]( auto& x ) {
					return strcmp( x.name, "Tr2sRGB" ) == 0 && x.type == Tr2EffectParameterAnnotation::BOOL;
				} );
				if( found != end( *annotations ) )
				{
					m_bindlessColorSpace = found->boolValue ? Tr2RenderContextEnum::COLOR_SPACE_SRGB : Tr2RenderContextEnum::COLOR_SPACE_LINEAR;
				}
			}
		}
	}
	CacheTexture();
}

void TriTextureParameter::CacheTexture()
{
	auto resource = GetResource();
	if( const Tr2TextureAL* tex = ( resource ? resource->GetTexture() : nullptr ) )
	{
		m_cachedTexture = *tex;
	}
	else
	{
		m_cachedTexture = Tr2Renderer::GetFallbackTexture( m_resourceType, m_name.c_str() );
	}
	m_cachedSrvIndex[0] = m_cachedTexture.GetSrvIndexInHeap( Tr2RenderContextEnum::COLOR_SPACE_LINEAR );
	m_cachedSrvIndex[1] = m_cachedTexture.GetSrvIndexInHeap( Tr2RenderContextEnum::COLOR_SPACE_SRGB );
}

void TriTextureParameter::OnTextureChanged()
{
	CacheTexture();
	for_each( begin( m_materials ), end( m_materials ), []( auto& material ) {
		material->ResourceChanged();
		material->MarkConstantBuffersDirty();
	} );
}