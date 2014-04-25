#include "StdAfx.h"

#if( TRINITY_PLATFORM==TRINITY_ORBIS )

#include "Tr2RenderContextOrbis.h"
#include "Tr2RenderContextEnum.h"
#include "Tr2MemoryAllocator.h"
#include "ITr2RenderContextEvents.h"
#include "ALLog.h"
#include <gnm/gpumem.h>
#include <video_out.h>
#include <gnmx/shader_parser.h>
#include <memory>

using namespace Tr2RenderContextEnum;

namespace
{
Tr2PrimaryRenderContextAL*& GetPrimaryRenderContext()
{
	static Tr2PrimaryRenderContextAL* renderContext = nullptr;
	return renderContext;
}

struct RECT
{
	int32_t left;
	int32_t top;
	int32_t right;
	int32_t bottom;
};

bool ConvertVertexTypeToNaitiveDataFormat( Tr2VertexDefinition::DataType type, sce::Gnm::DataFormat& naitiveFormat )
{
	switch( type )
	{
	case Tr2VertexDefinition::BYTE_1:
		naitiveFormat = sce::Gnm::kDataFormatR8Sint;
		break;
	case Tr2VertexDefinition::BYTE_2:
		naitiveFormat = sce::Gnm::kDataFormatR8G8Sint;
		break;
	case Tr2VertexDefinition::BYTE_4:
		naitiveFormat = sce::Gnm::kDataFormatR8G8B8A8Sint;
		break;

	case Tr2VertexDefinition::UBYTE_1:
		naitiveFormat = sce::Gnm::kDataFormatR8Uint;
		break;
	case Tr2VertexDefinition::UBYTE_2:
		naitiveFormat = sce::Gnm::kDataFormatR8G8Uint;
		break;
	case Tr2VertexDefinition::UBYTE_4:
		naitiveFormat = sce::Gnm::kDataFormatR8G8B8A8Uint;
		break;

	case Tr2VertexDefinition::SHORT_1:
		naitiveFormat = sce::Gnm::kDataFormatR16Sint;
		break;
	case Tr2VertexDefinition::SHORT_2:
		naitiveFormat = sce::Gnm::kDataFormatR16G16Sint;
		break;
	case Tr2VertexDefinition::SHORT_4:
		naitiveFormat = sce::Gnm::kDataFormatR16G16B16A16Sint;
		break;

	case Tr2VertexDefinition::USHORT_1:
		naitiveFormat = sce::Gnm::kDataFormatR16Uint;
		break;
	case Tr2VertexDefinition::USHORT_2:
		naitiveFormat = sce::Gnm::kDataFormatR16G16Uint;
		break;
	case Tr2VertexDefinition::USHORT_4:
		naitiveFormat = sce::Gnm::kDataFormatR16G16B16A16Uint;
		break;

	case Tr2VertexDefinition::INT32_1:
		naitiveFormat = sce::Gnm::kDataFormatR32Sint;
		break;
	case Tr2VertexDefinition::INT32_2:
		naitiveFormat = sce::Gnm::kDataFormatR32G32Sint;
		break;
	case Tr2VertexDefinition::INT32_3:
		naitiveFormat = sce::Gnm::kDataFormatR32G32B32Sint;
		break;
	case Tr2VertexDefinition::INT32_4:
		naitiveFormat = sce::Gnm::kDataFormatR32G32B32A32Sint;
		break;

	case Tr2VertexDefinition::UINT32_1:
		naitiveFormat = sce::Gnm::kDataFormatR32Uint;
		break;
	case Tr2VertexDefinition::UINT32_2:
		naitiveFormat = sce::Gnm::kDataFormatR32G32Uint;
		break;
	case Tr2VertexDefinition::UINT32_3:
		naitiveFormat = sce::Gnm::kDataFormatR32G32B32Uint;
		break;
	case Tr2VertexDefinition::UINT32_4:
		naitiveFormat = sce::Gnm::kDataFormatR32G32B32A32Uint;
		break;

	case Tr2VertexDefinition::FLOAT16_1:
		naitiveFormat = sce::Gnm::kDataFormatR16Float;
		break;
	case Tr2VertexDefinition::FLOAT16_2:
		naitiveFormat = sce::Gnm::kDataFormatR16G16Float;
		break;
	case Tr2VertexDefinition::FLOAT16_4:
		naitiveFormat = sce::Gnm::kDataFormatR16G16B16A16Float;
		break;

	case Tr2VertexDefinition::FLOAT32_1:
		naitiveFormat = sce::Gnm::kDataFormatR32Float;
		break;
	case Tr2VertexDefinition::FLOAT32_2:
		naitiveFormat = sce::Gnm::kDataFormatR32G32Float;
		break;
	case Tr2VertexDefinition::FLOAT32_3:
		naitiveFormat = sce::Gnm::kDataFormatR32G32B32Float;
		break;
	case Tr2VertexDefinition::FLOAT32_4:
		naitiveFormat = sce::Gnm::kDataFormatR32G32B32A32Float;
		break;

	case Tr2VertexDefinition::BYTE_1_NORM:
		naitiveFormat = sce::Gnm::kDataFormatR8Snorm;
		break;
	case Tr2VertexDefinition::BYTE_2_NORM:
		naitiveFormat = sce::Gnm::kDataFormatR8G8Snorm;
		break;
	case Tr2VertexDefinition::BYTE_4_NORM:
		naitiveFormat = sce::Gnm::kDataFormatR8G8B8A8Snorm;
		break;

	case Tr2VertexDefinition::UBYTE_1_NORM:
		naitiveFormat = sce::Gnm::kDataFormatR8Unorm;
		break;
	case Tr2VertexDefinition::UBYTE_2_NORM:
		naitiveFormat = sce::Gnm::kDataFormatR8G8Unorm;
		break;
	case Tr2VertexDefinition::UBYTE_4_NORM:
		naitiveFormat = sce::Gnm::kDataFormatR8G8B8A8Unorm;
		break;

	case Tr2VertexDefinition::SHORT_1_NORM:
		naitiveFormat = sce::Gnm::kDataFormatR16Snorm;
		break;
	case Tr2VertexDefinition::SHORT_2_NORM:
		naitiveFormat = sce::Gnm::kDataFormatR16G16Snorm;
		break;
	case Tr2VertexDefinition::SHORT_4_NORM:
		naitiveFormat = sce::Gnm::kDataFormatR16G16B16A16Snorm;
		break;

	case Tr2VertexDefinition::USHORT_1_NORM:
		naitiveFormat = sce::Gnm::kDataFormatR16Unorm;
		break;
	case Tr2VertexDefinition::USHORT_2_NORM:
		naitiveFormat = sce::Gnm::kDataFormatR16G16Unorm;
		break;
	case Tr2VertexDefinition::USHORT_4_NORM:
		naitiveFormat = sce::Gnm::kDataFormatR16G16B16A16Unorm;
		break;
	default:
		return false;
	}
	return true;
}

bool ConvertTopologyToPrimitiveType( Topology topology, sce::Gnm::PrimitiveType& primitiveType )
{
	switch( topology )
	{
	case TOP_TRIANGLES:
		primitiveType = sce::Gnm::kPrimitiveTypeTriList;
		return true;
	case TOP_TRIANGLE_STRIP:
		primitiveType = sce::Gnm::kPrimitiveTypeTriStrip;
		return true;
	case TOP_LINES:
		primitiveType = sce::Gnm::kPrimitiveTypeLineList;
		return true;
	case TOP_LINE_STRIP:
		primitiveType = sce::Gnm::kPrimitiveTypeLineStrip;
		return true;
	case TOP_POINTS:
		primitiveType = sce::Gnm::kPrimitiveTypePointList;
		return true;
	case TOP_TRIANGLE_FAN:
		primitiveType = sce::Gnm::kPrimitiveTypeTriFan;
		return true;
	default:
		CCP_ASSERT( false && "Unsupported topology" );
		return false;
	}
}

sce::Gnm::ShaderStage ConvertShaderTypeToShaderStage( Tr2RenderContextEnum::ShaderType type )
{
	switch( type )
	{
	case VERTEX_SHADER:
		return sce::Gnm::kShaderStageVs;
	case PIXEL_SHADER:
		return sce::Gnm::kShaderStagePs;
	default:
		return sce::Gnm::kShaderStageCs;
	}
}

sce::Gnm::StencilOp ConvertStencilOp( uint32_t value )
{
	switch( value )
	{
	case 2:
		return sce::Gnm::kStencilOpZero;
	case 3:
		return sce::Gnm::kStencilOpReplaceOp;
	case 4:
		return sce::Gnm::kStencilOpAddClamp;
	case 5:
		return sce::Gnm::kStencilOpSubClamp;
	case 6:
		return sce::Gnm::kStencilOpInvert;
	case 7:
		return sce::Gnm::kStencilOpAddWrap;
	case 8:
		return sce::Gnm::kStencilOpSubWrap;
	default:
		return sce::Gnm::kStencilOpKeep;
	}
}

sce::Gnm::BlendFunc ConvertBlendOp( uint32_t value )
{
	switch( value )
	{
	case BO_SUBTRACT:
		return sce::Gnm::kBlendFuncSubtract;
	case BO_REVSUBTRACT:
		return sce::Gnm::kBlendFuncReverseSubtract;
	case BO_MIN:
		return sce::Gnm::kBlendFuncMin;
	case BO_MAX:
		return sce::Gnm::kBlendFuncMax;
	default:
		return sce::Gnm::kBlendFuncAdd;
	}
}

sce::Gnm::PrimitiveSetupPolygonMode ConvertFillMode( uint32_t value )
{
	switch( value )
	{
	case FM_POINT:
		return sce::Gnm::kPrimitiveSetupPolygonModePoint;
		break;
	case FM_WIREFRAME:
		return sce::Gnm::kPrimitiveSetupPolygonModeLine;
		break;
	default:
		return sce::Gnm::kPrimitiveSetupPolygonModeFill;
	}
}

sce::Gnm::CompareFunc ConvertCompareFunc( uint32_t value )
{
	return sce::Gnm::CompareFunc( value - 1 );
}

sce::Gnm::BlendMultiplier ConvertBlendMultiplier( uint32_t value )
{
	return sce::Gnm::BlendMultiplier( value - 1 );
}

sce::Gnm::PrimitiveSetupCullFaceMode ConvertCullFace( uint32_t value )
{
	switch( value )
	{
	case CULLMODE_NONE:
		return sce::Gnm::kPrimitiveSetupCullFaceNone;
	case CULLMODE_CW:
		return sce::Gnm::kPrimitiveSetupCullFaceBack;
	case CULLMODE_CCW:
		return sce::Gnm::kPrimitiveSetupCullFaceFront;
	default:
		return sce::Gnm::kPrimitiveSetupCullFaceFrontAndBack;
	}
}

void CreateFetchRemapTable( 
	const Tr2VertexDefinition& vertexDefinition, 
	const Tr2ShaderInputDefinition& shaderInput, 
	uint32_t* remapTable, 
	sce::Gnm::FetchShaderInstancingMode* instanceData )
{
	for( size_t j = 0; j < vertexDefinition.m_items.size(); ++j )
	{
		auto& def = vertexDefinition.m_items[j];
		remapTable[j] = 0xffffffff;
		for( size_t i = 0; i < shaderInput.elements.size(); ++i )
		{
			auto& el = shaderInput.elements[i];
			if( def.m_usage == el.usage && def.m_usageIndex == el.usageIndex )
			{
				remapTable[j] = i;
				if( instanceData )
				{
					instanceData[j] = def.m_stream == 1 ? sce::Gnm::kFetchShaderUseInstanceId : sce::Gnm::kFetchShaderUseVertexIndex;
				}
				break;
			}
		}
	}
}

bool IsTileModeLinear( sce::Gnm::TileMode tileMode )
{
	return tileMode == sce::Gnm::kTileModeDisplay_LinearAligned;
}

bool TileModesAreSafeForMsaaResolve( sce::Gnm::TileMode destMode, sce::Gnm::TileMode srcMode )
{
	return IsTileModeLinear(destMode) == IsTileModeLinear(srcMode);
}

}

Tr2RenderContextAL::Tr2RenderContextAL()
	:m_backBuffer( nullptr ),
	m_registeredBuffers( -1 ),
	m_flipEventQueue( nullptr ),
	m_frameIndex( 0 ),
	m_layout( nullptr ),
	m_indices( nullptr ),
	m_drawPrimitiveIndexBuffer( nullptr ),
	m_drawPrimitiveIndexCount( 0 ),
	m_topology( TOP_TRIANGLES ),
	m_borderColorCount( 0 ),
	m_borderColorColors( nullptr ),
	m_dirtyFlag( 0 )
{
	for( uint32_t i = 0; i < BUFFER_COUNT; ++i )
	{
		m_buffers[i].cpRamShadow = nullptr;
		m_buffers[i].cueHeap = nullptr;
		m_buffers[i].dcbBuffer = nullptr;
		m_buffers[i].ccbBuffer = nullptr;
	}
	for( uint32_t i = 0; i < STREAM_COUNT; ++i )
	{
		m_streams[i].m_buffer = nullptr;
		m_streams[i].m_stride = 0;
	}
	for( uint32_t i = 0; i < MAX_RENDER_TARGET; ++i )
	{
		m_stackRT[i].SetName( "Tr2RenderContextAL::m_stackRT" );
		m_boundRenderTarget[i] = nullptr;
	}
	m_stackDS.SetName( "Tr2RenderContextAL::m_stackDS" );
	m_boundDepthStencil = nullptr;
	std::fill( std::begin( m_boundShaders ), std::end( m_boundShaders ), nullptr );
	std::fill( std::begin( m_renderStates ), std::end( m_renderStates ), 0 );

	memset( &m_fragmentOpSettings, 0, sizeof( m_fragmentOpSettings ) );

	m_alphaTestParameters.m_alphaTestEnabled = 0;
	m_alphaTestParameters.m_alphaTestRef = 0;
	m_alphaTestParameters.m_alphaTestFunc = CMP_ALWAYS;
}

Tr2RenderContextAL::~Tr2RenderContextAL()
{
	Destroy();
}

void Tr2RenderContextAL::SetPrimaryRenderContext( Tr2PrimaryRenderContextAL* renderContex )
{
	::GetPrimaryRenderContext() = renderContex;
}

Tr2PrimaryRenderContextAL& Tr2RenderContextAL::GetPrimaryRenderContext()
{
	CCP_ASSERT( ::GetPrimaryRenderContext() );
	return *::GetPrimaryRenderContext();
}

