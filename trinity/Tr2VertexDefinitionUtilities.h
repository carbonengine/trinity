// Copyright © 2013 CCP ehf.

#pragma once
#ifndef Tr2VertexDefinitionUtilities_H
#define Tr2VertexDefinitionUtilities_H


#if WITH_GRANNY
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




// Take a cmf vertex definition and build a Tr2VertexDefinition out of it.
Tr2VertexDefinition BuildFromCMFVertexDecl( const cmf::Span<cmf::VertexElement>& cmfVertexDecl );

// Convert a vertex definition back to a cmf layout
// Arguments:
// vd - input definition
// cmfVertexDecl - pointer to at least maxSize elements
// maxSize - size of cmfVertexDecl array
bool ConvertVertexDeclToCMF( Tr2VertexDefinition vd, cmf::Span<cmf::VertexElement>& cmfVertexDecl, unsigned maxSize );

Tr2VertexDefinition::DataType ConvertCMFTypeToDataType( const cmf::VertexElement& cmfVertexDecl );


#endif