////////////////////////////////////////////////////////////
//
//    Created:   November 2013
//    Copyright: CCP 2013
//

#pragma once
#ifndef Tr2PersistentPerObjectData_H
#define Tr2PersistentPerObjectData_H

#include "Tr2PerObjectData.h"
#include "Tr2Renderer.h"

// --------------------------------------------------------------------------------------
// Description:
//   Helper class that allows storing per-object data constant buffer with the owner
//   object for DX11. This in turn allows filling this constant buffer once per frame 
//   only. For other platforms this class does not store a constant buffer, but rather
//   fills the one provided. To fill the buffer the object calls methods 
//   GetPerObjectDataSize and UpdatePerObjectBuffer of the owner object.
// See Also:
//   Tr2PerObjectDataWithPersistentBuffers, Tr2PerObjectData
// --------------------------------------------------------------------------------------
template <typename Owner>
class Tr2PersistentPerObjectData
	: public Tr2DeviceResource
{
public:
	Tr2PersistentPerObjectData()
		:m_bufferDirty( true )
	{
	}

	// ----------------------------------------------------------------------------------
	// Description:
	//   Invalidates dirty flag for the object. Next time it is requested to apply its 
	//   constant buffer, the object fill re-fill it with new data.
	// ----------------------------------------------------------------------------------
	void InvalidateBufferData()
	{
		m_bufferDirty = true;
	}

	// ----------------------------------------------------------------------------------
	// Description:
	//   Applies constant buffer to the render context filling it if needed.
	// Arguments:
	//   owner - Owner object
	//   buffers - Array of transient constant buffers (usually provided by 
	//     Tr2EffectStateManager); not used in DX11 as we have our own buffer in this case
	//   renderContext - Current render context
	// ----------------------------------------------------------------------------------
	void SetPerObjectDataToDevice(
		Owner& owner,
		Tr2RenderContextEnum::ShaderType shaderType,
		Tr2ConstantBufferAL** buffers,
		Tr2RenderContext& renderContext )
	{
#if TRINITY_PLATFORM == TRINITY_METAL
		bool isPrimary = &renderContext == Tr2RenderContext::GetPrimaryRenderContextPointer();
#else
		bool isPrimary = true;
#endif

		if( isPrimary && !m_bufferDirty )
		{
			renderContext.SetConstants( m_constantBuffer,
				shaderType,
				Tr2Renderer::GetPerObjectStartRegister( shaderType ) );
			return;
		}
		uint32_t size = owner.GetPerObjectDataSize( shaderType );
		if( size > 0 )
		{
			auto& buffer = isPrimary ? m_constantBuffer : *buffers[shaderType];
			if( !buffer.IsValid() || size > buffer.GetSize() )
			{
				CR_RETURN( buffer.Create( size, renderContext.GetPrimaryRenderContext() ) );
			}
			void* data = nullptr;

			if( SUCCEEDED( buffer.Lock( &data, renderContext ) ) && data )
			{
				owner.UpdatePerObjectBuffer( shaderType, size, data );
				buffer.Unlock( renderContext );
				renderContext.SetConstants( buffer, shaderType, Tr2Renderer::GetPerObjectStartRegister( shaderType ) );
			}
		}
		if( isPrimary )
		{
			m_bufferDirty = false;
		}
	}

	virtual void ReleaseResources( TriStorage s )
	{
		if( s & TRISTORAGE_ALL )
		{
			m_constantBuffer = Tr2ConstantBufferAL();
			m_bufferDirty = true;
		}
	}
protected:
	virtual bool OnPrepareResources()
	{
		return true;
	}
private:
	Tr2ConstantBufferAL m_constantBuffer;
	bool m_bufferDirty;
};

// --------------------------------------------------------------------------------------
// Description:
//   Per-object data composed of two Tr2PersistentPerObjectData objects: one for vertex
//   stage and one for pixel stage. 
// See Also:
//   Tr2PersistentPerObjectData, Tr2PerObjectData
// --------------------------------------------------------------------------------------
template <typename Owner>
class Tr2PerObjectDataWithPersistentBuffers : public Tr2PerObjectData
{
public:
	typedef Tr2PersistentPerObjectData<Owner> PerObjectDataVs;
	typedef Tr2PersistentPerObjectData<Owner> PerObjectDataPs;

	Tr2PerObjectDataWithPersistentBuffers()
		:m_owner( nullptr ),
		m_perObjectDataVs( nullptr ),
		m_perObjectDataPs( nullptr )
	{
	}

	// ----------------------------------------------------------------------------------
	// Description:
	//   Initializes the object.
	// Arguments:
	//   owner - Owner object, cannot be NULL
	//   perObjectDataVs - Per-object constant buffer for vertex stage, can be NULL if VS 
	//     is not expecting per-object data
	//   perObjectDataPs - Per-object constant buffer for vertex stage, can be NULL if PS 
	//     is not expecting per-object data
	// ----------------------------------------------------------------------------------
	void Initialize( 
		Owner* owner, 
		PerObjectDataVs* perObjectDataVs, 
		PerObjectDataPs* perObjectDataPs )
	{
		m_owner = owner;
		m_perObjectDataVs = perObjectDataVs;
		m_perObjectDataPs = perObjectDataPs;
	}

	// ----------------------------------------------------------------------------------
	// Description:
	//   Initializes the object.
	// Arguments:
	//   buffers - Array of transient constant buffers (usually provided by 
	//     Tr2EffectStateManager)
	//   constantTypeMask - Mask with pipeline stages that are expected by the shader
	//   renderContext - Current render context
	// ----------------------------------------------------------------------------------
	virtual void SetPerObjectDataToDevice( Tr2ConstantBufferAL** buffers, unsigned constantTypeMask, Tr2RenderContext& renderContext ) const
	{
		if( ( constantTypeMask & ( 1 << Tr2RenderContextEnum::VERTEX_SHADER ) ) && m_perObjectDataVs )
		{
			m_perObjectDataVs->SetPerObjectDataToDevice( *m_owner, Tr2RenderContextEnum::VERTEX_SHADER, buffers, renderContext );
		}
		if( SHADER_TYPE_EXISTS( Tr2RenderContextEnum::GEOMETRY_SHADER ) && 
			( constantTypeMask & ( 1 << Tr2RenderContextEnum::GEOMETRY_SHADER ) ) && m_perObjectDataVs )
		{
			m_perObjectDataVs->SetPerObjectDataToDevice( *m_owner, Tr2RenderContextEnum::GEOMETRY_SHADER, buffers, renderContext );
		}
		if( ( constantTypeMask & ( 1 << Tr2RenderContextEnum::PIXEL_SHADER ) ) && m_perObjectDataPs )
		{
			m_perObjectDataPs->SetPerObjectDataToDevice( *m_owner, Tr2RenderContextEnum::PIXEL_SHADER, buffers, renderContext );
		}
	}
private:
	PerObjectDataVs* m_perObjectDataVs;
	PerObjectDataPs* m_perObjectDataPs;
	Owner* m_owner;
};


#endif
