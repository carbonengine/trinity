// Copyright © 2025 CCP ehf.

#pragma once

#include <ITriFunction.h>

BLUE_DECLARE( Tr2VectorFunctionModifier );

class Tr2VectorFunctionModifier : public ITriVectorFunction
{
public:
	EXPOSE_TO_BLUE();

	Tr2VectorFunctionModifier( IRoot* lockobj = NULL );

	//////////////////////////////////////////////////////////////////////////
	// ITriFunction
	//////////////////////////////////////////////////////////////////////////
	// This is a stub, so that Trinity will compile following CL 73220
	// It is functionality only required for curveset support, so I'm not sure what the correct functionality
	// here should be
	virtual void UpdateValue( double time ) override {};

	//////////////////////////////////////////////////////////////////////////
	// ITriVectorFunction
	//////////////////////////////////////////////////////////////////////////
	Vector3* Update( Vector3* in, Be::Time time ) override;
	Vector3* Update( Vector3* in, double time ) override;

	// position
	Vector3* GetValueAt( Vector3* in, Be::Time time ) override;
	Vector3* GetValueAt( Vector3* in, double time ) override;

	// velocity
	Vector3* GetValueDotAt( Vector3* in, Be::Time time ) override;
	Vector3* GetValueDotAt( Vector3* in, double time ) override;

	// acceleration
	Vector3* GetValueDoubleDotAt( Vector3* in, Be::Time time ) override;
	Vector3* GetValueDoubleDotAt( Vector3* in, double time ) override;

	Vector3d* InterpolatedPosition( Vector3d* out, Be::Time time ) override;

protected:
	Vector3* GetTransformedPosition( Vector3* in ) const;
	Vector3 GetOffsetPosition() const;

	// The base clientBall or other ITriVectorFunction objects
	ITriVectorFunctionPtr m_clientBall;

	// The positional offset to apply to clientBall
	Vector3 m_offsetPosition;

	// The positional scale to apply to clientBall
	float m_scaleModifier;

	// if true, the m_offsetPosition is transformed to view space
	bool m_useViewSpace;

	// if true, uses system coordinates instead of ship coordinates
	bool m_useSystemCoordinates;
};

TYPEDEF_BLUECLASS( Tr2VectorFunctionModifier );
