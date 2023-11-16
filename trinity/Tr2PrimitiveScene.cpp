#include "StdAfx.h"

#include "Tr2PrimitiveScene.h"
#include "TriProjection.h"
#include "TriView.h"
#include "Shader/Tr2Effect.h"
#include "Tr2PrimitiveSet.h"
#include "Tr2PrimitiveText.h"
#include "TriFrustum.h"
#include "Tr2ManipulationTool.h"
#include "TriViewport.h"
#include "Tr2Renderer.h"


struct PerFrameVSData
{
	Matrix ViewInverseTransposeMat;
	Vector4 sunDirWorld;
	Vector4 sceneFogColor;
	Matrix ViewProjectionMat;
	Matrix ViewMat;
	Matrix ProjectionMat;
};

Tr2PrimitiveScene::Tr2PrimitiveScene( IRoot* lockobj ) :
	PARENTLOCK( m_primitives ),
	PARENTLOCK( m_textLabels ),
	m_pickBuffer( NULL, Tr2RenderContextEnum::PIXEL_FORMAT_R32G32B32A32_FLOAT, 1 )
{
	m_pickBuffer.PrepareResources();	
	m_allocator = CCP_NEW( "Tr2PrimitiveScene/m_allocator" ) TriPoolAllocator();    
	m_opaqueBatches = CCP_NEW( "Tr2PrimitiveScene/m_opaqueBatches" ) TriRenderBatchAccumulator<>( m_allocator );
    m_pickingBatches = CCP_NEW( "Tr2PrimitiveScene/m_pickingBatches" ) TriRenderBatchAccumulator<>( m_allocator );
}

Tr2PrimitiveScene::~Tr2PrimitiveScene()
{
	CCP_DELETE( m_pickingBatches );
	CCP_DELETE( m_opaqueBatches );
	CCP_DELETE( m_allocator );
	m_pickBuffer.ReleaseResources( TRISTORAGE_ALL );
}

// --------------------------------------------------------------------------------------
// Description:
//   Implements ITr2Scene::Render. Renders a list of primitive sets.
// --------------------------------------------------------------------------------------
void Tr2PrimitiveScene::Render( Tr2RenderContext& renderContext )
{
    std::vector<ITr2Renderable*> visibleRenderObjects;
    // We render everything since it is so cheap
	visibleRenderObjects.clear();		
	for( PrimitiveIterator it = m_primitives.begin(); it != m_primitives.end(); ++it )
	{
		(*it)->UpdateTransform();
		visibleRenderObjects.push_back((*it));
	}

	if( m_manipulator != NULL )
	{
		m_manipulator->Update();
		std::vector<ITr2Renderable*> manipPrims = m_manipulator->GetPrimitivesToRender();
		for( RenderableIterator it = manipPrims.begin(); it != manipPrims.end(); ++it )
		{			
			visibleRenderObjects.push_back((*it));
		}
	}

	// Sort the list before we render since the primitives might not write to z
	Tr2RenderableSortList sortList;
	for( RenderableIterator it = visibleRenderObjects.begin(); it != visibleRenderObjects.end(); ++it )
	{
		ITr2RenderableEntry entry;
		entry.m_object = (*it);
		entry.m_distance = (*it)->GetSortValue();
		sortList.push_back( entry );	
	}
	std::sort( sortList.begin(), sortList.end() );
	// sort the primitives back to front
	std::reverse( sortList.begin(), sortList.end() );

	SetupPerFrameData( renderContext );
	renderContext.m_esm.BeginManagedRendering();

	for( Tr2RenderableSortList::iterator it = sortList.begin(); 
		 it != sortList.end(); ++it )
	{		
		ITr2RenderablePtr obj = (*it).m_object;		
		Tr2PerObjectData* perObject = obj->GetPerObjectData( m_opaqueBatches );		
		obj->GetBatches( m_opaqueBatches, TRIBATCHTYPE_OPAQUE, perObject );		
	}

	m_opaqueBatches->Finalize();

	renderContext.m_esm.SetInvertedDepthTest( true );
	renderContext.m_esm.ApplyStandardStates( Tr2EffectStateManager::RM_OPAQUE );
	renderContext.RenderBatches( m_opaqueBatches );
	renderContext.m_esm.SetInvertedDepthTest( false );

	m_opaqueBatches->Clear();
	m_allocator->Clear();

	renderContext.m_esm.EndManagedRendering();

	for( auto it = m_textLabels.begin(); it != m_textLabels.end(); it++ )
	{
		(*it)->Render();
	}
	Tr2Renderer::RenderDebugInfo( renderContext );
}

