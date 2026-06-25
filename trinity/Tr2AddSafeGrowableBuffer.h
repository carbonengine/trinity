// Copyright © 2023 CCP ehf.

#pragma once
#ifndef Tr2AddSafeGrowableBuffer_H
#define Tr2AddSafeGrowableBuffer_H

#include "TbbStub.h"

// --------------------------------------------------------------------------------------
// Description:
//   Helper class to store a growable array of objects. A super-limited version of
//   std::vector, but it allows appending new objects in parallel. All other operations
//   are not thread safe.
// --------------------------------------------------------------------------------------
template <typename T>
class Tr2AddSafeGrowableBuffer
{
public:
	Tr2AddSafeGrowableBuffer( uint32_t initialCount, const char* allocationName );
	void Clear();
	bool Add( const T& element, const char* allocationName );
	uint32_t GetCount() const;
	const T* GetData() const;
	T* GetData();

	const T& operator[]( size_t index ) const;
	T& operator[]( size_t index );
	const T* begin() const;
	T* begin();
	const T* end() const;
	T* end();

private:
	CcpMallocBuffer m_data;
	Tr2SpinMutex m_mutex;
	uint32_t m_count;
	uint32_t m_capacity;
};

template <typename T>
const T* begin( const Tr2AddSafeGrowableBuffer<T>& buffer )
{
	return buffer.begin();
}

template <typename T>
T* begin( Tr2AddSafeGrowableBuffer<T>& buffer )
{
	return buffer.begin();
}

template <typename T>
const T* end( const Tr2AddSafeGrowableBuffer<T>& buffer )
{
	return buffer.GetData() + buffer.GetCount();
}

template <typename T>
T* end( Tr2AddSafeGrowableBuffer<T>& buffer )
{
	return buffer.GetData() + buffer.GetCount();
}




template <typename T>
Tr2AddSafeGrowableBuffer<T>::Tr2AddSafeGrowableBuffer( uint32_t initialCount, const char* allocationName ) :
	m_capacity( initialCount ),
	m_count( 0 )
{
	m_data.resize( allocationName, m_capacity * sizeof( T ) );
}

template <typename T>
void Tr2AddSafeGrowableBuffer<T>::Clear()
{
	m_count = 0;
}

template <typename T>
bool Tr2AddSafeGrowableBuffer<T>::Add( const T& element, const char* allocationName )
{
	Tr2SpinMutex::scoped_lock lock( m_mutex );

	uint32_t index = m_count++;
	if( index == m_capacity )
	{
		m_data.resize( allocationName, m_capacity * 2 * sizeof( T ) );
		m_capacity = m_capacity * 2;
	}

	if( !m_data.get() )
	{
		return false;
	}
	reinterpret_cast<T*>( m_data.get() )[index] = element;
	return true;
}

template <typename T>
uint32_t Tr2AddSafeGrowableBuffer<T>::GetCount() const
{
	return m_count;
}

template <typename T>
const T* Tr2AddSafeGrowableBuffer<T>::GetData() const
{
	return reinterpret_cast<const T*>( m_data.get() );
}

template <typename T>
T* Tr2AddSafeGrowableBuffer<T>::GetData()
{
	return reinterpret_cast<T*>( m_data.get() );
}

template <typename T>
const T& Tr2AddSafeGrowableBuffer<T>::operator[]( size_t index ) const
{
	return this->GetData()[index];
}

template <typename T>
T& Tr2AddSafeGrowableBuffer<T>::operator[]( size_t index )
{
	return this->GetData()[index];
}

template <typename T>
const T* Tr2AddSafeGrowableBuffer<T>::begin() const
{
	return this->GetData();
}

template <typename T>
T* Tr2AddSafeGrowableBuffer<T>::begin()
{
	return this->GetData();
}

template <typename T>
const T* Tr2AddSafeGrowableBuffer<T>::end() const
{
	// cppcheck-suppress arithOperationsOnVoidPointer
	return this->GetData() + this->GetCount();
}

template <typename T>
T* Tr2AddSafeGrowableBuffer<T>::end()
{
	// cppcheck-suppress arithOperationsOnVoidPointer
	return this->GetData() + this->GetCount();
}

#endif