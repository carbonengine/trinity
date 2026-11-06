// Copyright © 2026 CCP ehf.

#include "StdAfx.h"
#include "Tr2SurfaceCuts.h"


namespace
{
constexpr float s_pi = 3.14159265358979f;

// Facets. Seeds sit one per spacing cube, jittered by s_facetJitter of it; each seed's plane is its surface point's tangent
// tilted by up to s_facetTilt and sunk the facet depth (scaled s_facetDepthMin..1 per seed) inside.
constexpr float s_facetJitter = 0.8f;
constexpr float s_facetTilt = 0.18f;
constexpr float s_facetDepthMin = 0.6f;
// A cut fades from full at s_facetFade of the spacing from its seed to nothing at one spacing, so it cannot flatten a
// concave region, and every plane that can reach a point is among the 27 seed cells around it
constexpr float s_facetFade = 0.6f;
// Seeds whose surface point is further than this * spacing get no plane
constexpr float s_facetSnap = 0.75f;
// A plane only cuts surface facing roughly its own way (full within 25 degrees, none past 40), so a plane seeded beside a
// ledge does not chew the ledge face into patches
const float s_facetAlignFull = std::cos( 25.f * s_pi / 180.f );
const float s_facetAlignNone = std::cos( 40.f * s_pi / 180.f );
// Newton projection of a seed onto the surface
constexpr int s_snapIterations = 8;
constexpr float s_snapTolerance = 0.02f; // |field| at which a projected seed counts as on the surface
constexpr float s_snapMinGradient = 0.2f; // weaker gradients have no reliable tangent plane

// Cleaves. A bite's cap depth is the cleave depth scaled into [s_cleaveDepthMin, 1], its plane leans s_cleaveTiltMin..
// s_cleaveTiltMax off the surface normal at its anchor, and it cuts within s_cleaveReachMin..s_cleaveReachMax of the
// anchor, at most s_cleaveReachRadius of the anchor's distance from the grid centre, so a small shape takes small bites
constexpr uint32_t s_cleaveCountMin = 14;
constexpr uint32_t s_cleaveCountMax = 24;
constexpr float s_cleaveDepthMin = 0.35f;
constexpr float s_cleaveTiltMin = 0.f;
constexpr float s_cleaveTiltMax = 15.f * s_pi / 180.f;
constexpr float s_cleaveReachMin = 2.f;
constexpr float s_cleaveReachMax = 10.f;
constexpr float s_cleaveReachRadius = 0.35f;
// Toward its edge a bite's cut is held under a cone, ( reach - distance ) * s_cleaveSlope, so it fades out no faster
// than the mesh can draw and meets the surface in a crisp kink
constexpr float s_cleaveSlope = 0.6f;
// A bite only cuts surface facing its side, so it does not reach a lobe behind it
constexpr float s_cleaveFacingMin = 0.f;
constexpr uint32_t s_cleaveSalt = 0x5C1EA7;
// Anchor search along a bite's direction: steps out from the grid centre, then bisection on the outermost crossing
constexpr float s_supportStep = 0.5f;
constexpr int s_supportBisect = 12;

// Deterministic 24-bit hash of an integer lattice cell
uint32_t LatticeHash24( int32_t x, int32_t y, int32_t z, uint32_t salt )
{
	uint32_t h = ( uint32_t( x ) * 73856093u ) ^ ( uint32_t( y ) * 19349663u ) ^ ( uint32_t( z ) * 83492791u ) ^ ( salt * 2654435761u );
	h = ( h ^ ( h >> 15 ) ) * 2246822519u;
	h = ( h ^ ( h >> 13 ) ) * 3266489917u;
	return ( h ^ ( h >> 16 ) ) & 0xffffff;
}

// LatticeHash24 scaled to [0, 1)
float Hash01( int32_t x, int32_t y, int32_t z, uint32_t salt )
{
	return LatticeHash24( x, y, z, salt ) / float( 0x1000000 );
}

float SmoothStep( float t )
{
	return t * t * ( 3.f - 2.f * t );
}

// `normal` turned by `angle` toward the direction `spin` radians round it
Vector3 Tilted( const Vector3& normal, float angle, float spin )
{
	const Vector3 helper = std::abs( normal.x ) < 0.9f ? Vector3( 1.f, 0.f, 0.f ) : Vector3( 0.f, 1.f, 0.f );
	const Vector3 u = Normalize( Cross( normal, helper ) );
	const Vector3 v = Cross( normal, u );
	const Vector3 side = u * std::cos( spin ) + v * std::sin( spin );
	return Normalize( normal * std::cos( angle ) + side * std::sin( angle ) );
}
}


