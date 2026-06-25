// Copyright © 2012 CCP ehf.

#include "StdAfx.h"
#include "Tr2CapsuleShapeAttributeGenerator.h"
#include "Tr2ParticleSystem.h"

// --------------------------------------------------------------------------------------
// Description:
//   Returns a random number between "a" and "b".
// Arguments:
//   a - Lower limit for random value range.
//   b - Upper limit for random value range.
// Return Value:
//   Random number between "a" and "b".
// --------------------------------------------------------------------------------------
inline float frand( float a, float b )
{
	return a + ( b - a ) * Tr2ParticleSystem::RandFloat();
}

// --------------------------------------------------------------------------------------
// Description:
//   Tr2CapsuleShapeAttributeGenerator default constructor
// --------------------------------------------------------------------------------------
Tr2CapsuleShapeAttributeGenerator::Tr2CapsuleShapeAttributeGenerator() :
	m_positionStart( 0.0f, 0.0f, 0.0f ),
	m_positionEnd( 0.0f, 0.0f, 0.0f ),
	m_rotationStart( 0.f, 0.f, 0.f, 1.f ),
	m_rotationEnd( 0.f, 0.f, 0.f, 1.f ),
	m_minPhi( 0.0f ),
	m_maxPhi( 360.0f ),
	m_minTheta( 0.0f ),
	m_maxTheta( 360.0f ),
	m_minRadius( 0.0f ),
	m_maxRadius( 00.0f ),
	m_controlVelocity( true ),
	m_minSpeed( 0.0f ),
	m_maxSpeed( 0.0f ),
	m_parentVelocityFactor( 1.0f ),
	m_valid( false )
{
	m_positionElement.m_offset = -1;
	m_velocityElement.m_offset = -1;
}

// --------------------------------------------------------------------------------------
// Description:
//   Tr2CapsuleShapeAttributeGenerator destructor
// --------------------------------------------------------------------------------------
Tr2CapsuleShapeAttributeGenerator::~Tr2CapsuleShapeAttributeGenerator()
{
}

// --------------------------------------------------------------------------------------
// Description:
//   Implements ITr2AttributeGenerator interface. Generates random values for new
//   particle component (element).
// Arguments:
//   position - Position of the "parent" particle.
//   velocity - Velocity of the "parent" particle.
//   paticle - (out) New particle data: Tr2ParticleElementData::COUNT of float arrays.
//		The generator fills position and/or velocity of the new particle.
// --------------------------------------------------------------------------------------
void Tr2CapsuleShapeAttributeGenerator::Generate( const Vector3* position,
												  const Vector3* velocity,
												  float** particle )
{
	if( !m_valid )
	{
		return;
	}

	float phi = XMConvertToRadians( frand( m_minPhi, m_maxPhi ) );
	float theta = XMConvertToRadians( frand( m_minTheta, m_maxTheta ) );

	// This is not exactly uniform, but it doesn't matter
	XMVECTOR randomVector = XMVectorSet( sin( phi ) * cos( theta ),
										 -cos( phi ),
										 sin( phi ) * sin( theta ),
										 0.f );
	float t = frand( 0.f, 1.f );
	XMVECTOR rotation = XMQuaternionSlerp( m_rotationStart, m_rotationEnd, t );
	randomVector = XMQuaternionMultiply(
		XMQuaternionMultiply( XMQuaternionConjugate( rotation ), randomVector ),
		rotation );

	if( m_controlVelocity && m_velocityElement.m_offset != -1 )
	{
		float speed = frand( m_minSpeed, m_maxSpeed );
		XMVECTOR particleVelocity = XMVectorScale( randomVector, speed );
		if( velocity )
		{
			particleVelocity = XMVectorAdd(
				particleVelocity,
				XMVectorScale( XMLoadFloat4A( reinterpret_cast<const XMFLOAT4A*>( velocity ) ), m_parentVelocityFactor ) );
		}
		XMStoreFloat4A(
			reinterpret_cast<XMFLOAT4A*>( particle[m_velocityElement.m_bufferType] + m_velocityElement.m_offset ),
			particleVelocity );
	}
	if( m_positionElement.m_offset != -1 )
	{
		randomVector = XMVectorScale( randomVector, frand( m_minRadius, m_maxRadius ) );
		if( position )
		{
			randomVector = XMVectorAdd(
				randomVector,
				XMLoadFloat4A( reinterpret_cast<const XMFLOAT4A*>( position ) ) );
		}
		randomVector = XMVectorAdd(
			randomVector,
			XMVectorLerp(
				XMLoadFloat4( reinterpret_cast<const XMFLOAT4*>( &m_positionStart ) ),
				XMLoadFloat4( reinterpret_cast<const XMFLOAT4*>( &m_positionEnd ) ),
				t ) );
		XMStoreFloat4A(
			reinterpret_cast<XMFLOAT4A*>( particle[m_positionElement.m_bufferType] + m_positionElement.m_offset ),
			randomVector );
	}
}

