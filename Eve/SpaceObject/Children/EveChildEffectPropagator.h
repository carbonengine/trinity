#pragma once
#define EveChildEffectPropagator_H

#include "EveChildContainer.h"
#include "EveChildInstanceContainer.h"

BLUE_DECLARE( Tr2CurveScalar );
BLUE_DECLARE( EveLocatorSets );
BLUE_DECLARE( Tr2SphereShapeAttributeGenerator );
BLUE_DECLARE_VECTOR( Tr2SphereShapeAttributeGenerator );

// --------------------------------------------------------------------------------------
// Description:
//   artists can trigger instanced effects by adding the effect and then
//   animating a TriggerSphere radius with a simple scalarCurve
//   see also: EveChildExplosion
// --------------------------------------------------------------------------------------
BLUE_CLASS( EveChildEffectPropagator ) :
	public EveChildContainer,
	public INotify
{
public:
	EXPOSE_TO_BLUE();

	EveChildEffectPropagator( IRoot* lockobj = nullptr );
	~EveChildEffectPropagator();

	void Play();
	void Stop();

	bool OnModified( Be::Var* value );

	void UpdateVisibility(const TriFrustum& frustum, const Matrix& parentTransform, Tr2Lod parentLod) override;
	void AddQuadsToQuadRenderer(const TriFrustum& frustum, Tr2QuadRenderer& quadRenderer) const override;
	void GetRenderables(std::vector<ITr2Renderable*>& renderables) override;
	void UpdateAsyncronous(EveUpdateContext& updateContext, const EveChildUpdateParams& params) override;
	void UpdateSyncronous( EveUpdateContext& updateContext, const EveChildUpdateParams& params );
	void RegisterWithQuadRenderer( Tr2QuadRenderer& quadRenderer );
	
	void GetDebugOptions( Tr2DebugRendererOptions & options ) override;
	void RenderDebugInfo( ITr2DebugRenderer2& renderer ) override;

	enum PropagationType 
	{ 
		LOCAL_LOCATORS, 
		LOCATOR_SET_BY_REF, 
		RANDOM_SPREAD, 
	};
	
private:
	struct Transform
	{
		Quaternion rotation;
		Vector3 position;
		Vector3 scale;
		float sqrDistToSphereCenter;
	};
	
	struct SortByCircleDist
	{
		bool operator()( const Transform& lhs, const Transform& rhs ) const
		{
			return lhs.sqrDistToSphereCenter < rhs.sqrDistToSphereCenter;
		}
	};
	
	void ProcessLocators();
	void DistanceSortLocators();
	void ManageTriggers();

	float m_playTime;

	EveChildInstanceContainerPtr m_effect;			// Child containing trigger effect
	IEveSpaceObjectChildPtr m_effectShared;			// Child containing shared parts of the effect (particle systems etc.)

	Vector3 m_effectScaling;
	float m_randScaleMin;
	float m_randScaleMax;
	
	Vector3 m_triggerSphereOffset;
	Tr2CurveScalarPtr m_triggerSphereRadiusCurve;

	EveLocatorSetsPtr m_localLocators;

	std::vector<Transform> m_processedTransforms;
	PropagationType m_type;

	int m_currentTriggerIndex; // skip processing triggers until this point

	float m_stopToClearDelay;
	float m_delayTimer;
	bool m_replayAfterDelay;

	// Locator By Referance 
	EveSpaceObject2Ptr m_refObject;
	BlueSharedString m_locatorSetName;
	float m_completeness;

	// RANDOM_SPREAD
	int64_t  m_numTriggers;
	float m_rndRange;
	float m_rndClosenessPreference;
	float m_rndMinRangeThreshold;
	
	// Is the effect playing
	bool m_isPlaying;
};

TYPEDEF_BLUECLASS( EveChildEffectPropagator );