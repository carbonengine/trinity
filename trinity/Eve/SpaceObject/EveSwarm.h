////////////////////////////////////////////////////////////
//
//    Created:   2015
//    Copyright: CCP 2015
//
#pragma once
#ifndef EveSwarm_H
#define EveSwarm_H

#include "Eve/IEveSpaceObject2.h"
#include "ITr2Renderable.h"
#include "TriFrustum.h"
#include "include/ITriFunction.h"

#include "Eve/SpaceObject/EveShip2.h"


BLUE_DECLARE( EveSwarm );


BLUE_CLASS( EveSwarmRenderable ) :
	public ITr2Renderable,
	public ITr2Pickable,
	public IShaderConfigurer,
	public IEveShadowCaster,
	public EveEntity
{
public:
	EXPOSE_TO_BLUE();

	explicit EveSwarmRenderable( IRoot* lockobj = nullptr );
	~EveSwarmRenderable();
	
	/////////////////////////////////////////////////////////////////////////////////////
	// ITr2Renderable
	void GetBatches( ITriRenderBatchAccumulator* batches, TriBatchType batchType, const Tr2PerObjectData* perObjectData, Tr2RenderReason reason = TR2RENDERREASON_NORMAL ) override;

    bool HasTransparentBatches() override;
    float GetSortValue() override; 

	Tr2PerObjectData* GetPerObjectData( ITriRenderBatchAccumulator* accumulator ) override;
	
	/////////////////////////////////////////////////////////////////////////////////////
	// PerObjectData
	void UpdatePerObjectBuffer( Tr2RenderContextEnum::ShaderType shaderType, uint32_t size, void* );
	uint32_t GetPerObjectDataSize( Tr2RenderContextEnum::ShaderType shaderType ) const;
	
	/////////////////////////////////////////////////////////////////////////////////////
	// EveSwarmRenderable
	void InitializeRenderable( EveSwarm* owner, Tr2MeshBase* mesh );
	void SetWorldTransform( const Matrix& transform );
	const Matrix* GetWorldTransform() const { return &m_worldTransform; }
	void SetBoosterIntensity( float intensity );
	void SetShaderData( const EveSpaceObjectVSData& vsData, const EveSpaceObjectPSData& psData );
	void InitDecals( const PEveSpaceObjectDecalVector &decals );
	void PushDecals( std::vector<ITr2Renderable*>& renderables, float screensize );
	void UpdateDecalVisibility( const TriFrustum& frustum, IEveSpaceObject2::ParentData& pd, Tr2GrannyAnimation* animationUpdater );

	/////////////////////////////////////////////////////////////////////////////////////
	// ITr2Pickable
	IRoot* GetID( uint16_t ) override;
	void GetPickingBatches( ITriRenderBatchAccumulator* batches, Tr2PickTypes pickTypes, const Tr2PerObjectData* perObjectData ) override;

	void SetShaderOption( const BlueSharedString& name, const BlueSharedString& value ) override;
	
	//////////////////////////////////////////////////////////////////////////////////////
	// IEveShadowCaster
	bool IsCastingShadow( const TriFrustum& cameraFrustum, const TriFrustumOrtho& shadowFrustum, const uint32_t shadowMapSize, const Vector3 sunDir, float& sizeInShadow ) const override;
	void GetShadowBatches( ITriRenderBatchAccumulator* batches, const Tr2PerObjectData* perObjectData, float shadowPixelSize ) override;
	Tr2PerObjectData* GetShadowPerObjectData( ITriRenderBatchAccumulator* accumulator ) override;

	//////////////////////////////////////////////////////////////////////////////////////
	// EveEntity
	void RegisterComponents() override;
private:
	Tr2MeshBasePtr m_mesh;
	BlueWeakRef<EveSwarm> m_owner;
	Matrix m_worldTransform;
	PEveSpaceObjectDecalVector m_decals;
	
	Tr2PersistentPerObjectData<EveSwarmRenderable> m_perObjectDataVs;
	Tr2PersistentPerObjectData<EveSwarmRenderable> m_perObjectDataPs;
	EveSpaceObjectPSData m_psData;
	EveSpaceObjectVSData m_vsData;
};
TYPEDEF_BLUECLASS( EveSwarmRenderable );
BLUE_DECLARE_VECTOR( EveSwarmRenderable );

