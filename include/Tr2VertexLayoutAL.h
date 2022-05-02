#pragma once


#include "../ALResult.h"
#include "../Tr2DeviceResourceAL.h"


class Tr2PrimaryRenderContextAL;
class Tr2VertexDefinition;

namespace TrinityALImpl
{
	class Tr2VertexLayoutAL;
	class PSODescription;
}

class Tr2VertexLayoutAL
{
public:
	Tr2VertexLayoutAL();

	ALResult Create( const Tr2VertexDefinition& definition, Tr2PrimaryRenderContextAL& renderContext );
	bool IsValid() const;

	bool operator==( const Tr2VertexLayoutAL& other ) const;
	Tr2ALMemoryType GetMemoryClass() const;

	ALResult SetName( const char* name );

private:
	std::shared_ptr<TrinityALImpl::Tr2VertexLayoutAL> m_layout;

	friend class Tr2RenderContextAL;
	friend class Tr2PrimaryRenderContextAL;
	friend class TrinityALImpl::PSODescription;
};
