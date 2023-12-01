////////////////////////////////////////////////////////////
//
//    Created:   June 2014
//    Copyright: CCP 2014
//

#include "StdAfx.h"
#include "Tr2ShLightingManager.h"
#include "Lights/Tr2PointLight.h"

CCP_STATS_DECLARE( shLightingSamples, "Trinity/shLighting/samples", true, CST_COUNTER_LOW, "How many SH lighting samples are evaluated per frame?" );
CCP_STATS_DECLARE( shLightingSecondarySources, "Trinity/shLighting/secondarySources", true, CST_COUNTER_LOW, "How many SH lighting secondary sources are evaluated per frame?" );

namespace
{

// Imeratively chosen threshold for distance/radius*intensity value to decide if the light affects a sample
static const float s_cutoffRadiusRatio = 0.045f * 7.f;

template<int Order>
class ShSolver
{
};

// --------------------------------------------------------------------------------------
// Description:
//   ShSolver<L1> class evaluates L1 SH coefficients for spherical light sources.
// --------------------------------------------------------------------------------------
template<>
class ShSolver<Tr2ShLightingManager::L1>
{
public:
	inline static void SHEvalSphericalLight(
		FXMVECTOR direction,
		float distance,
		float radius,
		float *result )
	{
		float o0, o1;
		if( distance <= radius )
		{
			o0 = 1;
			o1 = 1;
		}
		else
		{
			o1 = ( radius / distance ) * ( radius / distance );
			o0 = 1.f - sqrt( 1.f - o1 );
		}

		XMFLOAT3A vd;
		XMStoreFloat3A( &vd, direction );

		result[0] = o0;
		result[1] = vd.y * o1;
		result[2] = vd.z * o1;
		result[3] = vd.x * o1;
	}

	inline static void NormalizeShCoefficients( XMVECTOR* coefficients, float scale )
	{
		for( int i = 0; i < 4; ++i )
		{
			coefficients[i] = XMVectorScale( coefficients[i], s_normalizationCoefficients[i] * scale );
		}
	}

	inline static void PackCoefficients( XMVECTOR* coefficients, Vector4* packedCoefficients )
	{
		for( int i = 0; i < 3; i++ )
		{
			packedCoefficients[i].x = -s_packCoefficient1 * XMVectorGetByIndex( coefficients[3], i );
			packedCoefficients[i].y = -s_packCoefficient1 * XMVectorGetByIndex( coefficients[1], i );
			packedCoefficients[i].z = s_packCoefficient1 * XMVectorGetByIndex( coefficients[2], i );
			packedCoefficients[i].w = s_packCoefficient0 * XMVectorGetByIndex( coefficients[0], i );
		}
	}

