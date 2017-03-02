#include "StdAfx.h"
#include "Tr2ShaderALStub.h"
#if( TRINITY_PLATFORM==TRINITY_STUB )

#include "ALLog.h"

using namespace Tr2RenderContextEnum;

Tr2ShaderAL::Tr2ShaderAL()
	:m_type(INVALID_SHADER)
{
}

ALResult Tr2ShaderAL::Create( 
	Tr2RenderContextAL& renderContext, 
	Tr2RenderContextEnum::ShaderType type, 
	const void* bytecode, 
	uint32_t bytecodeSize, 
	const void* /*patchedBytecode*/, 
	uint32_t /*patchedBytecodeSize*/, 
	const Tr2ShaderInputDefinition& inputDefinition )
{
	m_bytecode.resize("Tr2ShaderALStub::m_bytecode", bytecodeSize);
	memcpy(m_bytecode.get(), bytecode, bytecodeSize);
	m_type = type;

	return S_OK;
}

Tr2ShaderAL::~Tr2ShaderAL()
{
	Destroy();
}

void Tr2ShaderAL::Destroy()
{
	m_type = INVALID_SHADER;
}

bool Tr2ShaderAL::IsValid() const
{
	return m_type != INVALID_SHADER;
}

Tr2RenderContextEnum::ShaderType Tr2ShaderAL::GetType() const
{
	return m_type;
}

ALResult Tr2ShaderAL::GetBytecode( const void*& bytecode, uint32_t& size ) const
{
	bytecode = m_bytecode.get();
	size = uint32_t( m_bytecode.size() );
	return S_OK;
}

void Tr2ShaderAL::SetNullShaderType( Tr2RenderContextEnum::ShaderType type )
{
	m_type = type;
}

#endif
