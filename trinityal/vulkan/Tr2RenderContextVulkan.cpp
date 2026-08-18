#include "StdAfx.h"

#if TRINITY_PLATFORM == TRINITY_VULKAN

#include "Tr2RenderContextVulkan.h"
#include "Tr2BufferALVulkan.h"
#include "Tr2ShaderProgramALVulkan.h"
#include "Tr2PrimaryRenderContextVulkan.h"
#include "Tr2ResourceSetALVulkan.h"
#include "Tr2VertexLayoutALVulkan.h"
#include "Tr2TextureALVulkan.h"
#include "Tr2ConstantBufferALVulkan.h"
#include "Tr2ShaderBindingABIVulkan.h"
#include "VkResult.h"
#include "../include/Tr2RtTopLevelAccelerationStructureAL.h"

bool g_gatherPipelineStatistics = false;

namespace
{

	std::pair<uint32_t, uint32_t> s_primitiveToVertexCount[] = {
		std::make_pair( 0, 0 ),
		std::make_pair( 3, 0 ),
		std::make_pair( 1, 2 ),
		std::make_pair( 0, 0 ),
		std::make_pair( 2, 0 ),
		std::make_pair( 1, 1 ),
		std::make_pair( 1, 0 ),
	};

}

size_t Tr2RenderContextAL::RenderPassSource::GetHash() const
{
	return CcpHashFNV1( this, sizeof( *this ) );
}

size_t Tr2RenderContextAL::PipelineSource::GetHash() const
{
	return CcpHashFNV1( this, sizeof( *this ) );
}

Tr2RenderContextAL::Tr2RenderContextAL() throw( )
	:m_dirtyPso( true ),
	m_dirtyPass( true ),
	m_owner( nullptr ),
	m_renderPass( 0 ),
	m_framebuffer( VK_NULL_HANDLE ),
	m_commandBuffer( VK_NULL_HANDLE ),
	m_constantPool( VK_NULL_HANDLE ),
	m_constantSet( VK_NULL_HANDLE ),
	m_boundConstantLayout( VK_NULL_HANDLE ),
	m_constantsDirty( false ),
	m_primitiveToVertexCount( 0, 0 )
{
	memset( &m_pipelineSource, 0, sizeof( m_pipelineSource ) );
	memset( &m_renderPassSource, 0, sizeof( m_renderPassSource ) );

	m_pipelineSource.m_depthStencilState.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
	m_pipelineSource.m_rasterizationState.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	m_pipelineSource.m_rasterizationState.lineWidth = 1;

	VkPipelineColorBlendAttachmentState defaultAttachment = {
		VK_FALSE,                                                     // VkBool32                                       blendEnable
		VK_BLEND_FACTOR_ONE,                                          // VkBlendFactor                                  srcColorBlendFactor
		VK_BLEND_FACTOR_ZERO,                                         // VkBlendFactor                                  dstColorBlendFactor
		VK_BLEND_OP_ADD,                                              // VkBlendOp                                      colorBlendOp
		VK_BLEND_FACTOR_ONE,                                          // VkBlendFactor                                  srcAlphaBlendFactor
		VK_BLEND_FACTOR_ZERO,                                         // VkBlendFactor                                  dstAlphaBlendFactor
		VK_BLEND_OP_ADD,                                              // VkBlendOp                                      alphaBlendOp
		VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |         // VkColorComponentFlags                          colorWriteMask
		VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT
	};

	for( uint32_t i = 0; i < _countof( m_pipelineSource.m_attachmentBlend ); ++i )
	{
		m_pipelineSource.m_attachmentBlend[i] = defaultAttachment;
	}

	VkPipelineColorBlendStateCreateInfo defaultColorBlend = {
		VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,     // VkStructureType                                sType
		nullptr,                                                      // const void                                    *pNext
		0,                                                            // VkPipelineColorBlendStateCreateFlags           flags
		VK_FALSE,                                                     // VkBool32                                       logicOpEnable
		VK_LOGIC_OP_COPY,                                             // VkLogicOp                                      logicOp
		1,                                                            // uint32_t                                       attachmentCount
		m_pipelineSource.m_attachmentBlend,                                // const VkPipelineColorBlendAttachmentState     *pAttachments
		{ 0.0f, 0.0f, 0.0f, 0.0f }                                    // float                                          blendConstants[4]
	};
	m_pipelineSource.m_colorBlendState = defaultColorBlend;

}

