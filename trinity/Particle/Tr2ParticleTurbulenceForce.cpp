// Copyright © 2011 CCP ehf.

#include "StdAfx.h"
#include "Tr2ParticleTurbulenceForce.h"

namespace
{

// Simplified 4D noise implementation

static const unsigned NOISE_MAX_CELL = 255;
static const unsigned NOISE_CELL_COUNT = NOISE_MAX_CELL + 1;
static XMFLOAT4A s_noiseLookup[NOISE_CELL_COUNT * 3];
static int s_permutations[NOISE_CELL_COUNT * 3];

float s_rand()
{
	return float( rand() ) / float( RAND_MAX ) - 0.5f;
}

struct InitializeNoise
{
	InitializeNoise()
	{
		for( int i = 0; i < NOISE_CELL_COUNT; i++ )
		{
			s_noiseLookup[i] = XMFLOAT4A( s_rand(), s_rand(), s_rand(), s_rand() );
			s_permutations[i] = i;
		}

		int i = NOISE_CELL_COUNT;
		while( --i )
		{
			std::swap( s_permutations[i], s_permutations[rand() % NOISE_CELL_COUNT] );
		}

		for( int i = 0; i < NOISE_CELL_COUNT; i++ )
		{
			s_permutations[NOISE_CELL_COUNT + i] = s_permutations[i];
			s_noiseLookup[NOISE_CELL_COUNT + i] = s_noiseLookup[i];
			s_permutations[NOISE_CELL_COUNT * 2 + i] = s_permutations[i];
			s_noiseLookup[NOISE_CELL_COUNT * 2 + i] = s_noiseLookup[i];
		}
	}
};

InitializeNoise s_initializeNoise;

#if defined( __GNUC__ )
#define ALIGN_16 __attribute__( ( aligned( 16 ) ) )
#else
#define ALIGN_16 __declspec( align( 16 ) )
#endif

XMVECTOR Noise4D( FXMVECTOR position )
{
	XMVECTOR dim = XMVectorReplicateInt( NOISE_MAX_CELL );
	XMVECTOR p = XMVectorAdd( position, XMVectorReplicate( 4096.f ) );
	XMVECTOR ipos = XMConvertVectorFloatToInt( p, 0 );
	XMVECTOR ipos1 = XMConvertVectorFloatToInt( XMVectorAdd( p, XMVectorSplatOne() ), 0 );
	XMVECTOR t = XMVectorSubtract( position, XMVectorFloor( position ) );
	ipos = XMVectorAndInt( ipos, dim );
	ipos1 = XMVectorAndInt( ipos1, dim );
	ALIGN_16 unsigned a[4], b[4];
	XMStoreInt4A( a, ipos );
	XMStoreInt4A( b, ipos1 );

	int i = s_permutations[a[0]];
	int j = s_permutations[b[0]];

	int b00 = s_permutations[i + a[1]];
	int b10 = s_permutations[j + a[1]];
	int b01 = s_permutations[i + b[1]];
	int b11 = s_permutations[j + b[1]];

	// Don't see a reason why we'd want force to have continous derivative, so
	// we simply use linear interpolation
	//t = XMVectorMultiply( t,
	//	XMVectorMultiply( t,
	//		XMVectorSubtract( XMVectorReplicate( 3.f ), XMVectorMultiply( XMVectorReplicate( 2.f ), t ) ) ) );

	XMVECTOR x = XMVectorSplatX( t );
	XMVECTOR c00 = XMVectorLerpV(
		XMLoadFloat4A( &s_noiseLookup[b00 + a[2] + a[3]] ),
		XMLoadFloat4A( &s_noiseLookup[b10 + a[2] + a[3]] ),
		x );
	XMVECTOR c10 = XMVectorLerpV(
		XMLoadFloat4A( &s_noiseLookup[b01 + a[2] + a[3]] ),
		XMLoadFloat4A( &s_noiseLookup[b11 + a[2] + a[3]] ),
		x );
	XMVECTOR c01 = XMVectorLerpV(
		XMLoadFloat4A( &s_noiseLookup[b00 + b[2] + a[3]] ),
		XMLoadFloat4A( &s_noiseLookup[b10 + b[2] + a[3]] ),
		x );
	XMVECTOR c11 = XMVectorLerpV(
		XMLoadFloat4A( &s_noiseLookup[b01 + b[2] + a[3]] ),
		XMLoadFloat4A( &s_noiseLookup[b11 + b[2] + a[3]] ),
		x );
	XMVECTOR c0 = XMVectorLerpV( c00, c10, XMVectorSplatY( t ) );
	XMVECTOR c1 = XMVectorLerpV( c01, c11, XMVectorSplatY( t ) );
	XMVECTOR c = XMVectorLerpV( c0, c1, XMVectorSplatZ( t ) );

	c00 = XMVectorLerpV(
		XMLoadFloat4A( &s_noiseLookup[b00 + a[2] + b[3]] ),
		XMLoadFloat4A( &s_noiseLookup[b10 + a[2] + b[3]] ),
		x );
	c10 = XMVectorLerpV(
		XMLoadFloat4A( &s_noiseLookup[b01 + a[2] + b[3]] ),
		XMLoadFloat4A( &s_noiseLookup[b11 + a[2] + b[3]] ),
		x );
	c01 = XMVectorLerpV(
		XMLoadFloat4A( &s_noiseLookup[b00 + b[2] + b[3]] ),
		XMLoadFloat4A( &s_noiseLookup[b10 + b[2] + b[3]] ),
		x );
	c11 = XMVectorLerpV(
		XMLoadFloat4A( &s_noiseLookup[b01 + b[2] + b[3]] ),
		XMLoadFloat4A( &s_noiseLookup[b11 + b[2] + b[3]] ),
		x );
	c0 = XMVectorLerpV( c00, c10, XMVectorSplatY( t ) );
	c1 = XMVectorLerpV( c01, c11, XMVectorSplatY( t ) );
	XMVECTOR d = XMVectorLerpV( c0, c1, XMVectorSplatZ( t ) );
	return XMVectorLerpV( c, d, XMVectorSplatW( t ) );
}

}

