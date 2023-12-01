////////////////////////////////////////////////////////////
//
//    Created:   January 2017
//    Copyright: CCP 2017
//
#pragma once

#include "Eve/SpaceObject/EveSpaceObject2.h"

BLUE_CLASS( EveUiObject ):
	public EveSpaceObject2
{
public:
	EXPOSE_TO_BLUE();

	EveUiObject( IRoot* lockobj = NULL );
	~EveUiObject();

	/////////////////////////////////////////////////////////////////////////////////////
	// EveSpaceObject2 override
	virtual void UpdateWorldTransform( Be::Time time );
	virtual void UpdateVisibility( const TriFrustum& frustum, const Matrix& parentTransform );

	// access
	std::string GetNameForPickingAreaID( uint32_t areaID ) const;
	void SetVisibilityForArea( const char* areaName, bool enable );

private:
	// scale with distance?
	bool m_usePerspectiveScale;
};

TYPEDEF_BLUECLASS( EveUiObject );

