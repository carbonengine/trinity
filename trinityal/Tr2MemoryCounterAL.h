// Copyright © 2023 CCP ehf.

#pragma once

#include "Tr2HalHelperStructures.h"

class Tr2MemoryCounterAL
{
public:
	enum MemoryType
	{
		SYSTEM_MEMORY,

		BUFFER,
		TEXTURE,
		OTHER,
	};

	Tr2MemoryCounterAL();
	Tr2MemoryCounterAL( Tr2MemoryCounterAL&& other );
	~Tr2MemoryCounterAL();

	Tr2MemoryCounterAL& operator=( Tr2MemoryCounterAL&& other );

	void Set( MemoryType memoryType, size_t size );
	void Set( MemoryType memoryType, const Tr2BitmapDimensions& bitmap, const Tr2MsaaDesc& msaa = Tr2MsaaDesc() );
	void Grow( size_t size );
	void Shrink( size_t size );
	void Reset();

private:
	Tr2MemoryCounterAL( const Tr2MemoryCounterAL& ) /* = delete */;
	Tr2MemoryCounterAL& operator=( const Tr2MemoryCounterAL& ) /* = delete */;

	void UpdateCounters( int32_t inc ) const;

	MemoryType m_memoryType;
	size_t m_size;
};
