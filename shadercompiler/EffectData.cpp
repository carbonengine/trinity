// Copyright © 2023 CCP ehf.

#include "stdafx.h"
#include "EffectData.h"

const char* GetStringForUsageCode( int usageCode )
{
	static const char* str[] = {
		"POSITION",
		"COLOR",
		"NORMAL",
		"TANGENT",
		"BINORMAL",
		"TEXCOORD",
		"BLENDINDICES",
		"BLENDWEIGHT"
	};

	assert( 0 <= usageCode && usageCode < int( sizeof( str ) / sizeof( *str ) ) );
	return str[usageCode];
}

const char* ToString( ConstantType constType )
{
	switch( constType )
	{
	case CONSTANT_TYPE_FLOAT:
		return "float";
	case CONSTANT_TYPE_INT:
		return "int";
	case CONSTANT_TYPE_UINT:
		return "uint";
	case CONSTANT_TYPE_BOOL:
		return "bool";
	default:
		return "other (struct, etc.)";
	}
}
