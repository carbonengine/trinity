#include "StdAfx.h"
#if TRINITY_PLATFORM == TRINITY_METAL
#include "Tr2ShaderProgramALMetal.h"
#include "Tr2ShaderALMetal.h"
#include "Tr2PrimaryRenderContextMetal.h"
#include "ALLog.h"

using namespace Tr2RenderContextEnum;


namespace TrinityALImpl
{
	Tr2ShaderProgramAL::Tr2ShaderProgramAL() :
		m_isValid( false ),
		m_vertexFunction (nil),
		m_fragmentFunction (nil),
		m_computeFunction(nil),
		m_iaInputsHash( 0 ),
		m_threadGroupSize( MTLSizeMake( 1, 1, 1 ) )
	{
		for (uint32_t i = 0; i  < Tr2RenderContextEnum::SHADER_TYPE_COUNT; i++)
		{
			m_resourceMask[i].ResetMasks();
		}
		m_iaInputs.clear();
	}

	// Like the DX12 implementation we'll do everything here rather than in the shader abstraction.
	ALResult Tr2ShaderProgramAL::Create( ::Tr2ShaderAL* shaders, size_t count, Tr2PrimaryRenderContextAL& renderContext )
	{
		Destroy();

		if( !renderContext.IsValid() || count == 0 )
		{
			return E_INVALIDARG;
		}

		uint32_t bitmask = 0;
		for( size_t i = 0; i < count; ++i )
		{
			if( !shaders[i].IsValid() )
			{
				return E_INVALIDARG;
			}
			auto mask = 1 << shaders[i].GetType();
			if( ( mask & bitmask ) != 0 )
			{
				return E_INVALIDARG;
			}
			bitmask |= mask;
		}

		// Checks that Compute kernels and other types don't concurrently exist
		auto csBit = 1 << COMPUTE_SHADER;
		if( ( bitmask & csBit ) != 0 && ( bitmask & ~csBit ) != 0 )
		{
			return E_INVALIDARG;
		}

		m_metalContext = renderContext.GetMetalContext();
		for( size_t i = 0; i < count; ++i )
		{
			::Tr2ShaderAL* shader = &shaders[i];
			auto shaderType = shader->GetType();

			m_resourceMask[shaderType] = shader->m_shader->m_resourceMask;

			switch( shaderType )
			{
			case VERTEX_SHADER:
				if( m_vertexFunction )
				{
					return E_INVALIDARG;
				}
				m_vertexFunction = CompileShader(*shader, @"mainVS", renderContext);
				if( !m_vertexFunction )
				{
					CCP_AL_LOGERR( "Tr2ShaderProgramAL: Vertex shader creation failed." );
					return E_FAIL;
				}
				m_iaInputs = shader->m_shader->m_signature.pipelineInputs;
				break;
			case PIXEL_SHADER:
				if( m_fragmentFunction )
				{
					return E_INVALIDARG;
				}
				m_fragmentFunction = CompileShader(*shader, @"mainPS", renderContext);
				if( !m_fragmentFunction )
				{
					CCP_AL_LOGERR( "Tr2ShaderProgramAL: Fragment shader creation failed." );
					return E_FAIL;
				}
				break;
			case COMPUTE_SHADER:
				if( m_computeFunction )
				{
					return E_INVALIDARG;
				}
				m_computeFunction = CompileShader(*shader, @"mainCS", renderContext);
				if( !m_computeFunction )
				{
					CCP_AL_LOGERR( "Tr2ShaderProgramAL: Compute shader creation failed." );
					return E_FAIL;
				}
				{
					const Tr2ShaderThreadGroupSizeAL& size = shader->m_shader->m_signature.threadGroupSize;
					m_threadGroupSize = MTLSizeMake( size.x, size.y, size.z );
				}
				break;
			case GEOMETRY_SHADER:
			case HULL_SHADER:
			case DOMAIN_SHADER:
				CCP_AL_LOGERR( "Tr2ShaderProgramAL: Unsupported shader type - %d.", shaderType );
				return E_INVALIDARG;
				break;
			default:
				CCP_AL_LOGERR( "Tr2ShaderProgramAL: Invalid shader type." );
				return E_INVALIDARG;
			}
		}

		m_registerMap = Tr2RegisterMapAL( shaders, count );
		if( m_iaInputs.empty() )
		{
			m_iaInputsHash = 0;
		}
		else
		{
			m_iaInputsHash = CcpHashFNV1( m_iaInputs.data(), m_iaInputs.size() * sizeof( Tr2ShaderPipelineInputAL ) );
		}

		m_isValid = true;
		return S_OK;
	}

