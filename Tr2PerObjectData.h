#pragma once

#ifndef Tr2PerObjectData_H
#define Tr2PerObjectData_H

#include "Tr2Renderer.h"

class Tr2PerObjectData
{
public:
	Tr2PerObjectData()
		:m_userData( 0 )
	{
	}

	virtual ~Tr2PerObjectData() = 0;

	virtual void SetPerObjectDataToDevice( Tr2ConstantBufferAL** buffers, unsigned constantTypeMask, Tr2RenderContext& renderContext ) const;

	unsigned int GetUserData() const { return m_userData; }
	void SetUserData(unsigned int val) { m_userData = val; }

	// --------------------------------------------------------------------------------------
	// Description:
	//   Type of update for UpdateConstantBuffer method.
	// --------------------------------------------------------------------------------------
	enum UpdateDestination
	{
		// Update constant buffer mirror
		UPDATE_MIRROR,
		// Update constant buffer and commit it to render context
		UPDATE_CONTEXT,
	};

	// --------------------------------------------------------------------------------------
	// Description:
	//   Updates constant buffer with per-object data for specified input stage (shader 
	//   type). Based on updateDestination parameter the function either updates buffer's
	//   mirror or commits data to the render context. This function is used for nested (for
	//   example per-area) data.
	//   Default implementation is empty, so derived classes are supposed to fully override
	//   either SetPerObjectDataToDevice or UpdateConstantBuffer (or both) methods.
	// Arguments:
	//   type - stage input type
	//   buffer - constant buffer to update
	//   updateDestination - what to update
	//   constantTypeMask - bitmask telling us which shaderTypes are actually in use by the current effect
	//   renderContext - current render context
	// --------------------------------------------------------------------------------------
	virtual void UpdateConstantBuffer( Tr2RenderContextEnum::ShaderType type, 
									   Tr2ConstantBufferAL& buffer, 
									   UpdateDestination updateDestination,
									   unsigned constantTypeMask, 
									   Tr2RenderContext& renderContext ) const 
	{
	}

private:
	unsigned int m_userData;
};

// -------------------------------------------------------------
// Description:
//   Tr2PerObjectDataPSBuffer maintains a buffer of PS registers.
//   It is a common functionality of Tr2PerObjectDataStandard,
//   Tr2PerAreaSHLightingData and Tr2PerObjectDataSkinned.
// -------------------------------------------------------------
class Tr2PerObjectDataPSBuffer : public Tr2PerObjectData
{
public:
	// cppcheck-suppress uninitMemberVar
	Tr2PerObjectDataPSBuffer()
		: m_pixelShaderFloatBufferSize( 0 )
	{
		memset( m_pixelShaderFloatConstantBuffer, 0, sizeof( m_pixelShaderFloatConstantBuffer ) );
	}
	
	template<typename T> void CopyToPSFloatBuffer( const T& objectRef )
	{
		// In-place buffer must be large enough to contain the data in the type being set
		static_assert( sizeof(T) <= sizeof(m_pixelShaderFloatConstantBuffer), "Size of per-object data is too large" );
		// Size of the data being set must fill registers (float4s)
		static_assert( sizeof(T) % (sizeof(float)*4) == 0, "Size of per-object data must be a multiple of Vector4" );
		// Object is a pointer ( please resolve possible NULL pointers yourself )
		static_assert( (!std::is_pointer<T>::value), "Need to pass reference to the data" );

		m_pixelShaderFloatBufferSize = sizeof(T);
		memcpy( &m_pixelShaderFloatConstantBuffer[0], (float*)&objectRef, sizeof(T) );
	}

protected:
	float m_pixelShaderFloatConstantBuffer[80*4];
	unsigned int m_pixelShaderFloatBufferSize;
};

// -------------------------------------------------------------
// Description:
//   Tr2PerAreaSHLightingData overrides some PS registers of
//   the original per-object data. It must be used in conjunction
//   with some other per-object data that is set with
//   SetPerObjectData method.
// -------------------------------------------------------------
class Tr2PerAreaSHLightingData : public Tr2PerObjectDataPSBuffer
{
public:
	Tr2PerAreaSHLightingData()
		:m_perObjectDataPtr( nullptr )
	{
	}

	virtual void UpdateConstantBuffer( Tr2RenderContextEnum::ShaderType type, 
									   Tr2ConstantBufferAL& buffer, 
									   UpdateDestination updateDestination,
									   unsigned constantTypeMask, 
									   Tr2RenderContext& renderContext ) const
	{
		CCP_STATS_ZONE( __FUNCTION__ );

		using namespace Tr2RenderContextEnum;
		if( type == PIXEL_SHADER )
		{
			m_perObjectDataPtr->UpdateConstantBuffer( type, buffer, UPDATE_MIRROR, constantTypeMask, renderContext );

			void* mirror = buffer.GetBufferMirror( m_pixelShaderFloatBufferSize, renderContext );
			if( mirror )
			{
				memcpy( mirror, m_pixelShaderFloatConstantBuffer, m_pixelShaderFloatBufferSize );
			}

			if( updateDestination == UPDATE_CONTEXT )
			{
				buffer.UpdateFromMirror( renderContext );
				renderContext.SetConstants( buffer, type, Tr2Renderer::GetPerObjectPSStartRegister() );
			}
		}
		else
		{
			m_perObjectDataPtr->UpdateConstantBuffer( type, buffer, updateDestination, constantTypeMask, renderContext );
		}
	}

	void SetPerObjectData( const Tr2PerObjectData* perObjectData ) 
	{ 
		m_perObjectDataPtr = perObjectData; 
	}

	const Tr2PerObjectData* GetPerObjectData() const
	{ 
		return m_perObjectDataPtr; 
	}
private:
	const Tr2PerObjectData* m_perObjectDataPtr;
};

