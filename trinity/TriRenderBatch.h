#pragma once
#ifndef TriRenderBatch_H
#define TriRenderBatch_H


#include "TriPoolAllocator.h"
#include "ITr2GeometryProvider.h"
#include "include/Rect.h"
#include "Shader/Tr2Material.h"

class Tr2PerObjectData;

BLUE_DECLARE( TriGeometryRes );

// See http://core/wiki/TriRenderBatch

// --------------------------------------------------------------------------------------
// Description
//   A render batch is an atomic D3D draw command.  It contains per-object data, a 
//   pointer to the Tr2Effect, state information, and sorting information.  Batches are
//   acquired from ITr2Renderables and submit their geometry to the device via the
//   SubmitGeometry function.  Batches are allocated and stored in an accumulator, which
//   is also responsible for batch sorting.  Batch submission to the device is managed
//   by the Tr2EffectStateManager.
//   <extlink http://core/wiki/TriRenderBatch>TriRenderBatch on CoreWiki</extlink>
// See Also
//   ITriRenderBatchAccumulator, Tr2EffectStateManager
// Summary
//   An atomic D3D draw command.
// --------------------------------------------------------------------------------------
class TriRenderBatch
{
public:
	// Constructor
	TriRenderBatch()
		:m_next( nullptr ),
		m_needsSyncronousSubmit( false ),
		m_objectData( nullptr ),
		m_userData( 0 ),
		m_depth( 0 ),
		m_renderingMode( Tr2EffectStateManager::RM_ANY )
	{ 
	}
	// Destructor
    virtual ~TriRenderBatch();

	// Get batch per-object data
	const Tr2PerObjectData* GetPerObjectData() const { return m_objectData; }
	// Set batch per-object data
	void SetPerObjectData( const Tr2PerObjectData* val ) { m_objectData = val; }


	void SetShaderMaterial( Tr2Material* material );

	Tr2Material* GetShaderMaterialInterface() const;
	Tr2Shader* GetShaderStateInterface() const;

	// Get the depth of the batch
	void SetDepth( unsigned int depth ) { m_depth = depth; }
	unsigned int GetDepth( void ) const { return m_depth; }

	int64_t GetUserData() const { return m_userData; }
	void SetUserData( int64_t userData ) { m_userData = userData; }

    TriRenderBatch* GetNext() const { return m_next; }

	virtual unsigned int GetPickingData() const { return 0; }

	enum OverrideOptions
	{
		RENDER_WITH_OVERRIDE,
		DO_NOT_RENDER_WITH_OVERRIDE,
		DO_NOT_USE_OVERRIDE_SHADERS,
	};

	virtual OverrideOptions RenderWithOverride( void ) const { return RENDER_WITH_OVERRIDE; }

	// Set rendering mode (expected render state) for batch
	void SetRenderingMode( Tr2EffectStateManager::RenderingMode mode ) 
	{ 
		m_renderingMode = mode; 
	}

	// Get rendering mode (expected render state) for batch
	Tr2EffectStateManager::RenderingMode GetRenderingMode( void ) const 
	{ 
		return m_renderingMode; 
	}

    // Subclasses must implement this function. It is expected to
    // call DrawIndexedPrimitive (or similar functions) on the
    // device. The caller is in turn expected to have set up the
    // proper world transform and set up the effect by calling
    // Begin and BeginPass on the effect kept in this batch.
    // After calling this function the caller is expected to call
    // EndPass and End on the effect.
    virtual void SubmitGeometry( Tr2RenderContext& renderContext ) = 0;
	
	// In case of parallel render encoding, SyncronousSubmit can be called
	// before if the batch needs to perform any work on the main render context.
	// For SyncronousSubmit to be called, m_needsSyncronousSubmit must be true.
	// SyncronousSubmit is only called for parallel render code paths.
	virtual void SyncronousSubmit( Tr2RenderContext& renderContext ) {}
	
	// Gets the batch type name for PIX debugging
	virtual const std::string& GetBatchTypeName( void ) const
	{ 
		static const std::string name = "TriRenderBatch";
		return name; 
	}

	// Next render batch in the linked-list
	TriRenderBatch* m_next;
	bool m_needsSyncronousSubmit;
protected:
	const Tr2PerObjectData* m_objectData;

private:
	Tr2Material* m_shaderMaterial;

