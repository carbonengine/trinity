// Copyright © 2023 CCP ehf.

#pragma once

#include "SymbolTable.h"

class ParserState;




enum PatchAction
{
	PATCH_ERROR,
	PATCH_SKIP,
	PATCH_USE,
};


struct GlobalInputElement
{
	Type type;
	InlineString name;
	ASTNode* declaration;
	Symbol* symbol = nullptr;

	bool operator==( const GlobalInputElement& other ) const
	{
		return type == other.type && name == other.name;
	}
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

extern void AssignRegisters( ASTNode* root, int32_t stage, const std::vector<GlobalInputElement>& globalInput = {} );

extern void SortProgramNodes( ASTNode* root );

extern void CreateGlobalsCB( ParserState& state );


ASTNode* NewStruct( ParserState& state, const InlineString& name = InlineString() );

ASTNode* NewVarIdentifier( ParserState& state, Symbol* var );
ASTNode* NewLiteralConst( ParserState& state, float value );
ASTNode* NewLiteralConst( ParserState& state, uint32_t value );
ASTNode* NewLiteralConst( ParserState& state, bool value );
ASTNode* NewDot( ParserState& state, ASTNode* expr, Symbol* field );
ASTNode* NewDot( ParserState& state, ASTNode* expr, const InlineString& field );
ASTNode* NewBinaryExpression( ParserState& state, int op, ASTNode* left, ASTNode* right );
ASTNode* NewConditionalExpression( ParserState& state, ASTNode* condition, ASTNode* trueExpr, ASTNode* falseExpr );

ASTNode* NewFunctionCall( ParserState& state, const Type& type, const char* name, const std::vector<ASTNode*>& args );
ASTNode* NewCastExpression( ParserState& state, const Type& type, ASTNode* child );

ASTNode* NewVarDeclaration( ParserState& state, const Type& type, const InlineString& name = InlineString() );
ASTNode* NewVarDeclaration( ParserState& state, Symbol* symbol, ASTNode* initializer = nullptr );
ASTNode* NewExpressionStatement( ParserState& state, ASTNode* expr );
ASTNode* NewReturn( ParserState& state, ASTNode* expr = nullptr );

ASTNode* NewFunctionParameter( ParserState& state, const Type& type, const InlineString& name = InlineString() );
ASTNode* NewFunctionParameter( ParserState& state, const Type& type, const char* name );
ASTNode* NewFunctionParameter( ParserState& state, const Type& type, const char* name, const RegisterSpecifier& reg );


std::optional<std::vector<GlobalInputElement>> ParseGlobalInput( ParserState& state, const ScannerToken& input );
bool ProcessGlobalInputAttribute( ParserState& state, ASTNode* func, std::vector<GlobalInputElement>& globalInput );
