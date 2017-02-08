#pragma once

class EveUpdateContext;
class TriFrustum;
class Tr2LightManager;
BLUE_DECLARE_INTERFACE( ITr2Renderable );

BLUE_INTERFACE( IEveFiringEffectElement ): public IRoot
{
	virtual void SetDestObjectScale( float scale ) = 0;
	virtual void StartMoving() = 0;
	virtual float GetCurveDuration() = 0;
	virtual void StartFiring( float delay ) = 0;
	virtual void StopFiring() = 0;

	virtual void SetFiringTransform( const Matrix& source, const Vector3& dest ) = 0;
	virtual void SetFiringTransform( const Vector3& source, const Vector3& dest ) = 0;
	virtual void DisplayEndPoints( bool displaySource, bool displayDest ) = 0;

	virtual void Update( EveUpdateContext& updateContext ) = 0;
	virtual void UpdateInactive( EveUpdateContext& updateContext ) = 0;

	virtual void UpdateVisibility( const TriFrustum& frustum, const Matrix& parentTransform ) = 0;
	virtual void GetRenderables( std::vector<ITr2Renderable*>& renderables ) = 0;

	virtual void GetLights( Tr2LightManager& lightManager ) const = 0;
};

BLUE_DECLARE_IVECTOR( IEveFiringEffectElement );