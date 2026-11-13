// Copyright © 2026 CCP ehf.

#pragma once

#include "Tr2Event.h"

/**
 * @brief Types allowed for payload channels in Tr2VoxelData. These types are used to define the type of data stored in each channel of the voxel data.
 * Note that these types mostly correspond to CMF or texture formats, with the exception of Boolean.
 */
enum class Tr2VoxelDataType : uint8_t
{
	Boolean,
	Float,
	Float16,
	UInt8,
	UInt16,
	UInt32,
	Int8,
	Int16,
	Int32
};

/**
 * @brief Returns the name of the given Tr2VoxelDataType as a string.
 * Useful for debugging and logging purposes.
 */
constexpr const char* Tr2VoxelDataTypeName( Tr2VoxelDataType type )
{
	switch( type )
	{
	case Tr2VoxelDataType::Boolean:
		return "Boolean";
	case Tr2VoxelDataType::Float:
		return "Float";
	case Tr2VoxelDataType::Float16:
		return "Float16";
	case Tr2VoxelDataType::UInt8:
		return "UInt8";
	case Tr2VoxelDataType::UInt16:
		return "UInt16";
	case Tr2VoxelDataType::UInt32:
		return "UInt32";
	case Tr2VoxelDataType::Int8:
		return "Int8";
	case Tr2VoxelDataType::Int16:
		return "Int16";
	case Tr2VoxelDataType::Int32:
		return "Int32";
	}
	return "Unknown";
}


BLUE_CLASS_IMPL( Tr2VoxelData );

/**
 * @brief A container for 3D voxel data. Allows storing multiple data points per voxel. Each voxel can have multiple channels of data, each with its own type and number of elements.
 */
class Tr2VoxelData : public ICustomPersist
{
public:
	EXPOSE_TO_BLUE();

	/**
	 * @brief Dimensions of the voxel data.
	 */
	struct Dimensions
	{
		uint32_t width = 0;
		uint32_t height = 0;
		uint32_t depth = 0;
	};

	using CoordType = int32_t;

	/**
	 * @brief Coordinates of a voxel in the 3D space. Used to access or modify the data of a specific voxel.
	 */
	struct Coord
	{
		CoordType x = 0;
		CoordType y = 0;
		CoordType z = 0;
	};

	/**
	 * @brief Definition of a payload channel. Each channel has a type, number of elements, and whether it is normalized.
	 * Normalized channels are expected to have values in the range [0, 1] for unsigned types and [-1, 1] for signed types.
	 * When accessing normalized data, in Python, the values will be returned as floats in the range [0, 1] or [-1, 1], regardless of the underlying storage type.
	 */
	struct PayloadType
	{
		Tr2VoxelDataType type = Tr2VoxelDataType::Float;
		uint8_t elementCount = 1;
		bool normalized = false;

		bool IsValid() const;
		size_t GetBytesPerValue() const;
		size_t GetStoreSize( const Dimensions& dimensions ) const;
	};

	/**
	 * @brief Snapshot of a payload channel. This is used to access the data of a specific channel in a read-only manner, 
	 * specifically in asynchronous contexts where the underlying voxel data may be modified concurrently. The snapshot holds a 
	 * reference to the voxel data, the type of the channel, the index of the channel, and a pointer to the data.
	 * If the data changes while a snapshot is held, the snapshot will still point to the old data until it is released. This allows for safe concurrent access to voxel data.
	 */
	class PayloadSnapshot
	{
	public:
		PayloadSnapshot() = default;
		PayloadSnapshot( PayloadSnapshot&& other ) noexcept;
		~PayloadSnapshot();
		PayloadSnapshot& operator=( PayloadSnapshot&& other ) noexcept;
		operator bool() const;

		Tr2VoxelDataPtr m_voxelData;
		PayloadType m_type;
		size_t m_channel = 0;
		const uint8_t* m_data = nullptr;
	};

	/**
	 * @brief Helper class to track which channels have been modified. This is used in conjunction with the OnDataChanged event to notify listeners of changes to specific channels.
	 */
	class ModifiedChannels
	{
	public:
		bool IsChannelModified( size_t channelIndex ) const;
		void MarkChannelModified( size_t channelIndex );
		operator bool() const;
		ModifiedChannels& operator|=( const ModifiedChannels& other );

	private:
		uint64_t m_modifiedChannelsBitmask = 0;
	};

	Tr2VoxelData( IRoot* lockobj = nullptr );

	/**
	 * @brief Second part of the two-phase construction for Tr2VoxelData. Initializes the voxel data with the given dimensions and payload definition.
	 * @param dimensions The dimensions of the voxel data (width, height, depth).
	 * @param payload A vector of payload channel definitions.
	 * @return A BlueStdResult indicating success or failure.
	 */
	BlueStdResult Create( const Dimensions& dimensions, std::vector<PayloadType> payload );

	/**
	 * @brief Checks if the given coordinates are within the bounds of the voxel data dimensions.
	 * @param coord The coordinates of the voxel to check.
	 * @return True if the coordinates are within bounds, false otherwise.
	 */
	bool IsWithinBounds( const Coord& coord ) const;

	/**
	 * @brief Gets the dimensions of the voxel data.
	 * @return The dimensions of the voxel data.
	 */
	Dimensions GetDimensions() const;

	/**
	 * @brief Gets the number of payload channels in the voxel data.
	 * @return The number of payload channels.
	 */
	size_t GetPayloadChannelCount() const;

	/**
	 * @brief Gets the payload type of the specified channel.
	 * @param index The index of the payload channel.
	 * @return The payload type of the specified channel.
	 */
	const PayloadType& GetPayloadType( size_t index ) const;