void Tr2RenderContextAL::Destroy()
{
	if( IsValid() )
	{
		InternalSyncToGpu();
	}

	m_emptyTexture.Destroy();

	m_dirtyFlag = 0;
	m_fragmentOpBuffer.Destroy();
	m_drawUP.Destroy();
	m_backBufferRT.Destroy();

	Tr2MemoryAllocator::Release( const_cast<uint64_t*>( m_labels ) );
	m_labels = nullptr;

	Tr2MemoryAllocator::Release( m_borderColorColors );
	m_borderColorColors = nullptr;
	m_borderColorCount = 0;

	for( auto it = m_delayDeletes.begin(); it != m_delayDeletes.end(); ++it )
	{
		Tr2MemoryAllocator::Release( it->pointer );
	}
	m_delayDeletes.clear();

	for( uint32_t i = 0; i < STREAM_COUNT; ++i )
	{
		m_streams[i].m_buffer = nullptr;
		m_streams[i].m_stride = 0;
	}
	for( uint32_t i = 0; i < SHADER_TYPE_COUNT; ++i )
	{
		m_boundShaders[i] = 0;
	}
	for( uint32_t i = 0; i < MAX_RENDER_TARGET; ++i )
	{
		while( !m_stackRT[i].empty() )
		{
			m_stackRT[i].pop();
		}
		m_boundRenderTarget[i] = nullptr;
	}
	while( !m_stackDS.empty() )
	{
		m_stackDS.pop();
	}
	m_boundDepthStencil = nullptr;
	Tr2MemoryAllocator::Release( m_drawPrimitiveIndexBuffer );
	m_drawPrimitiveIndexBuffer = nullptr;
	m_drawPrimitiveIndexCount = 0;

	m_clearShader.Destroy();
	if( m_flipEventQueue )
	{
		sceVideoOutDeleteFlipEvent( m_flipEventQueue, Tr2VideoAdapterInfo::InternalGetVideoHandle() );
		sceKernelDeleteEqueue( m_flipEventQueue );
		m_flipEventQueue = nullptr;
	}
	if( m_registeredBuffers >= 0 )
	{
		sceVideoOutUnregisterBuffers( Tr2VideoAdapterInfo::InternalGetVideoHandle(), m_registeredBuffers );
		m_registeredBuffers = -1;
	}
	for( uint32_t i = 0; i < BUFFER_COUNT; ++i )
	{
		free( m_buffers[i].cpRamShadow );
		m_buffers[i].cpRamShadow = nullptr;
		Tr2MemoryAllocator::Release( m_buffers[i].cueHeap );
		m_buffers[i].cueHeap = nullptr;
		Tr2MemoryAllocator::Release( m_buffers[i].dcbBuffer );
		m_buffers[i].dcbBuffer = nullptr;
		Tr2MemoryAllocator::Release( m_buffers[i].ccbBuffer );
		m_buffers[i].ccbBuffer = nullptr;
		Tr2MemoryAllocator::Release( m_buffers[i].renderTarget.getBaseAddress() );
		m_buffers[i].renderTarget.setBaseAddress( nullptr );


		Tr2MemoryAllocator::Release( m_buffers[i].depthTarget.getHtileAddress() );
		m_buffers[i].depthTarget.setHtileAddress( nullptr );
		Tr2MemoryAllocator::Release( m_buffers[i].depthTarget.getZWriteAddress() );
		Tr2MemoryAllocator::Release( m_buffers[i].depthTarget.getStencilReadAddress() );
		m_buffers[i].depthTarget.setAddresses( nullptr, nullptr );
	}
	m_frameIndex = 0;
	m_backBuffer = nullptr;

	memset( &m_fragmentOpSettings, 0, sizeof( m_fragmentOpSettings ) );

	m_alphaTestParameters.m_alphaTestEnabled = 0;
	m_alphaTestParameters.m_alphaTestRef = 0;
	m_alphaTestParameters.m_alphaTestFunc = CMP_ALWAYS;
}

ALResult Tr2RenderContextAL::CreateDevice(			
	uint32_t Adapter, 
	Tr2WindowHandle hFocusWindow, 
	const Tr2PresentParametersAL& presentationParameters )
{
	Tr2MemoryAllocator::Initialize();

	Destroy();

	int32_t videoHandle = Tr2VideoAdapterInfo::InternalGetVideoHandle();
	Tr2DisplayModeInfo defaultMode;
	CR_RETURN_HR( Tr2VideoAdapterInfo::GetAdapterDisplayMode( 0, defaultMode ) );
	//if( presentationParameters.mode.width != defaultMode.width ||
	//	presentationParameters.mode.height != defaultMode.height )
	//{
	//	return E_INVALIDARG;
	//}

	const uint32_t kNumRingEntries = 16;

	const uint32_t cueCpRamShadowSize = sce::Gnmx::ConstantUpdateEngine::computeCpRamShadowSize();
	const uint32_t cueHeapSize = sce::Gnmx::ConstantUpdateEngine::computeHeapSize( kNumRingEntries );
	const size_t dcbBufferSize = 2 * 1024 * 1024;
	const size_t ccbBufferSize = 2 * 1024 * 1024;

	void* renderTargetAddress[BUFFER_COUNT];

	m_labels = const_cast<volatile uint64_t*>(static_cast<uint64_t*>(Tr2MemoryAllocator::AllocateOnion(sizeof(uint64_t) * BUFFER_COUNT, sizeof(uint64_t))));

	for( uint32_t i = 0; i < BUFFER_COUNT; ++i )
	{
		m_labels[i] = 0xFFFFFFFF;
		m_buffers[i].m_expectedLabel = 0xFFFFFFFF;
		m_buffers[i].cpRamShadow = malloc( cueCpRamShadowSize );
		m_buffers[i].cueHeap = Tr2MemoryAllocator::AllocateGarlic( cueHeapSize, sce::Gnm::kAlignmentOfBufferInBytes );
		m_buffers[i].dcbBuffer = Tr2MemoryAllocator::AllocateOnion( dcbBufferSize, sce::Gnm::kAlignmentOfBufferInBytes );
		m_buffers[i].ccbBuffer = Tr2MemoryAllocator::AllocateOnion( ccbBufferSize, sce::Gnm::kAlignmentOfBufferInBytes );
		if( !m_buffers[i].cpRamShadow ||
			!m_buffers[i].cueHeap ||
			!m_buffers[i].dcbBuffer ||
			!m_buffers[i].ccbBuffer )
		{
			return E_OUTOFMEMORY;
		}
		m_buffers[i].context.init(
			m_buffers[i].cpRamShadow,
			m_buffers[i].cueHeap, 
			kNumRingEntries,		// Constant Update Engine
			m_buffers[i].dcbBuffer, 
			dcbBufferSize,		// Draw command buffer
			m_buffers[i].ccbBuffer, 
			ccbBufferSize			// Constant command buffer
			);

		sce::Gnm::TileMode tileMode;
		sce::Gnm::DataFormat format = sce::Gnm::kDataFormatB8G8R8A8UnormSrgb;
		sce::GpuAddress::computeSurfaceTileMode(
			&tileMode,
			sce::GpuAddress::kSurfaceTypeColorTargetDisplayable,
			format, 1);

		const sce::Gnm::SizeAlign sizeAlign = m_buffers[i].renderTarget.init(
			presentationParameters.mode.width,
			presentationParameters.mode.height,
			1,                // Number of slices
			format,
			tileMode,
			sce::Gnm::kNumSamples1,    // Number of samples per pixel
			sce::Gnm::kNumFragments1,    // Number of fragments per pixel
			nullptr,                // No CMASK
			nullptr );            // No FMASK
		renderTargetAddress[i] = Tr2MemoryAllocator::AllocateGarlic( sizeAlign );
		m_buffers[i].renderTarget.setAddresses( renderTargetAddress[i], nullptr, nullptr );

		const sce::Gnm::ZFormat kZFormat = sce::Gnm::kZFormat32Float;
		const sce::Gnm::StencilFormat kStencilFormat = sce::Gnm::kStencilInvalid;

		sce::Gnm::DataFormat depthFormat = sce::Gnm::DataFormat::build( kZFormat );
		sce::Gnm::TileMode depthTileMode;
		sce::GpuAddress::computeSurfaceTileMode(
			&depthTileMode,
			sce::GpuAddress::kSurfaceTypeDepthOnlyTarget,
			depthFormat,
			1 );

		sce::Gnm::SizeAlign stencilSizeAlign;
		sce::Gnm::SizeAlign htileSizeAlign;
		sce::Gnm::SizeAlign depthTargetSizeAlign = m_buffers[i].depthTarget.init(
			presentationParameters.mode.width,
			presentationParameters.mode.height,
			depthFormat.getZFormat(),
			kStencilFormat,
			depthTileMode,
			sce::Gnm::kNumFragments1,
			kStencilFormat != sce::Gnm::kStencilInvalid ? &stencilSizeAlign : nullptr,
			/*&htileSizeAlign*/ nullptr );

		/*if( kHtileEnabled )
		{
			m_buffers[i].depthTarget.setHtileAddress( Tr2MemoryAllocator::AllocateGarlic( htileSizeAlign ) );
		}*/

		void *stencilMemory = NULL;
		if( kStencilFormat != sce::Gnm::kStencilInvalid )
		{
			stencilMemory = Tr2MemoryAllocator::AllocateGarlic( stencilSizeAlign );
		}
		void* depthMemory = Tr2MemoryAllocator::AllocateGarlic( depthTargetSizeAlign );

		m_buffers[i].depthTarget.setAddresses( depthMemory, stencilMemory );

	}

	m_backBuffer = m_buffers;
	m_backBufferIndex = 0;


	SceVideoOutBufferAttribute videoOutBufferAttribute;
	sceVideoOutSetBufferAttribute(
		&videoOutBufferAttribute,
		SCE_VIDEO_OUT_PIXEL_FORMAT_A8R8G8B8_SRGB,
		SCE_VIDEO_OUT_TILING_MODE_TILE,
		SCE_VIDEO_OUT_ASPECT_RATIO_16_9,
		m_backBuffer->renderTarget.getWidth(),
		m_backBuffer->renderTarget.getHeight(),
		m_backBuffer->renderTarget.getPitch() );

	m_registeredBuffers = sceVideoOutRegisterBuffers(
		videoHandle,
		0, // Start index.
		renderTargetAddress,
		BUFFER_COUNT,
		&videoOutBufferAttribute );

	sceVideoOutSetFlipRate( videoHandle, 0 ); // 60Hz

	sceKernelCreateEqueue( &m_flipEventQueue, "FLIP QUEUE" );
	sceVideoOutAddFlipEvent( m_flipEventQueue, videoHandle, nullptr );



	// Create clear shader
	const uint32_t cs_set_uint_fast_c[] = {
		0x161e0300, 0x77035ad3, 0xb28faf81, 0x00000102,
		0x000000b0, 0x00000000, 0x00010040, 0x00000001,
		0x00000000, 0x72646853, 0x00000007, 0x00000d04,
		0x00000000, 0x0300006c, 0x00000000, 0x00000034,
		0xffffffff, 0x000c0080, 0x000000a0, 0x00000040,
		0x00000001, 0x00000001, 0x00000000, 0x00040000,
		0x00080004, 0x000c0002, 0xbeeb03ff, 0x00000009,
		0x8f008610, 0x4a000000, 0xc2400d00, 0xbf8c007f,
		0xd1880002, 0x00020000, 0xbe822402, 0x36020001,
		0xe0002000, 0x80010101, 0xbf8c1f70, 0xe0102000,
		0x80020100, 0xbf810000, 0x00040000, 0x00080004,
		0x000c0002, 0x00000000, 0x5362724f, 0x07726468,
		0x00004053, 0x00000301, 0x77035ad3, 0xb28faf81,
		0x683e50dc, 0x00000003, 0x00000002, 0x00000002,
		0x00000000, 0x00000000, 0x00000000, 0x00000048,
		0x00000000, 0x00000000, 0x000c0500, 0x00000000,
		0x00000000, 0x000000bc, 0x00000000, 0x00000008,
		0x006a0416, 0x00000002, 0x00000000, 0x000000ab,
		0x00000000, 0x00000004, 0x000c000c, 0x00000000,
		0xffffffff, 0x0000009d, 0x00000000, 0x00000000,
		0x00160c00, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x00000089, 0x00000004, 0x00000000,
		0x00160c00, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x00000075, 0x0000010c, 0x00000000,
		0x00000004, 0x00000000, 0xffffffff, 0x00000000,
		0x00000000, 0x00000049, 0x00000064, 0x0000010c,
		0x00000004, 0x00000004, 0x00000000, 0xffffffff,
		0x00000000, 0x00000000, 0x00000031, 0x00000040,
		0x72756f53, 0x43006563, 0x74736e6f, 0x73746e61,
		0x73654400, 0x616e6974, 0x6e6f6974, 0x645f6d00,
		0x55747365, 0x73746e69, 0x735f6d00, 0x69556372,
		0x4d73746e, 0x73756e69, 0x00656e4f, 0x5f6f6e28,
		0x656d616e, 0x00000029,
		};

	CR_RETURN_HR( m_clearShader.Create( 
		*this, 
		COMPUTE_SHADER, 
		cs_set_uint_fast_c, 
		sizeof( cs_set_uint_fast_c),
		nullptr,
		0,
		Tr2ShaderInputDefinition() ) );

	m_borderColorColors = reinterpret_cast<float*>( Tr2MemoryAllocator::AllocateGarlic( 
		MAX_BORDER_COLORS * 4 * sizeof( float ), 
		256 ) );
	m_borderColorCount = 0;

	const uint32_t quadVSBytecode[] = {
		0x170b0300, 0xfa7cbc73, 0xb4a7a05f, 0x00000100,
		0x00000094, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x72646853, 0x00000007, 0x00000a01,
		0x00000000, 0x0000005c, 0x00000000, 0x00000028,
		0xffffffff, 0x000c0000, 0x00000004, 0x00000000,
		0x00000004, 0x00000000, 0x00000000, 0xbeeb03ff,
		0x00000007, 0x36020081, 0x34020281, 0x360000c2,
		0x4a0202c1, 0x4a0000c1, 0x7e020b01, 0x7e000b00,
		0x7e040280, 0x7e0602f2, 0xf80008cf, 0x03020001,
		0xf800020f, 0x03030303, 0xbf810000, 0x5362724f,
		0x07726468, 0x00004047, 0x00000000, 0xfa7cbc73,
		0xb4a7a05f, 0x9531e409, 0x00000000, 0x00000000,
		0x00000000, 0x00000000, 0x00000101, 0x00000000,
		0x00000020, 0x00002f0c, 0x00000000, 0x00000018,
		0x00000017, 0x00002e03, 0x00000000, 0x00000015,
		0x00000007, 0x28006469, 0x6e5f6f6e, 0x29656d61,
		0x69616d00, 0x5f6d2e6e, 0x69736f70, 0x6e6f6974,
		0x00000000,
		};

	CR_RETURN_HR( m_quadVS.Create( 
		*this, 
		VERTEX_SHADER, 
		quadVSBytecode, 
		sizeof( quadVSBytecode),
		nullptr,
		0,
		Tr2ShaderInputDefinition() ) );

	const uint32_t dummyPSBytecode[] = {
		0x170b0300, 0xbfb3137f, 0xe15e3e30, 0x00000101,
		0x00000088, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x72646853, 0x00000007, 0x00000f02,
		0x00000000, 0x0000003c, 0x00000000, 0x0000003c,
		0xffffffff, 0x000c0000, 0x00000004, 0x00000000,
		0x00000004, 0x00000002, 0x00000002, 0x00000000,
		0x00000000, 0x00000010, 0x0000000f, 0x00000000,
		0xbeeb03ff, 0x00000003, 0x7e000280, 0x5e000100,
		0x7e000000, 0xf8001c0f, 0x00000000, 0xbf810000,
		0x5362724f, 0x07726468, 0x00002043, 0x00000000,
		0xbfb3137f, 0xe15e3e30, 0x0cdfeaab, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x00000100,
		0x00000000, 0x00000018, 0x00003803, 0x00000000,
		0x00000008, 0x0000000f, 0x6e69616d, 0x6c6f432e,
		0x2800726f, 0x6e5f6f6e, 0x29656d61, 0x00000000,
		};
	CR_RETURN_HR( m_dummyPS.Create( 
		*this, 
		PIXEL_SHADER, 
		dummyPSBytecode, 
		sizeof( dummyPSBytecode),
		nullptr,
		0,
		Tr2ShaderInputDefinition() ) );

	const uint32_t downsampleCSBytecode[] = {
		0x170b0300, 0xa21d5e86, 0x0210b3c6, 0x00000102,
		0x00000138, 0x00000000, 0x00080008, 0x00000001,
		0x00000000, 0x72646853, 0x00000007, 0x00000d04,
		0x00000000, 0x030000f4, 0x00000000, 0x00000034,
		0xffffffff, 0x000c0084, 0x00000998, 0x00000008,
		0x00000008, 0x00000001, 0x00000000, 0x0002011b,
		0x03040000, 0x03100004, 0xbeeb03ff, 0x0000001a,
		0x8f00830c, 0x8f01830d, 0x4a000000, 0x4a020201,
		0x34040081, 0x34060281, 0x4a1c0481, 0x4a160681,
		0x7e140302, 0x7e180280, 0xf0041f00, 0x0001060a,
		0x7e14030e, 0xf0041f00, 0x00010a0a, 0x7e1e0303,
		0x7e200280, 0xf0041f00, 0x00010e0e, 0x7e080280,
		0xf0041f00, 0x00010202, 0xc0c00300, 0xbf8c1f72,
		0x060c1506, 0x060e1707, 0x06101908, 0x06121b09,
		0xbf8c1f71, 0x060c0d0e, 0x060e0f0f, 0x06101110,
		0x06121311, 0xbf8c1f70, 0x06040d02, 0x06060f03,
		0x06081104, 0x060a1305, 0x7e0c02ff, 0x3e800000,
		0x10040506, 0x10060d03, 0x10080d04, 0x100a0d05,
		0xbf8c007f, 0xf0203f00, 0x00000200, 0xbf810000,
		0x0002011b, 0x03040000, 0x03100004, 0x00000000,
		0x5362724f, 0x07726468, 0x0000c851, 0x00000301,
		0xa21d5e86, 0x0210b3c6, 0x0b10241e, 0x00000002,
		0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x00000014, 0x00000000, 0x00000000,
		0x00030502, 0x00000000, 0x00000000, 0x0000001c,
		0x00000000, 0x00000010, 0x0003000e, 0x00000000,
		0xffffffff, 0x0000000e, 0x72756f53, 0x65546563,
		0x65440078, 0x65547473, 0x00000078,	};

	CR_RETURN_HR( m_downsampleCS.Create( 
		*this, 
		COMPUTE_SHADER, 
		downsampleCSBytecode, 
		sizeof( downsampleCSBytecode),
		nullptr,
		0,
		Tr2ShaderInputDefinition() ) );

	const uint32_t copyCSBytecode[] = {
		0x170b0300, 0x9f97fabf, 0xe31734b3, 0x00000102,
		0x000000e4, 0x00000000, 0x00080008, 0x00000001,
		0x00000000, 0x72646853, 0x00000007, 0x00000e04,
		0x00000000, 0x0400009c, 0x00000000, 0x00000038,
		0xffffffff, 0x000c0081, 0x000009a0, 0x00000008,
		0x00000008, 0x00000001, 0x00000000, 0x0002011b,
		0x03040000, 0x000c0002, 0x03100004, 0xbeeb03ff,
		0x0000000f, 0x8f008310, 0x8f018311, 0x4a000000,
		0x4a020201, 0xc2400d04, 0xbf8c007f, 0xd1880010,
		0x00020201, 0x7d880000, 0x87ea6a10, 0xbe80246a,
		0xbf88000d, 0xc2860d00, 0xbf8c007f, 0x4a04000e,
		0x4a06020f, 0x7e080280, 0xf0041f00, 0x00010202,
		0x4a00000c, 0x4a02020d, 0xc0c00300, 0xbf8c0070,
		0xf0203f00, 0x00000200, 0xbf810000, 0x0002011b,
		0x03040000, 0x000c0002, 0x03100004, 0x5362724f,
		0x07726468, 0x00007051, 0x00000400, 0x9f97fabf,
		0xe31734b3, 0xffff8149, 0x00000003, 0x00000002,
		0x00000002, 0x00000000, 0x00000000, 0x00000000,
		0x00000034, 0x00000000, 0x00000000, 0x00030502,
		0x00000000, 0x00000000, 0x000000bc, 0x00000000,
		0x00000020, 0x006a0416, 0x00000002, 0x00000000,
		0x000000ae, 0x00000000, 0x00000010, 0x0003000e,
		0x00000000, 0xffffffff, 0x000000a0, 0x00000000,
		0x00000000, 0x00160b00, 0x00000000, 0x00000000,
		0x00000000, 0x00000000, 0x00000088, 0x00000010,
		0x00000001, 0x00160b00, 0x00000000, 0x00000000,
		0x00000000, 0x00000000, 0x00000070, 0x0000010b,
		0x00000000, 0x00000010, 0x00000000, 0xffffffff,
		0x00000000, 0x00000000, 0x00000048, 0x00000052,
		0x0000010b, 0x00000010, 0x00000010, 0x00000000,
		0xffffffff, 0x00000000, 0x00000000, 0x0000002c,
		0x0000002e, 0x72756f53, 0x65546563, 0x6f430078,
		0x6174736e, 0x0073746e, 0x74736544, 0x00786554,
		0x7366666f, 0x00737465, 0x657a6973, 0x6e280073,
		0x616e5f6f, 0x0029656d,
		};

	CR_RETURN_HR( m_copyCS.Create( 
		*this, 
		COMPUTE_SHADER, 
		copyCSBytecode, 
		sizeof( copyCSBytecode),
		nullptr,
		0,
		Tr2ShaderInputDefinition() ) );
	
	m_viewport.m_x = 0;
	m_viewport.m_y = 0;
	m_viewport.m_width = presentationParameters.mode.width;
	m_viewport.m_height = presentationParameters.mode.height;
	m_viewport.m_minZ = 0;
	m_viewport.m_maxZ = 1;

	if( m_events )
	{
		m_events->OnContextCreated( *this );
	}

	uint32_t pixel = 0;
	Tr2SubresourceData initialData;
	initialData.m_sysMem = &pixel;
	initialData.m_sysMemPitch = 4;
	initialData.m_sysMemSlicePitch = 4;
	initialData.m_height = 1;

	m_emptyTexture.Create2D( 1, 1, 1, PIXEL_FORMAT_B8G8R8A8_UNORM, USAGE_IMMUTABLE, &initialData, *this );

	m_renderStates[RS_ZENABLE] = 0;
	m_renderStates[RS_FILLMODE] = FM_SOLID;
	m_renderStates[RS_ZWRITEENABLE] = 0;
	m_renderStates[RS_SRCBLEND] = 1;
	m_renderStates[RS_DESTBLEND] = 1;
	m_renderStates[RS_CULLMODE] = CULLMODE_NONE;
	m_renderStates[RS_ZFUNC] = CMP_NEVER;
	m_renderStates[RS_ALPHABLENDENABLE] = 0;
	m_renderStates[RS_STENCILENABLE] = 0;
	m_renderStates[RS_STENCILFAIL] = 1;
	m_renderStates[RS_STENCILZFAIL] = 1;
	m_renderStates[RS_STENCILPASS] = 1;
	m_renderStates[RS_STENCILFUNC] = 1;
	m_renderStates[RS_STENCILREF] = 0;
	m_renderStates[RS_STENCILMASK] = 0;
	m_renderStates[RS_STENCILWRITEMASK] = 0;
	m_renderStates[RS_CLIPPING] = 0;
	m_renderStates[RS_CLIPPLANEENABLE] = 0;
	m_renderStates[RS_MULTISAMPLEANTIALIAS] = 1;
	m_renderStates[RS_MULTISAMPLEMASK] = 0xffffffff;
	m_renderStates[RS_COLORWRITEENABLE] = 0xffffffff;
	m_renderStates[RS_BLENDOP] = 1;
	m_renderStates[RS_SCISSORTESTENABLE] = 0;
	m_renderStates[RS_SLOPESCALEDEPTHBIAS] = 0;
	m_renderStates[RS_TWOSIDEDSTENCILMODE] = 0;
	m_renderStates[RS_CCW_STENCILFAIL] = 1;
	m_renderStates[RS_CCW_STENCILZFAIL] = 1;
	m_renderStates[RS_CCW_STENCILPASS] = 1;
	m_renderStates[RS_CCW_STENCILFUNC] = 1;
	m_renderStates[RS_SRGBWRITEENABLE] = 0;
	m_renderStates[RS_DEPTHBIAS] = 0;
	m_renderStates[RS_SEPARATEALPHABLENDENABLE] = 0;
	m_renderStates[RS_SRCBLENDALPHA] = 1;
	m_renderStates[RS_DESTBLENDALPHA] = 1;
	m_renderStates[RS_BLENDOPALPHA] = 1;

	m_depthStencilControl.init();
	m_primitiveSetup.init();
	m_blendControl.init();
	m_stencilOpControl.init();
	m_stencilControl.init();
	m_clipControl.init();

	CR( DoBeginFrame() );

	return S_OK;
}