// --------------------------------------------------------------------------------------
// Description:
//   "Binds" generator to a particle system. Searches for particle elements POSITION and
//   VELOCITY.
// Arguments:
//   declaration - Particle element data coming from particle system.
//   boundElements - (in/out) The generator is expected to mark particle elements it will
//		be filling by adding their declaration names to this set. Emitter uses this set
//      to check if all particle elements were bound to some generator. The generator
//		is responsible for checking if its elements are overwritten by some other
//		generator using this set.
// Return Value:
//   true If the generator successfully binds to the particle system
//   false Otherwise
// --------------------------------------------------------------------------------------
bool Tr2CapsuleShapeAttributeGenerator::Bind(
	const Tr2ParticleElementDataMap& declaration,
	std::set<Tr2ParticleElementDeclarationName>& boundElements )
{
	m_valid = false;
	m_positionElement.m_offset = -1;
	m_velocityElement.m_offset = -1;

	Tr2ParticleElementDeclarationName position( Tr2ParticleElementDeclarationName::POSITION );
	Tr2ParticleElementDeclarationName velocity( Tr2ParticleElementDeclarationName::VELOCITY );

	auto i = declaration.find( position );
	if( i != declaration.end() )
	{
		if( boundElements.find( position ) != boundElements.end() )
		{
			CCP_LOGERR( "Multiple bindings to particle element POSITION" );
			return false;
		}
		m_positionElement = i->second;
		boundElements.insert( position );
	}
	if( m_controlVelocity )
	{
		i = declaration.find( velocity );
		if( i != declaration.end() )
		{
			if( boundElements.find( velocity ) != boundElements.end() )
			{
				CCP_LOGERR( "Multiple bindings to particle element VELOCITY" );
				return false;
			}
			m_velocityElement = i->second;
			boundElements.insert( velocity );
		}
	}

	if( m_positionElement.m_offset != -1 && ( !m_controlVelocity || m_velocityElement.m_offset != -1 ) )
	{
		m_valid = true;
		return true;
	}
	CCP_LOGERR( "Could not find particle element POSITION and/or VELOCITY in particle system" );
	return false;
}

// --------------------------------------------------------------------------------------
// Description:
//   Returns human-readable name for generator's declaration element. Used for Python
//   exposure.
// Return Value:
//   Human-readable name of particle declaration element.
// --------------------------------------------------------------------------------------
std::string Tr2CapsuleShapeAttributeGenerator::GetName() const
{
	if( m_controlVelocity )
	{
		return "POSITION + VELOCITY";
	}
	return "POSITION";
}

// --------------------------------------------------------------------------------------
// Description:
//   Assigns starting and ending emitter position and orientation.
// Arguments:
//   startPosition - Starting emitter position.
//   startRotation - Starting emitter orientation.
//   endPosition - Ending emitter position.
//   endRotation - Ending emitter orientation.
// --------------------------------------------------------------------------------------
void Tr2CapsuleShapeAttributeGenerator::SetPositions(
	const Vector3& startPosition,
	const Quaternion& startRotation,
	const Vector3& endPosition,
	const Quaternion& endRotation )
{
	m_positionStart = startPosition;
	m_rotationStart = startRotation;
	m_positionEnd = endPosition;
	m_rotationEnd = endRotation;
}
