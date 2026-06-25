// Copyright © 2019 CCP ehf.

#pragma once

#include "../ALResult.h"
#include "Tr2BufferAL.h"
#include "StdAfx.h"

class Tr2PrimaryRenderContextAL;

namespace TrinityALImpl
{
class Tr2RtBottomLevelAccelerationStructureAL;
}

namespace Tr2RtBlasGeometryFlags
{
enum Type
{
	NONE,
	OPAQUE_GEOMETRY,
};

inline Type operator|( Type a, Type b )
{
	return Type( int( a ) | int( b ) );
}

inline Type& operator|=( Type& a, Type b )
{
	a = Type( a | b );
	return a;
}

inline bool HasFlag( Type value, Type flag )
{
	return ( value & flag ) == flag;
}
}

namespace Tr2RtBuildFlags
{
enum Type
{
	NONE = 0,
	ALLOW_UPDATE = 0x1,
	ALLOW_COMPACTION = 0x2,
	PREFER_FAST_TRACE = 0x4,
	PREFER_FAST_BUILD = 0x8,
	MINIMIZE_MEMORY = 0x10,
	PERFORM_UPDATE = 0x20
};

inline Type operator|( Type a, Type b )
{
	return Type( int( a ) | int( b ) );
}

inline Type& operator|=( Type& a, Type b )
{
	a = Type( a | b );
	return a;
}

inline bool HasFlag( Type value, Type flag )
{
	return ( value & flag ) == flag;
}
}

struct Tr2RtPositionStreamAL
{
	Tr2RtPositionStreamAL();
	Tr2RtPositionStreamAL( const Tr2BufferAL& vb, uint32_t positionOffset = 0, Tr2RenderContextEnum::PixelFormat positionFormat = Tr2RenderContextEnum::PIXEL_FORMAT_R32G32B32_FLOAT );
	Tr2RtPositionStreamAL( const Tr2BufferAL& vb, uint32_t vertexCount, uint32_t vertexOffset, uint32_t positionOffset, Tr2RenderContextEnum::PixelFormat positionFormat );
	Tr2RtPositionStreamAL( const Tr2BufferAL& vb, uint32_t stride, uint32_t positionOffset = 0, Tr2RenderContextEnum::PixelFormat positionFormat = Tr2RenderContextEnum::PIXEL_FORMAT_R32G32B32_FLOAT );
	Tr2RtPositionStreamAL( const Tr2BufferAL& vb, uint32_t stride, uint32_t vertexCount, uint32_t vertexOffset, uint32_t positionOffset, Tr2RenderContextEnum::PixelFormat positionFormat );

	bool IsValid() const;

	Tr2BufferAL m_vertexBuffer;
	uint32_t m_vertexCount;
	uint32_t m_vertexOffset;
	uint32_t m_positionOffset;
	uint32_t m_stride;
	Tr2RenderContextEnum::PixelFormat m_positionFormat;
};

struct Tr2RtIndicesStreamAL
{
	Tr2RtIndicesStreamAL();
	explicit Tr2RtIndicesStreamAL( const Tr2BufferAL& ib );
	Tr2RtIndicesStreamAL( const Tr2BufferAL& ib, uint32_t offset, uint32_t count );
	Tr2RtIndicesStreamAL( const Tr2BufferAL& ib, uint32_t stride, uint32_t offset, uint32_t count );

	bool IsValid() const;

	Tr2BufferAL m_indexBuffer;
	uint32_t m_indexCount;
	uint32_t m_indexOffset;
	uint32_t m_stride;
};

struct Tr2RtGeometryAL
{
	Tr2RtPositionStreamAL positions;
	Tr2RtIndicesStreamAL indices;
};

class Tr2RtBottomLevelAccelerationStructureAL
{
public:
	Tr2RtBottomLevelAccelerationStructureAL();

	ALResult Create( const Tr2RtGeometryAL& geometry, Tr2RtBlasGeometryFlags::Type geometryFlags, Tr2RtBuildFlags::Type buildFlags, Tr2PrimaryRenderContextAL& renderContext );
	ALResult Create( const Tr2RtGeometryAL& geometry, const Tr2RtGeometryAL& capacity, Tr2RtBlasGeometryFlags::Type geometryFlags, Tr2RtBuildFlags::Type buildFlags, Tr2PrimaryRenderContextAL& renderContext );
	ALResult Update( const Tr2RtGeometryAL& geometry, Tr2RenderContextAL& renderContext );
	bool IsValid() const;
	ALResult CompactBlas( Tr2PrimaryRenderContextAL& renderContext );

	TrinityALImpl::Tr2RtBottomLevelAccelerationStructureAL* TrinityALImpl_GetObject() const;

private:
	std::shared_ptr<TrinityALImpl::Tr2RtBottomLevelAccelerationStructureAL> m_blas;
};