	static const size_t ORDER = 2;
private:
	static const float s_packCoefficient0;
	static const float s_packCoefficient1;
	static const float s_normalizationCoefficients[4];
};

const float ShSolver<Tr2ShLightingManager::L1>::s_packCoefficient0 = 1.0f / ( 2.0f * sqrt( XM_PI ) );
const float ShSolver<Tr2ShLightingManager::L1>::s_packCoefficient1 = sqrt( 3.0f ) / ( 3.0f * sqrt( XM_PI ) );
const float ShSolver<Tr2ShLightingManager::L1>::s_normalizationCoefficients[4] = {
	float( 2.0f * sqrt( XM_PI ) * 0.282094791773878140f * sqrt( 0.3141593E1f ) ),
	float( 2.0f / 3.0f * sqrt( 3.0f * XM_PI ) * -0.488602511902919920f * ( sqrt( 3.0f ) * sqrt( 0.3141593E1f ) / 2.0f ) ),
	float( 2.0f / 3.0f * sqrt( 3.0f * XM_PI ) * 0.488602511902919920f * ( sqrt( 3.0f ) * sqrt( 0.3141593E1f ) / 2.0f ) ),
	float( 2.0f / 3.0f * sqrt( 3.0f * XM_PI ) * -0.488602511902919920f * ( sqrt( 3.0f ) * sqrt( 0.3141593E1f ) / 2.0f ) ),
};


// --------------------------------------------------------------------------------------
// Description:
//   ShSolver<L2> class evaluates L2 SH coefficients for spherical light sources.
// --------------------------------------------------------------------------------------
template<>
class ShSolver<Tr2ShLightingManager::L2>
{
public:
	inline static void SHEvalSphericalLight( 
		FXMVECTOR direction,
		float distance,
		float radius,
		float *result )
	{
		float sinAngle, cosAngle;
		if( distance <= radius )
		{
			sinAngle = 1.f;
			cosAngle = 0.f;
		}
		else
		{
			sinAngle = radius / distance;
			cosAngle = sqrt( 1.f - sinAngle * sinAngle );
		}

		float coefficients[9];
		float angularCoefficients[3];

		ComputeCapInt( sinAngle, cosAngle, angularCoefficients );

		XMFLOAT3A vd;
		XMStoreFloat3A( &vd, direction );

		EvalBasis( vd.x, vd.y, vd.z, coefficients );

		result[0] = coefficients[0] * angularCoefficients[0];
		result[1] = coefficients[1] * angularCoefficients[1];
		result[2] = coefficients[2] * angularCoefficients[1];
		result[3] = coefficients[3] * angularCoefficients[1];
		result[4] = coefficients[4] * angularCoefficients[2];
		result[5] = coefficients[5] * angularCoefficients[2];
		result[6] = coefficients[6] * angularCoefficients[2];
		result[7] = coefficients[7] * angularCoefficients[2];
		result[8] = coefficients[8] * angularCoefficients[2];
	}

	inline static void NormalizeShCoefficients( XMVECTOR* coefficients, float scale )
	{
		for( int i = 0; i < 9; ++i )
		{
			coefficients[i] = XMVectorScale( coefficients[i], s_normalizationCoefficients[i] * scale );
		}
	}

	inline static void PackCoefficients( XMVECTOR* coefficients, Vector4* packedCoefficients )
	{
		for( int i = 0; i < 3; i++ ) 
		{ 
			packedCoefficients[i].x = -s_packCoefficient1 * XMVectorGetByIndex( coefficients[3], i ); 
			packedCoefficients[i].y = -s_packCoefficient1 * XMVectorGetByIndex( coefficients[1], i ); 
			packedCoefficients[i].z = s_packCoefficient1 * XMVectorGetByIndex( coefficients[2], i ); 
			packedCoefficients[i].w = s_packCoefficient0 * XMVectorGetByIndex( coefficients[0], i ) - 
				s_packCoefficient3 * XMVectorGetByIndex( coefficients[6], i );  
		} 
 
		for( int i = 0; i < 3; i++ ) 
		{ 
			packedCoefficients[i + 3].x = s_packCoefficient2 * XMVectorGetByIndex( coefficients[4], i );
			packedCoefficients[i + 3].y = -s_packCoefficient2 * XMVectorGetByIndex( coefficients[5], i );
			packedCoefficients[i + 3].z = 3.0f * s_packCoefficient3 * XMVectorGetByIndex( coefficients[6], i );
			packedCoefficients[i + 3].w = -s_packCoefficient2 * XMVectorGetByIndex( coefficients[7], i );
		}

		packedCoefficients[6] = Vector4( XMVectorScale( coefficients[8], s_packCoefficient4 ) );
		packedCoefficients[6].w = 1.0f;
	}

	static const size_t ORDER = 3;
private:
	inline static void ComputeCapInt( float sinAngle, float cosAngle, float angularCoefficients[3] )
	{
		angularCoefficients[0] = -cosAngle + 1.f;
		angularCoefficients[1] = sinAngle * sinAngle;
		angularCoefficients[2] = cosAngle * ( ( cosAngle * cosAngle ) - 1.0f );
	}

	inline static void EvalBasis( float x, float y, float z, float coefficients[9] )
	{
		coefficients[0] = 1.f;
		coefficients[1] = y;
		coefficients[2] = z;
		coefficients[3] = x;
		coefficients[4] = x * y + y * x;
		coefficients[5] = z * y;
		coefficients[6] = 3.f * ( z * z ) - 1.f;
		coefficients[7] = z * x;
		coefficients[8] = ( x * x - y * y );
	}

