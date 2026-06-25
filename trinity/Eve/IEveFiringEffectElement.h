// Copyright © 2023 CCP ehf.

#pragma once

class EveUpdateContext;
class TriFrustum;
class Tr2LightManager;
class Tr2QuadRenderer;
BLUE_DECLARE_INTERFACE( ITr2Renderable );

BLUE_INTERFACE( IEveFiringEffectElement ) :
	public IRoot
{
	virtual void SetDestObjectScale( float scale ) = 0;
	virtual void StartMoving() = 0;
	virtual float GetCurveDuration() = 0;
	virtual void StartFiring( float delay ) = 0;
	virtual void StopFiring() = 0;

	virtual void SetFiringTransform( const Matrix& source, const Vector3& dest ) = 0;
	virtual void SetFiringTransform( const Vector3& source, const Vector3& dest ) = 0;
	virtual void DisplayEndPoints( bool displaySource, bool displayDest ) = 0;

	virtual void Update( const EveUpdateContext& updateContext )
	{
	}
	virtual void UpdateEffectAsync( const EveUpdateContext& updateContext ) = 0;
	virtual void UpdateEffectSync( const EveUpdateContext& updateContext ) = 0;

	virtual void UpdateVisibility( const EveUpdateContext& updateContext, const Matrix& parentTransform ) = 0;
	virtual void GetRenderables( std::vector<ITr2Renderable*> & renderables ) = 0;

	virtual void RegisterWithQuadRenderer( Tr2QuadRenderer & quadRenderer )
	{
	}
	virtual void AddQuadsToQuadRenderer( const TriFrustum& frustum, Tr2QuadRenderer& quadRenderer )
	{
	}
	virtual void SetIntensity( float intensity )
	{
	}
	virtual void SetDisplay( bool display )
	{
	}
};

BLUE_DECLARE_IVECTOR( IEveFiringEffectElement );