Tr2SurfaceCuts::Tr2SurfaceCuts( const Tr2SignedDistanceField& field, const Tr2VoxelData::Dimensions& dimensions, const Tr2SurfaceCutParams& params ) :
	m_field( field ),
	m_params( params ),
	m_center( ( float( dimensions.width ) - 1.f ) * 0.5f, ( float( dimensions.height ) - 1.f ) * 0.5f, ( float( dimensions.depth ) - 1.f ) * 0.5f ),
	m_searchRadius( 0.5f * Length( Vector3( float( dimensions.width ), float( dimensions.height ), float( dimensions.depth ) ) ) ),
	m_seedMin( { 0, 0, 0 } ),
	m_seedCount( { 0, 0, 0 } )
{
	if( m_params.HasFacets() )
	{
		// Seed cells whose 27 cell ring covers every dual vertex, which sit within a cell of the grid
		const Int3 dims = { int32_t( dimensions.width ), int32_t( dimensions.height ), int32_t( dimensions.depth ) };
		for( int axis = 0; axis < 3; ++axis )
		{
			m_seedMin[axis] = int32_t( std::floor( -1.5f / m_params.facetSpacing ) ) - 1;
			const int32_t seedMax = int32_t( std::floor( ( dims[axis] + 0.5f ) / m_params.facetSpacing ) ) + 1;
			m_seedCount[axis] = seedMax - m_seedMin[axis] + 1;
		}
		m_facetStates.resize( size_t( m_seedCount[0] ) * m_seedCount[1] * m_seedCount[2], FacetState::Unbuilt );
		m_facets.resize( m_facetStates.size() );
	}
	if( m_params.HasCleaves() )
	{
		m_cleaves = CreateCleaves();
	}
}

Vector3 Tr2SurfaceCuts::GetCut( const Vector3& point, const Vector3& facing ) const
{
	Vector3 facetNormal;
	Vector3 cleaveNormal;
	const float facetCut = m_params.HasFacets() ? GetFacetCut( point, facing, facetNormal ) : 0.f;
	const float cleaveCut = m_params.HasCleaves() ? GetCleaveCut( point, facing, cleaveNormal ) : 0.f;
	if( facetCut <= 0.f && cleaveCut <= 0.f )
	{
		return Vector3( 0.f, 0.f, 0.f );
	}
	return facetCut >= cleaveCut ? facetNormal * -facetCut : cleaveNormal * -cleaveCut;
}

