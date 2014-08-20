////////////////////////////////////////////////////////////////////////////////
//
// Created:		August 2014
// Copyright:	CCP 2014
//

#include "StdAfx.h"
#include "Tr2Texture2dLodParameter.h"
#include "Resources/Tr2LodResource.h"
#include "Resources/TriTextureRes.h"


Tr2Texture2dLodParameter::Tr2Texture2dLodParameter( IRoot* lockobj /*= nullptr */ )
{

}

void Tr2Texture2dLodParameter::SetLodResource( Tr2LodResourcePtr newLodResource )
{
	m_lodResource = newLodResource;
}

TriTextureRes* Tr2Texture2dLodParameter::GetResource() const
{
	if( m_lodResource )
	{
		return dynamic_cast<TriTextureRes*>( m_lodResource->GetResource() );
	}

	return nullptr;
}
