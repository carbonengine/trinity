////////////////////////////////////////////////////////////
//
//    Created:   April 2020
//    Copyright: CCP 2020
//

#pragma once

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
	public EveChildContainer
{
public:
	EXPOSE_TO_BLUE();

	EveChildEffectPropagator( IRoot* lockobj = nullptr );
	~EveChildEffectPropagator();

	void Play();
	void Stop();

	bool Initialize() override;
	bool OnModified( Be::Var* value ) override;

	void UpdateVisibility( const EveUpdateContext& updateContext, const Matrix& parentTransform, Tr2Lod parentLod ) override;
	void AddQuadsToQuadRenderer( const TriFrustum& frustum, Tr2QuadRenderer& quadRenderer ) const override;
	void GetRenderables( std::vector<ITr2Renderable*>& renderables ) override;
	void UpdateAsyncronous( const EveUpdateContext& updateContext, const EveChildUpdateParams& params ) override;
	void GetLights( Tr2LightManager& lightManager ) const override;
	void UpdateSyncronous( const EveUpdateContext& updateContext, const EveChildUpdateParams& params ) override;
	void RegisterWithQuadRenderer( Tr2QuadRenderer& quadRenderer ) override;
	void SetControllerVariable( const char* name, float value ) override;

	void GetDebugOptions( Tr2DebugRendererOptions & options ) override;
	void RenderDebugInfo( ITr2DebugRenderer2& renderer ) override;

	
	//////////////////////////////////////////////////////////////////////////////////////
	// EveEntity
	void RegisterComponents() override;
	void UnRegisterComponents() override;

	enum PropagationType
	{
		LOCAL_LOCATORS = 0,
		LOCATOR_SET_BY_REF = 1,
		RANDOM_SPREAD = 2,
	};

	enum TriggerType
	{
		TRIGGER_SPHERE_CURVE = 0,
		INTERVAL_TRIGGERS = 1,
		INSTANT_PERMANENT = 2,
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

	void ProcessLocators( IEveSpaceObject2* parent );
	void ProcessRandomSpreadLocators();
	void RecalculateLocatorSizes();
	void ProcessLocalLocators();
	void ProcessRefLocators( IEveSpaceObject2* parent );

	void UpdateTriggerCurve( const EveUpdateContext& updateContext );
	void UpdateTriggerInterval( const EveUpdateContext& updateContext );

	void DistanceSortLocators();
	void ManageTriggers();
	int GetSmartRandomLocatorIndex();

	EveChildInstanceContainer* GetEffect() const;
	void SetEffect( EveChildInstanceContainer* effect );

	float m_playTime;

	EveChildInstanceContainerPtr m_effect;			// Child containing trigger effect
	IEveSpaceObjectChildPtr m_effectShared;			// Child containing shared parts of the effect (particle systems etc.)

	Vector3 m_effectScaling;
	float m_randScaleMin;
	float m_randScaleMax;
	Vector3 m_triggerSphereOffset;
	Tr2CurveScalarPtr m_triggerSphereRadiusCurve;
	float m_triggerSphereScalarMulti;
	EveLocatorSetsPtr m_localLocators;
	std::vector<Transform> m_processedTransforms;
	PropagationType m_type;
	TriggerType m_triggerMethod;

	int m_currentTriggerIndex; // skip processing triggers until this point

	bool m_skipCleanup;
	float m_stopToClearDelay;
	float m_delayTimer;
	bool m_replayAfterDelay;

	// Locator By Referance 
	BlueSharedString m_locatorSetName;
	float m_completeness;

	// RANDOM_SPREAD
	int64_t  m_numTriggers;
	float m_rndRange;
	float m_rndClosenessPreference;
	float m_rndMinRangeThreshold;

	// Interval triggers
	float m_frequency;
	float m_effectDuration;
	float m_stopAfterNumTriggers;
	int m_numDeleted;
	std::vector<int> m_lastTriggered;

	// Is the effect playing
	bool m_isPlaying;
	bool m_trigger;
};

TYPEDEF_BLUECLASS( EveChildEffectPropagator );