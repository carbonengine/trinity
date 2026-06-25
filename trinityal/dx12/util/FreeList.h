// Copyright © 2019 CCP ehf.

#pragma once

#if TRINITY_PLATFORM == TRINITY_DIRECTX12

#include <cstdint>
#include <vector>

/** FreeList class, used by D3D12 Allocators */
template <class T, typename tInitArgs>
class FreeList
{
public:
	/** List Entry State */
	enum EMode
	{
		/** Entry is free */
		EM_Free = 0,

		/** Entry is in use */
		EM_InUse = 1
	};

	/** Single list entry */
	struct Entry : T
	{
		Entry( uint32_t entryIndex, const tInitArgs& initArgs ) :
			T( entryIndex, initArgs ),
			m_mode( EM_Free ),
			m_prev( nullptr ),
			m_next( nullptr )
		{
		}

		EMode m_mode;
		Entry* m_prev;
		Entry* m_next;
	};

	/**  */
	FreeList( uint32_t entryCount, const tInitArgs& initArgs ) :
		m_entryCount( 0 ),
		m_freeCount( 0 ),
		m_freeHead( nullptr ),
		m_inUseHead( nullptr )
	{
		AddPage( entryCount, initArgs );

		// Point to the first element in the list
		m_freeHead = m_allEntries[0]->data();
	}

	/**  */
	~FreeList()
	{
		m_entryCount = 0;
		m_freeHead = nullptr;
		m_inUseHead = nullptr;
	}

	template <typename Op>
	void EnumerateEntries( const Op& operation )
	{
		for( auto& page : m_allEntries )
		{
			for( auto& entry : *page )
			{
				operation( entry );
			}
		}
	}

	void AddPage( uint32_t pageSize, const tInitArgs& initArgs )
	{
		m_allEntries.emplace_back( std::make_unique<std::vector<Entry>>() );
		// Create free list
		auto& page = *m_allEntries.back();
		page.reserve( pageSize );
		for( uint32_t entryIdx = 0; entryIdx < pageSize; ++entryIdx )
		{
			page.push_back( Entry( m_entryCount + entryIdx, initArgs ) );
		}

		for( uint32_t entryIdx = 0; entryIdx < pageSize; ++entryIdx )
		{
			Entry& entry = page[entryIdx];

			entry.m_prev = entryIdx > 0 ? &page[entryIdx - 1] : nullptr;
			entry.m_next = entryIdx < ( m_entryCount - 1 ) ? &page[entryIdx + 1] : nullptr;
		}

		m_entryCount += pageSize;
		m_freeCount += pageSize;
		page.back().m_next = m_freeHead;
		m_freeHead = &page[0];
	}

	/** Get whether the list is full */
	bool IsFull() const
	{
		return m_freeHead == nullptr;
	}

	/** Get whether the list is empty */
	bool IsEmpty() const
	{
		return m_inUseHead == nullptr;
	}

	/** Get the number of free entries */
	uint32_t FreeCount() const
	{
		return m_freeCount;
	}

	/** Gets the total number of entries */
	uint32_t EntryCount() const
	{
		return m_entryCount;
	}

	/** Get the number of entries in use */
	uint32_t InUseCount() const
	{
		return m_entryCount - m_freeCount;
	}

	/** Get the first free element in the list (unordered!) */
	Entry* GetFirstFree() const
	{
		return m_freeHead;
	}

	/** Allocate an an entry */
	T* Allocate()
	{
		// List is full
		if( m_freeHead == nullptr )
			return nullptr;

		Entry* entry = m_freeHead;

		// Move to the next free element, then unlink this entry
		m_freeHead = entry->m_next;
		if( m_freeHead != nullptr )
		{
			m_freeHead->m_prev = nullptr;
			entry->m_next = nullptr;
		}

		// Push into the allocated list
		if( m_inUseHead != nullptr )
		{
			m_inUseHead->m_prev = entry;
		}

		entry->m_next = m_inUseHead;
		entry->m_mode = EM_InUse;
		m_inUseHead = entry;

		CCP_ASSERT( m_freeCount > 0 );
		--m_freeCount;

		return entry;
	}

	/** Free an entry */
	void Free( T* entryAsT )
	{
		Entry* entry = reinterpret_cast<Entry*>( entryAsT );
		ValidateEntry( entry );

		if( entry == nullptr || entry->m_mode == EM_Free )
			return;

		// Unlink from in-use neighbors
		if( entry->m_prev != nullptr )
		{
			entry->m_prev->m_next = entry->m_next;
		}

		if( entry->m_next != nullptr )
		{
			entry->m_next->m_prev = entry->m_prev;
		}

		// Also, ensure head reference is updated
		if( m_inUseHead == entry )
		{
			m_inUseHead = entry->m_next;
		}


		// Insert into free section
		if( m_freeHead != nullptr )
		{
			m_freeHead->m_prev = entry;
		}

		entry->m_next = m_freeHead;
		entry->m_prev = nullptr;
		entry->m_mode = EM_Free;
		m_freeHead = entry;

		m_freeCount++;
		CCP_ASSERT( m_freeCount <= m_entryCount );
	}

	/** Validate that an entry exists within this lists scope */
	void ValidateEntry( T* entry )
	{
		// The pointer should lie within our pre-allocated list
		uintptr_t entryPtr = reinterpret_cast<uintptr_t>( entry );
		for( auto& page : m_allEntries )
		{
			uintptr_t startPtr = reinterpret_cast<uintptr_t>( &page->front() );
			uintptr_t endPtr = reinterpret_cast<uintptr_t>( &page->back() );

			if( entryPtr >= startPtr && entryPtr <= endPtr )
			{
				return;
			}
		}
		CCP_ASSERT( false );
	}

private:
	uint32_t m_entryCount;
	uint32_t m_freeCount;
	std::vector<std::unique_ptr<std::vector<Entry>>> m_allEntries;
	Entry* m_freeHead;
	Entry* m_inUseHead;
};

#endif
