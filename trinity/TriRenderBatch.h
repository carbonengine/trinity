#pragma once
#ifndef TriRenderBatch_H
#define TriRenderBatch_H


#include "TriPoolAllocator.h"
#include "include/Rect.h"
#include "Shader/Tr2Material.h"
#include "Shader/Tr2Shader.h"

class Tr2PerObjectData;

// --------------------------------------------------------------------------------------
// Description
//   Is a basic struct to hold a start index and a count, which indicates a block
//   of mesh-groups in a granny mesh.
// See Also
//   TriGeometryBatch
// --------------------------------------------------------------------------------------
class TriRenderBatchAreaBlock
{
public:
	TriRenderBatchAreaBlock() {}
	TriRenderBatchAreaBlock( unsigned int startIndex, unsigned int count ) : m_startIndex( startIndex ), m_count( count ) {}

	// optimizing lists
	static void Optimize( std::vector<TriRenderBatchAreaBlock>& areaBlockVector );

	unsigned int m_startIndex;
	unsigned int m_count;
};

class TriRenderBatchAreaBlocksWithSharedMaterial : public TriRenderBatchAreaBlock
{
public:
	TriRenderBatchAreaBlocksWithSharedMaterial();
	TriRenderBatchAreaBlocksWithSharedMaterial( std::vector<TriRenderBatchAreaBlock>& areaBlockVector, Tr2Material* shader );
	void Optimize();
	void Clear();

	Tr2MaterialPtr m_shaderMaterial;
	std::vector<TriRenderBatchAreaBlock> m_areaBlockVector;

};
// --------------------------------------------------------------------------------------
// Description
//   Enumeration of batch sorting algorithms.
// --------------------------------------------------------------------------------------
enum RenderBatchSortType
{
	RENDERBATCHSORTTYPE_NONE,			// Don't sort
	RENDERBATCHSORTTYPE_SORT,			// Use std::sort
	RENDERBATCHSORTTYPE_STABLE_SORT		// Use std::stable_sort
};


struct Tr2RenderBatch
{
	void SetMaterial( Tr2Material* material );

	void SetGeometry( unsigned vertexDecl, const Tr2BufferAL& vb, uint32_t stride, const Tr2BufferAL& ib, uint32_t indexStride );
	void SetGeometry( unsigned vertexDecl, const Tr2SuballocatedBuffer::Allocation& vb, const Tr2SuballocatedBuffer::Allocation& ib );
	void SetGeometry( unsigned vertexDecl, const Tr2SuballocatedBuffer::Allocation& vb1, const Tr2SuballocatedBuffer::Allocation& vb2, const Tr2SuballocatedBuffer::Allocation& ib );
	void SetVertexDeclaration( unsigned int vertexDeclaration );
	void SetStreamSource( uint32_t index, const Tr2BufferAL& vb, uint32_t stride );
	void SetStreamSource( uint32_t index, const Tr2SuballocatedBuffer::Allocation& vb );
	void SetInidices( const Tr2BufferAL& ib, uint32_t stride );
	void SetInidices( const Tr2SuballocatedBuffer::Allocation& ib );
	void SetTopology( Tr2RenderContextEnum::Topology topology );

	void SetPerObjectData( const Tr2PerObjectData* perObjectData );
	void SetDrawIndexedInstanced( uint32_t indexCountPerInstance, uint32_t instanceCount, uint32_t startIndexLocation, uint32_t baseVertexLocation, uint32_t startInstanceLocation );
	void SetDrawInstanced( uint32_t vertexCountPerInstance, uint32_t instanceCount, uint32_t startVertexLocation, uint32_t startInstanceLocation );

	void SetRenderingMode( Tr2EffectStateManager::RenderingMode mode );

	void SetPickingData( uint32_t pickingData );
	void SetPickingData( uint32_t meshIndex, uint32_t areaIndex );

	bool IsValid() const;
	operator bool() const;

	Tr2Material* m_material = nullptr;
	Tr2Shader* m_shader = nullptr;

	unsigned int m_vertexDeclaration = 0;
	const Tr2BufferAL* m_vertexStreams[2] = {};
	uint32_t m_stride[2] = {};
	const Tr2BufferAL* m_indexBuffer = nullptr;
	uint32_t m_indexStride = 0;

	const Tr2PerObjectData* m_objectData = nullptr;
	Tr2EffectStateManager::RenderingMode m_renderingMode = Tr2EffectStateManager::RM_ANY;
	Tr2RenderContextEnum::Topology m_topology = Tr2RenderContextEnum::TOP_TRIANGLES;

	uint32_t m_indexCountPerInstance = 0;
	uint32_t m_instanceCount = 0;
	uint32_t m_startIndexLocation = 0;
	uint32_t m_baseVertexLocation = 0;
	uint32_t m_startInstanceLocation = 0;

