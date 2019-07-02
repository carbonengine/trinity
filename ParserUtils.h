#pragma once

#include "SymbolTable.h"

class ParserState;




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

extern void AssignRegisters( ASTNode* root, int32_t stage );

extern void SortProgramNodes( ASTNode* root );

extern void CreateGlobalsCB( ParserState& state, int32_t stage );