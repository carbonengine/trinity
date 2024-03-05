#pragma once

#ifndef Tr2PerObjectData_H
#define Tr2PerObjectData_H


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

class Tr2PerObjectDataStandard : public Tr2PerObjectDataPSBuffer
{
public:

	Tr2PerObjectDataStandard();

	virtual void SetPerObjectDataToDevice( Tr2ConstantBufferAL** buffers, unsigned constantTypeMask, Tr2RenderContext& renderContext ) const;

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
		m_worldMat( IdentityMatrix() ),
		m_mirrorMatrix( IdentityMatrix() ),
		m_worldPos( 0.0f, 0.0f, 0.0f, 0.0f )
	{
	}

	virtual void SetPerObjectDataToDevice( Tr2ConstantBufferAL** buffers, unsigned constantTypeMask, Tr2RenderContext& renderContext ) const;
	
	void SetSkinningMatrices( unsigned int n, float* data );
	float* GetSkinningMatrices() const { return m_data; }
	float* GetSkinningMatrix( unsigned int ix ) const;
	void SetWorldMatrix( const Matrix& worldMat ) { m_worldMat = worldMat; }
	void SetMirrorMatrix( const Matrix& mirrorMat ) { m_mirrorMatrix = mirrorMat; }
	const Matrix& GetMirrorMatrix() const { return m_mirrorMatrix; }
	void SetWorldPosition( const Vector3& worldPos ) { m_worldPos = Vector4( worldPos.x, worldPos.y, worldPos.z, 0.0f ); }

	void UpdateVertexShaderCBMirror( void* destination, Tr2RenderContext& renderContext ) const;

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

	virtual void SetPerObjectDataToDevice( Tr2ConstantBufferAL** buffers, unsigned constantTypeMask, Tr2RenderContext& renderContext ) const;

	void SetJointCount( unsigned int n );
	void SetJointTransform( unsigned int ix, float* data );
	void SetPerObjectData( const Tr2PerObjectDataSkinned& perObjectData ) { m_perObjectDataPtr = &perObjectData; };
	
	const unsigned	GetJointCount() const { return m_jointCount; }
	const float*	GetMatrices()	const { return (const float*)m_jointTransforms; }
	
private:
	unsigned int m_jointCount;
	float m_jointTransforms[TR2_MAX_BONES_PER_MESHAREA * ( 3 * 4 )];
	const Tr2PerObjectDataSkinned* m_perObjectDataPtr;
};


#endif