// Copyright © 2019 CCP ehf.

#include "StdAfx.h"
#include "../include/Tr2RtBottomLevelAccelerationStructureAL.h"
#include "../include/Tr2CapsAL.h"

#if TRINITY_PLATFORM_SUPPORTS_RAY_TRACING

#include TRINITY_AL_PLATFORM_INCLUDE( Tr2RtBottomLevelAccelerationStructureAL )

namespace
{
std::shared_ptr<TrinityALImpl::Tr2RtBottomLevelAccelerationStructureAL> NullBlas()
{
	static std::shared_ptr<TrinityALImpl::Tr2RtBottomLevelAccelerationStructureAL> nullBlas = std::make_shared<TrinityALImpl::Tr2RtBottomLevelAccelerationStructureAL>();
	return nullBlas;
}
}

Tr2RtBottomLevelAccelerationStructureAL::Tr2RtBottomLevelAccelerationStructureAL() :
	m_blas( NullBlas() )
{
}

ALResult Tr2RtBottomLevelAccelerationStructureAL::Create( const Tr2RtGeometryAL& geometry, Tr2RtBlasGeometryFlags::Type geometryFlags, Tr2RtBuildFlags::Type buildFlags, Tr2PrimaryRenderContextAL& renderContext )
{
	m_blas = std::make_shared<TrinityALImpl::Tr2RtBottomLevelAccelerationStructureAL>();
	auto hr = m_blas->Create( geometry, geometry, geometryFlags, buildFlags, renderContext );
	if( FAILED( hr ) )
	{
		m_blas = NullBlas();
	}
	return hr;
}

ALResult Tr2RtBottomLevelAccelerationStructureAL::Create( const Tr2RtGeometryAL& geometry, const Tr2RtGeometryAL& capacity, Tr2RtBlasGeometryFlags::Type geometryFlags, Tr2RtBuildFlags::Type buildFlags, Tr2PrimaryRenderContextAL& renderContext )
{
	m_blas = std::make_shared<TrinityALImpl::Tr2RtBottomLevelAccelerationStructureAL>();
	auto hr = m_blas->Create( geometry, capacity, geometryFlags, buildFlags, renderContext );
	if( FAILED( hr ) )
	{
		m_blas = NullBlas();
	}
	return hr;
}

ALResult Tr2RtBottomLevelAccelerationStructureAL::Update( const Tr2RtGeometryAL& geometry, Tr2RenderContextAL& renderContext )
{
	if( !IsValid() )
	{
		return E_INVALIDCALL;
	}
	return m_blas->Update( geometry, renderContext );
}

bool Tr2RtBottomLevelAccelerationStructureAL::IsValid() const
{
	return m_blas->IsValid();
}

TrinityALImpl::Tr2RtBottomLevelAccelerationStructureAL* Tr2RtBottomLevelAccelerationStructureAL::TrinityALImpl_GetObject() const
{
	return m_blas.get();
}

ALResult Tr2RtBottomLevelAccelerationStructureAL::CompactBlas( Tr2PrimaryRenderContextAL& renderContext )
{
	if( !IsValid() )
	{
		return E_INVALIDCALL;
	}
	return m_blas->CompactBlas( renderContext );
}

#else

Tr2RtBottomLevelAccelerationStructureAL::Tr2RtBottomLevelAccelerationStructureAL()
{
}

ALResult Tr2RtBottomLevelAccelerationStructureAL::Create( const Tr2RtGeometryAL& geometry, Tr2RtBlasGeometryFlags::Type geometryFlags, Tr2RtBuildFlags::Type buildFlags, Tr2PrimaryRenderContextAL& renderContext )
{
	return E_FAIL;
}

ALResult Tr2RtBottomLevelAccelerationStructureAL::Create( const Tr2RtGeometryAL& geometry, const Tr2RtGeometryAL& capacity, Tr2RtBlasGeometryFlags::Type geometryFlags, Tr2RtBuildFlags::Type buildFlags, Tr2PrimaryRenderContextAL& renderContext )
{
	return E_FAIL;
}

ALResult Tr2RtBottomLevelAccelerationStructureAL::Update( const Tr2RtGeometryAL& geometry, Tr2RenderContextAL& renderContext )
{
	return E_FAIL;
}

bool Tr2RtBottomLevelAccelerationStructureAL::IsValid() const
{
	return false;
}

