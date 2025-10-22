#include "StdAfx.h"
#include "EveStretch.h"
#include "TriFloat.h"
#include "Eve/EveUpdateContext.h"
#include "Include/TriMath.h"
#include "Utilities/BoundingSphere.h"
#include "Eve/Turret/EveTurretSet.h"
#include "Curves/Tr2CurveScalar.h"
#include "Lights/Tr2PointLight.h"
#include "Audio/Tr2AudioStretchAuto.h"
#include "Audio/Tr2AudioStretchBase.h"

static const Vector3 Y_AXIS(0.0f, 1.0f, 0.0f);

EveStretch::EveStretch( IRoot* lockobj ) :
	PARENTLOCK( m_curveSets ),
	PARENTLOCK( m_sourceLights ),
	PARENTLOCK( m_destLights ),
	m_display( true ),
	m_update( true ),
	m_displaySourceObject( true ),
	m_displayDestObject( true ),
	m_useTransformsForStretch( false ),
	m_isNegZForward( false ),
	m_sourcePosition( 0.0f, 0.0f, 0.0f ),
	m_destinationPosition( 0.0f, 0.0f, 0.0f ),
	m_lastCurveUpdateTime( 0 ),
	m_lodLevel( TR2_LOD_LOW ),
	m_useCurveLod( true ),
	m_startTime( -1 ),
	m_moveCompleted( false ),
	m_moving( false ),
	m_destObjectScale( 1.0f ),
	m_sourceObjectScale( 1.0f ),
	m_sourceTransform( IdentityMatrix() ),
	m_destinationTransform( IdentityMatrix() )
{
	m_length.CreateInstance();
}

bool EveStretch::OnModified( Be::Var* value )
{
	if( IsMatch( value, m_display ) )
	{
		ReRegister();
	}
	return true;
}

// start the update mess!
void EveStretch::UpdateSyncronous( const EveUpdateContext& updateContext )
{
	CCP_STATS_ZONE( __FUNCTION__ );

	Be::Time time = updateContext.GetTime();
	if( !m_update )
	{
		return;
	}

	if( m_source )
	{
		m_source->Update( &m_sourcePosition, time );
	}
	else if( m_useTransformsForStretch )
	{
		m_sourcePosition = m_sourceTransform.GetTranslation();
	}

	if( m_dest )
	{
		m_dest->Update( &m_destinationPosition, time );
	}
}

void EveStretch::UpdateAsyncronous( const EveUpdateContext& updateContext )
{
	if( !m_update )
	{
		return;
	}

	UpdateCurves( updateContext );

	Vector3 directionVec( m_sourcePosition - m_destinationPosition );
	m_length->m_value = Length( directionVec );


	if( m_sourceObject && m_displaySourceObject )
	{
		m_sourceObject->Update( updateContext );
	}

	if( m_stretchObject )
	{
		m_stretchObject->Update( updateContext );
	}

	if( m_moveObject )
	{
		m_moveObject->Update( updateContext );
	}

	if( m_destObject && m_displayDestObject )
	{
		m_destObject->Update( updateContext );
	}

	if (auto tmp = dynamic_cast< Tr2AudioStretchBase* > ( m_audio.p ))
	{
		tmp->Update( m_sourcePosition, m_destinationPosition );
	}
}

void EveStretch::Update( const EveUpdateContext& updateContext )
{
	UpdateSyncronous( updateContext );
	UpdateAsyncronous( updateContext );
}

void EveStretch::UpdateEffectAsync( const EveUpdateContext& updateContext )
{
	Update( updateContext );
}

void EveStretch::UpdateEffectSync( const EveUpdateContext& updateContext )
{
	// do nothing here
}

void EveStretch::UpdateCurves( const EveUpdateContext& updateContext )
{
	Be::Time time = updateContext.GetTime();
	if( !m_update )
	{
		return;
	}

	if( m_moving && m_startTime == -1 )
	{
		m_startTime = time;
	}

	float delta = (float)TimeAsDouble( time - m_lastCurveUpdateTime );

	if( !m_useCurveLod || EveLODHelper::ShouldUpdate( m_lodLevel, delta ) )
	{
		m_lastCurveUpdateTime = time;
		if( m_moving && m_progressCurve )
		{
			Be::Time elapsedTime = time - m_startTime;
			m_progressCurve->UpdateValue( TimeAsDouble( elapsedTime ) );
		}

		if( m_moveCompletion )
		{
			m_moveCompletion->Update( TimeAsDouble( time ) );
		}

		for( TriCurveSetVector::const_iterator it = m_curveSets.begin(); it != m_curveSets.end(); ++it )
		{
			(*it)->Update( TimeAsDouble( time ) );
		}
	}
}


