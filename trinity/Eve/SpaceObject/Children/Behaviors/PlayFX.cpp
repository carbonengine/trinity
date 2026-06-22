// Copyright © 2023 CCP ehf.

#include "StdAfx.h"
#include "PlayFX.h"
#include "include/TriMath.h"
#include "Eve/Renderable/Stretch/EveStretch.h"

PlayFX::PlayFX( IRoot* lockobj ) :
	PARENTLOCK( m_firingEffects ),
	m_enabled( true ),
	m_count( 0 ),
	m_behaviorWeight( 20.f ),
	m_distanceFromCenter( 5.f ),
	m_sec( 1 ),
	m_stop( false ),
	m_priority( LEAST_PRIORITY )
{
	m_firingEffects.SetNotify( this );
}

PlayFX::~PlayFX()
{
}

int PlayFX::GetProcessPriority()
{
	return m_priority;
}

std::string PlayFX::GetBehaviorName()
{
	return "PlayFX";
}

size_t PlayFX::GetScratchMemorySize() const
{
	return sizeof( PlayFXData );
}

void PlayFX::InitializeScratch( void* scratchMemory )
{
	*static_cast<PlayFXData*>( scratchMemory ) = PlayFXData();
}

std::vector<Vector3> PlayFX::CalculateBehavior( std::vector<DroneAgent>& agents, void* scratchData, const float deltaTime, BehaviorGroup& group, EveChildBehaviorSystem& system, const std::vector<std::vector<DroneAgent*>>& dronesInSearchRadius )
{
	CCP_STATS_ZONE( __FUNCTION__ );


	if( m_behaviorWeight <= 0 || !m_enabled )
	{
		for( auto fx = m_firingEffects.begin(); fx != m_firingEffects.end(); ++fx )
		{
			( *fx )->StopFiring();
		}
		return m_todo;
	}

	if( m_firingEffect == nullptr )
	{
		return m_todo;
	}

	// If the drone count is 0 the m_count is not updated so this is needed
	if( m_firingEffects.size() == 0 )
	{
		m_count = 0;
	}

	size_t diff = m_count > agents.size() ? m_count - agents.size() : agents.size() - m_count;

	if( diff > 0 )
	{
		CheckCount( agents.size() );
	}

	auto data = static_cast<PlayFXData*>( scratchData );
	auto agent = agents.begin();
	auto firingEffect = m_firingEffects.begin();

	// This behavior will be activated when the drone has arrived near the damage locator
	for( ; agent != agents.end() && firingEffect != m_firingEffects.end(); ++agent, ++firingEffect, ++data )
	{
		if( m_stop )
		{
			data->droneArrived = false;
		}

		// Make sure the effect isn't showing when loading everything up
		if( data->droneArrived == false )
		{
			( *firingEffect )->SetDisplay( false );
		}

		// Drone has arrived to target so play effect
		if( agent->playFX && !data->effectPlaying )
		{
			if( data->droneArrived == false )
			{
				data->droneArrived = true;
				( *firingEffect )->SetDisplay( true );
			}

			( *firingEffect )->StartFiring( 0 );
			data->effectPlaying = true;
		}

		Matrix worldTransform = system.GetWorldTransform();
		Vector3 offsetEffect = agent->position + Normalize( agent->targetDirection ) * group.GetBoundingSphereRadius();

		// Set the effect]s pos to world space
		Vector3 offsetEffectWS = TransformCoord( offsetEffect, worldTransform );

		// Without this the drone will start shooting at the new target because of the cooldown of the effect
		if( data->oldTarget != Vector3( 0, 0, 0 ) )
		{
			( *firingEffect )->SetFiringTransform( offsetEffectWS, data->oldTarget );
		}

		if( data->effectPlaying )
		{
			Vector3 agentTargetWS = TransformCoord( agent->target, worldTransform );
			( *firingEffect )->SetFiringTransform( offsetEffectWS, agentTargetWS );

			Be::Time diff = BeOS->GetActualTime() - agent->fxStartTime;
			auto duration = m_sec * 10000000;

			if( diff > duration )
			{
				( *firingEffect )->StopFiring();
				data->effectPlaying = agent->playFX = false;
				data->oldTarget = agentTargetWS;
				agent->target = Vector3( 0, 0, 0 );
			}
		}
	}

	return m_todo;
}

void PlayFX::UpdateAsyncronous( const EveUpdateContext& updateContext, const Matrix& parentTransform )
{
	for( auto fx = m_firingEffects.begin(); fx != m_firingEffects.end(); ++fx )
	{
		( *fx )->UpdateEffectAsync( updateContext );
		( *fx )->UpdateVisibility( updateContext, parentTransform );
	}
}