	static const float s_packCoefficient0;
	static const float s_packCoefficient1;
	static const float s_packCoefficient2;
	static const float s_packCoefficient3;
	static const float s_packCoefficient4;

	static const float s_normalizationCoefficients[9];
};

const float ShSolver<Tr2ShLightingManager::L2>::s_packCoefficient0 = 1.0f / ( 2.0f * sqrt( XM_PI ) );
const float ShSolver<Tr2ShLightingManager::L2>::s_packCoefficient1 = sqrt( 3.0f ) / ( 3.0f * sqrt( XM_PI ) );
const float ShSolver<Tr2ShLightingManager::L2>::s_packCoefficient2 = sqrt( 15.0f ) / ( 8.0f * sqrt( XM_PI ) );
const float ShSolver<Tr2ShLightingManager::L2>::s_packCoefficient3 = sqrt( 5.0f ) / ( 16.0f * sqrt( XM_PI ) );
const float ShSolver<Tr2ShLightingManager::L2>::s_packCoefficient4 = 0.5f * s_packCoefficient2;

const float ShSolver<Tr2ShLightingManager::L2>::s_normalizationCoefficients[9] = {
	float( 2.0f * sqrt( XM_PI ) * 0.282094791773878140f * sqrt( 0.3141593E1f ) ),
	float( 2.0f / 3.0f * sqrt( 3.0f * XM_PI ) * -0.488602511902919920f * ( sqrt( 3.0f ) * sqrt( 0.3141593E1f ) / 2.0f ) ),
	float( 2.0f / 3.0f * sqrt( 3.0f * XM_PI ) * 0.488602511902919920f * ( sqrt( 3.0f ) * sqrt( 0.3141593E1f ) / 2.0f ) ),
	float( 2.0f / 3.0f * sqrt( 3.0f * XM_PI ) * -0.488602511902919920f * ( sqrt( 3.0f ) * sqrt( 0.3141593E1f ) / 2.0f ) ),
	float( 2.0f / 5.0f * sqrt( 5.0f * XM_PI ) * 0.546274215296039590f * ( -sqrt( 5.0f ) * sqrt( 0.3141593E1f ) / 2.0f ) ),
	float( 2.0f / 5.0f * sqrt( 5.0f * XM_PI ) * -1.092548430592079200f * ( -sqrt( 5.0f ) * sqrt( 0.3141593E1f ) / 2.0f ) ),
	float( 2.0f / 5.0f * sqrt( 5.0f * XM_PI ) * -0.315391565252520050f * ( -sqrt( 5.0f ) * sqrt( 0.3141593E1f ) / 2.0f ) ),
	float( 2.0f / 5.0f * sqrt( 5.0f * XM_PI ) * -1.092548430592079200f * ( -sqrt( 5.0f ) * sqrt( 0.3141593E1f ) / 2.0f ) ),
	float( 2.0f / 5.0f * sqrt( 5.0f * XM_PI ) * 0.546274215296039590f * ( -sqrt( 5.0f ) * sqrt( 0.3141593E1f ) / 2.0f ) ),
};

}


Tr2ShLightingManager::Tr2ShLightingManager( IRoot* lockobj )
	:m_sources( "Tr2ShLightingManager.m_sources" ),
	m_sourceData( "Tr2ShLightingManager.m_sourceData", sizeof( SourceData ) * 8, 16 ),
	m_sourceCount( 0 ),
	m_primaryIntensity( 1.f ),
	m_secondaryIntensity( 1.f ),
	m_quality( L2 ),
	m_sunColor( 0.f, 0.f, 0.f ),
	m_sunDirection( 0.f, 1.f, 0.f ),
	PARENTLOCK( m_lights )
{
}

Tr2ShLightingManager::~Tr2ShLightingManager()
{
}

