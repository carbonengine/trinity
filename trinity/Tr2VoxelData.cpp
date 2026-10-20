// Copyright © 2026 CCP ehf.

#include "StdAfx.h"
#include "Tr2VoxelData.h"

namespace
{
const std::array s_acceptedFormats = {
	ImageIO::PIXEL_FORMAT_R32G32B32A32_FLOAT,
	ImageIO::PIXEL_FORMAT_R32G32B32A32_UINT,
	ImageIO::PIXEL_FORMAT_R32G32B32A32_SINT,
	ImageIO::PIXEL_FORMAT_R32G32B32_FLOAT,
	ImageIO::PIXEL_FORMAT_R32G32B32_UINT,
	ImageIO::PIXEL_FORMAT_R32G32B32_SINT,
	ImageIO::PIXEL_FORMAT_R16G16B16A16_FLOAT,
	ImageIO::PIXEL_FORMAT_R16G16B16A16_UNORM,
	ImageIO::PIXEL_FORMAT_R16G16B16A16_UINT,
	ImageIO::PIXEL_FORMAT_R16G16B16A16_SNORM,
	ImageIO::PIXEL_FORMAT_R16G16B16A16_SINT,
	ImageIO::PIXEL_FORMAT_R32G32_FLOAT,
	ImageIO::PIXEL_FORMAT_R32G32_UINT,
	ImageIO::PIXEL_FORMAT_R32G32_SINT,
	ImageIO::PIXEL_FORMAT_R8G8B8A8_UNORM,
	ImageIO::PIXEL_FORMAT_R8G8B8A8_UINT,
	ImageIO::PIXEL_FORMAT_R8G8B8A8_SNORM,
	ImageIO::PIXEL_FORMAT_R8G8B8A8_SINT,
	ImageIO::PIXEL_FORMAT_R16G16_FLOAT,
	ImageIO::PIXEL_FORMAT_R16G16_UNORM,
	ImageIO::PIXEL_FORMAT_R16G16_UINT,
	ImageIO::PIXEL_FORMAT_R16G16_SNORM,
	ImageIO::PIXEL_FORMAT_R16G16_SINT,
	ImageIO::PIXEL_FORMAT_R32_FLOAT,
	ImageIO::PIXEL_FORMAT_R32_UINT,
	ImageIO::PIXEL_FORMAT_R32_SINT,
	ImageIO::PIXEL_FORMAT_R8G8_UNORM,
	ImageIO::PIXEL_FORMAT_R8G8_UINT,
	ImageIO::PIXEL_FORMAT_R8G8_SNORM,
	ImageIO::PIXEL_FORMAT_R8G8_SINT,
	ImageIO::PIXEL_FORMAT_R16_FLOAT,
	ImageIO::PIXEL_FORMAT_R16_UNORM,
	ImageIO::PIXEL_FORMAT_R16_UINT,
	ImageIO::PIXEL_FORMAT_R16_SNORM,
	ImageIO::PIXEL_FORMAT_R16_SINT,
	ImageIO::PIXEL_FORMAT_R8_UNORM,
	ImageIO::PIXEL_FORMAT_R8_UINT,
	ImageIO::PIXEL_FORMAT_R8_SNORM,
	ImageIO::PIXEL_FORMAT_R8_SINT
};
}

Tr2VoxelData::Tr2VoxelData( IRoot* )
{
}

bool Tr2VoxelData::Construct( const Dimensions& dimensions, const std::vector<ImageIO::PixelFormat>& payload )
{
	if( dimensions.width == 0 || dimensions.height == 0 || dimensions.depth == 0 )
	{
		return false;
	}
	for( auto& format : payload )
	{
		if( std::find( begin( s_acceptedFormats ), end( s_acceptedFormats ), format ) == end( s_acceptedFormats ) )
		{
			return false;
		}
	}

	m_dimensions = dimensions;
	auto size = dimensions.width * dimensions.height * dimensions.depth;
	m_occupied.resize( size, false );
	m_payload.clear();
	for( auto& format : payload )
	{
		m_payload.emplace_back( Payload{ format, std::vector<uint8_t>( size * ImageIO::GetBytesPerPixel( format ) ) } );
	}
	return true;
}

bool Tr2VoxelData::IsOccupied( CoordType x, CoordType y, CoordType z ) const
{
	if( !IsWithinBounds( x, y, z ) )
	{
		return false;
	}
	return m_occupied[OccupancyIndex( x, y, z )];
}

