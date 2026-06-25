// Copyright © 2023 CCP ehf.

#pragma once
#ifndef StringTable_H
#define StringTable_H

#include "InlineString.h"

class StringReference
{
public:
	bool operator==( const StringReference& other ) const
	{
		return reference == other.reference;
	}
	bool operator<( const StringReference& other ) const
	{
		return reference < other.reference;
	}

private:
	uint32_t reference;
	friend class StringTable;
};

extern const StringReference INVALID_REFERENCE;

class StringTable
{
public:
	StringTable();
	~StringTable();

	StringReference AddString( const char* string );
	StringReference AddString( const void* string, size_t length );
	StringReference AddString( const InlineString& string );
	uint32_t GetOffset( StringReference ref );
	const char* GetString( StringReference ref );
	bool Write( FILE* file );
	size_t GetSize() const;

	static StringReference GetInvalidReference();

private:
	struct Blob
	{
		Blob( const void* data, size_t size ) :
			m_data( nullptr ),
			m_size( size )
		{
			if( size )
			{
				m_data = new char[size];
				memcpy( m_data, data, size );
			}
		}

		Blob( const Blob& other ) :
			m_size( other.m_size )
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

		Blob( Blob&& other ) :
			m_data( other.m_data ),
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
			return *this;
		}

		bool operator==( const Blob& other ) const
		{
			return m_size == other.m_size && memcmp( m_data, other.m_data, m_size ) == 0;
		}

		bool operator<( const Blob& other ) const
		{
			auto length = std::min( m_size, other.m_size );
			auto cmp = memcmp( m_data, other.m_data, length );
			if( cmp < 0 )
			{
				return true;
			}
			if( cmp > 0 )
			{
				return false;
			}
			return m_size < other.m_size;
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

		char* m_data;
		size_t m_size;
	};

	struct BlobPtrHash
	{
		size_t operator()( const Blob* ptr ) const
		{
			return size_t( *ptr );
		}
	};

	struct BlobPtrEqual
	{
		bool operator()( const Blob* a, const Blob* b ) const
		{
			return *a == *b;
		}
	};

	struct StringReferenceHash
	{
		size_t operator()( StringReference ref ) const
		{
			return size_t( ref.reference );
		}
	};

	void Sort();
	static bool ValueCompare( std::pair<Blob*, size_t>* a, std::pair<Blob*, size_t>* b );

	std::mutex m_CS;
	std::unordered_map<Blob*, StringReference, BlobPtrHash, BlobPtrEqual> m_table;
	std::unordered_map<StringReference, std::pair<Blob*, size_t>, StringReferenceHash> m_revTable;
	size_t m_size;
	bool m_sorted;
};

#endif // StringTable_H
