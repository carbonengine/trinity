// Copyright © 2023 CCP ehf.

#include "StdAfx.h"
#include "EveDistanceField.h"

#include "TriView.h"
#include "EveUpdateContext.h"
#include "Curves/TriCurveSet.h"
#include "Curves/Tr2CurveScalar.h"

#include <ITriFunction.h>



EveDistanceField::EveDistanceField( IRoot* lockobj ) :
	PARENTLOCK( m_objects ),
	m_timeAdjustmentSecondsOut( 2.f ),
	m_timeAdjustmentSecondsIn( .25f ),
	m_distance( -1.f ),
	m_minDistance( 0.f ),
	m_maxDistance( 75000.f ),
	m_distanceThreshold( 3.f ),
	m_dirty( true ),
	m_updateDistanceCurve( false ),
	m_maxXZRatio( 1.5f ),
	m_minYRatio( 0.2f ),
	m_isDynamic( false )
{
	SetNeutralValues();
	m_objects.SetNotify( this );
}

float EveDistanceField::CalculateFieldCoverageAndDistance( Be::Time t, const Vector3& posRef, const Vector3& originShift )
{
	Vector3 minBounds( FLT_MAX, FLT_MAX, FLT_MAX );
	Vector3 maxBounds( -FLT_MAX, -FLT_MAX, -FLT_MAX );
	Vector3 averagePos( 0, 0, 0 );
	Vector3 posObj;

	float distanceNowSq = m_maxDistance * m_maxDistance;

	if( m_objects.empty() )
	{
		m_middle += originShift;
		return Length( posRef );
	}

	const float oneOverCount = 1.f / (float)m_objects.size();
	// Calculate bounds and center
	for( auto oit = m_objects.begin(); oit != m_objects.end(); ++oit )
	{
		( *oit )->GetValueAt( &posObj, t );
		averagePos += posObj * oneOverCount;
		posObj = posObj - posRef;
		distanceNowSq = std::min( distanceNowSq, LengthSq( posObj ) );
	}

	if( m_dirty )
	{
		// Find average distance from average position(can be calculated when list is modified)
		float averageDistance = 0;
		for( auto oit = m_objects.begin(); oit != m_objects.end(); ++oit )
		{
			( *oit )->GetValueAt( &posObj, t );
			posObj = posObj - averagePos;
			averageDistance += Length( posObj ) * oneOverCount;
		}

		// Calculate bounding box for objects close enough to the average position
		for( auto oit = m_objects.begin(); oit != m_objects.end(); ++oit )
		{
			( *oit )->GetValueAt( &posObj, t );
			const Vector3 d = posObj - averagePos;
			float distance = Length( d );
			if( m_distanceThreshold == 0.f || distance <= m_distanceThreshold * averageDistance )
			{
				minBounds = Minimize( posObj, minBounds );
				maxBounds = Maximize( posObj, maxBounds );
			}
		}

		m_middle = 0.5f * ( minBounds + maxBounds );

		// Now calculate the field size based on the bounding box and ratio constraints
		m_dimensions = maxBounds - minBounds;
		if( m_maxXZRatio && m_dimensions.x / m_dimensions.z > m_maxXZRatio )
		{
			m_dimensions.z = m_dimensions.x / m_maxXZRatio;
		}
		else if( m_maxXZRatio && m_dimensions.z / m_dimensions.x > m_maxXZRatio )
		{
			m_dimensions.x = m_dimensions.z / m_maxXZRatio;
		}
		if( m_minYRatio && m_dimensions.y / max( m_dimensions.x, m_dimensions.z ) < m_minYRatio )
		{
			m_dimensions.y = max( m_dimensions.x, m_dimensions.z ) * m_minYRatio;
		}
		m_dirty = false;
	}
	else
	{
		m_middle += originShift;
	}

	return std::sqrt( distanceNowSq );
}

void EveDistanceField::Update( const EveUpdateContext& updateContext )
{
	CCP_STATS_ZONE( __FUNCTION__ );

	Vector3 posRef( 0, 0, 0 );
	if( m_cameraView )
	{
		posRef = m_cameraView->GetTransform().GetTranslation();
	}
	Be::Time t = updateContext.GetTime();

	if( m_updateDistanceCurve )
	{
		UpdateDistanceCurveSize();
	}
	float distanceNow = m_maxDistance;
	if( m_distance < 0.f )
	{
		m_distance = m_maxDistance;
	}

	const Vector3 originShift = updateContext.GetOriginShift();
	if( m_isDynamic )
	{
		distanceNow = CalculateFieldCoverageAndDistance( t, posRef, originShift );
	}
	else
	{
		if( m_objects.size() == 1 )
		{
			m_objects[0]->GetValueAt( &m_middle, t );
		}
		else
		{
			m_middle += originShift;
		}

		Vector3 posObj = m_middle - posRef;
		distanceNow = std::min( distanceNow, Length( posObj ) );
	}

	float frac = ( ( distanceNow > m_distance ) ? m_timeAdjustmentSecondsOut : m_timeAdjustmentSecondsIn );
	float delta = ( updateContext.GetDeltaT() ) / ( frac ? frac : 1.f );
	if( delta > 1 )
	{
		delta = 1;
	}

	m_distance = m_distance * ( 1 - delta ) + distanceNow * delta;
	m_distance = min( m_distance, m_maxDistance );
	if( m_curveSet )
	{
		if( !m_curveSet->IsPlaying() )
		{
			m_curveSet->PlayFrom( m_distance );
		}
		else
		{
			m_curveSet->Update( m_distance );
		}
	}

	if( m_debug )
	{
		m_debugPositions.clear();
		for( auto it = m_objects.begin(); it != m_objects.end(); ++it )
		{
			Vector3 position;
			( *it )->GetValueAt( &position, t );
			m_debugPositions.push_back( position );
		}
		m_debug = false;
	}
}