Tr2ParticleTurbulenceForce::Tr2ParticleTurbulenceForce( IRoot* lockobj ) :
	m_amplitude( 1.f, 1.f, 1.f ),
	m_frequency( 1.f, 1.f, 1.f, 1.f ),
	m_noiseLevel( 3 ),
	m_noiseRatio( 0.5f ),
	m_time( 0.f )
{
}

Tr2ParticleTurbulenceForce::~Tr2ParticleTurbulenceForce()
{
}


// -------------------------------------------------------------
// Description:
//   Computes turbulence force for a given particle position.
// Arguments:
//   position - particle position
//   velocity - particle velocity (not used)
//   dt - Frame time (not used)
//   mass - Particle mass (not used)
// Return value:
//	 Turbulence force for the particle
// -------------------------------------------------------------
XMVECTOR Tr2ParticleTurbulenceForce::GetForce( FXMVECTOR position, FXMVECTOR velocity, float dt, float mass )
{
	if( m_noiseLevel == 0 )
	{
		return XMVectorReplicate( 0.f );
	}

	XMVECTOR pos = XMVectorSetW( position, m_time );

	pos = XMVectorMultiply( pos, m_frequency );

	XMVECTOR noise = XMVectorReplicate( 0.f );

	XMVECTOR power = XMVectorReplicate( 0.5f );
	XMVECTOR frequency = XMVectorReplicate( 1.f / m_noiseRatio );
	XMVECTOR sum = XMVectorReplicate( 0.f );
	XMVECTOR noiseRatio = XMVectorReplicate( m_noiseRatio );
	for( unsigned i = 0; i < m_noiseLevel; ++i )
	{
		noise = XMVectorAdd( noise, XMVectorMultiply( Noise4D( pos ), power ) );
		sum = XMVectorAdd( sum, power );
		pos = XMVectorMultiply( pos, frequency );
		power = XMVectorMultiply( power, noiseRatio );
	}
	noise = XMVectorMultiply( noise, XMVectorMultiply( m_amplitude, sum ) );
	noise = XMVectorSetW( noise, 0.f );
	return noise;
}

// -------------------------------------------------------------
// Description:
//   Updates timer for the time-dependant turbulence.
// Arguments:
//   time - current time
// -------------------------------------------------------------
void Tr2ParticleTurbulenceForce::Update( float dt )
{
	m_time += dt;
}
