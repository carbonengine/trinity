#include "StdAfx.h"
#include "TriTexture2DParameter.h"
#include "Tr2EffectStateManager.h"
#include "blue/include/IBlueResMan.h"
#include "Resources/TriTextureRes.h"
#include "blue/include/TransGaming.h"

TriTexture2DParameter::TriTexture2DParameter(IRoot* lockobj):
	m_isUsedByEffect( false ),
	m_useOverrides( false ),
	m_overrideSamplerStateIx( Tr2EffectStateManager::UNINITIALIZED_DECLARATION ),
	m_overrideAddressUMode( Tr2RenderContextEnum::TA_WRAP ),
	m_overrideAddressVMode( Tr2RenderContextEnum::TA_WRAP ),
	m_overrideAddressWMode( Tr2RenderContextEnum::TA_WRAP ),
	m_overrideFilterMode( Tr2RenderContextEnum::TF_LINEAR ),
	m_overrideMipFilterMode( Tr2RenderContextEnum::TF_LINEAR ),
	m_overrideMipmapLodBias( 0.f ),
	m_overrideMaxMipLevel( 0 ),
	m_overrideMaxAnisotropy( 4 ),
	m_overrideSrgb( false )
{
}


TriTexture2DParameter::~TriTexture2DParameter()
{
}

void TriTexture2DParameter::ReleaseResources( TriStorage s )
{
	m_overrideSamplerStateIx = Tr2EffectStateManager::UNINITIALIZED_DECLARATION;
}

bool TriTexture2DParameter::OnPrepareResources()
{
	BuildOverrideSamplerStateIx();
	return true;
}

const char* TriTexture2DParameter::GetParameterName() const
{
	return m_name.c_str();
}

void TriTexture2DParameter::SetParameterName( const char* name )
{
	m_name = BlueSharedString( name );
}

const char* TriTexture2DParameter::GetResourcePath() const
{
	return m_resourcePath.c_str();
}

void TriTexture2DParameter::SetResourcePath( const char* resourcePath )
{
	m_resourcePath = resourcePath;
	OnModified( (Be::Var*)&m_resourcePath );
}


// --------------------------------------------------------------------------------------
// Description:
//   Determines whether the value of this texture parameter is NULL and be ignored when
//   building the material situation.
// Return Value:
//   true, if the texture is NULL
//   false, otherwise
// --------------------------------------------------------------------------------------
bool TriTexture2DParameter::IsZeroOrNull( void ) const
{
	return m_resource == NULL;
}

void TriTexture2DParameter::ReloadResources()
{
	if ( m_resource )
	{
		m_resource->ReloadResources();
	}
}

bool TriTexture2DParameter::OnModified(	Be::Var* val )
{
	if( IsMatch( val, m_useOverrides          ) ||
		IsMatch( val, m_overrideAddressUMode  ) ||
		IsMatch( val, m_overrideAddressVMode  ) ||
		IsMatch( val, m_overrideAddressWMode  ) ||
		IsMatch( val, m_overrideFilterMode    ) ||
		IsMatch( val, m_overrideFilterMode    ) ||
		IsMatch( val, m_overrideMipFilterMode ) ||
		IsMatch( val, m_overrideMipmapLodBias ) ||
		IsMatch( val, m_overrideMaxMipLevel   ) ||
		IsMatch( val, m_overrideMaxAnisotropy ) ||
		IsMatch( val, m_overrideSrgb		  )
	  )
	{
		BuildOverrideSamplerStateIx();
		return true;
	}
		
	UnloadResources();

	Initialize();

	RebuildEffectHandles( m_cachedEffect );

	return true;
}

// ---------------------------------------------------------------------------------------------------------
void TriTexture2DParameter::BuildOverrideSamplerStateIx()
{
	// should we use the overrides?
	if(m_useOverrides)
	{
		// build a new setup...
		Tr2SamplerDescription sampler(
			m_overrideFilterMode,
			m_overrideFilterMode,
			m_overrideMipFilterMode,
			false,
			m_overrideAddressUMode,
			m_overrideAddressVMode,
			m_overrideAddressWMode,
			m_overrideMipmapLodBias,
			m_overrideMaxAnisotropy,
			Tr2RenderContextEnum::CMP_NEVER,
			Color( 0.f, 0.f, 0.f, 0.f ),
			float( m_overrideMaxMipLevel ),
			FLT_MAX,
			m_overrideSrgb
			);

		// register it! or "unregister" it
		m_overrideSamplerStateIx = Tr2EffectStateManager::RegisterSamplerSetup( sampler );
	}
	else
	{
		m_overrideSamplerStateIx = 0xffffffff;
	}
}

