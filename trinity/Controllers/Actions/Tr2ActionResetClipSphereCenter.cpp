// Copyright © 2023 CCP ehf.

#include "StdAfx.h"
#include "Tr2ActionResetClipSphereCenter.h"
#include "Controllers/Tr2Controller.h"


Tr2ActionResetClipSphereCenter::Tr2ActionResetClipSphereCenter( IRoot* lockobj ) :
	m_resetBehavior( OBJECT_CENTER ),
	m_locatorIndex( -1 ),
	m_locatorSetName( "" )
{
}

void Tr2ActionResetClipSphereCenter::ResetClipSphereToLocator( EveSpaceObject2* owner, BlueSharedString locatorSetName, int locatorIndex )
{
	auto locators = owner->GetLocatorsForSet( locatorSetName );
	if( locators && ( locatorIndex < 0 || locators->size() > size_t( locatorIndex ) ) )
	{
		if( locatorIndex < 0 )
		{
			locatorIndex = rand() % locators->size();
		}

		auto& locator = ( *locators )[locatorIndex];
		owner->ResetClipSphereCenterToPos( locator.position );
	}
}

void Tr2ActionResetClipSphereCenter::Start( ITr2ActionController& controller )
{
	if( EveSpaceObject2Ptr owner = BlueCastPtr( controller.GetOwner() ) )
	{
		switch( m_resetBehavior )
		{
		case LAST_DAMAGELOCATOR_HIT:
			ResetClipSphereToLocator( owner, BlueSharedString( "damage" ), owner->GetLastDamageLocatorHit() );
			break;
		case CUSTOM:
			ResetClipSphereToLocator( owner, m_locatorSetName, m_locatorIndex );
			break;
		case OBJECT_CENTER:
		default:
			owner->ResetClipSphereCenter();
			break;
		}
	}
}