ALResult Tr2RtBottomLevelAccelerationStructureAL::CompactBlas( Tr2PrimaryRenderContextAL& renderContext )
{
	return E_FAIL;
}

TrinityALImpl::Tr2RtBottomLevelAccelerationStructureAL* Tr2RtBottomLevelAccelerationStructureAL::TrinityALImpl_GetObject() const
{
	return nullptr;
}


#endif


Tr2RtPositionStreamAL::Tr2RtPositionStreamAL() :
	m_vertexCount( 0 ),
	m_vertexOffset( 0 ),
	m_positionOffset( 0 ),
	m_stride( 0 )
{
}

Tr2RtPositionStreamAL::Tr2RtPositionStreamAL( const Tr2BufferAL& vb, uint32_t positionOffset, Tr2RenderContextEnum::PixelFormat positionFormat ) :
	m_vertexBuffer( vb ),
	m_positionOffset( positionOffset ),
	m_vertexOffset( 0 ),
	m_positionFormat( positionFormat ),
	m_vertexCount( vb.GetDesc().count ),
	m_stride( vb.GetDesc().stride )
{
}

Tr2RtPositionStreamAL::Tr2RtPositionStreamAL( const Tr2BufferAL& vb, uint32_t vertexCount, uint32_t vertexOffset, uint32_t positionOffset, Tr2RenderContextEnum::PixelFormat positionFormat ) :
	m_vertexBuffer( vb ),
	m_positionOffset( positionOffset ),
	m_vertexOffset( vertexOffset ),
	m_positionFormat( positionFormat ),
	m_vertexCount( vertexCount ),
	m_stride( vb.GetDesc().stride )
{
}

Tr2RtPositionStreamAL::Tr2RtPositionStreamAL( const Tr2BufferAL& vb, uint32_t stride, uint32_t positionOffset, Tr2RenderContextEnum::PixelFormat positionFormat ) :
	m_vertexBuffer( vb ),
	m_positionOffset( positionOffset ),
	m_vertexOffset( 0 ),
	m_positionFormat( positionFormat ),
	m_vertexCount( vb.GetDesc().count ),
	m_stride( stride )
{
}

Tr2RtPositionStreamAL::Tr2RtPositionStreamAL( const Tr2BufferAL& vb, uint32_t stride, uint32_t vertexCount, uint32_t vertexOffset, uint32_t positionOffset, Tr2RenderContextEnum::PixelFormat positionFormat ) :
	m_vertexBuffer( vb ),
	m_positionOffset( positionOffset ),
	m_vertexOffset( vertexOffset ),
	m_positionFormat( positionFormat ),
	m_vertexCount( vertexCount ),
	m_stride( stride )
{
}


bool Tr2RtPositionStreamAL::IsValid() const
{
	return m_vertexBuffer.IsValid() && m_vertexCount > 0 && ( m_vertexOffset + m_vertexCount ) * m_stride <= m_vertexBuffer.GetSize() && m_positionOffset < m_stride;
}


Tr2RtIndicesStreamAL::Tr2RtIndicesStreamAL() :
	m_indexCount( 0 ),
	m_indexOffset( 0 ),
	m_stride( 0 )
{
}

Tr2RtIndicesStreamAL::Tr2RtIndicesStreamAL( const Tr2BufferAL& ib ) :
	m_indexBuffer( ib ),
	m_indexCount( ib.GetDesc().count ),
	m_indexOffset( 0 ),
	m_stride( ib.GetDesc().stride )
{
}

Tr2RtIndicesStreamAL::Tr2RtIndicesStreamAL( const Tr2BufferAL& ib, uint32_t offset, uint32_t count ) :
	m_indexBuffer( ib ),
	m_indexCount( count ),
	m_indexOffset( offset ),
	m_stride( ib.GetDesc().stride )
{
}

Tr2RtIndicesStreamAL::Tr2RtIndicesStreamAL( const Tr2BufferAL& ib, uint32_t stride, uint32_t offset, uint32_t count ) :
	m_indexBuffer( ib ),
	m_indexCount( count ),
	m_indexOffset( offset ),
	m_stride( stride )
{
}

bool Tr2RtIndicesStreamAL::IsValid() const
{
	return m_indexBuffer.IsValid() && ( m_indexOffset + m_indexCount ) * m_stride <= m_indexBuffer.GetSize();
}
