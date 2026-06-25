// Copyright © 2023 CCP ehf.

#pragma once
#ifndef Tr2BoundingLineSet_h
#define Tr2BoundingLineSet_h

#include "Tr2LineSet.h"

// A primitive specially made for bounding boxes
BLUE_CLASS( Tr2BoundingLineSet ) :
	public Tr2LineSet
{
public:
	EXPOSE_TO_BLUE();
	Tr2BoundingLineSet( IRoot* lockobj = NULL );

	//////////////////////////////////////////////////////////////////////////
	// INotify
	virtual bool OnModified( Be::Var * value );

private:
	void AddBox( const Vector3& min, const Vector3& max, const Vector4& color );
	void AddPickingBox( const Vector3& min, const Vector3& max );
	void UpdateBounds( const Vector3& min, const Vector3& max );
	Vector3 m_minBounds;
	Vector3 m_maxBounds;
};

TYPEDEF_BLUECLASS( Tr2BoundingLineSet );
#endif //Tr2BoundingLineSet_h
