// Copyright © 2023 CCP ehf.

#pragma once
#ifndef ITr2Scene_H
#define ITr2Scene_H

#include "ITr2Updateable.h"

class Tr2RenderContext;

BLUE_INTERFACE( ITr2Scene ) :
	public ITr2Updateable
{
	virtual void Render( Tr2RenderContext & renderContext ) = 0;
	virtual void RenderDebugInfo( Tr2RenderContext & renderContext ) = 0;
};

#endif // ITr2Scene_H