// --------------------------------------------------------------------------------------
// Description:
//   Implements ITr2Scene::RenderDebugInfo. Nothing to do here.
// --------------------------------------------------------------------------------------
void Tr2PrimitiveScene::RenderDebugInfo( Tr2RenderContext& renderContext )
{
}

// --------------------------------------------------------------------------------------
// Description:
//   Implements ITr2Scene::Update. Nothing to do here.
// --------------------------------------------------------------------------------------
void Tr2PrimitiveScene::Update( Be::Time realTime, Be::Time simTime )
{
}

// --------------------------------------------------------------------------------------
// Description:
//   Implements Tr2DeviceResource. Destroys the constant buffer when needed.
// --------------------------------------------------------------------------------------
void Tr2PrimitiveScene::ReleaseResources( TriStorage s )
{
	if( s & TRISTORAGE_ALL )
	{
		m_vertexConstants = Tr2ConstantBufferAL();
	}
}

// --------------------------------------------------------------------------------------
// Description:
//   Implements Tr2DeviceResource. Does nothing.
// --------------------------------------------------------------------------------------
bool Tr2PrimitiveScene::OnPrepareResources()
{
	return true;
}

void Tr2PrimitiveScene::SetupTransformsForPicking( float fx, float fy, TriProjection* proj, TriView* view, TriViewport* viewport )
{
	Tr2Renderer::SetViewTransform( view->GetTransform() );
	proj->SetProjection();

	
	if( Tr2Renderer::GetCurrentProjectionType() == PT_ORTHOGONAL )
	{		
		fx *= (Tr2Renderer::GetOrthoWidth()/2.0f);
		fy *= (Tr2Renderer::GetOrthoHeight()/2.0f);
		float metersPerPixel = (Tr2Renderer::GetOrthoWidth()/viewport->width)/2.0f;	
		Tr2Renderer::SetOrthoProjection(fx-metersPerPixel, 
										fx+metersPerPixel, 
										fy-metersPerPixel, 
										fy+metersPerPixel, 
										Tr2Renderer::GetFrontClip(), 
										Tr2Renderer::GetBackClip());
	}
	else
	{
		//
		// Projection is set up to scale the image such that the viewport is covered by one pixel.
		Vector2 scaling( float(viewport->width), float(viewport->height) );
		// translate the projection so that we center around the pick ray origin,
		// while remembering to scale this value as well:
		Vector2 translation;
		translation.x = -fx*scaling.x;
		translation.y = -fy*scaling.y;
		Tr2Renderer::AdjustProjection( scaling, translation );
	}
}

void Tr2PrimitiveScene::SetupPerFrameData( Tr2RenderContext& renderContext )
{
	PerFrameVSData data;

	// 0
	memset( &data, 0, sizeof( PerFrameVSData ) );

	// column_major for shaders
	data.ViewMat = XMMatrixTranspose( Tr2Renderer::GetViewTransform() );

	Matrix proj = Tr2Renderer::GetReversedDepthProjectionTransform();

	data.ProjectionMat = XMMatrixTranspose( proj );
	data.ViewProjectionMat = XMMatrixTranspose( 
		XMMatrixMultiply( Tr2Renderer::GetViewTransform(), proj ) );

	FillAndSetConstants( m_vertexConstants, data, Tr2RenderContextEnum::VERTEX_SHADER, Tr2Renderer::GetPerFrameVSStartRegister(), renderContext );
}

void Tr2PrimitiveScene::SetPerFrameDataForPicking( Tr2RenderContext& renderContext )
{
	SetupPerFrameData( renderContext );
}

