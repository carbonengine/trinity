// Copyright © 2023 CCP ehf.

#pragma once

BLUE_DECLARE( EveSpaceObjectDecal );

BLUE_INTERFACE( IEveSpaceObjectDecalOwner ) :
	public IRoot
{
	virtual void AddDecal( EveSpaceObjectDecalPtr newDecal ) = 0;
};