void EveStretch::UpdateVisibility( const EveUpdateContext& updateContext, const Matrix& parentTransform )
{
	m_lodLevel = TR2_LOD_LOW;

	if( !m_display )
	{
		return;
	}

	Vector3 directionVec = Normalize( m_sourcePosition - m_destinationPosition );
	float scalingLength = m_length->m_value;

	Matrix m;
	if( m_sourceObject && m_displaySourceObject )
	{
		if( m_useTransformsForStretch )
		{
			// Artwork is authored aligned to the y-axis rather than z
			// so we add a rotation here.
			m = RotationXMatrix( -XM_PI / 2.0f );
			m = m * m_sourceTransform;
		}
		else
		{
			Quaternion rotation( 0.0f, 0.0f, 0.0f, 1.0f );
			Quaternion tmpResult;
			rotation = *TriQuaternionRotationArc( &tmpResult, &Y_AXIS, &directionVec ) * rotation;

			m = TransformationMatrix( Vector3( 1, 1, 1 ), rotation, m_sourcePosition );
		}
		m_sourceObject->UpdateVisibility( updateContext, m );
		// The object's LOD is the highest of it's move, stretch, dest and source object's LODs
		m_lodLevel = EveLODHelper::MergeLOD( m_lodLevel, m_sourceObject->GetLODLevel() );
	}

	if( m_destObject && m_displayDestObject )
	{
		Quaternion rotation( 0.0f, 0.0f, 0.0f, 1.0f );
		Quaternion tmpResult;
		rotation = *TriQuaternionRotationArc( &tmpResult, &Y_AXIS, &directionVec ) * rotation;

		Vector3 scaling = Vector3( m_destObjectScale, m_destObjectScale, m_destObjectScale );
		Matrix m = TransformationMatrix( scaling, rotation, m_destinationPosition );

		m_destObject->UpdateVisibility( updateContext, m );
		// The object's LOD is a combination of it's move, stretch, dest and source object's LODs
		m_lodLevel = EveLODHelper::MergeLOD( m_lodLevel, m_destObject->GetLODLevel() );
	}

	if( m_stretchObject )
	{
		if( m_useTransformsForStretch )
		{
			Matrix scaling = ScalingMatrix( 1.0f, 1.0f, scalingLength );
			m = scaling * m_sourceTransform;
		}
		else
		{
			// support pointing in -z!
			if( !m_isNegZForward )
			{
				directionVec *= -1.f;
			}

			Quaternion rotation( 0.0f, 0.0f, 0.0f, 1.0f );
			TriQuaternionArcFromForward( &rotation, &directionVec );

			Vector3 scaling( 1.0f, 1.0f, scalingLength );

			m = TransformationMatrix( scaling, rotation, m_sourcePosition );
		}
		m_stretchObject->UpdateVisibility( updateContext, m );
		// The object's LOD is a combination of it's move, stretch, dest and source object's LODs
		m_lodLevel = EveLODHelper::MergeLOD( m_lodLevel, m_stretchObject->GetLODLevel() );

		// also combine in a LOD based on the sphere around dest and source points. This way we avoid getting
		// too low results when the stretcher is too small (animation!)
		Vector4 sourceDestSphere;
		BoundingSphereFromPoints( sourceDestSphere, m_sourcePosition, m_destinationPosition );
		m_lodLevel = EveLODHelper::MergeLOD( m_lodLevel, sourceDestSphere, updateContext );
	}

	if( m_moveObject )
	{
		// support pointing in -z!
		if( !m_isNegZForward )
		{
			directionVec *= -1.f;
		}

		Quaternion rotation( 0.0f, 0.0f, 0.0f, 1.0f );
		TriQuaternionArcFromForward( &rotation, &directionVec );

		Vector3 movedPostition = m_sourcePosition;
		// Calculate the current position of the move object
		if( m_progressCurve && m_moveObject )
		{
			float progress = 0;
			if( auto curve = dynamic_cast<Tr2CurveScalar*>( m_progressCurve.p ) )
			{
				progress = curve->GetCurrentValue();
			}
			if( progress >= 1.0 && !m_moveCompleted )
			{
				if( m_moveCompletion )
				{
					m_moveCompletion->Play();
				}
				m_moveObject->SetDisplay( false );
				m_moveCompleted = true;
			}
			movedPostition = Lerp( m_sourcePosition, m_destinationPosition, progress );
		}
		m = TransformationMatrix( Vector3( 1, 1, 1 ), rotation, movedPostition );
		m_moveObject->UpdateVisibility( updateContext, m );

		// The object's LOD is a combination of it's move, stretch, dest and source object's LODs
		m_lodLevel = EveLODHelper::MergeLOD( m_lodLevel, m_moveObject->GetLODLevel() );
	}
}