Tr2RenderContextAL::~Tr2RenderContextAL() throw( )
{
	Destroy();
}

void Tr2RenderContextAL::Destroy() throw( )
{
	if( m_framebuffer != VK_NULL_HANDLE )
	{
		m_owner->DestroyLaterVulkan( m_framebuffer, vkDestroyFramebuffer );
		m_framebuffer = VK_NULL_HANDLE;
	}

	// Release device resources held by this render context while the device is
	// still live. m_pipelineSource.m_shaderProgram is a member destroyed by the
	// base-class destructor, which runs after Tr2PrimaryRenderContextAL::Destroy()
	// has already vkDestroyDevice'd -- and Tr2ShaderAL::Destroy() destroys its
	// VkShaderModule directly against m_owner->m_device, so leaving the program
	// for the destructor means destroying the module against a null device.
	m_pipelineSource.m_shaderProgram = Tr2ShaderProgramAL();
	m_resourceSet = Tr2ResourceSetAL();

	if( m_constantPool != VK_NULL_HANDLE )
	{
		m_owner->DestroyLaterVulkan( m_constantPool, vkDestroyDescriptorPool );
		m_constantPool = VK_NULL_HANDLE;
	}
	m_constantSet = VK_NULL_HANDLE;
	m_boundConstantLayout = VK_NULL_HANDLE;
	m_constantBuffers.clear();
	m_constantsDirty = false;
}

bool Tr2RenderContextAL::IsValid() const throw( )
{
	return m_owner != nullptr;
}

ALResult Tr2RenderContextAL::BeginScene() throw( )
{
	return S_OK;
}

ALResult Tr2RenderContextAL::EndScene()
{
	return S_OK;
}

ALResult Tr2RenderContextAL::Clear(
	uint32_t clearFlags,
	uint32_t color,
	float depth,
	uint32_t stencil,
	uint32_t slot ) throw( )
{
	if( clearFlags & Tr2RenderContextEnum::CLEARFLAGS_TARGET )
	{
		if( m_boundRenderTargets[slot].IsValid() )
		{
			float f = 1.0f / 255.0f;
			VkClearColorValue clearColor = {
				{
					f * (float)(uint8_t)( color >> 16 ),
					f * (float)(uint8_t)( color >> 8 ),
					f * (float)(uint8_t)( color >> 0 ),
					f * (float)(uint8_t)( color >> 24 )
				} 
			};

			if( m_renderPass )
			{
				vkCmdEndRenderPass( m_commandBuffer );
				m_renderPass = VK_NULL_HANDLE;

				VkImageSubresourceRange subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };
				VkImageMemoryBarrier barrier = {
					VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
					nullptr,
					VK_ACCESS_TRANSFER_WRITE_BIT,
					VK_ACCESS_MEMORY_READ_BIT,
					VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
					VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
					VK_QUEUE_FAMILY_IGNORED,
					VK_QUEUE_FAMILY_IGNORED,
					m_boundRenderTargets[slot].m_texture->GetImageVulkan(),
					subresourceRange
				};
				vkCmdPipelineBarrier( m_commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrier );
			}

			VkImageSubresourceRange subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };
			vkCmdClearColorImage( m_commandBuffer, m_boundRenderTargets[slot].m_texture->GetImageVulkan(), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, &clearColor, 1, &subresourceRange );

			m_dirtyPass = true;
		}
		else
		{
			return E_INVALIDCALL;
		}
	}
	if( clearFlags & Tr2RenderContextEnum::CLEARFLAGS_ZBUFFER )
	{
		return E_NOTIMPL;
	}
	if( clearFlags & Tr2RenderContextEnum::CLEARFLAGS_STENCIL )
	{
		return E_NOTIMPL;
	}

	return S_OK;
}

