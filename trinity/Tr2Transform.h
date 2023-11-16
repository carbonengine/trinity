#pragma once

#ifndef Tr2Transform_h
#define Tr2Transform_h


#include "ITr2Renderable.h"

BLUE_DECLARE( Tr2Transform );
BLUE_DECLARE_VECTOR( Tr2Transform );
BLUE_DECLARE( Tr2MeshBase );
BLUE_DECLARE( TriCurveSet );
BLUE_DECLARE_VECTOR( TriCurveSet );
class TriFrustum;


enum Tr2TransformModifier
{
	TR2TM_NONE = 0,
	TR2TM_BILLBOARD = 1,
	TR2TM_TRANSLATE_WITH_CAMERA = 2,
	TR2TM_LOOK_AT_CAMERA = 3,
	TR2TM_SIMPLE_HALO = 4,

	TR2TM_PRE_TRANSLATE_WITH_CAMERA = 5,

	// Compatibility modifiers - behave like the old TriTransform ones - less efficient
	TR2TM_EVE_CAMERA_ROTATION_ALIGNED = 100,
	TR2TM_EVE_BOOSTER = 101,
	TR2TM_EVE_SIMPLE_HALO = 102,
	TR2TM_EVE_CAMERA_ROTATION = 103,

	TR2TM_FORCE_DWORD = 0xffffffff
};

class ITriRenderBatchAccumulator;

class Tr2Transform:
     public ITr2Renderable
{
public:
    EXPOSE_TO_BLUE();
    Tr2Transform( IRoot* lockobj = NULL );

	/////////////////////////////////////////////////////////////////////////////////////
	// ITr2Renderable
	virtual void GetBatches( ITriRenderBatchAccumulator* batches, TriBatchType batchType, const Tr2PerObjectData* perObjectData, Tr2RenderReason reason = TR2RENDERREASON_NORMAL );
	virtual void GetShadowBatches( ITriRenderBatchAccumulator* batches, const Tr2PerObjectData* perObjectData, float shadowPixelSize );

	virtual bool HasTransparentBatches();
	virtual float GetSortValue();

	void SetScaling( Vector3 scaling ) { m_scaling = scaling; };
	void SetRotation( Quaternion rotation ) { m_rotation = rotation; };
	void SetTranslation( Vector3 translation ) { m_translation = translation; };

	Tr2MeshBasePtr GetMesh() const;

	// GetPerObjectData is left unimplemented. Subclasses
	// must be created for each type of scene to create the proper per object data.

protected:
	// Update everything that's time dependent, such as animation curves, but that does not need the camera
	// position or other render-specific things for special effects.  Is in principle called just once per frame.
	virtual void Update( Be::Time time );
	// Update everything that's camera dependent, such as billboards.  Is called for every render, to support
	// multiple camera's looking at the same scene.
	// For now, this is called from any code that has a Tr2Transform, from inside GetRenderables; this way things
	// such as a view dependent m_worldTransform are set up properly for GetSortValue and GetPerObjectData to work right.	
	virtual void UpdateViewDependentData( const TriFrustum& frustum, const Matrix& parentTransform );

protected:
	std::string m_name;

	Vector3 m_scaling;
	Quaternion m_rotation;
	Vector3 m_translation;

	Matrix m_localTransform;
	Matrix m_worldTransform;

	Tr2TransformModifier m_modifier;
	bool m_useDistanceBasedScale;
	bool m_display;
	bool m_update;

	float m_distanceBasedScaleArg1;
	float m_distanceBasedScaleArg2;

	Tr2MeshBasePtr m_mesh;
	PTriCurveSetVector m_curveSets;

	float m_sortValueMultiplier;
};

TYPEDEF_BLUECLASS( Tr2Transform );

#endif //Tr2Transform_h
