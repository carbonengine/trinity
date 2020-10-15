#pragma once

#include "../Tr2DeviceResourceAL.h"
#include "../ALResult.h"


class Tr2ShaderAL;
class Tr2PrimaryRenderContextAL;
struct Tr2RegisterMapAL;
namespace TrinityALImpl
{
	class Tr2ShaderProgramAL;
	class Tr2ResourceSetAL;
	class PSODescription;
}

class Tr2ShaderProgramAL
{
public:
	Tr2ShaderProgramAL();

	ALResult Create( Tr2ShaderAL* shaders, size_t count, Tr2PrimaryRenderContextAL& renderContext );

	bool IsValid() const;
	Tr2ALMemoryType GetMemoryClass() const;

	bool operator==( const Tr2ShaderProgramAL& other ) const;

	const Tr2RegisterMapAL& GetRegisterMap() const;
private:
	std::shared_ptr<TrinityALImpl::Tr2ShaderProgramAL> m_program;

	friend class Tr2RenderContextAL;
	friend class Tr2PrimaryRenderContextAL;
	friend class TrinityALImpl::Tr2ResourceSetAL;
	friend class TrinityALImpl::PSODescription;
};