ALResult Tr2RenderContextAL::SetPresentParameters( unsigned adapter, const Tr2PresentParametersAL& presentationParameters )
{
	m_backBufferRT.Attach( m_backBuffer->renderTarget, GetBackBufferFormat() );
	return S_OK;
}

const Tr2CapsAL& Tr2RenderContextAL::GetCaps() const
{
	return m_caps;
}

bool Tr2RenderContextAL::IsValid() const
{
	return m_backBuffer != nullptr;
}

ALResult Tr2RenderContextAL::Clear(						
	uint32_t clearFlags, 
	uint32_t color, 
	float depth, 
	uint32_t stencil )
{
	sce::Gnmx::GfxContext& gfxc = m_backBuffer->context;
	if( clearFlags & CLEARFLAGS_TARGET )
	{
		// TODO: only 32bpp RTs
		sce::Gnm::RenderTarget renderTarget;
		if( m_boundRenderTarget[0] )
		{
			m_boundRenderTarget[0]->InternalGetRenderTargetForGpu( 0, renderTarget, *this );
		}
		else
		{
			renderTarget = m_backBuffer->renderTarget;
		}
		if( renderTarget.getCmaskAddress() )
		{
			uint32_t clearColor[2] = { color, 0x00000000 };
			gfxc.setCmaskClearColor( 0, clearColor );

			gfxc.triggerEvent( sce::Gnm::kEventTypeFlushAndInvalidateCbPixelData );

			uint32_t *source = static_cast<uint32_t*>( gfxc.allocateFromCommandBuffer( sizeof( uint32_t ) * 4, sce::Gnm::kEmbeddedDataAlignment4 ) );
			source[0] = 0;

			ClearMemoryWithCompute( renderTarget.getCmaskAddress(), renderTarget.getCmaskSizeInBytes() / sizeof(uint32_t), source, 1 );

			// Flush the CB color cache
			volatile uint32_t *cbLabel = (uint32_t*)gfxc.allocateFromCommandBuffer( sizeof( uint32_t ), sce::Gnm::kEmbeddedDataAlignment8 );
			*cbLabel = 0;
			gfxc.writeImmediateDwordAtEndOfPipe( sce::Gnm::kEopFlushCbDbCaches, const_cast<uint32_t*>( cbLabel ), 1, sce::Gnm::kCacheActionNone );
			gfxc.waitOnAddress( const_cast<uint32_t*>( cbLabel ), 0xFFFFFFFF, sce::Gnm::kWaitCompareFuncEqual, 1 );
		}
		else
		{
			uint32_t *source = static_cast<uint32_t*>( gfxc.allocateFromCommandBuffer( sizeof( uint32_t ) * 4, sce::Gnm::kEmbeddedDataAlignment4 ) );
			source[0] = color;

			void* destination = renderTarget.getBaseAddress();
			uint32_t destUint32s = renderTarget.getSizeInBytes() / sizeof( uint32_t );

			ClearMemoryWithCompute( destination, destUint32s, source, 1 );
		}
	}
	if( ( clearFlags & CLEARFLAGS_ZBUFFER ) && ( m_boundDepthStencil != &nullDS ) )
	{
		sce::Gnm::DbRenderControl dbRenderControl;
		dbRenderControl.init();
		dbRenderControl.setDepthClearEnable( true );
		gfxc.setDbRenderControl(dbRenderControl);

		sce::Gnm::DepthStencilControl depthControl;
		depthControl.init();
		depthControl.setDepthControl( sce::Gnm::kDepthControlZWriteEnable, sce::Gnm::kCompareFuncAlways );
		depthControl.setStencilFunction( sce::Gnm::kCompareFuncNever );
		depthControl.setDepthEnable( true );
		gfxc.setDepthStencilControl( depthControl );

		gfxc.setDepthClearValue( depth );


		const sce::Gnm::DepthRenderTarget* depthTarget = m_boundDepthStencil ? &m_boundDepthStencil->m_depthStencil : &m_backBuffer->depthTarget;
		gfxc.setRenderTargetMask( 0x0 );
		gfxc.setDepthRenderTarget( depthTarget );

		//float* constantBuffer = static_cast<float*>( gfxc.allocateFromCommandBuffer( sizeof( float ) * 4, sce::Gnm::kEmbeddedDataAlignment4 ) );
		//*constantBuffer = ToVector4Unaligned(Vector4(0.f, 0.f, 0.f, 0.f));
		//sce::Gnm::Buffer buffer;
		//buffer.initAsConstantBuffer( constantBuffer, sizeof( float ) * 4 );
		//buffer.setResourceMemoryType( sce::Gnm::kResourceMemoryTypeRO );
		//gfxc.setConstantBuffers( sce::Gnm::kShaderStagePs, 0, 1, &buffer );

		const uint32_t width = depthTarget->getWidth();
		const uint32_t height = depthTarget->getHeight();
		gfxc.setupScreenViewport( 0, 0, width, height, 1.0f, 0.0f );
		gfxc.setPsShader( m_dummyPS.m_shader.m_psShader );

		gfxc.setVsShader( m_quadVS.m_shader.m_vsShader, 0, nullptr );
		gfxc.setPrimitiveType( sce::Gnm::kPrimitiveTypeRectList );
		gfxc.drawIndexAuto( 3 );

		gfxc.setRenderTargetMask( 0xF );

		sce::Gnm::DbRenderControl dbRenderControl2;
		dbRenderControl2.init();
		gfxc.setDbRenderControl( dbRenderControl2 );

		SetDepthStencil();
		SetRenderTarget( 0 );
		m_dirtyFlag |= Tr2FragmentOpSettings::DIRTY_PATCH_VS | Tr2FragmentOpSettings::DIRTY_PATCH_VS;
	}
	return S_OK;
}

