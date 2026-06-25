// Copyright © 2023 CCP ehf.

#include "StdAfx.h"
#if TRINITY_PLATFORM == TRINITY_METAL

#include "Tr2ShaderALMetal.h"
#include "ALLog.h"
#include "Tr2HalHelperStructures.h"

using namespace Tr2RenderContextEnum;

namespace TrinityALImpl
{
Tr2ShaderAL::Tr2ShaderAL() : m_type( INVALID_SHADER )
{
}

ALResult Tr2ShaderAL::Create( Tr2RenderContextEnum::ShaderType type,
							  const Tr2ShaderBytecodeAL& bytecode,
							  const Tr2ShaderSignatureAL& signature,
							  const char* shaderPath,
							  Tr2PrimaryRenderContextAL& renderContext )
{
	if( !signature.samplers.empty() )
	{
		return E_INVALIDARG;
	}
	m_bytecode.resize( "Tr2ShaderALMetal::m_bytecode", bytecode.size );
	if( m_bytecode.empty() )
	{
		return E_OUTOFMEMORY;
	}

	memcpy( m_bytecode.get(), bytecode.bytecode, bytecode.size );
	m_signature = signature;
	m_type = type;

	m_resourceMask.constantBufferMask = 0;
	m_resourceMask.bufferMask = 0;
	m_resourceMask.textureMask = 0;
	m_resourceMask.samplerMask = 0;
	// for (auto pipelineInput : signature.pipelineInputs)
	// {
	// 	int index = METAL_VERTEX_STREAM_BUFFER_OFFSET + reg.registerIndex;
	// 	m_resourceMask.bufferMask |= 1 << pipelineInput.registerIndex;
	// }
	for( auto reg : signature.registers )
	{
		switch( reg.registerType )
		{
		case Tr2ShaderRegisterAL::CONSTANT_BUFFER: {
			int index = METAL_CONST_BUFFER_OFFSET + reg.registerIndex;
			assert( ( m_resourceMask.constantBufferMask & ( 1 << index ) ) == 0 );
			m_resourceMask.constantBufferMask |= 1 << index;
			break;
		}
		case Tr2ShaderRegisterAL::SAMPLER: {
			if( reg.arrayCount != 1 )
			{
				// This is an array of samplers, and it's bound as an SRV buffer
				m_resourceMask.bufferMask |= 1 << ( METAL_SRV_BUFFER_OFFSET + reg.registerIndex );
			}
			else
			{
				m_resourceMask.samplerMask |= 1 << reg.registerIndex;
			}
			break;
		}
		case Tr2ShaderRegisterAL::SRV_BUFFER:
		case Tr2ShaderRegisterAL::SRV_STRUCTURED_BUFFER: {
			int index = METAL_SRV_BUFFER_OFFSET + reg.registerIndex;
			assert( ( m_resourceMask.bufferMask & ( 1 << index ) ) == 0 );
			m_resourceMask.bufferMask |= 1 << index;
			break;
		}
		case Tr2ShaderRegisterAL::SRV_TEXTURE1D: {
			int index = METAL_SRV_TEXTURE_OFFSET + reg.registerIndex;
			assert( ( m_resourceMask.textureMask & ( 1 << index ) ) == 0 );
			m_resourceMask.textureMask |= 1 << index;
			m_resourceMask.textureTypes[index] = uint8_t( MTLTextureType1D );
			break;
		}
		case Tr2ShaderRegisterAL::SRV_TEXTURE1DARRAY: {
			int index = METAL_SRV_TEXTURE_OFFSET + reg.registerIndex;
			assert( ( m_resourceMask.textureMask & ( 1 << index ) ) == 0 );
			m_resourceMask.textureMask |= 1 << index;
			m_resourceMask.textureTypes[index] = uint8_t( MTLTextureType1DArray );
			break;
		}
		case Tr2ShaderRegisterAL::SRV_TEXTURE2D: {
			int index = METAL_SRV_TEXTURE_OFFSET + reg.registerIndex;
			assert( ( m_resourceMask.textureMask & ( 1 << index ) ) == 0 );
			m_resourceMask.textureMask |= 1 << index;
			m_resourceMask.textureTypes[index] = uint8_t( MTLTextureType2D );
			break;
		}
		case Tr2ShaderRegisterAL::SRV_TEXTURE2DARRAY: {
			int index = METAL_SRV_TEXTURE_OFFSET + reg.registerIndex;
			assert( ( m_resourceMask.textureMask & ( 1 << index ) ) == 0 );
			m_resourceMask.textureMask |= 1 << index;
			m_resourceMask.textureTypes[index] = uint8_t( MTLTextureType2DArray );
			break;
		}
		case Tr2ShaderRegisterAL::SRV_TEXTURE2DMS: {
			int index = METAL_SRV_TEXTURE_OFFSET + reg.registerIndex;
			assert( ( m_resourceMask.textureMask & ( 1 << index ) ) == 0 );
			m_resourceMask.textureMask |= 1 << index;
			m_resourceMask.textureTypes[index] = uint8_t( MTLTextureType2DMultisample );
			break;
		}
		case Tr2ShaderRegisterAL::SRV_TEXTURE2DMSARRAY: {
			int index = METAL_SRV_TEXTURE_OFFSET + reg.registerIndex;
			assert( ( m_resourceMask.textureMask & ( 1 << index ) ) == 0 );
			m_resourceMask.textureMask |= 1 << index;
			m_resourceMask.textureTypes[index] = uint8_t( MTLTextureType2DMultisampleArray );
			break;
		}
		case Tr2ShaderRegisterAL::SRV_TEXTURE3D: {
			int index = METAL_SRV_TEXTURE_OFFSET + reg.registerIndex;
			assert( ( m_resourceMask.textureMask & ( 1 << index ) ) == 0 );
			m_resourceMask.textureMask |= 1 << index;
			m_resourceMask.textureTypes[index] = uint8_t( MTLTextureType3D );
			break;
		}
		case Tr2ShaderRegisterAL::SRV_TEXTURECUBE: {
			int index = METAL_SRV_TEXTURE_OFFSET + reg.registerIndex;
			assert( ( m_resourceMask.textureMask & ( 1 << index ) ) == 0 );
			m_resourceMask.textureMask |= 1 << index;
			m_resourceMask.textureTypes[index] = uint8_t( MTLTextureTypeCube );
			break;
		}
		case Tr2ShaderRegisterAL::SRV_TEXTURECUBEARRAY: {
			int index = METAL_SRV_TEXTURE_OFFSET + reg.registerIndex;
			assert( ( m_resourceMask.textureMask & ( 1 << index ) ) == 0 );
			m_resourceMask.textureMask |= 1 << index;
			m_resourceMask.textureTypes[index] = uint8_t( MTLTextureTypeCubeArray );
			break;
		}
		case Tr2ShaderRegisterAL::UAV_BUFFER:
		case Tr2ShaderRegisterAL::UAV_STRUCTURED_BUFFER: {
			int index = METAL_UAV_BUFFER_OFFSET + reg.registerIndex;
			assert( ( m_resourceMask.bufferMask & ( 1 << index ) ) == 0 );
			m_resourceMask.bufferMask |= 1 << index;
			break;
		}
		case Tr2ShaderRegisterAL::UAV_TEXTURE1D: {
			int index = METAL_UAV_TEXTURE_OFFSET + reg.registerIndex;
			assert( ( m_resourceMask.textureMask & ( 1 << index ) ) == 0 );
			m_resourceMask.textureMask |= 1 << index;
			m_resourceMask.textureTypes[index] = uint8_t( MTLTextureType1D );
			break;
		}
		case Tr2ShaderRegisterAL::UAV_TEXTURE1DARRAY: {
			int index = METAL_UAV_TEXTURE_OFFSET + reg.registerIndex;
			assert( ( m_resourceMask.textureMask & ( 1 << index ) ) == 0 );
			m_resourceMask.textureMask |= 1 << index;
			m_resourceMask.textureTypes[index] = uint8_t( MTLTextureType1DArray );
			break;
		}
		case Tr2ShaderRegisterAL::UAV_TEXTURE2D: {
			int index = METAL_UAV_TEXTURE_OFFSET + reg.registerIndex;
			assert( ( m_resourceMask.textureMask & ( 1 << index ) ) == 0 );
			m_resourceMask.textureMask |= 1 << index;
			m_resourceMask.textureTypes[index] = uint8_t( MTLTextureType2D );
			break;
		}
		case Tr2ShaderRegisterAL::UAV_TEXTURE2DARRAY: {
			int index = METAL_UAV_TEXTURE_OFFSET + reg.registerIndex;
			assert( ( m_resourceMask.textureMask & ( 1 << index ) ) == 0 );
			m_resourceMask.textureMask |= 1 << index;
			m_resourceMask.textureTypes[index] = uint8_t( MTLTextureType2DArray );
			break;
		}
		case Tr2ShaderRegisterAL::UAV_TEXTURE2DMS: {
			int index = METAL_UAV_TEXTURE_OFFSET + reg.registerIndex;
			assert( ( m_resourceMask.textureMask & ( 1 << index ) ) == 0 );
			m_resourceMask.textureMask |= 1 << index;
			m_resourceMask.textureTypes[index] = uint8_t( MTLTextureType2DMultisample );
			break;
		}
		case Tr2ShaderRegisterAL::UAV_TEXTURE2DMSARRAY: {
			int index = METAL_UAV_TEXTURE_OFFSET + reg.registerIndex;
			assert( ( m_resourceMask.textureMask & ( 1 << index ) ) == 0 );
			m_resourceMask.textureMask |= 1 << index;
			m_resourceMask.textureTypes[index] = uint8_t( MTLTextureType2DMultisampleArray );
			break;
		}
		case Tr2ShaderRegisterAL::UAV_TEXTURE3D: {
			int index = METAL_UAV_TEXTURE_OFFSET + reg.registerIndex;
			assert( ( m_resourceMask.textureMask & ( 1 << index ) ) == 0 );
			m_resourceMask.textureMask |= 1 << index;
			m_resourceMask.textureTypes[index] = uint8_t( MTLTextureType3D );
			break;
		}
		case Tr2ShaderRegisterAL::UAV_TEXTURECUBE: {
			int index = METAL_UAV_TEXTURE_OFFSET + reg.registerIndex;
			assert( ( m_resourceMask.textureMask & ( 1 << index ) ) == 0 );
			m_resourceMask.textureMask |= 1 << index;
			m_resourceMask.textureTypes[index] = uint8_t( MTLTextureTypeCube );
			break;
		}
		case Tr2ShaderRegisterAL::UAV_TEXTURECUBEARRAY: {
			int index = METAL_UAV_TEXTURE_OFFSET + reg.registerIndex;
			assert( ( m_resourceMask.textureMask & ( 1 << index ) ) == 0 );
			m_resourceMask.textureMask |= 1 << index;
			m_resourceMask.textureTypes[index] = uint8_t( MTLTextureTypeCubeArray );
			break;
		}
		default:
			assert( false );
			break;
		}
	}

	return S_OK;
}

Tr2ShaderAL::~Tr2ShaderAL()
{
	Destroy();
}

void Tr2ShaderAL::Destroy()
{
	m_signature = Tr2ShaderSignatureAL();
	m_bytecode.clear();
	m_type = INVALID_SHADER;
}

bool Tr2ShaderAL::IsValid() const
{
	return m_type != INVALID_SHADER && !m_bytecode.empty();
}

Tr2RenderContextEnum::ShaderType Tr2ShaderAL::GetType() const
{
	return m_type;
}

ALResult Tr2ShaderAL::GetBytecode( Tr2ShaderBytecodeAL& bytecode ) const
{
	if( m_bytecode.empty() )
	{
		bytecode = Tr2ShaderBytecodeAL();
		return E_INVALIDCALL;
	}

	bytecode.bytecode = m_bytecode.get();

	bytecode.size = m_bytecode.size();
	return S_OK;
}

const Tr2ShaderSignatureAL& Tr2ShaderAL::GetSignature() const
{
	return m_signature;
}

void Tr2ShaderAL::SetNullShaderType( Tr2RenderContextEnum::ShaderType type )
{
	m_type = type;
}

void Tr2ShaderAL::Describe( Tr2DeviceResourceDescriptionAL& description ) const
{
	description["type"] = "Tr2ShaderAL";
	description["name"] = m_name;
}

ALResult Tr2ShaderAL::SetName( const char* name )
{
	m_name = name;
	return S_OK;
}
}

#endif
