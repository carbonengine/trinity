// Copyright © 2023 CCP ehf.

#pragma once


BLUE_DECLARE_INTERFACE( IEveSpaceObjectChild );


BLUE_INTERFACE( IEveEffectChildrenOwner ) :
	public IRoot
{
	virtual IEveSpaceObjectChildPtr GetEffectChildByName( const char* name ) const = 0;
	virtual void AddToEffectChildrenList( IEveSpaceObjectChild * child ) = 0;
	virtual void RemoveFromEffectChildrenList( IEveSpaceObjectChild * child ) = 0;
};