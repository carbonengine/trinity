// Copyright © 2023 CCP ehf.

#pragma once


BLUE_DECLARE( EveSpaceObjectChild );


BLUE_INTERFACE( IEveEffectChildrenOwner ) :
	public IRoot
{
	virtual EveSpaceObjectChildPtr GetEffectChildByName( const char* name ) const = 0;
	virtual void AddToEffectChildrenList( EveSpaceObjectChild * child ) = 0;
	virtual void RemoveFromEffectChildrenList( EveSpaceObjectChild * child ) = 0;
};