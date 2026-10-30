// Copyright © 2026 CCP ehf.

#include "StdAfx.h"
#include "Tr2SignedDistanceField.h"


Tr2SignedDistanceField::Tr2SignedDistanceField( const Tr2VoxelData::PayloadSnapshot& sdf, const LinearTransform& transform, const std::atomic<bool>& cancel ) :
	m_dimensions( sdf.m_voxelData->GetDimensions() )
{
	const auto* suppliedValues = reinterpret_cast<const float*>( sdf.m_data );
	size_t size = size_t( m_dimensions.width ) * m_dimensions.height * m_dimensions.depth;
	m_sdf.reserve( size );
	if( sdf.m_type.elementCount != 1 )
	{
		return;
	}

	switch( sdf.m_type.type )
	{
	case Tr2VoxelDataType::Float:
		std::transform(
			suppliedValues,
			suppliedValues + size,
			std::back_inserter( m_sdf ),
			[&]( float value ) {
				return transform.Apply( value );
			} );
		return;
	case Tr2VoxelDataType::Int8:
		if( sdf.m_type.normalized )
		{
			LinearTransform normalizedTransform = { 1.f / 127.5f, 0.f };
			std::transform(
				reinterpret_cast<const int8_t*>( suppliedValues ),
				reinterpret_cast<const int8_t*>( suppliedValues ) + size,
				std::back_inserter( m_sdf ),
				[&]( auto value ) {
					return transform.Apply( normalizedTransform.Apply( float( value ) ) );
				} );
			return;
		}
	default:
		return;
	}
}

Tr2SignedDistanceField::Tr2SignedDistanceField( const Tr2VoxelData::PayloadSnapshot& occupancy, const std::atomic<bool>& cancel ) :
	m_dimensions( occupancy.m_voxelData->GetDimensions() ),
	m_sdf( CreateFieldFromOccupancy( occupancy, cancel ) )
{
}

bool Tr2SignedDistanceField::IsValid() const
{
	return !m_sdf.empty();
}

float Tr2SignedDistanceField::GetValue( size_t index ) const
{
	return m_sdf[index];
}

float Tr2SignedDistanceField::GetValue( int32_t x, int32_t y, int32_t z ) const
{
	Tr2VoxelData::Coord coord = { x, y, z };
	if( !IsInside( m_dimensions, coord ) )
	{
		return s_deepEmpty;
	}
	return m_sdf[LinearIndex( m_dimensions, coord )];
}

bool Tr2SignedDistanceField::IsValidSdfChannelType( const Tr2VoxelData::PayloadType& type )
{
	return type.elementCount == 1 && ( type.type == Tr2VoxelDataType::Float || ( type.type == Tr2VoxelDataType::Int8 && type.normalized ) );
}


Tr2SignedDistanceField::Int3 Tr2SignedDistanceField::GetGridSize() const
{
	return { Tr2VoxelData::CoordType( m_dimensions.width ), Tr2VoxelData::CoordType( m_dimensions.height ), Tr2VoxelData::CoordType( m_dimensions.depth ) };
}

