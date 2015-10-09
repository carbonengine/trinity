#include "StdAfx.h"
#include "EveStretch.h"
#include "TriFloat.h"
#include "EveUpdateContext.h"
#include "Include/TriMath.h"
#include "Utilities/BoundingSphere.h"

static const Vector3 Y_AXIS(0.0f, 1.0f, 0.0f);

EveStretch::EveStretch( IRoot* lockobj ) :
	PARENTLOCK( m_curveSets ),
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
	m_startTime( -1 ),
	m_moveCompleted( false ),
	m_moving( false ),
	m_destObjectScale( 1.0f ),
	m_sourceObjectScale( 1.0f )
{
	m_length.CreateInstance();

	m_sourceTransform = Tr2Renderer::GetIdentityTransform();
	m_destinationTransform = Tr2Renderer::GetIdentityTransform();
}

void EveStretch::UpdateSyncronous( EveUpdateContext& updateContext )
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

void EveStretch::UpdateAsyncronous( EveUpdateContext& updateContext )
{
	Be::Time time = updateContext.GetTime();
	if( !m_update )
	{
		return;
	}

	UpdateCurves( updateContext );

	Vector3 directionVec( m_destinationPosition - m_sourcePosition);
	float scalingLength = D3DXVec3Length( &directionVec );
	m_length->m_value = scalingLength;

	D3DXVec3Normalize( &directionVec, &directionVec );

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
}

void EveStretch::Update( EveUpdateContext& updateContext )
{
	UpdateSyncronous( updateContext );
	UpdateAsyncronous( updateContext );
}

void EveStretch::UpdateCurves( EveUpdateContext& updateContext )
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
	m_lastCurveUpdateTime = time;

	if( EveLODHelper::ShouldUpdate( m_lodLevel, delta ) )
	{
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

void EveStretch::RenderDebugInfo( Tr2RenderContext& renderContext )
{

}

void EveStretch::GetRenderables( const TriFrustum& frustum, std::vector<ITr2Renderable*>& renderables, const Matrix& parentTransform )
{
	m_lodLevel = TR2_LOD_LOW;

	if( !m_display )
	{
		return;
	}

	Vector3 directionVec( m_sourcePosition - m_destinationPosition );
	float scalingLength = D3DXVec3Length( &directionVec );
	
	if( m_sourceObject && m_displaySourceObject )
	{
		if( m_useTransformsForStretch )
		{
			// Artwork is authored aligned to the y-axis rather than z
			// so we add a rotation here.
			Matrix m;
			D3DXMatrixRotationX( &m, -XM_PI / 2.0f );
			D3DXMatrixMultiply( &m, &m, &m_sourceTransform );
			m_sourceObject->GetRenderables( frustum, renderables, m );
		}
		else
		{
			Quaternion rotation( 0.0f, 0.0f, 0.0f, 1.0f );
			Quaternion tmpResult;
			D3DXQuaternionMultiply( &rotation,
				TriQuaternionRotationArc( &tmpResult, &Y_AXIS, &directionVec ), &rotation );

			Matrix m;
			D3DXMatrixTransformation( &m, NULL, NULL, NULL, NULL, &rotation, &m_sourcePosition );

			m_sourceObject->GetRenderables( frustum, renderables, m );
		}
		// The object's LOD is the highest of it's move, stretch, dest and source object's LODs
		m_lodLevel = EveLODHelper::MergeLOD( m_lodLevel, m_sourceObject->GetLODLevel() );
	}

	if( m_destObject && m_displayDestObject )
	{
		Quaternion rotation( 0.0f, 0.0f, 0.0f, 1.0f );
		Quaternion tmpResult;
		D3DXQuaternionMultiply(&rotation,				
			TriQuaternionRotationArc(&tmpResult, &Y_AXIS, &directionVec), &rotation);

		Vector3 scaling = Vector3( m_destObjectScale, m_destObjectScale, m_destObjectScale );
		Matrix m;
		D3DXMatrixTransformation( &m, NULL, NULL, &scaling, NULL, &rotation, &m_destinationPosition );

		m_destObject->GetRenderables( frustum, renderables, m );
		// The object's LOD is a combination of it's move, stretch, dest and source object's LODs
		m_lodLevel = EveLODHelper::MergeLOD( m_lodLevel, m_destObject->GetLODLevel() );
	}

	if( m_stretchObject )
	{
		Matrix m;

		if( m_useTransformsForStretch )
		{
			Matrix scaling;

			D3DXMatrixScaling( &scaling, 1.0f, 1.0f, scalingLength );
			D3DXMatrixMultiply( &m, &scaling, &m_sourceTransform );
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

			D3DXMatrixTransformation( &m, NULL, NULL, &scaling, NULL, &rotation, &m_sourcePosition );
		}

		m_stretchObject->GetRenderables( frustum, renderables, m );
		// The object's LOD is a combination of it's move, stretch, dest and source object's LODs
		m_lodLevel = EveLODHelper::MergeLOD( m_lodLevel, m_stretchObject->GetLODLevel() );
	}

	if( m_moveObject )
	{
		Matrix m;

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
			float progress = m_progressCurve->m_currentValue;
			if( progress >= 1.0 && !m_moveCompleted )
			{
				if( m_moveCompletion )
				{
					m_moveCompletion->Play();
				}
				m_moveObject->SetDisplay( false );
				m_moveCompleted = true;
			}
			D3DXVec3Lerp(&movedPostition, &m_sourcePosition, &m_destinationPosition, progress);
		}
		D3DXMatrixTransformation( &m, NULL, NULL, NULL, NULL, &rotation, &movedPostition );
		m_moveObject->GetRenderables( frustum, renderables, m );

		// The object's LOD is a combination of it's move, stretch, dest and source object's LODs
		m_lodLevel = EveLODHelper::MergeLOD( m_lodLevel, m_moveObject->GetLODLevel() );
	}
}

void EveStretch::Start()
{
	m_startTime = -1;
	m_moving = true;
	m_moveCompleted = false;
	
	if( m_moveObject )
	{
		m_moveObject->SetDisplay( true );
	}

	for( auto it = m_curveSets.begin(); it != m_curveSets.end(); it++ )
	{
		(*it)->Play();
	}
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

