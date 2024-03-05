#pragma once

#include "Tr2DebugRenderer.h"

BLUE_DECLARE( EveVirtualCameraBehaviourVector3Base );
BLUE_DECLARE_VECTOR( EveVirtualCameraBehaviourVector3Base );
BLUE_DECLARE( EveVirtualCameraBehaviourFloatBase );
BLUE_DECLARE_VECTOR( EveVirtualCameraBehaviourFloatBase );
BLUE_DECLARE( TriProjection );
BLUE_DECLARE( TriView );
BLUE_DECLARE_INTERFACE( IEveSpaceObject2 );
BLUE_DECLARE_IVECTOR( IEveSpaceObject2 );

BLUE_CLASS( EveVirtualCamera ) :
	public ITr2DebugRenderable
{
public:
	EXPOSE_TO_BLUE();

	EveVirtualCamera( IRoot* lockobj = NULL );
	~EveVirtualCamera();

    Matrix GetViewMatrix();
	Matrix GetProjectionMatrix( float aspectRatio, float frontClip, float backClip );

    Vector3 GetViewDirection() const;
    Vector3 GetForwardDirection() const;
	Vector3 GetUpDirection() const;
	Vector3 GetRightDirection() const;

    void Update( float deltaTime );

	void Play();
	void Pause();
	void Stop();
	void Reset();

	void UpdateToLocalTime( float time );

	void CopyTransform( const EveVirtualCameraPtr& source );

	/* Not for general use, only in situations where you need a virtual camera bound to an external system. */
	void UpdateExternal( Vector3 position, Vector3 pointOfInterest, float fov, float roll );
	
	const std::string& GetName() const;
	void SetName( const std::string& name );
	float GetAnimationTimelineLength() const;
	void SetAnimationTimelineLength( float value );
	float GetFov() const;
	void SetFov( float fov );
	float GetRoll() const;
	void SetRoll( float roll );
	Vector3 GetPosition() const;
	void SetPosition( const Vector3& position );
	Vector3 GetPointOfInterest() const;
	void SetPointOfInterest( const Vector3& pointOfInterest );

	void AddPositionBehaviour( EveVirtualCameraBehaviourVector3Base* behaviour );
	void AddPointOfInterestBehaviour( EveVirtualCameraBehaviourVector3Base* behaviour );
	void AddFOVBehaviour( EveVirtualCameraBehaviourFloatBase* behaviour );
	void AddRollBehaviour( EveVirtualCameraBehaviourFloatBase* behaviour );
	
	/////////////////////////////////////////////////////////////////////////////////////
	// ITr2DebugRenderable
	void GetDebugOptions( Tr2DebugRendererOptions& options ) override;
	void RenderDebugInfo( ITr2DebugRenderer2& renderer ) override;

private:
	std::string m_name;

	bool m_isRunning;
	
	PEveVirtualCameraBehaviourVector3BaseVector m_positionBehaviours;
	PEveVirtualCameraBehaviourVector3BaseVector m_pointOfInterestBehaviours;
	PEveVirtualCameraBehaviourFloatBaseVector m_fovBehaviours;
	PEveVirtualCameraBehaviourFloatBaseVector m_rollBehaviours;

	float m_fov;
	float m_roll;
	Vector3 m_position;
	Vector3 m_pointOfInterest;
	
	PIEveSpaceObject2Vector m_positionAnchors;
	PIEveSpaceObject2Vector m_pointOfInterestAnchors;
	
	float m_localElapsedTime;
	float m_animationTimelineLength;

	TriProjectionPtr m_projection;
	
	Vector3 m_positionAnchorCenter;
	float m_positionAnchorRadius;
	Vector3 m_positionAnchorForwardDirection;
	
	Vector3 m_pointOfInterestAnchorCenter;
	float m_pointOfInterestAnchorRadius;
	Vector3 m_pointOfInterestAnchorForwardDirection;
};

TYPEDEF_BLUECLASS( EveVirtualCamera );
