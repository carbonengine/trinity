// Copyright © 2026 CCP ehf.

#pragma once
#ifndef EveEllipseDefinition_H
#define EveEllipseDefinition_H

BLUE_DECLARE( EveEllipseDefinition );

/*
 * A single closed elliptical orbit. Held in lists by EveEllipseSet.
 * Parent registers a dirty flag pointer so editor edits can trigger geometry rebuilds.
 */
BLUE_CLASS( EveEllipseDefinition ) :
	public INotify
{
public:
	EXPOSE_TO_BLUE();

	EveEllipseDefinition( IRoot* lockobj = nullptr );

	bool OnModified( Be::Var * val ) override;

	void SetDirtyFlag( bool* dirtyFlag );

	Vector3 m_center{};
	float m_semiMajor = 1.f;
	float m_semiMinor = 1.f;
	Vector3 m_planeNormal{ 0.f, 1.f, 0.f };
	float m_rotationDegrees = 0.f;

private:
	bool* m_dirtyFlag = nullptr;
};

TYPEDEF_BLUECLASS( EveEllipseDefinition );
BLUE_DECLARE_VECTOR( EveEllipseDefinition );

#endif