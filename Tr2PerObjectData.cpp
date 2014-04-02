#include "StdAfx.h"
#include "Tr2PerObjectData.h"

using namespace Tr2RenderContextEnum;


// --------------------------------------------------------------------------------------
// Description:
//   Set the perObject data to the device.
//   The function should use the provided buffer(s) to first map them and fill them up 
//   with per object data, _and_ then set the buffer to the renderContext at the correct 
//   register.
//   Default implementation calls UpdateConstantBuffer for each shader type.
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
	using namespace Tr2RenderContextEnum;
	for( unsigned i = SHADER_TYPE_FIRST; i < SHADER_TYPE_COUNT; ++i )
	{
		if( constantTypeMask & ( 1 << i ) )
		{
			UpdateConstantBuffer( ShaderType( i ), *buffers[i], UPDATE_CONTEXT, constantTypeMask, renderContext );
		}
	}
}


Tr2PerObjectDataStandard::Tr2PerObjectDataStandard()
	: m_vertexShaderFloatBufferSize( 0 )
{}

void Tr2PerObjectDataStandard::UpdateConstantBuffer(	Tr2RenderContextEnum::ShaderType type, 
														Tr2ConstantBufferAL& buffer, 
														UpdateDestination updateDestination,
														unsigned constantTypeMask,
														Tr2RenderContext& renderContext ) const
{
	CCP_STATS_ZONE( __FUNCTION__ );

	using namespace Tr2RenderContextEnum;
	if( type == VERTEX_SHADER && m_vertexShaderFloatBufferSize )
	{
		if( updateDestination == UPDATE_CONTEXT )
		{
			static const unsigned perFrameVsMask = 
				SHADER_TYPE_EXISTS( VERTEX_SHADER )		|
				SHADER_TYPE_EXISTS( COMPUTE_SHADER )	|
				SHADER_TYPE_EXISTS( GEOMETRY_SHADER )	|
				SHADER_TYPE_EXISTS( HULL_SHADER )		|
				SHADER_TYPE_EXISTS( DOMAIN_SHADER)		;
			FillAndSetConstants( buffer, 
											  m_vertexShaderFloatConstantBuffer, 
											  m_vertexShaderFloatBufferSize, 
											  perFrameVsMask & constantTypeMask,
											  Tr2Renderer::GetPerObjectVSStartRegister(), 
											  renderContext );
		}
		else if( void* mirror = buffer.GetBufferMirror( m_vertexShaderFloatBufferSize, renderContext ) )
		{
			memcpy( mirror, m_vertexShaderFloatConstantBuffer, m_vertexShaderFloatBufferSize );
		}
	}
	else if( type == PIXEL_SHADER && m_pixelShaderFloatBufferSize )
	{
		if( updateDestination == UPDATE_CONTEXT )
		{
			FillAndSetConstants( buffer, 
											  m_pixelShaderFloatConstantBuffer, 
											  m_pixelShaderFloatBufferSize, 
											  PIXEL_SHADER, 
											  Tr2Renderer::GetPerObjectPSStartRegister(), 
											  renderContext );
		}
		else if( void* mirror = buffer.GetBufferMirror( m_pixelShaderFloatBufferSize, renderContext ) )
		{
			memcpy( mirror, m_pixelShaderFloatConstantBuffer, m_pixelShaderFloatBufferSize );
		}
	}
}

Tr2PerObjectDataPrePass::Tr2PerObjectDataPrePass()
	: m_vertexShaderFloatBufferSize( 0 )
{}

void Tr2PerObjectDataPrePass::UpdateConstantBuffer(		Tr2RenderContextEnum::ShaderType type, 
														Tr2ConstantBufferAL& buffer, 
														UpdateDestination updateDestination,
														unsigned constantTypeMask,
														Tr2RenderContext& renderContext ) const
{
	CCP_STATS_ZONE( __FUNCTION__ );

	using namespace Tr2RenderContextEnum;
	if( type == VERTEX_SHADER && m_vertexShaderFloatBufferSize )
	{
		if( updateDestination == UPDATE_CONTEXT )
		{
			static const unsigned perFrameVsMask = 
				SHADER_TYPE_EXISTS( VERTEX_SHADER )		|
				SHADER_TYPE_EXISTS( COMPUTE_SHADER )	|
				SHADER_TYPE_EXISTS( GEOMETRY_SHADER )	|
				SHADER_TYPE_EXISTS( HULL_SHADER )		|
				SHADER_TYPE_EXISTS( DOMAIN_SHADER)		;
			FillAndSetConstants( buffer, 
											  m_vertexShaderFloatConstantBuffer, 
											  m_vertexShaderFloatBufferSize, 
											  perFrameVsMask & constantTypeMask,
											  Tr2Renderer::GetPerObjectVSStartRegister(), 
											  renderContext );
		}
		else if( void* mirror = buffer.GetBufferMirror( m_vertexShaderFloatBufferSize, renderContext ) )
		{
			memcpy( mirror, m_vertexShaderFloatConstantBuffer, m_vertexShaderFloatBufferSize );
		}
	}
}