bool Tr2VoxelData::IsWithinBounds( CoordType x, CoordType y, CoordType z ) const
{
	return x >= 0 && x < CoordType(m_dimensions.width) && y >= 0 && y < CoordType(m_dimensions.height) && z >= 0 && z < CoordType(m_dimensions.depth);
}


Tr2VoxelData::Dimensions Tr2VoxelData::GetDimensions() const
{
	return m_dimensions;
}

size_t Tr2VoxelData::GetPayloadChannelCount() const
{
	return m_payload.size();
}

size_t Tr2VoxelData::OccupancyIndex( CoordType x, CoordType y, CoordType z ) const
{
	return x + m_dimensions.width * ( y + m_dimensions.height * z );
}

BlueStdResult Tr2VoxelData::__init__( uint32_t width, uint32_t height, uint32_t depth, std::vector<ImageIO::PixelFormat> payload )
{
	if ( !Construct( Dimensions{ width, height, depth }, payload ) )
	{
		return BlueStdResult( BLUE_STD_RESULT_VALUE_ERROR, "Invalid dimensions or payload definition" );
	}
	return {};
}

BluePy Tr2VoxelData::PopulateSparse( PyObject* generator )
{
	auto iter = BluePy( PyObject_GetIter( generator ), true );
	if( !iter )
	{
		PyErr_SetString( PyExc_TypeError, "PopulateSparse the first argument must be a sequence of tuples of the form (x, y, z, p0, p1,...)" );
		return {};
	}
	while( BluePy element = BluePy( PyIter_Next( iter ) ) )
	{
		if( !element || !PyTuple_Check( element ) || PyTuple_Size( element ) != 3 + m_payload.size() )
		{
			PyErr_SetString( PyExc_TypeError, "PopulateSparse the first argument must be a sequence of tuples of the form (x, y, z, p0, p1,...)" );
			return {};
		}
		CoordType x = 0;
		CoordType y = 0;
		CoordType z = 0;
		if( !BlueExtractArgument( PyTuple_GetItem( element, 0 ), x, 0 ) || !BlueExtractArgument( PyTuple_GetItem( element, 1 ), y, 1 ) || !BlueExtractArgument( PyTuple_GetItem( element, 2 ), z, 2 ) )
		{
			PyErr_SetString( PyExc_TypeError, "PopulateSparse the first argument must be a sequence of tuples of the form (x, y, z, p0, p1,...)" );
			return {};
		}
		if( !IsWithinBounds( x, y, z ) )
		{
			PyErr_SetString( PyExc_ValueError, "PopulateSparse coordinates out of bounds" );
			return {};
		}
		m_occupied[OccupancyIndex( x, y, z )] = true;
		if( !ExtractPayload( element, x, y, z ) )
		{
			return {};
		}
	}
	return BluePy( Py_None, true );
}

BluePy Tr2VoxelData::SetVoxels( PyObject* set, PyObject* remove )
{
	auto setIter = BluePy( PyObject_GetIter( set ), true );
	if( !setIter )
	{
		PyErr_SetString( PyExc_TypeError, "SetVoxels the first argument must be a sequence of tuples of the form (x, y, z, p0, p1,...)" );
		return {};
	}
	auto removeIter = BluePy( PyObject_GetIter( remove ), true );
	if( !removeIter )
	{
		PyErr_SetString( PyExc_TypeError, "SetVoxels the second argument must be a sequence of tuples of the form (x, y, z)" );
		return {};
	}
	while( BluePy element = BluePy( PyIter_Next( setIter ) ) )
	{
		if( !element || !PySequence_Check( element ) || PySequence_Size( element ) != 3 + m_payload.size() )
		{
			PyErr_SetString( PyExc_TypeError, "SetVoxels the first argument must be a sequence of tuples of the form (x, y, z, p0, p1,...)" );
			return {};
		}
		CoordType x = 0;
		CoordType y = 0;
		CoordType z = 0;
		if( !BlueExtractArgument( PyTuple_GetItem( element, 0 ), x, 0 ) || !BlueExtractArgument( PyTuple_GetItem( element, 1 ), y, 1 ) || !BlueExtractArgument( PyTuple_GetItem( element, 2 ), z, 2 ) )
		{
			PyErr_SetString( PyExc_TypeError, "SetVoxels the first argument must be a sequence of tuples of the form (x, y, z, p0, p1,...)" );
			return {};
		}
		m_occupied[OccupancyIndex( x, y, z )] = true;
		if( !ExtractPayload( element, x, y, z ) )
		{
			return {};
		}
	}
	while( BluePy element = BluePy( PyIter_Next( removeIter ) ) )
	{
		if( !element || !PySequence_Check( element ) || PySequence_Size( element ) != 3 + m_payload.size() )
		{
			PyErr_SetString( PyExc_TypeError, "SetVoxels the second argument must be a sequence of tuples of the form (x, y, z)" );
			return {};
		}
		CoordType x = 0;
		CoordType y = 0;
		CoordType z = 0;
		if( !BlueExtractArgument( PyTuple_GetItem( element, 0 ), x, 0 ) || !BlueExtractArgument( PyTuple_GetItem( element, 1 ), y, 1 ) || !BlueExtractArgument( PyTuple_GetItem( element, 2 ), z, 2 ) )
		{
			PyErr_SetString( PyExc_TypeError, "SetVoxels the second argument must be a sequence of tuples of the form (x, y, z)" );
			return {};
		}
		ExtractPayload( element, x, y, z );
		m_occupied[OccupancyIndex( x, y, z )] = false;
	}

	return BluePy( Py_None, true );
}