void EveDistanceField::UpdateDistanceCurveSize()
{
	if( m_distanceCurve )
	{
		auto& keys = m_distanceCurve->GetKeys();
		if( keys.size() == 2 )
		{
			keys[0].m_time = m_minDistance;
			keys[1].m_time = m_maxDistance;
			m_distanceCurve->OnKeysChanged();
		}
		m_distanceCurve->SetTimeOffset( 0 );
		m_updateDistanceCurve = false;
	}
}

void EveDistanceField::CreateCurveSet()
{
	m_curveSet.CreateInstance();
	m_distanceCurve.CreateInstance();
	m_distanceCurve->SetName( "DistanceCurve" );
	m_distanceCurve->AddKey( 0, 1, Tr2CurveInterpolation::LINEAR, 0, 0, Tr2CurveTangentType::AUTO );
	m_distanceCurve->AddKey( 50000.0f, 0, Tr2CurveInterpolation::LINEAR, 0, 0, Tr2CurveTangentType::AUTO );
	m_distanceCurve->SetTimeOffset( 0 );

	m_curveSet->AddCurve( (ITriFunctionPtr)m_distanceCurve );
}

void EveDistanceField::SetupStaticDistanceField( Vector3 dimensions, Vector3 position, float distanceThreshold, float timeAdjustmentSecondsOut, float timeAdjustmentSecondsIn )
{
	m_isDynamic = false;
	m_dimensions = dimensions;
	m_distanceThreshold = distanceThreshold;
	m_timeAdjustmentSecondsIn = timeAdjustmentSecondsIn;
	m_timeAdjustmentSecondsOut = timeAdjustmentSecondsOut;
	m_middle = position;

	CreateCurveSet();
	UpdateDistanceCurveSize();
}

void EveDistanceField::SetupDynamicDistanceField( float distanceThreshold, float timeAdjustmentSecondsOut, float timeAdjustmentSecondsIn )
{
	m_isDynamic = true;
	m_dirty = true;
	m_distanceThreshold = distanceThreshold;
	m_timeAdjustmentSecondsIn = timeAdjustmentSecondsIn;
	m_timeAdjustmentSecondsOut = timeAdjustmentSecondsOut;

	CreateCurveSet();
}

void EveDistanceField::SetNeutralValues()
{
	m_middle = Vector3( 0, 0, 0 );
	m_dimensions = Vector3( 0, 0, 0 );
}

// --------------------------------------------------------------------------------------
// Description:
//   Implements IListNotify interface.
// Arguments:
//   event - List event type
//   key - First element index (unused)
//   key2 - Second element index (unused)
//   value - Element value
//   theList - The list being modified
// Return Value:
//   true always
// --------------------------------------------------------------------------------------
void EveDistanceField::OnListModified(
	long event,
	ssize_t key,
	ssize_t key2,
	IRoot* value,
	const IList* theList )
{
	if( theList != &m_objects )
	{
		return;
	}

	switch( event & BELIST_EVENTMASK )
	{
	case BELIST_INSERTED:
		m_dirty = true;
		break;
	case BELIST_REMOVED:
		if( m_objects.empty() )
		{
			SetNeutralValues();
		}
		break;
	default:
		break;
	};
}


// --------------------------------------------------------------------------------------
bool EveDistanceField::OnModified( Be::Var* value )
{

	if( IsMatch( value, m_maxDistance ) ||
		IsMatch( value, m_minDistance ) )
	{
		m_updateDistanceCurve = true;
	}

	return true;
}


// --------------------------------------------------------------------------------------
void EveDistanceField::GetDebugOptions( Tr2DebugRendererOptions& options )
{
	options.insert( "DistanceField" );
}


// --------------------------------------------------------------------------------
void EveDistanceField::RenderDebugInfo( ITr2DebugRenderer2& renderer )
{
	if( !renderer.HasOption( this, "DistanceField" ) )
	{
		return;
	}
	m_debug = true;

	// Show the dimension and midpoint of the field
	const Vector3 aabbMin = m_middle - m_dimensions * 0.5f;
	const Vector3 aabbMax = m_middle + m_dimensions * 0.5f;
	renderer.DrawSphere( this, m_middle, 1000.f, 3, Tr2DebugRenderer::Wireframe, 0xff00ff00 );
	renderer.DrawBox( this, aabbMin, aabbMax, Tr2DebugRenderer::Wireframe, 0xff00ff00 );
	char str[128];
	sprintf_s( str, "Object Count: %zd", m_objects.GetSize() );
	renderer.DrawText( TRI_DBG_FONT_LARGE, m_middle, 0xff00ff00, str );

	for( auto it = m_debugPositions.begin(); it != m_debugPositions.end(); ++it )
	{
		const Vector3 pos = *it;
		renderer.DrawLine( this, m_middle, pos, 0x7f00ff00 );
		renderer.DrawSphere( this, pos, m_minDistance, 8, Tr2DebugRenderer::Wireframe, 0x3f3f3f3f );
		renderer.DrawSphere( this, pos, m_maxDistance, 8, Tr2DebugRenderer::Wireframe, 0x1f1f1f1f );
	}
}