	uint32_t m_pickingData = 0;
	uint32_t m_depth = 0;

	uint32_t m_groupCount = 1;
};

bool CanBeBinned( Tr2RenderBatch& batch1, const Tr2RenderBatch& batch2 );

template <class ForwardIt>
void Tr2GdprBatchFullPartition( ForwardIt first, ForwardIt last )
{
	while( first != last )
	{
		auto groupEnd = std::partition( first, last, [&first]( auto& x ) { return first->m_shader == x.m_shader; } );
		while( first != groupEnd )
		{
			auto binEnd = std::partition( first, groupEnd, [&first]( auto& x ) { return CanBeBinned( *first, x ); } );
			first->m_groupCount = uint32_t( binEnd - first );
			first = binEnd;
		}
	}
}

template <class BidirIt>
void Tr2GdprBatchStableFullPartition( BidirIt first, BidirIt last )
{
	while( first != last )
	{
		auto groupEnd = std::stable_partition( first, last, [&first]( auto& x ) { return first->m_shader == x.m_shader; } );
		first->m_groupCount = uint32_t( groupEnd - first );
		while( first != groupEnd )
		{
			auto binEnd = std::stable_partition( first, groupEnd, [&first]( auto& x ) { return CanBeBinned( *first, x ); } );
			first->m_groupCount = uint32_t( binEnd - first );
			first = binEnd;
		}
	}
}


// --------------------------------------------------------------------------------------
// Description
//   Default render batch sort key generator.  Assigns 0 to the sort key, and requests
//   no sorting.
// See Also
//   TriRenderBatch, TriRenderBatchAccumulator, RenderBatchSortEntry
// --------------------------------------------------------------------------------------
struct DefaultKeyGenerator
{
	static bool Less( const Tr2RenderBatch& batch1, const Tr2RenderBatch& batch2 )
	{
		return false;
	}

	// Gets the sort type (no sorting)
	constexpr static RenderBatchSortType GetSortType()
	{
		return RENDERBATCHSORTTYPE_NONE;
	}

	static constexpr bool ALLOW_GDPR = false;
};

struct EffectKeyGenerator
{
	static bool Less( const Tr2RenderBatch& batch1, const Tr2RenderBatch& batch2 )
	{
		if( batch1.m_shader < batch2.m_shader )
		{
			return true;
		}
		if( batch1.m_shader > batch2.m_shader )
		{
			return false;
		}
		return batch1.m_vertexDeclaration < batch2.m_vertexDeclaration;
	}

	// Requests regular std::sort
	constexpr static RenderBatchSortType GetSortType()
	{
		return RENDERBATCHSORTTYPE_SORT;
	}

	static constexpr bool ALLOW_GDPR = true;
};

// --------------------------------------------------------------------------------------
// Description
//  Interface base class for render batch accumulators.  Accumulators are responsible for
//  allocating render batches, sorting lists of batches, and dispatching them to the
//  device via Tr2EffectStateManager.
// See Also
//   TriRenderBatch, Tr2EffectStateManager
// Summary
//   Interface base class for render batch accumulators.
// --------------------------------------------------------------------------------------
class ITriRenderBatchAccumulator
{
public:
	// Constructor
	ITriRenderBatchAccumulator( TriPoolAllocator* allocator ) :
		m_userData( 0x0 ),
		m_allocator( allocator ),
		m_renderingMode( Tr2EffectStateManager::RM_ANY )
	{
	}

	// Destructor
	virtual ~ITriRenderBatchAccumulator() {}

	// Allocate memory block of the given size
	void* Allocate( size_t size )
	{
		return m_allocator->Allocate( size );
	}

	// Allocate an object of the given type (this function cannot be virtual)
	template <class T>
	T* Allocate()
	{
		return m_allocator->Allocate<T>();
	}

	// Clear the accumulator
	virtual void Clear( void ) = 0;

	virtual void Commit( const Tr2RenderBatch& batch ) = 0;
	virtual const std::vector<Tr2RenderBatch>& GetGdprBatches() const = 0;
	virtual const std::vector<Tr2RenderBatch>& GetBatches() const = 0;
	virtual std::vector<Tr2RenderBatch>& GetBatches() = 0;

	// Finalize & sort the accumulator
	virtual void Finalize( void ) = 0;

	// Get the number of batches
    virtual size_t GetBatchCount( void ) const = 0;

	// Are batches sorted by effect?
	virtual bool IsChainedByEffect( void ) const = 0;

	virtual void TransferFrom( ITriRenderBatchAccumulator* source ) = 0;

	// Set userdata
	void SetUserData( int64_t userData ) { m_userData = userData; }

	// Set rendering mode
	void SetRenderingMode( Tr2EffectStateManager::RenderingMode mode ) { m_renderingMode = mode; }