namespace
{
template <typename T, size_t N>
bool ExtractPayloadElement( PyObject* element, size_t index, std::array<T, N>& payload )
{
	auto pp = PyTuple_GetItem( element, 3 + index );
	if( N > 1 && ( !PyTuple_Check( pp ) || PyTuple_Size( pp ) != N ) )
	{
		PyErr_Format( PyExc_TypeError, "Payload must be a tuple of %zu unsigned ints", N );
		return false;
	}
	for( size_t j = 0; j < N; ++j )
	{
		// BlueExtractArgument does not compile for int16_t and int8_t, so we need to extract as int32_t and cast
		if constexpr( std::is_same_v<T, int16_t> || std::is_same_v<T, int8_t> )
		{
			int32_t tmp = 0;
			if( !BlueExtractArgument( N == 1 ? pp : PyTuple_GetItem( pp, j ), tmp, static_cast<unsigned int>( j ) ) )
			{
				PyErr_Format( PyExc_TypeError, "Payload must be a tuple of %zu unsigned ints", N );
				return false;
			}
			payload[j] = static_cast<T>( tmp );
		}
		else
		{
			if( !BlueExtractArgument( N == 1 ? pp : PyTuple_GetItem( pp, j ), payload[j], static_cast<unsigned int>( j ) ) )
			{
				PyErr_Format( PyExc_TypeError, "Payload must be a tuple of %zu unsigned ints", N );
				return false;
			}
		}
	}
	return true;
}

template <typename T>
struct PythonStorageType
{
	using type = T;
};

template <>
struct PythonStorageType<Float_16>
{
	using type = float;
};
}

template <typename Dst, size_t N>
bool Tr2VoxelData::ExtractPayloadElement( PyObject* element, size_t index, CoordType x, CoordType y, CoordType z )
{
	std::array<typename PythonStorageType<Dst>::type, N> p;
	if( !::ExtractPayloadElement( element, index, p ) )
	{
		return false;
	}
	size_t coord = x + m_dimensions.width * ( y + m_dimensions.height * z );
	auto& dest = m_payload[index];
	auto bytesPerPixel = ImageIO::GetBytesPerPixel( dest.format );
	for( size_t j = 0; j < N; ++j )
	{
		reinterpret_cast<Dst*>( dest.data.data() + coord * bytesPerPixel )[j] = static_cast<Dst>( p[j] );
	}
	return true;
}

template <typename Dst, size_t N>
bool Tr2VoxelData::ExtractUniformPayloadElement( PyObject* element, size_t index, CoordType x, CoordType y, CoordType z )
{
	std::array<float, 4> p;
	if( !::ExtractPayloadElement( element, index, p ) )
	{
		return false;
	}
	auto coord = x + m_dimensions.width * ( y + m_dimensions.height * z );
	auto& dest = m_payload[index];
	auto bytesPerPixel = ImageIO::GetBytesPerPixel( dest.format );
	for( size_t j = 0; j < N; ++j )
	{
		auto value = p[j];
		if constexpr( std::is_signed_v<Dst> )
		{
			reinterpret_cast<Dst*>( dest.data.data() + coord * bytesPerPixel )[j] = Dst( std::round( std::clamp( float( value ), -1.0f, 1.0f ) * float( std::numeric_limits<Dst>::max() ) ) );
		}
		else
		{
			reinterpret_cast<Dst*>( dest.data.data() + coord * bytesPerPixel )[j] = Dst( std::round( std::clamp( float( value ), 0.0f, 1.0f ) * float( std::numeric_limits<Dst>::max() ) ) );
		}
	}
	return true;
}

