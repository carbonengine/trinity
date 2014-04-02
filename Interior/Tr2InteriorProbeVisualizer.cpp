#include "StdAfx.h"

#if INTERIORS_ENABLED

#include "Tr2InteriorProbeVisualizer.h"
#include "Tr2Renderer.h"
#include "Tr2Effect.h"

using namespace Tr2RenderContextEnum;

struct Tr2InteriorProbeVisualizer::Vertex
{
    Vector3 m_position;
    Vector3 m_normal;
};

struct Tr2InteriorProbeVisualizer::PerObjectVS
{
	Matrix viewProjection;
	Vector4 position;
};

struct Tr2InteriorProbeVisualizer::PerObjectPS
{
	Matrix redMat;
	Matrix greenMat;
	Matrix blueMat;
	Vector4 lightingFactor;
};

// ------------------------------------------------------------------------------------------------------
Tr2InteriorProbeVisualizer::Tr2InteriorProbeVisualizer( IRoot* lockobj )
{
	USE_MAIN_THREAD_RENDER_CONTEXT();


	Tr2VertexDefinition vd;
	vd.Add( vd.FLOAT32_3, vd.POSITION );
	vd.Add( vd.FLOAT32_3, vd.NORMAL );

	m_vertexDeclaration = Tr2EffectStateManager::GetVertexDeclarationHandle( vd );

	const int segments = 8;

	m_bytesPerVertex = sizeof( Vertex );
	m_vertexCount = 6 * segments * segments * 2;

	CR( m_vertexBuffer.Create( m_vertexCount * m_bytesPerVertex, USAGE_CPU_WRITE, nullptr, renderContext ) );

    Vertex* pVerts;
	CR( m_vertexBuffer.Lock( pVerts, LOCK_WRITEONLY, renderContext ) );
	if( !pVerts )
	{
		return;
	}

#define VERTEX( x, y ) pVerts->m_position = pVerts->m_normal = Vector3( sin( x ) * cos( y ), cos( x ) * cos( y ), sin( y ) ); ++pVerts;

	for( int i = 0; i < segments * 2; i++ )
	{
		float x0 = float(i) / float(segments * 2 - 1) * D3DX_PI * 2;
		float x1 = float(i+1) / float(segments * 2 - 1) * D3DX_PI * 2;
		for( int j = 0; j < segments; j++ )
		{
			float y0 = float(j) / float(segments-1) * D3DX_PI - D3DX_PI * 0.5f;
			float y1 = float(j+1) / float(segments-1) * D3DX_PI - D3DX_PI * 0.5f;

			VERTEX( x0, y0 );
			VERTEX( x1, y0 );
			VERTEX( x0, y1 );

			VERTEX( x0, y1 );
			VERTEX( x1, y0 );
			VERTEX( x1, y1 );
		}
	}

#undef VERTEX

	m_vertexBuffer.Unlock( renderContext );

	m_effect.CreateInstance();
	m_effect->SetEffectPathName( "res:/Graphics/Effect/Managed/Debug/SHProbeDebug.fx" );
}

// ------------------------------------------------------------------------------------------------------
Tr2InteriorProbeVisualizer::~Tr2InteriorProbeVisualizer()
{
}

// ------------------------------------------------------------------------------------------------------
void Tr2InteriorProbeVisualizer::Render( const Vector3& position, const Matrix& redMat, const Matrix& greenMat, const Matrix& blueMat, float brightness )
{
    if( !m_vertexBuffer.IsValid() )
    {
        return;
    }

	USE_MAIN_THREAD_RENDER_CONTEXT();

	renderContext.m_esm.ApplyStreamSource( 0, m_vertexBuffer, 0, sizeof( Vertex ) );
	renderContext.m_esm.ApplyVertexDeclaration( m_vertexDeclaration );

	Matrix view = Tr2Renderer::GetViewTransform();
	Matrix proj = Tr2Renderer::GetProjectionTransform();

	PerObjectVS perObjectVS;
	perObjectVS.viewProjection = view * proj;
	D3DXMatrixTranspose( &perObjectVS.viewProjection, &perObjectVS.viewProjection );
	perObjectVS.position = Vector4( position, 0.05f);
	FillAndSetConstants( m_VSBuffer, perObjectVS, VERTEX_SHADER, Tr2Renderer::GetPerObjectVSStartRegister(), renderContext );

	PerObjectPS perObjectPS;
	perObjectPS.redMat = redMat;
	perObjectPS.greenMat = greenMat;
	perObjectPS.blueMat = blueMat;
	perObjectPS.lightingFactor = Vector4( brightness, brightness, brightness, brightness );
	FillAndSetConstants( m_PSBuffer, perObjectPS, PIXEL_SHADER, Tr2Renderer::GetPerObjectPSStartRegister(), renderContext );

	m_effect->Render( this, renderContext );
}

// ------------------------------------------------------------------------------------------------------
void Tr2InteriorProbeVisualizer::SubmitGeometry( Tr2RenderContext& renderContext )
{
	renderContext.SetTopology( TOP_TRIANGLES );
    renderContext.DrawPrimitive( 0, m_vertexCount / 3 );
}

#endif
