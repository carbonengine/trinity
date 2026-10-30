// Copyright © 2011 CCP ehf.

#pragma once

#ifndef ITr2BoundingBox_h
#define ITr2BoundingBox_h

struct Vector3;
struct Matrix;

BLUE_INTERFACE( ITr2BoundingBox ) :
	IRoot
{
	virtual bool GetOrientedBoundingBox( Vector3 & localMin, Vector3 & localMax, Matrix & localToWorld ) const = 0;
	virtual bool IsBoundingBoxReady( void ) const = 0;
};

#endif