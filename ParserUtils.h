#pragma once

#include <strstream>
#include "SymbolTable.h"

class ParserState;


class CodeStream: public std::ostrstream
{
public:
	virtual void Endl() = 0;
	virtual void ChangeLocation( const FileLocation& location ) = 0;
};

class CompilerInputStream: public CodeStream
{
public:
	CompilerInputStream( ParserState& state );

	void Endl();
	void ChangeLocation( const FileLocation& location );
private:
	ParserState& m_state;
	FileLocation m_location;
};

class ListingStream: public CodeStream
{
	void Endl() { put( '\n' ); flush(); }
	void ChangeLocation( const FileLocation& location ) {}
};

class LineMapInputStream: public CodeStream
{
public:
	LineMapInputStream( ParserState& state );

	void Endl();
	void ChangeLocation( const FileLocation& location );
	bool GetLocation( unsigned lineNumber, FileLocation& location );
private:
	ParserState& m_state;
	FileLocation m_location;
	unsigned m_lineNumber;
	std::vector<std::pair<unsigned, FileLocation>> m_locations;
};

template<typename T>
T& operator<<( T& os, const FileLocation& location )
{
	os.ChangeLocation( location );
	return os;
}

enum PatchAction
{
	PATCH_ERROR,
	PATCH_SKIP,
	PATCH_USE,
};



extern long ParseNumber( const char* start, const char* end );
extern bool ParseRegisterID( const char* start, const char* end, char& registerType, int& registerNumber );
extern double ParseFloat( const char* start, const char* end );
extern std::string ParseString( const InlineString& string );

extern std::string ToString( const InlineString& string );

extern void MarkUsedSymbols( ASTNode* entryPoint, ParserState& state );

extern bool ComputeMemberType( const Type& leftType, const InlineString& member, Type& type, Symbol*& symbol );

extern unsigned GetCBufferIndex( Symbol* symbol );

extern void PatchCBuffers( ParserState& state );
extern bool HasRegisterBinding( Symbol* symbol, const char* shaderProfile, char registerType, int registerNumber );

extern bool IsUniformInputArgument( ASTNode* argument );
