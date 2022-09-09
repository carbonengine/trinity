////////////////////////////////////////////////////////////
//
//    Created:   August 2014
//    Copyright: CCP 2014
//
#pragma once
#ifndef StringUtils_H
#define StringUtils_H

// --------------------------------------------------------------------------------
// Description:
//   Inserts insertStr before the last instance of beforeSubstr in baseString.
// --------------------------------------------------------------------------------
inline bool StringInsertStubBefore( std::string& baseString, const char* beforeSubstr, const char* insertStr )
{
	size_t index = baseString.rfind( beforeSubstr );
	if( index == std::string::npos )
	{
		return false;
	}

	baseString.insert( index, insertStr );
	return true;
}

// --------------------------------------------------------------------------------
// Description:
//   Inserts insertStr after the last instance of afterSubstr in baseString.
// --------------------------------------------------------------------------------
inline bool StringInsertStubAfter( std::string& baseString, const char* afterSubstr, const char* insertStr )
{
	size_t index = baseString.rfind( afterSubstr );
	if( index == std::string::npos )
	{
		return false;
	}

	baseString.insert( index + strlen( afterSubstr ), insertStr );
	return true;
}

// --------------------------------------------------------------------------------
// Description:
//   Split up a string into a vector of strings using a seperator.
//   Similar to Python's split()
// --------------------------------------------------------------------------------
inline void StringSplit( std::vector<std::string>& result, const char* original, char seperator )
{
	std::string originalStr( original );
	for( size_t i = 0; i < originalStr.size(); ++i )
	{
		size_t next = originalStr.find( seperator, i );
		if( next == std::string::npos )
		{
			next = originalStr.size();
		}

		result.push_back( originalStr.substr( i, next - i ) );

		i = next;
	}
}

// --------------------------------------------------------------------------------
// Description:
//   Remove a subpart from a string
// --------------------------------------------------------------------------------
inline bool StringRemove( std::string& baseString, const char* remove )
{
	// find what to remove
	size_t removeStart = baseString.find( remove );
	if( removeStart != std::string::npos )
	{
		baseString.erase( removeStart, strlen( remove ) );
		return true;
	}
	return false;
}

// --------------------------------------------------------------------------------
// Description:
//   Check if a string starts with another string! Case-insensitve!
//   Similar to Python's starts_with()
// --------------------------------------------------------------------------------
inline bool StringStartsWithI( const char* baseString, const char* startString )
{
	while( *startString )
	{
		if( tolower( *startString ) != tolower( *baseString ) )
		{
			return false;
		}
		++baseString;
		++startString;
	}
	return true;
}

// --------------------------------------------------------------------------------
// Description:
//   Try to find a substring
//   Similar to Python's x in y
// --------------------------------------------------------------------------------
inline bool StringFind( const char* baseString, const char* searchString )
{
	std::string b( baseString );
	return ( b.find( searchString ) != std::string::npos );
}

// --------------------------------------------------------------------------------
// Description:
//   Replace substring
//   Similar to Python's replace()
// --------------------------------------------------------------------------------
inline bool StringReplace( std::string& baseString, const char* oldString, const char* newString )
{
	// find what to replace
	size_t replaceStart = baseString.find( oldString );
	if( replaceStart != std::string::npos )
	{
		baseString.replace( replaceStart, strlen( oldString ), std::string( newString ) );
		return true;
	}
	return false;
}




#endif // StringUtils_H