void Tr2RenderContextAL::ClearMemoryWithCompute( void* destination, size_t destUint32s, uint32_t* source, size_t sourceCount )
{
	sce::Gnmx::GfxContext& gfxc = m_backBuffer->context;
	gfxc.setShaderType( sce::Gnm::kShaderTypeCompute );
	gfxc.setCsShader( m_clearShader.m_shader.m_csShader );
	sce::Gnm::Buffer destinationBuffer;
	destinationBuffer.initAsDataBuffer( destination, sce::Gnm::kDataFormatR32Uint, destUint32s );
	destinationBuffer.setResourceMemoryType( sce::Gnm::kResourceMemoryTypeGC );
	gfxc.setRwBuffers( sce::Gnm::kShaderStageCs, 0, 1, &destinationBuffer );

	sce::Gnm::Buffer sourceBuffer;
	sourceBuffer.initAsDataBuffer( source, sce::Gnm::kDataFormatR32Uint, sourceCount );
	sourceBuffer.setResourceMemoryType( sce::Gnm::kResourceMemoryTypeRO );
	gfxc.setBuffers( sce::Gnm::kShaderStageCs, 0, 1, &sourceBuffer );

	struct Constants
	{
		uint32_t m_destUints;
		uint32_t m_srcUints;
	};
	Constants *constants = (Constants*)gfxc.allocateFromCommandBuffer( sizeof( Constants ), sce::Gnm::kEmbeddedDataAlignment4 );
	constants->m_destUints = destUint32s;
	constants->m_srcUints = sourceCount - 1;
	sce::Gnm::Buffer constantBuffer;
	constantBuffer.initAsConstantBuffer( constants, sizeof( *constants ) );
	gfxc.setConstantBuffers( sce::Gnm::kShaderStageCs, 0, 1, &constantBuffer );

	gfxc.dispatch( ( destUint32s + sce::Gnm::kThreadsPerWavefront - 1 ) / sce::Gnm::kThreadsPerWavefront, 1, 1);

	volatile uint64_t* label = (volatile uint64_t*)gfxc.m_dcb.allocateFromCommandBuffer( sizeof( uint64_t ), sce::Gnm::kEmbeddedDataAlignment8 ); // allocate memory from the command buffer
	*label = 0x0; // set the memory to have the val 0
	gfxc.m_dcb.writeAtEndOfShader( sce::Gnm::kEosCsDone, const_cast<uint64_t*>(label), 0x1 ); // tell the CP to write a 1 into the memory only when all compute shaders have finished
	gfxc.m_dcb.waitOnAddress( const_cast<uint64_t*>( label ), 0xffffffff, sce::Gnm::kWaitCompareFuncEqual, 0x1 ); // tell the CP to wait until the memory has the val 1
	gfxc.m_dcb.flushShaderCachesAndWait( sce::Gnm::kCacheActionWriteBackAndInvalidateL1andL2, 0, sce::Gnm::kStallCommandBufferParserDisable ); // tell the CP to flush the L1$ and L2$

	gfxc.setShaderType( sce::Gnm::kShaderTypeGraphics );
}

ALResult Tr2RenderContextAL::BeginScene()
{
	return S_OK;
}

ALResult Tr2RenderContextAL::EndScene()
{
	return S_OK;
}

ALResult Tr2RenderContextAL::DoBeginFrame()
{
	sce::Gnmx::GfxContext &gfxc = m_backBuffer->context;
	gfxc.reset();
	gfxc.initializeDefaultHardwareState();
	gfxc.waitUntilSafeForRendering( Tr2VideoAdapterInfo::InternalGetVideoHandle(), m_backBufferIndex );
	gfxc.setupScreenViewport(
		0,            // Left
		0,            // Top
		m_backBuffer->renderTarget.getWidth(),    // Right
		m_backBuffer->renderTarget.getHeight(),    // Bottom
		( m_viewport.m_maxZ - m_viewport.m_minZ ), 
		m_viewport.m_minZ );        // Z-offset
	gfxc.setRenderTarget( 0, &m_backBuffer->renderTarget );
	gfxc.setDepthRenderTarget( &m_backBuffer->depthTarget );

	m_dirtyFlag = 0xffffffff;

	m_depthStencilControlDirty = true;
	m_primitiveSetupDirty = true;
	m_blendControl.setSeparateAlphaEnable( false );
	m_blendControlDirty = true;
	m_stencilOpControlDirty = true;
	m_stencilControlDirty = true;
	m_clipControlDirty = true;

	m_msaaEnabled = sce::Gnm::kScanModeControlAaEnable;
	m_scissorEnabled = sce::Gnm::kScanModeControlViewportScissorDisable;
	m_scanModeControlDirty = true;

	m_polygonOffsetScale = 0.f;
	m_polygonOffset = 0.f;
	m_polygonOffsetDirty = true;

	m_linearRenderTarget = true;

	for( uint32_t i = 0; i < MAX_RENDER_TARGET; ++i )
	{
		m_boundRenderTarget[i] = nullptr;
	}

	for( uint32_t i = 0; i < sizeof( m_sharedConstantBuffers ) / sizeof( m_sharedConstantBuffers[0] ); ++i )
	{
		m_sharedConstantBuffers[i].size = 0;
	}

	for( auto it = std::begin( m_boundTextures ); it != std::end( m_boundTextures ); ++it )
	{
		it->texture = nullptr;
	}

	m_backBufferRT.Attach( m_backBuffer->renderTarget, GetBackBufferFormat() );

	for( uint32_t shaderType = VERTEX_SHADER; shaderType <= PIXEL_SHADER; ++shaderType )
	{
		for( uint32_t i = 0; i < 16; ++i )
		{
			SetTexture( ShaderType( shaderType ), i, nullTX );
		}
	}
	return S_OK;
}

#include <algorithm>

ALResult Tr2RenderContextAL::Present()
{
	sce::Gnmx::GfxContext& gfxc = m_backBuffer->context;

	++m_frameIndex;

	m_backBuffer->m_expectedLabel = m_frameIndex;
	gfxc.writeImmediateAtEndOfPipe( 
		sce::Gnm::kEopFlushCbDbCaches, 
		const_cast<uint64_t*>( m_labels + m_backBufferIndex ), 
		m_frameIndex, 
		sce::Gnm::kCacheActionNone );

	int32_t state = gfxc.submit();
	CCP_ASSERT( state == sce::Gnm::kSubmissionSuccess );
	sce::Gnm::submitDone();

	volatile uint32_t wait = 0;
	while( static_cast<uint32_t>( m_labels[m_backBufferIndex] ) != m_buffers[m_backBufferIndex].m_expectedLabel )
	{
		wait++;
	}

	// Set Flip Request
	int ret=sceVideoOutSubmitFlip(Tr2VideoAdapterInfo::InternalGetVideoHandle(), m_backBufferIndex, SCE_VIDEO_OUT_FLIP_MODE_VSYNC, 0);
	CCP_ASSERT(ret >= 0);

	{
		SceKernelEvent flipEvent;
		int count;
		sceKernelWaitEqueue( m_flipEventQueue, &flipEvent, 1, &count, nullptr );
	}

	size_t size = m_delayDeletes.size();
	for( size_t i = 0; i < size; )
	{
		uint32_t distance = std::min( m_delayDeletes[i].frame - m_frameIndex, m_frameIndex - m_delayDeletes[i].frame );
		if( distance > InternalGetMaxFrameLatency() )
		{
			Tr2MemoryAllocator::Release( m_delayDeletes[i].pointer );
			m_delayDeletes[i] = m_delayDeletes[--size];
		}
		else
		{
			++i;
		}
	}
	m_delayDeletes.resize( size );

	m_backBufferIndex = ( m_backBufferIndex + 1 ) % BUFFER_COUNT;
	m_backBuffer = m_buffers + m_backBufferIndex;

	CR( DoBeginFrame() );
	return S_OK;
}

ALResult Tr2RenderContextAL::SetStreamSource(	
	uint32_t stream, 
	const Tr2VertexBufferAL & buffer, 
	uint32_t offset, 
	uint32_t stride )
{
	if( stream >= STREAM_COUNT )
	{
		return E_INVALIDARG;
	}
	if( offset != 0 )
	{
		return E_INVALIDARG;
	}
	if( !buffer.IsValid() )
	{
		if( &buffer != &nullVB )
		{
			return E_FAIL;
		}
	}
	m_streams[stream].m_buffer = &buffer;
	m_streams[stream].m_stride = stride;
	return S_OK;
}

ALResult Tr2RenderContextAL::SetVertexLayout( Tr2VertexLayoutAL& layout )
{
	if( !layout.IsValid() )
	{
		return E_INVALIDARG;
	}

	m_layout = &layout;

	return S_OK;
}

ALResult Tr2RenderContextAL::SetShader( const Tr2ShaderAL& shader )
{
	if( shader.GetType() == INVALID_SHADER )
	{
		return E_INVALIDARG;
	}
	if( shader.GetType() == VERTEX_SHADER )
	{
		if( !m_boundShaders[VERTEX_SHADER] || !( *m_boundShaders[VERTEX_SHADER] == shader ) )
		{
			m_dirtyFlag |= Tr2FragmentOpSettings::DIRTY_PATCH_VS;
		}
	}
	else if( shader.GetType() == PIXEL_SHADER )
	{
		if( !m_boundShaders[PIXEL_SHADER] || !( *m_boundShaders[PIXEL_SHADER] == shader ) )
		{
			m_dirtyFlag |= Tr2FragmentOpSettings::DIRTY_PATCH_PS;
		}		
	}

	m_boundShaders[shader.GetType()] = shader.m_bytecode ? &shader : nullptr;
	shader.m_frameUsed = InternalGetCurentFrameIndex();
	return S_OK;
}

ALResult Tr2RenderContextAL::SetTopology( long topology )
{
	sce::Gnm::PrimitiveType primitiveType;
	if( !ConvertTopologyToPrimitiveType( Topology( topology ), primitiveType ) )
	{
		return E_FAIL;
	}
	m_topology = Topology( topology );
	m_backBuffer->context.setPrimitiveType( primitiveType );
	return S_OK;
}

ALResult Tr2RenderContextAL::SetIndices( const Tr2IndexBufferAL& buffer )
{
	if( !buffer.IsValid() )
	{
		if( &buffer == &nullIB )
		{
			m_indices = nullptr;
			return S_OK;
		}
		return E_INVALIDARG;
	}
	m_indices = &buffer;
	return S_OK;
}

Tr2VertexLayoutAL::FetchShader Tr2RenderContextAL::CreateFetchShader( 
	const Tr2VertexLayoutAL& layout, 
	const Tr2ShaderAL& vertexShader, 
	Tr2VertexLayoutAL::FetchShaderType type )
{
	sce::Gnmx::VsShader* vs = vertexShader.m_shader.m_vsShader;
	size_t definitionSize = layout.m_definition.m_items.size();
	std::unique_ptr<uint32_t[]> remapTable( new uint32_t[definitionSize] );
	std::unique_ptr<sce::Gnm::FetchShaderInstancingMode[]> instanceData;
	if( type == Tr2VertexLayoutAL::INSTANCED_DRAW )
	{
		instanceData.reset( new sce::Gnm::FetchShaderInstancingMode[definitionSize] );
	}
	CreateFetchRemapTable( layout.m_definition, vertexShader.m_inputDefinition, remapTable.get(), instanceData.get() );

	sce::Gnm::FetchShaderBuildState fb;
	sce::Gnm::generateVsFetchShaderBuildState( 
		&fb, 
		( const sce::Gnm::VsStageRegisters* )&vs->m_vsStageRegisters, 
		vs->m_numInputSemantics, 
		instanceData.get() );
	fb.m_numInputSemantics = vs->m_numInputSemantics;
	fb.m_inputSemantics = vs->getInputSemanticTable();
	fb.m_numInputUsageSlots = vs->m_common.m_numInputUsageSlots;
	fb.m_inputUsageSlots = vs->getInputUsageSlotTable();
	fb.m_numElementsInRemapTable = layout.m_definition.m_items.size();
	fb.m_semanticsRemapTable = remapTable.get();

	void* fetchShader = (uint32_t *)Tr2MemoryAllocator::AllocateGarlic( fb.m_fetchShaderBufferSize, sce::Gnm::kAlignmentOfFetchShaderInBytes );

	sce::Gnm::generateFetchShader( fetchShader, &fb );

	Tr2VertexLayoutAL::FetchShader result;
	result.shader = fetchShader;
	result.modifier = fb.m_shaderModifier;
	return result;
}

Tr2VertexLayoutAL::FetchShader Tr2RenderContextAL::GetFetchShader( 
	const Tr2VertexLayoutAL& layout, 
	const Tr2ShaderAL& vertexShader, 
	Tr2VertexLayoutAL::FetchShaderType type )
{
	auto key = Tr2VertexLayoutAL::FetchShaderKey( type, vertexShader.m_inputDefinition.hash );
	auto found = layout.m_fetchShaders.find( key );
	Tr2VertexLayoutAL::FetchShader fetchShader;
	if( found == layout.m_fetchShaders.end() )
	{
		fetchShader = CreateFetchShader( layout, vertexShader, type );
		layout.m_fetchShaders[key] = fetchShader;
	}
	else
	{
		fetchShader = found->second;
	}
	return fetchShader;
}

