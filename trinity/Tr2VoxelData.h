// Copyright © 2026 CCP ehf.

#pragma once


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

		Tr2VoxelDataPtr m_voxelData;
		PayloadType m_type;
		size_t m_channel = 0;
		const uint8_t* m_data = nullptr;
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

	// ICustomPersist interface for persisting m_payload as a custom binary block
	void GetWriteBufferAndSize( const char* memberName, unsigned char** buffer, size_t* bufferSize ) override;
	void ReleaseWriteBuffer( unsigned char* buffer ) override;
	unsigned char* AllocateReadBuffer( const char* memberName, size_t bufferSize ) override;
	void SetBufferAndSize( const char* memberName, unsigned char* buffer, size_t bufferSize ) override;

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

	BluePy SetVoxels( PyObject* set );
	BluePy PyGetPayloadData( size_t index, const Coord& coord );

	bool ExtractPayload( PyObject* element, const Coord& coord );

	template <typename Dst>
	bool ExtractPayloadElement( PyObject* element, size_t index, const PayloadType& payloadType, const Coord& coord );
	template <typename Dst>
	bool ExtractNormalizedPayloadElement( PyObject* element, size_t index, uint8_t elementCount, const Coord& coord );

	Dimensions m_dimensions;
	std::vector<Payload> m_payload;
	std::mutex m_payloadMutex;
};

TYPEDEF_BLUECLASS( Tr2VoxelData );


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
