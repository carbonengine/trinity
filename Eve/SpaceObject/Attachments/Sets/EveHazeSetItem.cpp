////////////////////////////////////////////////////////////
//
//    Created:   November 2017
//    Copyright: CCP 2017
//
#include "StdAfx.h"
#include "EveHazeSetItem.h"

// --------------------------------------------------------------------------------
// Description:
//   Initialize data members
// --------------------------------------------------------------------------------
EveHazeSetItem::EveHazeSetItem( IRoot* lockobj ) :
	m_position( 0.f, 0.f, 0.f ),
	m_scaling( 1.f, 1.f, 1.f ),
	m_rotation( 0.f, 0.f, 0.f, 1.f ),
	m_color( 1.f, 1.f, 1.f, 1.f ),
	m_hazeData( 4.0, 0.2, 2.0, 0.0 ),
	m_boneIndex( 0 )
{
}

// --------------------------------------------------------------------------------
// Description:
//   Cleanup
// --------------------------------------------------------------------------------
EveHazeSetItem::~EveHazeSetItem()
{
}

