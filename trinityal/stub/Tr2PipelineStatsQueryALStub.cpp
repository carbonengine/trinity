// Copyright © 2023 CCP ehf.

#include "StdAfx.h"

#if TRINITY_PLATFORM == TRINITY_STUB
#include "Tr2PipelineStatsQueryALStub.h"


namespace TrinityALImpl
{

Tr2PipelineStatsQueryAL::Tr2PipelineStatsQueryAL()
{
}

ALResult Tr2PipelineStatsQueryAL::Create( Tr2PrimaryRenderContextAL& )
{
	return S_OK;
}

bool Tr2PipelineStatsQueryAL::IsValid() const
{
	return true;
}

void Tr2PipelineStatsQueryAL::Destroy()
{
}

ALResult Tr2PipelineStatsQueryAL::Begin( Tr2RenderContextAL& )
{
	return S_OK;
}

ALResult Tr2PipelineStatsQueryAL::End( Tr2RenderContextAL& )
{
	return S_OK;
}

ALResult Tr2PipelineStatsQueryAL::GetStats( Tr2PipelineStatsDataAL&, Tr2RenderContextAL& )
{
	return S_OK;
}

size_t Tr2PipelineStatsQueryAL::GetValueCount( const Tr2PipelineStatsDataAL& )
{
	return 0;
}

const char* Tr2PipelineStatsQueryAL::GetLabel( const Tr2PipelineStatsDataAL&, size_t )
{
	return "";
}

const char* Tr2PipelineStatsQueryAL::GetDescription( const Tr2PipelineStatsDataAL&, size_t )
{
	return "";
}

::Tr2PipelineStatsQueryAL::Value Tr2PipelineStatsQueryAL::GetValue( const Tr2PipelineStatsDataAL&, size_t )
{
	return 0;
}

void Tr2PipelineStatsQueryAL::Describe( Tr2DeviceResourceDescriptionAL& description ) const
{
	description["type"] = "Tr2PipelineStatsQueryAL";
}

ALResult Tr2PipelineStatsQueryAL::SetName( const char* )
{
	return S_OK;
}

}

#endif