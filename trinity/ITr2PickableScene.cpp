// Copyright © 2023 CCP ehf.

// Precompiled header
#include "StdAfx.h"

// ITr2PickableScene header
#include "ITr2PickableScene.h"

// Trinity project includes
#include "ITr2Renderable.h"
#include "TriDevice.h"
#include "TriProjection.h"
#include "TriRenderBatch.h"
#include "TriView.h"
#include "Tr2PerObjectData.h"
#include "Tr2PickBuffer.h"
#include "Include/TriMath.h"
#include "Tr2Renderer.h"


// --------------------------------------------------------------------------------------
// Description:
//   Picks an object and its properties in the given pixel.
// Arguments:
//   renderContext - Current render context
//   x - X viewport coordinate of the pixel to perform picking in
//   y - Y viewport coordinate of the pixel to perform picking in
//   proj - Current projection transform
//   view - Current view transform
//   viewport - Current viewport
//   results - (in/out) Results of the picking operation. The results.componets field
//		as an input determines what properties of the object needs to be picked. On the
//		output the results.componets is modified to contain only properties that were
//		successfully picked (for scenes that might not have support for picking some
//		of the properties).
// --------------------------------------------------------------------------------------
void ITr2PickableScene::PickObject( Tr2RenderContext& renderContext, int x, int y, TriProjection* proj, TriView* view, TriViewport* viewport, PickResults& results )
{
	CCP_STATS_ZONE( __FUNCTION__ );

	// Backup current state
	Tr2Renderer::PushProjection();
	ON_BLOCK_EXIT( Tr2Renderer::PopProjection );
	Tr2Renderer::PushViewTransform();
	ON_BLOCK_EXIT( Tr2Renderer::PopViewTransform );
	renderContext.m_esm.PushViewport();
	ON_BLOCK_EXIT( [&] { renderContext.m_esm.PopViewport(); } );

	float fx, fy;
	Vector3 startWorld;
	Vector3 dirWorld;
	gTriDev->ScreenToProjection( x, y, &fx, &fy, viewport );

	results.object = NULL;

	if( !Tr2Renderer::GetPoolAllocator() )
	{
		results.components = 0;
		return;
	}

	// Get view and projection transforms
	Matrix projTransform;
	proj->GetMatrixWithoutViewAdjustment( projTransform );
	const Matrix& viewTransform = view->GetTransform();

	ConvertProjectionCoordToWorldPickRay( fx, fy, &projTransform, &viewTransform, &startWorld, &dirWorld );

	// Render for picking, limit our view to the pick ray
	SetupTransformsForPicking( fx, fy, proj, view, viewport );
	SetPerFrameDataForPicking( renderContext );

	const std::vector<ITr2Renderable*>& visibleObjects = GetPickingObjectsToRender( dirWorld );

	// Collect vector of objects to render
	std::vector<ITr2Renderable*> pickableObjects;
	for( std::vector<ITr2Renderable*>::const_iterator it = visibleObjects.begin(); it != visibleObjects.end(); ++it )
	{
		ITr2PickablePtr pickedObj( BlueCastPtr( *it ) );
		if( pickedObj )
		{
			pickableObjects.push_back( *it );
		}
	}

	PickComponents passes[MAX_PICK_PASSES];
	unsigned int count = GetRequiredPasses( results.components, passes );

	results.components = 0;

	if( !pickableObjects.empty() )
	{
		if( count == 0 )
		{
			return;
		}

		ITriRenderBatchAccumulator* pOpaquePickingBatches;
		ITriRenderBatchAccumulator* pPickingBatches;

		CR_RETURN( Tr2Renderer::BeginRenderContext() );
		ON_BLOCK_EXIT( Tr2Renderer::EndRenderContext );

		Tr2Renderer::SetWorldTransform( IdentityMatrix() );

		SetPerFrameDataForPicking( renderContext );

		GetBatches( pickableObjects, pOpaquePickingBatches, pPickingBatches );

		for( unsigned int i = 0; i < count; ++i )
		{
			if( RenderPicking( pOpaquePickingBatches, pPickingBatches, passes[i] ) )
			{
				Tr2PickBuffer& pickBuffer = GetPickBuffer();

				BufferResults buffer;

				const void* data;
				uint32_t pitch;
				if( pickBuffer.MapForReading( true, data, pitch, renderContext ) )
				{
					DecodeBufferPixel( data, passes[i], buffer );
					pickBuffer.UnmapForReading( renderContext );

					results.components |= passes[i];

					if( passes[i] & PICK_OBJECT )
					{
						if( buffer.objectId < pickableObjects.size() )
						{
							ITr2PickablePtr pickedObj( BlueCastPtr( pickableObjects[buffer.objectId] ) );
							results.object = pickedObj->GetID( buffer.areaId );
						}
						else
						{
							results.object = NULL;
						}
					}

					if( passes[i] & PICK_AREA )
					{
						results.area = buffer.areaId;
					}

					if( passes[i] & PICK_POSITION )
					{
						float dist = buffer.depth - Tr2Renderer::GetFrontClip();
						results.position = startWorld + ( dist * dirWorld );
					}

					if( passes[i] & PICK_UV )
					{
						results.uv = buffer.uv;
					}
				}
			}
		}

		pOpaquePickingBatches->Clear();
		if( pPickingBatches )
		{
			pPickingBatches->Clear();
		}
	}
}

