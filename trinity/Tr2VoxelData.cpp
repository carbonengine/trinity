// Copyright © 2026 CCP ehf.

#include "StdAfx.h"
#include "Tr2VoxelData.h"
#include <numeric>


namespace
{

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

/**
 * @brief Extracts a single payload element from a Python object. The payload is either a scalar (if elementCount is 1) or a tuple of scalars.
 * @tparam T Destination type of the payload element.
 * @param pp The Python object containing a payload element representation.
 * @param elementCount The number of elements in the payload.
 * @param payload The array to store the extracted payload elements.
 * @return True if the extraction was successful, false otherwise.
 */
template <typename T>
bool ExtractPayloadElement( PyObject* pp, uint8_t elementCount, std::array<T, 4>& payload )
{
	if( elementCount > 1 && ( !PyTuple_Check( pp ) || PyTuple_Size( pp ) != elementCount ) )
	{
		PyErr_Format( PyExc_TypeError, "Payload must be a %u-tuple", elementCount );
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
float NormalizedValue( T value )
{
	if constexpr( std::is_signed_v<T> )
	{
		return float( std::max( float( value ) / float( std::numeric_limits<T>::max() ), -1.0f ) );
	}
	else
	{
		return float( value ) / float( std::numeric_limits<T>::max() );
	}
}

template <typename T>
T DenormalizedValue( float value )
{
	if constexpr( std::is_signed_v<T> )
	{
		return T( std::round( std::clamp( float( value ), -1.0f, 1.0f ) * float( std::numeric_limits<T>::max() ) ) );
	}
	else
	{
		return T( std::round( std::clamp( float( value ), 0.0f, 1.0f ) * float( std::numeric_limits<T>::max() ) ) );
	}
}

/**
 * @brief Converts a payload element from the internal storage format to a Python object. The payload is either a scalar (if elementCount is 1) or a tuple of scalars.
 * @tparam T Underlying storage type of the payload element.
 * @param dimensions The dimensions of the voxel data.
 * @param payloadType The type of the payload.
 * @param payloadData The raw payload data.
 * @param coord The coordinates of the voxel.
 * @return A Python object representing the payload element.
 */
template <typename T>
BluePy WrapPayloadElement( const Tr2VoxelData::Dimensions& dimensions, const Tr2VoxelData::PayloadType& payloadType, const uint8_t* payloadData, const Tr2VoxelData::Coord& coord )
{
	std::array<typename PythonStorageType<T>::type, 4> p = {};
	size_t index = coord.x + dimensions.width * ( coord.y + dimensions.height * coord.z );
	auto bytesPerPixel = payloadType.GetBytesPerValue();
	for( size_t j = 0; j < payloadType.elementCount; ++j )
	{
		if constexpr( std::is_same_v<T, bool> )
		{
			auto byte = reinterpret_cast<const uint8_t*>( payloadData + ( index * payloadType.elementCount + j ) / 8 );
			p[j] = ( *byte & ( 1 << ( ( index * payloadType.elementCount + j ) % 8 ) ) ) != 0;
		}
		else
		{
			p[j] = reinterpret_cast<const T*>( payloadData + index * bytesPerPixel )[j];
		}
	}

	if( payloadType.normalized )
	{
		if( payloadType.elementCount == 1 )
		{
			// 1-element payloads are returned as scalars, not tuples
			return BluePy( BlueWrapReturnValue( {}, NormalizedValue( p[0] ) ) );
		}
		PyObject* tuple = PyTuple_New( payloadType.elementCount );
		for( size_t j = 0; j < payloadType.elementCount; ++j )
		{
			PyTuple_SetItem( tuple, Py_ssize_t( j ), BlueWrapReturnValue( {}, NormalizedValue( p[j] ) ) );
		}
		return BluePy( tuple );
	}
	if( payloadType.elementCount == 1 )
	{
		// 1-element payloads are returned as scalars, not tuples
		return BluePy( BlueWrapReturnValue( {}, p[0] ) );
	}
	PyObject* tuple = PyTuple_New( payloadType.elementCount );
	for( size_t j = 0; j < payloadType.elementCount; ++j )
	{
		PyTuple_SetItem( tuple, j, BlueWrapReturnValue( {}, p[j] ) );
	}
	return BluePy( tuple );
}

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


Tr2VoxelData::PayloadSnapshot::PayloadSnapshot( PayloadSnapshot&& other ) noexcept :
	m_voxelData( other.m_voxelData ),
	m_type( other.m_type ),
	m_channel( other.m_channel ),
	m_data( other.m_data )
{
	other.m_voxelData = nullptr;
	other.m_type = {};
	other.m_channel = 0;
	other.m_data = nullptr;
}

Tr2VoxelData::PayloadSnapshot::~PayloadSnapshot()
{
	if( m_voxelData )
	{
		std::lock_guard guard( m_voxelData->m_payloadMutex );

		auto& payload = m_voxelData->m_payload[m_channel];
		if( payload.current.data.data() == m_data )
		{
			CCP_ASSERT( payload.current.readerCount > 0 );
			--payload.current.readerCount;
			return;
		}
		auto found = std::find_if( payload.old.begin(), payload.old.end(), [this]( const Snapshot& s ) { return s.data.data() == m_data; } );
		if( found != payload.old.end() )
		{
			CCP_ASSERT( found->readerCount > 0 );
			--found->readerCount;
			if( found->readerCount == 0 )
			{
				payload.old.erase( found );
			}
			return;
		}
		CCP_ASSERT_M( false, "PayloadSnapshot destructor called with invalid data pointer" );
	}
}

Tr2VoxelData::PayloadSnapshot& Tr2VoxelData::PayloadSnapshot::operator=( PayloadSnapshot&& other ) noexcept
{
	if( this != &other )
	{
		m_voxelData = other.m_voxelData;
		m_type = other.m_type;
		m_channel = other.m_channel;
		m_data = other.m_data;

		other.m_voxelData = nullptr;
		other.m_type = {};
		other.m_channel = 0;
		other.m_data = nullptr;
	}
	return *this;
}


bool Tr2VoxelData::ModifiedChannels::IsChannelModified( size_t channelIndex ) const
{
	return ( m_modifiedChannelsBitmask & ( 1ull << channelIndex ) ) != 0;
}

void Tr2VoxelData::ModifiedChannels::MarkChannelModified( size_t channelIndex )
{
	m_modifiedChannelsBitmask |= ( 1ull << channelIndex );
}



Tr2VoxelData::Tr2VoxelData( IRoot* )
{
}

BlueStdResult Tr2VoxelData::Create( const Dimensions& dimensions, std::vector<PayloadType> payload )
{
	if( m_dimensions.width != 0 || m_dimensions.height != 0 || m_dimensions.depth != 0 )
	{
		return BlueStdResult( BLUE_STD_RESULT_VALUE_ERROR, "Voxel data has already been created" );
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
		m_payload.emplace_back( Payload{ format, { std::vector<uint8_t>( format.GetStoreSize( dimensions ) ) } } );
	}
	return {};
}

bool Tr2VoxelData::IsWithinBounds( const Coord& coord ) const
{
	return coord.x >= 0 && coord.x < CoordType( m_dimensions.width ) && coord.y >= 0 && coord.y < CoordType( m_dimensions.height ) && coord.z >= 0 && coord.z < CoordType( m_dimensions.depth );
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


Tr2VoxelData::PayloadSnapshot Tr2VoxelData::GetPayloadSnapshot( size_t index )
{
	std::lock_guard guard( m_payloadMutex );
	PayloadSnapshot snapshot;
	if( index < m_payload.size() )
	{
		snapshot.m_voxelData = this;
		snapshot.m_type = m_payload[index].type;
		snapshot.m_channel = index;
		snapshot.m_data = m_payload[index].current.data.data();
		++m_payload[index].current.readerCount;
	}
	return snapshot;
}


BluePy Tr2VoxelData::SetVoxels( PyObject* set )
{
	auto setIter = BluePy( PyObject_GetIter( set ), true );
	if( !setIter )
	{
		PyErr_SetString( PyExc_TypeError, "SetVoxels the first argument must be a sequence of tuples of the form ((x, y, z), p0, p1,...)" );
		return {};
	}

	{
		std::lock_guard guard( m_payloadMutex );

		for( auto& payload : m_payload )
		{
			if( payload.current.readerCount )
			{
				auto& snapshot = payload.old.emplace_back();
				snapshot.readerCount = payload.current.readerCount;
				std::swap( snapshot.data, payload.current.data );
				payload.current.data.insert( payload.current.data.begin(), snapshot.data.begin(), snapshot.data.end() );
				payload.current.readerCount = 0;
			}
		}
	}

	ModifiedChannels modifiedChannels;

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
		if( !ExtractPayload( element, coord, modifiedChannels ) )
		{
			return {};
		}
	}

	m_onDataChanged.Broadcast( modifiedChannels );

	return BluePy( Py_None, true );
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
		return WrapPayloadElement<bool>( m_dimensions, payload.type, payload.current.data.data(), coord );
	case Tr2VoxelDataType::Float:
		return WrapPayloadElement<float>( m_dimensions, payload.type, payload.current.data.data(), coord );
	case Tr2VoxelDataType::Float16:
		return WrapPayloadElement<Float_16>( m_dimensions, payload.type, payload.current.data.data(), coord );
	case Tr2VoxelDataType::UInt8:
		return WrapPayloadElement<uint8_t>( m_dimensions, payload.type, payload.current.data.data(), coord );
	case Tr2VoxelDataType::UInt16:
		return WrapPayloadElement<uint16_t>( m_dimensions, payload.type, payload.current.data.data(), coord );
	case Tr2VoxelDataType::UInt32:
		return WrapPayloadElement<uint32_t>( m_dimensions, payload.type, payload.current.data.data(), coord );
	case Tr2VoxelDataType::Int8:
		return WrapPayloadElement<int8_t>( m_dimensions, payload.type, payload.current.data.data(), coord );
	case Tr2VoxelDataType::Int16:
		return WrapPayloadElement<int16_t>( m_dimensions, payload.type, payload.current.data.data(), coord );
	case Tr2VoxelDataType::Int32:
		return WrapPayloadElement<int32_t>( m_dimensions, payload.type, payload.current.data.data(), coord );
	default:
		return {};
	}
}

template <typename Dst>
bool Tr2VoxelData::ExtractPayloadElement( PyObject* element, size_t index, const PayloadType& payloadType, const Coord& coord, ModifiedChannels& modifiedChannels )
{
	if( payloadType.normalized )
	{
		return ExtractNormalizedPayloadElement<Dst>( element, index, payloadType.elementCount, coord, modifiedChannels );
	}
	std::array<typename PythonStorageType<Dst>::type, 4> p;
	// The 1+ here is because the first element of the tuple is the coordinate, and the payloads start at index 1
	if( !::ExtractPayloadElement( PyTuple_GetItem( element, Py_ssize_t( 1 + index ) ), payloadType.elementCount, p ) )
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
			auto& byte = *reinterpret_cast<uint8_t*>( dest.current.data.data() + ( coordIndex * payloadType.elementCount + j ) / 8 );
			auto oldByte = byte;
			if( p[j] )
			{
				byte |= 1 << ( ( coordIndex * payloadType.elementCount + j ) % 8 );
			}
			else
			{
				byte &= ~( 1 << ( ( coordIndex * payloadType.elementCount + j ) % 8 ) );
			}
			if( oldByte != byte )
			{
				modifiedChannels.MarkChannelModified( index );
			}
		}
		else
		{
			auto& destElement = reinterpret_cast<Dst*>( dest.current.data.data() + coordIndex * bytesPerPixel )[j];
			if( destElement != static_cast<Dst>( p[j] ) )
			{
				modifiedChannels.MarkChannelModified( index );
			}
			destElement = static_cast<Dst>( p[j] );
		}
	}
	return true;
}

