// Copyright © 2024 CCP ehf.

//
//  Tr2RtBottomLevelAccelerationStructureMetal.m
//  carbon-trinity
//
//  Created by Iris Dogg Skarphedinsdottir on 3.1.2024.
//
#include "StdAfx.h"

#if TRINITY_PLATFORM == TRINITY_METAL

#include "Tr2RtBottomLevelAccelerationStructureALMetal.h"
#include "Tr2BufferALMetal.h"
#include "Tr2RenderContextMetal.h"
#include "MetalUtils.h"

namespace
{
MTLAttributeFormat ConvertVertexFormat( Tr2RenderContextEnum::PixelFormat format )
{
	switch( format )
	{
	case Tr2RenderContextEnum::PIXEL_FORMAT_R32G32B32_FLOAT:
		return MTLAttributeFormatFloat3;
	case Tr2RenderContextEnum::PIXEL_FORMAT_R16G16B16A16_FLOAT:
		return MTLAttributeFormatHalf3;

	default:
		return MTLAttributeFormatInvalid;
	}
}

// NOTE: this has no compaction
API_AVAILABLE( macos( 11.0 ) )
std::pair<id<MTLAccelerationStructure>, NSUInteger>
	BuildAccelerationStructure( MTLAccelerationStructureDescriptor* descriptor,
								TrinityALImpl::MetalContext* metalContext )
{
	id<MTLDevice> device = metalContext->GetDevice();

	// Query for the sizes needed to store and build the acceleration structure.
	MTLAccelerationStructureSizes accelSizes = [device accelerationStructureSizesWithDescriptor:descriptor];

	// Allocate an acceleration structure large enough for this descriptor. This method
	// doesn't actually build the acceleration structure, but rather allocates memory.
	id<MTLAccelerationStructure> accelerationStructure =
		[device newAccelerationStructureWithSize:accelSizes.accelerationStructureSize];

	// Allocate scratch space Metal uses to build the acceleration structure.
	id<MTLBuffer> scratchBuffer = [device newBufferWithLength:accelSizes.buildScratchBufferSize
													  options:MTLResourceStorageModePrivate];

	auto commandEncoder = metalContext->GetPrimaryWorkQueue()->GetAccelerationStructureEncoder();
	[commandEncoder buildAccelerationStructure:accelerationStructure
									descriptor:descriptor
								 scratchBuffer:scratchBuffer
						   scratchBufferOffset:0];
	metalContext->GetPrimaryWorkQueue()->ReleaseEncoder( false );

	return { accelerationStructure, accelSizes.refitScratchBufferSize };
}

}