ALResult Tr2RenderContextAL::SetConstants(			
	const Tr2ConstantBufferAL& buffer, 
	Tr2RenderContextEnum::ShaderType constantType, 
	uint32_t registerIndex, 
	uint32_t unusedArgument )
{
	if( !buffer.IsValid() )
	{
		if( &buffer == &nullCB )
		{
			m_backBuffer->context.setConstantBuffers( ConvertShaderTypeToShaderStage( constantType ), registerIndex, 1, nullptr );
			return S_OK;
		}
		else
		{
			return E_INVALIDARG;
		}
	}

	const Tr2ConstantBufferAL* bufferToSet = &buffer;

	if( buffer.GetUsage() & USAGE_LOCK_FREQUENTLY )
	{
		if( constantType < SHADER_TYPE_FIRST || constantType >= SHADER_TYPE_COUNT || registerIndex >= CB_SLOT_COUNT )
		{
			return E_INVALIDARG;
		}

		uint32_t constantDataSize = buffer.GetSize();
		const void* constantData = buffer.m_bufferMirror.get();

		if( constantDataSize == 0 )
		{
			return S_OK;
		}
		SharedConstantBuffer& cb = m_sharedConstantBuffers[constantType * CB_SLOT_COUNT + registerIndex];
		if( cb.mirror.size() < constantDataSize )
		{
			cb.mirror.resize( "Tr2RenderContextAL.m_sharedConstantBuffers.mirror", constantDataSize );
		}
		if( cb.size == constantDataSize )
		{
			if( memcmp( cb.mirror.get(), constantData, constantDataSize ) == 0 )
			{
				CCP_STATS_INC( cbCacheHit );
				return S_OK;
			}
		}

		memcpy( cb.mirror.get(), constantData, constantDataSize );
		cb.size = buffer.GetSize();
		if( !cb.constantBuffer.IsValid() || cb.constantBuffer.GetSize() < constantDataSize )
		{
			CR_RETURN_HR( cb.constantBuffer.Create( constantDataSize, USAGE_CPU_WRITE, nullptr, *this ) );
		}
		void* data;
		CR_RETURN_HR( cb.constantBuffer.Lock( &data, *this ) );
		memcpy( data, constantData, constantDataSize );
		CR_RETURN_HR( cb.constantBuffer.Unlock( *this ) );

		cb.constantBuffer.m_frameUse = buffer.FRAME_USE_NOT_USED_YET;

		if( &buffer != &nullCB )
		{
			buffer.m_frameUse = buffer.FRAME_USE_NOT_USED_YET;
		}

		bufferToSet = &cb.constantBuffer;
	}
	else
	{
		if( constantType >= SHADER_TYPE_FIRST && constantType < SHADER_TYPE_COUNT && registerIndex < CB_SLOT_COUNT )
		{
			SharedConstantBuffer& cb = m_sharedConstantBuffers[constantType * CB_SLOT_COUNT + registerIndex];
			cb.size = 0;
		}
		buffer.m_frameUse = buffer.FRAME_USE_NOT_USED_YET;	// it's been set, so user has the choice again between using Lock or the mirror buffer.
	}

	sce::Gnm::Buffer constantBuffer;
	constantBuffer.initAsConstantBuffer( bufferToSet->m_buffer.GetMemoryForGpuReading( *this ), bufferToSet->GetSize() );
	constantBuffer.setResourceMemoryType( sce::Gnm::kResourceMemoryTypeRO ); // it's a constant buffer, so read-only is OK
	m_backBuffer->context.setConstantBuffers( ConvertShaderTypeToShaderStage( constantType ), registerIndex, 1, &constantBuffer );
	bufferToSet->m_frameUse = Tr2ConstantBufferAL::FRAME_USE_NOT_USED_YET;
	return S_OK;
}

ALResult Tr2RenderContextAL::SetTexture(				
		Tr2RenderContextEnum::ShaderType inputType, 
		uint32_t slot, 
		const Tr2TextureAL& texture, 
		Tr2RenderContextEnum::ColorSpace colorSpace )
{
	if( slot >= TEX_SLOT_COUNT )
	{
		return E_INVALIDARG;
	}
	sce::Gnmx::GfxContext& gfxc = m_backBuffer->context;
	sce::Gnm::Texture tex;
	if( !texture.IsValid() )
	{
		if( &texture != &nullTX )
		{
			return E_INVALIDARG;
		}
		m_boundTextures[slot + TEX_SLOT_COUNT * inputType].texture = nullptr;
		CR_RETURN_HR( m_emptyTexture.InternalGetTextureForGpu( colorSpace, tex, *this ) );
	}
	else
	{
		m_boundTextures[slot + TEX_SLOT_COUNT * inputType].texture = &texture;
		m_boundTextures[slot + TEX_SLOT_COUNT * inputType].colorSpace = colorSpace;
		CR_RETURN_HR( texture.InternalGetTextureForGpu( colorSpace, tex, *this ) );
	}

	gfxc.setTextures( ConvertShaderTypeToShaderStage( inputType ), slot, 1, &tex );
	return S_OK;
}

ALResult Tr2RenderContextAL::SetSamplerState(		
	const Tr2SamplerStateAL& samplerState, 
	Tr2RenderContextEnum::ShaderType inputType, 
	uint32_t registerNumber )
{
	if( !IsValid() )
	{
		return E_INVALIDCALL;
	}
	sce::Gnmx::GfxContext& gfxc = m_backBuffer->context;
	gfxc.setSamplers( ConvertShaderTypeToShaderStage( inputType ), registerNumber, 1, &samplerState.m_sampler );
	return S_OK;
}

ALResult Tr2RenderContextAL::SetRenderState( Tr2RenderContextEnum::RenderState state, uint32_t value )
{
	uint32_t sv[2] = { state, value };
	return SetRenderStates( sv, 1 );
}

ALResult Tr2RenderContextAL::SetRenderStates( const uint32_t* stateValuePairs, uint32_t count )
{
	// count of 0 means look for a null terminator
	for(	uint32_t i = 0; 
		( count != 0 && i != count ) || ( count == 0 && *stateValuePairs ); 
		++i, stateValuePairs += 2 )
	{
		const RenderState state = static_cast<RenderState>( stateValuePairs[0] );
		const uint32_t value    = stateValuePairs[1];

		if( ( uint32_t )state >= RS_MAX_STATE || m_renderStates[state] == value )
		{
			continue;
		}

		m_renderStates[ state ] = value;

		if( RS_CLIPPLANEENABLE != state )
		{
			const uint32_t dirty = m_fragmentOpSettings.SetRenderState( state, value, m_alphaTestParameters );
			if( dirty == m_fragmentOpSettings.HANDLED_BUT_NO_CHANGES )
			{
				continue;
			}
			if( dirty )
			{
				m_dirtyFlag |= dirty;
				continue;
			}
		}

		switch( state )
		{
		case RS_ZENABLE:
			m_depthStencilControl.setDepthEnable( value != 0 );
			m_depthStencilControlDirty = true;
			break;
		case RS_FILLMODE:
			m_primitiveSetup.setPolygonMode( ConvertFillMode( value ), ConvertFillMode( value ) );
			m_primitiveSetupDirty = true;
			break;
		case RS_ZWRITEENABLE:
			m_depthStencilControlDirty = true;
			break;
		case RS_SRCBLEND:
			m_blendControlDirty = true;
			break;
		case RS_DESTBLEND:
			m_blendControlDirty = true;
			break;
		case RS_CULLMODE:
			m_primitiveSetup.setCullFace( ConvertCullFace( value ) );
			m_primitiveSetupDirty = true;
			break;
		case RS_ZFUNC:
			m_depthStencilControlDirty = true;
			break;
		case RS_ALPHABLENDENABLE:
			m_blendControl.setBlendEnable( value != 0 );
			m_blendControlDirty = true;
			break;
		case RS_STENCILENABLE:
			m_depthStencilControl.setStencilEnable( value != 0 );
			m_depthStencilControlDirty = true;
			break;
		case RS_STENCILFAIL:
			m_stencilOpControlDirty = true;
			break;
		case RS_STENCILZFAIL:
			m_stencilOpControlDirty = true;
			break;
		case RS_STENCILPASS:
			m_stencilOpControlDirty = true;
			break;
		case RS_STENCILFUNC:
			m_depthStencilControl.setStencilFunction( ConvertCompareFunc( value ) );
			m_depthStencilControlDirty = true;
			break;
		case RS_STENCILREF:
			m_stencilControl.m_testVal = m_stencilControl.m_opVal = value;
			m_stencilControlDirty = true;
			break;
		case RS_STENCILMASK:
			m_stencilControl.m_mask = value;
			m_stencilControlDirty = true;
			break;
		case RS_STENCILWRITEMASK:
			m_stencilControl.m_writeMask = value;
			m_stencilControlDirty = true;
			break;
		case RS_CLIPPING:
			m_clipControl.setClipEnable( value != 0 );
			m_clipControlDirty = true;
			break;
		case RS_CLIPPLANEENABLE:
			m_clipControl.setUserClipPlanes( value );
			m_clipControlDirty = true;
			break;
		case RS_MULTISAMPLEANTIALIAS:
			m_msaaEnabled = value ? sce::Gnm::kScanModeControlAaEnable : sce::Gnm::kScanModeControlAaDisable;
			m_scanModeControlDirty = true;
			break;
		case RS_MULTISAMPLEMASK:
			m_backBuffer->context.setAaSampleMask( value );
			break;
		case RS_COLORWRITEENABLE:
			m_backBuffer->context.setRenderTargetMask( value );
			break;
		case RS_BLENDOP:
			m_blendControlDirty = true;
			break;
		case RS_SCISSORTESTENABLE:
			m_scissorEnabled = value ? sce::Gnm::kScanModeControlViewportScissorEnable : sce::Gnm::kScanModeControlViewportScissorDisable;
			m_scanModeControlDirty = true;
			break;
		case RS_SLOPESCALEDEPTHBIAS:
			m_polygonOffsetScale = *reinterpret_cast<const float*>( &value );
			m_polygonOffsetDirty = true;
			break;
		case RS_TWOSIDEDSTENCILMODE:
			m_depthStencilControl.setSeparateStencilEnable( value != 0 );
			m_depthStencilControlDirty = true;
			break;
		case RS_CCW_STENCILFAIL:
			m_stencilOpControlDirty = true;
			break;
		case RS_CCW_STENCILZFAIL:
			m_stencilOpControlDirty = true;
			break;
		case RS_CCW_STENCILPASS:
			m_stencilOpControlDirty = true;
			break;
		case RS_CCW_STENCILFUNC:
			m_depthStencilControl.setStencilFunctionBack( ConvertCompareFunc( value ) );
			m_depthStencilControlDirty = true;
			break;
		case RS_SRGBWRITEENABLE:
			m_linearRenderTarget = value != 0;
			break;
		case RS_DEPTHBIAS:
			m_polygonOffset = *reinterpret_cast<const float*>( &value );
			m_polygonOffsetDirty = true;
			break;
		case RS_SEPARATEALPHABLENDENABLE:
			m_blendControl.setSeparateAlphaEnable( value != 0 );
			m_blendControlDirty = true;
			break;
		case RS_SRCBLENDALPHA:
			m_blendControlDirty = true;
			break;
		case RS_DESTBLENDALPHA:
			m_blendControlDirty = true;
			break;
		case RS_BLENDOPALPHA:
			m_blendControlDirty = true;
			break;
		default:
			CCP_AL_LOGWARN( "No Orbis support for SetRenderState( %d, %d )", (uint32_t)state, value );
		}
	}
	return S_OK;
}

ALResult Tr2RenderContextAL::GetRenderState( Tr2RenderContextEnum::RenderState state, uint32_t* value )
{
	if( state < RS_MAX_STATE )
	{
		*value = m_renderStates[state];
		return S_OK;
	}
	return E_INVALIDARG;
}

ALResult Tr2RenderContextAL::SetClipPlane( uint32_t planeIndex, const float* planeEq )
{
	sce::Gnmx::GfxContext& gfxc = m_backBuffer->context;
	gfxc.setUserClipPlane( planeIndex, planeEq[0], planeEq[1], planeEq[2], planeEq[3] );
	return S_OK;
}

ALResult Tr2RenderContextAL::SetScissorRect(
	uint32_t left, 
	uint32_t top, 
	uint32_t right, 
	uint32_t bottom )
{
	sce::Gnmx::GfxContext& gfxc = m_backBuffer->context;
	gfxc.setScreenScissor( left, top, right, bottom );
	return S_OK;
}

ALResult Tr2RenderContextAL::SetViewport( const Tr2Viewport& viewport )
{
	m_viewport = viewport;
	sce::Gnmx::GfxContext& gfxc = m_backBuffer->context;
	gfxc.setupScreenViewport( 
		std::max( 0.f, m_viewport.m_x ), 
		std::max( 0.f, m_viewport.m_y ), 
		m_viewport.m_x + m_viewport.m_width, 
		m_viewport.m_y + m_viewport.m_height, 
		( m_viewport.m_maxZ - m_viewport.m_minZ ),
		m_viewport.m_minZ );
	float invWidth = 1.f / viewport.m_width;
	float invHeight = -1.f / viewport.m_height;
	if( invWidth != m_fragmentOpSettings.m_renderTargetSize[0] ||
		invHeight != m_fragmentOpSettings.m_renderTargetSize[1] )
	{
		m_fragmentOpSettings.m_renderTargetSize[0] = invWidth;
		m_fragmentOpSettings.m_renderTargetSize[1] = invHeight;
		m_dirtyFlag |= Tr2FragmentOpSettings::DIRTY_FRAGMENTOP;
	}
	return S_OK;
}

ALResult Tr2RenderContextAL::GetViewport( Tr2Viewport& viewport )
{
	viewport = m_viewport;
	return S_OK;
}

Tr2RenderContextEnum::PixelFormat Tr2RenderContextAL::GetBackBufferFormat() const
{
	return PIXEL_FORMAT_B8G8R8A8_UNORM;
}

ALResult Tr2RenderContextAL::GetAFRGroupCount( uint32_t& count )
{
	count = 1;
	return S_OK;
}

ALResult Tr2RenderContextAL::PushRenderTarget( uint32_t slot )
{
	CCP_ASSERT( slot < MAX_RENDER_TARGET );
	if( slot >= MAX_RENDER_TARGET )
	{
		return E_INVALIDARG;
	}

	m_stackRT[slot].push( m_boundRenderTarget[slot] );
	return S_OK;
}

ALResult Tr2RenderContextAL::PopRenderTarget( uint32_t slot )
{
	CCP_ASSERT( slot < MAX_RENDER_TARGET );
	if( slot >= MAX_RENDER_TARGET )
	{
		return E_INVALIDARG;
	}
	CCP_ASSERT( !m_stackRT[slot].empty() );
	if( m_stackRT[slot].empty() )
	{
		return E_FAIL;
	}

	if( m_boundRenderTarget[slot] )
	{
		sce::Gnmx::GfxContext& gfxc = m_backBuffer->context;
		sce::Gnm::RenderTarget rt;
		if( SUCCEEDED( m_boundRenderTarget[slot]->InternalGetRenderTargetForGpu( 0, rt, *this ) ) )
		{
			gfxc.waitForGraphicsWrites(
				rt.getBaseAddress256ByteBlocks(), 
				rt.getSizeInBytes() >> 8,
				sce::Gnm::kWaitTargetSlotCb0 << slot, 
				sce::Gnm::kCacheActionWriteBackAndInvalidateL1andL2, 
				sce::Gnm::kExtendedCacheActionFlushAndInvalidateCbCache,
				sce::Gnm::kStallCommandBufferParserDisable );
		}
	}
	SyncRenderTarget( slot );
	m_boundRenderTarget[slot] = m_stackRT[slot].top();
	m_stackRT[slot].pop();
	SetRenderTarget( slot );
	return S_OK;
}

ALResult Tr2RenderContextAL::SetRenderTarget( const Tr2RenderTargetAL& renderTarget, uint32_t slot )
{
	CCP_ASSERT( slot < MAX_RENDER_TARGET );
	if( slot >= MAX_RENDER_TARGET )
	{
		return E_FAIL;
	}
	SyncRenderTarget( slot );
	m_boundRenderTarget[slot] = renderTarget.IsValid() ? &renderTarget : nullptr;
	SetRenderTarget( slot );
	return S_OK;
}

