// Copyright © 2023 CCP ehf.

#include "StdAfx.h"

#include "Tr2DeviceResource.h"

#include "Tr2Renderer.h"
#include "TriDevice.h"

Tr2DeviceResource::Tr2DeviceResource()
{
	TriDevice::RegisterResource( this );
}

Tr2DeviceResource::~Tr2DeviceResource()
{
	TriDevice::UnregisterResource( this );
}


bool Tr2DeviceResource::PrepareResources()
{
	// Do not create anything if we're in the middle of an attempted reset
	if( Tr2Renderer::IsResourceCreationAllowed() )
	{
		if( !OnPrepareResources() )
		{
			return false;
		}
	}

	return true;
}
