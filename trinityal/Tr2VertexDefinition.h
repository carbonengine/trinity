// Copyright © 2023 CCP ehf.

#pragma once
#ifndef Tr2VertexDefinition_h_
#define Tr2VertexDefinition_h_

// -------------------------------------------------------------
// Description:
//   Simple array to store a vertex declaration -- type codes, offsets,
//   sizes and usage index.  This replaces granny_data_type_definition,
//   which requires string comparisons and offset recomputations, and
//   D3DVERTEXELEMENT9[] which is DX9 specific.
// -------------------------------------------------------------
class Tr2VertexDefinition
{
public:
	enum UsageCode
	{
		POSITION,
		COLOR,
		NORMAL,
		TANGENT,
		BITANGENT,
		TEXCOORD,
		BLENDINDICES,
		BLENDWEIGHTS,

		NUM_USAGE_CODE
	};

	enum DataType
	{
		DT_INT8 = 0,
		DT_INT16 = 1,
		DT_INT32 = 2,
		DT_FLOAT16 = 3,
		DT_FLOAT32 = 4,

		DT_TYPE_MASK = 7,

		DT_UNSIGNED_BIT = 1 << 3,
		DT_NORMALIZED_BIT = 1 << 4,

		DT_SIZE_OFFSET = 5,
		DT_SIZE_MASK = 3 << DT_SIZE_OFFSET,
		DT_SIZE_1 = 0 << DT_SIZE_OFFSET,
		DT_SIZE_2 = 1 << DT_SIZE_OFFSET,
		DT_SIZE_3 = 2 << DT_SIZE_OFFSET,
		DT_SIZE_4 = 3 << DT_SIZE_OFFSET,

		BYTE_1 = DT_INT8 | DT_SIZE_1,
		BYTE_2 = DT_INT8 | DT_SIZE_2,
		BYTE_3 = DT_INT8 | DT_SIZE_3,
		BYTE_4 = DT_INT8 | DT_SIZE_4,

		UBYTE_1 = DT_INT8 | DT_SIZE_1 | DT_UNSIGNED_BIT,
		UBYTE_2 = DT_INT8 | DT_SIZE_2 | DT_UNSIGNED_BIT,
		UBYTE_3 = DT_INT8 | DT_SIZE_3 | DT_UNSIGNED_BIT,
		UBYTE_4 = DT_INT8 | DT_SIZE_4 | DT_UNSIGNED_BIT,

		SHORT_1 = DT_INT16 | DT_SIZE_1,
		SHORT_2 = DT_INT16 | DT_SIZE_2,
		SHORT_3 = DT_INT16 | DT_SIZE_3,
		SHORT_4 = DT_INT16 | DT_SIZE_4,

		USHORT_1 = DT_INT16 | DT_SIZE_1 | DT_UNSIGNED_BIT,
		USHORT_2 = DT_INT16 | DT_SIZE_2 | DT_UNSIGNED_BIT,
		USHORT_3 = DT_INT16 | DT_SIZE_3 | DT_UNSIGNED_BIT,
		USHORT_4 = DT_INT16 | DT_SIZE_4 | DT_UNSIGNED_BIT,

		INT32_1 = DT_INT32 | DT_SIZE_1,
		INT32_2 = DT_INT32 | DT_SIZE_2,
		INT32_3 = DT_INT32 | DT_SIZE_3,
		INT32_4 = DT_INT32 | DT_SIZE_4,

		UINT32_1 = DT_INT32 | DT_SIZE_1 | DT_UNSIGNED_BIT,
		UINT32_2 = DT_INT32 | DT_SIZE_2 | DT_UNSIGNED_BIT,
		UINT32_3 = DT_INT32 | DT_SIZE_3 | DT_UNSIGNED_BIT,
		UINT32_4 = DT_INT32 | DT_SIZE_4 | DT_UNSIGNED_BIT,

		FLOAT16_1 = DT_FLOAT16 | DT_SIZE_1,
		FLOAT16_2 = DT_FLOAT16 | DT_SIZE_2,
		FLOAT16_3 = DT_FLOAT16 | DT_SIZE_3,
		FLOAT16_4 = DT_FLOAT16 | DT_SIZE_4,

		UFLOAT16_1 = DT_FLOAT16 | DT_SIZE_1 | DT_UNSIGNED_BIT,
		UFLOAT16_2 = DT_FLOAT16 | DT_SIZE_2 | DT_UNSIGNED_BIT,
		UFLOAT16_3 = DT_FLOAT16 | DT_SIZE_3 | DT_UNSIGNED_BIT,
		UFLOAT16_4 = DT_FLOAT16 | DT_SIZE_4 | DT_UNSIGNED_BIT,

		FLOAT32_1 = DT_FLOAT32 | DT_SIZE_1,
		FLOAT32_2 = DT_FLOAT32 | DT_SIZE_2,
		FLOAT32_3 = DT_FLOAT32 | DT_SIZE_3,
		FLOAT32_4 = DT_FLOAT32 | DT_SIZE_4,

