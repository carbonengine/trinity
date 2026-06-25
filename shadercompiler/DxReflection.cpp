// Copyright © 2023 CCP ehf.

#include "stdafx.h"
#if _WIN32
#include "DxReflection.h"
#include "HLSLParser.h"
#include "ParserUtils.h"


namespace DxReflection
{

bool MakeEffectAnnotationFromSymbolAnnotation( const SymbolAnnotation& annotation, Annotation& result, bool& isSRGB, bool& isAutoregister )
{
	switch( annotation.type )
	{
	case OP_FLOAT:
	case OP_HALF:
	case OP_DOUBLE:
		result.type = ANNOTATION_TYPE_FLOAT;
		result.floatValue = float( ParseFloat( annotation.value.stringValue.start, annotation.value.stringValue.end ) );
		return true;

	case OP_UINT:
	case OP_INT:
		result.type = ANNOTATION_TYPE_INT;
		result.intValue = ParseNumber( annotation.value.stringValue.start, annotation.value.stringValue.end );
		return true;

	case OP_BOOL:
		result.type = ANNOTATION_TYPE_BOOL;
		result.intValue = annotation.value.intValue ? 1 : 0;
		if( ToString( annotation.name ) == "Tr2sRGB" )
		{
			isSRGB = result.intValue != 0;
		}
		else if( ToString( annotation.name ) == "AutoRegister" )
		{
			isAutoregister = result.intValue != 0;
		}
		return true;

	case OP_STRING:
		result.type = ANNOTATION_TYPE_STRING;
		result.stringValue = g_stringTable.AddString( ParseString( annotation.value.stringValue ).c_str() );
		return true;
	}

	return false;
}

}
#endif