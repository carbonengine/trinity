//
//  Tr2RtTopLevelAccelerationStructureALMetal.cpp
//  carbon-trinity
//
//  Created by Iris Dogg Skarphedinsdottir on 3.1.2024.
//



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
