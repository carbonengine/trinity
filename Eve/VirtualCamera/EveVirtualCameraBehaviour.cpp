#include "StdAfx.h"
#include "EveVirtualCameraBehaviour.h"
#include "EveVirtualCamera.h"
#include "Curves/Tr2CurveScalar.h"
#include "include/TriMath.h"

namespace
{
	static const Vector3 UP = Vector3( 0, 1, 0 );
	static const Vector3 FORWARD = Vector3( 0, 0, 1 );

	Vector3 RotateVectorWithAnchor( Vector3 value, Vector3 anchorForwardDirection )
	{

		Vector3 horizontalDirection = Normalize( Vector3( anchorForwardDirection.x, 0, anchorForwardDirection.z ) );
		auto rot = RotationQuaternion( Cross( FORWARD, horizontalDirection ), AngleFromNormalized( horizontalDirection, FORWARD ) );
		return XMVector3TransformCoord( Normalize( value ), XMMatrixRotationQuaternion( rot ) ) * Length( value );
	}

	void InitializeCurveAsEaseInOut( Tr2CurveScalarPtr& curve )
	{
		curve.CreateInstance();
		curve->SetExtrapolation( Tr2CurveExtrapolation::LINEAR );
		curve->AddKey( 0.0f, 0.0f, Tr2CurveInterpolation::HERMITE, 0, 0, Tr2CurveTangentType::AUTO_CLAMP );
		curve->AddKey( 1.0f, 1.0f, Tr2CurveInterpolation::HERMITE, 0, 0, Tr2CurveTangentType::AUTO_CLAMP );
	}

	void InitializeCurveAsConstant( Tr2CurveScalarPtr& curve, float value )
	{
		curve.CreateInstance();
		curve->SetExtrapolation( Tr2CurveExtrapolation::LINEAR );
		curve->AddKey( 0.0f, value, Tr2CurveInterpolation::HERMITE, 0, 0, Tr2CurveTangentType::AUTO_CLAMP );
		curve->AddKey( 1.0f, value, Tr2CurveInterpolation::HERMITE, 0, 0, Tr2CurveTangentType::AUTO_CLAMP );
	}

	// Get perlin noise in the range -1 to 1.
	float ClampedNoise(double offset, float frequency, int octaves)
	{
		return float( PerlinNoise1D( offset * frequency, 2.0, 2.0, octaves ) );
	}
}

// =============================================================================
// Float Behaviour Interface / Base
// =============================================================================

EveVirtualCameraBehaviourFloatBase::EveVirtualCameraBehaviourFloatBase( const char* name ):
	m_name( name ),
	m_active( true )
{}

EveVirtualCameraBehaviourFloatBase::~EveVirtualCameraBehaviourFloatBase()
{
}
	
const std::string& EveVirtualCameraBehaviourFloatBase::GetName() const
{
	return m_name;
}

void EveVirtualCameraBehaviourFloatBase::SetName( const std::string& name )
{
	m_name = name;
}

bool EveVirtualCameraBehaviourFloatBase::OnModified( Be::Var * value )
{
	if ( IsMatch( value, m_name ) )
	{
		SetName( m_name );
	}
	return true;
}

bool EveVirtualCameraBehaviourFloatBase::IsActive()
{
	return m_active;
}

// =============================================================================
// Set Value
// =============================================================================

EveVirtualCameraBehaviourFloatSet::EveVirtualCameraBehaviourFloatSet( IRoot* lockobj ) :
	EveVirtualCameraBehaviourFloatBase( "Set" ),
	m_value( 0.0f )
{
}

EveVirtualCameraBehaviourFloatSet::~EveVirtualCameraBehaviourFloatSet()
{
}

float EveVirtualCameraBehaviourFloatSet::Update( const EveVirtualCamera& camera, float current, float deltaTime, float localElapsedTime, const Vector3& anchorPosition, float anchorRadius, const Vector3& anchorForwardDirection )
{
	return m_value - current;
}