ALResult Tr2RenderContextAL::SetStreamSource( uint32_t stream, const Tr2BufferAL & buffer, uint32_t offset, uint32_t stride ) throw( )
{
	VkDeviceSize deviceOffset = offset;
	if( !buffer.IsValid() )
	{
		auto buf = m_owner->GetZeroBufferVulkan();
		vkCmdBindVertexBuffers( m_commandBuffer, stream, 1, &buf, &deviceOffset );
		return S_OK;
	}
	m_pipelineSource.m_streams[stream].stride = stride;
	vkCmdBindVertexBuffers( m_commandBuffer, stream, 1, &buffer.m_buffer->m_buffer, &deviceOffset );
	return S_OK;
}

ALResult Tr2RenderContextAL::SetIndices( const Tr2BufferAL & buffer ) throw( )
{
	if( !buffer.IsValid() )
	{
		vkCmdBindIndexBuffer( m_commandBuffer, m_owner->GetZeroBufferVulkan(), 0, VK_INDEX_TYPE_UINT16 );
		return S_OK;
	}
	vkCmdBindIndexBuffer( m_commandBuffer, buffer.m_buffer->m_buffer, 0, buffer.GetDesc().stride == 4 ?  VK_INDEX_TYPE_UINT32 : VK_INDEX_TYPE_UINT16 );
	return S_OK;
}


ALResult Tr2RenderContextAL::SetVertexLayout( const Tr2VertexLayoutAL& layout ) throw( )
{
	if( !( m_pipelineSource.m_layout == layout ) )
	{
		m_pipelineSource.m_layout = layout;
		m_dirtyPso = true;
	}
	return S_OK;
}

namespace
{
	VkPrimitiveTopology s_topologyMap[] = {
		VK_PRIMITIVE_TOPOLOGY_MAX_ENUM,
		VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
		VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP,
		VK_PRIMITIVE_TOPOLOGY_TRIANGLE_FAN,
		VK_PRIMITIVE_TOPOLOGY_LINE_LIST,
		VK_PRIMITIVE_TOPOLOGY_LINE_STRIP,
		VK_PRIMITIVE_TOPOLOGY_POINT_LIST,
	};
}

ALResult Tr2RenderContextAL::SetTopology( Tr2RenderContextEnum::Topology topology ) throw( )
{
	auto top = s_topologyMap[topology];
	if( top != m_pipelineSource.m_topology )
	{
		m_primitiveToVertexCount = s_primitiveToVertexCount[topology];
		m_pipelineSource.m_topology = top;
		m_dirtyPso = true;
	}
	return S_OK;
}

ALResult Tr2RenderContextAL::SetShaderProgram( const Tr2ShaderProgramAL& shader ) throw( )
{
	if( !( m_pipelineSource.m_shaderProgram == shader ) )
	{
		// The constant descriptor set/pool belong to the old program's layout, which
		// is destroyed when the program is released (and the driver may reuse the
		// handle). Invalidate them so the next SetPipeline reallocates against the
		// new program's layout rather than comparing stale layout handles.
		if( m_constantPool )
		{
			m_owner->DestroyLaterVulkan( m_constantPool, vkDestroyDescriptorPool );
			m_constantPool = VK_NULL_HANDLE;
		}
		m_constantSet = VK_NULL_HANDLE;
		m_boundConstantLayout = VK_NULL_HANDLE;
		m_constantsDirty = true;

		m_pipelineSource.m_shaderProgram = shader;
		m_dirtyPso = true;
	}
	return S_OK;
}

ALResult Tr2RenderContextAL::SetResourceSet( const Tr2ResourceSetAL& resourceSet ) throw( )
{
	m_resourceSet = resourceSet;
	return S_OK;
}

