#pragma once
#ifndef StringTable_H
#define StringTable_H

#include <unordered_map>

typedef unsigned StringReference;

class StringTable
{
public:
	StringTable();
	~StringTable();

	StringReference AddString( const char* string );
	StringReference AddString( const void* string, size_t length );
	const char* GetString( StringReference ref );
	bool Write( HANDLE file ) const;
	size_t GetSize() const;
private:
	CRITICAL_SECTION m_CS;

	struct Blob
	{
		Blob( const void* data, size_t size )
			:m_size( size )
		{
			if( size )
			{
				m_data = new char[size];
				memcpy( m_data, data, size );
			}
			else
			{
				m_data = nullptr;
			}
		}

		Blob( const Blob& other )
			:m_size( other.m_size )
		{
			if( other.m_data )
			{
				m_data = new char[m_size];
				memcpy( m_data, other.m_data, m_size );
			}
			else
			{
				m_data = nullptr;
			}
		}

		Blob( Blob&& other )
			:m_data( other.m_data ),
			m_size( other.m_size )
		{
			other.m_data = nullptr;
		}

		~Blob()
		{
			delete[] m_data;
		}

		Blob& operator=( const Blob& other )
		{
			if( &other == this )
			{
				return *this;
			}
			delete[] m_data;
			m_size = other.m_size;
			if( other.m_data )
			{
				m_data = new char[m_size];
				memcpy( m_data, other.m_data, m_size );
			}
			else
			{
				m_data = nullptr;
			}
		}

		bool operator==( const Blob& other ) const
		{
			return m_size == other.m_size && memcmp( m_data, other.m_data, m_size ) == 0;
		}

		operator size_t() const
		{
			size_t result = 2166136261U;
			size_t stride = 1 + m_size / 10;

			for( size_t i = 0; i < m_size; i += stride )
			{
				result = 16777619U * result ^ (size_t)static_cast<const char*>( m_data )[i];
			}
			return result;
		}

		void* m_data;
		size_t m_size;
	};

	std::unordered_map<Blob, StringReference> m_table;
	size_t m_size;
};

#endif // StringTable_H