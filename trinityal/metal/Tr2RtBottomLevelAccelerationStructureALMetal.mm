//
//  Tr2RtBottomLevelAccelerationStructureMetal.m
//  carbon-trinity
//
//  Created by Iris Dogg Skarphedinsdottir on 3.1.2024.
//


Tr2RtBottomLevelAccelerationStructureAL()::Tr2RtBottomLevelAccelerationStructureAL()
{
    
}

Tr2RtBottomLevelAccelerationStructureAL()::~Tr2RtBottomLevelAccelerationStructureAL()
{

}

ALResult Tr2RtBottomLevelAccelerationStructureAL::Create( const Tr2RtPositionStreamAL& positions, const Tr2RtIndicesStreamAL& indices, int numObjects, Tr2RtBuildFlags::Type buildFlags, Tr2PrimaryRenderContextAL& renderContext )
{
    if( !renderContext.IsValid() || !renderContext.GetCaps().SupportsRayTracing() )
    {
        return E_INVALIDCALL;
    }
    if( !positions.IsValid() || !HasFlag( positions.m_vertexBuffer.GetDesc().gpuUsage, Tr2GpuUsage::SHADER_RESOURCE ) )
    {
        return E_INVALIDARG;
    }
    if( !indices.IsValid() || !HasFlag( indices.m_indexBuffer.GetDesc().gpuUsage, Tr2GpuUsage::SHADER_RESOURCE ) )
    {
        return E_INVALIDARG;
    }
}


ALResult Tr2RtBottomLevelAccelerationStructureAL::Update( const Tr2RtPositionStreamAL& positions, const Tr2RtIndicesStreamAL& indices, Tr2RenderContextAL& renderContext )
{
    
}

ALResult Tr2RtBottomLevelAccelerationStructureAL::CompactBlas( Tr2PrimaryRenderContextAL& renderContext )
{
    return S_OK;
}

bool Tr2RtBottomLevelAccelerationStructureAL::IsValid() const
{
    if( m_primitiveAccelerationStructures )
    {
        return true;
    }
}