// --------------------------------------------------------------------------------------
// Description:
//   Sets view and projection transform to be used for picking.
// Arguments:
//   fx - X offset of the frustum in post-projected space
//   fy - Y offset of the frustum in post-projected space
//   proj - Current projection transform
//   view - Current view transform
//   viewport - Current viewport
// --------------------------------------------------------------------------------------
void ITr2PickableScene::SetupTransformsForPicking( float fx, float fy, TriProjection* proj, TriView* view, TriViewport* viewport )
{
	Tr2Renderer::SetViewTransform( view->GetTransform() );
	proj->SetProjection();
	//
	// Projection is set up to scale the image such that the viewport is covered by one pixel.
	Vector2 scaling( float( viewport->width ), float( viewport->height ) );
	// translate the projection so that we center around the pick ray origin,
	// while remembering to scale this value as well:
	Vector2 translation;
	translation.x = -fx * scaling.x;
	translation.y = -fy * scaling.y;
	Tr2Renderer::AdjustProjection( scaling, translation );

	// We do NOT set the viewport here. It will get set automatically when SetRenderTarget is called.
}

// --------------------------------------------------------------------------------------
// Description:
//   Collects opaque and picking batches from given objects.
// Arguments:
//   pickableObjects - Vector of potentially visible objects for picking
//   pOpaquePickingBatches - (out) Opaque batch accumulator
//   pPickingBatches - (out) Picking batch accumulator
// --------------------------------------------------------------------------------------
void ITr2PickableScene::GetBatches( std::vector<ITr2Renderable*> const& pickableObjects,
									ITriRenderBatchAccumulator*& pOpaquePickingBatches,
									ITriRenderBatchAccumulator*& pPickingBatches )
{
	TriPoolAllocator* allocator = Tr2Renderer::GetPoolAllocator();
	CCP_ASSERT( allocator );

	// Get the picking batches from opaque areas to be overridden in the PS
	pOpaquePickingBatches = GetOpaquePickingBatchAccumulator();
	// Get the dedicated picking areas
	pPickingBatches = GetPickingBatchAccumulator();

	CCP_ASSERT( pOpaquePickingBatches && pOpaquePickingBatches->GetBatchCount() == 0 );
	if( !pOpaquePickingBatches )
	{
		return;
	}

	if( pPickingBatches )
	{
		CCP_ASSERT( pPickingBatches->GetBatchCount() == 0 );
	}

	for( unsigned int i = 0; i < pickableObjects.size(); ++i )
	{
		// We cannot rely on the object data to be up-to-date because this would assume that all
		// objects in the picked list were rendered on the previous frame and that is tooooo much of an assumption.
		// <halldor 2008-04-23>
		Tr2PerObjectData* perObjectData = pickableObjects[i]->GetPerObjectData( pOpaquePickingBatches );
		if( perObjectData )
		{
			perObjectData->SetUserData( i );

			size_t curBatchCount = pOpaquePickingBatches->GetBatchCount();
			pickableObjects[i]->GetBatches( pOpaquePickingBatches, TRIBATCHTYPE_OPAQUE, perObjectData );
			size_t newBatchCount = pOpaquePickingBatches->GetBatchCount();

			// Fall back to opaque prepass batches if there were no opaque batches
			if( curBatchCount == newBatchCount )
			{
				pickableObjects[i]->GetBatches( pOpaquePickingBatches, TRIBATCHTYPE_OPAQUE_PREPASS, perObjectData );
			}

			// Dedicated picking areas do not support UV picking (and it doesn't really make sense)
			if( pPickingBatches )
			{
				pickableObjects[i]->GetBatches( pPickingBatches, TRIBATCHTYPE_PICKING, perObjectData );
			}
		}
	}

	pOpaquePickingBatches->Finalize();

	if( pPickingBatches )
	{
		pPickingBatches->Finalize();
	}
}