		UFLOAT32_1 = DT_FLOAT32 | DT_SIZE_1 | DT_UNSIGNED_BIT,
		UFLOAT32_2 = DT_FLOAT32 | DT_SIZE_2 | DT_UNSIGNED_BIT,
		UFLOAT32_3 = DT_FLOAT32 | DT_SIZE_3 | DT_UNSIGNED_BIT,
		UFLOAT32_4 = DT_FLOAT32 | DT_SIZE_4 | DT_UNSIGNED_BIT,

		BYTE_1_NORM = BYTE_1 | DT_NORMALIZED_BIT,
		BYTE_2_NORM = BYTE_2 | DT_NORMALIZED_BIT,
		BYTE_3_NORM = BYTE_3 | DT_NORMALIZED_BIT,
		BYTE_4_NORM = BYTE_4 | DT_NORMALIZED_BIT,

		UBYTE_1_NORM = UBYTE_1 | DT_NORMALIZED_BIT,
		UBYTE_2_NORM = UBYTE_2 | DT_NORMALIZED_BIT,
		UBYTE_3_NORM = UBYTE_3 | DT_NORMALIZED_BIT,
		UBYTE_4_NORM = UBYTE_4 | DT_NORMALIZED_BIT,

		SHORT_1_NORM = SHORT_1 | DT_NORMALIZED_BIT,
		SHORT_2_NORM = SHORT_2 | DT_NORMALIZED_BIT,
		SHORT_3_NORM = SHORT_3 | DT_NORMALIZED_BIT,
		SHORT_4_NORM = SHORT_4 | DT_NORMALIZED_BIT,

		USHORT_1_NORM = USHORT_1 | DT_NORMALIZED_BIT,
		USHORT_2_NORM = USHORT_2 | DT_NORMALIZED_BIT,
		USHORT_3_NORM = USHORT_3 | DT_NORMALIZED_BIT,
		USHORT_4_NORM = USHORT_4 | DT_NORMALIZED_BIT,

		DT_UNKNOWN_TYPE = 0xffFFffFFu
	};

	struct Item
	{
		Item();

		UsageCode m_usage;
		unsigned m_usageIndex;
		DataType m_dataType;
		unsigned m_offset;
		unsigned m_stream;
		unsigned m_instanceStepRate;

		bool operator==( const Item& other ) const;

		// most common way to sort these is by offset
		bool operator<( const Item& other ) const
		{
			return m_offset < other.m_offset;
		}
	};

	std::vector<Item> m_items;

	// Return true if there are no items at all in m_items.
	bool empty() const
	{
		return m_items.empty();
	}

	// Stores the next offset for an item that will be added with Add(). Otherwise unused,
	// the vertex definition and layout don't care about this value.
	// We support up to OFFSET_COUNT streams, if you have more than that then you will need
	// to track the offsets manually.
	enum
	{
		OFFSET_COUNT = 4
	};
	unsigned m_nextOffset[OFFSET_COUNT];

	Tr2VertexDefinition();
	bool operator==( const Tr2VertexDefinition& other ) const;

	Tr2VertexDefinition( const Tr2VertexDefinition& other );
	Tr2VertexDefinition& operator=( const Tr2VertexDefinition& other );
	Tr2VertexDefinition& operator=( Tr2VertexDefinition&& other );


	// Find the first item that has m_usage == usage, if any, else returns nullptr.
	Item* Find( UsageCode usage );

	// Find the first item that has m_usage == usage and a matching usageIndex, if any, else returns nullptr.
	Item* Find( UsageCode usage, unsigned usageIndex );

	// Helper function to add an item, which
	// - has an offset equal to the current m_nextOffset,
	// - with the provided usageIndex and stream etc,
	// - and updating m_nextOffset with the size of the newly added element.
	// stepRate should almost always be a bool that indicates if the added item is coming from instanced data or not.
	// Returns the item just added.
	Item& Add( DataType type, UsageCode usage, unsigned usageIndex = 0, unsigned stream = 0, unsigned stepRate = 0 );

	static inline unsigned GetDataTypeSizeInMembers( DataType dataType )
	{
		return ( ( dataType & DT_SIZE_MASK ) >> DT_SIZE_OFFSET ) + 1;
	}
	// Static helper array that tells you how many bytes a given data type takes.
	static inline unsigned GetDataTypeSizeInBytes( DataType dataType )
	{
		const unsigned size = GetDataTypeSizeInMembers( dataType );
		switch( dataType & DT_TYPE_MASK )
		{
		case DT_INT8:
			return size;
		case DT_INT16:
			return size * 2;
		case DT_INT32:
			return size * 4;
		case DT_FLOAT16:
			return size * 2;
		case DT_FLOAT32:
			return size * 4;
		}
		return 0;
	}
};


#endif // Tr2VertexDefinition_h_