	// Get rendering mode
	Tr2EffectStateManager::RenderingMode GetRenderingMode() const { return m_renderingMode; }

protected:
	int64_t m_userData;
	TriPoolAllocator* m_allocator;
	Tr2EffectStateManager::RenderingMode m_renderingMode;
};

// --------------------------------------------------------------------------------------
// Description
//  Templated subclass of ITriRenderBatchAccumulator.  The template parameter provides
//  a sort key generator, defaulting to DefaultKeyGenerator (unsorted).  The sort key
//  generator constructs sort keys for batches at commit time & specifies a sorting
//  policy.
// See Also
//   TriRenderBatch, Tr2EffectStateManager
// Summary
//  Templated subclass of ITriRenderBatchAccumulator
// --------------------------------------------------------------------------------------
template <class KeyGenerator = DefaultKeyGenerator>
class TriRenderBatchAccumulator : public ITriRenderBatchAccumulator
{
public:
    // Construct an accumulator, associating it with the given allocator.
    TriRenderBatchAccumulator( TriPoolAllocator* allocator ) :
		ITriRenderBatchAccumulator( allocator ),
		m_keyGen()
	{
	}

    virtual ~TriRenderBatchAccumulator()
	{
	}

    // Clears the accumulator, resetting to initial state. Note that the
	// allocator must be cleared separately as it may be shared with other things.
    virtual void Clear( void )
	{
		m_userData = 0x0;
		m_renderingMode = Tr2EffectStateManager::RM_ANY;

		m_gdprBatches.clear();
		m_batches.clear();
	}

	void Commit( const Tr2RenderBatch& batch ) override
	{
		if( !batch.IsValid() )
		{
			return;
		}
		if( KeyGenerator::ALLOW_GDPR && batch.m_material->CompatibleWithGdr() && batch.m_topology == Tr2RenderContextEnum::TOP_TRIANGLES && batch.m_indexBuffer )
		{
			m_gdprBatches.push_back( batch );
			m_gdprBatches.back().m_renderingMode = m_renderingMode;
		}
		else
		{
			m_batches.push_back( batch );
			m_batches.back().m_renderingMode = m_renderingMode;
		}
	}

	void TransferFrom( ITriRenderBatchAccumulator* source ) override
	{
		auto& gdpr = source->GetGdprBatches();
		auto& batches = source->GetBatches();
		if( KeyGenerator::ALLOW_GDPR )
		{
			m_gdprBatches.insert( end( m_gdprBatches ), begin( gdpr ), end( gdpr ) );
		}
		else
		{
			m_batches.insert( end( m_batches ), begin( gdpr ), end( gdpr ) );
		}
		m_batches.insert( end( m_batches ), begin( batches ), end( batches ) );
		source->Clear();
	}

	const std::vector<Tr2RenderBatch>& GetGdprBatches() const override
	{
		return m_gdprBatches;
	}

	const std::vector<Tr2RenderBatch>& GetBatches() const override
	{
		return m_batches;
	}

	std::vector<Tr2RenderBatch>& GetBatches() override
	{
		return m_batches;
	}

    // Call this after committing the last batch.
    virtual void Finalize( void )
	{
		if constexpr ( m_keyGen.GetSortType() == RENDERBATCHSORTTYPE_SORT )
		{
			if( KeyGenerator::ALLOW_GDPR )
			{
				Tr2GdprBatchFullPartition( begin( m_gdprBatches ), end( m_gdprBatches ) );
				Tr2GdprBatchFullPartition( begin( m_batches ), end( m_batches ) );
			}
			else
			{
				sort( begin( m_batches ), end( m_batches ), KeyGenerator::Less );
			}
		}
		else if constexpr ( m_keyGen.GetSortType() == RENDERBATCHSORTTYPE_STABLE_SORT )
		{
			if( KeyGenerator::ALLOW_GDPR )
			{
				Tr2GdprBatchStableFullPartition( begin( m_gdprBatches ), end( m_gdprBatches ) );
				Tr2GdprBatchStableFullPartition( begin( m_batches ), end( m_batches ) );
			}
			else
			{
				stable_sort( begin( m_batches ), end( m_batches ), KeyGenerator::Less );
			}
		}
	}

	// Get the batch count
    virtual size_t GetBatchCount() const { return m_gdprBatches.size() + m_batches.size(); }

	// Are the batches sorted by effect?
	virtual bool IsChainedByEffect() const { return m_keyGen.GetSortType() != RENDERBATCHSORTTYPE_NONE; }

private:
	KeyGenerator m_keyGen;

	std::vector<Tr2RenderBatch> m_gdprBatches;
	std::vector<Tr2RenderBatch> m_batches;
};

#endif // TriRenderBatch_H
