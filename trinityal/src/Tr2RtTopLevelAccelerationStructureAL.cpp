// Copyright © 2019 CCP ehf.

#include "StdAfx.h"
#include "../include/Tr2RtTopLevelAccelerationStructureAL.h"
#include "../include/Tr2CapsAL.h"


#if TRINITY_PLATFORM_SUPPORTS_RAY_TRACING

#include TRINITY_AL_PLATFORM_INCLUDE( Tr2RtTopLevelAccelerationStructureAL )

namespace
{
std::shared_ptr<TrinityALImpl::Tr2RtTopLevelAccelerationStructureAL> NullTlas()
{
	static std::shared_ptr<TrinityALImpl::Tr2RtTopLevelAccelerationStructureAL> nullTlas = std::make_shared<TrinityALImpl::Tr2RtTopLevelAccelerationStructureAL>();
	return nullTlas;
}
}

Tr2RtTopLevelAccelerationStructureAL::Tr2RtTopLevelAccelerationStructureAL() :
	m_tlas( NullTlas() )
{
}

ALResult Tr2RtTopLevelAccelerationStructureAL::Create( const size_t count, const Tr2RtInstanceAL* instances, Tr2RtBuildFlags::Type buildFlags, Tr2PrimaryRenderContextAL& renderContext )
{
	m_tlas = std::make_shared<TrinityALImpl::Tr2RtTopLevelAccelerationStructureAL>();
	auto hr = m_tlas->Create( count, instances, buildFlags, renderContext );
	if( FAILED( hr ) )
	{
		m_tlas = NullTlas();
	}
	return hr;
}

ALResult Tr2RtTopLevelAccelerationStructureAL::Update( const size_t count, const Tr2RtInstanceAL* instances, Tr2RenderContextAL& renderContext )
{
	return m_tlas->Update( count, instances, renderContext );
}

bool Tr2RtTopLevelAccelerationStructureAL::IsValid() const
{
	return m_tlas->IsValid();
}

const Tr2BufferAL& Tr2RtTopLevelAccelerationStructureAL::GetBuffer() const
{
	return m_tlas->GetBuffer();
}

size_t Tr2RtTopLevelAccelerationStructureAL::GetCapacity() const
{
	return m_tlas->GetCapacity();
}

TrinityALImpl::Tr2RtTopLevelAccelerationStructureAL* Tr2RtTopLevelAccelerationStructureAL::TrinityALImpl_GetObject() const
{
	return m_tlas.get();
}

#else

Tr2RtTopLevelAccelerationStructureAL::Tr2RtTopLevelAccelerationStructureAL()
{
}

ALResult Tr2RtTopLevelAccelerationStructureAL::Create( size_t count, const Tr2RtInstanceAL* instances, Tr2RtBuildFlags::Type buildFlags, Tr2PrimaryRenderContextAL& renderContext )
{
	return E_FAIL;
}

ALResult Tr2RtTopLevelAccelerationStructureAL::Update( size_t count, const Tr2RtInstanceAL* instances, Tr2RenderContextAL& renderContext )
{
	return E_FAIL;
}

bool Tr2RtTopLevelAccelerationStructureAL::IsValid() const
{
	return false;
}

const Tr2BufferAL& Tr2RtTopLevelAccelerationStructureAL::GetBuffer() const
{
	static Tr2BufferAL s_emptyBuffer;
	return s_emptyBuffer;
}

size_t Tr2RtTopLevelAccelerationStructureAL::GetCapacity() const
{
	return 0;
}

TrinityALImpl::Tr2RtTopLevelAccelerationStructureAL* Tr2RtTopLevelAccelerationStructureAL::TrinityALImpl_GetObject() const
{
	return nullptr;
}


#endif
