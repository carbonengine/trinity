#pragma once
#ifndef TRIOBSERVERLOCAL_H
#define TRIOBSERVERLOCAL_H

#include "trinity/include/ITriObserverLocal.h"


struct Vector3;
struct Matrix;

BLUE_CLASS( TriObserverLocal ) : public ITriObserverLocal
{
public:
	TriObserverLocal( IRoot* lockobj = NULL );
	virtual ~TriObserverLocal(); //Standalone object, do we need virtual destructor

	EXPOSE_TO_BLUE();

	void Update( const Matrix& worldTransform );

	// ITriObserverLocal
	IBluePlacementObserver* GetObserver();
	void SetObserver( IBluePlacementObserver* obs );

private:
	Vector3 m_front;
	Vector3 m_position;
	IBluePlacementObserverPtr m_observer;
};

BLUE_DECLARE_VECTOR( TriObserverLocal );
TYPEDEF_BLUECLASS( TriObserverLocal );

#endif