float Tr2SurfaceCuts::GetFacetCut( const Vector3& point, const Vector3& facing, Vector3& normal ) const
{
	const float spacing = m_params.facetSpacing;
	const Int3 cell = { int32_t( std::floor( point.x / spacing ) ), int32_t( std::floor( point.y / spacing ) ), int32_t( std::floor( point.z / spacing ) ) };
	float best = 0.f;
	for( int32_t dx = -1; dx <= 1; ++dx )
	{
		for( int32_t dy = -1; dy <= 1; ++dy )
		{
			for( int32_t dz = -1; dz <= 1; ++dz )
			{
				const Facet* facet = GetFacet( { cell[0] + dx, cell[1] + dy, cell[2] + dz } );
				if( !facet )
				{
					continue;
				}
				float above = Dot( facet->normal, point ) - facet->offset;
				if( above <= best )
				{
					continue;
				}
				const float reach = Length( point - facet->seed ) / spacing;
				if( reach >= 1.f )
				{
					continue;
				}
				if( reach > s_facetFade )
				{
					above *= 1.f - SmoothStep( ( reach - s_facetFade ) / ( 1.f - s_facetFade ) );
				}
				const float align = Dot( facet->normal, facing );
				if( align <= s_facetAlignNone )
				{
					continue;
				}
				if( align < s_facetAlignFull )
				{
					above *= SmoothStep( ( align - s_facetAlignNone ) / ( s_facetAlignFull - s_facetAlignNone ) );
				}
				if( above > best )
				{
					best = above;
					normal = facet->normal;
				}
			}
		}
	}
	return std::min( best, m_params.facetMaxCut );
}

float Tr2SurfaceCuts::GetCleaveCut( const Vector3& point, const Vector3& facing, Vector3& normal ) const
{
	float best = 0.f;
	for( const auto& cleave : m_cleaves )
	{
		float above = Dot( cleave.normal, point ) - cleave.offset;
		if( above <= best )
		{
			continue;
		}
		const float distance = Length( point - cleave.anchor );
		if( distance >= cleave.reach || Dot( cleave.normal, facing ) <= s_cleaveFacingMin )
		{
			continue;
		}
		above = std::min( above, ( cleave.reach - distance ) * s_cleaveSlope );
		if( above > best )
		{
			best = above;
			normal = cleave.normal;
		}
	}
	return std::min( best, m_params.cleaveDepth );
}

const Tr2SurfaceCuts::Facet* Tr2SurfaceCuts::GetFacet( const Int3& cell ) const
{
	size_t index = 0;
	size_t stride = 1;
	for( int axis = 0; axis < 3; ++axis )
	{
		const int32_t c = cell[axis] - m_seedMin[axis];
		if( c < 0 || c >= m_seedCount[axis] )
		{
			return nullptr;
		}
		index += size_t( c ) * stride;
		stride *= size_t( m_seedCount[axis] );
	}
	if( m_facetStates[index] == FacetState::Unbuilt )
	{
		auto facet = CreateFacet( cell );
		m_facetStates[index] = facet ? FacetState::Built : FacetState::Empty;
		if( facet )
		{
			m_facets[index] = *facet;
		}
	}
	return m_facetStates[index] == FacetState::Built ? &m_facets[index] : nullptr;
}

std::optional<Tr2SurfaceCuts::Facet> Tr2SurfaceCuts::CreateFacet( const Int3& cell ) const
{
	const float spacing = m_params.facetSpacing;
	const uint32_t salt = m_params.seed & 0x7fffffff;
	Vector3 seed;
	for( int axis = 0; axis < 3; ++axis )
	{
		seed[axis] = spacing * ( float( cell[axis] ) + 0.5f + s_facetJitter * ( Hash01( cell[0], cell[1], cell[2], salt + axis ) - 0.5f ) );
	}
	// The field is distance-like (about 1 per cell), so a seed this far from the surface cannot snap to it
	if( std::abs( m_field.Sample( seed ) ) > 1.5f * s_facetSnap * spacing )
	{
		return std::nullopt;
	}

	// Newton steps onto the surface
	Vector3 surface = seed;
	float value = 0.f;
	Vector3 gradient;
	float gradientSq = 0.f;
	for( int i = 0; i < s_snapIterations; ++i )
	{
		value = m_field.Sample( surface );
		gradient = m_field.SampleGradient( surface );
		gradientSq = Dot( gradient, gradient );
		if( gradientSq < 1e-6f )
		{
			return std::nullopt;
		}
		if( std::abs( value ) < s_snapTolerance )
		{
			break;
		}
		surface -= gradient * ( value / gradientSq );
	}
	const float gradientLength = std::sqrt( gradientSq );
	if( std::abs( value ) >= s_snapTolerance || gradientLength < s_snapMinGradient || Length( surface - seed ) > s_facetSnap * spacing )
	{
		return std::nullopt;
	}

	Vector3 normal = gradient / gradientLength;
	for( int axis = 0; axis < 3; ++axis )
	{
		normal[axis] += s_facetTilt * 2.f * ( Hash01( cell[0], cell[1], cell[2], salt + 3 + axis ) - 0.5f );
	}
	normal = Normalize( normal );
	const float depth = m_params.facetDepth * ( s_facetDepthMin + ( 1.f - s_facetDepthMin ) * Hash01( cell[0], cell[1], cell[2], salt + 6 ) );
	return Facet{ seed, normal, Dot( normal, surface ) - depth };
}

