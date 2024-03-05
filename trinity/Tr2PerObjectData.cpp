#include "StdAfx.h"
#include "Tr2PerObjectData.h"
#include "Tr2Renderer.h"


using namespace Tr2RenderContextEnum;


Tr2PerObjectData::~Tr2PerObjectData()
{
}

// --------------------------------------------------------------------------------------
// Description:
//   Set the perObject data to the device.
//   The function should use the provided buffer(s) to first map them and fill them up 
//   with per object data, _and_ then set the buffer to the renderContext at the correct 
//   register.
// Arguments:
//   buffers - points to an array of SHADER_TYPE_COUNT constant buffers; the elements in 
//             the array are pointers to make it easy to mix-and-match "global" buffers 
//             (that are shared) and buffers that specific objects hold on to because 
//             they know for sure that the data isn't changing (eg static placeables). 
//             Each pointer is guaranteed to be non-null.
//   renderContext - current render context
// --------------------------------------------------------------------------------------
void Tr2PerObjectData::SetPerObjectDataToDevice( Tr2ConstantBufferAL** buffers, 
												 unsigned constantTypeMask,
												 Tr2RenderContext& renderContext ) const
{
}


// cppcheck-suppress uninitMemberVar
Tr2PerObjectDataStandard::Tr2PerObjectDataStandard()
	: m_vertexShaderFloatBufferSize( 0 )
{
}

void Tr2PerObjectDataStandard::SetPerObjectDataToDevice( Tr2ConstantBufferAL** buffers, unsigned constantTypeMask, Tr2RenderContext& renderContext ) const
{
	const unsigned perFrameVsMask =
		SHADER_TYPE_EXISTS( VERTEX_SHADER ) |
		SHADER_TYPE_EXISTS( COMPUTE_SHADER ) |
		SHADER_TYPE_EXISTS( GEOMETRY_SHADER ) |
		SHADER_TYPE_EXISTS( HULL_SHADER ) |
		SHADER_TYPE_EXISTS( DOMAIN_SHADER );
	FillAndSetConstants( *buffers[VERTEX_SHADER],
		m_vertexShaderFloatConstantBuffer,
		m_vertexShaderFloatBufferSize,
		perFrameVsMask & constantTypeMask,
		Tr2Renderer::GetPerObjectVSStartRegister(),
		renderContext );
	FillAndSetConstants( *buffers[PIXEL_SHADER],
		m_pixelShaderFloatConstantBuffer,
		m_pixelShaderFloatBufferSize,
		PIXEL_SHADER,
		Tr2Renderer::GetPerObjectPSStartRegister(),
		renderContext );
}

void Tr2PerObjectDataSkinned::SetPerObjectDataToDevice( Tr2ConstantBufferAL** buffers, unsigned constantTypeMask, Tr2RenderContext& renderContext ) const
{
	if( ( constantTypeMask & ( 1 << PIXEL_SHADER ) ) != 0 )
	{
		FillAndSetConstants( *buffers[PIXEL_SHADER],
			m_pixelShaderFloatConstantBuffer,
			m_pixelShaderFloatBufferSize,
			PIXEL_SHADER,
			Tr2Renderer::GetPerObjectPSStartRegister(),
			renderContext );
	}
	const unsigned perFrameVsMask =
		SHADER_TYPE_EXISTS( VERTEX_SHADER ) |
		SHADER_TYPE_EXISTS( COMPUTE_SHADER ) |
		SHADER_TYPE_EXISTS( GEOMETRY_SHADER ) |
		SHADER_TYPE_EXISTS( HULL_SHADER ) |
		SHADER_TYPE_EXISTS( DOMAIN_SHADER );
	if( ( constantTypeMask & perFrameVsMask ) != 0 )
	{
		const uint32_t totalSize = ( TR2_MAX_BONES_PER_MESHAREA * 3 + 5 + 4 ) * 16;
		if( !buffers[VERTEX_SHADER]->IsValid() || buffers[VERTEX_SHADER]->GetSize() < totalSize )
		{
			buffers[VERTEX_SHADER]->Create( totalSize, renderContext.GetPrimaryRenderContext() );
		}
		char* VS = nullptr;
		if( SUCCEEDED( buffers[VERTEX_SHADER]->Lock( reinterpret_cast<void**>( &VS ), renderContext ) ) && VS )
		{
			memcpy( VS + ( TR2_MAX_BONES_PER_MESHAREA * 3 ) * 16, &m_worldMat.m[0][0], 4 * 16 );
			memcpy( VS + ( TR2_MAX_BONES_PER_MESHAREA * 3 + 5 ) * 16, &m_mirrorMatrix.m[0][0], 4 * 16 );
			buffers[VERTEX_SHADER]->Unlock( renderContext );
		}
		SetConstants( *buffers[VERTEX_SHADER], perFrameVsMask & constantTypeMask, Tr2Renderer::GetPerObjectVSStartRegister(), renderContext );
	}
}

