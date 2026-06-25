// Copyright © 2019 CCP ehf.

#pragma once

#include "Tr2RtBottomLevelAccelerationStructureAL.h"

namespace TrinityALImpl
{
class Tr2RtTopLevelAccelerationStructureAL;
}

// The TLAS references the bottom level structures, with each reference containing local-to-world transformation matrix
struct Tr2RtInstanceAL
{
	enum Flags
	{
		NONE = 0,
		FORCE_NON_OPAQUE = 0x8,
	};

	const TrinityALImpl::Tr2RtBottomLevelAccelerationStructureAL* blas;
	float transform[3][4];
	uint32_t materialIndex;
	Flags flags;
};


class Tr2RtTopLevelAccelerationStructureAL
{
public:
	Tr2RtTopLevelAccelerationStructureAL();

	ALResult Create( const size_t count, const Tr2RtInstanceAL* instances, Tr2RtBuildFlags::Type buildFlags, Tr2PrimaryRenderContextAL& renderContext );
	ALResult Update( const size_t count, const Tr2RtInstanceAL* instances, Tr2RenderContextAL& renderContext );

	bool IsValid() const;
	const Tr2BufferAL& GetBuffer() const;
	size_t GetCapacity() const;
	TrinityALImpl::Tr2RtTopLevelAccelerationStructureAL* TrinityALImpl_GetObject() const;

private:
	std::shared_ptr<TrinityALImpl::Tr2RtTopLevelAccelerationStructureAL> m_tlas;
};
