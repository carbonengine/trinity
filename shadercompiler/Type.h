// Copyright © 2023 CCP ehf.

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
	bool IsTextureArray() const;
	bool IsSampler() const;
	bool IsBuffer() const;
	bool IsVector() const;
	bool IsMatrix() const;
	bool IsStruct() const;
	bool IsBindlessHandle() const;
	bool CanImplicitCast( const Type& to, int& casts ) const;
	bool GetIndexedType( Type& type ) const;

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
	int metalTextureAccess;
	bool isDepthTexture;
};


Type TypeFromSymbol( const Symbol* symbol );
Type TypeFromTokenType( int type, int width = 1, int height = 1 );

bool GetCommonType( const Type& type0, const Type& type1, Type& type );
int GetNumericTypePrecedence( int type );

namespace hlsl
{

extern const Type void_t;

extern const Type bool_t;
extern const Type int_t;
extern const Type uint_t;
extern const Type float_t;

extern const Type bool2_t;
extern const Type int2_t;
extern const Type uint2_t;
extern const Type float2_t;

extern const Type bool3_t;
extern const Type int3_t;
extern const Type uint3_t;
extern const Type float3_t;

extern const Type bool4_t;
extern const Type int4_t;
extern const Type uint4_t;
extern const Type float4_t;

}