// --------------------------------------------------------------------------------------
// Description:
//   Registers a secondary light source with the manager. All pointers passed to this
//   function need to stay alive until UnregisterSecondaryLightSource is called.
// Arguments:
//   position - Pointer to source position vector
//   radius - Pointer to source radius
//   albedo - Pointer to source albedo color
//   emissive - Pointer to source self-emissive color
// --------------------------------------------------------------------------------------
void Tr2ShLightingManager::RegisterSecondaryLightSource( const Vector3* position, const float* radius, const Color* albedo, const Color* emissive )
{
	Source source;
	source.position = position;
	source.radius = radius;
	source.albedo = albedo;
	source.emissive = emissive;
	m_sources.push_back( source );
}

// --------------------------------------------------------------------------------------
// Description:
//   Unregisters a secondary light source previously registered with 
//   RegisterSecondaryLightSource call.
// Arguments:
//   position - Pointer to source position vector
// --------------------------------------------------------------------------------------
void Tr2ShLightingManager::UnregisterSecondaryLightSource( const Vector3* position )
{
	for( auto it = m_sources.begin(); it != m_sources.end(); ++it )
	{
		if( it->position == position )
		{
			m_sources.erase( it );
			return;
		}
	}
}

// --------------------------------------------------------------------------------------
// Description:
//   Updates manager lighting. The primary light source is set to the passed directional
//   light. A user can call GetLighting method after calling this method once per frame.
// Arguments:
//   direction - Primary light direction
//   color - Primary light color/intensity
// --------------------------------------------------------------------------------------
void Tr2ShLightingManager::UpdateWithDirectionalLight( const Vector3& direction, const Vector3& color )
{
	CCP_STATS_ZONE( __FUNCTION__ );

	m_sunColor = color;
	m_sunDirection = Normalize( direction );
	UpdateSourceData();
}

// --------------------------------------------------------------------------------------
// Description:
//   Helper function that calculates SH lighting coefficient for the given sample 
//   position using specified SH order (template parameter).
// Arguments:
//   position - Sample position
//   intensity - Lighting intensity factor
//   lightingCoefficients - (out) SH lighting coefficients (8 for L2 and 4 for L1)
// --------------------------------------------------------------------------------------
template <int Order> 
void Tr2ShLightingManager::CalculateSecondaryLighting( const Vector3& position, float intensity, float cutoffRadius, Vector4* lightingCoefficients )
{
	XMVECTOR lightDirection = m_sunDirection;
	XMVECTOR lightColor = m_sunColor;

	XMVECTOR receiver = position;
	XMVECTOR sh[ShSolver<Order>::ORDER * ShSolver<Order>::ORDER];
	for( int i = 0; i < ShSolver<Order>::ORDER * ShSolver<Order>::ORDER; ++i )
	{
		sh[i] = g_XMZero;
	}

	const SourceData* source = reinterpret_cast<const SourceData*>( m_sourceData.get() );

	for( size_t j = 0; j < m_sourceCount; ++j, ++source )
	{
		if( source->radius < cutoffRadius * source->cutoffMultiplier )
		{
			continue;
		}
		XMVECTOR sourcePos = XMLoadFloat4A( reinterpret_cast<const XMFLOAT4A*>( &source->position ) );
		XMVECTOR toSource = XMVectorSubtract( sourcePos, receiver );
		XMVECTOR distance = XMVector3LengthEst( toSource );
		XMVECTOR oneOverDistance = XMVectorReciprocalEst( distance );
		XMVECTOR dir = XMVectorMultiply( toSource, oneOverDistance );
		
		XMVECTOR condition = XMVectorOrInt( XMVectorOrInt( 
			XMVectorIsInfinite( distance ), 
			XMVectorLess( XMVectorMultiply( XMVectorMultiply( sourcePos, oneOverDistance ), XMLoadFloat4A( reinterpret_cast<const XMFLOAT4A*>( &source->emissive ) ) ), 
				XMVectorReplicate( s_cutoffRadiusRatio ) ) ),
			XMVectorLess( distance, g_XMOne ) );
		if( XMVectorGetIntW( condition ) )
		{
			continue;
		}

		float r[ShSolver<Order>::ORDER * ShSolver<Order>::ORDER];
		ShSolver<Order>::SHEvalSphericalLight( dir, XMVectorGetX( distance ), source->radius, r );

		XMVECTOR dot = XMVectorMultiplyAdd( XMVector3Dot( lightDirection, dir ), g_XMOneHalf, g_XMOneHalf );
		XMVECTOR color = XMLoadFloat4A( reinterpret_cast<const XMFLOAT4A*>( &source->albedo ) );
		color = XMVectorMultiply( dot, color );
		color = XMVectorMultiply( lightColor, color );
		color = XMVectorAdd( color, XMLoadFloat4A( reinterpret_cast<const XMFLOAT4A*>( &source->emissive ) ) );

		for( int i = 0; i < ShSolver<Order>::ORDER * ShSolver<Order>::ORDER; ++i )
		{
			sh[i] = XMVectorMultiplyAdd( XMVectorReplicate( r[i] ), color, sh[i] );
		}
	}

	ShSolver<Order>::NormalizeShCoefficients( sh, intensity );
	ShSolver<Order>::PackCoefficients( sh, lightingCoefficients );
}


