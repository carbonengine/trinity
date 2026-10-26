// Copyright © 2026 CCP ehf.

#pragma once

BLUE_CLASS_IMPL( Tr2VoxelData );

enum class Tr2VoxelDataType
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

	Tr2VoxelData( IRoot* lockobj = nullptr );

	BlueStdResult Construct( const Dimensions& dimensions, std::vector<PayloadType> payload );

	bool IsWithinBounds( const Coord& coord ) const;

	Dimensions GetDimensions() const;
	size_t GetPayloadChannelCount() const;
	const PayloadType& GetPayloadType( size_t index ) const;
	const void* GetPayloadData( size_t index ) const;

	void GetWriteBufferAndSize( const char* memberName, unsigned char** buffer, size_t* bufferSize ) override;
	void ReleaseWriteBuffer( unsigned char* buffer ) override;
	unsigned char* AllocateReadBuffer( const char* memberName, size_t bufferSize ) override;
	void SetBufferAndSize( const char* memberName, unsigned char* buffer, size_t bufferSize ) override;

private:
	struct Payload
	{
		PayloadType type;
		std::vector<uint8_t> data;
	};

	//BluePy PopulateSparse( PyObject* generator );
	BluePy SetVoxels( PyObject* set );
	BluePy PyGetPayloadData( size_t index, const Coord& coord );

	bool ExtractPayload( PyObject* element, const Coord& coord );

	template <typename Dst>
	bool ExtractPayloadElement( PyObject* element, size_t index, const PayloadType& payloadType, const Coord& coord );
	template <typename Dst>
	bool ExtractNormalizedPayloadElement( PyObject* element, size_t index, uint8_t elementCount, const Coord& coord );
	template <typename T>
	BluePy WrapPayloadElement( const Payload& payload, const Coord& coord ) const;

	Dimensions m_dimensions;
	std::vector<Payload> m_payload;
};

TYPEDEF_BLUECLASS( Tr2VoxelData );

bool BlueExtractArgument( BlueScriptValue argument, Tr2VoxelData::Dimensions& resultRef, unsigned int argID );
bool BlueExtractArgument( BlueScriptValue argument, Tr2VoxelData::Coord& resultRef, unsigned int argID );
bool BlueExtractArgument( BlueScriptValue argument, Tr2VoxelData::PayloadType& resultRef, unsigned int argID );


