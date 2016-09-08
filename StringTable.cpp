#include "stdafx.h"
#include "StringTable.h"

StringTable::StringTable()
{
	InitializeCriticalSection( &m_CS );
}

StringTable::~StringTable()
{
	DeleteCriticalSection( &m_CS );
}

StringReference StringTable::AddString( const char* string )
{
	return AddString( string, strlen( string ) + 1 );
}

StringReference StringTable::AddString( const void* string, size_t length )
{
	Blob blob( string, length );
	EnterCriticalSection( &m_CS );
	auto it = m_table.find( blob );
	if( it != m_table.end() )
	{
		StringReference result = it->second;
		LeaveCriticalSection( &m_CS );
		return result;
	}
	StringReference index = StringReference( m_size );
	m_size += blob.m_size;
	m_table[blob] = index;
	LeaveCriticalSection( &m_CS );
	return index;
}

const char* StringTable::GetString( StringReference ref )
{
	for( auto it = m_table.begin(); it != m_table.end(); ++it )
	{
		if( it->second == ref )
		{
			return static_cast<const char*>( it->first.m_data );
		}
	}
	return nullptr;
}

size_t StringTable::GetSize() const
{
	return m_size + sizeof( DWORD );
}

bool StringTable::Write( HANDLE file ) const
{
	DWORD bytesWritten = 0;
	DWORD size = m_size;
	if( !WriteFile( file, &size, sizeof( size ), &bytesWritten, nullptr ) || bytesWritten != sizeof( size ) )
	{
		return false;
	}

	if( m_size == 0 )
	{
		return true;
	}

	char* buffer = new char[m_size];
	for( auto it = m_table.begin(); it != m_table.end(); ++it )
	{
		memcpy( buffer + it->second, it->first.m_data, it->first.m_size );
	}
	if( !WriteFile( file, buffer, m_size, &bytesWritten, nullptr ) || bytesWritten != m_size )
	{
		delete[] buffer;
		return false;
	}
	delete[] buffer;
	return true;
}
