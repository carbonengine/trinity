#include "StdAfx.h"
#include "Tr2RenderUtils.h"

using namespace Tr2RenderContextEnum;

#include "Tr2Renderer.h"
#include "TriViewport.h"

void SetupScreenQuad(	Tr2ScreenVertex quad[4], 
                        const Vector2& tlTexCoord, 
						const Vector2& brTexCoord,
                        const Vector2& tlVertexCoord, 
						const Vector2& brVertexCoord )
{
	const TriViewport& viewport = Tr2Renderer::GetViewport();

	// Note: I know the multipliers of 1.0 look spurious but they are here to underline an important
	// aspect of the following code (formula).  In D3D literature you will find the "half pixel offset"
	// trick that allows you map texels directly to pixels.  This is most important for texture compositing
	// and pixel-perfect UI drawing.  It so happens that the code below is working in clip-space, which
	// essentially maps the range [-1,1] to the viewport (pixels).  This means that we have the range of
	// 2 (in clip space) mapped to width and height (in pixels).  This means that one pixel horizontally
	// is 2.0/viewport-width-in-pixels units in clip space.  So a half pixel is 1.0/viewport-width but
	// NOT 0.5 as one might expect. <halldor 2009-09-03>
	const Vector2 offset( 1.f / (float)viewport.width, -1.f / (float)viewport.height );

	Vector2 tlScreen( tlVertexCoord[0] * 2.0f - 1.0f, -(tlVertexCoord[1] * 2.0f - 1.0f) );
	Vector2 brScreen( brVertexCoord[0] * 2.0f - 1.0f, -(brVertexCoord[1] * 2.0f - 1.0f) );
	const float z = 1.0f;
	const float w = 1.0f;

	int edge1 = 1;
	int edge2 = 2;

	USE_MAIN_THREAD_RENDER_CONTEXT();	//TODO! pass it in so we look at actual current cull mode
	if( Tr2Renderer::IsRightHanded() != renderContext.m_esm.IsCullModeInverted() )
	{
		// Flip the interior edge direction in the quad which produces a correctly 
		// oriented strip of two triangles
		edge1 = 2;
		edge2 = 1;
	}	

	// Top Left
	quad[0].p = Vector4( tlScreen.x - offset.x, tlScreen.y - offset.y, z, w );
	quad[0].t = tlTexCoord;

	// Top right
	quad[edge1].p = Vector4( brScreen.x - offset.x, tlScreen.y - offset.y, z, w );
	quad[edge1].t = Vector2( brTexCoord.x, tlTexCoord.y );

	// Bottom left
	quad[edge2].p = Vector4( tlScreen.x - offset.x, brScreen.y - offset.y, z, w );
	quad[edge2].t = Vector2( tlTexCoord.x, brTexCoord.y );

	// Bottom right
	quad[3].p = Vector4( brScreen.x - offset.x, brScreen.y - offset.y, z, w );
	quad[3].t = brTexCoord;
}

void SetupScreenQuadInCameraSpace( Tr2ScreenVertex quad[4], int width, int height )
{
	const TriViewport& viewport = Tr2Renderer::GetViewport();
	const Vector2 offset( 1.f / (float)viewport.width, -1.f / (float)viewport.height );
	
	// Top-left and bottom-right in projection space:
	// See explanation in SetupScreenQuad for pixel offsets
	Vector3 tl( -1.0f - offset.x,  1.0f - offset.y, 1.0f );
	Vector3 br(  1.0f - offset.x, -1.0f - offset.y, 1.0f );

	// Transform to view space:
	const Matrix& projectionRaw = Tr2Renderer::GetProjectionRawTransform();
	Matrix proj2view;
	D3DXMatrixInverse( &proj2view, NULL, &projectionRaw );
	D3DXVec3TransformCoord( &tl, &tl, &proj2view );
	D3DXVec3TransformCoord( &br, &br, &proj2view );
	
	int edge1 = 1;
	int edge2 = 2;

	USE_MAIN_THREAD_RENDER_CONTEXT();	//TODO! pass it in for cull mode
	if( Tr2Renderer::IsRightHanded() != renderContext.m_esm.IsCullModeInverted() )
	{
		// Flip the interior edge direction in the quad which produces a correctly 
		// oriented strip of two triangles
		edge1 = 2;
		edge2 = 1;
	}	

	// The vertex shader should ensure that the z coordinate is pushed to w
	Vector2 tlTC( 0.0f, 0.0f );
	Vector2 brTC( 1.0f, 1.0f );

	// Top Left
	quad[0].p = Vector4( tl.x, tl.y, tl.z, 1.0f );
	quad[0].t = tlTC;

	// Top right
	quad[edge1].p = Vector4( br.x, tl.y, tl.z, 1.0f );
	quad[edge1].t = Vector2( brTC.x, tlTC.y );

	// Bottom left
	quad[edge2].p = Vector4( tl.x, br.y, tl.z, 1.0f );
	quad[edge2].t = Vector2( tlTC.x, brTC.y );

	// Bottom right
	quad[3].p = Vector4( br.x, br.y, tl.z, 1.0f );
	quad[3].t = brTC;
}


bool FillAndSetConstants( Tr2ConstantBufferAL& buffer, const void* const data, const size_t dataSize, unsigned constantTypeMask, const unsigned registerIndex, Tr2RenderContext& renderContext )
{
	if( !buffer.IsValid() || dataSize > buffer.GetSize() )
	{
		USE_MAIN_THREAD_RENDER_CONTEXT();
		CR_RETURN_VAL( buffer.Create( (unsigned int)dataSize, renderContext ), false );
	}

	void* mapped = nullptr;
	if( !SUCCEEDED( buffer.Lock( &mapped, renderContext ) ) || !mapped )
	{
		return false;
	}

	memcpy( mapped, data, std::min( (uint32_t)dataSize, buffer.GetSize() ) );
	buffer.Unlock( renderContext );

	SetConstants( buffer, constantTypeMask, registerIndex, renderContext );

	return true;
}

void SetConstants( Tr2ConstantBufferAL& buffer, unsigned constantTypeMask, const unsigned registerIndex, Tr2RenderContext& renderContext )
{
	for( unsigned i = SHADER_TYPE_FIRST; i != SHADER_TYPE_COUNT && constantTypeMask; ++i )
	{
		if( constantTypeMask & ( 1 << i ) )
		{
			renderContext.SetConstants( buffer, static_cast<ShaderType>(i), registerIndex );
			constantTypeMask &= ~( 1 << i );
		}
	}
}