	/**
	 * @brief Gets a snapshot of the specified payload channel.
	 * @param index The index of the payload channel.
	 * @return A snapshot of the specified payload channel.
	 */
	PayloadSnapshot GetPayloadSnapshot( size_t index );

	/**
	 * @brief Pauses "on data changed" notifications for subscribed views if the voxel data changes. Useful when making multiple changes to the 
	 * voxel data, to avoid unnecessary notifications. Call ResumeNotifications to resume notifications.
	 * It is possible to call PauseNotifications multiple times, but ResumeNotifications must be called the same number of times to resume
	 * the notifications.
	 */
	void PauseNotifications();

	/**
	 * @brief Resumes "on data changed" notifications for subscribed views after they have been paused. This should be called after PauseNotifications 
	 * to ensure that listeners are notified of any changes made while notifications were paused.
	 */
	void ResumeNotifications();

	/**
	 * @brief Reads one element of a Boolean payload. Boolean payloads store one bit per element, least significant bit first:
	 * element j of voxel i is bit ( i * elementCount + j ) of the payload data.
	 * @param data The payload data.
	 * @param bitIndex The index of the element's bit.
	 * @return The value of the element.
	 */
	static bool GetPackedBit( const uint8_t* data, size_t bitIndex );

	/**
	 * @brief Writes one element of a Boolean payload (see GetPackedBit for the layout).
	 * @param data The payload data.
	 * @param bitIndex The index of the element's bit.
	 * @param value The value to write.
	 * @return True if the stored value changed.
	 */
	static bool SetPackedBit( uint8_t* data, size_t bitIndex, bool value );

	// ICustomPersist interface for persisting m_payload as a custom binary block
	void GetWriteBufferAndSize( const char* memberName, unsigned char** buffer, size_t* bufferSize ) override;
	void ReleaseWriteBuffer( unsigned char* buffer ) override;
	unsigned char* AllocateReadBuffer( const char* memberName, size_t bufferSize ) override;
	void SetBufferAndSize( const char* memberName, unsigned char* buffer, size_t bufferSize ) override;

	using OnDataChanged = Tr2Event<ModifiedChannels>;

	/**
	 * @brief Event that is broadcast when the voxel data is modified. This allows listeners to be notified of changes to the voxel data, enabling them to react accordingly.
	 */
	OnDataChanged m_onDataChanged;

private:
	struct Snapshot
	{
		std::vector<uint8_t> data;
		size_t readerCount = 0;
	};

	struct Payload
	{
		PayloadType type;
		Snapshot current;
		std::vector<Snapshot> old;
	};

	BluePy SetVoxels( const std::vector<size_t>& channels, PyObject* set );
	BluePy SetAllVoxels( const std::vector<size_t>& channels, PyObject* set );
	BluePy PyGetPayloadData( size_t index, const Coord& coord );
	void BroadcastDataChanged( const ModifiedChannels& modifiedChannels );

	bool ExtractPayload( PyObject* element, Py_ssize_t tupleOffset, const Coord& coord, const std::vector<size_t>& channels, ModifiedChannels& modifiedChannels );

	template <typename Dst>
	bool ExtractPayloadElement( PyObject* element, Py_ssize_t tupleIndex, size_t channelIndex, const PayloadType& payloadType, const Coord& coord, ModifiedChannels& modifiedChannels );
	template <typename Dst>
	bool ExtractNormalizedPayloadElement( PyObject* element, Py_ssize_t tupleIndex, size_t channelIndex, uint8_t elementCount, const Coord& coord, ModifiedChannels& modifiedChannels );

	Dimensions m_dimensions;
	std::vector<Payload> m_payload;
	std::mutex m_payloadMutex;
	ModifiedChannels m_modifiedChannels;
	int32_t m_notificationsPaused = 0;
};

TYPEDEF_BLUECLASS( Tr2VoxelData );

/**
 * @brief Checks if the given coordinates are inside the specified dimensions of the voxel data.
 * @param dims The dimensions of the voxel data.
 * @param coord The coordinates to check.
 * @return True if the coordinates are inside the dimensions, false otherwise.
 */
bool IsInside( const Tr2VoxelData::Dimensions& dims, const Tr2VoxelData::Coord& coord );

/**
 * @brief Converts the given coordinates to a linear index based on the specified dimensions.
 * @param dims The dimensions of the voxel data.
 * @param coord The coordinates to convert.
 * @return The linear index corresponding to the given coordinates.
 */
int32_t LinearIndex( const Tr2VoxelData::Dimensions& dims, const Tr2VoxelData::Coord& coord );


bool BlueExtractArgument( BlueScriptValue argument, Tr2VoxelData::Dimensions& resultRef, unsigned int argID );
bool BlueExtractArgument( BlueScriptValue argument, Tr2VoxelData::Coord& resultRef, unsigned int argID );
bool BlueExtractArgument( BlueScriptValue argument, Tr2VoxelData::PayloadType& resultRef, unsigned int argID );


BLUE_CLASS_IMPL( Tr2VoxelDataTestAccessor );

/**
 * @brief Testing helper class for Tr2VoxelData. Provides access to the payload data of a specific channel for testing and debugging purposes.
 */
class Tr2VoxelDataTestAccessor : public IRoot
{
public:
	EXPOSE_TO_BLUE();

	BlueStdResult Create( Tr2VoxelData* voxelData, size_t channelIndex );
	BluePy PyGetPayloadData( const Tr2VoxelData::Coord& coord ) const;

	Tr2VoxelData::PayloadSnapshot m_snapshot;
};

TYPEDEF_BLUECLASS( Tr2VoxelDataTestAccessor );
