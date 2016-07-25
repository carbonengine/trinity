#pragma once
#ifndef EveStretch_h
#define EveStretch_h


#include "Eve/SpaceObject/EveSpaceObject2.h"
#include "Eve/EveTransform.h"
#include "Curves/TriCurveSet.h"
#include "Curves/Tr2ScalarCurve.h"

BLUE_DECLARE( EveStretch );
BLUE_DECLARE( TriFloat );

class EveStretch:
	public IEveTransform,
	public IEveSpaceObject2
{
public:
    EXPOSE_TO_BLUE();
    EveStretch( IRoot* lockobj = NULL );

	void SetSourcePosition( Vector3 val );
	void SetDestinationPosition( Vector3 val );

	void SetSourceTransform( const Matrix& val );
	void SetDestinationTransform( const Matrix& val );

	// stretch effect can go "both ways"
	void SetIsNegZForward( bool val );

	/////////////////////////////////////////////////////////////////////////////////////
	// IEveSpaceObject2
	virtual void UpdateSyncronous( EveUpdateContext& updateContext );
	virtual void UpdateAsyncronous( EveUpdateContext& updateContext );
	virtual void RenderDebugInfo( Tr2RenderContext& renderContext );
	virtual void GetRenderables( const TriFrustum& frustum, std::vector<ITr2Renderable*>& renderables, const Matrix& parentTransform );
	virtual bool GetBoundingSphere( Vector4& sphere, BoundingSphereQuery query=EVE_BOUNDS_NORMAL ) const;
	virtual void UpdateModelCenterWorldPosition( Vector3 &position, Be::Time t ) {};
	virtual void GetModelCenterWorldPosition( Vector3 &position ) const {};
	virtual bool GetLocalBoundingBox( Vector3 &min, Vector3 &max ) { return false; }
	virtual void GetLocalToWorldTransform( Matrix &transform ) const { D3DXMatrixIdentity( &transform ); }

	/////////////////////////////////////////////////////////////////////////////////////
	// IEveTranfrom
	virtual void Update( EveUpdateContext& updateContext );
	virtual void UpdateViewDependentData( const Matrix& parentTransform, bool updateChildren=true ) {};
	virtual Tr2Lod GetLODLevel() const { return m_lodLevel; }
	void PlayCurveSet( const std::string& name ) {}
	void StopCurveSet( const std::string& name ) {}
	float GetCurveSetDuration( const std::string& name ) const { return 0.f; } 

	unsigned int GetCurveSetCount() const { return (unsigned int)m_curveSets.size(); }
	TriCurveSetPtr GetCurveSet( unsigned int n ) { return m_curveSets[n]; }

	// toggle display of source and dest objects of the stretcher
	void SetDisplayDestObject( bool display ) { m_displayDestObject = display; }
	void SetDisplaySourceObject( bool display ) { m_displaySourceObject = display; }

	void UpdateCurves( EveUpdateContext& updateContext );
	void Start();
	void StartMoving();

	void SetSourceObjectScale( float scale ) { m_sourceObjectScale = scale; };
	void SetDestObjectScale( float scale ) { m_destObjectScale = scale; };
private:
	Tr2Lod m_lodLevel;
	Be::Time m_lastCurveUpdateTime;

	std::string m_name;

	bool m_display;
	bool m_update;
	bool m_displaySourceObject;
	bool m_displayDestObject;
	bool m_useCurveLod;

	float m_sourceObjectScale;
	float m_destObjectScale;

	// Set to true when source transform is set to point towards the destination.
	// This is used for turret firing effects - the turrets already take care of aiming
	// and the stretch only needs to apply the scaling. This is more efficient than
	// calculating the aiming twice, and allows to use one stretch for multi-barrel
	// guns.
	bool m_useTransformsForStretch;

	// what is the direction of the stretch? +z or -z?
	bool m_isNegZForward;

	Matrix m_sourceTransform;
	Matrix m_destinationTransform;

	Vector3 m_sourcePosition; 
	Vector3 m_destinationPosition;

	ITriVectorFunctionPtr m_source;
	ITriVectorFunctionPtr m_dest;

	EveTransformPtr m_sourceObject;
	EveTransformPtr m_destObject;
	EveTransformPtr m_stretchObject;
	EveTransformPtr m_moveObject;

	Tr2ScalarCurvePtr m_progressCurve;
	PTriCurveSetVector m_curveSets;
	TriCurveSetPtr m_moveCompletion;
	bool m_moveCompleted;
	bool m_moving;
	Be::Time m_startTime;

	// This can't be stored directly here as we must allow
	// value bindings to the length.
	TriFloatPtr m_length;
};

TYPEDEF_BLUECLASS( EveStretch );
BLUE_DECLARE_VECTOR( EveStretch );
#endif //EveStretch_h