void Tr2PerObjectDataSkinned::UpdateVertexShaderCBMirror( void* destination, Tr2RenderContext& renderContext ) const
{
	uint8_t* vs = static_cast<uint8_t*>( destination );
	memcpy( vs + ( TR2_MAX_BONES_PER_MESHAREA * 3 ) * 16, &m_worldMat.m[0][0], 4 * 16 );
	memcpy( vs + ( TR2_MAX_BONES_PER_MESHAREA * 3 + 5 ) * 16, &m_mirrorMatrix.m[0][0], 4 * 16 );
}

void Tr2PerObjectDataSkinned::SetSkinningMatrices( unsigned int n, float* data )
{
	m_jointCount = n;
	m_data = data;
}

float* Tr2PerObjectDataSkinned::GetSkinningMatrix( unsigned int ix ) const
{
	return &m_data[ix*3*4];
}

void Tr2PerAreaDataSkinned::SetPerObjectDataToDevice( Tr2ConstantBufferAL** buffers, unsigned constantTypeMask, Tr2RenderContext& renderContext ) const
{
	const unsigned perFrameVsMask =
		SHADER_TYPE_EXISTS( VERTEX_SHADER ) |
		SHADER_TYPE_EXISTS( COMPUTE_SHADER ) |
		SHADER_TYPE_EXISTS( GEOMETRY_SHADER ) |
		SHADER_TYPE_EXISTS( HULL_SHADER ) |
		SHADER_TYPE_EXISTS( DOMAIN_SHADER );
	if( ( constantTypeMask & perFrameVsMask ) != 0 )
	{
		auto& buffer = *buffers[VERTEX_SHADER];

		CCP_ASSERT( m_jointCount <= TR2_MAX_BONES_PER_MESHAREA );

		const unsigned totalSize = ( TR2_MAX_BONES_PER_MESHAREA * 3 + 5 + 4 ) * 16;
		const unsigned jointSize = m_jointCount * 3 * 16;

		if( !buffer.IsValid() || buffer.GetSize() < totalSize )
		{
			buffer.Create( totalSize, renderContext.GetPrimaryRenderContext() );
		}
		char* vs = nullptr;
		if( SUCCEEDED( buffer.Lock( reinterpret_cast<void**>( &vs ), renderContext ) ) && vs )
		{
			m_perObjectDataPtr->UpdateVertexShaderCBMirror( vs, renderContext );
			memcpy( vs, &m_jointTransforms, jointSize );
			buffer.Unlock( renderContext );
		}
		SetConstants( buffer, perFrameVsMask & constantTypeMask, Tr2Renderer::GetPerObjectVSStartRegister(), renderContext );
	}
	constantTypeMask = constantTypeMask & ~perFrameVsMask;
	if( constantTypeMask )
	{
		m_perObjectDataPtr->SetPerObjectDataToDevice( buffers, constantTypeMask, renderContext );
	}
}

void Tr2PerAreaDataSkinned::SetJointCount( unsigned int n )
{
	m_jointCount = n;

	CCP_ASSERT( m_jointCount <= TR2_MAX_BONES_PER_MESHAREA );

	if( m_jointCount > TR2_MAX_BONES_PER_MESHAREA )
	{
		m_jointCount = TR2_MAX_BONES_PER_MESHAREA;
	}
}

void Tr2PerAreaDataSkinned::SetJointTransform( unsigned int ix, float* data )
{
	CCP_ASSERT( ix < TR2_MAX_BONES_PER_MESHAREA );

	memcpy( &m_jointTransforms[ix*3*4], data, 3*4 * sizeof( float ) );
}
