// Copyright © 2018 CCP ehf.

#include "StdAfx.h"
#include "Tr2ActionPlayMeshAnimation.h"
#include "Controllers/Tr2Controller.h"
#include "Tr2GrannyAnimation.h"


Tr2ActionPlayMeshAnimation::Tr2ActionPlayMeshAnimation( IRoot* lockobj ) :
	m_controller( nullptr ),
	m_destinationType( DestinationType::OWNER ),
	m_playAction( ENQUEUE_PLAY ),
	m_stopAction( ENQUEUE_STOP ),
	m_loops( -1 ),
	m_delay( 0 ),
	m_speed( 1 ),
	m_delayBinding( false )
{
}

void Tr2ActionPlayMeshAnimation::Link( ITr2ActionController& controller )
{
	m_controller = &controller;
	if( !HasDelayedBinding() )
	{
		LinkDestination( controller );
	}
}

void Tr2ActionPlayMeshAnimation::Unlink()
{
	m_controller = nullptr;
	m_destination.Unlink();
}

void Tr2ActionPlayMeshAnimation::Start( ITr2ActionController& controller )
{
	if( m_animation.empty() )
	{
		return;
	}
	if( HasDelayedBinding() )
	{
		LinkDestination( controller );
	}
	ITr2GrannyAnimationOwnerPtr owner = BlueCastPtr( m_destinationType == DestinationType::OWNER ? controller.GetOwner() : m_destination.GetBoundObject() );
	if( !owner )
	{
		return;
	}
	auto ac = owner->GetAnimationController();
	if( !ac )
	{
		return;
	}
	if( !m_mask.empty() )
	{
		ac->AddAnimationLayerWithTrackMask( m_mask.c_str(), m_mask.c_str() );
	}
	ac->PlayLayerAnimationByName( m_mask.empty() ? nullptr : m_mask.c_str(), m_animation.c_str(), m_playAction == PLAY, std::max( m_loops, 0 ), m_delay, m_speed, false );
}

void Tr2ActionPlayMeshAnimation::Stop( ITr2ActionController& controller )
{
	if( m_animation.empty() )
	{
		return;
	}
	ITr2GrannyAnimationOwnerPtr owner = BlueCastPtr( m_destinationType == DestinationType::OWNER ? controller.GetOwner() : m_destination.GetBoundObject() );
	if( !owner )
	{
		return;
	}
	auto ac = owner->GetAnimationController();
	if( !ac )
	{
		return;
	}
	auto layer = ac->GetAnimationLayer( m_mask.empty() ? nullptr : m_mask.c_str() );
	if( !layer )
	{
		return;
	}
	switch( m_stopAction )
	{
	case STOP:
		layer->ClearAnimations();
		break;
	case ENQUEUE_STOP:
		layer->EndAnimation();
		break;
	default:
		break;
	}
}

bool Tr2ActionPlayMeshAnimation::OnModified( Be::Var* value )
{
	if( !m_controller )
	{
		return true;
	}
	if( IsMatch( value, m_destinationType ) || IsMatch( value, m_destination.m_path ) || IsMatch( value, m_destination.m_attribute ) || IsMatch( value, m_destination.m_object ) || IsMatch( value, m_delayBinding ) )
	{
		if( !HasDelayedBinding() )
		{
			LinkDestination( *m_controller );
		}
	}
	return true;
}

void Tr2ActionPlayMeshAnimation::LinkDestination( const ITr2ActionController& controller )
{
	if( m_destinationType == DestinationType::OWNER )
	{
		m_destination.Unlink();
		return;
	}
	m_destination.Link( controller.GetBindingPathRoots() );
}

bool Tr2ActionPlayMeshAnimation::HasDelayedBinding() const
{
	return m_delayBinding && !m_destination.m_path.empty();
}

bool Tr2ActionPlayMeshAnimation::IsBindingValid() const
{
	if( m_destinationType == DestinationType::OWNER )
	{
		return true;
	}
	return m_destination.GetBoundObject() != nullptr;
}

IRootPtr Tr2ActionPlayMeshAnimation::GetDestination() const
{
	if( m_destinationType == DestinationType::OWNER )
	{
		return m_controller ? m_controller->GetOwner() : nullptr;
	}
	else
	{
		return m_destination.GetBoundObject();
	}
}
