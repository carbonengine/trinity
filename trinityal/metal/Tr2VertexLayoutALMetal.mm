#include "StdAfx.h"

#if( TRINITY_PLATFORM == TRINITY_METAL )

#include "Tr2VertexLayoutALMetal.h"
#include "Tr2VertexDefinition.h"
#include "Tr2RenderContextMetal.h"
#include "Tr2ShaderALMetal.h"
#include "MetalContext.h"
#include "ALLog.h"


using namespace Tr2RenderContextEnum;

namespace TrinityALImpl
{
	Tr2VertexLayoutAL::Tr2VertexLayoutAL()
		: m_renderContext( nullptr )
	{
	}

	Tr2VertexLayoutAL::~Tr2VertexLayoutAL()
	{
		Destroy();
	}

	ALResult Tr2VertexLayoutAL::Create( const Tr2VertexDefinition& definition, Tr2RenderContextAL& renderContext )
	{
		Destroy();

		if( !renderContext.IsValid() )
		{
			return E_INVALIDCALL;
		}

		m_renderContext = &renderContext;
		MetalContext* metalContext = m_renderContext->GetMetalContext();

		uint32_t streamMask = 0;

		for( const auto& src : definition.m_items )
		{
			MTLVertexStepFunction metalStepFunction;
			unsigned stepRate = src.m_instanceStepRate;

			if( stepRate )
			{
				metalStepFunction = MTLVertexStepFunctionPerInstance;
			}
			else
			{
				metalStepFunction = MTLVertexStepFunctionPerVertex;
				// Metal requires step rate to be 1 for per vertex function.
				stepRate = 1;
			}

			MTLVertexFormat metalVertexFormat = metalContext->m_utils->GetMTLVertexFormat( src.m_dataType );
			CCP_ASSERT( metalVertexFormat != MTLVertexFormatInvalid );

			Item dst;
			dst.m_usage = src.m_usage;
			dst.m_usageIndex = src.m_usageIndex;
			dst.m_offset = src.m_offset;
			dst.m_stream = src.m_stream;
			dst.m_format = metalVertexFormat;
			dst.m_stepFunction = metalStepFunction;
			dst.m_stepRate = stepRate;

			m_items.push_back( dst );

			streamMask |= ( 1 << src.m_stream );
		};

		return S_OK;
	}

	bool Tr2VertexLayoutAL::IsValid() const
	{
		return m_renderContext != nullptr;
	}

	void Tr2VertexLayoutAL::Destroy()
	{
		if( m_renderContext )
		{
			MetalContext* metalContext = m_renderContext->GetMetalContext();
			
			for( auto descriptor : m_metalVertexDescriptors )
			{
				metalContext->DestroyVertexLayout( descriptor.second.descriptor );
			}
			m_metalVertexDescriptors.clear();
		}
		m_items.clear();
		m_renderContext = nullptr;
	}

	void Tr2VertexLayoutAL::Describe( Tr2DeviceResourceDescriptionAL& description ) const
	{
		description["type"] = "Tr2VertexLayoutAL";
		description["name"] = m_name;
	}

	ALResult Tr2VertexLayoutAL::SetName( const char* name )
	{
		m_name = name;
		return S_OK;
	}

