// Copyright © 2014 CCP ehf.

#pragma once
#ifndef EveDistanceField_H
#define EveDistanceField_H

#include "Tr2DebugRenderer.h"

class EveUpdateContext;
BLUE_DECLARE_INTERFACE( ITriVectorFunction );
BLUE_DECLARE_VECTOR( ITriVectorFunction );
BLUE_DECLARE( TriCurveSet );
BLUE_DECLARE( TriView );
BLUE_DECLARE( Tr2CurveScalar );

// --------------------------------------------------------------------------------------
// Description:
//   Keeps track of a number of objects, max camera distance from them and estimates
//   a simple volume the covers the objects.
// --------------------------------------------------------------------------------------
BLUE_CLASS( EveDistanceField ) :
	public IListNotify,
	public INotify,
	public ITr2DebugRenderable
{
public:
	EveDistanceField( IRoot* lockobj = 0 );

	EXPOSE_TO_BLUE();

	/////////////////////////////////////////////////////////////////////////////////////
	// IListNotify
	void OnListModified( long event, ssize_t key, ssize_t key2, IRoot* value, const IList* theList );

	/////////////////////////////////////////////////////////////////////////////////////
	// INotify
	bool OnModified( Be::Var * value );


	/////////////////////////////////////////////////////////////////////////////////////
	// ITr2DebugRenderable
	void GetDebugOptions( Tr2DebugRendererOptions & options );
	void RenderDebugInfo( ITr2DebugRenderer2 & renderer );

	void Update( const EveUpdateContext& updateContext );

	void SetupStaticDistanceField( Vector3 dimensions, Vector3 position, float distanceThreshold, float timeAdjustmentSecondsOut, float timeAdjustemntSecondsIn );
	void SetupDynamicDistanceField( float distanceThreshold, float timeAdjustmentSecondsOut, float timeAdjustemntSecondsIn );
	void SetMaxDistance( float maxDistnace );

private:
	void CreateCurveSet();
	void UpdateDistanceCurveSize();

	PITriVectorFunctionVector m_objects;

	TriCurveSetPtr m_curveSet;
	Tr2CurveScalarPtr m_distanceCurve;

	TriViewPtr m_cameraView;

	// min and max distance for determining how far inside the environment you are
	float m_minDistance;
	float m_maxDistance;
	// distance value used by curve set
	float m_distance;
	// Adjust how long it takes to settle on a new value when zooming out
	float m_timeAdjustmentSecondsOut;
	// Adjust how long it takes to settle on a new value when zooming in
	float m_timeAdjustmentSecondsIn;

	// used for rejecting objects that are too far away when calcuating
	// area bounds for this field
	float m_distanceThreshold;
	// maximum allowed ratio between x and z sizes of field
	float m_maxXZRatio;
	// minimum allowed ratio between y size and max of x and z sizes
	float m_minYRatio;

	// indicate if bounds must be re-evaluated
	bool m_dirty;

	// indicate if distance curve needs to be updated
	bool m_updateDistanceCurve;

	// indicate that the distance field is dynamic and therefore should be rebuilt when balls are added
	bool m_isDynamic;

	// middle of the field modified by distance threshold and so on
	Vector3 m_middle;
	// dimensions of the field/volume fog
	Vector3 m_dimensions;
	void SetNeutralValues();
	float CalculateFieldCoverageAndDistance( Be::Time t, const Vector3& posRef, const Vector3& originShift );

	// members for debug purposes
	bool m_debug;
	std::vector<Vector3> m_debugPositions;
};

TYPEDEF_BLUECLASS( EveDistanceField );

#endif