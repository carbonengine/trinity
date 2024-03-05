////////////////////////////////////////////////////////////
//
//    Created:   March 2023
//    Copyright: CCP 2023
//

#include "StdAfx.h"
#include "Tr2TextureAnimationParameter.h"
#include "../../Tr2TextureAnimation.h"
#include "../../Tr2Renderer.h"
#include "../Tr2Shader.h"
#include "../Tr2Material.h"


Tr2TextureAnimationParameter::Tr2TextureAnimationParameter( IRoot* lockobj ) :
	m_resourceType( Tr2EffectResource::TEXTURE_TYPELESS )
{
}

bool Tr2TextureAnimationParameter::OnModified( Be::Var* value )
{
	if( IsMatch( value, m_animation ) )
	{
		for( auto it = begin( m_materials ); it != end( m_materials ); ++it )
		{
			( *it )->InvalidateResourceSets();
		}
	}
	return true;
}

bool Tr2TextureAnimationParameter::CopyToResourceSet(
	Tr2ResourceSetDescriptionAL& resourceDesc,
	Tr2RenderContextEnum::ShaderType stage,
	uint32_t registerIndex,
	ResourceFlags flags ) const
{
	bool isSrgb = ( flags & RESOURCE_FLAG_SRGB ) != 0;
	auto colorSpace = isSrgb ? Tr2RenderContextEnum::COLOR_SPACE_SRGB : Tr2RenderContextEnum::COLOR_SPACE_LINEAR;
	if( m_animation )
	{
		return resourceDesc.SetSrv( stage, registerIndex, m_animation->GetTexture( m_channel ), colorSpace );
	}
	else
	{
		return resourceDesc.SetSrv( stage, registerIndex, Tr2Renderer::GetFallbackTexture( m_resourceType, m_name.c_str() ), colorSpace );
	}
}

bool Tr2TextureAnimationParameter::ApplyUav(
	Tr2ResourceSetDescriptionAL& resourceDesc,
	Tr2RenderContextEnum::ShaderType stage,
	uint32_t registerIndex ) const
{
	return false;
}

const char* Tr2TextureAnimationParameter::GetParameterName() const
{
	return m_name.c_str();
}

void Tr2TextureAnimationParameter::RebuildEffectHandles( Tr2Shader* effectRes )
{
	if( m_name.empty() || !effectRes )
	{
		return;
	}

	auto resource = effectRes->GetResource( m_name.c_str() );
	if( !resource )
	{
		return;
	}
	m_resourceType = resource->type;
}

unsigned Tr2TextureAnimationParameter::GetHashValue( unsigned startingHash ) const
{
	return CcpHashFNV1( &m_animation.p, sizeof( m_animation.p ), startingHash );
}

void Tr2TextureAnimationParameter::OnAddedToMaterial( Tr2Material* material )
{
	m_materials.push_back( material );
}

void Tr2TextureAnimationParameter::OnRemovedFromMaterial( Tr2Material* material )
{
	auto found = find( begin( m_materials ), end( m_materials ), material );
	if( found != end( m_materials ) )
	{
		m_materials.erase( found );
	}
}

Tr2TextureAL Tr2TextureAnimationParameter::GetTexture() const
{
	if( m_animation )
	{
		return m_animation->GetTexture( m_channel );
	}
	return Tr2TextureAL();
}