// =============================================================================
// Add Value
// =============================================================================

EveVirtualCameraBehaviourFloatAdd::EveVirtualCameraBehaviourFloatAdd( IRoot* lockobj ) :
	EveVirtualCameraBehaviourFloatBase( "Add" ),
	m_value( 0.0f ),
	m_scaleCurve()
{
	InitializeCurveAsConstant( m_scaleCurve, 1.0f );
	SetName( m_name );
}

EveVirtualCameraBehaviourFloatAdd::~EveVirtualCameraBehaviourFloatAdd()
{
}

void EveVirtualCameraBehaviourFloatAdd::SetName( const std::string& name )
{
	m_name = name;
	m_scaleCurve->SetName( ( name + " - Scale Curve" ).c_str() );
}

float EveVirtualCameraBehaviourFloatAdd::Update( const EveVirtualCamera& camera, float current, float deltaTime, float localElapsedTime, const Vector3& anchorPosition, float anchorRadius, const Vector3& anchorForwardDirection )
{
	if( m_scaleCurve )
	{
		return m_value * m_scaleCurve->GetValue( localElapsedTime / camera.GetAnimationTimelineLength() );
	}
	else
	{
		return m_value;
	}
}

// =============================================================================
// Noise
// =============================================================================

EveVirtualCameraBehaviourFloatNoise::EveVirtualCameraBehaviourFloatNoise( IRoot* lockobj ) :
	EveVirtualCameraBehaviourFloatBase( "Shake" ),
	m_frequency( 1.0f ),
	m_octaves( 8 ),
	m_magnitude( 1.0f ),
	m_magnitudeCurve()
{
	m_magnitudeCurve.CreateInstance();
	m_magnitudeCurve->SetExtrapolation( Tr2CurveExtrapolation::LINEAR );
	m_magnitudeCurve->AddKey( 0.0f, 0.0f, Tr2CurveInterpolation::HERMITE, 0, 0, Tr2CurveTangentType::AUTO_CLAMP );
	m_magnitudeCurve->AddKey( 0.001f, 0.8f, Tr2CurveInterpolation::HERMITE, 0, 0, Tr2CurveTangentType::AUTO_CLAMP );
	m_magnitudeCurve->AddKey( 0.1f, 1.0f, Tr2CurveInterpolation::HERMITE, 0, 0, Tr2CurveTangentType::AUTO_CLAMP );
	m_magnitudeCurve->AddKey( 1.0f, 0.0f, Tr2CurveInterpolation::HERMITE, 0, 0, Tr2CurveTangentType::AUTO_CLAMP );
	
	SetName( m_name );

	TriSrand( BeOS->GetCurrentFrameTime() );
}

EveVirtualCameraBehaviourFloatNoise::~EveVirtualCameraBehaviourFloatNoise()
{
}

void EveVirtualCameraBehaviourFloatNoise::SetName( const std::string& name )
{
	m_name = name;
	m_magnitudeCurve->SetName( ( name + " - Magnitude Curve" ).c_str() );
}

float EveVirtualCameraBehaviourFloatNoise::Update( const EveVirtualCamera& camera, float current, float deltaTime, float localElapsedTime, const Vector3& anchorPosition, float anchorRadius, const Vector3& anchorForwardDirection )
{
	auto offset = m_magnitude;

	offset *= ClampedNoise( localElapsedTime + double( uintptr_t( this ) & 0xfff ), m_frequency, m_octaves );

	if( m_magnitudeCurve )
	{
		offset *= m_magnitudeCurve->GetValue( localElapsedTime / camera.GetAnimationTimelineLength() );
	}

	return offset;
}

// =============================================================================
// Damping
// =============================================================================

EveVirtualCameraBehaviourFloatDamping::EveVirtualCameraBehaviourFloatDamping( IRoot* lockobj ) :
	EveVirtualCameraBehaviourFloatBase( "Damping" ),
	m_lastValue( 0.0f ),
	m_dampingRatio( 1.0f )
{
}