void EveStretch::GetRenderables( std::vector<ITr2Renderable*>& renderables)
{
	GetRenderables( renderables, nullptr );
}

void EveStretch::GetRenderables( std::vector<ITr2Renderable*>& renderables, Tr2ImpostorManager* impostors )
{
	if( !m_display )
	{
		return;
	}

	if( m_sourceObject && m_displaySourceObject )
	{
		m_sourceObject->GetRenderables( renderables, impostors );
	}

	if( m_destObject && m_displayDestObject )
	{
		m_destObject->GetRenderables( renderables, impostors );
	}

	if( m_stretchObject )
	{
		m_stretchObject->GetRenderables( renderables, impostors );
	}

	if( m_moveObject )
	{
		m_moveObject->GetRenderables( renderables, impostors );
	}
}

void EveStretch::StartMoving()
{
	m_startTime = -1;
	m_moving = true;
	m_moveCompleted = false;

	if( m_moveObject )
	{
		m_moveObject->SetDisplay( true );
	}

	if ( auto tmp = dynamic_cast< Tr2AudioStretchAuto* > ( m_audio.p ) )
	{
		tmp->TriggerStretchEvent();
	}
}

// For future developers, this is the method that is used in client, not StartFiring
void EveStretch::Start()
{
	StartMoving();

	if ( !m_curveSets.empty() )
	{
		m_curveSets.front()->Play();

		if ( auto tmp = dynamic_cast< Tr2AudioStretchAuto* > ( m_audio.p ) )
		{
			tmp->TriggerOutburstEvent();
			tmp->TriggerImpactEvent();
		}
	}
}

void EveStretch::SetDisplay( bool display )
{
	m_display = display;
	ReRegister();
}

bool EveStretch::GetBoundingSphere( Vector4& sphere, BoundingSphereQuery query ) const
{
	bool valid = false;

	Vector4 v;
	if( m_destObject )
	{
		if( m_destObject->GetBoundingSphere( v, query ) )
		{
			sphere = v;
			valid = true;
		}
	}
	if( m_sourceObject )
	{
		if( m_sourceObject->GetBoundingSphere( v, query ) )
		{
			BoundingSphereSetOrUpdate( v, sphere, valid );
			valid = true;
		}
	}
	if( m_stretchObject )
	{
		if( m_stretchObject->GetBoundingSphere( v, query ) )
		{
			BoundingSphereSetOrUpdate( v, sphere, valid );
			valid = true;
		}
	}

	return valid;
}

void EveStretch::SetSourcePosition( Vector3 val )
{
	m_useTransformsForStretch = false;
	m_sourcePosition = val;
}

void EveStretch::SetDestinationPosition( Vector3 val )
{
	m_destinationPosition = val;
}

void EveStretch::SetSourceTransform( const Matrix& val )
{
	m_useTransformsForStretch = true;
	m_sourceTransform = val;
}

void EveStretch::SetDestinationTransform( const Matrix& val )
{
	m_destinationTransform = val;
}

// --------------------------------------------------------------------------------
// Description:
//   The stretch part of an EveStretch effect goes from source to dest and is
//   authored to face either -z or +z direction. Default is +z, but with this
//   function we support -z as well
// SeeAlso:
//   EveTurretSet
// --------------------------------------------------------------------------------
void EveStretch::SetIsNegZForward( bool val )
{
	m_isNegZForward = val;
}

float EveStretch::GetCurveDuration()
{
	float maxDuration = 0;
	for( auto it = m_curveSets.begin(); it != m_curveSets.end(); ++it )
	{
		maxDuration = std::max( maxDuration, ( *it )->GetMaxCurveDuration() / ( *it )->GetTimeScale() );
	}
	return maxDuration;
}

