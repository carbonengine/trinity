// Copyright © 2023 CCP ehf.

#pragma once

#include "ParserState.h"

struct Type;

enum class ShadingLanguage
{
	HLSL,
	MSL
};

class CodeStream : public std::stringstream
{
public:
	explicit CodeStream( const ShadingLanguage lang ) :
		shadingLanguage( lang )
	{
	}

	virtual void Endl() = 0;
	virtual void ChangeLocation( const FileLocation& location ) = 0;
	virtual void Indent() = 0;
	virtual void Unindent() = 0;

	ShadingLanguage shadingLanguage;
};

class CompilerInputStream : public CodeStream
{
public:
	CompilerInputStream( ParserState& state, const ShadingLanguage lang );

	void Endl() override;
	void ChangeLocation( const FileLocation& location ) override;
	void Indent() override;
	void Unindent() override;

private:
	ParserState& m_state;
	FileLocation m_location;
};

template <typename T>
inline CodeStream& operator<<( CodeStream& os, const T& t )
{
	static_cast<std::ostream&>( os ) << t;
	return os;
}

inline CodeStream& operator<<( CodeStream& os, const FileLocation& location )
{
	os.ChangeLocation( location );
	return os;
}

struct Indent
{
};
struct Unindent
{
};
struct Endl
{
};

inline CodeStream& operator<<( CodeStream& os, const Indent& )
{
	os.Indent();
	return os;
}

inline CodeStream& operator<<( CodeStream& os, const Unindent& )
{
	os.Unindent();
	return os;
}

inline CodeStream& operator<<( CodeStream& os, const Endl& )
{
	os.Endl();
	return os;
}

struct HLSL
{
	ASTNode* node;
	SymbolTable* symbolTable;
};

struct MSL
{
	ASTNode* node;
	SymbolTable* symbolTable;
};

CodeStream& operator<<( CodeStream& os, const HLSL& );
CodeStream& operator<<( CodeStream& os, const MSL& );
CodeStream& operator<<( CodeStream& os, const Type& type );
