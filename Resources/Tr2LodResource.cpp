////////////////////////////////////////////////////////////////////////////////
//
// Created:		August 2014
// Copyright:	CCP 2014
//

#include "StdAfx.h"
#include "Tr2LodResource.h"

Tr2LodResource::Tr2LodResource( IRoot* lockobj ) :
	m_currentLod( TR2_LOD_UNSPECIFIED )
{
}

// --------------------------------------------------------------------------------
// Description:
//   Set the individual lod's resource path.
// Arguments:
//   lod - which lod level?
//   resPath - the new respath for that lod
// --------------------------------------------------------------------------------
void Tr2LodResource::SetResourcePath( Tr2Lod lod, const char* resPath )
{
	m_resPath[ lod ] = resPath;
}

IBlueResource* Tr2LodResource::GetResource()
{
	if( m_requested )
	{
		if( m_requested->IsGood() )
		{
			m_active = m_requested;
			m_requested.Unlock();
		}
	}
	
	if( m_active )
	{
		return m_active;
	}

	return nullptr;
}

void Tr2LodResource::SelectLod( Tr2Lod lod )
{
	if( lod == m_currentLod )
	{
		return;
	}

	m_currentLod = lod;

	auto& respath = m_resPath[ m_currentLod ];
	BeResMan->GetResource( respath, "", m_requested );
}
