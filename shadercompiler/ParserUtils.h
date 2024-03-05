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

extern int GetCBufferIndex( const InlineString& name );
extern int GetCBufferIndex( const Symbol* symbol );

extern void PatchCBuffers( ParserState& state );
extern bool HasRegisterBinding( const Symbol* symbol, const char* shaderProfile, char registerType, int registerNumber );

extern bool IsUniformInputArgument( ASTNode* argument );

extern void AssignRegisters( ASTNode* root, int32_t stage );

extern void SortProgramNodes( ASTNode* root );

extern void CreateGlobalsCB( ParserState& state );


ASTNode* NewStruct( ParserState& state, const InlineString& name = InlineString() );

ASTNode* NewVarIdentifier( ParserState& state, Symbol* var );
ASTNode* NewLiteralConst( ParserState& state, float value );
ASTNode* NewDot( ParserState& state, ASTNode* expr, Symbol* field );
ASTNode* NewDot( ParserState& state, ASTNode* expr, const InlineString& field );
ASTNode* NewBinaryExpression( ParserState& state, int op, ASTNode* left, ASTNode* right );
ASTNode* NewConditionalExpression( ParserState& state, ASTNode* condition, ASTNode* trueExpr, ASTNode* falseExpr );

ASTNode* NewVarDeclaration( ParserState& state, const Type& type, const InlineString& name = InlineString() );
ASTNode* NewExpressionStatement( ParserState& state, ASTNode* expr );
ASTNode* NewReturn( ParserState& state, ASTNode* expr = nullptr );

ASTNode* NewFunctionParameter( ParserState& state, const Type& type, const InlineString& name = InlineString() );