ALResult Tr2RenderContextAL::SetConstants( const Tr2ConstantBufferAL& buffer, Tr2RenderContextEnum::ShaderType constantType, uint32_t registerIndex, uint32_t ) throw( )
{
	if( !m_owner )
	{
		return E_INVALIDCALL;
	}

	const uint32_t binding = Tr2VulkanBindingABI::BindingNumber( Tr2ShaderRegisterAL::CONSTANT_BUFFER, registerIndex, constantType );

	if( !buffer.IsValid() )
	{
		m_constantBuffers.erase( binding );
	}
	else
	{
		m_constantBuffers[binding] = buffer.m_buffer->GetBufferVulkan();
	}
	m_constantsDirty = true;
	return S_OK;
}


ALResult Tr2RenderContextAL::SetRenderState( Tr2RenderContextEnum::RenderState state, uint32_t value ) throw( )
{
	switch( state )
	{
	case Tr2RenderContextEnum::RS_ZENABLE:
		m_pipelineSource.m_depthStencilState.depthTestEnable = value != 0;
		m_dirtyPso = true;
		return S_OK;
	case Tr2RenderContextEnum::RS_CULLMODE:
		m_pipelineSource.m_rasterizationState.cullMode = value - 1;
		m_dirtyPso = true;
		return S_OK;
	case Tr2RenderContextEnum::RS_ALPHABLENDENABLE:
		m_pipelineSource.m_attachmentBlend[0].blendEnable = value != 0;
		m_pipelineSource.m_attachmentBlend[1].blendEnable = value != 0;
		m_pipelineSource.m_attachmentBlend[2].blendEnable = value != 0;
		m_pipelineSource.m_attachmentBlend[3].blendEnable = value != 0;
		m_dirtyPso = true;
		return S_OK;
	default:
		return E_NOTIMPL;
	}
}

ALResult Tr2RenderContextAL::SetRenderStates( const uint32_t* stateValuePairs, uint32_t count ) throw( )
{
	for( uint32_t i = 0; i < count; ++i )
	{
		FORWARD_HR( SetRenderState( Tr2RenderContextEnum::RenderState( *stateValuePairs++ ), *stateValuePairs++ ) );
	}
	return S_OK;
}

ALResult Tr2RenderContextAL::SetRenderTarget( const Tr2TextureAL& renderTarget, uint32_t slot, uint32_t slice ) throw()
{
	m_boundRenderTargets[slot] = renderTarget;

	if( renderTarget.IsValid() )
	{
		VkAttachmentDescription attachment = {
			0,
			renderTarget.m_texture->m_format,
			VkSampleCountFlagBits( renderTarget.GetMsaaDesc().samples ),
			VK_ATTACHMENT_LOAD_OP_LOAD,
			VK_ATTACHMENT_STORE_OP_STORE,
			VK_ATTACHMENT_LOAD_OP_DONT_CARE,
			VK_ATTACHMENT_STORE_OP_DONT_CARE,
			VK_IMAGE_LAYOUT_UNDEFINED,
			VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
		};
		if( memcmp( &m_renderPassSource.m_rt[slot + 1], &attachment, sizeof( attachment ) ) != 0 )
		{
			m_renderPassSource.m_rt[slot + 1] = attachment;
			m_dirtyPass = true;
		}
	}
	else
	{
		VkAttachmentDescription attachment = {
			0,
			VK_FORMAT_UNDEFINED,
			VK_SAMPLE_COUNT_1_BIT,
			VK_ATTACHMENT_LOAD_OP_LOAD,
			VK_ATTACHMENT_STORE_OP_STORE,
			VK_ATTACHMENT_LOAD_OP_DONT_CARE,
			VK_ATTACHMENT_STORE_OP_DONT_CARE,
			VK_IMAGE_LAYOUT_UNDEFINED,
			VK_IMAGE_LAYOUT_UNDEFINED,
		};
		if( memcmp( &m_renderPassSource.m_rt[slot + 1], &attachment, sizeof( attachment ) ) != 0 )
		{
			m_renderPassSource.m_rt[slot + 1] = attachment;
			m_dirtyPass = true;
		}
	}
	return S_OK;
}

