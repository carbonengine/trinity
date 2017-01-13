#include "StdAfx.h"
#include "EveDistanceField.h"

#include "EveCamera.h"
#include "TriView.h"
#include "EveUpdateContext.h"
#include "Curves/TriCurveSet.h"
#include "Curves/Tr2ScalarCurve.h"


static const float MAX_DISTANCE = 1e6;

EveDistanceField::EveDistanceField( IRoot* lockobj ) :
	PARENTLOCK( m_objects ),
	m_timeAdjustmentSecondsOut( 2.f ),
	m_timeAdjustmentSecondsIn( .25f ),
	m_distance( MAX_DISTANCE ),
	m_distanceThreshold( 3.f ),
	m_dirty( false ),
	m_maxXZRatio( 1.5f ),
	m_minYRatio( 0.2f ),
	m_isDynamic( false )
{
	m_objects.SetNotify( this );
}

void EveDistanceField::CalculateFieldCoverage( Be::Time t )
{
	Vector3 minBounds( FLT_MAX, FLT_MAX, FLT_MAX );
	Vector3 maxBounds( FLT_MIN, FLT_MIN, FLT_MIN );
	Vector3 averagePos( 0, 0, 0 );
	Vector3 posObj;

	if( m_objects.empty() )
	{
		return;
	}
	float oneOverCount = 1.f / (float)m_objects.size();

	// Find average distance from average position(can be calculated when list is modified)
	float averageDistance = 0;
	for( auto oit = m_objects.begin(); oit != m_objects.end(); ++oit )
	{
		(*oit)->GetValueAt( &posObj, t );
		averageDistance += D3DXVec3Length( D3DXVec3Subtract( &posObj, &posObj, &averagePos ) ) * oneOverCount;
	}

	Vector3 d;
	// Calculate bounding box for objects close enough to the average position
	for( auto oit = m_objects.begin(); oit != m_objects.end(); ++oit )
	{
		(*oit)->GetValueAt( &posObj, t );
		float distance = D3DXVec3Length( D3DXVec3Subtract( &d, &posObj, &averagePos ) );
		if( m_distanceThreshold == 0.f || distance < m_distanceThreshold * averageDistance )
		{
			D3DXVec3Minimize( &minBounds, &posObj, &minBounds );
			D3DXVec3Maximize( &maxBounds, &posObj, &maxBounds );
		}
	}

	D3DXVec3Scale( &m_middle, D3DXVec3Add( &posObj, &minBounds, &maxBounds ), 0.5 );

	// Now calculate the field size based on the bounding box and ratio constraints
	D3DXVec3Subtract( &m_dimensions, &maxBounds, &minBounds );
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
}

void EveDistanceField::Update( const EveUpdateContext& updateContext )
{
	CCP_STATS_ZONE( __FUNCTION__ );

	Vector3 posObj;

	Vector3 posRef( 0, 0, 0 );
	if( m_cameraView )
	{
		posRef = m_cameraView->GetTransform().GetTranslation();
	}
	Be::Time t = updateContext.GetTime();
	
	float distanceNow = MAX_DISTANCE;
	
	if( m_isDynamic )
	{
		auto count = m_objects.size();
		if( count )
		{
			distanceNow = MAX_DISTANCE * MAX_DISTANCE;
			if( m_dirty )
			{
				CalculateFieldCoverage( t );
				m_dirty = false;
			}
			else
			{
				Vector3 originShift = updateContext.GetOriginShift();
				m_middle += originShift;
			}

			// Calculate bounds and center
			for( auto oit = m_objects.begin(); oit != m_objects.end(); ++oit )
			{
				(*oit)->GetValueAt( &posObj, t );
				D3DXVec3Subtract( &posObj, &posObj, &posRef );
				distanceNow = min( distanceNow, D3DXVec3LengthSq( &posObj ) );
			}
			distanceNow = sqrt( distanceNow );
		}
	}
	else
	{	
		D3DXVec3Subtract( &posObj, &m_middle, &posRef );
		distanceNow = min( distanceNow, D3DXVec3Length( &posObj ) );	
	}

	float frac = ( ( distanceNow > m_distance ) ? m_timeAdjustmentSecondsOut : m_timeAdjustmentSecondsIn );
	float delta = ( updateContext.GetDeltaT() ) / ( frac ? frac : 1.f );
	if( delta > 1 )
	{
		delta = 1;
	}

	m_distance = m_distance * ( 1 - delta ) + distanceNow * delta;
	m_distance = min( m_distance, MAX_DISTANCE );
	if( m_curveSet )
	{
		if( !m_curveSet->IsPlaying() )
		{
			m_curveSet->Play();
			m_curveSet->Update( 0 );
		}
		m_curveSet->Update( m_distance );
	}
}

void EveDistanceField::CreateCurveSet()
{
	m_curveSet.CreateInstance();
	Tr2ScalarCurvePtr distanceCurve;
	distanceCurve.CreateInstance();
	distanceCurve->m_name = "DistanceCurve";
	distanceCurve->m_length = 50000.0f;
	distanceCurve->m_timeOffset = 25000.0f;
	distanceCurve->m_startValue = 1.0f;

	m_curveSet->AddCurve( (ITriFunctionPtr)distanceCurve );
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
	const IList* theList
	)
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