std::vector<Tr2SurfaceCuts::Cleave> Tr2SurfaceCuts::CreateCleaves() const
{
	const uint32_t salt = ( m_params.seed & 0x7fffffff ) + s_cleaveSalt;
	auto hash = [&]( uint32_t i, uint32_t offset ) {
		return Hash01( int32_t( i ), 0, 0, salt + offset );
	};
	const uint32_t count = s_cleaveCountMin + uint32_t( hash( 0, 7 ) * float( s_cleaveCountMax - s_cleaveCountMin + 1 ) );
	std::vector<Cleave> cleaves;
	for( uint32_t i = 0; i < count; ++i )
	{
		// Uniform on the sphere
		const float z = 2.f * hash( i, 0 ) - 1.f;
		const float angle = 2.f * s_pi * hash( i, 1 );
		const float r = std::sqrt( std::max( 1.f - z * z, 0.f ) );
		const auto anchor = FindAnchor( Vector3( r * std::cos( angle ), r * std::sin( angle ), z ) );
		if( !anchor )
		{
			continue;
		}
		const auto surfaceNormal = GetSurfaceNormal( *anchor );
		if( !surfaceNormal )
		{
			continue;
		}
		const float tilt = s_cleaveTiltMin + ( s_cleaveTiltMax - s_cleaveTiltMin ) * hash( i, 2 );
		const Vector3 normal = Tilted( *surfaceNormal, tilt, 2.f * s_pi * hash( i, 3 ) );
		const float cap = m_params.cleaveDepth * ( s_cleaveDepthMin + ( 1.f - s_cleaveDepthMin ) * hash( i, 4 ) );
		float reach = s_cleaveReachMin + ( s_cleaveReachMax - s_cleaveReachMin ) * hash( i, 5 );
		reach = std::min( reach, s_cleaveReachRadius * Length( *anchor - m_center ) );
		cleaves.push_back( { *anchor, normal, Dot( normal, *anchor ) - cap, reach } );
	}
	return cleaves;
}

std::optional<Vector3> Tr2SurfaceCuts::FindAnchor( const Vector3& direction ) const
{
	// The outermost solid to empty crossing on the ray from the grid center
	std::optional<float> lastSolid;
	for( float t = 0.f; t <= m_searchRadius; t += s_supportStep )
	{
		if( m_field.Sample( m_center + direction * t ) <= 0.f )
		{
			lastSolid = t;
		}
	}
	if( !lastSolid )
	{
		return std::nullopt;
	}
	float lo = *lastSolid;
	float hi = lo + s_supportStep;
	for( int i = 0; i < s_supportBisect; ++i )
	{
		const float mid = 0.5f * ( lo + hi );
		if( m_field.Sample( m_center + direction * mid ) <= 0.f )
		{
			lo = mid;
		}
		else
		{
			hi = mid;
		}
	}
	return m_center + direction * ( 0.5f * ( lo + hi ) );
}

std::optional<Vector3> Tr2SurfaceCuts::GetSurfaceNormal( const Vector3& point ) const
{
	const Vector3 gradient = m_field.SampleGradient( point );
	const float length = Length( gradient );
	if( length < 1e-6f )
	{
		return std::nullopt;
	}
	return gradient / length;
}
