#include "StdAfx.h"
#include "Tr2SyncToAnimation.h"
#include "Controllers/Tr2Controller.h"
#include "Eve/SpaceObject/EveSpaceObject2.h"
#include "Tr2GrannyAnimation.h"


bool Tr2SyncToAnimation::CanTransition( Tr2Controller& controller ) const
{
	EveSpaceObject2Ptr spaceObject = BlueCastPtr( controller.GetOwner() );
	if( !spaceObject )
	{
		return true;
	}
	auto ac = spaceObject->GetAnimationController();
	if( !ac )
	{
		return true;
	}
	auto layer = ac->GetAnimationLayer( m_mask.empty()  ? nullptr : m_mask.c_str() );
	if( !layer )
	{
		return true;
	}
	auto remaining = layer->GetAnimationRemainingTime();
	return remaining <= 0;
}