EveVirtualCameraBehaviourFloatDamping::~EveVirtualCameraBehaviourFloatDamping()
{
}

float EveVirtualCameraBehaviourFloatDamping::Update( const EveVirtualCamera& camera, float current, float deltaTime, float localElapsedTime, const Vector3& anchorPosition, float anchorRadius, const Vector3& anchorForwardDirection )
{
	if( localElapsedTime <= 0.0 )
	{
		m_lastValue = current;
		return 0.0f;
	}
	else
	{
		m_lastValue = m_lastValue + ( current - m_lastValue ) * m_dampingRatio;
		return 	m_lastValue - current;
	}
}

// =============================================================================
// Vector3 Behaviour Interface / Base
// =============================================================================

EveVirtualCameraBehaviourVector3Base::EveVirtualCameraBehaviourVector3Base( const char* name ) :
	m_name( name ),
	m_active( true )
{}

EveVirtualCameraBehaviourVector3Base::~EveVirtualCameraBehaviourVector3Base()
{
}
	
const std::string& EveVirtualCameraBehaviourVector3Base::GetName() const
{
	return m_name;
}

void EveVirtualCameraBehaviourVector3Base::SetName( const std::string& name )
{
	m_name = name;
}

bool EveVirtualCameraBehaviourVector3Base::OnModified( Be::Var * value )
{
	if ( IsMatch( value, m_name ) )
	{
		SetName( m_name );
	}
	return true;
}

bool EveVirtualCameraBehaviourVector3Base::IsActive()
{
	return m_active;
}

// =============================================================================
// Move Between / Lerp
// =============================================================================

EveVirtualCameraBehaviourVector3MoveBetween::EveVirtualCameraBehaviourVector3MoveBetween( IRoot* lockobj ) :
	EveVirtualCameraBehaviourVector3Base( "Move Between" ),
	m_start(0, 0, 0),
	m_end(0, 0, 0),
	m_interpolationCurve(),
	m_proportional(false),
	m_world(true)
{
	InitializeCurveAsEaseInOut( m_interpolationCurve );
	SetName( m_name );
}

EveVirtualCameraBehaviourVector3MoveBetween::~EveVirtualCameraBehaviourVector3MoveBetween()
{
}

void EveVirtualCameraBehaviourVector3MoveBetween::SetName( const std::string& name )
{
	m_name = name;
	m_interpolationCurve->SetName( ( name + " - Interpolation Curve" ).c_str() );
}

Vector3 EveVirtualCameraBehaviourVector3MoveBetween::Update( const EveVirtualCamera& camera, const Vector3& current, float deltaTime, float localElapsedTime, const Vector3& anchorPosition, float anchorRadius, const Vector3& anchorForwardDirection )
{
	auto start = m_start;
	auto end = m_end;
	if( m_proportional )
	{
		start *= anchorRadius;
		end *= anchorRadius;
	}
	if(!m_world)
	{
		start = RotateVectorWithAnchor( start, anchorForwardDirection );
		end = RotateVectorWithAnchor( end, anchorForwardDirection );
	}
	if( camera.GetAnimationTimelineLength() != 0.0f )
	{
		if( m_interpolationCurve )
		{
			return start + ( end - start ) * m_interpolationCurve->GetValue( localElapsedTime / camera.GetAnimationTimelineLength() );
		}
		else
		{
			return start + ( end - start ) * localElapsedTime / camera.GetAnimationTimelineLength();
		}
	}
	else
	{
		return m_end;
	}
}

// =============================================================================
// Offset
// =============================================================================

EveVirtualCameraBehaviourVector3Offset::EveVirtualCameraBehaviourVector3Offset( IRoot* lockobj ) :
	EveVirtualCameraBehaviourVector3Base( "Offset" ),
	m_offset(0, 0, 0),
	m_proportional(true),
	m_world(false)
{
}

EveVirtualCameraBehaviourVector3Offset::~EveVirtualCameraBehaviourVector3Offset()
{
}

