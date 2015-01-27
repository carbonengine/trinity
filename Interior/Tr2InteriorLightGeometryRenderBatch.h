#pragma once
#ifndef Tr2InteriorLightGeometryRenderBatch_H
#define Tr2InteriorLightGeometryRenderBatch_H

// Trinity headers
#include "TriRenderBatch.h"
#include "Tr2ConstGeometry.h"

//---------------------------------------------------------------------------------------
// Description:
//   Render batch for light geometry used in light accumulation pass. 
//---------------------------------------------------------------------------------------
class Tr2InteriorLightGeometryRenderBatch : 
    public TriRenderBatch
{
public:
	Tr2InteriorLightGeometryRenderBatch()
		:m_vertexDecl( Tr2EffectStateManager::UNINITIALIZED_DECLARATION ),
		m_VB( nullptr ),
		m_IB( nullptr ),
		m_isInside( false )
	{
	}

	//---------------------------------------------------------------------------------------
	// Description:
	//   Initializes the batch. 
	//---------------------------------------------------------------------------------------
	void Initialize( Tr2VertexBufferAL* vb, Tr2IndexBufferAL* ib, unsigned vertexDeclaration, bool isInside )
	{
		m_VB = vb;
		m_IB = ib;
		m_vertexDecl = vertexDeclaration;
		m_isInside = isInside;
	}

    ///////////////////////////////////////////////////////////////////////////
    // TriRenderBatch
    virtual void SubmitGeometry( Tr2RenderContext& renderContext )
    {
		using namespace Tr2RenderContextEnum;

        renderContext.SetTopology( TOP_TRIANGLES );

		renderContext.m_esm.ApplyVertexDeclaration( m_vertexDecl );
		renderContext.m_esm.ApplyStreamSource( 0, *m_VB, 0, sizeof( Vector4 ) );
		if( m_isInside )
		{
			// cull front faces & disable depth test
			if( Tr2Renderer::IsRightHanded() != renderContext.m_esm.IsCullModeInverted() )
			{
				renderContext.SetRenderState( RS_CULLMODE, CULLMODE_CCW );
			}
			else
			{
				renderContext.SetRenderState( RS_CULLMODE, CULLMODE_CW );
			}
			renderContext.SetRenderState( RS_ZFUNC, CMP_GREATER );
		}
		else
		{
			// cull back faces
			if( Tr2Renderer::IsRightHanded() )
			{
				renderContext.SetRenderState( RS_CULLMODE, CULLMODE_CW );
			}
			else
			{
				renderContext.SetRenderState( RS_CULLMODE, CULLMODE_CCW );
			}
			renderContext.SetRenderState( RS_ZFUNC, CMP_LESSEQUAL );
		}

		if( m_IB && m_IB->IsValid() )
		{
			renderContext.m_esm.ApplyIndexBuffer( *m_IB );
			renderContext.DrawIndexedPrimitive(		m_VB->GetTotalSizeInBytes() / sizeof( Vector4 ), 
													0, 
													m_IB->GetNumIndices() / 3 );
		}
		else
		{
			renderContext.DrawPrimitive(	0, 
											m_VB->GetTotalSizeInBytes() / sizeof( Vector4 ) / 3 );
		}
    }
private:
	// Vertex declaration handle
	unsigned int m_vertexDecl;
	// Vertex buffer
	Tr2VertexBufferAL* m_VB;
	// Index buffer (might be nullptr)
	Tr2IndexBufferAL* m_IB;
	// Is the camera inside light volume (need to flip culling and Z test)?
	bool m_isInside;
};


//---------------------------------------------------------------------------------------
// Description:
//   Render batch for fullscreen light used in light accumulation pass. It also handles 
//   inverted cull mode for lights to render correctly in mirrors.
//---------------------------------------------------------------------------------------
class Tr2InteriorFullScreenLightBatch: public TriRenderBatch
{
public:
	//---------------------------------------------------------------------------------------
	// Description:
	//   Sets vertex declaration handle. 
	//---------------------------------------------------------------------------------------
	void SetVertexDeclaration( unsigned int vertexDecl )
	{
		m_vertexDecl = vertexDecl;
	}

    ///////////////////////////////////////////////////////////////////////////
    // TriRenderBatch
    virtual void SubmitGeometry( Tr2RenderContext& renderContext )
    {
		static const Vector4 vertexes[4] = {
			Vector4( -1.f, -1.f, 1.f, 1.f ),
			Vector4( -1.f, 1.f, 1.f, 1.f ),
			Vector4( 1.f, 1.f, 1.f, 1.f ),
			Vector4( 1.f, -1.f, 1.f, 1.f ),
		};

		using namespace Tr2RenderContextEnum;

        static const unsigned indexesRH[6] = {
			0, 2, 1,
			2, 0, 3,
		};
		static const unsigned indexesLH[6] = {
			2, 0, 1,
			0, 2, 3,
		};

		static Tr2VertexBufferAL* vb = Tr2ConstGeometry::GetVB( vertexes, sizeof( vertexes ) );
		static Tr2IndexBufferAL* ibRH = Tr2ConstGeometry::GetIB( IB_32BIT, indexesRH, 6 );
		static Tr2IndexBufferAL* ibLH = Tr2ConstGeometry::GetIB( IB_32BIT, indexesLH, 6 );

		renderContext.SetTopology( TOP_TRIANGLES );        

		renderContext.m_esm.ApplyVertexDeclaration( m_vertexDecl );
		renderContext.m_esm.ApplyStreamSource( 0, *vb, 0, sizeof( Vector4 ) );

		if( Tr2Renderer::IsRightHanded() != renderContext.m_esm.IsCullModeInverted() )
		{
			renderContext.m_esm.ApplyIndexBuffer( *ibRH );
		}
		else
		{
			renderContext.m_esm.ApplyIndexBuffer( *ibLH );
		}
		
		renderContext.DrawIndexedPrimitive( 4, 0, 2 );
    }

private:
	// Vertex declaration handle
	unsigned int m_vertexDecl;
};

#endif // Tr2InteriorLightGeometryRenderBatch_H
