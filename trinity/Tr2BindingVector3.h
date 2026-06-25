// Copyright © 2012 CCP ehf.

#pragma once
#ifndef Tr2BindingVector3_H
#define Tr2BindingVector3_H

// --------------------------------------------------------------------------------------
// Description:
//   Tr2BindingVector3 is a simple blue-exposed class that contains a Vector3 value. Used
//   for bindings.
// --------------------------------------------------------------------------------------
BLUE_CLASS( Tr2BindingVector3 ) :
	public IRoot
{
public:
	Tr2BindingVector3( IRoot* = nullptr )
	{
	}

	EXPOSE_TO_BLUE();

	Vector3 m_value;
};

TYPEDEF_BLUECLASS( Tr2BindingVector3 );

#endif // Tr2BindingVector3_H