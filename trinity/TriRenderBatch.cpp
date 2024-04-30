#include "StdAfx.h"

#include "TriRenderBatch.h"
#include "Resources/TriGeometryRes.h"


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

TriRenderBatchAreaBlocksWithSharedMaterial::TriRenderBatchAreaBlocksWithSharedMaterial()
{
}

void TriRenderBatchAreaBlocksWithSharedMaterial::Optimize()
{
	TriRenderBatchAreaBlock::Optimize( m_areaBlockVector );
}

void TriRenderBatchAreaBlocksWithSharedMaterial::Clear()
{
	m_areaBlockVector.clear();
}


TriRenderBatchAreaBlocksWithSharedMaterial::TriRenderBatchAreaBlocksWithSharedMaterial()
{
}


void TriRenderBatchAreaBlocksWithSharedMaterial::Optimize()
{
	TriRenderBatchAreaBlock::Optimize(m_areaBlockVector);
}

void TriRenderBatchAreaBlocksWithSharedMaterial::Clear()
{
	m_areaBlockVector.clear();
}


void Tr2RenderBatch::SetMaterial( Tr2Material* material )
{
	m_material = material;
	m_shader = material->GetShaderStateInterface();
}

void Tr2RenderBatch::SetGeometry( unsigned vertexDecl, const Tr2BufferAL& vb, uint32_t stride, const Tr2BufferAL& ib, uint32_t indexStride )
{
	m_vertexDeclaration = vertexDecl;
	m_vertexStreams[0] = &vb;
	m_vertexStreams[1] = nullptr;
	m_stride[0] = stride;
	m_indexBuffer = &ib;
	m_indexStride = indexStride;
}

void Tr2RenderBatch::SetGeometry( unsigned vertexDecl, const Tr2SuballocatedBuffer::Allocation& vb, const Tr2SuballocatedBuffer::Allocation& ib )
{
	SetGeometry( vertexDecl, vb.GetBuffer(), vb.GetStride(), ib.GetBuffer(), ib.GetStride() );
}

void Tr2RenderBatch::SetGeometry( unsigned vertexDecl, const Tr2SuballocatedBuffer::Allocation& vb1, const Tr2SuballocatedBuffer::Allocation& vb2, const Tr2SuballocatedBuffer::Allocation& ib )
{
	SetGeometry( vertexDecl, vb1, ib );
	m_vertexStreams[1] = &vb2.GetBuffer();
	m_stride[1] = vb2.GetStride();
}

void Tr2RenderBatch::SetVertexDeclaration( unsigned int vertexDeclaration )
{
	m_vertexDeclaration = vertexDeclaration;
}

void Tr2RenderBatch::SetStreamSource( uint32_t index, const Tr2BufferAL& vb, uint32_t stride )
{
	m_vertexStreams[index] = &vb;
	m_stride[index] = stride;
}

void Tr2RenderBatch::SetStreamSource( uint32_t index, const Tr2SuballocatedBuffer::Allocation& vb )
{
	m_vertexStreams[index] = &vb.GetBuffer();
	m_stride[index] = vb.GetStride();
}

void Tr2RenderBatch::SetInidices( const Tr2BufferAL& ib, uint32_t stride )
{
	m_indexBuffer = &ib;
	m_indexStride = stride;
}

void Tr2RenderBatch::SetInidices( const Tr2SuballocatedBuffer::Allocation& ib )
{
	m_indexBuffer = &ib.GetBuffer();
	m_indexStride = ib.GetStride();
}

void Tr2RenderBatch::SetTopology( Tr2RenderContextEnum::Topology topology )
{
	m_topology = topology;
}

void Tr2RenderBatch::SetPerObjectData( const Tr2PerObjectData* perObjectData )
{
	m_objectData = perObjectData;
}

void Tr2RenderBatch::SetDrawIndexedInstanced( uint32_t indexCountPerInstance, uint32_t instanceCount, uint32_t startIndexLocation, uint32_t baseVertexLocation, uint32_t startInstanceLocation )
{
	m_indexCountPerInstance = indexCountPerInstance;
	m_instanceCount = instanceCount;
	m_startIndexLocation = startIndexLocation;
	m_baseVertexLocation = baseVertexLocation;
	m_startInstanceLocation = startInstanceLocation;
}

void Tr2RenderBatch::SetDrawInstanced( uint32_t vertexCountPerInstance, uint32_t instanceCount, uint32_t startVertexLocation, uint32_t startInstanceLocation )
{
	m_indexCountPerInstance = vertexCountPerInstance;
	m_instanceCount = instanceCount;
	m_startIndexLocation = startVertexLocation;
	m_startInstanceLocation = startInstanceLocation;
}

void Tr2RenderBatch::SetRenderingMode( Tr2EffectStateManager::RenderingMode mode )
{
	m_renderingMode = mode;
}

void Tr2RenderBatch::SetPickingData( uint32_t pickingData )
{
	m_pickingData = pickingData;
}

void Tr2RenderBatch::SetPickingData( uint32_t meshIndex, uint32_t areaIndex )
{
	SetPickingData( ( meshIndex << 8 ) | ( areaIndex & 0xff ) );
}



bool Tr2RenderBatch::IsValid() const
{
	return m_shader != nullptr;
}

Tr2RenderBatch::operator bool() const
{
	return IsValid();
}


bool CanBeBinned( Tr2RenderBatch& batch1, const Tr2RenderBatch& batch2 )
{
	if( batch1.m_shader != batch2.m_shader )
	{
		return false;
	}
	if( batch1.m_vertexDeclaration != batch2.m_vertexDeclaration )
	{
		return false;
	}
	if( batch1.m_indexStride != batch2.m_indexStride )
	{
		return false;
	}
	if( batch1.m_vertexStreams[0] != batch2.m_vertexStreams[0] || batch1.m_vertexStreams[1] != batch2.m_vertexStreams[1] )
	{
		return false;
	}
	if( batch1.m_renderingMode != batch2.m_renderingMode )
	{
		return false;
	}
	return true;
}
