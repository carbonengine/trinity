#include "StdAfx.h"

#include "TriRenderBatch.h"
#include "Resources/TriGeometryRes.h"

BLUE_DEFINE_INTERFACE( ITr2GeometryProvider );

TriRenderBatchStore::TriRenderBatchStore( TriPoolAllocator* allocator ) :
	ITriRenderBatchAccumulator( allocator ),
	m_batchCount( 0 ),
	m_batches()	
{
}

TriRenderBatchStore::~TriRenderBatchStore()
{
	Clear();
}

void TriRenderBatchStore::Clear( void )
{
	for( std::vector<TriRenderBatch*>::iterator it = m_batches.begin(); it != m_batches.end(); ++it )
	{
		TriRenderBatch* doomed = *it;
		if( doomed )
		{
			doomed->~TriRenderBatch();
			doomed = NULL;
		}
	}

	m_batches.clear();
	m_batchCount = 0;
}

void TriRenderBatchStore::Commit( TriRenderBatch* batch )
{
	if( batch )
	{
		// Set the user data and rendering mode for the batch
		batch->SetUserData( m_userData );
		batch->SetRenderingMode( m_renderingMode );

		m_batches.push_back( batch );
		++m_batchCount;
	}
}

void TriRenderBatchStore::TransferBatchToOtherAccumulator( 
		ITriRenderBatchAccumulator* accumulator, 
		size_t index )
{
	// Verify that the other accumulator is valid
	CCP_ASSERT_M( accumulator != NULL, "Attempt to transfer render batch to NULL accumulator" );

	// Verify that the index is in range
	CCP_ASSERT_M( index < m_batches.size(), "Render batch index out of range" );

	// Get the batch to transfer
	TriRenderBatch* batchToTransfer = m_batches[index];
	// Verify that the batch is valid
	CCP_ASSERT_M( batchToTransfer != NULL, "Attempt to transfer NULL render batch" );

	accumulator->SetRenderingMode( batchToTransfer->GetRenderingMode() );
	// Transfer the batch
	accumulator->Commit( batchToTransfer );
	// Flag the batch as NULL
	m_batches[index] = NULL;
}

TriRenderBatch::~TriRenderBatch()
{
}

void TriRenderBatch::SetShaderMaterial( Tr2Material* val )
{
	m_shaderMaterial = val;
}

Tr2Material* TriRenderBatch::GetShaderMaterialInterface() const
{
	return m_shaderMaterial;
}

Tr2Shader* TriRenderBatch::GetShaderStateInterface() const
{
	if (m_shaderMaterial)
	{
		return m_shaderMaterial->GetShaderStateInterface();
	}
	else
	{
		return NULL;
	}
}




// -------------------------------------------------------------
// Description:
//   If you have a list of blocks (a startindex and a count), you
//   can optimize this list by combining two or more subsequent
//   elements into one by just increasing the count! Thats what
//   this function does.
// Arguments:
//	 areaBlockVector - the to-be-optimized vector
// -------------------------------------------------------------
void TriRenderBatchAreaBlock::Optimize( std::vector<TriRenderBatchAreaBlock>& areaBlockVector )
{
	// turn area block list into a std::set, so we get sorting and remove duplicates
	std::set< unsigned int > overlayHullAreas;
	for( auto it = areaBlockVector.begin(); it != areaBlockVector.end(); ++it )
	{
		for( unsigned n = 0; n < it->m_count; ++n )
		{
			overlayHullAreas.insert( it->m_startIndex + n );
		}
	}

	// convert the set back into an area block list, but this time it will be condensed
	areaBlockVector.clear();
	TriRenderBatchAreaBlock ab( 0, 0 );
	for( auto it = overlayHullAreas.begin(); it != overlayHullAreas.end(); ++it )
	{
		// this is for the loop iteration
		if( ab.m_count == 0 )
		{
			ab.m_startIndex = *it;
			ab.m_count = 1;
		}
		// still going?
		else if( ab.m_startIndex + ab.m_count == *it )
		{
			++ab.m_count;
		}
		// is a finisher, so put it on list
		else
		{
			areaBlockVector.push_back( ab );
			ab.m_startIndex = *it;
			ab.m_count = 1;
		}
	}
	// don't forget the last element!
	if( ab.m_count > 0 )
	{
		areaBlockVector.push_back( ab );
	}
}



void TriGeometryBatch::SubmitGeometry( Tr2RenderContext& renderContext )
{
	if( m_geometryResource )
	{
		m_geometryResource->RenderAreas( m_screenSize, m_meshIndex, m_areaIndex, m_areaCount, renderContext, m_reversed, !m_performedSyncronousSubmit );
	}
}

void TriGeometryBatch::SyncronousSubmit( Tr2RenderContext& renderContext )
{
	if( m_geometryResource && m_reversed )
	{
		if( auto meshData = m_geometryResource->GetMeshData( m_meshIndex, m_screenSize ) )
		{
			m_geometryResource->ReverseIndexBuffer( *meshData, renderContext );
		}
	}
	m_performedSyncronousSubmit = true;
}

void TriGeometryBatch::SetGeometryResource( TriGeometryRes* val )
{
	m_geometryResource = val;
}