struct SwarmVehicle
{
	SwarmVehicle() :
		rotation( 0, 0, 0, 1 ),
		acceleration( 0, 0, 0 ),
		velocity( 0, 0, 0 ),
		position( 0, 0, 0 ),
		wanderTarget( 0, 0, 0 ),
		roll( 0.f )
	{}

	Quaternion rotation;
	Vector3 acceleration;
	Vector3 velocity;
	Vector3 position;
	Vector3 wanderTarget;
	float roll;
};

// For force debug
struct SwarmVehicleDebug
{
	Vector3 separation, cohesion, alignment, wander, anchor, formation, deceleration;
};


BLUE_CLASS( EveSwarm ) :
	public EveShip2
{
public:
	EXPOSE_TO_BLUE();

	explicit EveSwarm( IRoot* lockobj = nullptr );
	~EveSwarm();
	
	/////////////////////////////////////////////////////////////////////////////////////
	// EveSwarm
	void AddSwarmer();
	Vector3 RemoveSwarmer();
	void SetCount( int count );
	void EnableSwarming( bool enable );
	void PickFiringOrigin();
	
	struct BehaviorProperties
	{
		BehaviorProperties() :
			m_speedMultiplier( 1.1f ),
			m_speedMinimum( 10.f ),
			m_mass( 1.f ),
			m_agility( 2.f ),
			m_maxDistance0( 500.f ),
			m_maxDistance1( 125.f ),
			m_timeMultiplier( 1.f ),
			m_maxTime( 0.2f ),
			m_speed0( 700.f ),
			m_speed1( 1000.f ),

			// Behavior characteristics
			m_wanderDistance( 100.f ),
			m_wanderRadius( 80.f ),
			m_wanderFluctuation( 0.05f ),
			m_weightAlign( 50.f ),
			m_weightAnchor( 0.5f ),
			m_anchorRadius0( 75.f ),
			m_anchorRadius1( 250.f ),
			m_weightCohesion( 0.1f ),
			m_weightSeparation( 0.1f ),
			m_separationDistance( 250.f ),
			m_weightWander( 0.33f ),
			m_weightDecelerate( 0.1f ),
			m_maxDeceleration( 200.f ),
			m_weightFormation( 1.f ),
			m_formationDistance( 50.f )
		{}

		float m_mass;
		float m_speedMultiplier;
		float m_speedMinimum;
		float m_agility;
	
		float m_maxDistance0;  // Max allowed distance from ball
		float m_maxDistance1;  // Max allowed distance from ball
		float m_timeMultiplier; // Time multiplier, mostly for debug
		float m_maxTime; // Never update by more than this, anything too long and things stop making sense

		// Alter some values based on linearization of these two speeds
		float m_speed0;
		float m_speed1;

		// Cohesion: steers all vehicles/swarmers towards their average position, keeps vehicles close to each other
		float m_weightCohesion;

		// Separation: steers vehicles away from other vehicles depending on (inverted)distance, avoiding bumping etc
		float m_weightSeparation;
		float m_separationDistance;

		// Align: Steers each vehicle in the average direction of all vehicles. Gives a sense of formation and shared... purpose?
		// This weight is in newtons and uses direction rather than velocity so we don't have to change all paremeters if max
		// velocity changes.
		float m_weightAlign;

		// Wander: Steers a vehicle to a fluctuating point on a sphere in front of the vehicle. Creates some interesting 'natural'
		// looking random movement characteristics.
		float m_weightWander;
		float m_wanderFluctuation; // How fast the point on the sphere changes
		float m_wanderDistance;  // How far in front of the vehicle is the sphere
		float m_wanderRadius;  // Radius of the sphere

		// Anchor: Steer vehicles toward the center point/ball
		float m_weightAnchor;
		float m_anchorRadius0;
		float m_anchorRadius1;

		// Decelerate: Basically works to avoid the vehicles maintaining maximum velocity all the time(if the ball is stationary f.x.) resulting
		// in ugly orbiting style behaviors
		float m_weightDecelerate;
		float m_maxDeceleration;

		// Formation: Have all swarmer except the first try to form a v behind the swarmer 0
		float m_weightFormation;
		float m_formationDistance;
	};
	void SetBehavior( const BehaviorProperties* behavior ) { m_behavior = *behavior; }

	/////////////////////////////////////////////////////////////////////////////////////
	// EveShip2 overrides
	void UpdateSyncronous( EveUpdateContext& updateContext );
	void UpdateAsyncronous( EveUpdateContext& updateContext );
	void UpdateTurretsAsyncronous( EveUpdateContext& updateContext );
	bool GetBoundingSphere( Vector4& sphere, BoundingSphereQuery query=EVE_BOUNDS_NORMAL ) const;
	void UpdateVisibility( const TriFrustum& frustum, const Matrix& parentTransform ) override;
	void PushRenderables( std::vector<ITr2Renderable*>& renderables );
	void RebuildCachedData( BlueAsyncRes* p );
	void UpdateModelCenterWorldPosition( Vector3 &position, Be::Time t );
	void GetModelCenterWorldPosition( Vector3 &position ) const;
	bool GetLocalBoundingBox( Vector3 &min, Vector3 &max );
	
	virtual void RegisterWithQuadRenderer( Tr2QuadRenderer& quadRenderer );
	virtual void AddQuadsToQuadRenderer( const TriFrustum& frustum, Tr2QuadRenderer& quadRenderer );

	/////////////////////////////////////////////////////////////////////////////////////
	// IInitialize
	virtual bool Initialize();
	
	/////////////////////////////////////////////////////////////////////////////////////
	// INotify
	bool OnModified( Be::Var* val );

	/////////////////////////////////////////////////////////////////////////////////////
	// ITr2DebugRenderable
	virtual void GetDebugOptions( Tr2DebugRendererOptions& options );
	virtual void RenderDebugInfo( ITr2DebugRenderer2& renderer );
	
	//////////////////////////////////////////////////////////////////////////////////////
	// EveEntity
	void RegisterComponents() override;
	void UnRegisterComponents() override;
protected:
	/////////////////////////////////////////////////////////////////////////////////////
	// Object space damage locator information
	virtual void GetLocatorInObjectSpace( Vector3& position, Vector3& direction, const Locator& locator ) const;
	virtual bool GetDamageLocatorPosition( Vector3* out, int index, bool inWorldSpace );
	
	/////////////////////////////////////////////////////////////////////////////////////
	// EveShip2 override
	void UpdateWorldBounds();
	void EstimatePixelDiameter( const TriFrustum& frustum );
	Matrix GetObserverTransform() override;
	const Matrix* GetTurretTransform( unsigned int turretSetIndex ) const;
	
	void UpdateBoosters( EveUpdateContext& updateContext ) {}
	void UpdateWorldTransform( Be::Time time );

private:
	std::vector<SwarmVehicle> m_vehicles;
	std::vector<SwarmVehicleDebug> m_debugInfo;
	PEveSwarmRenderableVector m_renderables;

	void UpdateSwarm( Be::Time t );
	Vector3d m_origin;
	Be::Time m_timeLast;
	float m_timeSinceUpdate;
	float m_lodUpdateTime;

	// Which fighter is being shot
	int m_targetIndex;
	// And which fighter is the origin for firing effects
	int m_firingIndex;

	// Swarming properties
	bool m_swarmingEnabled;
	bool m_started;

	int32_t m_count;
	float m_debugSize;

	// frustum so we can update the decal visibility
	TriFrustum m_frustum;

	// Squad bounds
	Vector3 m_squadBoundsMin;
	Vector3 m_squadBoundsMax;

	Vector3 m_worldAcceleration;

	// Behavior data and functions
	BehaviorProperties m_behavior;

	Vector3 CalculateForces( int i0, std::vector<SwarmVehicle>& swarmers, const Vector3& followPosition, const Vector3& centerOfMass, const Vector3& alignment, const Vector3& formationDirection, const Vector3& formationSide, float timeSeconds );
	Vector3 Calculate_Cohesion( Vector3 p0, Vector3 p1 );
	Vector3 Calculate_Separation( Vector3 p0, Vector3 p1 );
	Vector3 Calculate_Wander( SwarmVehicle& s, float wanderDistance, float radius, float fluctuation, float t );
	void UpdateOrientation( SwarmVehicle* s, float t );
	
	bool m_debugShowForces;
	void EnableSwarmForceDebug( bool enable );
};
TYPEDEF_BLUECLASS( EveSwarm );

#endif
