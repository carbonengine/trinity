#include "StdAfx.h"

#if( TRINITY_PLATFORM==TRINITY_DIRECTX9 )

#include "Tr2OcclusionQueryALDx9.h"
#include "Tr2RenderContextDx9.h"

Tr2OcclusionQueryAL::Tr2OcclusionQueryAL()
{
}

Tr2OcclusionQueryAL::~Tr2OcclusionQueryAL()
{
}

ALResult Tr2OcclusionQueryAL::Create( Tr2RenderContextAL& renderContext )
{
	Destroy();

	if( !renderContext.IsValid() )
	{
		return E_FAIL;
	}

	return renderContext.m_d3dDevice9->CreateQuery( D3DQUERYTYPE_OCCLUSION, &m_query );
}

bool Tr2OcclusionQueryAL::IsValid() const
{
	return m_query != nullptr;
}

void Tr2OcclusionQueryAL::Destroy()
{
	if( m_query )
	{
		Tr2RenderContextAL::GetPrimaryRenderContext().TrashQuery( m_query );
	}
	m_query = nullptr;
}

ALResult Tr2OcclusionQueryAL::Begin( Tr2RenderContextAL& /*renderContext*/ )
{
	if( m_query == nullptr )
	{
		return E_INVALIDARG;
	}
	AL_UPDATE_RESOURCE_FRAME_USAGE( *this );
	return m_query->Issue( D3DISSUE_BEGIN );
}

ALResult Tr2OcclusionQueryAL::End( Tr2RenderContextAL& /*renderContext*/ )
{
	if( m_query == nullptr )
	{
		return E_INVALIDARG;
	}
	AL_UPDATE_RESOURCE_FRAME_USAGE( *this );
	return m_query->Issue( D3DISSUE_END );
}

ALResult Tr2OcclusionQueryAL::GetPixelCount( Tr2RenderContextAL& /*renderContext*/, uint32_t& count, WaitMode waitMode )
{
	if( m_query == nullptr )
	{
		return E_INVALIDARG;
	}
	AL_UPDATE_RESOURCE_FRAME_USAGE( *this );
	return m_query->GetData( (void*)&count, sizeof( uint32_t ), waitMode == WAIT ?  D3DGETDATA_FLUSH : 0 );
}

#endif