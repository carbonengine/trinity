#include "stdafx.h"
#include "StringTable.h"

const StringReference INVALID_REFERENCE = StringTable::GetInvalidReference();


StringTable::StringTable()
	:m_sorted( true )
{
}

StringTable::~StringTable()
{
}

StringReference StringTable::AddString( const char* string )
{
	return AddString( string, strlen( string ) + 1 );
}

StringReference StringTable::AddString( const void* string, size_t length )
{
	Blob blob( string, length );
	std::lock_guard scope( m_CS );
	auto it = m_table.find( &blob );
	if( it != m_table.end() )
	{
		StringReference result = it->second;
		return result;
	}
	StringReference index;
	index.reference = uint32_t( m_size );
	m_size += blob.m_size;
	auto blobPtr = new Blob( blob );
	m_table[blobPtr] = index;
	m_revTable[index] = std::make_pair( blobPtr, 0 );
	m_sorted = false;
	return index;
}

StringReference StringTable::AddString( const InlineString& string )
{
	auto length = string.end - string.start;
	std::unique_ptr<char[]> str( new char[length + 1] );
	memcpy( str.get(), string.start, length );
	str.get()[length] = 0;
	return AddString( str.get(), length + 1 );
}

uint32_t StringTable::GetOffset( StringReference ref )
{
	if( ref.reference == uint32_t( -1 ) )
	{
		return 0xffffffff;
	}
	std::lock_guard scope( m_CS );
	if( !m_sorted )
	{
		Sort();
	}
	auto it = m_revTable.find( ref );
	if( it == m_revTable.end() )
	{
		return 0xffffffff;
	}
	return uint32_t( it->second.second );
}

const char* StringTable::GetString( StringReference ref )
{
	std::lock_guard scope( m_CS );
	auto it = m_revTable.find( ref );
	if( it == m_revTable.end() )
	{
		return nullptr;
	}
	return static_cast<const char*>( it->second.first->m_data );
}

size_t StringTable::GetSize() const
{
	return m_size + sizeof( uint32_t );
}

bool StringTable::ValueCompare( std::pair<Blob*, size_t>* a, std::pair<Blob*, size_t>* b )
{
	return *a->first < *b->first;
}

void StringTable::Sort()
{
	std::vector<std::pair<Blob*, size_t>*> values;
	values.reserve( m_revTable.size() );
	for( auto it = begin( m_revTable ); it != end( m_revTable ); ++it )
	{
		values.push_back( &it->second );
	}
	std::sort( begin( values ), end( values ), ValueCompare );
	size_t offset = 0;
	for( auto it = begin( values ); it != end( values ); ++it )
	{
		( *it )->second = offset;
		offset += ( *it )->first->m_size;
	}
	m_sorted = true;
}

bool StringTable::Write( FILE* file )
{
	std::lock_guard scope( m_CS );

	if( !m_sorted )
	{
		Sort();
	}

	uint32_t size = uint32_t( m_size );
	if( fwrite( &size, 1, sizeof( size ), file ) != sizeof( size ) )
	{
		return false;
	}

	if( m_size == 0 )
	{
		return true;
	}

	char* buffer = new char[m_size];
	for( auto it = begin( m_revTable ); it != end( m_revTable ); ++it )
	{
		memcpy( buffer + it->second.second, it->second.first->m_data, it->second.first->m_size );
	}

	if( fwrite( buffer, 1, m_size, file ) != m_size )
	{
		delete[] buffer;
		return false;
	}
	delete[] buffer;
	return true;
}


StringReference StringTable::GetInvalidReference()
{
	StringReference ref;
	ref.reference = 0xffffffff;
	return ref;
}