bool Tr2VoxelData::ExtractPayload( PyObject* element, CoordType x, CoordType y, CoordType z )
{
	for( size_t i = 0; i < m_payload.size(); ++i )
	{
		switch( m_payload[i].format )
		{
		case ImageIO::PIXEL_FORMAT_R32G32B32A32_FLOAT:
			if( !ExtractPayloadElement<float, 4>( element, i, x, y, z ) )
			{
				return false;
			}
			break;
		case ImageIO::PIXEL_FORMAT_R32G32B32A32_UINT: 
			if( !ExtractPayloadElement<uint32_t, 4>( element, i, x, y, z ) )
			{
				return false;
			}
			break;
		case ImageIO::PIXEL_FORMAT_R32G32B32A32_SINT:
			if( !ExtractPayloadElement<int32_t, 4>( element, i, x, y, z ) )
			{
				return false;
			}
			break;
		case ImageIO::PIXEL_FORMAT_R32G32B32_FLOAT:
			if( !ExtractPayloadElement<float, 3>( element, i, x, y, z ) )
			{
				return false;
			}
			break;
		case ImageIO::PIXEL_FORMAT_R32G32B32_UINT:
			if( !ExtractPayloadElement<uint32_t, 3>( element, i, x, y, z ) )
			{
				return false;
			}
			break;
		case ImageIO::PIXEL_FORMAT_R32G32B32_SINT:
			if( !ExtractPayloadElement<int32_t, 3>( element, i, x, y, z ) )
			{
				return false;
			}
			break;
		case ImageIO::PIXEL_FORMAT_R16G16B16A16_FLOAT:
			if( !ExtractPayloadElement<Float_16, 4>( element, i, x, y, z ) )
			{
				return false;
			}
			break;
		case ImageIO::PIXEL_FORMAT_R16G16B16A16_UNORM:
			if( !ExtractUniformPayloadElement<uint16_t, 4>( element, i, x, y, z ) )
			{
				return false;
			}
			break;
		case ImageIO::PIXEL_FORMAT_R16G16B16A16_UINT:
			if( !ExtractPayloadElement<uint16_t, 4>( element, i, x, y, z ) )
			{
				return false;
			}
			break;
		case ImageIO::PIXEL_FORMAT_R16G16B16A16_SNORM:
			if( !ExtractUniformPayloadElement<int16_t, 4>( element, i, x, y, z ) )
			{
				return false;
			}
			break;
		case ImageIO::PIXEL_FORMAT_R16G16B16A16_SINT:
			if( !ExtractPayloadElement<int16_t, 4>( element, i, x, y, z ) )
			{
				return false;
			}
			break;
		case ImageIO::PIXEL_FORMAT_R32G32_FLOAT:
			if( !ExtractPayloadElement<float, 2>( element, i, x, y, z ) )
			{
				return false;
			}
			break;
		case ImageIO::PIXEL_FORMAT_R32G32_UINT:
			if( !ExtractPayloadElement<uint32_t, 2>( element, i, x, y, z ) )
			{
				return false;
			}
			break;
		case ImageIO::PIXEL_FORMAT_R32G32_SINT:
			if( !ExtractPayloadElement<int32_t, 2>( element, i, x, y, z ) )
			{
				return false;
			}
			break;
		case ImageIO::PIXEL_FORMAT_R8G8B8A8_UNORM:
			if( !ExtractUniformPayloadElement<uint8_t, 4>( element, i, x, y, z ) )
			{
				return false;
			}
			break;
		case ImageIO::PIXEL_FORMAT_R8G8B8A8_UINT:
			if( !ExtractPayloadElement<uint8_t, 4>( element, i, x, y, z ) )
			{
				return false;
			}
			break;
		case ImageIO::PIXEL_FORMAT_R8G8B8A8_SNORM:
			if( !ExtractUniformPayloadElement<int8_t, 4>( element, i, x, y, z ) )
			{
				return false;
			}
			break;
		case ImageIO::PIXEL_FORMAT_R8G8B8A8_SINT:
			if( !ExtractPayloadElement<int8_t, 4>( element, i, x, y, z ) )
			{
				return false;
			}
			break;
		case ImageIO::PIXEL_FORMAT_R16G16_FLOAT:
			if( !ExtractPayloadElement<Float_16, 2>( element, i, x, y, z ) )
			{
				return false;
			}
			break;
		case ImageIO::PIXEL_FORMAT_R16G16_UNORM:
			if( !ExtractUniformPayloadElement<uint16_t, 2>( element, i, x, y, z ) )
			{
				return false;
			}
			break;
		case ImageIO::PIXEL_FORMAT_R16G16_UINT:
			if( !ExtractPayloadElement<uint16_t, 2>( element, i, x, y, z ) )
			{
				return false;
			}
			break;
		case ImageIO::PIXEL_FORMAT_R16G16_SNORM:
			if( !ExtractUniformPayloadElement<int16_t, 2>( element, i, x, y, z ) )
			{
				return false;
			}
			break;
		case ImageIO::PIXEL_FORMAT_R16G16_SINT:
			if( !ExtractPayloadElement<int16_t, 2>( element, i, x, y, z ) )
			{
				return false;
			}
			break;
		case ImageIO::PIXEL_FORMAT_R32_FLOAT:
			if( !ExtractPayloadElement<float, 1>( element, i, x, y, z ) )
			{
				return false;
			}
			break;
		case ImageIO::PIXEL_FORMAT_R32_UINT:
			if( !ExtractPayloadElement<uint32_t, 1>( element, i, x, y, z ) )
			{
				return false;
			}
			break;
		case ImageIO::PIXEL_FORMAT_R32_SINT:
			if( !ExtractPayloadElement<int32_t, 1>( element, i, x, y, z ) )
			{
				return false;
			}
			break;
		case ImageIO::PIXEL_FORMAT_R8G8_UNORM:
			if( !ExtractUniformPayloadElement<uint8_t, 2>( element, i, x, y, z ) )
			{
				return false;
			}
			break;
		case ImageIO::PIXEL_FORMAT_R8G8_UINT:
			if( !ExtractPayloadElement<uint8_t, 2>( element, i, x, y, z ) )
			{
				return false;
			}
			break;
		case ImageIO::PIXEL_FORMAT_R8G8_SNORM:
			if( !ExtractUniformPayloadElement<int8_t, 2>( element, i, x, y, z ) )
			{
				return false;
			}
			break;
		case ImageIO::PIXEL_FORMAT_R8G8_SINT:
			if( !ExtractPayloadElement<int8_t, 2>( element, i, x, y, z ) )
			{
				return false;
			}
			break;
		case ImageIO::PIXEL_FORMAT_R16_FLOAT:
			if( !ExtractPayloadElement<Float_16, 1>( element, i, x, y, z ) )
			{
				return false;
			}
			break;
		case ImageIO::PIXEL_FORMAT_R16_UNORM:
			if( !ExtractUniformPayloadElement<uint16_t, 1>( element, i, x, y, z ) )
			{
				return false;
			}
			break;
		case ImageIO::PIXEL_FORMAT_R16_UINT:
			if( !ExtractPayloadElement<uint16_t, 1>( element, i, x, y, z ) )
			{
				return false;
			}
			break;
		case ImageIO::PIXEL_FORMAT_R16_SNORM:
			if( !ExtractUniformPayloadElement<int16_t, 1>( element, i, x, y, z ) )
			{
				return false;
			}
			break;
		case ImageIO::PIXEL_FORMAT_R16_SINT:
			if( !ExtractPayloadElement<int16_t, 1>( element, i, x, y, z ) )
			{
				return false;
			}
			break;

		case ImageIO::PIXEL_FORMAT_R8_UNORM:
			if( !ExtractUniformPayloadElement<uint8_t, 1>( element, i, x, y, z ) )
			{
				return false;
			}
			break;
		case ImageIO::PIXEL_FORMAT_R8_UINT:
			if( !ExtractPayloadElement<uint8_t, 1>( element, i, x, y, z ) )
			{
				return false;
			}
			break;
		case ImageIO::PIXEL_FORMAT_R8_SNORM:
			if( !ExtractUniformPayloadElement<int8_t, 1>( element, i, x, y, z ) )
			{
				return false;
			}
			break;
		case ImageIO::PIXEL_FORMAT_R8_SINT:
			if( !ExtractPayloadElement<int8_t, 1>( element, i, x, y, z ) )
			{
				return false;
			}
			break;
		default:
			PyErr_Format( PyExc_TypeError, "Unsupported pixel format for voxel (%u, %u, %u)", x, y, z );
			return false;
		}
	}
	return true;
}