void PlayFX::UpdateSyncronous( const EveUpdateContext& updateContext )
{
	for( auto fx = m_firingEffects.begin(); fx != m_firingEffects.end(); ++fx )
	{
		( *fx )->UpdateEffectSync( updateContext );
	}
}

void PlayFX::GetRenderables( std::vector<ITr2Renderable*>& renderables )
{
	for( auto fx = m_firingEffects.begin(); fx != m_firingEffects.end(); ++fx )
	{
		( *fx )->GetRenderables( renderables );
	}
}

void PlayFX::OnListModified( long event, ssize_t key, ssize_t key2, IRoot* value, const IList* list )
{
	if( list == &m_firingEffects && ( event & BELIST_LOADING ) == 0 )
	{
		if( IsInRegistry() )
		{
			switch( event & BELIST_EVENTMASK )
			{
			case BELIST_INSERTED:
				if( EveEntityPtr entity = BlueCastPtr( value ) )
				{
					entity->Register( GetComponentRegistry() );
				}
				break;
			case BELIST_REMOVED:
				if( EveEntityPtr entity = BlueCastPtr( value ) )
				{
					entity->UnRegister( GetComponentRegistry() );
				}
				break;
			case BELIST_UNLOADSTART:
				for( ssize_t i = 0; i < list->GetSize(); ++i )
				{
					if( EveEntityPtr entity = BlueCastPtr( list->GetAt( i ) ) )
					{
						entity->UnRegister( GetComponentRegistry() );
					}
				}
				break;
			default:
				break;
			}
		}
	}
}

void PlayFX::RegisterComponents()
{
	auto registry = this->GetComponentRegistry();
	if( registry )
	{
		for( auto fx = m_firingEffects.begin(); fx != m_firingEffects.end(); ++fx )
		{
			if( EveEntityPtr entity = BlueCastPtr( *fx ) )
			{
				entity->Register( GetComponentRegistry() );
			}
		}
	}
}

void PlayFX::UnRegisterComponents()
{
	auto registry = this->GetComponentRegistry();
	if( registry )
	{
		for( auto fx = m_firingEffects.begin(); fx != m_firingEffects.end(); ++fx )
		{
			if( EveEntityPtr entity = BlueCastPtr( *fx ) )
			{
				entity->UnRegister( GetComponentRegistry() );
			}
		}
	}
}

// --------------------------------------------------------------------------------
// Description:
//   Registers space object attachments (sprite and spotlight sets) with quad
//   renderer.
// Arguments:
//   quadRenderer - quad renderer
// --------------------------------------------------------------------------------
void PlayFX::RegisterWithQuadRenderer( Tr2QuadRenderer& quadRenderer )
{
	for( auto fx = m_firingEffects.begin(); fx != m_firingEffects.end(); ++fx )
	{
		( *fx )->RegisterWithQuadRenderer( quadRenderer );
	}
}

// --------------------------------------------------------------------------------
// Description:
//   Adds sprites from sprite sets and spotlight sets to quad renderer.
// Arguments:
//   quadRenderer - quad renderer
// --------------------------------------------------------------------------------
void PlayFX::AddQuadsToQuadRenderer( const TriFrustum& frustum, Tr2QuadRenderer& quadRenderer ) const
{
	for( auto fx = m_firingEffects.begin(); fx != m_firingEffects.end(); ++fx )
	{
		( *fx )->AddQuadsToQuadRenderer( frustum, quadRenderer );
	}
}


void PlayFX::CheckCount( size_t agentSize )
{
	if( m_count > agentSize )
	{
		size_t diff = m_count - agentSize;

		for( size_t i = 0; i < diff; ++i )
		{
			m_firingEffects.Remove( m_firingEffects.size() - 1 );
		}
		m_count = agentSize;
	}
	else if( agentSize > m_count )
	{
		if( m_firingEffect == nullptr )
		{
			return;
		}

		auto firingEffect = m_firingEffect;

		size_t diff = agentSize - m_count;

		for( size_t i = 0; i < diff; ++i )
		{
			IEveFiringEffectElementPtr newFx;

			// Copies data from 'source' into '*dest'
			if( !BeClasses->CloneTo( firingEffect, (IRoot**)&newFx.p ) )
			{
				return;
			}

			m_firingEffects.Append( newFx );
		}
		m_count = agentSize;
	}
}