#include "StdAfx.h"
#include "TriTextureParameter.h"
#include "ITr2TextureProvider.h"
#include "Shader/Tr2Shader.h"
#include "Tr2Renderer.h"

TriTextureParameter::TriTextureParameter(IRoot* lockobj):
	m_isUsedByEffect( false ),
	m_resourceType( Tr2EffectResource::TEXTURE_TYPELESS )
{
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

bool TriTextureParameter::OnModified(	Be::Var* val )
{
	m_resource.Unlock();

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
		return resourceDesc.Set( stage, registerIndex, *tex, colorSpace );
	}
	else
	{
		return resourceDesc.Set( stage, registerIndex, Tr2Renderer::GetFallbackTexture( m_resourceType, m_name.c_str() ), colorSpace );
	}
}

// --------------------------------------------------------------------------------------
void TriTextureParameter::ApplyUav(
	Tr2RenderContextEnum::ShaderType stage,
	uint32_t registerIndex,
	uint32_t initialCount,
	Tr2RenderContext &renderContext ) const
{
	auto resource = GetResource();
	if( Tr2TextureAL* tex = ( resource ? resource->GetTexture() : nullptr ) )
	{
		renderContext.SetUav( stage, registerIndex, *tex );
	}
	else
	{
		// TODO: Fix the signature of SetUav to take a const reference
		renderContext.SetUav( stage, registerIndex, const_cast<Tr2TextureAL&>( nullTX ) );
	}
}

// ---------------------------------------------------------------
bool TriTextureParameter::Initialize()
{
	if( !m_resourcePath.empty() )
	{
		m_resource = nullptr;
		BeResMan->GetResource( m_resourcePath.c_str(), "", BlueInterfaceIID<ITr2TextureProvider>(), (void**)&m_resource );
	}
	else
	{
		m_resource.Unlock();
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
	RebuildEffectHandles( m_cachedEffect );
}

ITr2TextureProvider* TriTextureParameter::GetResource() const
{
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
