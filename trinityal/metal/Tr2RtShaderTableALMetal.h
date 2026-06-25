// Copyright © 2024 CCP ehf.

//
//  Tr2RtShaderTableALMetal.hpp
//  ShaderCompiler
//
//  Created by Iris Dogg Skarphedinsdottir on 4.1.2024.
//


#pragma once

#if TRINITY_PLATFORM == TRINITY_METAL

#include <Metal/Metal.h>
#include "../include/Tr2RtShaderTableAL.h"
#include "../include/Tr2RtPipelineStateAL.h"

namespace TrinityALImpl
{
class Tr2RtShaderTableAL : public Tr2DeviceResourceAL<Tr2RtShaderTableAL>
{
public:
	Tr2RtShaderTableAL();
	~Tr2RtShaderTableAL();

	ALResult Create( const Tr2RtShaderTableDescriptionAL& desc,
					 const ::Tr2RtPipelineStateAL& pipeline,
					 Tr2PrimaryRenderContextAL& renderContext );
	void Destroy();
	bool IsValid() const;

	Tr2ALMemoryType GetMemoryClass() const;
	void Describe( Tr2DeviceResourceDescriptionAL& description ) const;

	struct API_AVAILABLE( macos( 13.0 ) ) ShaderTableData
	{
		MTLResourceID intersectionShaderTable;
		MTLResourceID missShaderTable;
		MTLResourceID closestHitShaderTable;
		uint64_t missMaterials;
		uint64_t hitMaterials;
		uint64_t globalInput;
	};

	API_AVAILABLE( macos( 13.0 ) )
	ShaderTableData GetShaderTableData( uint32_t rayGenIndex, uint64_t globalInputAddress ) const;

	API_AVAILABLE( macos( 13.0 ) )
	void AddUsedResources( uint32_t rayGenIndex, std::vector<id<MTLResource>>& readResources ) const;

	void SetGlobalInputBuffer( uint32_t rayGenIndex, const id<MTLBuffer>& buffer, uint32_t offset ) const;

	id<MTLBuffer> GetMaterialBuffer() const;
	NSUInteger GetRayGenMaterialOffset( uint32_t rayGenIndex ) const;


private:
	Tr2RtShaderTableDescriptionAL m_desc;

	struct API_AVAILABLE( macos( 11.0 ) ) FunctionTables
	{
		id<MTLIntersectionFunctionTable> anyHit;
		id<MTLVisibleFunctionTable> closestHit;
		id<MTLVisibleFunctionTable> miss;
	};

	API_AVAILABLE( macos( 11.0 ) )
	std::vector<FunctionTables> m_functionTables;

	id<MTLBuffer> m_materialBuffer;
	NSUInteger m_missMaterialOffset = 0;
	NSUInteger m_rayGenMaterialOffset = 0;
};
}

#endif
