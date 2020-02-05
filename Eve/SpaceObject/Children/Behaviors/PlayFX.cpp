#include "StdAfx.h"
#include "PlayFX.h"
#include "include/TriMath.h"
#include "Eve/Renderable/Stretch/EveStretch.h"

PlayFX::PlayFX( IRoot* lockobj ) :
	PARENTLOCK( m_firingEffects ),
	m_count( 0 ),
	m_behaviorWeight( 20.f ),
	m_delay( 0.f ),
	m_minSec( 10 ),
	m_maxSec( 20 ),
	m_stop( false )
{
	m_firingEffect = nullptr;
}

PlayFX::~PlayFX()
{
}

int PlayFX::GetProcessPriority()
{
	return PROCESS_LATER;
}

std::string PlayFX::GetBehaviorName()
{
	return "PlayFX";
}

size_t PlayFX::GetScratchMemorySize() const
{
	return sizeof( PlayFXData );
}

void PlayFX::InitializeScratch( const DroneAgent& drone, void* scratchMemory )
{
	*static_cast<PlayFXData*>( scratchMemory ) = PlayFXData();
}

std::vector<Vector3> PlayFX::CalculateBehavior( std::vector<DroneAgent>& agents, void* scratchData, const float deltaTime,
	BehaviorGroup& group, EveChildBehaviorSystem& system, const std::vector<std::vector<DroneAgent*>>& dronesInSearchRadius )
{
	if( m_stop )
	{
		ClearEffects();
		m_behaviorWeight = 0;
		return m_todo;
	}

	if( m_behaviorWeight <= 0 )
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
	int i = 0;
	// This behavior will be activated when the drone has arrived near the damage locator
	for( auto agent = agents.begin(); agent != agents.end(); ++agent, ++i, ++data )
	{
		// Drone has arrived to target so play effect
		if( agent->playFX && !data->effectPlaying )
		{
			data->seconds = TriRandInt( m_minSec, m_maxSec );
			data->effectPlaying = true;

			m_firingEffects[i]->StartFiring( m_delay );
		}

		// Set the agent's position to world space because if the parent object had an offset the effect would also offset
		Matrix worldTransform = system.GetWorldTransform();
		Vector3 agentPositionWS = XMVector3TransformCoord( agent->position, worldTransform );

		// Without this drone will start shooting at the new target because of the cooldown of the effect
		if( data->oldTarget != Vector3( 0, 0, 0 ) )
		{
			m_firingEffects[i]->SetFiringTransform( agentPositionWS, data->oldTarget );
		}

		if( data->effectPlaying )
		{
			m_firingEffects[i]->SetFiringTransform( agentPositionWS, agent->target );

			Be::Time diff = BeOS->GetActualTime() - agent->fxStartTime;

			auto duration = data->seconds * 10000000;

			if( diff > duration )
			{
				m_firingEffects[i]->StopFiring();
				data->effectPlaying = agent->playFX = false;
				data->oldTarget = agent->target;
				agent->target = Vector3( 0, 0, 0 );
			}
		}
	}

	return m_todo;
}

void PlayFX::RenderDebugInfo( ITr2DebugRenderer2& renderer, std::vector<DroneAgent>& agents, Matrix& parentWorldLocation )
{
}

void PlayFX::Update( EveUpdateContext& updateContext, const TriFrustum & frustum, const Matrix & parentTransform )
{
	for( auto fx = m_firingEffects.begin(); fx != m_firingEffects.end(); ++fx )
	{
		( *fx )->Update( updateContext );
		( *fx )->UpdateVisibility( frustum, parentTransform );
	}
}

void PlayFX::GetRenderables( std::vector<ITr2Renderable*>& renderables )
{
	for( auto fx = m_firingEffects.begin(); fx != m_firingEffects.end(); ++fx )
	{
		( *fx )->GetRenderables( renderables );
	}
}

void PlayFX::ClearEffects()
{
	m_firingEffects.Clear();
	m_stop = false;
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
		size_t diff = agentSize - m_count;

		for( size_t i = 0; i < diff; ++i )
		{
			IEveFiringEffectElementPtr newFx;

			auto firingEffect = m_firingEffect;
	
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