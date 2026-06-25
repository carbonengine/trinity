// Copyright © 2010 CCP ehf.

#pragma once
#ifndef Tr2MaterialRes_H
#define Tr2MaterialRes_H

BLUE_DECLARE( Tr2MaterialArea );
BLUE_DECLARE( Tr2MaterialRes );
BLUE_DECLARE( Tr2MaterialParameterStore );

// --------------------------------------------------------------------------------------
// Description:
//   Tr2MaterialArea describes an area on a mesh, and provides hints at how it should be propagated
//		to tr2MeshAreas
// --------------------------------------------------------------------------------------


//================================================================
class Tr2MaterialArea : public IRoot
{
public:
	Tr2MaterialArea( IRoot* lockobj = NULL ) {};
	virtual ~Tr2MaterialArea() {};

	EXPOSE_TO_BLUE();

protected:
	//	The name of the metamaterial to use for this area
	std::string m_metaType;

	//	The material overrides applied to this area
	Tr2MaterialParameterStorePtr m_material;
};

TYPEDEF_BLUECLASS( Tr2MaterialArea );
typedef BlueDict<Tr2MaterialArea> Tr2MaterialAreaDict;
TYPEDEF_BLUECLASS( Tr2MaterialAreaDict );


//================================================================
class Tr2MaterialMesh : public IRoot
{
public:
	EXPOSE_TO_BLUE();

	Tr2MaterialMesh( IRoot* lockobj = NULL );
	virtual ~Tr2MaterialMesh();

public:
	PTr2MaterialAreaDict m_areas;
};

TYPEDEF_BLUECLASS( Tr2MaterialMesh );
BLUE_DECLARE_VECTOR( Tr2MaterialMesh );

typedef BlueDict<Tr2MaterialMesh> Tr2MaterialMeshDict;
TYPEDEF_BLUECLASS( Tr2MaterialMeshDict );

//================================================================

class Tr2MaterialRes : public IRoot
{
public:
	EXPOSE_TO_BLUE();

	Tr2MaterialRes( IRoot* lockobj = NULL );
	virtual ~Tr2MaterialRes();

public:
	std::string m_name;

	PTr2MaterialMeshDict m_meshes;
};

TYPEDEF_BLUECLASS( Tr2MaterialRes );
BLUE_DECLARE_VECTOR( Tr2MaterialRes );

#endif
