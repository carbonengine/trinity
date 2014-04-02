////////////////////////////////////////////////////////////
//
//    Created:   October 2012
//    Copyright: CCP 2012
//

#include "StdAfx.h"
#include "Tr2GeometryBufferParameter.h"
#include "include/ITr2GpuBuffer.h"
#include "ITr2ShaderState.h"

// --------------------------------------------------------------------------------------
// Description:
//   Tr2GeometryBufferParameter default constructor
// --------------------------------------------------------------------------------------
Tr2GeometryBufferParameter::Tr2GeometryBufferParameter( IRoot* lockobj )
	:m_meshIndex( 0 ),
	m_isUsedByEffect( false )
{
}

// --------------------------------------------------------------------------------------
// Description:
//   Tr2GeometryBufferParameter destructor
// --------------------------------------------------------------------------------------
Tr2GeometryBufferParameter::~Tr2GeometryBufferParameter()
{
}

// --------------------------------------------------------------------------------------
// Description:
//   Implements IInitialize interface. Loads geometry resource.
// Return Value:
//   true always
// --------------------------------------------------------------------------------------
bool Tr2GeometryBufferParameter::Initialize()
{
	LoadResources();
	return true;
}

// --------------------------------------------------------------------------------------
// Description:
//   Implements INotify interface. Allows the object to respond to parameter changes 
//   generated in Python. Reloads geometry if geometry path has changed.  
// Arguments:
//   value - The Blue-exposed parameter that changed
// Return Value:
//   true always
// --------------------------------------------------------------------------------------
bool Tr2GeometryBufferParameter::OnModified( Be::Var* val )
{
	if( IsMatch( val, m_resourcePath ) )
	{
		UnloadResources();
		Initialize();
		RebuildEffectHandles( m_cachedEffect );
	}
	return true;
}

// --------------------------------------------------------------------------------------
// Description:
//   Implements ITriEffectResourceParameter interface. Returns effect parameter name.
// Return Value:
//   Effect parameter name
// --------------------------------------------------------------------------------------
const char* Tr2GeometryBufferParameter::GetParameterName() const
{
	return m_name.c_str();
}

// --------------------------------------------------------------------------------------
// Description:
//   Implements ITriEffectResourceParameter interface. Checks if parameter is empty, 
//   needed for "magic" Tr2ShaderMaterial situations.
// Return Value:
//   true if parameter contains geometry or UAV buffer
//   false otherwise
// --------------------------------------------------------------------------------------
bool Tr2GeometryBufferParameter::IsZeroOrNull( void ) const
{
	return m_gpuBuffer == nullptr;
}

// --------------------------------------------------------------------------------------
// Description:
//   Implements ITriEffectResourceParameter interface. Called when owner effect changes.
//   Stores owner effect and updates "is used" flag.
// Arguments:
//   effectRes - Owner effect resource
// --------------------------------------------------------------------------------------
void Tr2GeometryBufferParameter::RebuildEffectHandles( ITr2ShaderState* effectRes )
{
	m_cachedEffect = effectRes;

	if ( m_name.empty() || !effectRes || !effectRes->GetResource( m_name.c_str() ) )
	{
		m_isUsedByEffect = false;
		return;
	}

	m_isUsedByEffect = true;
}

// --------------------------------------------------------------------------------------
// Description:
//   Implements ITriEffectResourceParameter interface. Supposed to reload parameter data.
//   Does nothing.
// --------------------------------------------------------------------------------------
void Tr2GeometryBufferParameter::ReloadResources()
{
}

// --------------------------------------------------------------------------------------
// Description:
//   Implements ITriEffectResourceParameter interface. Loads geometry resource.
// --------------------------------------------------------------------------------------
void Tr2GeometryBufferParameter::LoadResources()
{
	if ( !m_resourcePath.empty() )
	{
		m_gpuBuffer.Unlock();
		BeResMan->GetResourceW( m_resourcePath.c_str(), L"", BlueInterfaceIID<ITr2GpuBuffer>(), (void**)&m_gpuBuffer );
	}
}

// --------------------------------------------------------------------------------------
// Description:
//   Implements ITriEffectResourceParameter interface. Unlocks geometry resource and UAV
//   buffer.
// --------------------------------------------------------------------------------------
void Tr2GeometryBufferParameter::UnloadResources()
{
	m_gpuBuffer.Unlock();
}

// --------------------------------------------------------------------------------------
// Description:
//   Implements ITriEffectResourceParameter interface. Returns parameter value as a raw
//   pointer. Used for caching.
// Return value:
//   Value as a raw pointer.
// --------------------------------------------------------------------------------------
void* Tr2GeometryBufferParameter::GetResourcePointer() const
{
	if( m_gpuBuffer )
	{
		return m_gpuBuffer->GetGpuBuffer( m_meshIndex );
	}
	return nullptr;
}

// --------------------------------------------------------------------------------------
// Description:
//   Implements ITriEffectResourceParameter interface. Checks if the parameter is ready
//   to be used.
// Return value:
//   true If paramter contains geometry or UAV buffer
//   false Otherwise
// --------------------------------------------------------------------------------------
bool Tr2GeometryBufferParameter::IsPrepared() const
{
	return m_gpuBuffer && m_gpuBuffer->GetGpuBuffer( m_meshIndex );
}

// --------------------------------------------------------------------------------------
// Description:
//   Implements ITriEffectResourceParameter interface. Applies paramter value to effect.
// Arguments:
//   inputType - Shader type to apply resource to
//   destHandle - Slot number
//   resourceFlags - union of ITr2EffectValue::ResourceFlags
//   renderContext - current render context
// --------------------------------------------------------------------------------------
void Tr2GeometryBufferParameter::CopyValueToEffect(		Tr2RenderContextEnum::ShaderType inputType, 
														unsigned char* destHandle, 
														size_t resourceFlags,
														Tr2RenderContext &renderContext ) const
{
	if( !m_gpuBuffer )
	{
		return;
	}
	bool isUav = ( resourceFlags & RESOURCE_FLAG_UAV ) != 0;
	unsigned int ix = *destHandle;
	Tr2GpuBufferAL* buffer = m_gpuBuffer->GetGpuBuffer( m_meshIndex );
	if( !buffer )
	{
		return;
	}
	if( isUav )
	{
		renderContext.SetUav( inputType, ix, *buffer );
	}
	else
	{
		renderContext.m_esm.ApplyShaderBuffer( inputType, ix, *buffer );
	}
}

// --------------------------------------------------------------------------------------
// Description:
//   Checks if the parameter data is valid. Used for debugging.
// Return value:
//   true If paramter contains valid geometry or UAV buffer
//   false Otherwise
// --------------------------------------------------------------------------------------
bool Tr2GeometryBufferParameter::IsValid() const
{
	return m_gpuBuffer && m_gpuBuffer->GetGpuBuffer( m_meshIndex );
}