void Tr2PerObjectDataSkinned::UpdateConstantBuffer( Tr2RenderContextEnum::ShaderType type, 
													Tr2ConstantBufferAL& buffer, 
													UpdateDestination updateDestination,
													unsigned constantTypeMask,
													Tr2RenderContext& renderContext ) const
{
	CCP_STATS_ZONE( __FUNCTION__ );

	using namespace Tr2RenderContextEnum;
	if( type == PIXEL_SHADER && m_pixelShaderFloatBufferSize )
	{
		if( updateDestination == UPDATE_CONTEXT )
		{
			FillAndSetConstants( buffer, 
											  m_pixelShaderFloatConstantBuffer, 
											  m_pixelShaderFloatBufferSize, 
											  PIXEL_SHADER, 
											  Tr2Renderer::GetPerObjectPSStartRegister(), 
											  renderContext );
		}
		else if( void* mirror = buffer.GetBufferMirror( m_pixelShaderFloatBufferSize, renderContext ) )
		{
			memcpy( mirror, m_pixelShaderFloatConstantBuffer, m_pixelShaderFloatBufferSize );
		}
	}
	else if( type == VERTEX_SHADER )
	{
		const unsigned totalSize = ( TR2_MAX_BONES_PER_MESHAREA * 3 + 5 + 4 ) * 16;
		if( char* VS = (char*)buffer.GetBufferMirror( totalSize, renderContext ) )
		{
			memcpy( VS + ( TR2_MAX_BONES_PER_MESHAREA * 3     ) * 16, &m_worldMat.m[0][0],     4 * 16 );
			memcpy( VS + ( TR2_MAX_BONES_PER_MESHAREA * 3 + 5 ) * 16, &m_mirrorMatrix.m[0][0], 4 * 16 );
		}
		if( updateDestination == UPDATE_CONTEXT )
		{
			buffer.UpdateFromMirror( renderContext );
			static const unsigned perFrameVsMask = 
				SHADER_TYPE_EXISTS( VERTEX_SHADER )		|
				SHADER_TYPE_EXISTS( COMPUTE_SHADER )	|
				SHADER_TYPE_EXISTS( GEOMETRY_SHADER )	|
				SHADER_TYPE_EXISTS( HULL_SHADER )		|
				SHADER_TYPE_EXISTS( DOMAIN_SHADER)		;
			SetConstants( buffer, perFrameVsMask, Tr2Renderer::GetPerObjectVSStartRegister(), renderContext );
		}
	}
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


void Tr2PerAreaDataSkinned::UpdateConstantBuffer(	Tr2RenderContextEnum::ShaderType type, 
													Tr2ConstantBufferAL& buffer, 
													UpdateDestination updateDestination,
													unsigned constantTypeMask,
													Tr2RenderContext& renderContext ) const
{
	CCP_STATS_ZONE( __FUNCTION__ );

	using namespace Tr2RenderContextEnum;
	if( type == VERTEX_SHADER )
	{
		m_perObjectDataPtr->UpdateConstantBuffer( type, buffer, UPDATE_MIRROR, constantTypeMask, renderContext );

		CCP_ASSERT( m_jointCount <= TR2_MAX_BONES_PER_MESHAREA );

		const unsigned totalSize = m_jointCount * 3 * 16;

		if( char* VS = (char*)buffer.GetBufferMirror( totalSize, renderContext ) )
		{
			memcpy( VS, &m_jointTransforms, totalSize );
		}	

		if( updateDestination == UPDATE_CONTEXT )
		{
			buffer.UpdateFromMirror( renderContext );
			static const unsigned perFrameVsMask = 
				SHADER_TYPE_EXISTS( VERTEX_SHADER )		|
				SHADER_TYPE_EXISTS( COMPUTE_SHADER )	|
				SHADER_TYPE_EXISTS( GEOMETRY_SHADER )	|
				SHADER_TYPE_EXISTS( HULL_SHADER )		|
				SHADER_TYPE_EXISTS( DOMAIN_SHADER)		;
			SetConstants( buffer, perFrameVsMask & constantTypeMask, Tr2Renderer::GetPerObjectVSStartRegister(), renderContext );
		}
	}
	else
	{
		m_perObjectDataPtr->UpdateConstantBuffer( type, buffer, updateDestination, constantTypeMask, renderContext );
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
