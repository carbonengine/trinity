#include "StdAfx.h"

#if( TRINITY_PLATFORM == TRINITY_METAL )

#include "Tr2SamplerStateALMetal.h"
#include "Tr2RenderContextMetal.h"
#include "MetalContext.h"
#include "ALLog.h"

extern uint32_t g_forceAnisotropy;

namespace
{
	bool MapMinMagFilter( const Tr2RenderContextEnum::TextureFilter filter, MTLSamplerMinMagFilter& result )
	{
		using namespace Tr2RenderContextEnum;

		switch( filter )
		{
		case TF_POINT:
			result = MTLSamplerMinMagFilterNearest;
			break;
		case TF_LINEAR:
		case TF_ANISOTROPIC:
			result = MTLSamplerMinMagFilterLinear;
			break;
		default:
			CCP_AL_LOGERR( "Cannot map sampler min-mag filter (%d) to Metal enum.", filter );
			return false;
		}
		// TODO: TF_COMPARISON is unused?

		return true;
	}

	bool MapMipFilter( const Tr2RenderContextEnum::TextureFilter filter, MTLSamplerMipFilter& result )
	{
		using namespace Tr2RenderContextEnum;

		switch( filter )
		{
		case TF_NONE:
			result = MTLSamplerMipFilterNotMipmapped;
			break;
		case TF_POINT:
			result = MTLSamplerMipFilterNearest;
			break;
		case TF_LINEAR:
		case TF_ANISOTROPIC:
			result = MTLSamplerMipFilterLinear;
			break;
		default:
			CCP_AL_LOGERR( "Cannot map sampler mip filter (%d) to Metal enum.", filter );
			return false;
		}

		return true;
	}

	bool MapAddressMode( const Tr2RenderContextEnum::TextureAddressMode mode, MTLSamplerAddressMode& result )
	{
		using namespace Tr2RenderContextEnum;

		switch( mode )
		{
		case TA_WRAP:
			result = MTLSamplerAddressModeRepeat;
			break;
		case TA_MIRROR:
			result = MTLSamplerAddressModeMirrorRepeat;
			break;
		case TA_CLAMP:
			result = MTLSamplerAddressModeClampToEdge;
			break;
		case TA_BORDER:
			result = MTLSamplerAddressModeClampToBorderColor;
			break;
		case TA_MIRROR_ONCE:
			result = MTLSamplerAddressModeMirrorClampToEdge;
			break;
		default:
			CCP_AL_LOGERR( "Cannot map sampler address mode (%d) to Metal enum.", mode );
			return false;
		}

		return true;
	}

	bool MapCompareFunction( const Tr2RenderContextEnum::CompareFunc func, MTLCompareFunction& result )
	{
		using namespace Tr2RenderContextEnum;
		
		// Default value set by ShaderCompiler, basically it means the function
		// wasn't initialized in the shader. So we pick a sensible default mapping.
        // Can't put this in the switch statement as it causes compile errors in Xcode as 0 is not a valid enum.
		if (func == 0) {
			result = MTLCompareFunctionAlways;
			return true;
		}

		switch( func )
		{
		case CMP_NEVER:
			result = MTLCompareFunctionNever;
			break;
		case CMP_LESS:
			result = MTLCompareFunctionLess;
			break;
		case CMP_EQUAL:
			result = MTLCompareFunctionEqual;
			break;
		case CMP_LESSEQUAL:
			result = MTLCompareFunctionLessEqual;
			break;
		case CMP_GREATER:
			result = MTLCompareFunctionGreater;
			break;
		case CMP_NOTEQUAL:
			result = MTLCompareFunctionNotEqual;
			break;
		case CMP_GREATEREQUAL:
			result = MTLCompareFunctionGreaterEqual;
			break;
		case CMP_ALWAYS:
			result = MTLCompareFunctionAlways;
			break;
		default:
			CCP_AL_LOGERR( "Cannot map sampler compare function (%d) to Metal enum.", func );
			return false;
		}

		return true;
	}

	bool MapBorderColor( const float borderColor[4], MTLSamplerBorderColor result )
	{
		if( borderColor[0] == 0.f && borderColor[1] == 0.f && borderColor[2] == 0.f )
		{
			if( borderColor[3] == 0.f )
			{
				result = MTLSamplerBorderColorTransparentBlack;
				return true;
			}
			else if( borderColor[3] == 1.f )
			{
				result = MTLSamplerBorderColorOpaqueBlack;
				return true;
			}
		}
		else if( borderColor[0] == 1.f && borderColor[1] == 1.f && borderColor[2] == 1.f && borderColor[3] == 1.f )
		{
			result = MTLSamplerBorderColorOpaqueWhite;
			return true;
		}

		CCP_AL_LOGERR( "Cannot map sampler border color (%.f, %.f, %.f, %.f) to Metal enum.", borderColor[0], borderColor[1], borderColor[2], borderColor[3] );
		return false;
	}
}