ALResult Tr2RenderContextAL::PushDepthStencil()
{
	m_stackDS.push( m_boundDepthStencil );
	return S_OK;
}

ALResult Tr2RenderContextAL::PopDepthStencil()
{
	CCP_ASSERT( !m_stackDS.empty() );
	if( m_stackDS.empty() )
	{
		return E_FAIL;
	}

	m_boundDepthStencil = m_stackDS.top();
	m_stackDS.pop();
	SetDepthStencil();
	return S_OK;
}

ALResult Tr2RenderContextAL::SetDepthStencil( const Tr2DepthStencilAL& depthStencil )
{
	if( !depthStencil.IsValid() )
	{
		if( &depthStencil != &nullDS )
		{
			return E_INVALIDARG;
		}
	}

	m_boundDepthStencil = &depthStencil;
	SetDepthStencil();
	return S_OK;
}

ALResult Tr2RenderContextAL::SetNumberOfLights( uint32_t numLights ) 
{ 
	m_dirtyFlag |= m_fragmentOpSettings.SetNumberOfLights( numLights );	
	return S_OK;
}

void Tr2RenderContextAL::InternalSyncRenderTarget( Tr2RenderTargetAL& rt )
{
	if( rt == m_backBufferRT )
	{
		if( m_boundRenderTarget[0] == nullptr )
		{
			SyncRenderTarget( 0 );
		}
		return;
	}
	for( uint32_t i = 0; i < MAX_RENDER_TARGET; ++i )
	{
		if( m_boundRenderTarget[i] == &rt )
		{
			SyncRenderTarget( i );
			return;
		}
	}
}

ALResult Tr2RenderContextAL::InternalGenerateMips( Tr2TextureAL& texture )
{
	if( !texture.IsValid() )
	{
		return E_FAIL;
	}
	uint32_t mipCount = texture.GetTrueMipCount();
	sce::Gnmx::GfxContext& gfxc = m_backBuffer->context;

	gfxc.setShaderType( sce::Gnm::kShaderTypeCompute );
	ON_BLOCK_EXIT( [&]{ gfxc.setShaderType( sce::Gnm::kShaderTypeGraphics ); } );

	gfxc.setCsShader( m_downsampleCS.m_shader.m_csShader );

	sce::Gnm::Texture textureDst, textureSrc, tex;
	sce::Gnm::RenderTarget rtDst, rtSrc;

	CR_RETURN_HR( texture.InternalGetTextureForGpu( COLOR_SPACE_LINEAR, tex, *this ) );

	for( uint32_t i = 1; i < mipCount; ++i )
	{
		if( rtDst.initFromTexture( &tex, i ) )
		{
			return E_FAIL;
		}
		if( rtSrc.initFromTexture( &tex, i - 1 ) )
		{
			return E_FAIL;
		}

		textureDst.initFromRenderTarget( &rtDst, false );
		textureDst.setResourceMemoryType( sce::Gnm::kResourceMemoryTypeGC ); // this texture will be bound as an RWTexture, and different CUs may hit different cache lines, so GPU-coherent it is.
		textureSrc.initFromRenderTarget( &rtSrc, false );
		textureSrc.setResourceMemoryType( sce::Gnm::kResourceMemoryTypeRO ); // this texture won't be bound as an RWTexture, so it's OK to declare it read-only.
		gfxc.setTextures( sce::Gnm::kShaderStageCs, 0, 1, &textureSrc );
		gfxc.setRwTextures( sce::Gnm::kShaderStageCs, 0, 1, &textureDst );

		gfxc.dispatch( ( rtDst.getWidth() + 7 ) / 8, ( rtDst.getHeight() + 7 ) / 8, 1 );

		volatile uint64_t* label = (volatile uint64_t*)gfxc.m_dcb.allocateFromCommandBuffer( sizeof( uint64_t ), sce::Gnm::kEmbeddedDataAlignment8 ); // allocate memory from the command buffer
		*label = 0x0; // set the memory to have the val 0
		gfxc.m_dcb.writeAtEndOfShader( sce::Gnm::kEosCsDone, const_cast<uint64_t*>(label), 0x1 ); // tell the CP to write a 1 into the memory only when all compute shaders have finished
		gfxc.m_dcb.waitOnAddress( const_cast<uint64_t*>( label ), 0xffffffff, sce::Gnm::kWaitCompareFuncEqual, 0x1 ); // tell the CP to wait until the memory has the val 1
		gfxc.m_dcb.flushShaderCachesAndWait( sce::Gnm::kCacheActionWriteBackAndInvalidateL1andL2, 0, sce::Gnm::kStallCommandBufferParserDisable ); // tell the CP to flush the L1$ and L2$
	}
	gfxc.setShaderType( sce::Gnm::kShaderTypeGraphics );

	return S_OK;
}

ALResult Tr2RenderContextAL::InternalCopySubresourceRegion( 
	Tr2RenderTargetAL& destination, 
	uint32_t destX, 
	uint32_t destY, 
	Tr2RenderTargetAL& source, 
	uint32_t* ltrb )
{
	RECT srcRect;
	RECT* srcRectPtr = nullptr;
	int32_t srcWidth, srcHeight;
	if( ltrb )
	{
		srcRect.left	= ltrb[0];
		srcRect.top		= ltrb[1];
		srcRect.right	= ltrb[2];
		srcRect.bottom	= ltrb[3];
		
		if( srcRect.bottom > source.GetHeight() )
		{
			srcRect.bottom = source.GetHeight();
		}
		if( srcRect.right > source.GetWidth() )
		{
			srcRect.right = source.GetWidth();
		}
		srcRectPtr = &srcRect;

		srcWidth = srcRect.right - srcRect.left;
		srcHeight = srcRect.bottom - srcRect.top;
	}
	else
	{
		srcWidth = source.GetWidth();
		srcHeight = source.GetHeight();
	}
	
	RECT destRect;	
	destRect.left	= destX;
	destRect.top	= destY;
	destRect.right	= destX + srcWidth;
	destRect.bottom	= destY + srcHeight;
		
	if( destRect.bottom > destination.GetHeight() )
	{
		destRect.bottom = destination.GetHeight();
	}
	if( destRect.right > destination.GetWidth() )
	{
		destRect.right = destination.GetWidth();
	}

	if( destRect.right <= destRect.left || destRect.bottom <= destRect.top )
	{
		return S_OK;
	}

	sce::Gnmx::GfxContext& gfxc = m_backBuffer->context;

	gfxc.setShaderType( sce::Gnm::kShaderTypeCompute );
	ON_BLOCK_EXIT( [&]{ gfxc.setShaderType( sce::Gnm::kShaderTypeGraphics ); } );

	gfxc.setCsShader( m_copyCS.m_shader.m_csShader );

	sce::Gnm::Texture textureDst, textureSrc;

	sce::Gnm::RenderTarget destRT;
	CR_RETURN_HR( destination.InternalGetRenderTargetForGpu( 0, destRT, *this ) );
	textureDst.initFromRenderTarget( &destRT, false );
	textureDst.setResourceMemoryType( sce::Gnm::kResourceMemoryTypeGC ); // this texture will be bound as an RWTexture, and different CUs may hit different cache lines, so GPU-coherent it is.
	sce::Gnm::RenderTarget srcRT;
	CR_RETURN_HR( source.InternalGetRenderTargetForGpu( 0, srcRT, *this ) );
	textureSrc.initFromRenderTarget( &srcRT, false );
	textureSrc.setResourceMemoryType( sce::Gnm::kResourceMemoryTypeRO ); // this texture won't be bound as an RWTexture, so it's OK to declare it read-only.
	gfxc.setTextures( sce::Gnm::kShaderStageCs, 0, 1, &textureSrc );
	gfxc.setRwTextures( sce::Gnm::kShaderStageCs, 0, 1, &textureDst );

	int32_t* constants = (int32_t*)gfxc.allocateFromCommandBuffer( sizeof( int32_t ) * 8, sce::Gnm::kEmbeddedDataAlignment4 );
	constants[0] = destRect.top;
	constants[1] = destRect.left;
	constants[2] = srcRect.left;
	constants[3] = srcRect.top;
	constants[4] = destRect.right - destRect.left;
	constants[5] = destRect.bottom - destRect.top;
	sce::Gnm::Buffer constantBuffer;
	constantBuffer.initAsConstantBuffer( constants, sizeof( int32_t ) * 8 );
	gfxc.setConstantBuffers( sce::Gnm::kShaderStageCs, 0, 1, &constantBuffer );

	gfxc.dispatch( ( destRect.right - destRect.left + 7 ) / 8, ( destRect.bottom - destRect.top + 7 ) / 8, 1 );

	volatile uint64_t* label = (volatile uint64_t*)gfxc.m_dcb.allocateFromCommandBuffer( sizeof( uint64_t ), sce::Gnm::kEmbeddedDataAlignment8 ); // allocate memory from the command buffer
	*label = 0x0; // set the memory to have the val 0
	gfxc.m_dcb.writeAtEndOfShader( sce::Gnm::kEosCsDone, const_cast<uint64_t*>(label), 0x1 ); // tell the CP to write a 1 into the memory only when all compute shaders have finished
	gfxc.m_dcb.waitOnAddress( const_cast<uint64_t*>( label ), 0xffffffff, sce::Gnm::kWaitCompareFuncEqual, 0x1 ); // tell the CP to wait until the memory has the val 1
	gfxc.m_dcb.flushShaderCachesAndWait( sce::Gnm::kCacheActionWriteBackAndInvalidateL1andL2, 0, sce::Gnm::kStallCommandBufferParserDisable ); // tell the CP to flush the L1$ and L2$

	gfxc.setShaderType( sce::Gnm::kShaderTypeGraphics );

	return S_OK;
}

void Tr2RenderContextAL::SyncRenderTarget( uint32_t slot )
{
	const Tr2RenderTargetAL* rt;
	if( m_boundRenderTarget[slot] )
	{
		rt = m_boundRenderTarget[slot];
	}
	else if( slot == 0 )
	{
		rt = &m_backBufferRT;
	}
	else
	{
		return;
	}

	sce::Gnmx::GfxContext& gfxc = m_backBuffer->context;
	sce::Gnm::RenderTarget r;
	if( SUCCEEDED( rt->InternalGetRenderTargetForGpu( 0, r, *this ) ) )
	{
		gfxc.waitForGraphicsWrites(
			r.getBaseAddress256ByteBlocks(), 
			r.getSizeInBytes() >> 8,
			sce::Gnm::kWaitTargetSlotCb0 << slot, 
			sce::Gnm::kCacheActionWriteBackAndInvalidateL1andL2, 
			sce::Gnm::kExtendedCacheActionFlushAndInvalidateCbCache,
			sce::Gnm::kStallCommandBufferParserDisable );
	}
	rt->InternalWriteSync( *this );
}

void Tr2RenderContextAL::SetRenderTarget( uint32_t slot )
{
	sce::Gnmx::GfxContext& gfxc = m_backBuffer->context;
	sce::Gnm::RenderTarget rt;
	if( m_boundRenderTarget[slot] )
	{
		CR_RETURN( m_boundRenderTarget[slot]->InternalGetRenderTargetForGpu( 0, rt, *this ) );
	}
	else if( slot == 0 )
	{
		rt = m_backBuffer->renderTarget;
	}
	else
	{
		gfxc.setRenderTarget( slot, nullptr );
		return;
	}
	gfxc.setRenderTarget( slot, &rt );
	if( slot == 0 )
	{
		SetViewport( Tr2Viewport( rt.getWidth(), rt.getHeight() ) );
		SetScissorRect( 0, 0, rt.getWidth(), rt.getHeight() );

		gfxc.setAaSampleCount( rt.getNumSamples() );
		gfxc.setAaDefaultSampleLocations( rt.getNumSamples() );

		sce::Gnm::DepthEqaaControl eqaaReg;
		eqaaReg.init();
		eqaaReg.setMaxAnchorSamples( rt.getNumSamples() );
		eqaaReg.setAlphaToMaskSamples( rt.getNumSamples() );
		eqaaReg.setStaticAnchorAssociations(true);
		gfxc.setDepthEqaaControl(eqaaReg);
	}
}

void Tr2RenderContextAL::SetDepthStencil()
{
	sce::Gnmx::GfxContext& gfxc = m_backBuffer->context;
	if( m_boundDepthStencil )
	{
		if( m_boundDepthStencil == &nullDS )
		{
			gfxc.setDepthRenderTarget( nullptr );
		}
		else
		{
			gfxc.setDepthRenderTarget( &m_boundDepthStencil->m_depthStencil );
		}
	}
	else
	{
		gfxc.setDepthRenderTarget( &m_backBuffer->depthTarget );
	}
}

ALResult Tr2RenderContextAL::GetRenderTargetSize(	
	uint32_t& width, 
	uint32_t& height, 
	uint32_t slot )
{
	if( m_boundRenderTarget[slot] )
	{
		width = m_boundRenderTarget[slot]->GetWidth();
		height = m_boundRenderTarget[slot]->GetHeight();
		return S_OK;
	}
	else if( slot == 0 )
	{
		width = m_backBuffer->renderTarget.getWidth();
		height = m_backBuffer->renderTarget.getHeight();
		return S_OK;
	}
	else
	{
		return E_FAIL;
	}
}

bool Tr2RenderContextAL::IsSupportedRenderTargetFormat( PixelFormat /*format*/, bool /*withAutoGenMipmap*/ )
{
	return true;	//TODO? 
}

Tr2RenderTargetAL& Tr2RenderContextAL::GetDefaultBackBuffer()
{
	return m_backBufferRT;
}

