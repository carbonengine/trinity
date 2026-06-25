// Copyright © 2025 CCP ehf.

#include "StdAfx.h"
#include "Base64Encoding.h"

namespace Base64
{

std::vector<uint8_t> Decode( const std::string_view& str )
{
	std::vector<uint8_t> result;
	result.reserve( ( str.size() / 4 ) * 3 );

	auto Index = []( char p ) -> uint8_t {
		if( p >= 'A' && p <= 'Z' )
		{
			return p - 'A';
		}
		else if( p >= 'a' && p <= 'z' )
		{
			return p - 'a' + 26;
		}
		else if( p >= '0' && p <= '9' )
		{
			return p - '0' + 52;
		}
		else if( p == '+' )
		{
			return 62;
		}
		else if( p == '/' )
		{
			return 63;
		}
		return 64;
	};

	char quad[4];
	size_t q = 0;

	for( size_t i = 0; i < str.size() && str[i] != '='; i++ )
	{
		quad[q] = Index( str[i] );
		if( quad[q] == 64 )
		{
			return {};
		}
		q++;
		if( q == 4 )
		{
			q = 0;
			result.push_back( ( ( quad[0] << 2 ) | ( quad[1] >> 4 ) ) & 0xFF );
			result.push_back( ( ( quad[1] & 0x0F ) << 4 ) | ( quad[2] >> 2 ) );
			result.push_back( ( ( quad[2] & 0x03 ) << 6 ) | quad[3] );
		}
	}
	if( q > 0 )
	{
		for( size_t i = q; i < 4; i++ )
		{
			quad[i] = 0;
		}
		result.push_back( ( ( quad[0] << 2 ) | ( quad[1] >> 4 ) ) & 0xFF );
		if( q > 2 )
		{
			result.push_back( ( ( quad[1] & 0x0F ) << 4 ) | ( quad[2] >> 2 ) );
		}
		if( q > 3 )
		{
			result.push_back( ( ( quad[2] & 0x03 ) << 6 ) | quad[3] );
		}
	}

	return result;
}

std::string Encode( const uint8_t* data, size_t size )
{
	const char* base64 =
		"ABCDEFGHIJKLMNOPQRSTUVWXYZ"
		"abcdefghijklmnopqrstuvwxyz"
		"0123456789"
		"+/";

	std::string result;
	result.reserve( ( ( size + 2 ) / 3 ) * 4 );

	for( size_t i = 0; i < size; i += 3 )
	{
		uint32_t triple = ( data[i] << 16 ) |
			( ( i + 1 < size ? data[i + 1] : 0 ) << 8 ) |
			( i + 2 < size ? data[i + 2] : 0 );

		result.push_back( base64[( triple >> 18 ) & 0x3F] );
		result.push_back( base64[( triple >> 12 ) & 0x3F] );
		result.push_back( i + 1 < size ? base64[( triple >> 6 ) & 0x3F] : '=' );
		result.push_back( i + 2 < size ? base64[triple & 0x3F] : '=' );
	}

	return result;
}

}