// --------------------------------------------------------------------------------------
// Description:
//   Calculates SH lighting coefficient for the given sample position.
// Arguments:
//   position - Sample position
//   intensity - Lighting intensity factor
//   lightingCoefficients - (out) SH lighting coefficients (8 for L2 and 4 for L1)
// --------------------------------------------------------------------------------------
void Tr2ShLightingManager::GetLighting( const Vector3& position, float intensity, float cutoffRadius, Vector4* lightingCoefficients )
{
	CCP_STATS_INC( shLightingSamples );

	if( m_quality == L2 )
	{
		CalculateSecondaryLighting<L2>( position, intensity, cutoffRadius, lightingCoefficients );
	}
	else
	{
		CalculateSecondaryLighting<L1>( position, intensity, cutoffRadius, lightingCoefficients );
	}
}

inline float MaxVectorComponent( const Vector3& v )
{
	return std::max( v.x, std::max( v.y, v.z ) );
}

// --------------------------------------------------------------------------------------
// Description:
//   Updates packed secondary source data.
// --------------------------------------------------------------------------------------
void Tr2ShLightingManager::UpdateSourceData()
{
	CCP_STATS_ZONE( __FUNCTION__ );

	m_sourceCount = 0;
	size_t dataSize = sizeof( SourceData ) * ( ( m_sources.size() + m_lights.size() + 3 ) / 4 ) * 4;
	if( m_sourceData.size() < dataSize )
	{
		m_sourceData.resize( "Tr2ShLightingManager.m_sourceData", dataSize );
	}
	if( m_sourceData.empty() )
	{
		return;
	}
	float maxLight = MaxVectorComponent( m_sunColor );
	SourceData* data = reinterpret_cast<SourceData*>( m_sourceData.get() );
	for( auto it = m_sources.begin(); it != m_sources.end(); ++it )
	{
		if( *it->radius > 0 )
		{
			data->position = *it->position;
			data->radius = *it->radius;
			data->albedo = *reinterpret_cast<const Vector3*>( it->albedo ) * m_secondaryIntensity;
			data->cutoffMultiplier = 1;
			data->emissive = *reinterpret_cast<const Vector3*>( it->emissive ) * m_secondaryIntensity;
			data->maxColorComponent = std::max( MaxVectorComponent( data->albedo ) * maxLight, MaxVectorComponent( data->emissive ) );
			++data;
			++m_sourceCount;
		}
	}
	for( auto it = m_lights.begin(); it != m_lights.end(); ++it )
	{
		Color color;
		( *it )->GetLight( data->position, data->radius, color );
		data->albedo = Vector3( 0, 0, 0 );
		data->cutoffMultiplier = 0;
		data->emissive = *reinterpret_cast<const Vector3*>( &color ) * m_primaryIntensity;
		data->maxColorComponent = MaxVectorComponent( data->emissive );
		++data;
		++m_sourceCount;
	}
	memset( data, 0, dataSize - sizeof( SourceData ) * ( m_sources.size() + m_lights.size() ) );
	CCP_STATS_ADD( shLightingSecondarySources, m_sourceCount );
}