	id<MTLFunction> Tr2ShaderProgramAL::CompileShader( const ::Tr2ShaderAL& shader, NSString* entryFunction, Tr2PrimaryRenderContextAL& renderContext )
	{
		id<MTLDevice> mtlDevice = m_metalContext->GetDevice();

		dispatch_data_t shaderData = dispatch_data_create(
			shader.m_shader->m_bytecode.get(),
			shader.m_shader->m_bytecode.size(),
			dispatch_get_global_queue(0, 0),
			DISPATCH_DATA_DESTRUCTOR_DEFAULT );
		NSError* error = nullptr;
		id<MTLLibrary> mtlLib = [mtlDevice newLibraryWithData:shaderData error:&error];
#if !__has_feature(objc_arc)
		dispatch_release(shaderData);
#endif
		if( !mtlLib )
		{
			CCP_AL_LOGERR( "Tr2ShaderProgramAL: Failed to create Metal shader library. Error: %s",
				error.localizedDescription.UTF8String );
#if !__has_feature(objc_arc)
			if( error )
			{
				[error release];
			}
#endif
			return nil;
		}
		id<MTLFunction> fn = [mtlLib newFunctionWithName:entryFunction];
		return fn;
	}

	void Tr2ShaderProgramAL::Destroy()
	{
		m_isValid = false;
#if !__has_feature(objc_arc)
		[m_vertexFunction release];
		[m_fragmentFunction release];
		[m_computeFunction release];
#endif
		m_vertexFunction   = nil;
		m_fragmentFunction = nil;
		m_computeFunction  = nil;
		m_threadGroupSize = MTLSizeMake( 1, 1, 1 );
		m_iaInputs.clear();
		m_iaInputsHash = 0;
		m_registerMap = Tr2RegisterMapAL();
	}

	bool Tr2ShaderProgramAL::IsValid() const
	{
		return m_isValid;
	}

	id<MTLFunction> Tr2ShaderProgramAL::GetVertexShader() const
	{
		return m_vertexFunction;
	}

	id<MTLFunction> Tr2ShaderProgramAL::GetFragmentShader() const
	{
		return m_fragmentFunction;
	}

	id<MTLFunction> Tr2ShaderProgramAL::GetComputeKernel() const
	{
		return m_computeFunction;
	}

	MTLSize Tr2ShaderProgramAL::GetThreadGroupSize() const
	{
		return m_threadGroupSize;
	}
	
	const ShaderResourceMask* Tr2ShaderProgramAL::GetResourceMasks() const
	{
		return m_resourceMask;
	}

	const std::vector<Tr2ShaderPipelineInputAL>& Tr2ShaderProgramAL::GetInputs() const
	{
		return m_iaInputs;
	}
	
	size_t Tr2ShaderProgramAL::GetInputsHash() const
	{
		return m_iaInputsHash;
	}
	
	const Tr2RegisterMapAL& Tr2ShaderProgramAL::GetRegisterMap() const
	{
		return m_registerMap;
	}

	Tr2ALMemoryType Tr2ShaderProgramAL::GetMemoryClass() const
	{
		return AL_MEMORY_MANAGED;
	}

	void Tr2ShaderProgramAL::Describe( Tr2DeviceResourceDescriptionAL& description ) const
	{
		description["type"] = "Tr2ShaderProgramAL";
		description["name"] = m_name;
	}

	ALResult Tr2ShaderProgramAL::SetName( const char* name )
	{
		m_name = name;
		if( m_vertexFunction )
		{
			m_vertexFunction.label = [NSString stringWithUTF8String:name];
		}
		if( m_fragmentFunction )
		{
			m_fragmentFunction.label = [NSString stringWithUTF8String:name];
		}
		if( m_computeFunction )
		{
			m_computeFunction.label = [NSString stringWithUTF8String:name];
		}
		return S_OK;
	}

	void Tr2ShaderProgramAL::SetDummyResources( TrinityALImpl::MetalWorkQueue& workQueue )
	{
		for( uint32_t i = 0; i < Tr2RenderContextEnum::SHADER_TYPE_COUNT; ++i )
		{
			Tr2RenderContextEnum::ShaderType shaderType = (Tr2RenderContextEnum::ShaderType)i;

			// If there's not resource set then we need to set dummies for all textures and samplers.
			uint32_t missingTextureMask = m_resourceMask[i].textureMask;
			uint32_t missingSamplerMask = m_resourceMask[i].samplerMask;

			if( !missingTextureMask && !missingSamplerMask )
			{
				continue;
			}

			// Set any missing textures and samplers to the dummy object.
			uint32_t index = 0;
			while( missingTextureMask && missingSamplerMask )
			{
				if( missingTextureMask & 0x1 )
				{
					auto dummyTexture = m_metalContext->GetDummyTexture( MTLTextureType( m_resourceMask[i].textureTypes[index] ) );
					workQueue.SetTextures( shaderType, &dummyTexture, NSMakeRange( index, 1 ) );
				}
				missingTextureMask >>= 1;

				if( missingSamplerMask & 0x1 )
				{
					auto dummySampler = m_metalContext->GetDummySampler();
					workQueue.SetSamplers( shaderType, &dummySampler, NSMakeRange( index, 1 ) );
				}
				missingSamplerMask >>= 1;

				++index;
			}
		}
	}
}
#endif