ALResult Tr2RenderContextAL::DrawIndexedPrimitive(
	uint32_t numVertices,
	uint32_t startIndex,
	uint32_t primitiveCount,
	uint32_t minimumIndex ) throw( )
{
	SetPipeline();

	vkCmdDrawIndexed( m_commandBuffer, m_primitiveToVertexCount.first * primitiveCount + m_primitiveToVertexCount.second, 1, 0, 0, 0 );
	return S_OK;
}

ALResult Tr2RenderContextAL::DrawPrimitive( uint32_t startVertex, uint32_t primitiveCount ) throw( )
{
	SetPipeline();

	vkCmdDraw( m_commandBuffer, m_primitiveToVertexCount.first * primitiveCount + m_primitiveToVertexCount.second, 1, 0, 0 );
	return S_OK;
}

ALResult Tr2RenderContextAL::SetPass()
{
	if( !m_dirtyPass )
	{
		return S_OK;
	}

	auto hash = m_renderPassSource.GetHash();
	auto found = m_owner->m_renderPasses.find( hash );
	if( found == m_owner->m_renderPasses.end() )
	{
		FORWARD_HR( CreateRenderPass( m_renderPass ) );
		m_owner->m_renderPasses[hash] = m_renderPass;
	}
	else
	{
		m_renderPass = found->second;
	}
	UpdateFramebuffer();

	VkRenderPassBeginInfo render_pass_begin_info = {
		VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
		nullptr,
		m_renderPass,
		m_framebuffer,
		{ { 0, 0 }, { m_boundRenderTargets[0].GetWidth(), m_boundRenderTargets[0].GetHeight() } },
		0,
		nullptr
	};

	vkCmdBeginRenderPass( m_commandBuffer, &render_pass_begin_info, VK_SUBPASS_CONTENTS_INLINE );

	VkViewport viewport = {
		0.0f,
		float( m_boundRenderTargets[0].GetHeight() ),
		float( m_boundRenderTargets[0].GetWidth() ),
		-float( m_boundRenderTargets[0].GetHeight() ),
		0.0f,
		1.0f
	};

	VkRect2D scissor = {
		{ 0, 0 },
		{ m_boundRenderTargets[0].GetWidth(), m_boundRenderTargets[0].GetHeight() }
	};

	vkCmdSetViewport( m_commandBuffer, 0, 1, &viewport );
	vkCmdSetScissor( m_commandBuffer, 0, 1, &scissor );

	m_dirtyPass = false;
	m_dirtyPso = true;
	return S_OK;
}

ALResult Tr2RenderContextAL::CreateRenderPass( VkRenderPass& renderPass )
{
	VkAttachmentDescription attachments[5];
	VkAttachmentReference ds = { 0xffffffff };
	VkAttachmentReference rts[4];

	uint32_t count = 0;
	uint32_t rtCount = 0;

	for( uint32_t i = 0; i < 5; ++i )
	{
		if( m_renderPassSource.m_rt[i].format != VK_FORMAT_UNDEFINED )
		{
			if( i == 0 )
			{
				ds.attachment = count;
			}
			else
			{
				rts[i - 1].attachment = count;
				rts[i - 1].layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
				rtCount = i;
			}
			attachments[count++] = m_renderPassSource.m_rt[i];
		}
	}

	VkSubpassDescription subpass = {
		0,
		VK_PIPELINE_BIND_POINT_GRAPHICS,
		0,
		nullptr,
		rtCount,
		rts,
		nullptr,
		ds.attachment != 0xffffffff ? &ds : nullptr,
		0,
		nullptr
	};

	VkRenderPassCreateInfo renderPassInfo = {
		VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
		nullptr,
		0,
		count,
		attachments,
		1,
		&subpass,
		0,
		nullptr
	};

	return Vk2Al( vkCreateRenderPass( m_owner->m_device, &renderPassInfo, nullptr, &renderPass ) );
}

