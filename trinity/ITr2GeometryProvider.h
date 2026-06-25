// Copyright © 2023 CCP ehf.

#pragma once
#ifndef ITr2GeometryProvider_H
#define ITr2GeometryProvider_H

class Tr2RenderContext;

BLUE_INTERFACE( ITr2GeometryProvider ) :
	public IRoot
{
	virtual void SubmitGeometry( Tr2RenderContext & renderContext ) = 0;
};


#endif // ITr2GeometryProvider_H