Vector3 EveVirtualCameraBehaviourVector3Offset::Update( const EveVirtualCamera& camera, const Vector3& current, float deltaTime, float localElapsedTime, const Vector3& anchorPosition, float anchorRadius, const Vector3& anchorForwardDirection )
{
	Vector3 offset = m_offset;
	if( !m_world )
	{
		offset = RotateVectorWithAnchor( offset, anchorForwardDirection );
	}
	if( m_proportional )
	{
		return offset * anchorRadius;
	}
	else
	{
		return offset;
	}
}

// =============================================================================
// Orbit
// =============================================================================

EveVirtualCameraBehaviourVector3Orbit::EveVirtualCameraBehaviourVector3Orbit( IRoot* lockobj ) :
	EveVirtualCameraBehaviourVector3Base( "Orbit" ),
	m_start(0.0f),
	m_end(180.0f),
	m_distance(1.0f),
	m_distanceScalarCurve(),
	m_orbitCurve(),
	m_proportional( true ),
	m_world( false )
{
	InitializeCurveAsConstant( m_distanceScalarCurve, 1.0f );
	InitializeCurveAsEaseInOut( m_orbitCurve );
	SetName( m_name );
}

EveVirtualCameraBehaviourVector3Orbit::~EveVirtualCameraBehaviourVector3Orbit()
{
}

void EveVirtualCameraBehaviourVector3Orbit::SetName( const std::string& name )
{
	m_name = name;
	m_distanceScalarCurve->SetName( ( name + " - Distance Scalar Curve" ).c_str() );
	m_orbitCurve->SetName( ( name + " - Orbit Curve" ).c_str() );
}

Vector3 EveVirtualCameraBehaviourVector3Orbit::Update( const EveVirtualCamera& camera, const Vector3& current, float deltaTime, float localElapsedTime, const Vector3& anchorPosition, float anchorRadius, const Vector3& anchorForwardDirection )
{
	Vector3 orbitDir = Vector3( 0, 0, 1 );

	if( !m_world )
	{
		// project the forward onto our orbit plane, i.e. the horizontal plane.
		orbitDir = Normalize( Vector3( anchorForwardDirection.x, 0, anchorForwardDirection.z ) );
	}

	float angle = 0.0f;
	if( m_orbitCurve )
	{
		angle = m_start + ( m_end - m_start ) * m_orbitCurve->GetValue( localElapsedTime / camera.GetAnimationTimelineLength() );
	}
	else
	{
		angle = m_start + ( m_end - m_start ) * localElapsedTime / camera.GetAnimationTimelineLength();
	}
	auto rotation = RotationQuaternion( UP, XMConvertToRadians( angle ) );
	
	TriVectorRotateQuaternion( &orbitDir, &orbitDir, &rotation );

	float range = m_distance;
	if( m_proportional )
	{
		range *= anchorRadius;
	}
	if( m_distanceScalarCurve )
	{
		range *= m_distanceScalarCurve->GetValue( localElapsedTime / camera.GetAnimationTimelineLength() );
	}

	return orbitDir * range;
}

// =============================================================================
// Move Forward / Dolly
// =============================================================================

EveVirtualCameraBehaviourVector3MoveForward::EveVirtualCameraBehaviourVector3MoveForward( IRoot* lockobj, const char* name ) :
	EveVirtualCameraBehaviourVector3Base( name ),
	m_value( 0.0f ),
	m_scaleCurve(),
	m_proportional( true )
{
	InitializeCurveAsConstant( m_scaleCurve, 1.0f );
	SetName( m_name );
}

EveVirtualCameraBehaviourVector3MoveForward::~EveVirtualCameraBehaviourVector3MoveForward()
{
}

void EveVirtualCameraBehaviourVector3MoveForward::SetName( const std::string& name )
{
	m_name = name;
	m_scaleCurve->SetName( ( name + " - Scale Curve" ).c_str() );
}