void EveStretch::StartFiring( float delay )
{
	for( auto it = m_curveSets.begin(); it != m_curveSets.end(); ++it )
	{
		auto curveSet = *it;
		if( curveSet )
		{
			if( curveSet->GetName() == "play_start" )
			{
				curveSet->PlayFrom( -delay );
				StartMoving();

			}
			else if( curveSet->GetName() == "play_loop" )
			{
				curveSet->PlayFrom( -delay );
			}
			else if( curveSet->GetName() == "play_end" )
			{
				curveSet->Stop();
			}
		}
	}
}

void EveStretch::StopFiring()
{
	for( auto it = m_curveSets.begin(); it != m_curveSets.end(); ++it )
	{
		auto curveSet = *it;
		if( curveSet )
		{
			if( curveSet->GetName() == "play_start" )
			{
				curveSet->Stop();
				StartMoving();
			}
			else if( curveSet->GetName() == "play_loop" )
			{
				curveSet->Stop();
			}
			else if( curveSet->GetName() == "play_end" )
			{
				curveSet->Play();
			}
		}
	}
}

void EveStretch::SetFiringTransform( const Matrix& source, const Vector3& dest )
{
	SetSourceTransform( source );
	SetDestinationPosition( dest );
	SetIsNegZForward( true );
}

void EveStretch::SetFiringTransform( const Vector3& source, const Vector3& dest )
{
	SetSourcePosition( source );
	SetDestinationPosition( dest );
	SetIsNegZForward( true );
}

void EveStretch::DisplayEndPoints( bool displaySource, bool displayDest )
{
	SetDisplaySourceObject( displaySource );
	SetDisplayDestObject( displayDest );
}

void EveStretch::GetLights( Tr2LightManager& lightManager ) const
{
	if( !m_display )
	{
		return;
	}

	Vector3 directionVec = Normalize( m_sourcePosition - m_destinationPosition );

	if( !m_sourceLights.empty() && m_displaySourceObject )
	{
		Matrix m;

		float scaling = 1;

		if( m_useTransformsForStretch )
		{
			// Artwork is authored aligned to the y-axis rather than z
			// so we add a rotation here.
			m = RotationXMatrix( -XM_PI / 2.0f );
			m = m * m_sourceTransform;
			scaling = XMVectorGetX( XMVectorAdd( XMVector3LengthEst( m.GetX() ),
				XMVectorAdd( XMVector3LengthEst( m.GetY() ), XMVector3LengthEst( m.GetZ() ) ) ) ) / 3.f;
		}
		else
		{
			Quaternion rotation( 0.0f, 0.0f, 0.0f, 1.0f );
			Quaternion tmpResult;
			rotation = *TriQuaternionRotationArc( &tmpResult, &Y_AXIS, &directionVec ) * rotation;
			m = TransformationMatrix( Vector3( 1, 1, 1 ), rotation, m_sourcePosition );
		}

		for( auto it = std::begin( m_sourceLights ); it != std::end( m_sourceLights ); ++it )
		{
			( *it )->AddLight( lightManager, m, scaling );
		}
	}
	if( !m_destLights.empty() && m_displayDestObject )
	{
		Quaternion rotation( 0.0f, 0.0f, 0.0f, 1.0f );
		Quaternion tmpResult;
		rotation = *TriQuaternionRotationArc( &tmpResult, &Y_AXIS, &directionVec ) * rotation;

		Vector3 scaling = Vector3( m_destObjectScale, m_destObjectScale, m_destObjectScale );
		Matrix m = TransformationMatrix( scaling, rotation, m_destinationPosition );

		for( auto it = std::begin( m_destLights ); it != std::end( m_destLights ); ++it )
		{
			( *it )->AddLight( lightManager, m, m_destObjectScale );
		}
	}
}

void EveStretch::GetDebugOptions( Tr2DebugRendererOptions& options )
{
	if (auto tmp = dynamic_cast< Tr2AudioStretchBase* > ( m_audio.p ))
	{
		tmp->GetDebugOptions( options );
	}
}

void EveStretch::RenderDebugInfo( ITr2DebugRenderer2& renderer )
{
	if (auto tmp = dynamic_cast< Tr2AudioStretchBase* > ( m_audio.p ))
	{
		tmp->RenderDebugInfo( renderer );
	}
}

void EveStretch::RegisterComponents()
{
	auto registry = this->GetComponentRegistry();
	if( registry && m_display )
	{
		registry->RegisterComponent<ITr2LightOwner>( this );
	}
}