namespace TrinityALImpl
{
	Tr2SamplerStateAL::Tr2SamplerStateAL()
		: m_metalContext( nullptr )
		, m_mtlSamplerState( nullptr )
	{
	}

	ALResult Tr2SamplerStateAL::Create( const Tr2SamplerDescription& desc, Tr2RenderContextAL& renderContext )
	{
		Destroy();

		if( !renderContext.IsValid() )
		{
			return  E_INVALIDCALL;
		}

		bool success = true;

		MTLSamplerAddressMode rAddressMode = MTLSamplerAddressModeClampToEdge;
		MTLSamplerAddressMode sAddressMode = MTLSamplerAddressModeClampToEdge;
		MTLSamplerAddressMode tAddressMode = MTLSamplerAddressModeClampToEdge;
		MTLSamplerBorderColor borderColor = MTLSamplerBorderColorTransparentBlack;
		MTLSamplerMinMagFilter minFilter = MTLSamplerMinMagFilterNearest;
		MTLSamplerMinMagFilter magFilter = MTLSamplerMinMagFilterNearest;
		MTLSamplerMipFilter mipFilter = MTLSamplerMipFilterNotMipmapped;
		MTLCompareFunction compareFunction = MTLCompareFunctionNever;

		success = success && MapAddressMode( desc.m_addressU, sAddressMode );
		success = success && MapAddressMode( desc.m_addressV, tAddressMode );
		success = success && MapAddressMode( desc.m_addressW, rAddressMode );
		// Border color value is used only when any of the address modes is
		// MTLSamplerAddressModeClampToBorderColor. In all other cases we can
		// choose a default value for border color and assume a successful remapping.
		success = success && MapBorderColor( desc.m_borderColor, borderColor );

		success = success && MapMinMagFilter( desc.m_minFilter, minFilter );
		success = success && MapMinMagFilter( desc.m_magFilter, magFilter );
		success = success && MapMipFilter( desc.m_mipFilter , mipFilter );

		success = success && MapCompareFunction( desc.m_comparisonFunc, compareFunction );

		if( !success )
		{
			return E_INVALIDARG;
		}

		m_metalContext = renderContext.GetMetalContext();
		MTLSamplerDescriptor* metalSamplerDescriptor = m_metalContext->CreateSamplerDescriptor();

		metalSamplerDescriptor.rAddressMode = rAddressMode;
		metalSamplerDescriptor.sAddressMode = sAddressMode;
		metalSamplerDescriptor.tAddressMode = tAddressMode;
		metalSamplerDescriptor.borderColor = borderColor;

		metalSamplerDescriptor.minFilter = minFilter;
		metalSamplerDescriptor.magFilter = magFilter;
		metalSamplerDescriptor.mipFilter = mipFilter;

		metalSamplerDescriptor.lodMinClamp = desc.m_minLOD;
		metalSamplerDescriptor.lodMaxClamp = desc.m_maxLOD;
		// metalSamplerDescriptor.lodAverage = NO; // iOS only
		metalSamplerDescriptor.maxAnisotropy = g_forceAnisotropy ? g_forceAnisotropy : desc.m_maxAnisotropy;
		// desc.m_mipLODBias

		metalSamplerDescriptor.compareFunction = compareFunction;
		// desc.m_isComparisonFilter

		metalSamplerDescriptor.supportArgumentBuffers = NO;

		m_mtlSamplerState = m_metalContext->CreateSamplerState( metalSamplerDescriptor );
		m_metalContext->DestroySamplerDescriptor( metalSamplerDescriptor );

		return S_OK;
	}

	void Tr2SamplerStateAL::Destroy()
	{
		m_metalContext->DestroySamplerState( m_mtlSamplerState );
		m_mtlSamplerState = nil;
		m_metalContext = nil;
	}

	bool Tr2SamplerStateAL::IsValid() const
	{
		return ( m_mtlSamplerState != nil );
	}

	void Tr2SamplerStateAL::Describe( Tr2DeviceResourceDescriptionAL& description ) const
	{
        description["type"] = "Tr2SamplerStateAL";
	}
}

#endif
