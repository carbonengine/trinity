// Copyright © 2024 CCP ehf.

//
//  Tr2RtTopLevelAccelerationStructureALMetal.cpp
//  carbon-trinity
//
//  Created by Iris Dogg Skarphedinsdottir on 3.1.2024.
//

#include "StdAfx.h"

#if TRINITY_PLATFORM == TRINITY_METAL

#include "Tr2RtTopLevelAccelerationStructureALMetal.h"
#include "Tr2BufferALMetal.h"

namespace TrinityALImpl
{



Tr2RtTopLevelAccelerationStructureAL::Tr2RtTopLevelAccelerationStructureAL()
{
}


Tr2RtTopLevelAccelerationStructureAL::~Tr2RtTopLevelAccelerationStructureAL()
{
}

// NOTE: this has no compaction
id<MTLAccelerationStructure>
	Tr2RtTopLevelAccelerationStructureAL::BuildAccelerationStructure( MTLAccelerationStructureDescriptor* descriptor,
																	  id<MTLDevice> device,
																	  MetalContext* metalContext )
{
	// Query for the sizes needed to store and build the acceleration structure.
	MTLAccelerationStructureSizes accelSizes = [device accelerationStructureSizesWithDescriptor:descriptor];

	// Allocate an acceleration structure large enough for this descriptor. This method
	// doesn't actually build the acceleration structure, but rather allocates memory.
	id<MTLAccelerationStructure> accelerationStructure =
		[device newAccelerationStructureWithSize:accelSizes.accelerationStructureSize];

	// Allocate scratch space Metal uses to build the acceleration structure.
	// Use MTLResourceStorageModePrivate for the best performance because the sample
	// doesn't need access to buffer's contents.
	id<MTLBuffer> scratchBuffer = [device newBufferWithLength:accelSizes.buildScratchBufferSize
													  options:MTLResourceStorageModeShared];

	auto commandEncoder = metalContext->GetPrimaryWorkQueue()->GetAccelerationStructureEncoder();
	[commandEncoder buildAccelerationStructure:accelerationStructure
									descriptor:descriptor
								 scratchBuffer:scratchBuffer
						   scratchBufferOffset:0];
	metalContext->GetPrimaryWorkQueue()->ReleaseEncoder( false );
	return accelerationStructure;
}

ALResult Tr2RtTopLevelAccelerationStructureAL::Create( size_t count,
													   const Tr2RtInstanceAL* instances,
													   Tr2RtBuildFlags::Type buildFlags,
													   Tr2PrimaryRenderContextAL& renderContext )
{
	if( @available( macOS 11.0, * ) )
	{
		if( !renderContext.IsValid() || !renderContext.GetCaps().SupportsRaytracing() )
		{
			return E_INVALIDCALL;
		}
		if( count == 0 )
		{
			return E_INVALIDARG;
		}

		m_primitiveAccelerationStructures = [[NSMutableArray alloc] init];

		MetalContext* metalContext = renderContext.GetMetalContext();
		id<MTLDevice> device = metalContext->GetDevice();

		// Allocate a buffer of acceleration structure instance descriptors. Each descriptor represents
		// an instance of one of the primitive acceleration structures created above, with its own
		// transformation matrix.

		// maybe the option should be MTLResourceStorageModePrivate
		m_instanceBuffer = [device newBufferWithLength:sizeof( MTLAccelerationStructureInstanceDescriptor ) * count
											   options:MTLResourceStorageModeShared];

		MTLAccelerationStructureInstanceDescriptor* instanceDescriptors =
			(MTLAccelerationStructureInstanceDescriptor*)m_instanceBuffer.contents;

		std::vector<uint32_t> offsets;
		offsets.reserve( 2 + count );
		offsets.push_back( 0 );
		offsets.push_back( 0 );

		// Fill out instance descriptors.
		for( NSUInteger instanceIndex = 0; instanceIndex < count; ++instanceIndex )
		{

			if( !instances[instanceIndex].blas->IsValid() )
			{
				return E_INVALIDARG;
			}

			// TODO: FIX THIS
			// it's supposed to map the instance to its acceleration structure.
			instanceDescriptors[instanceIndex].accelerationStructureIndex = (uint32_t)instanceIndex;

			// Mark the instance as opaque so that the
			// ray intersector doesn't attempt to execute a function that doesn't exist.
			instanceDescriptors[instanceIndex].options = 0; //MTLAccelerationStructureInstanceOptionOpaque;

			// Metal adds the geometry intersection function table offset and instance intersection
			// function table offset together to determine which intersection function to execute.

			// Metal adds the geometry intersection function table offset and instance intersection
			// function table offset together to determine which intersection function to execute.
			instanceDescriptors[instanceIndex].intersectionFunctionTableOffset = instances[instanceIndex].materialIndex;

			// Set the instance mask, which the sample uses to filter out intersections between rays
			// and geometry. For example, it uses masks to prevent light sources from being visible
			// to secondary rays, which would result in their contribution being double-counted.
			instanceDescriptors[instanceIndex].mask = 3;

			// Copy the first three rows of the instance transformation matrix. Metal
			// assumes that the bottom row is (0, 0, 0, 1), which allows the renderer to
			// tightly pack instance descriptors in memory.
			for( int column = 0; column < 4; column++ )
				for( int row = 0; row < 3; row++ )
					instanceDescriptors[instanceIndex].transformationMatrix.columns[column][row] =
						instances[instanceIndex].transform[row][column];

			id<MTLAccelerationStructure> blas = instances[instanceIndex].blas->m_primitiveAccelerationStructure;

			if( blas != nil )
			{
				//gather all the BLAS in one list
				[m_primitiveAccelerationStructures addObject:blas];
				offsets.push_back( instances[instanceIndex].materialIndex );
			}
		}

		// Create an instance acceleration structure descriptor
		MTLInstanceAccelerationStructureDescriptor* accelDescriptor =
			[MTLInstanceAccelerationStructureDescriptor descriptor];

		accelDescriptor.instancedAccelerationStructures = m_primitiveAccelerationStructures;
		accelDescriptor.instanceCount = count;
		accelDescriptor.instanceDescriptorBuffer = m_instanceBuffer;

		// Create the instance acceleration structure that contains all instances in the scene.
		m_instanceAccelerationStructure = BuildAccelerationStructure( accelDescriptor, device, metalContext );

		// add vector of contents
		// gpu usage is probs srv and cpu none
		if( @available( macOS 13.0, * ) )
		{
			Tr2BufferDescriptionAL bufferDesc = Tr2BufferDescriptionAL(
				1, uint32_t( offsets.size() * sizeof( uint32_t ) ), Tr2GpuUsage::SHADER_RESOURCE, Tr2CpuUsage::NONE );

			auto gpuID = m_instanceAccelerationStructure.gpuResourceID;

			memcpy( offsets.data(), &gpuID, sizeof( gpuID ) );

			m_buffer.Create( bufferDesc, offsets.data(), renderContext );
			m_buffer.SetName( "TLASBuffer" );
		}

		return S_OK;
	}
	else
	{
		return E_FAIL;
	}
}

ALResult Tr2RtTopLevelAccelerationStructureAL::Update( size_t count,
													   const Tr2RtInstanceAL* instances,
													   Tr2RenderContextAL& renderContext )
{
	return E_FAIL;
}

bool Tr2RtTopLevelAccelerationStructureAL::IsValid() const
{
	return m_instanceBuffer != nullptr;
}

const ::Tr2BufferAL& Tr2RtTopLevelAccelerationStructureAL::GetBuffer() const
{
	return m_buffer;
	//return nullptr;
}

size_t Tr2RtTopLevelAccelerationStructureAL::GetCapacity() const
{
	return 0;
}

Tr2ALMemoryType Tr2RtTopLevelAccelerationStructureAL::GetMemoryClass() const
{
	return AL_MEMORY_MANAGED;
}

void Tr2RtTopLevelAccelerationStructureAL::Describe( Tr2DeviceResourceDescriptionAL& description ) const
{
	description["type"] = "Tr2RtTopLevelAccelerationStructureAL";
}

void Tr2RtTopLevelAccelerationStructureAL::Destroy()
{
}
}

#endif
