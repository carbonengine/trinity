#pragma once
#ifndef Tr2HalHelperStructures_h_
#define Tr2HalHelperStructures_h_

#include "Tr2RenderContextEnum.h"

class Tr2ShaderAL;
class Tr2ConstantBufferAL;
class Tr2TextureAL;
class Tr2VertexLayoutAL;
class Tr2ShaderProgramAL;

// -------------------------------------------------------------
// Description:
//	A simple structure to hold data used for initializing USAGE_IMMUTABLE textures
//	with intial pixel data.
// -------------------------------------------------------------
struct Tr2SubresourceData
{
	const void*	m_sysMem;			// pointer to pixels for this mip
	uint32_t	m_sysMemPitch;		// size in bytes of one line of pixels
	uint32_t	m_sysMemSlicePitch;	// size in bytes of entire mip level. cannot be zero!
};

// -------------------------------------------------------------
// Description:
//  Viewport for use with Tr2RenderContextAL.
// -------------------------------------------------------------
struct Tr2Viewport
{
	Tr2Viewport() {}

	Tr2Viewport( uint32_t width, uint32_t height )
	: m_x(0)
	, m_y(0)
	, m_width( (float)width )
	, m_height( (float)height )
	, m_minZ(0)
	, m_maxZ(1.0f)
	{}
	float	m_x;
	float	m_y;
	float	m_width;
	float	m_height;
	float	m_minZ;
	float	m_maxZ;
};

struct Tr2TextureCoordBox
{
	uint32_t left;
	uint32_t top;
	uint32_t front;
	uint32_t right;
	uint32_t bottom;
	uint32_t back;

	uint32_t GetWidth() const
	{
		return right - left;
	}
	uint32_t GetHeight() const
	{
		return bottom - top;
	}
	uint32_t GetDepth() const
	{
		return back - front;
	}

	bool operator==( const Tr2TextureCoordBox& other ) const
	{
		return left == other.left && top == other.top && front == other.front && right == other.right && bottom == other.bottom && back == other.back;
	}
};

// --------------------------------------------------------------------------------------
// Description:
//   Descibes region of texture (range of cubemap faces, mip levels, rectangle/box 
//   coordinates) for Tr2TextureAL::CopySubresourceRegion member function. 
// --------------------------------------------------------------------------------------
struct Tr2TextureSubresource
{
	Tr2TextureSubresource();
	explicit Tr2TextureSubresource( uint32_t mipLevel );
	Tr2TextureSubresource( uint32_t face, uint32_t mipLevel );

	void ClampToTexture( const Tr2BitmapDimensions& texture );
	bool IsSubresourceFull( const Tr2BitmapDimensions& texture ) const;
	bool IsValidForBitmap( const Tr2BitmapDimensions& bitmap ) const;
	bool HasBox() const;
	bool IsSingleSubresource() const;

	Tr2TextureSubresource& SetBox( const uint32_t* ltfrbb );
	Tr2TextureSubresource& SetRect( const uint32_t* ltrb );
	Tr2TextureSubresource& SetRect( uint32_t left, uint32_t top, uint32_t right, uint32_t bottom );

	bool operator==( const Tr2TextureSubresource& other ) const;
	bool IsValid() const;

	uint32_t GetWidth() const
	{
		return m_box.GetWidth();
	}
	uint32_t GetHeight() const
	{
		return m_box.GetHeight();
	}
	uint32_t GetDepth() const
	{
		return m_box.GetDepth();
	}
	uint32_t GetMipCount() const
	{
		return m_endMipLevel - m_startMipLevel;
	}
	uint32_t GetFaceCount() const
	{
		return m_endFace - m_startFace;
	}

	// Starting cubemap face
	uint32_t m_startFace;
	// One past ending cubemap face
	uint32_t m_endFace;
	// Starting mip level (0-based)
	uint32_t m_startMipLevel;
	// One past ending mip level
	uint32_t m_endMipLevel;
	Tr2TextureCoordBox m_box;
};

// --------------------------------------------------------------------------------------
// Description:
//   Descibes texture sampler. Used in creation of Tr2SamplerStateAL objects.  
// --------------------------------------------------------------------------------------
struct Tr2SamplerDescription
{
	Tr2SamplerDescription()
		: m_minFilter( Tr2RenderContextEnum::TF_POINT ),
		m_magFilter( Tr2RenderContextEnum::TF_POINT ),
		m_mipFilter( Tr2RenderContextEnum::TF_POINT ),
		m_isComparisonFilter( false ),
		m_addressU( Tr2RenderContextEnum::TA_WRAP ),
		m_addressV( Tr2RenderContextEnum::TA_WRAP ),
		m_addressW( Tr2RenderContextEnum::TA_WRAP ),
		m_mipLODBias( 0 ),
		m_maxAnisotropy( 1 ),
		m_comparisonFunc( Tr2RenderContextEnum::CMP_ALWAYS ),
		m_minLOD( 0.f ),
		m_maxLOD( std::numeric_limits<float>::max() )
	{
		m_borderColor[0] = 0;
		m_borderColor[1] = 0;
		m_borderColor[2] = 0;
		m_borderColor[3] = 0;
	}