std::vector<float> Tr2SignedDistanceField::ComputeSdf( const Tr2VoxelData::PayloadSnapshot& occupancyChannel, const std::atomic<bool>& cancel ) const
{
	auto occupancy = [&]( size_t i ) {
		return ( occupancyChannel.m_data[i / 8] & ( 1 << ( i % 8 ) ) ) != 0;
	};

	const Int3 dims = GetGridSize();
	const std::array<size_t, 3> strides = { 1, size_t( dims[0] ), size_t( dims[0] ) * dims[1] };
	const size_t count = dims[0] * dims[1] * dims[2];

	// Cells within Chebyshev s_sdfSearchRadius of a face occupancy transition
	std::vector<uint8_t> band( count, 0 );
	for( int32_t z = 0; z < dims[2]; ++z )
	{
		for( int32_t y = 0; y < dims[1]; ++y )
		{
			for( int32_t x = 0; x < dims[0]; ++x )
			{
				const Int3 coord = { x, y, z };
				const size_t i = x + strides[1] * y + strides[2] * z;
				for( int axis = 0; axis < 3; ++axis )
				{
					if( coord[axis] + 1 < dims[axis] && occupancy( i + strides[axis] ) != occupancy( i ) )
					{
						band[i] = 1;
						band[i + strides[axis]] = 1;
					}
				}
			}
		}
	}
	for( int axis = 0; axis < 3; ++axis )
	{
		const int u = s_axisUV[axis][0];
		const int v = s_axisUV[axis][1];
		const auto mask = band;
		for( int32_t j = 0; j < dims[v]; ++j )
		{
			for( int32_t i = 0; i < dims[u]; ++i )
			{
				const size_t base = strides[u] * i + strides[v] * j;
				int32_t last = -( 1 << 30 );
				for( int32_t k = 0; k < dims[axis]; ++k )
				{
					if( mask[base + strides[axis] * k] )
					{
						last = k;
					}
					if( k - last <= s_sdfSearchRadius )
					{
						band[base + strides[axis] * k] = 1;
					}
				}
				last = 1 << 30;
				for( int32_t k = dims[axis] - 1; k >= 0; --k )
				{
					if( mask[base + strides[axis] * k] )
					{
						last = k;
					}
					if( last - k <= s_sdfSearchRadius )
					{
						band[base + strides[axis] * k] = 1;
					}
				}
			}
		}
	}
	if( cancel )
	{
		return {};
	}

	std::vector<uint32_t> cells;
	const auto infinity = std::numeric_limits<float>::infinity();
	std::vector<float> distanceToEmpty( count, infinity );
	std::vector<float> distanceToSolid( count, infinity );
	for( size_t i = 0; i < count; ++i )
	{
		if( band[i] )
		{
			cells.push_back( uint32_t( i ) );
			( occupancy( i ) ? distanceToSolid : distanceToEmpty )[i] = 0.0;
		}
	}

	// Local Euclidean step lengths for the 26 neighbours, split by raster direction
	struct Neighbor
	{
		Int3 offset;
		ptrdiff_t flatOffset;
		float weight;
	};
	std::vector<Neighbor> forward;
	std::vector<Neighbor> backward;
	for( int32_t dz = -1; dz <= 1; ++dz )
	{
		for( int32_t dy = -1; dy <= 1; ++dy )
		{
			for( int32_t dx = -1; dx <= 1; ++dx )
			{
				if( dx || dy || dz )
				{
					const ptrdiff_t flatOffset = dx + ptrdiff_t( strides[1] ) * dy + ptrdiff_t( strides[2] ) * dz;
					( flatOffset < 0 ? forward : backward ).push_back( { { dx, dy, dz }, flatOffset, std::sqrt( float( dx * dx + dy * dy + dz * dz ) ) } );
				}
			}
		}
	}

	auto propagate = [&]( uint32_t i, const std::vector<Neighbor>& neighbors ) {
		const Int3 coord = { int32_t( i % strides[1] ), int32_t( ( i / strides[1] ) % dims[1] ), int32_t( i / strides[2] ) };
		auto bestEmpty = distanceToEmpty[i];
		auto bestSolid = distanceToSolid[i];
		for( const auto& neighbor : neighbors )
		{
			bool inside = true;
			for( int axis = 0; axis < 3; ++axis )
			{
				const int32_t c = coord[axis] + neighbor.offset[axis];
				inside = inside && c >= 0 && c < dims[axis];
			}
			if( !inside )
			{
				continue;
			}
			const size_t j = size_t( ptrdiff_t( i ) + neighbor.flatOffset );
			bestEmpty = std::min( bestEmpty, distanceToEmpty[j] + neighbor.weight );
			bestSolid = std::min( bestSolid, distanceToSolid[j] + neighbor.weight );
		}
		distanceToEmpty[i] = bestEmpty;
		distanceToSolid[i] = bestSolid;
	};
	for( auto i : cells )
	{
		propagate( i, forward );
	}
	if( cancel )
	{
		return {};
	}
	for( auto iter = cells.rbegin(); iter != cells.rend(); ++iter )
	{
		propagate( *iter, backward );
	}

	const auto encodedSolid = -s_sdfClamp;
	const auto encodedEmpty = s_sdfClamp;
	std::vector<float> sdf( count );
	for( size_t i = 0; i < count; ++i )
	{
		if( !band[i] )
		{
			sdf[i] = occupancy( i ) ? encodedSolid : encodedEmpty;
		}
		else if( occupancy( i ) )
		{
			const auto d = distanceToEmpty[i] - float( 0.5 );
			sdf[i] = d >= s_sdfClamp ? encodedSolid : -d;
		}
		else
		{
			const auto d = distanceToSolid[i] - float( 0.5 );
			sdf[i] = d >= s_sdfClamp ? encodedEmpty : d;
		}
	}
	return sdf;
}

