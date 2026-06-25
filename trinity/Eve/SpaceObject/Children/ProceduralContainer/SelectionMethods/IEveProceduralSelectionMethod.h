// Copyright © 2023 CCP ehf.

#pragma once

#include "Eve/SpaceObject/Children/IEveSpaceObjectChild.h"
#include "Eve/SpaceObject/Children/EveChildRef.h"
#include "Eve/Volume/IEveVolume.h"

BLUE_INTERFACE( IEveProceduralSelectionMethod ) :
	public IRoot
{
	virtual void UpdateAsyncronous( const EveUpdateContext& updateContext, const EveChildUpdateParams& params ) = 0;
	virtual bool IsSelectedChildModified() const = 0;
	virtual IEveVolumeVector* GetDebugVolumes() = 0;
	virtual EveChildRefPtr GetSelectedChild() = 0;

	virtual void SetProceduralMethodVariable( const char* name, float value ) {};
	virtual const char* GetProceduralMethodVariable()
	{
		return "not Implemented";
	};
};
