#pragma once
#ifndef TRIOBSERVERLOCAL_H
#define TRIOBSERVERLOCAL_H

#include "include/ITriObserverLocal.h"
#include "Tr2DebugRenderer.h"


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
	void SetPosition( Vector3 pos );
	void SetFront( const Vector3& front );

	// debug
	void GetDebugOptions( Tr2DebugRendererOptions& options );
	void RenderDebugInfo( ITr2DebugRenderer2& renderer );

	std::string m_name;
private:
	Vector3 m_front;
	Vector3 m_position;
	IBluePlacementObserverPtr m_observer;
};

BLUE_DECLARE_VECTOR( TriObserverLocal );
TYPEDEF_BLUECLASS( TriObserverLocal );

//--- Utility functions that act on TriObserverLocal objects. ---//

// Send an audio event to an observer if it is not a nullptr and can be cast to ITr2AudEmitter.
void SendEventToAudEmitter( TriObserverLocal* observer, const std::wstring& audioEvent );

#endif