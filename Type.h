#pragma once

struct Symbol;
struct ScannerToken;
class ASTNode;

struct Type
{
	bool FromToken( const ScannerToken& token );
	bool FromTokenType( int type );
	bool FromSymbol( const Symbol* symbol );
	bool IsScalar() const;
	bool IsScalarOrVector() const;
	bool IsTexture() const;
	bool IsSampler() const;
	bool IsVector() const;
	bool CanImplicitCast( const Type& to, int& casts ) const;
	bool GetIndexedType( Type& type );

	bool GetMethodType( ASTNode* methodCall, Type& returnType ) const;

	std::string ToString() const;

	bool operator==( const Type& type ) const;
	bool operator!=( const Type& type ) const;

	const Symbol* symbol;
	int builtInType;
	int width;
	int height;
	Type* templateParameter;
	int templateSamples;
	int arrayDimensions;
	int arraySizes[8];

	int modifier;
	int storageClass;
};

bool GetCommonType( const Type& type0, const Type& type1, Type& type );
int GetNumericTypePrecedence( int type );