namespace TrinityALImpl
{

Tr2RtBottomLevelAccelerationStructureAL::Tr2RtBottomLevelAccelerationStructureAL() : m_scratchBufferSize( 0 )
{
}

Tr2RtBottomLevelAccelerationStructureAL::~Tr2RtBottomLevelAccelerationStructureAL()
{
}

// Function to build a singular BLAS, list of BLAS to be kept elsewhere
ALResult Tr2RtBottomLevelAccelerationStructureAL::Create( const Tr2RtGeometryAL& geometry,
														  const Tr2RtGeometryAL&,
														  Tr2RtBlasGeometryFlags::Type geometryFlags,
														  Tr2RtBuildFlags::Type buildFlags,
														  Tr2PrimaryRenderContextAL& renderContext )
{
	if( !renderContext.IsValid() || !renderContext.GetCaps().SupportsRaytracing() )
	{
		return E_INVALIDCALL;
	}
	if( !geometry.positions.IsValid() ||
		!HasFlag( geometry.positions.m_vertexBuffer.GetDesc().gpuUsage, Tr2GpuUsage::SHADER_RESOURCE ) )
	{
		return E_INVALIDARG;
	}
	if( !geometry.indices.IsValid() ||
		!HasFlag( geometry.indices.m_indexBuffer.GetDesc().gpuUsage, Tr2GpuUsage::SHADER_RESOURCE ) )
	{
		return E_INVALIDARG;
	}
	if( @available( macOS 13.0, * ) )
	{
	}
	else
	{
		if( geometry.positions.m_positionFormat != Tr2RenderContextEnum::PIXEL_FORMAT_R32G32B32_FLOAT )
		{
			return E_INVALIDARG;
		}
	}

	if( @available( macOS 11.0, * ) )
	{

		MetalContext* metalContext = renderContext.GetMetalContext();

		// GEOMETRY DESCRIPTOR
		m_geomDesc = [MTLAccelerationStructureTriangleGeometryDescriptor descriptor];

		m_geomDesc.indexBuffer = geometry.indices.m_indexBuffer.TrinityALImpl_GetObject()->GetMetalBuffer();
		m_geomDesc.indexType = geometry.indices.m_stride == 2 ? MTLIndexTypeUInt16 : MTLIndexTypeUInt32;
		m_geomDesc.indexBufferOffset = geometry.indices.m_stride * geometry.indices.m_indexOffset;

		m_geomDesc.vertexBuffer = geometry.positions.m_vertexBuffer.TrinityALImpl_GetObject()->GetMetalBuffer();
		m_geomDesc.vertexStride = geometry.positions.m_stride;
		m_geomDesc.vertexBufferOffset =
			geometry.positions.m_vertexOffset * geometry.positions.m_stride + geometry.positions.m_positionOffset;

		m_geomDesc.triangleCount = geometry.indices.m_indexCount / 3;
		if( @available( macOS 13.0, * ) )
		{
			m_geomDesc.vertexFormat = ConvertVertexFormat( geometry.positions.m_positionFormat );
		}

		// Acceleration structure descriptor ( a descriptor for descriptors )
		m_accelerationStructureDesc = [MTLPrimitiveAccelerationStructureDescriptor descriptor];
		m_accelerationStructureDesc.geometryDescriptors = @[m_geomDesc];
		if( HasFlag( buildFlags, Tr2RtBuildFlags::ALLOW_UPDATE ) )
		{
			m_accelerationStructureDesc.usage |= MTLAccelerationStructureUsageRefit;
		}
		if( HasFlag( buildFlags, Tr2RtBuildFlags::PREFER_FAST_BUILD ) )
		{
			m_accelerationStructureDesc.usage |= MTLAccelerationStructureUsagePreferFastBuild;
		}

		auto result = BuildAccelerationStructure( m_accelerationStructureDesc, metalContext );
		m_primitiveAccelerationStructure = result.first;
		m_scratchBufferSize = result.second;

		m_geomDesc.indexBuffer = nil;
		m_geomDesc.vertexBuffer = nil;

		return S_OK;
	}
	else
	{
		return E_INVALIDCALL;
	}
}


ALResult Tr2RtBottomLevelAccelerationStructureAL::Update( const Tr2RtGeometryAL& geometry,
														  Tr2RenderContextAL& renderContext )
{
	if( !renderContext.IsValid() )
	{
		return E_INVALIDARG;
	}

	if( @available( macOS 11.0, * ) )
	{
		if( ( m_accelerationStructureDesc.usage & MTLAccelerationStructureUsageRefit ) == 0 )
		{
			return E_INVALIDARG;
		}
		if( m_geomDesc.vertexStride != geometry.positions.m_stride ||
			m_geomDesc.indexType != ( geometry.indices.m_stride == 2 ? MTLIndexTypeUInt16 : MTLIndexTypeUInt32 ) ||
			m_geomDesc.triangleCount != geometry.indices.m_indexCount / 3 )
		{
			return E_INVALIDARG;
		}

		m_geomDesc.indexBuffer = geometry.indices.m_indexBuffer.TrinityALImpl_GetObject()->GetMetalBuffer();
		m_geomDesc.indexBufferOffset = geometry.indices.m_stride * geometry.indices.m_indexOffset;

		m_geomDesc.vertexBuffer = geometry.positions.m_vertexBuffer.TrinityALImpl_GetObject()->GetMetalBuffer();
		m_geomDesc.vertexBufferOffset =
			geometry.positions.m_vertexOffset * geometry.positions.m_stride + geometry.positions.m_positionOffset;

		MetalContext* metalContext = renderContext.GetMetalContext();

		if( !m_scratchBuffer && m_scratchBufferSize > 0 )
		{
			m_scratchBuffer = [metalContext->GetDevice() newBufferWithLength:m_scratchBufferSize
																	 options:MTLResourceStorageModePrivate];
		}
		auto commandEncoder = metalContext->GetPrimaryWorkQueue()->GetAccelerationStructureEncoder();

		// refit
		[commandEncoder refitAccelerationStructure:m_primitiveAccelerationStructure
										descriptor:m_accelerationStructureDesc
									   destination:m_primitiveAccelerationStructure
									 scratchBuffer:m_scratchBuffer
							   scratchBufferOffset:0];
		metalContext->GetPrimaryWorkQueue()->ReleaseEncoder( false );

		m_geomDesc.indexBuffer = nil;
		m_geomDesc.vertexBuffer = nil;

		return S_OK;
	}
	else
	{
		return E_INVALIDCALL;
	}
}

ALResult Tr2RtBottomLevelAccelerationStructureAL::CompactBlas( Tr2PrimaryRenderContextAL& renderContext )
{
	return S_OK;
}

bool Tr2RtBottomLevelAccelerationStructureAL::IsValid() const
{
	if( @available( macOS 11.0, * ) )
	{
		return m_primitiveAccelerationStructure != nullptr;
	}
	else
	{
		return false;
	}
}

Tr2ALMemoryType Tr2RtBottomLevelAccelerationStructureAL::GetMemoryClass() const
{
	return AL_MEMORY_MANAGED;
}

void Tr2RtBottomLevelAccelerationStructureAL::Destroy()
{
	if( @available( macOS 11.0, * ) )
	{
		m_primitiveAccelerationStructure = nullptr;
		m_geomDesc = nullptr;
		m_accelerationStructureDesc = nullptr;
	}
	m_scratchBuffer = nullptr;
	m_scratchBufferSize = 0;
}

void Tr2RtBottomLevelAccelerationStructureAL::Describe( Tr2DeviceResourceDescriptionAL& description ) const
{
	description["type"] = "Tr2RtBottomLevelAccelerationStructureAL";
}

}
#endif