template <typename Dst>
bool Tr2VoxelData::ExtractNormalizedPayloadElement( PyObject* element, size_t index, uint8_t elementCount, const Coord& coord, ModifiedChannels& modifiedChannels )
{
	std::array<float, 4> p;
	// The 1+ here is because the first element of the tuple is the coordinate, and the payloads start at index 1
	if( !::ExtractPayloadElement( PyTuple_GetItem( element, Py_ssize_t( 1 + index ) ), elementCount, p ) )
	{
		return false;
	}
	auto coordIndex = coord.x + m_dimensions.width * ( coord.y + m_dimensions.height * coord.z );
	auto& dest = m_payload[index];
	auto bytesPerPixel = dest.type.GetBytesPerValue();
	for( size_t j = 0; j < elementCount; ++j )
	{
		auto newValue = DenormalizedValue<Dst>( p[j] );
		auto& dstElement = reinterpret_cast<Dst*>( dest.current.data.data() + coordIndex * bytesPerPixel )[j];
		if( dstElement != newValue )
		{
			modifiedChannels.MarkChannelModified( index );
		}
		dstElement = newValue;
	}
	return true;
}

bool Tr2VoxelData::ExtractPayload( PyObject* element, const Coord& coord, ModifiedChannels& modifiedChannels )
{
	for( size_t i = 0; i < m_payload.size(); ++i )
	{
		switch( m_payload[i].type.type )
		{
		case Tr2VoxelDataType::Boolean:
			if( !ExtractPayloadElement<bool>( element, i, m_payload[i].type, coord, modifiedChannels ) )
			{
				return false;
			}
			break;
		case Tr2VoxelDataType::Float:
			if( !ExtractPayloadElement<float>( element, i, m_payload[i].type, coord, modifiedChannels ) )
			{
				return false;
			}
			break;
		case Tr2VoxelDataType::Float16:
			if( !ExtractPayloadElement<Float_16>( element, i, m_payload[i].type, coord, modifiedChannels ) )
			{
				return false;
			}
			break;
		case Tr2VoxelDataType::UInt8:
			if( !ExtractPayloadElement<uint8_t>( element, i, m_payload[i].type, coord, modifiedChannels ) )
			{
				return false;
			}
			break;
		case Tr2VoxelDataType::UInt16:
			if( !ExtractPayloadElement<uint16_t>( element, i, m_payload[i].type, coord, modifiedChannels ) )
			{
				return false;
			}
			break;
		case Tr2VoxelDataType::UInt32:
			if( !ExtractPayloadElement<uint32_t>( element, i, m_payload[i].type, coord, modifiedChannels ) )
			{
				return false;
			}
			break;
		case Tr2VoxelDataType::Int8:
			if( !ExtractPayloadElement<int8_t>( element, i, m_payload[i].type, coord, modifiedChannels ) )
			{
				return false;
			}
			break;
		case Tr2VoxelDataType::Int16:
			if( !ExtractPayloadElement<int16_t>( element, i, m_payload[i].type, coord, modifiedChannels ) )
			{
				return false;
			}
			break;
		case Tr2VoxelDataType::Int32:
			if( !ExtractPayloadElement<int32_t>( element, i, m_payload[i].type, coord, modifiedChannels ) )
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

void Tr2VoxelData::GetWriteBufferAndSize( const char*, unsigned char** buffer, size_t* bufferSize )
{
	*bufferSize = std::accumulate( m_payload.begin(), m_payload.end(), sizeof( PayloadType ) * m_payload.size(), []( size_t sum, const Payload& p ) { return sum + p.current.data.size(); } );
	uint8_t* buf = new uint8_t[*bufferSize];
	if( !buf )
	{
		*bufferSize = 0;
		return;
	}
	*buffer = buf;
	for( const auto& p : m_payload )
	{
		new( buf ) PayloadType( p.type );
		buf += sizeof( PayloadType );
		memcpy( buf, p.current.data.data(), p.current.data.size() );
		buf += p.current.data.size();
	}
}

void Tr2VoxelData::ReleaseWriteBuffer( unsigned char* buffer )
{
	delete[] buffer;
}

unsigned char* Tr2VoxelData::AllocateReadBuffer( const char*, size_t bufferSize )
{
	return new uint8_t[bufferSize];
}

void Tr2VoxelData::SetBufferAndSize( const char*, unsigned char* buffer, size_t bufferSize )
{
	auto initialBuffer = buffer;

	while( true )
	{
		if( bufferSize < sizeof( PayloadType ) )
		{
			break;
		}
		PayloadType payloadType = *reinterpret_cast<PayloadType*>( buffer );
		buffer += sizeof( PayloadType );
		bufferSize -= sizeof( PayloadType );
		size_t payloadSize = payloadType.GetStoreSize( m_dimensions );
		if( bufferSize < payloadSize )
		{
			break;
		}
		m_payload.emplace_back( Payload{ payloadType, { std::vector<uint8_t>( buffer, buffer + payloadSize ) } } );
		buffer += payloadSize;
		bufferSize -= payloadSize;
	}
	delete[] initialBuffer;
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

BlueStdResult Tr2VoxelDataTestAccessor::Create( Tr2VoxelData* voxelData, size_t channelIndex )
{
	m_snapshot = voxelData->GetPayloadSnapshot( channelIndex );
	if( m_snapshot.m_data == nullptr )
	{
		return BlueStdResult( BLUE_STD_RESULT_VALUE_ERROR, "Invalid channel index" );
	}
	return {};
}

BluePy Tr2VoxelDataTestAccessor::PyGetPayloadData( const Tr2VoxelData::Coord& coord ) const
{
	if( !m_snapshot.m_voxelData->IsWithinBounds( coord ) )
	{
		PyErr_SetString( PyExc_ValueError, "Coordinates out of bounds" );
		return {};
	}

	switch( m_snapshot.m_type.type )
	{
	case Tr2VoxelDataType::Boolean:
		return WrapPayloadElement<bool>( m_snapshot.m_voxelData->GetDimensions(), m_snapshot.m_type, m_snapshot.m_data, coord );
	case Tr2VoxelDataType::Float:
		return WrapPayloadElement<float>( m_snapshot.m_voxelData->GetDimensions(), m_snapshot.m_type, m_snapshot.m_data, coord );
	case Tr2VoxelDataType::Float16:
		return WrapPayloadElement<Float_16>( m_snapshot.m_voxelData->GetDimensions(), m_snapshot.m_type, m_snapshot.m_data, coord );
	case Tr2VoxelDataType::UInt8:
		return WrapPayloadElement<uint8_t>( m_snapshot.m_voxelData->GetDimensions(), m_snapshot.m_type, m_snapshot.m_data, coord );
	case Tr2VoxelDataType::UInt16:
		return WrapPayloadElement<uint16_t>( m_snapshot.m_voxelData->GetDimensions(), m_snapshot.m_type, m_snapshot.m_data, coord );
	case Tr2VoxelDataType::UInt32:
		return WrapPayloadElement<uint32_t>( m_snapshot.m_voxelData->GetDimensions(), m_snapshot.m_type, m_snapshot.m_data, coord );
	case Tr2VoxelDataType::Int8:
		return WrapPayloadElement<int8_t>( m_snapshot.m_voxelData->GetDimensions(), m_snapshot.m_type, m_snapshot.m_data, coord );
	case Tr2VoxelDataType::Int16:
		return WrapPayloadElement<int16_t>( m_snapshot.m_voxelData->GetDimensions(), m_snapshot.m_type, m_snapshot.m_data, coord );
	case Tr2VoxelDataType::Int32:
		return WrapPayloadElement<int32_t>( m_snapshot.m_voxelData->GetDimensions(), m_snapshot.m_type, m_snapshot.m_data, coord );
	default:
		return {};
	}
}