	Tr2SamplerDescription(
		Tr2RenderContextEnum::TextureFilter minFilter,
		Tr2RenderContextEnum::TextureFilter magFilter,
		Tr2RenderContextEnum::TextureFilter mipFilter,
		bool isComparisonFilter,
		Tr2RenderContextEnum::TextureAddressMode addressU,
		Tr2RenderContextEnum::TextureAddressMode addressV,
		Tr2RenderContextEnum::TextureAddressMode addressW,
		float mipLODBias,
		uint32_t maxAnisotropy,
		Tr2RenderContextEnum::CompareFunc comparisonFunc,
		const float* borderColor,
		float minLOD,
		float maxLOD )
	:	m_minFilter( minFilter ),
		m_magFilter( magFilter ),
		m_mipFilter( mipFilter ),
		m_isComparisonFilter( isComparisonFilter ),
		m_addressU( addressU ),
		m_addressV( addressV ),
		m_addressW( addressW ),
		m_mipLODBias( mipLODBias ),
		m_maxAnisotropy( maxAnisotropy ),
		m_comparisonFunc( comparisonFunc ),		
		m_minLOD( minLOD ),
		m_maxLOD( maxLOD )
	{
		m_borderColor[0] = borderColor[0];
		m_borderColor[1] = borderColor[1];
		m_borderColor[2] = borderColor[2];
		m_borderColor[3] = borderColor[3];
	}

	Tr2SamplerDescription(
		Tr2RenderContextEnum::TextureFilter filter,
		Tr2RenderContextEnum::TextureAddressMode address,
		uint32_t maxAnisotropy = 1,
		float minLOD = 0.0f,
		float maxLOD = std::numeric_limits<float>::max() )
		: m_minFilter( filter ),
		m_magFilter( filter ),
		m_mipFilter( filter ),
		m_isComparisonFilter( false ),
		m_addressU( address ),
		m_addressV( address ),
		m_addressW( address ),
		m_mipLODBias( 0 ),
		m_maxAnisotropy( maxAnisotropy ),
		m_comparisonFunc( Tr2RenderContextEnum::CMP_ALWAYS ),
		m_minLOD( minLOD ),
		m_maxLOD( maxLOD )
	{
		m_borderColor[0] = 0;
		m_borderColor[1] = 0;
		m_borderColor[2] = 0;
		m_borderColor[3] = 0;
	}

	bool operator==( const Tr2SamplerDescription& other ) const
	{
		return m_minFilter == other.m_minFilter &&
			m_magFilter == other.m_magFilter &&
			m_mipFilter == other.m_mipFilter &&
			m_isComparisonFilter == other.m_isComparisonFilter &&
			m_addressU == other.m_addressU &&
			m_addressV == other.m_addressV &&
			m_addressW == other.m_addressW &&
			m_mipLODBias == other.m_mipLODBias &&
			m_maxAnisotropy == other.m_maxAnisotropy &&
			m_comparisonFunc == other.m_comparisonFunc &&
			m_borderColor[0] == other.m_borderColor[0] &&
			m_borderColor[1] == other.m_borderColor[1] &&
			m_borderColor[2] == other.m_borderColor[2] &&
			m_borderColor[3] == other.m_borderColor[3] &&
			m_minLOD == other.m_minLOD &&
			m_maxLOD == other.m_maxLOD;
	}

	Tr2RenderContextEnum::TextureFilter m_minFilter;
	Tr2RenderContextEnum::TextureFilter m_magFilter;
	Tr2RenderContextEnum::TextureFilter m_mipFilter;
	bool m_isComparisonFilter;
	Tr2RenderContextEnum::TextureAddressMode m_addressU;
	Tr2RenderContextEnum::TextureAddressMode m_addressV;
	Tr2RenderContextEnum::TextureAddressMode m_addressW;
	float m_mipLODBias;
	uint32_t m_maxAnisotropy;
	Tr2RenderContextEnum::CompareFunc m_comparisonFunc;
	float m_borderColor[4];
	float m_minLOD;
	float m_maxLOD;
};

namespace std
{
	template<> struct hash<Tr2SamplerDescription>
	{
		std::size_t operator()( const Tr2SamplerDescription& desc ) const
		{
			return
				std::hash<int32_t>()( desc.m_minFilter ) ^
				( std::hash<int32_t>()( desc.m_mipFilter ) << 1 ) ^
				( std::hash<int32_t>()( desc.m_magFilter ) << 2 ) ^
				( std::hash<int32_t>()( desc.m_addressU ) << 3 ) ^
				( std::hash<int32_t>()( desc.m_addressV ) << 4 ) ^
				( std::hash<int32_t>()( desc.m_addressW ) << 5 );
		}
	};
}


bool Crop(	Tr2TextureSubresource& sourceSR,
			const Tr2BitmapDimensions& sourceBD, 
			Tr2TextureSubresource& destSR,
			const Tr2BitmapDimensions& destBD );

void AdvanceMip( Tr2TextureSubresource& sub, const Tr2BitmapDimensions& bd, uint32_t mip );


struct Tr2MsaaDesc
{
	uint32_t samples;
	uint32_t quality;

	Tr2MsaaDesc( uint32_t samples_ = 1, uint32_t quality_ = 0 )
		:samples( std::max( samples_, 1u ) ),
		quality( quality_ )
	{
	}

	bool operator == ( const Tr2MsaaDesc& other ) const
	{
		return std::max( samples, 1u ) == std::max( other.samples, 1u ) && quality == other.quality;
	}

	bool operator != ( const Tr2MsaaDesc& other ) const
	{
		return std::max( samples, 1u ) != std::max( other.samples, 1u ) || quality != other.quality;
	}
};

#endif //Tr2HalHelperStructures_h_