Vector3 EveVirtualCameraBehaviourVector3MoveForward::Update( const EveVirtualCamera& camera, const Vector3& current, float deltaTime, float localElapsedTime, const Vector3& anchorPosition, float anchorRadius, const Vector3& anchorForwardDirection )
{
	return camera.GetForwardDirection() * GetCurrentValue( camera, localElapsedTime, anchorRadius );
}

float EveVirtualCameraBehaviourVector3MoveForward::GetCurrentValue( const EveVirtualCamera& camera, float localElapsedTime, float anchorRadius )
{
	float value = m_value;
	if( m_scaleCurve )
	{
		value *= m_scaleCurve->GetValue( localElapsedTime / camera.GetAnimationTimelineLength() );
	}
	if( m_proportional )
	{
		value *= anchorRadius;
	}
	return value;
}

// =============================================================================
// Move Right / Truck
// =============================================================================

EveVirtualCameraBehaviourVector3MoveRight::EveVirtualCameraBehaviourVector3MoveRight( IRoot* lockobj ) :
	EveVirtualCameraBehaviourVector3MoveForward( lockobj, "Move Right" )
{
}

Vector3 EveVirtualCameraBehaviourVector3MoveRight::Update( const EveVirtualCamera& camera, const Vector3& current, float deltaTime, float localElapsedTime, const Vector3& anchorPosition, float anchorRadius, const Vector3& anchorForwardDirection )
{
	return camera.GetRightDirection() * GetCurrentValue( camera, localElapsedTime, anchorRadius );
}

// =============================================================================
// Move Up / Pedestal
// =============================================================================

EveVirtualCameraBehaviourVector3MoveUp::EveVirtualCameraBehaviourVector3MoveUp( IRoot* lockobj ) :
	EveVirtualCameraBehaviourVector3MoveForward( lockobj, "Move Up" )
{
}

Vector3 EveVirtualCameraBehaviourVector3MoveUp::Update( const EveVirtualCamera& camera, const Vector3& current, float deltaTime, float localElapsedTime, const Vector3& anchorPosition, float anchorRadius, const Vector3& anchorForwardDirection )
{
	return camera.GetUpDirection() * GetCurrentValue( camera, localElapsedTime, anchorRadius );
}

// =============================================================================
// Shake
// =============================================================================

EveVirtualCameraBehaviourVector3Shake::EveVirtualCameraBehaviourVector3Shake( IRoot* lockobj ) :
	EveVirtualCameraBehaviourVector3Base( "Shake" ),
	m_frequency( 1.0f ),
	m_octaves( 8 ),
	m_magnitude( 1.0f, 0.6f, 0.2f ),
	m_magnitudeCurve(),
	m_scaleByView( true )
{
	m_magnitudeCurve.CreateInstance();
	m_magnitudeCurve->SetExtrapolation( Tr2CurveExtrapolation::LINEAR );
	m_magnitudeCurve->AddKey( 0.0f, 0.0f, Tr2CurveInterpolation::HERMITE, 0, 0, Tr2CurveTangentType::AUTO_CLAMP );
	m_magnitudeCurve->AddKey( 0.001f, 0.8f, Tr2CurveInterpolation::HERMITE, 0, 0, Tr2CurveTangentType::AUTO_CLAMP );
	m_magnitudeCurve->AddKey( 0.1f, 1.0f, Tr2CurveInterpolation::HERMITE, 0, 0, Tr2CurveTangentType::AUTO_CLAMP );
	m_magnitudeCurve->AddKey( 1.0f, 0.0f, Tr2CurveInterpolation::HERMITE, 0, 0, Tr2CurveTangentType::AUTO_CLAMP );
	
	SetName( m_name );
}

EveVirtualCameraBehaviourVector3Shake::~EveVirtualCameraBehaviourVector3Shake()
{
}

void EveVirtualCameraBehaviourVector3Shake::SetName( const std::string& name )
{
	m_name = name;
	m_magnitudeCurve->SetName( ( name + " - Magnitude Curve" ).c_str() );
}

