// Copyright © 2011 CCP ehf.

#pragma once

#ifndef ITr2BoundingBox_h
#define ITr2BoundingBox_h

struct Vector3;

BLUE_INTERFACE( ITr2BoundingBox ) :
	IRoot
{
	virtual bool GetWorldBoundingBox( Vector3 & min, Vector3 & max ) const = 0;
	virtual bool IsBoundingBoxReady( void ) const = 0;
};

#endif