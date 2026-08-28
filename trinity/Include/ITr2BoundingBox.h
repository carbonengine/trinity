// Copyright © 2011 CCP ehf.

#pragma once

#ifndef ITr2BoundingBox_h
#define ITr2BoundingBox_h

struct Obb;

BLUE_INTERFACE( ITr2BoundingBox ) :
	IRoot
{
	virtual bool GetWorldBoundingObb( Obb & obb ) const = 0;
	virtual bool IsBoundingBoxReady( void ) const = 0;
};

#endif