Vector3 EveVirtualCameraBehaviourVector3Shake::Update( const EveVirtualCamera& camera, const Vector3& current, float deltaTime, float localElapsedTime, const Vector3& anchorPosition, float anchorRadius, const Vector3& anchorForwardDirection )
{
	auto offset = m_magnitude;

	offset.x *= ClampedNoise( localElapsedTime + double( uintptr_t( this ) & 0xfff ) + 1.1f, m_frequency, m_octaves );
	offset.y *= ClampedNoise( localElapsedTime + double( uintptr_t( this ) & 0xfff ) + 10.1f, m_frequency, m_octaves );
	offset.z *= ClampedNoise( localElapsedTime + double( uintptr_t( this ) & 0xfff ) + 18.3f, m_frequency, m_octaves );

	if( m_magnitudeCurve )
	{
		offset *= m_magnitudeCurve->GetValue( localElapsedTime / camera.GetAnimationTimelineLength() );
	}
	
	if(m_scaleByView)
	{
		offset.x = atan(offset.x) * Length( camera.GetPointOfInterest() - camera.GetPosition() );
		offset.y = atan(offset.y) * Length( camera.GetPointOfInterest() - camera.GetPosition() );
		offset.z = atan(offset.z) * Length( camera.GetPointOfInterest() - camera.GetPosition() );
	}

	return camera.GetRightDirection() * offset.x + camera.GetUpDirection() * offset.y + camera.GetForwardDirection() * offset.z;
}

// =============================================================================
// Damping
// =============================================================================

EveVirtualCameraBehaviourVector3Damping::EveVirtualCameraBehaviourVector3Damping( IRoot* lockobj ) :
	EveVirtualCameraBehaviourVector3Base( "Damping" ),
	m_lastPosition( 0, 0, 0 ),
	m_dampingRatio( 1.0f )
{
}

EveVirtualCameraBehaviourVector3Damping::~EveVirtualCameraBehaviourVector3Damping()
{
}

Vector3 EveVirtualCameraBehaviourVector3Damping::Update( const EveVirtualCamera& camera, const Vector3& current, float deltaTime, float localElapsedTime, const Vector3& anchorPosition, float anchorRadius, const Vector3& anchorForwardDirection )
{
	if( localElapsedTime <= 0.0 )
	{
		m_lastPosition = current;
		return Vector3( 0, 0, 0 );
	}
	else
	{
		m_lastPosition = m_lastPosition + ( current - m_lastPosition ) * m_dampingRatio;
		return 	m_lastPosition - current;
	}
} 


// =============================================================================
// Intertia
// =============================================================================

EveVirtualCameraBehaviourVector3Inertia::EveVirtualCameraBehaviourVector3Inertia( IRoot* lockobj ) :
	EveVirtualCameraBehaviourVector3Base( "Inertia" ),
	m_lastPosition( 0, 0, 0 ),
	m_lastVelocity( 0, 0, 0 ),
	m_inertiaFactor( 1.0f )
{
}

EveVirtualCameraBehaviourVector3Inertia::~EveVirtualCameraBehaviourVector3Inertia()
{
}

Vector3 EveVirtualCameraBehaviourVector3Inertia::Update( const EveVirtualCamera& camera, const Vector3& current, float deltaTime, float localElapsedTime, const Vector3& anchorPosition, float anchorRadius, const Vector3& anchorForwardDirection )
{
	if( localElapsedTime <= 0.0 )
	{
		m_lastVelocity = Vector3( 0, 0, 0 );
		m_lastPosition = current;
		return Vector3( 0, 0, 0 );
	}
	else
	{
		auto velocity = m_lastVelocity + ( ( current - m_lastPosition ) - m_lastVelocity ) * ( 1.0f / m_inertiaFactor );
		m_lastPosition = m_lastPosition + velocity;
		m_lastVelocity = velocity * deltaTime;
		return 	m_lastPosition - current;
	}
} 
