////////////////////////////////////////////////////////////////////////////////
//
// Created:		August 2014
// Copyright:	CCP 2014
//

#include "StdAfx.h"
#include "Tr2Texture2dLodParameter.h"
#include "ITr2TextureProvider.h"
#include "Resources/Tr2LodResource.h"


Tr2Texture2dLodParameter::Tr2Texture2dLodParameter( IRoot* lockobj /*= nullptr */ )
{

}

void Tr2Texture2dLodParameter::SetLodResource( Tr2LodResource* newLodResource )
{
	m_lodResource = newLodResource;
}

ITr2TextureProvider* Tr2Texture2dLodParameter::GetResource() const
{
	if( m_lodResource )
	{
		return dynamic_cast<ITr2TextureProvider*>( m_lodResource->GetResource() );
	}

	return nullptr;
}
