// Copyright © 2025 CCP ehf.

#ifndef ITr2MeshMorph_h
#define ITr2MeshMorph_h

#include "Eve/EveComponentRegistry.h"

BLUE_INTERFACE( ITr2MeshMorph ) :
	public IRoot
{
	virtual bool UpdateMeshMorphs( Tr2RenderContext & renderContext ) = 0;
};

REGISTER_COMPONENT_TYPE( "MeshMorph", ITr2MeshMorph );

#endif
