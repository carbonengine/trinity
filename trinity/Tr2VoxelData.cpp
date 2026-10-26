// Copyright © 2026 CCP ehf.

#include "StdAfx.h"
#include "Tr2VoxelData.h"

namespace
{
template <typename T>
bool ExtractPayloadElement( PyObject* element, size_t index, uint8_t elementCount, std::array<T, 4>& payload )
{
	auto pp = PyTuple_GetItem( element, Py_ssize_t( 1 + index ) );
	if( elementCount > 1 && ( !PyTuple_Check( pp ) || PyTuple_Size( pp ) != elementCount ) )
	{
		PyErr_Format( PyExc_TypeError, "Payload must be a tuple of %u unsigned ints", elementCount );
		return false;
	}
	for( size_t j = 0; j < elementCount; ++j )
	{
		// BlueExtractArgument does not compile for int16_t and int8_t, so we need to extract as int32_t and cast
		if constexpr( std::is_same_v<T, int16_t> || std::is_same_v<T, int8_t> )
		{
			int32_t tmp = 0;
			if( !BlueExtractArgument( elementCount == 1 ? pp : PyTuple_GetItem( pp, Py_ssize_t( j ) ), tmp, static_cast<unsigned int>( j ) ) )
			{
				PyErr_Format( PyExc_TypeError, "Payload must be a tuple of %u unsigned ints", elementCount );
				return false;
			}
			payload[j] = static_cast<T>( tmp );
		}
		else
		{
			if( !BlueExtractArgument( elementCount == 1 ? pp : PyTuple_GetItem( pp, Py_ssize_t( j ) ), payload[j], static_cast<unsigned int>( j ) ) )
			{
				PyErr_Format( PyExc_TypeError, "Payload must be a tuple of %u unsigned ints", elementCount );
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


bool Tr2VoxelData::PayloadType::IsValid() const
{
	if( elementCount == 0 || elementCount > 4 )
	{
		return false;
	}
	if( normalized && ( type == Tr2VoxelDataType::Float || type == Tr2VoxelDataType::Float16 || type == Tr2VoxelDataType::Boolean ) )
	{
		return false;
	}
	return true;
}

size_t Tr2VoxelData::PayloadType::GetBytesPerValue() const
{
	switch( type )
	{
	case Tr2VoxelDataType::Boolean:
	case Tr2VoxelDataType::UInt8:
	case Tr2VoxelDataType::Int8:
		return elementCount;
	case Tr2VoxelDataType::UInt16:
	case Tr2VoxelDataType::Int16:
	case Tr2VoxelDataType::Float16:
		return 2 * size_t( elementCount );
	case Tr2VoxelDataType::UInt32:
	case Tr2VoxelDataType::Int32:
	case Tr2VoxelDataType::Float:
		return 4 * size_t( elementCount );
	default:
		return 0;
	}
}

size_t Tr2VoxelData::PayloadType::GetStoreSize( const Dimensions& dimensions ) const
{
	if( type == Tr2VoxelDataType::Boolean )
	{
		return ( dimensions.width * dimensions.height * dimensions.depth * elementCount + 7 ) / 8;
	}
	return dimensions.width * dimensions.height * dimensions.depth * GetBytesPerValue();
}

Tr2VoxelData::Tr2VoxelData( IRoot* )
{
}

BlueStdResult Tr2VoxelData::Construct( const Dimensions& dimensions, std::vector<PayloadType> payload )
{
	if( m_dimensions.width != 0 || m_dimensions.height != 0 || m_dimensions.depth != 0 )
	{
		return BlueStdResult( BLUE_STD_RESULT_VALUE_ERROR, "Voxel data has already been constructed" );
	}
	if( dimensions.width == 0 || dimensions.height == 0 || dimensions.depth == 0 )
	{
		return BlueStdResult( BLUE_STD_RESULT_VALUE_ERROR, "Invalid dimensions" );
	}
	if( payload.empty() )
	{
		return BlueStdResult( BLUE_STD_RESULT_VALUE_ERROR, "Payload cannot be empty" );
	}
	for( auto& format : payload )
	{
		if( !format.IsValid() )
		{
			return BlueStdResult( BLUE_STD_RESULT_VALUE_ERROR, "Invalid payload definition" );
		}
	}

	m_dimensions = dimensions;
	m_payload.clear();
	for( auto& format : payload )
	{
		m_payload.emplace_back( Payload{ format, std::vector<uint8_t>( format.GetStoreSize( dimensions ) ) } );
	}
	return {};
}

bool Tr2VoxelData::IsWithinBounds( const Coord& coord ) const
{
	return coord.x >= 0 && coord.x < CoordType(m_dimensions.width) && coord.y >= 0 && coord.y < CoordType(m_dimensions.height) && coord.z >= 0 && coord.z < CoordType(m_dimensions.depth);
}


Tr2VoxelData::Dimensions Tr2VoxelData::GetDimensions() const
{
	return m_dimensions;
}

size_t Tr2VoxelData::GetPayloadChannelCount() const
{
	return m_payload.size();
}

const Tr2VoxelData::PayloadType& Tr2VoxelData::GetPayloadType( size_t index ) const
{
	return m_payload[index].type;
}

const void* Tr2VoxelData::GetPayloadData( size_t index ) const
{
	return m_payload[index].data.data();
}


BluePy Tr2VoxelData::SetVoxels( PyObject* set )
{
	auto setIter = BluePy( PyObject_GetIter( set ), true );
	if( !setIter )
	{
		PyErr_SetString( PyExc_TypeError, "SetVoxels the first argument must be a sequence of tuples of the form ((x, y, z), p0, p1,...)" );
		return {};
	}
	while( BluePy element = BluePy( PyIter_Next( setIter ) ) )
	{
		if( !element || !PyTuple_Check( element ) || PyTuple_Size( element ) != 1 + m_payload.size() )
		{
			PyErr_SetString( PyExc_TypeError, "SetVoxels the first argument must be a sequence of tuples of the form ((x, y, z), p0, p1,...)" );
			return {};
		}
		Coord coord = {};
		if( !BlueExtractArgument( PyTuple_GET_ITEM( element.o, 0 ), coord, 1 ) )
		{
			PyErr_SetString( PyExc_TypeError, "SetVoxels the first argument must be a sequence of tuples of the form ((x, y, z), p0, p1,...)" );
			return {};
		}
		if( !IsWithinBounds( coord ) )
		{
			PyErr_SetString( PyExc_ValueError, "SetVoxels coordinates out of bounds" );
			return {};
		}
		if( !ExtractPayload( element, coord ) )
		{
			return {};
		}
	}
	return BluePy( Py_None, true );
}

template <typename T>
BluePy Tr2VoxelData::WrapPayloadElement( const Payload& payload, const Coord& coord ) const
{
	std::array<typename PythonStorageType<T>::type, 4> p = {};
	size_t index = coord.x + m_dimensions.width * ( coord.y + m_dimensions.height * coord.z );
	auto bytesPerPixel = payload.type.GetBytesPerValue();
	for( size_t j = 0; j < payload.type.elementCount; ++j )
	{
		if constexpr( std::is_same_v<T, bool> )
		{
			auto byte = reinterpret_cast<const uint8_t*>( payload.data.data() + ( index * payload.type.elementCount + j ) / 8 );
			p[j] = ( *byte & ( 1 << ( ( index * payload.type.elementCount + j ) % 8 ) ) ) != 0;
		}
		else
		{
			p[j] = reinterpret_cast<const T*>( payload.data.data() + index * bytesPerPixel )[j];
		}
	}

	if( payload.type.normalized )
	{
		if( payload.type.elementCount == 1 )
		{
			float x;
			if constexpr( std::is_signed_v<T> )
			{
				x = float( std::max( float( p[0] ) / float( std::numeric_limits<T>::max() ), -1.0f ) );
			}
			else
			{
				x = float( p[0] ) / float( std::numeric_limits<T>::max() );
			}
			return BluePy( BlueWrapReturnValue( {}, x ) );
		}
		PyObject* tuple = PyTuple_New( payload.type.elementCount );
		for( size_t j = 0; j < payload.type.elementCount; ++j )
		{
			float x;
			if constexpr( std::is_signed_v<T> )
			{
				x = float( std::max( float( p[j] ) / float( std::numeric_limits<T>::max() ), -1.0f ) );
			}
			else
			{
				x = float( p[j] ) / float( std::numeric_limits<T>::max() );
			}
			PyTuple_SetItem( tuple, Py_ssize_t( j ), BlueWrapReturnValue( {}, x ) );
		}
		return BluePy( tuple );
	}
	if( payload.type.elementCount == 1 )
	{
		return BluePy( BlueWrapReturnValue( {}, p[0] ) );
	}
	PyObject* tuple = PyTuple_New( payload.type.elementCount );
	for( size_t j = 0; j < payload.type.elementCount; ++j )
	{
		PyTuple_SetItem( tuple, j, BlueWrapReturnValue( {}, p[j] ) );
	}
	return BluePy( tuple );
}


BluePy Tr2VoxelData::PyGetPayloadData( size_t index, const Coord& coord )
{
	if( index >= m_payload.size() )
	{
		PyErr_SetString( PyExc_IndexError, "Payload index out of range" );
		return {};
	}
	if( !IsWithinBounds( coord ) )
	{
		PyErr_SetString( PyExc_ValueError, "Coordinates out of bounds" );
		return {};
	}
	
	const auto& payload = m_payload[index];
	switch( payload.type.type )
	{
	case Tr2VoxelDataType::Boolean:
		return WrapPayloadElement<bool>( payload, coord );
	case Tr2VoxelDataType::Float:
		return WrapPayloadElement<float>( payload, coord );
	case Tr2VoxelDataType::Float16:
		return WrapPayloadElement<Float_16>( payload, coord );
	case Tr2VoxelDataType::UInt8:
		return WrapPayloadElement<uint8_t>( payload, coord );
	case Tr2VoxelDataType::UInt16:
		return WrapPayloadElement<uint16_t>( payload, coord );
	case Tr2VoxelDataType::UInt32:
		return WrapPayloadElement<uint32_t>( payload, coord );
	case Tr2VoxelDataType::Int8:
		return WrapPayloadElement<int8_t>( payload, coord );
	case Tr2VoxelDataType::Int16:
		return WrapPayloadElement<int16_t>( payload, coord );
	case Tr2VoxelDataType::Int32:
		return WrapPayloadElement<int32_t>( payload, coord );
	default:
		return {};
	}
}

template <typename Dst>
bool Tr2VoxelData::ExtractPayloadElement( PyObject* element, size_t index, const PayloadType& payloadType, const Coord& coord )
{
	if( payloadType.normalized )
	{
		return ExtractNormalizedPayloadElement<Dst>( element, index, payloadType.elementCount, coord );
	}
	std::array<typename PythonStorageType<Dst>::type, 4> p;
	if( !::ExtractPayloadElement( element, index, payloadType.elementCount, p ) )
	{
		return false;
	}
	size_t coordIndex = coord.x + m_dimensions.width * ( coord.y + m_dimensions.height * coord.z );
	auto& dest = m_payload[index];
	auto bytesPerPixel = dest.type.GetBytesPerValue();
	for( size_t j = 0; j < payloadType.elementCount; ++j )
	{
		if constexpr( std::is_same_v<Dst, bool> )
		{
			auto byte = reinterpret_cast<uint8_t*>( dest.data.data() + ( coordIndex * payloadType.elementCount + j ) / 8 );
			if( p[j] )
			{
				*byte |= 1 << ( ( coordIndex * payloadType.elementCount + j ) % 8 );
			}
			else
			{
				*byte &= ~( 1 << ( ( coordIndex * payloadType.elementCount + j ) % 8 ) );
			}
		}
		else
		{
			reinterpret_cast<Dst*>( dest.data.data() + coordIndex * bytesPerPixel )[j] = static_cast<Dst>( p[j] );
		}
	}
	return true;
}

template <typename Dst>
bool Tr2VoxelData::ExtractNormalizedPayloadElement( PyObject* element, size_t index, uint8_t elementCount, const Coord& coord )
{
	std::array<float, 4> p;
	if( !::ExtractPayloadElement( element, index, elementCount, p ) )
	{
		return false;
	}
	auto coordIndex = coord.x + m_dimensions.width * ( coord.y + m_dimensions.height * coord.z );
	auto& dest = m_payload[index];
	auto bytesPerPixel = dest.type.GetBytesPerValue();
	for( size_t j = 0; j < elementCount; ++j )
	{
		auto value = p[j];
		if constexpr( std::is_signed_v<Dst> )
		{
			reinterpret_cast<Dst*>( dest.data.data() + coordIndex * bytesPerPixel )[j] = Dst( std::round( std::clamp( float( value ), -1.0f, 1.0f ) * float( std::numeric_limits<Dst>::max() ) ) );
		}
		else
		{
			reinterpret_cast<Dst*>( dest.data.data() + coordIndex * bytesPerPixel )[j] = Dst( std::round( std::clamp( float( value ), 0.0f, 1.0f ) * float( std::numeric_limits<Dst>::max() ) ) );
		}
	}
	return true;
}

bool Tr2VoxelData::ExtractPayload( PyObject* element, const Coord& coord )
{
	for( size_t i = 0; i < m_payload.size(); ++i )
	{
		switch( m_payload[i].type.type )
		{
		case Tr2VoxelDataType::Boolean:
			if( !ExtractPayloadElement<bool>( element, i, m_payload[i].type, coord ) )
			{
				return false;
			}
			break;
		case Tr2VoxelDataType::Float:
			if( !ExtractPayloadElement<float>( element, i, m_payload[i].type, coord ) )
			{
				return false;
			}
			break;
		case Tr2VoxelDataType::Float16:
			if( !ExtractPayloadElement<Float_16>( element, i, m_payload[i].type, coord ) )
			{
				return false;
			}
			break;
		case Tr2VoxelDataType::UInt8:
			if( !ExtractPayloadElement<uint8_t>( element, i, m_payload[i].type, coord ) )
			{
				return false;
			}
			break;
		case Tr2VoxelDataType::UInt16:
			if( !ExtractPayloadElement<uint16_t>( element, i, m_payload[i].type, coord ) )
			{
				return false;
			}
			break;
		case Tr2VoxelDataType::UInt32: 
			if( !ExtractPayloadElement<uint32_t>( element, i, m_payload[i].type, coord ) )
			{
				return false;
			}
			break;
		case Tr2VoxelDataType::Int8:
			if( !ExtractPayloadElement<int8_t>( element, i, m_payload[i].type, coord ) )
			{
				return false;
			}
			break;
		case Tr2VoxelDataType::Int16:
			if( !ExtractPayloadElement<int16_t>( element, i, m_payload[i].type, coord ) )
			{
				return false;
			}
			break;
		case Tr2VoxelDataType::Int32:
			if( !ExtractPayloadElement<int32_t>( element, i, m_payload[i].type, coord ) )
			{
				return false;
			}
			break;
		default:
			return false;
		}
	}
	return true;
}

bool BlueExtractArgument( BlueScriptValue argument, Tr2VoxelData::Dimensions& resultRef, unsigned int argID )
{
	if( !PyTuple_Check( argument ) )
	{
		PyErr_Format( PyExc_TypeError, "Could not match argument %i to expected type: tuple expected.", argID );
		return false;
	}
	if( PyTuple_Size( argument ) != 3 )
	{
		PyErr_Format( PyExc_TypeError, "Could not match argument %i to expected type: tuple with three elements expected.", argID );
		return false;
	}

	uint32_t x;
	if( !BlueExtractArgument( PyTuple_GET_ITEM( argument, 0 ), x, 0 ) )
	{
		return false;
	}
	uint32_t y;
	if( !BlueExtractArgument( PyTuple_GET_ITEM( argument, 1 ), y, 1 ) )
	{
		return false;
	}
	uint32_t z;
	if( !BlueExtractArgument( PyTuple_GET_ITEM( argument, 2 ), z, 2 ) )
	{
		return false;
	}

	resultRef = Tr2VoxelData::Dimensions{ x, y, z };
	return true;
}

bool BlueExtractArgument( BlueScriptValue argument, Tr2VoxelData::Coord& resultRef, unsigned int argID )
{
	if( !PyTuple_Check( argument ) )
	{
		PyErr_Format( PyExc_TypeError, "Could not match argument %i to expected type: tuple expected.", argID );
		return false;
	}
	if( PyTuple_Size( argument ) != 3 )
	{
		PyErr_Format( PyExc_TypeError, "Could not match argument %i to expected type: tuple with three elements expected.", argID );
		return false;
	}

	Tr2VoxelData::CoordType x;
	if( !BlueExtractArgument( PyTuple_GET_ITEM( argument, 0 ), x, 0 ) )
	{
		return false;
	}
	Tr2VoxelData::CoordType y;
	if( !BlueExtractArgument( PyTuple_GET_ITEM( argument, 1 ), y, 1 ) )
	{
		return false;
	}
	Tr2VoxelData::CoordType z;
	if( !BlueExtractArgument( PyTuple_GET_ITEM( argument, 2 ), z, 2 ) )
	{
		return false;
	}

	resultRef = Tr2VoxelData::Coord{ x, y, z };
	return true;
}

bool BlueExtractArgument( BlueScriptValue argument, Tr2VoxelData::PayloadType& resultRef, unsigned int argID )
{
	if( !argument || !PyDict_Check( argument ) )
	{
		PyErr_Format( PyExc_TypeError, "Could not match argument %i to expected type: payload definition expected.", argID );
		return false;
	}
	Tr2VoxelData::PayloadType pt;
	if( !BlueExtractArgument( BluePy( PyDict_GetItemString( argument, "type" ) ), pt.type, 0 ) ||
		!BlueExtractArgument( BluePy( PyDict_GetItemString( argument, "elementCount" ) ), pt.elementCount, 1 ) ||
		!BlueExtractArgument( BluePy( PyDict_GetItemString( argument, "normalized" ) ), pt.normalized, 2 ) )
	{
		PyErr_Format( PyExc_TypeError, "Could not match argument %i to expected type: payload definition expected.", argID );
		return false;
	}
	resultRef = pt;
	return true;
}
