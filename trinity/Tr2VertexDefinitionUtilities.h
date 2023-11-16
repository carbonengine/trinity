////////////////////////////////////////////////////////////
//
//    Created:   July 2013
//    Copyright: CCP 2013
//

#pragma once
#ifndef Tr2VertexDefinitionUtilities_H
#define Tr2VertexDefinitionUtilities_H


// Take a granny vertex definition and build a Tr2VertexDefinition out of it.
Tr2VertexDefinition BuildFromGrannyVertexDecl( const granny_data_type_definition* grannyVertexDecl );

// Convert a vertex definition back to a granny layout
// Arguments:
// vd - input definition
// grannyVertexDecl - pointer to at least maxSize elements
// maxSize - size of grannyVertexDecl array
bool ConvertVertexDeclToGranny( Tr2VertexDefinition vd, granny_data_type_definition* grannyVertexDecl, unsigned maxSize );

Tr2VertexDefinition::DataType ConvertGrannyTypeToDataType( const granny_data_type_definition& src );


#endif