	// User data, for generating arbitrary sort keys
	int64_t m_userData;

	// Depth
	unsigned int m_depth;

	// The rendering mode specifies which render state the batch expects to be in
	Tr2EffectStateManager::RenderingMode m_renderingMode;
};

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
//   Render batch specialization that submits geometry from a TriGeometryRes.
// See Also
//   TriRenderBatch
// --------------------------------------------------------------------------------------
class TriGeometryBatch : public TriRenderBatch
{
public:
    TriGeometryRes* GetGeometryResource() const { return m_geometryResource; }
    void SetGeometryResource( TriGeometryRes* val);

	void SetMeshParameters( unsigned int meshIx, 
		                    unsigned int areaIx, 
							unsigned int areaCount,
							bool reversed = false )
	{
		SetMeshParameters( meshIx, areaIx, areaCount, std::numeric_limits<float>::max(), reversed );
	}

	void SetMeshParameters( unsigned int meshIx,
							unsigned int areaIx,
							unsigned int areaCount,
							float screenSize,
							bool reversed = false )
	{
		m_meshIndex = meshIx;
		m_areaIndex = areaIx;
		m_areaCount = areaCount;
		m_screenSize = screenSize;
		m_reversed = reversed;
		m_performedSyncronousSubmit = false;
		if( reversed )
		{
			// Reversing an index buffer may involved reading it back from GPU memory
			// which can't be done with parallel rendering
			m_needsSyncronousSubmit = true;
		}
	}


	unsigned int GetMeshIndex()
	{
		return m_meshIndex;
	}

	unsigned int GetAreaIndex()
	{
		return m_areaIndex;
	}

	unsigned int GetAreaCount()
	{
		return m_areaCount;
	}

	bool GetReversed()
	{
		return m_reversed;
	}

	virtual unsigned int GetPickingData() const
	{ 
		return (m_meshIndex << 8) + m_areaIndex; 
	}

	// Submits geometry to the device
    virtual void SubmitGeometry( Tr2RenderContext& renderContext );
	
	void SyncronousSubmit( Tr2RenderContext& renderContext ) override;
	
	// Gets the batch type name for PIX debugging
	virtual const std::string& GetBatchTypeName( void ) const 
	{ 
		static const std::string name = "TriGeometryBatch";
		return name; 
	}

protected:
    TriGeometryResPtr m_geometryResource;
    unsigned int m_meshIndex;
    unsigned int m_areaIndex;
    unsigned int m_areaCount;
	float m_screenSize;
	bool m_reversed;
	bool m_performedSyncronousSubmit;
};

// --------------------------------------------------------------------------------------
// Description
//   Render batch specialization that forwards the SubmitGeometry call to an object which
//   implements ITr2GeometryProvider.  It is used by a number of EVE classes for drawing
//   things like line sets.
// See Also
//   TriRenderBatch, ITr2GeometryProvider
// --------------------------------------------------------------------------------------
class TriForwardingBatch : public TriRenderBatch
{
public:
	// Set the geometry provider
	void SetGeometryProvider( ITr2GeometryProvider* val )
	{
		m_geom = val;
	}

	// Forward the SubmitGeometry call to the geometry provider
	void SubmitGeometry( Tr2RenderContext& renderContext )
	{
		if( m_geom )
		{
			m_geom->SubmitGeometry( renderContext );
		}
	}

	// Gets the batch type name for PIX debugging
	virtual const std::string& GetBatchTypeName( void ) const
	{ 
		static const std::string name = "TriForwardingBatch";
		return name; 
	}

private:
	ITr2GeometryProviderPtr m_geom;
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

// --------------------------------------------------------------------------------------
// Description
//   Structure used for sorting TriRenderBatches.  It holds a pointer to the render batch
//   and a 64-bit integer sorting key.  The key is produced by a KeyGenerator object,
//   managed by the TriRenderBatchAccumulator.
// See Also
//   TriRenderBatch, TriRenderBatchAccumulator
// --------------------------------------------------------------------------------------
struct RenderBatchSortEntry
{
	TriRenderBatch* m_batch;
	int64_t m_sortKey;
	