// --------------------------------------------------------------------------------------
// Description:
//   Applies texture and possibly state overrides to the render context.
// Arguments:
//   inputType - shader type
//   destHandle - reinterpreted as a sampler index
//   resourceFlags - union of ITr2EffectValue::ResourceFlags
//   renderContext - current render context
// --------------------------------------------------------------------------------------
void TriTexture2DParameter::CopyValueToEffect(	Tr2RenderContextEnum::ShaderType inputType, 
												unsigned char* destHandle, 
												size_t resourceFlags,
												Tr2RenderContext &renderContext ) const
{
	unsigned int ix = *destHandle;
	bool isUav = ( resourceFlags & RESOURCE_FLAG_UAV ) != 0;
	if( Tr2TextureAL* tex = ( m_resource ? m_resource->GetTexture() : nullptr ) )
	{		
		if( isUav )
		{
			renderContext.SetUav( inputType, ix, *tex );
		}
		else
		{
			bool isSrgb = m_useOverrides ? m_overrideSrgb : ( resourceFlags & RESOURCE_FLAG_SRGB ) != 0;
			auto colorSpace = isSrgb ? Tr2RenderContextEnum::COLOR_SPACE_SRGB : Tr2RenderContextEnum::COLOR_SPACE_LINEAR;
			renderContext.m_esm.ApplyTexture( inputType, ix, *tex, colorSpace );
		}
	}
	else if( !IsTransgaming() )
	{
		// For some reason doing this on the Mac seems to cause more problems - we get a black
		// screen on character select, at least on some older Macs with 9400m or 320m video cards
		if( isUav )
		{
			// TODO: Fix the signature of SetUav to take a const reference
			renderContext.SetUav( inputType, ix, const_cast<Tr2TextureAL&>( nullTX ) );
		}
		else
		{
			renderContext.m_esm.ApplyTexture( inputType, ix, nullTX );
		}
	}

	if( m_useOverrides )
	{ 
		renderContext.m_esm.ApplySamplerSetup( inputType, ix, m_overrideSamplerStateIx );
		destHandle[1] = 1;
	}
}

// --------------------------------------------------------------------------------------
// Description:
//   Checks if the texture resource is prepared.
// Return value:
//   true If the resource path is empty or resource is good
//	 false Otherwise
// --------------------------------------------------------------------------------------
bool TriTexture2DParameter::IsPrepared() const
{
	if( *m_resourcePath.c_str() == 0 || ( m_resource && m_resource->IsPrepared() ) )
	{
		return true;
	}
	return false;
}

// ---------------------------------------------------------------
bool TriTexture2DParameter::Initialize()
{
	LoadResources();

	// make sure we get our own SamplerStateBlock, if we need to override!
	BuildOverrideSamplerStateIx();
	return true;
}

// --------------------------------------------------------------------------------------
// Description:
//   Let go of our current resource, and take ownership of newRes instead.
// --------------------------------------------------------------------------------------
void TriTexture2DParameter::SetResource( TriTextureRes* newRes )
{
	m_resource = newRes;
	RebuildEffectHandles( m_cachedEffect );
}

TriTextureRes* TriTexture2DParameter::GetResource()
{
	return m_resource;
}

void* TriTexture2DParameter::GetResourcePointer() const
{
	return m_resource ? m_resource->GetTexture() : nullptr;
}

void TriTexture2DParameter::LoadResources()
{
	if ( !m_resourcePath.empty() )
	{
		m_resource = nullptr;
		BeResMan->GetResource( m_resourcePath.c_str(), "", BlueInterfaceIID<TriTextureRes>(), (void**)&m_resource );		
	}
	else
	{
		UnloadResources();
	}
}

void TriTexture2DParameter::UnloadResources()
{
	m_resource.Unlock();
}

// --------------------------------------------------------------------------------------
//  Description:
//    Copies any resource that was dynamically assigned to the parameter to the new copy
// --------------------------------------------------------------------------------------
bool TriTexture2DParameter::AssignTo( ICopierCustomAssignment* other, 
											  ICopier* copier )
{
	if( m_resourcePath.empty() && m_resource )
	{
		// texture that was dynamically assigned
		TriTexture2DParameter* dest = (TriTexture2DParameter*)other;
		dest->SetResource( m_resource );
	}
	return true;
}

void TriTexture2DParameter::RebuildEffectHandles( ITr2ShaderState* effectRes )
{
	m_cachedEffect = effectRes;

	if ( m_name.empty() || !effectRes || !effectRes->GetResource( m_name.c_str() ) )
	{
		m_isUsedByEffect = false;
		return;
	}

	m_isUsedByEffect = true;
}