// Decoded field at the grid cell centres, low-passed and clamped back to the occupancy sign so no solid cell can
// vanish from the mesh and no empty cell can seal over. Uses the supplied SDF if available, otherwise computes one
// from occupancy.
std::vector<float> Tr2SignedDistanceField::CreateFieldFromOccupancy( const Tr2VoxelData::PayloadSnapshot& occupancyChannel, const std::atomic<bool>& cancel ) const
{
	const Int3 dims = GetGridSize();
	const auto sdf = ComputeSdf( occupancyChannel, cancel );
	if( cancel )
	{
		return {};
	}

	// Decode into a copy padded by one ring of deep empty so every in-grid sample has its full blur stencil
	const Int3 padded = { dims[0] + 2, dims[1] + 2, dims[2] + 2 };
	const std::array<size_t, 3> strides = { 1, size_t( padded[0] ), size_t( padded[0] ) * padded[1] };
	auto paddedIndex = [&]( int32_t x, int32_t y, int32_t z ) {
		return size_t( x + 1 ) + strides[1] * size_t( y + 1 ) + strides[2] * size_t( z + 1 );
	};
	std::vector<float> values( strides[2] * padded[2], s_deepEmpty );
	size_t i = 0;
	for( int32_t z = 0; z < dims[2]; ++z )
	{
		for( int32_t y = 0; y < dims[1]; ++y )
		{
			for( int32_t x = 0; x < dims[0]; ++x, ++i )
			{
				values[paddedIndex( x, y, z )] = sdf[i];
			}
		}
	}

	// Separable Jacobi low-pass per axis. Only interior samples of each line move. The correction form is an exact
	// identity wherever the field is locally linear, so only cell-frequency curvature is damped.
	const float half = 0.5 * s_blurAlpha;
	for( int axis = 0; axis < 3; ++axis )
	{
		const int u = s_axisUV[axis][0];
		const int v = s_axisUV[axis][1];
		const size_t step = strides[axis];
		const auto source = values;
		for( int32_t b = 0; b < padded[v]; ++b )
		{
			for( int32_t a = 0; a < padded[u]; ++a )
			{
				const size_t base = strides[u] * a + strides[v] * b;
				for( int32_t k = 1; k + 1 < padded[axis]; ++k )
				{
					const size_t index = base + step * k;
					const auto value = source[index];
					values[index] = value + half * ( ( source[index - step] - value ) + ( source[index + step] - value ) );
				}
			}
		}
	}

	auto occupancy = [&]( size_t i ) {
		return ( occupancyChannel.m_data[i / 8] & ( 1 << ( i % 8 ) ) ) != 0;
	};

	std::vector<float> field( size_t( dims[0] ) * dims[1] * dims[2] );
	i = 0;
	for( int32_t z = 0; z < dims[2]; ++z )
	{
		for( int32_t y = 0; y < dims[1]; ++y )
		{
			for( int32_t x = 0; x < dims[0]; ++x, ++i )
			{
				auto value = values[paddedIndex( x, y, z )];
				// Clamp the field to the occupancy sign so no solid cell can vanish from the mesh and no empty cell can seal over
				if( occupancy( i ) )
				{
					value = std::min( value, -s_signFloor );
				}
				else
				{
					value = std::max( value, s_signFloor );
				}
				field[i] = value;
			}
		}
	}
	return field;
}
