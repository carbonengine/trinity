#pragma once

#include <cstring>
#include <ostream>

struct InlineString
{
	const char* start;
	const char* end;

	operator bool() const { return start != nullptr; }
};

inline InlineString MakeInlineString( const char* start, const char* end )
{
	InlineString result;
	result.start = start;
	result.end = end;
	return result;
}

inline InlineString MakeInlineString( const char* string )
{
	InlineString result;
	result.start = string;
	result.end = string + strlen( string );
	return result;
}

inline std::ostream& operator<<( std::ostream& os, const InlineString& string )
{
	return os.write( string.start, string.end - string.start );
}

inline bool operator==( const InlineString& str0, const InlineString& str1 )
{
	size_t len0 = str0.end - str0.start;
	size_t len1 = str1.end - str1.start;
	if( len0 != len1 )
	{
		return false;
	}
	return strncmp( str0.start, str1.start, len0 ) == 0;
}

inline bool operator!=( const InlineString& str0, const InlineString& str1 )
{
	return !( str0 == str1 );
}

inline bool operator<( const InlineString& str0, const InlineString& str1 )
{
	size_t len0 = str0.end - str0.start;
	size_t len1 = str1.end - str1.start;
	int cmp = strncmp( str0.start, str1.start, __min( len0, len1 ) );
	if( cmp < 0 )
	{
		return true;
	}
	if( cmp == 0 )
	{
		if( len0 < len1 )
		{
			return true;
		}
		return false;
	}
	return false;
}

inline std::string ToString( const InlineString& string )
{
	std::string ret( string.start, string.end );
	return ret;
}

inline bool ContainsSubstring( const InlineString& name, const char* substr )
{
	size_t len = strlen( substr );
	for( const char* s = name.start; s + len <= name.end; ++s )
	{
		if( strncmp( s, substr, len ) == 0 )
		{
			return true;
		}
	}
	return false;
}
