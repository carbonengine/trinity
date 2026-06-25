// Copyright © 2023 CCP ehf.

#pragma once

#include "Eve/SpaceObject/Children/EveChildBehaviorSystem.h"
#include "TriFrustum.h"
#include "EveKDdroneManagementTree.h"
#include "Eve/SpaceObject/EveSpaceObject2.h"
#include "Eve/EveEntity.h"

struct ITr2Renderable;

BLUE_DECLARE( BehaviorGroupBooster );
BLUE_DECLARE( EveChildBehaviorSystem );
BLUE_DECLARE( Tr2Mesh );
BLUE_DECLARE_INTERFACE( IBehavior );
BLUE_DECLARE_IVECTOR( IBehavior );
BLUE_DECLARE( KDdroneManagementTree );
BLUE_DECLARE( Tr2LightManager );
BLUE_DECLARE( Tr2QuadRenderer );
BLUE_DECLARE( PlayFX );


BLUE_CLASS( BehaviorGroup ) :
	public IInitialize,
	public IListNotify,
	public INotify,
	public ITr2LightOwner,
	public EveEntity
{
public:
	EXPOSE_TO_BLUE();
	BehaviorGroup( IRoot* lockobj = nullptr );
	~BehaviorGroup();


	/////////////////////////////////////////////////////////////////////////////////////
	// IInitialize
	bool Initialize();

	/////////////////////////////////////////////////////////////////////////////////////
	// INotify
	bool OnModified( Be::Var * value );

	/////////////////////////////////////////////////////////////////////////////////////
	// IListNotify
	void OnListModified(
		long event, // BLUELISTEVENT values
		ssize_t key,
		ssize_t key2,
		IRoot* value,
		const struct IList* theList );

	/////////////////////////////////////////////////////////////////////////////////////
	// ITr2DebugRenderable
	virtual void GetDebugOptions( Tr2DebugRendererOptions & options );
	virtual void RenderDebugInfo( ITr2DebugRenderer2 & renderer, Matrix & parentWorldLocation );

	/////////////////////////////////////////////////////////////////////////////////////
	// BehaviorGroup
	void AddAgent();
	void AddAgents( const std::vector<Vector3>& positions );
	void RemoveAgent();
	void RemoveSpecificAgent( int index );
	void UpdateAsyncronous( const EveUpdateContext& updateContext );
	void UpdateSyncronous( const EveUpdateContext& updateContext, const EveChildUpdateParams& params );
	void UpdateAgents( const float dt, EveChildBehaviorSystem& system );
	float AllTheSame();
	bool IsGroupVisible() const;
	void CreateVertexDeclaration();

	void RegisterWithQuadRenderer( Tr2QuadRenderer & quadRenderer );
	void AddQuadsToQuadRenderer( const TriFrustum& frustum, Tr2QuadRenderer& quadRenderer ) const;

	// Getters
	size_t GetSize();
	float GetMaxVelocity() const;
	float GetBoundingSphereRadius();
	EveKDdroneManagementTreePtr GetKDTree();
	unsigned int GetCount();
	IBehavior* GetBehaviorByName( const std::string& name );
	int GetGroupIndexIndicator() const;
	unsigned int GetVertexDeclarationHandle() const;

	void GetShipInfoForBuffer( uint8_t* data, const Matrix& parentWorldLocation );
	void GetBoosterInfoForBuffer( uint8_t* data, const Matrix& parentWorldLocation );

	void GetRenderables( std::vector<ITr2Renderable*> & renderables );

	// Setters
	void SetCount( int count );
	void SetGroupIndexIndicator( int index );
	void SetVertexFunctionReferance( const std::function<void( void )>& F );

	// Variables
	Matrix m_parentTransform;
	bool m_display;
	bool m_update; // we can have static drones so in those cases we don't want to update the behaviors and kd tree
	float m_estimatedPixelDiameter;
	bool m_collectForces; // Bool toggle to skip bunch of calculations when debug is not being used

	/////////////////////////////////////////////////////////////////////////////////////
	// Geometry Resource
	void InitializeGeometryResource();
	Tr2MeshPtr GetMesh() const;

	void UpdateVisibility( const EveUpdateContext& updateContext, const Matrix& parentTransform );

	void SetupRenderables();

	BehaviorGroupBoosterPtr GetBooster() const;

	/////////////////////////////////////////////////////////////////////////////////////
	// ITr2LightOwner
	void GetLights( Tr2LightManager & lightManager ) const override;
	void AddLight( Tr2Light * light ) override{};
	void ClearLights() override {};

	//////////////////////////////////////////////////////////////////////////////////////
	// EveEntity
	void RegisterComponents() override;
	void UnRegisterComponents() override;

	Vector3 m_spawnPosition;

	EveSpaceObject2* m_parent;
	EveSpaceObject2* GetParent();


private:
	/////////////////////////////////////////////////////////////////////////////////////
	// BehaviorGroup
	void CreateAgentTree();
	void AddAgentPrivate();
	void OnAgentCountChanged();
	void SortBehaviorIndexes();
	void ReleaseCachedData( BlueAsyncRes* );
	void RebuildCachedData( BlueAsyncRes* );
	float GetBlendModifier() const;
	void SetPlayFXBehavior();
	void AddAgentsByCount( int count );
	void RemoveAgentsByCount( int count );


	// Variables
	BlueSharedString m_behaviorGroupName; // name to identify group
	int32_t m_count; // Number of agents to spawn initially
	int32_t m_actualCount; // Number of actual agents spawned for this system
	bool m_updatedOnce; // We want to update the behaviors at least once so behaviors like SpawnDrones can populate the buffer
	int m_groupIndex; // ID
	Tr2MeshPtr m_mesh; // Instanced mesh
	unsigned int m_cachedVD; // A cached Vertex Declaration to detect change
	PIBehaviorVector m_behaviors; // Autonomous systems for the AgentGroup
	std::vector<int> m_sortedBehaviorIndexes; // A sorted list by processPriority
	std::vector<DroneAgent> m_agents; // The agents
	std::vector<CcpMallocBuffer> m_scratchData; // Additional data for each behavior
	unsigned int m_vertexDeclarationHandle; // VertexDeclHandle for the BehaviorGroup agent mesh
	std::function<void()> m_changeBufferVertexCount; // A reference to a function on the parent class
	float m_maxVelocity; // Steering behavior characteristics
	float m_boundingSphereRadius;
	bool m_createAgentTree;
	PlayFX* m_playFXBehavior;

	// Lod-ing
	float m_scale; // Size Multiplier for the agent mesh

	// Tr2Debug
	std::vector<Vector3> m_forces; // A debug vector that represents the forces applied to the agent

	// Crossfade blend range
	float m_currentScreenSize; // READONLY attribute to show artist what the current agent screen size
	float m_renderThreshold; // Do not render group if all agents have a screen size below this threshold.
	float m_blendScreenSizeMin; // If mesh screen size (in pixels) is smaller than this, it will be drawn as a sprite
	float m_blendScreenSizeMax; // If mesh screen size exceeds this, it will be drawn as mesh

	// Spatial partitioning manager/tree
	EveKDdroneManagementTreePtr m_tree;

	// boosters
	BehaviorGroupBoosterPtr m_booster;
	std::map<uint32_t, Vector4> m_lightInfo;

	// debug stuff
	bool m_debugMode;
	float m_debugLodLevel;
	float m_debugIntensity;
};

TYPEDEF_BLUECLASS( BehaviorGroup );