bool Tr2PrimitiveScene::RenderPicking( 
	ITriRenderBatchAccumulator* pOpaquePickingBatches,
	ITriRenderBatchAccumulator* pPickingBatches,
	PickComponents pass )
{
	USE_MAIN_THREAD_RENDER_CONTEXT();
	renderContext.m_esm.SetInvertedDepthTest( true );
	ON_BLOCK_EXIT( [&] { renderContext.m_esm.SetInvertedDepthTest( false ); } );

	// Use a shader variable for identifying which components are expected.
	// We can't use situations here because the flags might change during a single
	// frame.
	Vector4 componentsParam = Vector4( 
		( pass & PICK_OBJECT ) ? 1.0f : 0.0f, 
		( pass & PICK_AREA ) ? 1.0f : 0.0f, 
		( pass & PICK_POSITION ) ? 1.0f : 0.0f, 
		( pass & PICK_UV ) ? 1.0f : 0.0f );

	GlobalStore().RegisterVariable( "PickingComponents", componentsParam );

    // Get the pick buffer
    Tr2PickBuffer& pickBuffer = GetPickBuffer();

    if( !pickBuffer.BeginRendering( 0, renderContext ) )
    {
        return false;
    }

    if( pPickingBatches && RenderPickingAreasForComponents( pass ) )
    {
        pPickingBatches->Finalize();

        renderContext.m_esm.ApplyStandardStates( Tr2EffectStateManager::RM_PICKING );
		renderContext.RenderBatchesForPicking( pPickingBatches, DEFAULT_TECHNIQUE );
    }

    if( !pickBuffer.EndRendering( renderContext ) )
    {
        return false;
    }

    return true;
}

const std::vector<ITr2Renderable*>& Tr2PrimitiveScene::GetPickingObjectsToRender( const Vector3& dirWorld )
{
	m_pickingObjects.clear();
	for( auto it = m_primitives.begin(); it != m_primitives.end(); ++it )
	{		
		((Tr2PrimitiveSet*)(*it))->UpdateTransform();
		m_pickingObjects.push_back((*it));
	}
    if (m_manipulator != NULL)
    {
        m_manipulator->Update();
        std::vector<ITr2Renderable*> manipPrims = m_manipulator->GetPrimitivesToRender();
        for (RenderableIterator it = manipPrims.begin(); it != manipPrims.end(); ++it)
        {
            m_pickingObjects.push_back((*it));
        }
    }

	return m_pickingObjects;
}

const std::vector<ITr2Renderable*>& Tr2PrimitiveScene::GetPickingObjectsToRender( const Vector3& dirWorld, float fov, float aspect )
{
	return GetPickingObjectsToRender( dirWorld );
}

// We always render opaque batches for picking
ITriRenderBatchAccumulator* Tr2PrimitiveScene::GetOpaquePickingBatchAccumulator( void )
{
	return m_opaqueBatches;
}

// If you need special behaviour for picking, these batches are rendered without a picking override
ITriRenderBatchAccumulator* Tr2PrimitiveScene::GetPickingBatchAccumulator( void )
{
	return m_pickingBatches;
}

// -------------------------------------------------------------
// Description:
//   Returns an array of passes that need to be rendered in order
//   to get all picking components. 
// Arguments:
//   requestedComponents  - Components requested for picking operation
//                          (union of PickComponent).
//   passes - (out) Array components actually rendered during each
//            picking pass (union of PickComponent). Maximum number
//            of passes is MAX_PICK_PASSES.
// Return Value:
//   Number of passes required to query all requested picking
//   components.
// -------------------------------------------------------------
unsigned int Tr2PrimitiveScene::GetRequiredPasses( PickComponents requestedComponents, PickComponents* passes )
{
	unsigned count = 0;
	if( requestedComponents & PICK_OBJECT || requestedComponents & PICK_AREA )
	{
		passes[0] = requestedComponents & ( PICK_OBJECT | PICK_AREA );
		++count;
	}
	if( requestedComponents & PICK_UV )
	{
		passes[count] = PICK_UV;
		++count;
	}
	if( requestedComponents & PICK_POSITION )
	{
		if( count )
		{
			passes[0] |= PICK_POSITION;
		}
		else
		{
			passes[0] = PICK_POSITION;
			count = 1;
		}
	}
	return count;
}

void Tr2PrimitiveScene::DecodeBufferPixel( const void* pBuffer, PickComponents pass, BufferResults& results ) const
{
	// helpers: get each channel
	float a = *((float*)pBuffer + 3);
	float r = *((float*)pBuffer + 2);
	float g = *((float*)pBuffer + 1);
	float b = *((float*)pBuffer + 0);
	if( pass & PICK_UV )
	{
		results.uv.x = b;
		results.uv.y = g;
		results.depth = r;
	}
	else
	{
		// put it "together"
		results.objectId = (unsigned short)g;
		results.objectId--;
		results.depth = r;
		// If alpha values are set to 1.0. We did not pick anything
		if ( a == 1.0f )
		{
			results.objectId = -1;
		}
	}
}

Tr2PickBuffer& Tr2PrimitiveScene::GetPickBuffer( void )
{
	return m_pickBuffer;
}