class Tr2PerObjectDataStandard : public Tr2PerObjectDataPSBuffer
{
public:

	Tr2PerObjectDataStandard();

	virtual void UpdateConstantBuffer( Tr2RenderContextEnum::ShaderType type, 
									   Tr2ConstantBufferAL& buffer, 
									   UpdateDestination updateDestination,
									   unsigned constantTypeMask, 
									   Tr2RenderContext& renderContext ) const;

	template<typename T> void CopyToVSFloatBuffer( const T& objectRef )
	{
		// In-place buffer must be large enough to contain the data in the type being set
		static_assert( sizeof(T) <= sizeof(m_vertexShaderFloatConstantBuffer), "Size of per-object data is too large" );
		// Size of the data being set must fill registers (float4s)
		static_assert( sizeof(T) % (sizeof(float)*4) == 0, "Size of per-object data must be a multiple of Vector4" );
		// Object is a pointer ( please resolve possible NULL pointers yourself )
		static_assert( (!std::is_pointer<T>::value), "Need to pass reference to the data" );

		m_vertexShaderFloatBufferSize = sizeof(T);
		memcpy( &m_vertexShaderFloatConstantBuffer[0], (float*)&objectRef, sizeof(T) );
	}

protected:
	float m_vertexShaderFloatConstantBuffer[40*4];
	unsigned int m_vertexShaderFloatBufferSize;
};

class Tr2PerObjectDataPrePass : public Tr2PerObjectData
{
public:
	Tr2PerObjectDataPrePass();

	virtual void UpdateConstantBuffer( Tr2RenderContextEnum::ShaderType type, 
									   Tr2ConstantBufferAL& buffer, 
									   UpdateDestination updateDestination,
									   unsigned constantTypeMask, 
									   Tr2RenderContext& renderContext ) const;

	template<typename T> void CopyToVSFloatBuffer( const T& objectRef )
	{
		// In-place buffer must be large enough to contain the data in the type being set
		static_assert( sizeof(T) <= sizeof(m_vertexShaderFloatConstantBuffer), "Size of per-object data is too large" );
		// Size of the data being set must fill registers (float4s)
		static_assert( sizeof(T) % (sizeof(float)*4) == 0, "Size of per-object data must be a multiple of Vector4" );
		// Object is a pointer ( please resolve possible NULL pointers yourself )
		static_assert( (!std::is_pointer<T>::value), "Need to pass reference to the data" );

		m_vertexShaderFloatBufferSize = sizeof(T);
		memcpy( &m_vertexShaderFloatConstantBuffer[0], (float*)&objectRef, sizeof(T) );
	}

protected:
	float m_vertexShaderFloatConstantBuffer[40*4];
	unsigned int m_vertexShaderFloatBufferSize;
};

#define TR2_MAX_BONES_PER_MESHAREA (65)

class Tr2PerObjectDataSkinned : public Tr2PerObjectDataPSBuffer
{
public:
	Tr2PerObjectDataSkinned()
		:m_jointCount( 0 ),
		m_data( nullptr ),
		m_worldMat( Tr2Renderer::GetIdentityTransform() ),
		m_mirrorMatrix( Tr2Renderer::GetIdentityTransform() ),
		m_worldPos( 0.0f, 0.0f, 0.0f, 0.0f )
	{
	}

	virtual void UpdateConstantBuffer( Tr2RenderContextEnum::ShaderType type, 
									   Tr2ConstantBufferAL& buffer, 
									   UpdateDestination updateDestination,
									   unsigned constantTypeMask, 
									   Tr2RenderContext& renderContext ) const;
	
	void SetSkinningMatrices( unsigned int n, float* data );
	float* GetSkinningMatrices() const { return m_data; }
	float* GetSkinningMatrix( unsigned int ix ) const;
	void SetWorldMatrix( const Matrix& worldMat ) { m_worldMat = worldMat; }
	void SetMirrorMatrix( const Matrix& mirrorMat ) { m_mirrorMatrix = mirrorMat; }
	const Matrix& GetMirrorMatrix() const { return m_mirrorMatrix; }
	void SetWorldPosition( const Vector3& worldPos ) { m_worldPos = Vector4( worldPos.x, worldPos.y, worldPos.z, 0.0f ); }

private:
	unsigned int m_jointCount;
	float* m_data;

protected:
	Matrix m_worldMat;
	Matrix m_mirrorMatrix;
	Vector4 m_worldPos;
};

class Tr2PerAreaDataSkinned : public Tr2PerObjectData
{
public:

	// cppcheck-suppress uninitMemberVar
	Tr2PerAreaDataSkinned() : 
		m_jointCount( 0 ),
		m_perObjectDataPtr( NULL )
	{
		memset( m_jointTransforms, 0, sizeof( m_jointTransforms ) );
	}

	virtual void UpdateConstantBuffer( Tr2RenderContextEnum::ShaderType type, 
									   Tr2ConstantBufferAL& buffer, 
									   UpdateDestination updateDestination,
									   unsigned constantTypeMask, 
									   Tr2RenderContext& renderContext ) const;

	void SetJointCount( unsigned int n );
	void SetJointTransform( unsigned int ix, float* data );
	void SetPerObjectData( const Tr2PerObjectData& perObjectData ) { m_perObjectDataPtr = &perObjectData; };
	
	const unsigned	GetJointCount() const { return m_jointCount; }
	const float*	GetMatrices()	const { return (const float*)m_jointTransforms; }
	
private:
	unsigned int m_jointCount;
	float m_jointTransforms[TR2_MAX_BONES_PER_MESHAREA * ( 3 * 4 )];
	const Tr2PerObjectData* m_perObjectDataPtr;
};


#endif