ALResult Tr2RenderContextAL::ApplyShadowState( Tr2VertexLayoutAL::FetchShaderType fetchType )
{
	sce::Gnmx::GfxContext& gfxc = m_backBuffer->context;
	gfxc.setActiveShaderStages(sce::Gnm::kActiveShaderStagesVsPs);

	size_t count = m_layout->m_definition.m_items.size();
	const unsigned MAX_INPUT_ELEMENTS = sce::Gnm::kSlotCountVertexBuffer;
	sce::Gnm::Buffer buffer[MAX_INPUT_ELEMENTS];
	void* bufferData[MAX_INPUT_ELEMENTS] = { nullptr };

	for( size_t i = 0; i < count; ++i )
	{
		const Tr2VertexDefinition::Item& item = m_layout->m_definition.m_items[i];
		sce::Gnm::DataFormat format;
		if( !ConvertVertexTypeToNaitiveDataFormat( item.m_dataType, format ) )
		{
			return E_FAIL;
		}
		if( !bufferData[item.m_stream] )
		{
			bufferData[item.m_stream] = m_streams[item.m_stream].m_buffer->m_buffer.GetMemoryForGpuReading( *this );
		}
		buffer[i].initAsVertexBuffer( 
			static_cast<uint8_t*>( bufferData[item.m_stream] ) + item.m_offset, 
			format, 
			m_streams[item.m_stream].m_stride,
			m_streams[item.m_stream].m_buffer->GetTotalSizeInBytes() / m_streams[item.m_stream].m_stride );
		buffer[i].setResourceMemoryType( sce::Gnm::kResourceMemoryTypeRO );
	}
	gfxc.setVertexBuffers( sce::Gnm::kShaderStageVs, 0, count, buffer );

	if( m_depthStencilControlDirty )
	{
		m_depthStencilControlDirty = false;
		m_depthStencilControl.setDepthControl( 
			m_renderStates[RS_ZWRITEENABLE] ? sce::Gnm::kDepthControlZWriteEnable : sce::Gnm::kDepthControlZWriteDisable, 
			ConvertCompareFunc( m_renderStates[RS_ZFUNC] ) );
		gfxc.setDepthStencilControl( m_depthStencilControl );
	}
	if( m_primitiveSetupDirty )
	{
		m_primitiveSetupDirty = false;
		gfxc.setPrimitiveSetup( m_primitiveSetup );
	}
	if( m_blendControlDirty )
	{
		m_blendControlDirty = false;
		m_blendControl.setColorEquation( 
			ConvertBlendMultiplier( m_renderStates[RS_SRCBLEND] ), 
			ConvertBlendOp( m_renderStates[RS_BLENDOP] ), 
			ConvertBlendMultiplier( m_renderStates[RS_DESTBLEND] ) );
		m_blendControl.setAlphaEquation( 
			ConvertBlendMultiplier( m_renderStates[RS_SRCBLENDALPHA] ), 
			ConvertBlendOp( m_renderStates[RS_BLENDOPALPHA] ), 
			ConvertBlendMultiplier( m_renderStates[RS_DESTBLENDALPHA] ) );
		// TODO: mrt
		gfxc.setBlendControl( 0, m_blendControl );
	}
	if( m_stencilOpControlDirty )
	{
		m_stencilOpControlDirty = false;
		m_stencilOpControl.setStencilOps( 
			ConvertStencilOp( m_renderStates[RS_STENCILFAIL] ), 
			ConvertStencilOp( m_renderStates[RS_STENCILPASS] ), 
			ConvertStencilOp( m_renderStates[RS_STENCILZFAIL] ) );
		m_stencilOpControl.setStencilOpsBack( 
			ConvertStencilOp( m_renderStates[RS_CCW_STENCILFAIL] ), 
			ConvertStencilOp( m_renderStates[RS_CCW_STENCILPASS] ), 
			ConvertStencilOp( m_renderStates[RS_CCW_STENCILZFAIL] ) );
		gfxc.setStencilOpControl( m_stencilOpControl );
	}
	if( m_stencilControlDirty )
	{
		m_stencilControlDirty = false;
		gfxc.setStencil( m_stencilControl );
	}
	if( m_clipControlDirty )
	{
		m_clipControlDirty = false;
		gfxc.setClipControl( m_clipControl );
	}
	if( m_scanModeControlDirty )
	{
		m_scanModeControlDirty = false;
		gfxc.setScanModeControl( m_msaaEnabled, m_scissorEnabled, sce::Gnm::kScanModeControlLineStippleDisable );
	}
	if( m_polygonOffsetDirty )
	{
		m_polygonOffsetDirty = false;
		gfxc.setPolygonOffsetFront( m_polygonOffsetScale, m_polygonOffset );
		gfxc.setPolygonOffsetBack( m_polygonOffsetScale, m_polygonOffset );
	}
	if( m_dirtyFlag )
	{
		if( m_dirtyFlag & Tr2FragmentOpSettings::DIRTY_FRAGMENTOP )
		{
			m_fragmentOpSettings.UpdateContents( m_alphaTestParameters );

			if( !m_fragmentOpBuffer.IsValid() )
			{
				auto& renderContext = Tr2RenderContextAL::GetPrimaryRenderContext();
				CR_RETURN_HR( 
					m_fragmentOpBuffer.Create(	sizeof( m_fragmentOpSettings ), 
												USAGE_CPU_WRITE,
												nullptr,
												renderContext ) );
			}

			{
				void * mapped = nullptr;
				CR_RETURN_HR( m_fragmentOpBuffer.Lock( &mapped, *this ) );

				if( !mapped )
				{
					return E_FAIL;
				}
				memcpy( mapped, &m_fragmentOpSettings, sizeof( m_fragmentOpSettings ) );
				m_fragmentOpBuffer.Unlock( *this );
			}

			{
				SetConstants( m_fragmentOpBuffer, PIXEL_SHADER,  CONSTANT_BUFFER_FOR_FRAGMENT_OP_EMULATION );
				SetConstants( m_fragmentOpBuffer, VERTEX_SHADER, CONSTANT_BUFFER_FOR_FRAGMENT_OP_EMULATION );
			}
		}

		if( ( m_dirtyFlag & Tr2FragmentOpSettings::DIRTY_PATCH_PS ) && m_boundShaders[PIXEL_SHADER] )
		{
			if( m_alphaTestParameters.m_alphaTestEnabled && 
				m_alphaTestParameters.m_alphaTestFunc != CMP_ALWAYS )
			{
				gfxc.setPsShader( m_boundShaders[PIXEL_SHADER]->m_pachedShader.m_psShader );
			}
			else
			{
				gfxc.setPsShader( m_boundShaders[PIXEL_SHADER]->m_shader.m_psShader );
			}
		}

		if( ( m_dirtyFlag & Tr2FragmentOpSettings::DIRTY_PATCH_VS ) && m_boundShaders[VERTEX_SHADER] )
		{
			auto fetchShader = GetFetchShader( *m_layout, *m_boundShaders[VERTEX_SHADER], fetchType );
			gfxc.setVsShader( m_boundShaders[VERTEX_SHADER]->m_shader.m_vsShader, fetchShader.modifier, fetchShader.shader );
		}

		m_dirtyFlag = 0;
	}

	for( uint32_t i = 0; i <= PIXEL_SHADER; ++i )
	{
		for( uint32_t j = 0; j < TEX_SLOT_COUNT; ++j )
		{
			if( m_boundTextures[j + i * TEX_SLOT_COUNT].texture )
			{
				sce::Gnm::Texture tex;
				if( SUCCEEDED( m_boundTextures[j + i * TEX_SLOT_COUNT].texture->InternalGetTextureForGpu( m_boundTextures[j + i * TEX_SLOT_COUNT].colorSpace, tex, *this ) ) )
				{
					gfxc.setTextures( ConvertShaderTypeToShaderStage( ShaderType( i ) ), j, 1, &tex );
				}
			}
		}
	}

	return S_OK;
}

void Tr2RenderContextAL::ResizeDrawPrimitiveIndexBuffer( uint32_t vertexCount )
{
	if( m_drawPrimitiveIndexCount < vertexCount )
	{
		Tr2MemoryAllocator::Release( m_drawPrimitiveIndexBuffer );
		m_drawPrimitiveIndexBuffer = reinterpret_cast<uint16_t*>( Tr2MemoryAllocator::AllocateGarlic( vertexCount * sizeof( uint16_t ), sce::Gnm::kAlignmentOfBufferInBytes ) );
		for( uint32_t i = 0; i < vertexCount; ++i )
		{
			m_drawPrimitiveIndexBuffer[i] = uint16_t( i );
		}
		m_drawPrimitiveIndexCount = vertexCount;
	}
}

ALResult Tr2RenderContextAL::DrawPrimitive(	uint32_t startVertex, uint32_t primitiveCount )
{
	if( startVertex )
	{
		CCP_AL_LOGERR( "Tr2RenderContextAL::DrawPrimitive: does not support for non-zero startVertex" );
		return E_INVALIDARG;
	}
	CR_RETURN_HR( ApplyShadowState( Tr2VertexLayoutAL::SINGLE_DRAW ) );

	const uint32_t vertexCount = ComputeVertexCount( primitiveCount );
	ResizeDrawPrimitiveIndexBuffer( vertexCount );

	sce::Gnmx::GfxContext& gfxc = m_backBuffer->context;
	gfxc.setIndexSize( sce::Gnm::kIndexSize16 );
	gfxc.drawIndex( vertexCount, m_drawPrimitiveIndexBuffer );

	return S_OK;
}

ALResult Tr2RenderContextAL::DrawIndexedPrimitive(	
		uint32_t numVertices, 
		uint32_t startIndex, 
		uint32_t primitiveCount, 
		uint32_t minimumIndex )
{
	if( !m_indices )
	{
		return E_INVALIDCALL;
	}
	CR_RETURN_HR( ApplyShadowState( Tr2VertexLayoutAL::SINGLE_DRAW ) );

	const uint32_t indexCount = ComputeVertexCount( primitiveCount );

	sce::Gnmx::GfxContext& gfxc = m_backBuffer->context;
	gfxc.setIndexSize( m_indices->Is16Bit() ? sce::Gnm::kIndexSize16 : sce::Gnm::kIndexSize32 );
	const uint8_t* indexBuffer = reinterpret_cast<const uint8_t*>( m_indices->m_buffer.GetMemoryForGpuReading( *this ) );
	uint32_t bytesPerIndex;
	if( m_indices->GetIBBitcount() == IB_16BIT )
	{
		bytesPerIndex = 2;
	}
	else
	{
		bytesPerIndex = 4;
	}
	indexBuffer += startIndex * bytesPerIndex;
	gfxc.drawIndex( indexCount, indexBuffer );

	return S_OK;
}

ALResult Tr2RenderContextAL::DrawIndexedInstanced(	
	uint32_t numVertices, 
	uint32_t startIndex, 
	uint32_t primitiveCount, 
	uint32_t numInstances )
{
	if( !m_indices )
	{
		return E_INVALIDCALL;
	}
	CR_RETURN_HR( ApplyShadowState( Tr2VertexLayoutAL::INSTANCED_DRAW ) );

	const uint32_t indexCount = ComputeVertexCount( primitiveCount );

	sce::Gnmx::GfxContext& gfxc = m_backBuffer->context;
	gfxc.setIndexSize( sce::Gnm::kIndexSize16 );
	const uint8_t* indexBuffer = reinterpret_cast<const uint8_t*>( m_indices->m_buffer.GetMemoryForGpuReading( *this ) );
	uint32_t bytesPerIndex;
	if( m_indices->GetIBBitcount() == IB_16BIT )
	{
		bytesPerIndex = 2;
	}
	else
	{
		bytesPerIndex = 4;
	}
	indexBuffer += startIndex * bytesPerIndex;

	gfxc.setNumInstances( numInstances );
	gfxc.drawIndex( indexCount, indexBuffer );
	gfxc.setNumInstances( 1 );

	return S_OK;
}

ALResult Tr2RenderContextAL::DrawIndexedPrimitiveUP( 
	uint32_t numVertices, 
	uint32_t primitiveCount, 
	const uint32_t* indexData, 
	const void* vertexStreamZeroData, 
	uint32_t vertexStreamZeroStride )
{
	return m_drawUP.DrawIndexedPrimitiveUP(	numVertices,
											primitiveCount,
											indexData,
											vertexStreamZeroData,
											vertexStreamZeroStride,
											*this );
}

ALResult Tr2RenderContextAL::DrawIndexedPrimitiveUP( 
	uint32_t numVertices, 
	uint32_t primitiveCount, 
	const uint16_t* indexData, 
	const void* vertexStreamZeroData, 
	uint32_t vertexStreamZeroStride)
{
	return m_drawUP.DrawIndexedPrimitiveUP(	numVertices,
											primitiveCount,
											indexData,
											vertexStreamZeroData,
											vertexStreamZeroStride,
											*this );
}

ALResult Tr2RenderContextAL::DrawPrimitiveUP(		
	uint32_t primitiveCount, 
	const void* vertexStreamZeroData, 
	uint32_t vertexStreamZeroStride )
{
	return m_drawUP.DrawPrimitiveUP(	primitiveCount,
										vertexStreamZeroData,
										vertexStreamZeroStride,
										*this );
}

