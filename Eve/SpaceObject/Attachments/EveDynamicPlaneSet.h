////////////////////////////////////////////////////////////
//
//    Created:   February 2014
//    Copyright: CCP 2014
//
#pragma once
#ifndef EveDynamicPlaneSet_H
#define EveDynamicPlaneSet_H

#include "ITr2GeometryProvider.h"

#include "EvePlaneSet.h"

// forwards
BLUE_DECLARE( EveDynamicPlaneSet );

// --------------------------------------------------------------------------------
// Description:
//   This class extends the static planesets into planesets with dynamic content.
//   Like for holograms, billboards, etc.
// SeeAlso:
//   EvePlaneSet
// --------------------------------------------------------------------------------
BLUE_CLASS( EveDynamicPlaneSet ):
	public EvePlaneSet
{
public:
	EXPOSE_TO_BLUE();

	EveDynamicPlaneSet( IRoot* lockobj = NULL );
	~EveDynamicPlaneSet();

private:
	// reference to the dynamic content creator
	std::string m_contentID;
};

TYPEDEF_BLUECLASS( EveDynamicPlaneSet );

#endif // EveDynamicPlaneSet_H