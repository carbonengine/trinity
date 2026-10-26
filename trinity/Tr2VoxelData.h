// Copyright © 2026 CCP ehf.

#pragma once


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

class Tr2VoxelData : public ICustomPersist
{
public:
	EXPOSE_TO_BLUE();

	struct Dimensions
	{
		uint32_t width = 0;
		uint32_t height = 0;
		uint32_t depth = 0;
	};

	using CoordType = int32_t;

	struct Coord
	{
		CoordType x = 0;
		CoordType y = 0;
		CoordType z = 0;
	};


	struct PayloadType
	{
		Tr2VoxelDataType type = Tr2VoxelDataType::Float;
		uint8_t elementCount = 1;
		bool normalized = false;

		bool IsValid() const;
		size_t GetBytesPerValue() const;
		size_t GetStoreSize( const Dimensions& dimensions ) const;
	};

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

	BlueStdResult Create( const Dimensions& dimensions, std::vector<PayloadType> payload );

	bool IsWithinBounds( const Coord& coord ) const;

	Dimensions GetDimensions() const;
	size_t GetPayloadChannelCount() const;
	const PayloadType& GetPayloadType( size_t index ) const;
	PayloadSnapshot GetPayloadSnapshot( size_t index );

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

class Tr2VoxelDataTestAccessor : public IRoot
{
public:
	EXPOSE_TO_BLUE();

	BlueStdResult Create( Tr2VoxelData* voxelData, size_t channelIndex );
	BluePy PyGetPayloadData( const Tr2VoxelData::Coord& coord ) const;

	Tr2VoxelData::PayloadSnapshot m_snapshot;
};

TYPEDEF_BLUECLASS( Tr2VoxelDataTestAccessor );