bool Tr2RenderContextAL::ConvertPixelFormatToDataFormat( Tr2RenderContextEnum::PixelFormat format, sce::Gnm::DataFormat& result )
{
	switch( format )
	{
	case PIXEL_FORMAT_R32G32B32A32_FLOAT:
		result = sce::Gnm::kDataFormatR32G32B32A32Float;
		return true;
	case PIXEL_FORMAT_R32G32B32A32_UINT:
		result = sce::Gnm::kDataFormatR32G32B32A32Uint;
		return true;
	case PIXEL_FORMAT_R32G32B32A32_SINT:
		result = sce::Gnm::kDataFormatR32G32B32A32Sint;
		return true;
	case PIXEL_FORMAT_R32G32B32_FLOAT:
		result = sce::Gnm::kDataFormatR32G32B32Float;
		return true;
	case PIXEL_FORMAT_R32G32B32_UINT:
		result = sce::Gnm::kDataFormatR32G32B32Uint;
		return true;
	case PIXEL_FORMAT_R32G32B32_SINT:
		result = sce::Gnm::kDataFormatR32G32B32Sint;
		return true;
	case PIXEL_FORMAT_R16G16B16A16_FLOAT:
		result = sce::Gnm::kDataFormatR16G16B16A16Float;
		return true;
	case PIXEL_FORMAT_R16G16B16A16_UNORM:
		result = sce::Gnm::kDataFormatR16G16B16A16Unorm;
		return true;
	case PIXEL_FORMAT_R16G16B16A16_UINT:
		result = sce::Gnm::kDataFormatR16G16B16A16Uint;
		return true;
	case PIXEL_FORMAT_R16G16B16A16_SNORM:
		result = sce::Gnm::kDataFormatR16G16B16A16Snorm;
		return true;
	case PIXEL_FORMAT_R16G16B16A16_SINT:
		result = sce::Gnm::kDataFormatR16G16B16A16Sint;
		return true;
	case PIXEL_FORMAT_R32G32_FLOAT:
		result = sce::Gnm::kDataFormatR32G32Float;
		return true;
	case PIXEL_FORMAT_R32G32_UINT:
		result = sce::Gnm::kDataFormatR32G32Uint;
		return true;
	case PIXEL_FORMAT_R32G32_SINT:
		result = sce::Gnm::kDataFormatR32G32Sint;
		return true;
	case PIXEL_FORMAT_R10G10B10A2_UNORM:
		result = sce::Gnm::kDataFormatR10G10B10A2Unorm;
		return true;
	case PIXEL_FORMAT_R10G10B10A2_UINT:
		result = sce::Gnm::kDataFormatR10G10B10A2Uint;
		return true;
	case PIXEL_FORMAT_R11G11B10_FLOAT:
		result = sce::Gnm::kDataFormatR11G11B10Float;
		return true;
	case PIXEL_FORMAT_R8G8B8A8_UNORM:
		result = sce::Gnm::kDataFormatR8G8B8A8Unorm;
		return true;
	case PIXEL_FORMAT_R8G8B8A8_UNORM_SRGB:
		result = sce::Gnm::kDataFormatR8G8B8A8UnormSrgb;
		return true;
	case PIXEL_FORMAT_R8G8B8A8_UINT:
		result = sce::Gnm::kDataFormatR8G8B8A8Uint;
		return true;
	case PIXEL_FORMAT_R8G8B8A8_SNORM:
		result = sce::Gnm::kDataFormatR8G8B8A8Snorm;
		return true;
	case PIXEL_FORMAT_R8G8B8A8_SINT:
		result = sce::Gnm::kDataFormatR8G8B8A8Sint;
		return true;
	case PIXEL_FORMAT_R16G16_FLOAT:
		result = sce::Gnm::kDataFormatR16G16Float;
		return true;
	case PIXEL_FORMAT_R16G16_UNORM:
		result = sce::Gnm::kDataFormatR16G16Unorm;
		return true;
	case PIXEL_FORMAT_R16G16_UINT:
		result = sce::Gnm::kDataFormatR16G16Uint;
		return true;
	case PIXEL_FORMAT_R16G16_SNORM:
		result = sce::Gnm::kDataFormatR16G16B16A16Snorm;
		return true;
	case PIXEL_FORMAT_R16G16_SINT:
		result = sce::Gnm::kDataFormatR16G16B16A16Sint;
		return true;
	case PIXEL_FORMAT_D32_FLOAT:
	case PIXEL_FORMAT_R32_FLOAT:
		result = sce::Gnm::kDataFormatR32Float;
		return true;
	case PIXEL_FORMAT_R32_UINT:
		result = sce::Gnm::kDataFormatR32Uint;
		return true;
	case PIXEL_FORMAT_R32_SINT:
		result = sce::Gnm::kDataFormatR32Sint;
		return true;
	case PIXEL_FORMAT_R8G8_UNORM:
		result = sce::Gnm::kDataFormatR8G8Unorm;
		return true;
	case PIXEL_FORMAT_R8G8_UINT:
		result = sce::Gnm::kDataFormatR8G8Uint;
		return true;
	case PIXEL_FORMAT_R8G8_SNORM:
		result = sce::Gnm::kDataFormatR8G8Snorm;
		return true;
	case PIXEL_FORMAT_R8G8_SINT:
		result = sce::Gnm::kDataFormatR8G8Sint;
		return true;
	case PIXEL_FORMAT_R16_FLOAT:
		result = sce::Gnm::kDataFormatR16Float;
		return true;
	case PIXEL_FORMAT_D16_UNORM:
	case PIXEL_FORMAT_R16_UNORM:
		result = sce::Gnm::kDataFormatR16Unorm;
		return true;
	case PIXEL_FORMAT_R16_UINT:
		result = sce::Gnm::kDataFormatR16Uint;
		return true;
	case PIXEL_FORMAT_R16_SNORM:
		result = sce::Gnm::kDataFormatR16Snorm;
		return true;
	case PIXEL_FORMAT_R16_SINT:
		result = sce::Gnm::kDataFormatR16Sint;
		return true;
	case PIXEL_FORMAT_R8_UNORM:
		result = sce::Gnm::kDataFormatR8Unorm;
		return true;
	case PIXEL_FORMAT_R8_UINT:
		result = sce::Gnm::kDataFormatR8Uint;
		return true;
	case PIXEL_FORMAT_R8_SNORM:
		result = sce::Gnm::kDataFormatR8Snorm;
		return true;
	case PIXEL_FORMAT_R8_SINT:
		result = sce::Gnm::kDataFormatR8Sint;
		return true;
	case PIXEL_FORMAT_A8_UNORM:
		result = sce::Gnm::kDataFormatA8Unorm;
		return true;
	case PIXEL_FORMAT_BC1_UNORM:
		result = sce::Gnm::kDataFormatBc1Unorm;
		return true;
	case PIXEL_FORMAT_BC1_UNORM_SRGB:
		result = sce::Gnm::kDataFormatBc1UnormSrgb;
		return true;
	case PIXEL_FORMAT_BC2_UNORM:
		result = sce::Gnm::kDataFormatBc2Unorm;
		return true;
	case PIXEL_FORMAT_BC2_UNORM_SRGB:
		result = sce::Gnm::kDataFormatBc1UnormSrgb;
		return true;
	case PIXEL_FORMAT_BC3_UNORM:
		result = sce::Gnm::kDataFormatBc3Unorm;
		return true;
	case PIXEL_FORMAT_BC3_UNORM_SRGB:
		result = sce::Gnm::kDataFormatBc3UnormSrgb;
		return true;
	case PIXEL_FORMAT_BC4_UNORM:
		result = sce::Gnm::kDataFormatBc4Unorm;
		return true;
	case PIXEL_FORMAT_BC4_SNORM:
		result = sce::Gnm::kDataFormatBc4Snorm;
		return true;
	case PIXEL_FORMAT_BC5_UNORM:
		result = sce::Gnm::kDataFormatBc5Unorm;
		return true;
	case PIXEL_FORMAT_BC5_SNORM:
		result = sce::Gnm::kDataFormatBc5Snorm;
		return true;
	case PIXEL_FORMAT_B5G6R5_UNORM:
		result = sce::Gnm::kDataFormatB5G6R5Unorm;
		return true;
	case PIXEL_FORMAT_B5G5R5A1_UNORM:
		result = sce::Gnm::kDataFormatB5G5R5A1Unorm;
		return true;
	case PIXEL_FORMAT_B8G8R8A8_UNORM:
		result = sce::Gnm::kDataFormatB8G8R8A8Unorm;
		return true;
	case PIXEL_FORMAT_B8G8R8X8_UNORM:
		result = sce::Gnm::kDataFormatB8G8R8X8Unorm;
		return true;
	case PIXEL_FORMAT_B8G8R8A8_UNORM_SRGB:
		result = sce::Gnm::kDataFormatB8G8R8A8UnormSrgb;
		return true;
	case PIXEL_FORMAT_B8G8R8X8_UNORM_SRGB:
		result = sce::Gnm::kDataFormatB8G8R8X8UnormSrgb;
		return true;
	case PIXEL_FORMAT_BC6H_UF16:
		result = sce::Gnm::kDataFormatBc6Uf16;
		return true;
	case PIXEL_FORMAT_BC6H_SF16:
		result = sce::Gnm::kDataFormatBc6Sf16;
		return true;
	case PIXEL_FORMAT_BC7_UNORM:
		result = sce::Gnm::kDataFormatBc7Unorm;
		return true;
	case PIXEL_FORMAT_BC7_UNORM_SRGB:
		result = sce::Gnm::kDataFormatBc7UnormSrgb;
		return true;
	default:
		return false;
	}
}

uint32_t Tr2RenderContextAL::InternalGetCurentFrameIndex() const
{
	return m_frameIndex;
}

uint32_t Tr2RenderContextAL::InternalGetMaxFrameLatency() const
{
	return 0;
}

void Tr2RenderContextAL::InternalDelayDelete( uint32_t frameUsed, void* pointer )
{
	Tr2PrimaryRenderContextAL* renderContext = ::GetPrimaryRenderContext();
	if( renderContext == nullptr )
	{
		Tr2MemoryAllocator::Release( pointer );
		return;
	}
	uint32_t distance = std::min( frameUsed - renderContext->m_frameIndex, renderContext->m_frameIndex - frameUsed );
	if( distance > renderContext->InternalGetMaxFrameLatency() )
	{
		Tr2MemoryAllocator::Release( pointer );
	}
	else
	{
		DelayDeleteResource resource;
		resource.frame = frameUsed;
		resource.pointer = pointer;
		renderContext->m_delayDeletes.push_back( resource );
	}
}

void Tr2RenderContextAL::InternalSyncToGpu()
{
	sce::Gnmx::GfxContext& gfxc = m_backBuffer->context;

	volatile uint64_t* label = (volatile uint64_t*)gfxc.m_dcb.allocateFromCommandBuffer( sizeof( uint64_t ), sce::Gnm::kEmbeddedDataAlignment8 ); // allocate memory from the command buffer
	*label = 0x0; // set the memory to have the val 0
	gfxc.writeImmediateAtEndOfPipe( 
		sce::Gnm::kEopFlushCbDbCaches, 
		const_cast<uint64_t*>( label ), 
		1, 
		sce::Gnm::kCacheActionNone );

	volatile uint32_t wait = 0;
	while( *label )
	{
		wait++;
	}
}

uint32_t Tr2RenderContextAL::InternalGetBorderColorIndex( const float* color )
{
	for( uint32_t i = 0; i < m_borderColorCount; ++i )
	{
		if( m_borderColorColors[i * 4] == color[0] &&
			m_borderColorColors[i * 4 + 1] == color[1] &&
			m_borderColorColors[i * 4 + 2] == color[2] &&
			m_borderColorColors[i * 4 + 3] == color[3] )
		{
			return i;
		}
	}
	if( m_borderColorCount < MAX_BORDER_COLORS )
	{
		m_borderColorColors[m_borderColorCount * 4] = color[0];
		m_borderColorColors[m_borderColorCount * 4 + 1] = color[1];
		m_borderColorColors[m_borderColorCount * 4 + 2] = color[2];
		m_borderColorColors[m_borderColorCount * 4 + 3] = color[3];
		return m_borderColorCount++;
	}
	return 0;
}

ALResult Tr2RenderContextAL::InternalResolveRT( Tr2RenderTargetAL* destinationRT, Tr2RenderTargetAL* sourceRT )
{
	sce::Gnmx::GfxContext& gfxc = m_backBuffer->context;

	sce::Gnm::RenderTarget destination;
	CR_RETURN_HR( destinationRT->InternalGetRenderTargetForGpu( 0, destination, *this ) );
	sce::Gnm::RenderTarget source;
	CR_RETURN_HR( sourceRT->InternalGetRenderTargetForGpu( 0, source, *this ) );

	gfxc.setAaSampleCount( source.getNumSamples() );
	gfxc.setAaDefaultSampleLocations( source.getNumSamples() );
	
	if( sourceRT == &m_backBufferRT )
	{
		if( m_boundRenderTarget[0] == nullptr )
		{
			gfxc.waitForGraphicsWrites(
				source.getBaseAddress256ByteBlocks(), 
				source.getSizeInBytes() >> 8,
				sce::Gnm::kWaitTargetSlotCb0, 
				sce::Gnm::kCacheActionWriteBackAndInvalidateL1andL2, 
				sce::Gnm::kExtendedCacheActionFlushAndInvalidateCbCache,
				sce::Gnm::kStallCommandBufferParserDisable );
		}
	}
	for( uint32_t i = 0; i < MAX_RENDER_TARGET; ++i )
	{
		if( m_boundRenderTarget[i] == sourceRT )
		{
			gfxc.waitForGraphicsWrites(
				source.getBaseAddress256ByteBlocks(), 
				source.getSizeInBytes() >> 8,
				sce::Gnm::kWaitTargetSlotCb0 << i, 
				sce::Gnm::kCacheActionWriteBackAndInvalidateL1andL2, 
				sce::Gnm::kExtendedCacheActionFlushAndInvalidateCbCache,
				sce::Gnm::kStallCommandBufferParserDisable );
			break;
		}
	}

	if( sourceRT->GetMsaaType() < 2 )
	{
		uint32_t size = source.getSizeInBytes();
		gfxc.copyData( destination.getBaseAddress(), source.getBaseAddress(), size, sce::Gnm::kDmaDataBlockingEnable );
	}
	else
	{
		CCP_ASSERT( TileModesAreSafeForMsaaResolve( destination.getTileMode(), source.getTileMode() ) );
		CCP_ASSERT( !source.getFmaskCompressionEnable() || ( source.getFmaskAddress() != NULL ) );

		gfxc.setScanModeControl( sce::Gnm::kScanModeControlAaEnable, sce::Gnm::kScanModeControlViewportScissorDisable, sce::Gnm::kScanModeControlLineStippleDisable );

		gfxc.triggerEvent( sce::Gnm::kEventTypeFlushAndInvalidateCbMeta );
	
		gfxc.setRenderTarget( 0, &source );
		gfxc.setRenderTarget( 1, &destination );
		gfxc.setDepthRenderTarget( nullptr );
		gfxc.setPsShader( m_dummyPS.m_shader.m_psShader );

		sce::Gnm::BlendControl blendControl;
		blendControl.init();
		blendControl.setBlendEnable(false);
		gfxc.setBlendControl( 0, blendControl );
		gfxc.setRenderTargetMask( 0x0000000F );

		sce::Gnm::DepthStencilControl dsc;
		dsc.init();
		dsc.setDepthControl( sce::Gnm::kDepthControlZWriteDisable, sce::Gnm::kCompareFuncAlways );
		dsc.setDepthEnable( false );
		gfxc.setDepthStencilControl( dsc );
		gfxc.setupScreenViewport( 0, 0, destination.getWidth(), destination.getHeight(), 1.0f, 0.0f );
		gfxc.setDepthStencilDisable();

		gfxc.setVsShader( m_quadVS.m_shader.m_vsShader, 0, nullptr );
		gfxc.setPrimitiveType( sce::Gnm::kPrimitiveTypeRectList );

		gfxc.setCbControl( sce::Gnm::kCbModeResolve, sce::Gnm::kRasterOpSrcCopy );
		gfxc.drawIndexAuto(3);
		gfxc.setCbControl( sce::Gnm::kCbModeNormal, sce::Gnm::kRasterOpSrcCopy );

		gfxc.triggerEvent( sce::Gnm::kEventTypeFlushAndInvalidateCbMeta );

		gfxc.waitForGraphicsWrites(
			destination.getBaseAddress256ByteBlocks(), 
			destination.getSizeInBytes() >> 8,
			sce::Gnm::kWaitTargetSlotCb1, 
			sce::Gnm::kCacheActionWriteBackAndInvalidateL1andL2, 
			sce::Gnm::kExtendedCacheActionFlushAndInvalidateCbCache,
			sce::Gnm::kStallCommandBufferParserDisable );

		SetRenderTarget( 0 );
		SetRenderTarget( 1 );
		SetDepthStencil();
		m_dirtyFlag |= Tr2FragmentOpSettings::DIRTY_PATCH_VS | Tr2FragmentOpSettings::DIRTY_PATCH_PS;
	}

	destinationRT->InternalWriteSync( *this );

	return S_OK;
}

sce::Gnmx::GfxContext& Tr2RenderContextAL::InternalGetGfxContext()
{
	return m_backBuffer->context;
}

uint32_t Tr2RenderContextAL::ComputeVertexCount( uint32_t primitiveCount ) const
{
	switch( m_topology )
	{
	case TOP_TRIANGLES:
		return primitiveCount * 3;

	case TOP_TRIANGLE_STRIP:
		return primitiveCount + 2;

	case TOP_LINES:
		return primitiveCount * 2;

	case TOP_LINE_STRIP:
		return primitiveCount + 1;

	case TOP_POINTS:
	case TOP_TRIANGLE_FAN:
		return primitiveCount;
			
	
	default:
		CCP_ASSERT( false && "Unsupported topology" );
		return 0;
	}
}

#endif