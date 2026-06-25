// Copyright © 2010 CCP ehf.

#include "StdAfx.h"
#include "Tr2MaterialRes.h"

//================================================================
Tr2MaterialMesh::Tr2MaterialMesh( IRoot* lockobj ) :
	PARENTLOCK( m_areas )
{
}

//================================================================
Tr2MaterialMesh::~Tr2MaterialMesh()
{
}


//================================================================
Tr2MaterialRes::Tr2MaterialRes( IRoot* lockobj ) :
	PARENTLOCK( m_meshes )
{
}

//================================================================
Tr2MaterialRes::~Tr2MaterialRes()
{
}
