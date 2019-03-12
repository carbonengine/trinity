////////////////////////////////////////////////////////////
//
//    Created:   June 2010
//    Copyright: CCP 2010
//
#include "StdAfx.h"
#include "EveOccluder.h"

#include "Shader/Tr2Effect.h"

#include "EveUpdateContext.h"
#include "EveTransform.h"
#include "include/TriMath.h"


// --------------------------------------------------------------------------------
// Description:
//   Initialize data members, create a batch accumulator, load a shader and
//   register this class as a device resource, so we can handle device resets
//   for the dx objects
// --------------------------------------------------------------------------------
EveOccluder::EveOccluder( IRoot* lockobj ) :
	PARENTLOCK( m_sprites ),
	m_display( true ),
	m_totalNumOfPixels( 1 ),
	m_actualNumOfPixels( 1 ),
	m_value( 1.f ),
	m_isTotalQueryIssued( false ),
	m_isActualQueryIssued( false )
{
	// create batch accumulator for rendering occlusion sprites
	TriPoolAllocator* allocator = Tr2Renderer::GetPoolAllocator();
	m_batches = CCP_NEW( "EveOccluder/m_batches" ) TriRenderBatchAccumulator<EffectKeyGenerator>( allocator );

	PrepareResources();
}

// --------------------------------------------------------------------------------
// Description:
//   Cleanup
// --------------------------------------------------------------------------------
EveOccluder::~EveOccluder()
{
	CCP_DELETE m_batches;

	ReleaseResources( TRISTORAGE_ALL );
}

// --------------------------------------------------------------------------------
// Description:
//   Release all d3d query objects.
// Arguments:
//   s - type of reset
// --------------------------------------------------------------------------------
void EveOccluder::ReleaseResources( TriStorage s )
{
	m_isTotalQueryIssued = false;
	m_isActualQueryIssued = false;

	m_totalQuery.Destroy();
	m_actualQuery.Destroy();
}

// --------------------------------------------------------------------------------
// Description:
//   create two d3d query objects: need two to calculate a ratio
// --------------------------------------------------------------------------------
bool EveOccluder::OnPrepareResources()
{
	USE_MAIN_THREAD_RENDER_CONTEXT();
	// create the dx query objects
	if( !m_totalQuery.IsValid() )
	{
		CR_RETURN_VAL( m_totalQuery.Create( renderContext ), false );
		m_isTotalQueryIssued = false;
	}

	if( !m_actualQuery.IsValid() )
	{
		CR_RETURN_VAL( m_actualQuery.Create( renderContext ), false );
		m_isActualQueryIssued = false;
	}

	return true;
}

// --------------------------------------------------------------------------------
// Description:
//   just return the ratio value
// --------------------------------------------------------------------------------
float EveOccluder::GetValue() const
{
	return m_value;
}

// --------------------------------------------------------------------------------
// Description:
//   Performs two render calls with different states: with and without depth-test
//   to get the visibility. WARNING: a query might not return a value immediatly,
//   if it still "runs", don't do anything and wait until it finishes
// Arguments:
//   renderContext - current render context
//   frustum - the current view frustum of the current frame
//   transform - parent transform of the lensflare to turn this into a 2d sprite
// --------------------------------------------------------------------------------
void EveOccluder::RunQuery( Tr2RenderContext& renderContext, const TriFrustum& frustum, const Matrix& transform )
{
	// visible?
	if( !m_display )
	{
		return;
	}

	if( !m_totalQuery.IsValid() || !m_actualQuery.IsValid() )
	{
		return;
	}

	// Get the results from last frame
	unsigned queryValue = 1;
	if( m_isTotalQueryIssued )
	{
		HRESULT hr = m_totalQuery.GetPixelCount( renderContext, queryValue );
		if( hr == S_OK )
		{
			m_isTotalQueryIssued = false;
			m_totalNumOfPixels = queryValue;
		}
	}
	if( m_isActualQueryIssued )
	{
		HRESULT hr = m_actualQuery.GetPixelCount( renderContext, queryValue );
		if( hr == S_OK )
		{
			m_isActualQueryIssued = false;
			m_actualNumOfPixels = queryValue;
		}
	}

	// only update mValue when both queries have succesfully finished
	if( !m_isTotalQueryIssued && !m_isActualQueryIssued )
	{
		// If mTotalSize is 0 then we are off screen and we retain the last mValue (whatever it was).
		if( m_totalNumOfPixels )
		{
			m_value = (float)m_actualNumOfPixels / (float)m_totalNumOfPixels;
			m_value = TriClamp( m_value, 0.0f, 1.0f );
		}
	}




	// prepare batches for rendering
	renderContext.m_esm.ApplyStandardStates( Tr2EffectStateManager::RM_OPAQUE );
	// collect renderables from sprite, so we can pass the parent transform and EveTransform handles
	// all of the view update for us
	std::vector<ITr2Renderable*> renderables;
	EveUpdateContext dummyContext;
	for( EveTransformVector::iterator it = m_sprites.begin(); it != m_sprites.end(); ++it )
	{
		(*it)->UpdateSyncronous( dummyContext );
		(*it)->UpdateVisibility( frustum, transform );
		(*it)->GetRenderables( renderables, nullptr );
	}
	// collect batches from renderables (only from decal area, nothing else is important for
	// occlusion queries)
	for( std::vector<ITr2Renderable*>::iterator it = renderables.begin(); it != renderables.end(); ++it )
	{
		Tr2PerObjectData* objectData = (*it)->GetPerObjectData( m_batches );
		(*it)->GetBatches( m_batches, TRIBATCHTYPE_OPAQUE, objectData );
	}
	m_batches->Finalize();

	bool issueQueries = !m_isTotalQueryIssued && !m_isActualQueryIssued;
	if( issueQueries )
	{
		CR_RETURN( m_totalQuery.Begin( renderContext ) );
		renderContext.RenderBatches( m_batches, BlueSharedString( "TotalQuery" ) );
		CR_RETURN( m_totalQuery.End( renderContext ) );
		m_isTotalQueryIssued = true;
	}

	if( issueQueries )
	{
		CR_RETURN( m_actualQuery.Begin( renderContext ) );
		renderContext.RenderBatches( m_batches, BlueSharedString( "ActualQuery" ) );
		CR_RETURN( m_actualQuery.End( renderContext ) );
		m_isActualQueryIssued = true;
	}

	// cleanup
	m_batches->Clear();
}