void Tr2RenderContextAL::UpdateFramebuffer()
{
	if( m_framebuffer != VK_NULL_HANDLE )
	{
		m_owner->DestroyLaterVulkan( m_framebuffer, vkDestroyFramebuffer );
		m_framebuffer = VK_NULL_HANDLE;
	}

	uint32_t width = m_boundRenderTargets[0].GetWidth();
	uint32_t height = m_boundRenderTargets[0].GetHeight();

	VkImageView views[4] = {};
	views[0] = m_boundRenderTargets[0].m_texture->GetImageView();

	VkFramebufferCreateInfo framebufferInfo = {
		VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
		nullptr,
		0,
		m_renderPass,
		1,
		views,
		width,
		height,
		1
	};

	Vk2Al( vkCreateFramebuffer( m_owner->m_device, &framebufferInfo, nullptr, &m_framebuffer ) );
}

ALResult Tr2RenderContextAL::SetPipeline()
{
	if( m_dirtyPass )
	{
		SetPass();
	}
	if( !m_dirtyPso )
	{
		return S_OK;
	}

	auto hash = m_pipelineSource.GetHash();
	auto found = m_owner->m_pipelines.find( hash );
	VkPipeline pipeline;
	if( found == m_owner->m_pipelines.end() )
	{
		FORWARD_HR( CreatePipeline( pipeline ) );
		m_owner->m_pipelines[hash] = pipeline;
	}
	else
	{
		pipeline = found->second;
	}
	vkCmdBindPipeline( m_commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline );

	auto* program = m_pipelineSource.m_shaderProgram.m_program.get();
	if( m_resourceSet.IsValid() && m_resourceSet.m_resourceSet->m_descriptorSet && program && program->m_resourceLayout )
	{
		vkCmdBindDescriptorSets( 
			m_commandBuffer, 
			VK_PIPELINE_BIND_POINT_GRAPHICS, 
			program->m_pipelineLayout, 
			1, 
			1,
			&m_resourceSet.m_resourceSet->m_descriptorSet, 0, nullptr );
	}

	// Bind constant buffers at descriptor set 0, if the program declares any and
	// any were set. The descriptor set is allocated once per layout and re-written
	// in place when the constants change.
	{
		if( program && program->m_constantLayout && !m_constantBuffers.empty() )
		{
			if( m_boundConstantLayout != program->m_constantLayout )
			{
				m_boundConstantLayout = program->m_constantLayout;
				if( m_constantPool )
				{
					m_owner->DestroyLaterVulkan( m_constantPool, vkDestroyDescriptorPool );
					m_constantPool = VK_NULL_HANDLE;
				}
				m_constantSet = VK_NULL_HANDLE;
			}

			if( !m_constantSet )
			{
				if( !m_constantPool )
				{
					VkDescriptorPoolSize poolSize = { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 32 };
					VkDescriptorPoolCreateInfo poolInfo = {
						VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
						nullptr,
						0,
						1,
						1,
						&poolSize
					};
					CR_RETURN_HR( Vk2Al( vkCreateDescriptorPool( m_owner->m_device, &poolInfo, nullptr, &m_constantPool ) ) );
				}

				VkDescriptorSetAllocateInfo allocateInfo = {
					VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
					nullptr,
					m_constantPool,
					1,
					&program->m_constantLayout
				};
				CR_RETURN_HR( Vk2Al( vkAllocateDescriptorSets( m_owner->m_device, &allocateInfo, &m_constantSet ) ) );
				m_constantsDirty = true;
			}

			if( m_constantsDirty )
			{
				std::vector<VkWriteDescriptorSet> writes;
				std::vector<VkDescriptorBufferInfo> bufferInfos;
				writes.reserve( m_constantBuffers.size() );
				bufferInfos.reserve( m_constantBuffers.size() );
				for( auto it = begin( m_constantBuffers ); it != end( m_constantBuffers ); ++it )
				{
					VkDescriptorBufferInfo bufferInfo = { it->second, 0, VK_WHOLE_SIZE };
					bufferInfos.push_back( bufferInfo );
					VkWriteDescriptorSet write = {
						VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
						nullptr,
						m_constantSet,
						it->first,
						0,
						1,
						VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
						nullptr,
						&bufferInfos.back(),
						nullptr
					};
					writes.push_back( write );
				}
				vkUpdateDescriptorSets( m_owner->m_device, uint32_t( writes.size() ), writes.data(), 0, nullptr );

				m_constantsDirty = false;
			}

			vkCmdBindDescriptorSets(
				m_commandBuffer,
				VK_PIPELINE_BIND_POINT_GRAPHICS,
				program->m_pipelineLayout,
				0,
				1,
				&m_constantSet,
				0,
				nullptr );
		}
	}
	return S_OK;
}