	Tr2VertexLayoutAL::MetalVertexDescriptor Tr2VertexLayoutAL::GenerateVertexDescriptor( const std::vector<Tr2ShaderPipelineInputAL>& shaderInputs ) const
	{
		MetalContext* metalContext = m_renderContext->GetMetalContext();
		MetalVertexDescriptor result = { metalContext->CreateVertexLayout(), 0, false };

		static_assert(
			sizeof( result.streamMask ) * 8 >= METAL_VERTEX_STREAM_BUFFER_COUNT,
			"Please use a type with more bits for m_vertexStreamMask." );

		for( auto& input : shaderInputs )
		{
			bool found = false;
			for( auto& item : m_items )
			{
				if( input.usage == item.m_usage && input.usageIndex == item.m_usageIndex )
				{
					auto attrIndex = input.registerIndex;
					auto stream = item.m_stream;

					result.descriptor.attributes[attrIndex].format = item.m_format;
					result.descriptor.attributes[attrIndex].offset = item.m_offset;
					result.descriptor.attributes[attrIndex].bufferIndex = stream;

					result.descriptor.layouts[stream].stepFunction = item.m_stepFunction;
					result.descriptor.layouts[stream].stepRate = item.m_stepRate;

					result.streamMask |= 1 << stream;

					found = true;
					break;
				}
			}

			// Point any missing inputs at the dummy stream.
			if( !found )
			{
				auto attrIndex = input.registerIndex;
				switch( input.type )
				{
				case Tr2ShaderPipelineInputAL::INT:
					result.descriptor.attributes[attrIndex].format = MTLVertexFormatChar4;
					break;
				case Tr2ShaderPipelineInputAL::UINT:
					result.descriptor.attributes[attrIndex].format = MTLVertexFormatUChar4;
					break;
				default:
					result.descriptor.attributes[attrIndex].format = MTLVertexFormatFloat4;
					break;
				}
				result.descriptor.attributes[attrIndex].offset        = 0;
				result.descriptor.attributes[attrIndex].bufferIndex   = METAL_VERTEX_STREAM_DUMMY;
				result.descriptor.layouts[METAL_VERTEX_STREAM_DUMMY].stepFunction = MTLVertexStepFunctionConstant;
				result.descriptor.layouts[METAL_VERTEX_STREAM_DUMMY].stepRate     = 0;

				result.streamMask |= 1 << METAL_VERTEX_STREAM_DUMMY;

				result.needsDummyStream = true;
			}
		}
		return result;
	}

	void Tr2VertexLayoutAL::SetVertexLayout( const std::shared_ptr<TrinityALImpl::Tr2ShaderProgramAL>& shaderProgram, Tr2RenderContextAL& renderContext )
	{
		MetalVertexDescriptor descriptor;

		bool isInParallelEncoding = renderContext.GetMetalContext()->IsInParallelEncoding();
		
		auto inputHash = shaderProgram->GetInputsHash();
		auto found = m_metalVertexDescriptors.find( inputHash );
		if( found != end( m_metalVertexDescriptors ) )
		{
			descriptor = found->second;
		}
        else if( isInParallelEncoding )
        {
            auto queue = renderContext.GetMetalWorkQueue();
            descriptor.descriptor = queue->GetCachedVertexDescriptor( this, inputHash, descriptor.streamMask, descriptor.needsDummyStream );
            if( !descriptor.descriptor )
            {
                descriptor = GenerateVertexDescriptor( shaderProgram->GetInputs() );
                queue->CacheVertexDescriptor( this, inputHash, descriptor.descriptor, descriptor.streamMask, descriptor.needsDummyStream );
            }
        }
		else
		{
			descriptor = GenerateVertexDescriptor( shaderProgram->GetInputs() );
			m_metalVertexDescriptors[inputHash] = descriptor;
		}

		if( descriptor.needsDummyStream )
		{
			renderContext.GetMetalWorkQueue()->SetVertexStream( METAL_VERTEX_STREAM_DUMMY, renderContext.GetMetalContext()->GetDummyBuffer(), sizeof(float) * 4, 0 );
		}
		renderContext.GetMetalWorkQueue()->SetCurrentVertexDescriptor( descriptor.descriptor, descriptor.streamMask );
	}

    void Tr2VertexLayoutAL::AddVertexDescriptor( size_t inputHash, MTLVertexDescriptor* descriptor, uint8_t streamMask, bool needsDummyStream )
    {
        m_metalVertexDescriptors[inputHash] = { descriptor, streamMask, needsDummyStream };
    }
}

#endif
