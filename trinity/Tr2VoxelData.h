// Copyright © 2026 CCP ehf.

#pragma once

BLUE_CLASS_IMPL( Tr2VoxelData );

class Tr2VoxelData : public IRoot
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

	Tr2VoxelData( IRoot* lockobj = nullptr );

	bool Construct( const Dimensions& dimensions, const std::vector<ImageIO::PixelFormat>& payload );

	bool IsOccupied( CoordType x, CoordType y, CoordType z ) const;
	bool IsWithinBounds( CoordType x, CoordType y, CoordType z ) const;

	Dimensions GetDimensions() const;
	size_t GetPayloadChannelCount() const;

private:
	struct Payload
	{
		ImageIO::PixelFormat format;
		std::vector<uint8_t> data;
	};

	size_t OccupancyIndex( CoordType x, CoordType y, CoordType z ) const;

	BlueStdResult __init__( uint32_t width, uint32_t height, uint32_t depth, std::vector<ImageIO::PixelFormat> payload );
	BluePy PopulateSparse( PyObject* generator );
	BluePy SetVoxels( PyObject* set, PyObject* remove );

	bool ExtractPayload( PyObject* element, CoordType x, CoordType y, CoordType z );

	template <typename Dst, size_t N>
	bool ExtractPayloadElement( PyObject* element, size_t index, CoordType x, CoordType y, CoordType z );
	template <typename Dst, size_t N>
	bool ExtractUniformPayloadElement( PyObject* element, size_t index, CoordType x, CoordType y, CoordType z );

	Dimensions m_dimensions;
	std::vector<bool> m_occupied;
	std::vector<Payload> m_payload;
};

TYPEDEF_BLUECLASS( Tr2VoxelData );