ALResult Tr2RenderContextAL::CreatePipeline( VkPipeline& pipeline )
{
	std::vector<VkVertexInputAttributeDescription> layout;
	m_pipelineSource.m_layout.m_layout->PopulateInputLayoutVulkan( layout, m_pipelineSource.m_shaderProgram.m_program->m_shaderInputs );

	VkPipelineVertexInputStateCreateInfo vertexInput = {
		VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
		nullptr,
		0,
		m_pipelineSource.m_layout.m_layout->m_streamCount,
		m_pipelineSource.m_streams,
		uint32_t( layout.size() ),
		layout.data()
	};

	VkPipelineInputAssemblyStateCreateInfo inputAssembly = {
		VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
		nullptr,
		0,
		m_pipelineSource.m_topology,
		VK_FALSE
	};

	VkPipelineViewportStateCreateInfo viewport = {
		VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
		nullptr,
		0,
		1,
		nullptr,
		1,
		nullptr
	};

	VkPipelineMultisampleStateCreateInfo msaa = {
		VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
		nullptr,
		0,
		VK_SAMPLE_COUNT_1_BIT,
		VK_FALSE,
		1.0f,
		nullptr,
		VK_FALSE,
		VK_FALSE
	};

	VkDynamicState dynamicStates[] = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR, };

	VkPipelineDynamicStateCreateInfo dynamicInfo = {
		VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
		nullptr,
		0,
		2,
		dynamicStates
	};

	VkGraphicsPipelineCreateInfo pipelineCreateInfo = {
		VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
		nullptr,
		0,
		static_cast<uint32_t>( m_pipelineSource.m_shaderProgram.m_program->m_shaderInfo.size() ),
		m_pipelineSource.m_shaderProgram.m_program->m_shaderInfo.data(),
		&vertexInput,
		&inputAssembly,
		nullptr,
		&viewport,
		&m_pipelineSource.m_rasterizationState,
		&msaa,
		nullptr,
		&m_pipelineSource.m_colorBlendState,
		&dynamicInfo,
		m_pipelineSource.m_shaderProgram.m_program->m_pipelineLayout,
		m_renderPass,
		0,
		VK_NULL_HANDLE,
		-1
	};

	return Vk2Al( vkCreateGraphicsPipelines( m_owner->m_device, VK_NULL_HANDLE, 1, &pipelineCreateInfo, nullptr, &pipeline ) );
}

void Tr2RenderContextAL::RenderPassHint( const Tr2ColorAttachment&, const Tr2DepthAttachment& )
{
}

void Tr2RenderContextAL::RenderPassHint( const Tr2ColorAttachment&, const Tr2ColorAttachment&, const Tr2DepthAttachment& )
{
}

ALResult Tr2RenderContextAL::UseTextures( Tr2GpuUsage::Type, size_t, Tr2TextureAL* )
{
	return S_OK;
}

ALResult Tr2RenderContextAL::UseAccelerationStructure( Tr2RtTopLevelAccelerationStructureAL tlas )
{
    return S_OK;
}


#endif