	// Compare batches by sort key
	bool operator<(const RenderBatchSortEntry& other) const
	{
		return ( m_sortKey < other.m_sortKey );
	}
};

// --------------------------------------------------------------------------------------
// Description
//   Default render batch sort key generator.  Assigns 0 to the sort key, and requests
//   no sorting.
// See Also
//   TriRenderBatch, TriRenderBatchAccumulator, RenderBatchSortEntry
// --------------------------------------------------------------------------------------
struct DefaultKeyGenerator
{
	// Assigns 0 to the sort key
	void GenerateKey( RenderBatchSortEntry& entry ) const
{
		entry.m_sortKey = 0;
	}

	// Gets the sort type (no sorting)
	RenderBatchSortType GetSortType( void ) const { return RENDERBATCHSORTTYPE_NONE; }
};

struct EffectKeyGenerator
{
	// Generates a key from the Tr2Effect used by the batch
	void GenerateKey( RenderBatchSortEntry& entry ) const
	{
		int64_t effectKey = 0xFFFFFFFF;
		auto shaderMaterial = entry.m_batch->GetShaderMaterialInterface();
		if( shaderMaterial )
		{
			effectKey = (int64_t)shaderMaterial->GetSortValue();
		}
		
		entry.m_sortKey = effectKey;
	}

	// Requests regular std::sort
	RenderBatchSortType GetSortType( void ) const { return RENDERBATCHSORTTYPE_SORT; }
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
	// Commit batch to the accumulator
	virtual void Commit( TriRenderBatch* batch ) = 0;
	// Finalize & sort the accumulator
	virtual void Finalize( void ) = 0;

	// Get the first batch in the list
	virtual TriRenderBatch* GetFirstBatch( void ) const = 0;
	// Get the number of batches
    virtual size_t GetBatchCount( void ) const = 0;

	// Transfer ownership of batch at specified index to another accumulator, 
	// which is responsible for destroying the batch
	virtual void TransferBatchToOtherAccumulator( ITriRenderBatchAccumulator* accumulator, 
		                                     size_t index 
										   ) = 0;

	// Are batches sorted by effect?
	virtual bool IsChainedByEffect( void ) const = 0;

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
		m_keyGen(),
		m_head( NULL ),
		m_tail( NULL ),
		m_batchCount( 0 ),
		m_batchesToSort()
	{
	}

    virtual ~TriRenderBatchAccumulator()
	{
		m_tail = NULL;
	}

    // Clears the accumulator, resetting to initial state. Note that the
	// allocator must be cleared separately as it may be shared with other things.
    virtual void Clear( void )
	{
		TriRenderBatch* p = m_head;
		while( p )
		{
			TriRenderBatch* next = p->m_next;
			p->~TriRenderBatch();
			p = next;
		}

		m_head = m_tail = NULL;
		m_batchCount = 0;
		m_batchesToSort.clear();
		m_userData = 0x0;
		m_renderingMode = Tr2EffectStateManager::RM_ANY;
	}

    // Commits the batch, chaining it according to how the accumulator was
    // constructed, i.e. either by effect or in a single chain.
    virtual void Commit( TriRenderBatch* batch )
	{
		// Set the user data & rendering mode for the batch
		batch->SetUserData( m_userData );
		batch->SetRenderingMode( m_renderingMode );

		// If the accumulator has an unsorted key policy, build the linked list immediately
		if( m_keyGen.GetSortType() == RENDERBATCHSORTTYPE_NONE )
		{
			if( m_tail )
			{
				m_tail->m_next = batch;
				m_tail = batch;
			}
			else
			{
				m_head = batch;
				m_tail = batch;
			}
			m_tail->m_next = NULL;
		}
		// Otherwise store the batches for later sorting
		else
		{
			RenderBatchSortEntry se;
			se.m_batch = batch;
			m_keyGen.GenerateKey( se );

			m_batchesToSort.push_back( se );
		}

		++m_batchCount;
	}