// --------------------------------------------------------------------------------------
// Description:
//   Renders picking batches into the picking buffer.
// Arguments:
//   pOpaquePickingBatches - Opaque batch accumulator
//   pPickingBatches - Picking batch accumulator
//   pass  - union of PickComponent that are to be queried during this pass.
// Return value:
//   true If the rendering succeeded
//   false On error
// --------------------------------------------------------------------------------------
bool ITr2PickableScene::RenderPicking( ITriRenderBatchAccumulator* pOpaquePickingBatches,
									   ITriRenderBatchAccumulator* pPickingBatches,
									   PickComponents pass )
{
	USE_MAIN_THREAD_RENDER_CONTEXT(); //TODO

	// Use a shader variable for identifying which components are expected.
	// We can't use situations here because the flags might change during a single
	// frame.
	Vector4 componentsParam = Vector4(
		( pass & PICK_OBJECT ) ? 1.0f : 0.0f,
		( pass & PICK_AREA ) ? 1.0f : 0.0f,
		( pass & PICK_POSITION ) ? 1.0f : 0.0f,
		( pass & PICK_UV ) ? 1.0f : 0.0f );

	GlobalStore().RegisterVariable( "PickingComponents", componentsParam );

	float initialDepth = ( HUGE_NUMBER - Tr2Renderer::GetFrontClip() ) / ( Tr2Renderer::GetBackClip() - Tr2Renderer::GetFrontClip() );
	initialDepth = std::max( 0.0f, std::min( 1.0f, initialDepth ) );

	// Get the pick buffer
	Tr2PickBuffer& pickBuffer = GetPickBuffer();

	if( !pickBuffer.BeginRendering( initialDepth, renderContext ) )
	{
		return false;
	}

	if( pOpaquePickingBatches->GetBatchCount() )
	{
		pOpaquePickingBatches->Finalize();

		renderContext.m_esm.ApplyStandardStates( Tr2EffectStateManager::RM_PICKING );
		renderContext.RenderBatchesForPicking( pOpaquePickingBatches, BlueSharedString( "Picking" ) );
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

namespace
{
Be::VarChooser Tr2PickTypeChooser[] = {
	{ "PICK_TYPE_PICKING",
	  BeCast( PICK_TYPE_PICKING ),
	  "Authored picking areas" },
	{ "PICK_TYPE_OPAQUE",
	  BeCast( PICK_TYPE_OPAQUE ),
	  "Opaque objects" },
	{ "PICK_TYPE_TRANSPARENT",
	  BeCast( PICK_TYPE_TRANSPARENT ),
	  "Transparent objects" },
	{ "PICK_TYPE_ATTACHMENTS",
	  BeCast( PICK_TYPE_ATTACHMENTS ),
	  "Object attachments (decals, blinkies, etc.)" },
	{ "PICK_TYPE_LOCATORS",
	  BeCast( PICK_TYPE_LOCATORS ),
	  "Object locators" },
	{ 0 }
};
}

BLUE_REGISTER_ENUM_EX( "Tr2PickType", Tr2PickType, Tr2PickTypeChooser, ENUM_REG_ENUM_OBJECT_ON_MODULE );