    // Call this after committing the last batch.
    virtual void Finalize( void )
	{
		if( !m_batchesToSort.empty() )
		{
			if( m_keyGen.GetSortType() == RENDERBATCHSORTTYPE_SORT )
			{
				std::sort( m_batchesToSort.begin(), m_batchesToSort.end() );
			}
			else if( m_keyGen.GetSortType() == RENDERBATCHSORTTYPE_STABLE_SORT )
			{
				std::stable_sort( m_batchesToSort.begin(), m_batchesToSort.end() );
			}

			SortEntryVector::const_iterator it = m_batchesToSort.begin();

			m_head = it->m_batch;
			m_tail = it->m_batch;

			for( ++it; it != m_batchesToSort.end(); ++it )
			{
				m_tail->m_next = it->m_batch;
				m_tail = it->m_batch;
			}

			m_tail->m_next = NULL;
		}
	}

    // Gets the head of the batch chain
	virtual TriRenderBatch* GetFirstBatch() const { return m_head; }

	// Get the batch count
    virtual size_t GetBatchCount() const { return m_batchCount; }

	// Are the batches sorted by effect?
	virtual bool IsChainedByEffect() const { return m_keyGen.GetSortType() != RENDERBATCHSORTTYPE_NONE; }

	// Transfer ownership of batch at specified index to another accumulator, 
	// which is responsible for destroying the batch
	virtual void TransferBatchToOtherAccumulator( ITriRenderBatchAccumulator* accumulator, 
		                                     size_t index )
	{
		// Verify that the other accumulator is valid
		CCP_ASSERT_M( accumulator != NULL, "Attempt to transfer render batch to NULL accumulator" );
		
		// Verify that the index is in range
		CCP_ASSERT_M( index < m_batchesToSort.size(), "Render batch index out of range" );

		// Get the batch to transfer
		TriRenderBatch* batchToTransfer = m_batchesToSort[index].m_batch;
		// Verify that the batch is valid
		CCP_ASSERT_M( batchToTransfer != NULL, "Attempt to transfer NULL render batch" );

		// Transfer the batch
		accumulator->Commit( batchToTransfer );
		// Get rid of the transferred batch
		m_batchesToSort.erase( m_batchesToSort.begin() + index );
	}

	void ReverseBatchOrder( size_t beginIndex, size_t endIndex )
	{
		std::reverse( m_batchesToSort.begin() + beginIndex, m_batchesToSort.begin() + endIndex );
	}

private:
	KeyGenerator m_keyGen;

    TriRenderBatch* m_head;
    TriRenderBatch* m_tail;

	size_t m_batchCount;

    typedef std::vector<RenderBatchSortEntry> SortEntryVector;
    SortEntryVector m_batchesToSort;
};

// --------------------------------------------------------------------------------------
// Description
//  Subclass of ITriRenderBatchAccumulator for simple batch storage.  Batches are stored
//  in this object, but ownership of batches must be passed to another accumulator for
//  cleanup.  
// See Also
//   TriRenderBatch, Tr2EffectStateManager
// Summary
//   Provides basic batch storage.
// --------------------------------------------------------------------------------------
class TriRenderBatchStore : public ITriRenderBatchAccumulator
    {
public:
	TriRenderBatchStore( TriPoolAllocator* allocator );
	virtual ~TriRenderBatchStore();

	// Clears the accumulator, resetting to initial state. Note that the
	// allocator must be cleared separately as it may be shared with other things.
    virtual void Clear( void );

	// Call this after committing the last batch.
	virtual void Finalize( void ) { /* Do nothing */ }
	// Commits the batch to the store
	virtual void Commit( TriRenderBatch* batch );

	// Returns NULL
	virtual TriRenderBatch* GetFirstBatch() const { return NULL; }

	// Get the number of batches in the store
    virtual size_t GetBatchCount() const { return m_batchCount; }

	// Are batches sorted by effect?  (Not for batch stores)
	virtual bool IsChainedByEffect() const { return false; }

	// Transfer ownership of batch at specified index to another accumulator, 
	// which is responsible for destroying the batch
	virtual void TransferBatchToOtherAccumulator( ITriRenderBatchAccumulator* accumulator, 
		                                     size_t index );

private:
	size_t m_batchCount;

    std::vector<TriRenderBatch*> m_batches;
};

struct TriRenderBatchStats
{
	unsigned int m_numDiscreteEffects;
	unsigned int m_numDiscreteEffectRes;
	unsigned int m_drawCallsPerEffectResMax;
	unsigned int m_drawCallsPerEffectMax;
	float m_drawCallsPerEffectResAverage;
	float m_drawCallsPerEffectAverage;
};

#endif // TriRenderBatch_H
