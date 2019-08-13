#pragma once

#include "Tr2DebugRenderer.h"
#include "Eve/Volume/IEveVolume.h"
#include "Eve/SpaceObject/Children/EveChildBehaviorSystem.h"
#include <functional>
#include "TriFrustum.h"
#include "Eve/SpaceObject/Utils/EveLocatorSets.h"
#include "KDdroneManagementTree.h"


struct DroneAgent
{

	DroneAgent() :
		mass( 1.f ),
		position( 0, 0, 0 ),
		rotation( 0, 0, 0, 1 ),
		acceleration( 0, 0, 0 ),
		velocity( 0, 0, 0 ),
		target( 0, 0, 0 ),
		lifetime( 0.f ),
		xfade( 0.f ),
		id( 0 ),
		isVisible( false ),
		screenSize( 0.f ),
		tunnelLock( -1 ),
		tunnelPoint( 0 ),
		seek( true ),
		deliver( false ),
		arrived( true ),
		avoidanceWeight( 75.f ),
		lastAcceleration( 0, 0, 0 ),
		hasUsedEntryTunnels( false ),
		hasUsedExitTunnels( false )
	{}

	float mass;
	Vector3 position;
	Quaternion rotation;
	Vector3 acceleration;
	Vector3 velocity;
	Vector3 target;
	float lifetime;
	float xfade; // Crossfade between mesh and sprite. 1.0 = mesh, 0.0 = sprite
	int id;
	bool isVisible; // Don't render agents off-screen
	float screenSize;
	
	// SplineTunnels
	int tunnelLock;
	int tunnelPoint;

	// ProcessLifetime
	bool hasUsedEntryTunnels;
	bool hasUsedExitTunnels;

	//This will need to be moved to an extra attribute array
	bool seek;
	bool deliver;
	bool arrived;
	float avoidanceWeight;
	Vector3 lastAcceleration;
};



struct BehaviorProperties
{
	BehaviorProperties() :
		target( 0, 0, 0 )
	{}

	Vector3 target;
};

struct ITr2Renderable;

BLUE_DECLARE( BehaviorGroup );
BLUE_DECLARE( EveChildBehaviorSystem );
BLUE_DECLARE( Tr2Mesh );
BLUE_DECLARE( TriGeometryRes );
BLUE_DECLARE_INTERFACE( IBehavior );
BLUE_DECLARE_IVECTOR( IBehavior );
BLUE_DECLARE_INTERFACE( IEveVolume );
BLUE_DECLARE_IVECTOR( IEveVolume );
BLUE_DECLARE( EveLocatorSets );
BLUE_DECLARE_VECTOR( EveLocatorSets );
BLUE_DECLARE( KDdroneManagementTree );

BLUE_CLASS( BehaviorGroup ) :
	public IRoot
{
public:
	EXPOSE_TO_BLUE();
	BehaviorGroup( IRoot* lockobj = nullptr );
	~BehaviorGroup();



	/////////////////////////////////////////////////////////////////////////////////////
	// ITr2DebugRenderable
	virtual void GetDebugOptions( Tr2DebugRendererOptions& options );
	float GetBoundingSphereRadius();
	virtual void RenderDebugInfo( Tr2DebugRenderer& renderer, Matrix& parentWorldLocation );

	void UpdateVisibility( const TriFrustum& frustum, const Matrix& parentTransform );

	// geom res
	void InitializeGeometryResource();
	Tr2MeshPtr GetMesh() const;
	Tr2MeshPtr GetSpriteMesh() const;

	void AddAgent();
	size_t GetSize();
	unsigned int GetCount();
	void CreateAgentTree();
	void SetMeshToggle( bool toggle );
	void RemoveAgent();
	void SetCount( int count );
	float AllTheSame();
	int GetGroupIndexIndicator() const;
	void CreateVertexDeclaration();
	void ReleaseCachedData( BlueAsyncRes* );
	void RebuildCachedData( BlueAsyncRes* );
	void SetGroupIndexIndicator( int index );
	void UpdateAgents(const float dt, EveChildBehaviorSystem& system);
	void ProcessLOD( DroneAgent& agent );
	void SetBlendRange( float min, float max );
	unsigned int GetVertexDeclarationHandle() const;
	unsigned GetSpriteVertexDeclarationHandle() const;
	void SetVertexFunctionReferance( const std::function<void( void )>& F );
	void GetInfoForBuffer( uint8_t* data, Matrix& parentWorldLocation );
	void CreateSpriteVertexDeclaration();

	bool m_display;
	bool IsGroupVisible();
	float m_estimatedPixelDiameter;

	// locators
	const LocatorStructureList* GetLocatorsForSet( const BlueSharedString& setName ) const;
	PEveLocatorSetsVector m_locatorSets;

	// Behavior data, public so behaviors can use it
	BehaviorProperties m_behavior;

	PIEveVolumeVector m_exclusionVolumes;

private:
	void ToggleMesh();
	void AddAgentPrivate();
	Vector3 RemoveAgentPrivate();

	int m_count; // Number of agents
	Vector3 m_scale; // Size Multiplier for the agent mesh
	Vector3 m_spriteScale; // Size Multiplier for the sprite mesh
	bool m_collectForces; // Bool toggle to skip bunch of calculations when debug is not being used
	int m_groupIndex; // ID
	bool m_meshToggle; // To configure sprite during development
	Tr2MeshPtr m_mesh;
	Tr2MeshPtr m_spriteMesh;
	unsigned int m_cachedVD; // A cached Vertex Declaration to detect change
	unsigned int m_cachedSVD; // A cached Vertex Declaration for the sprite to detect change
	BlueSharedString m_name; // A string so you can find the thing by name
	PIEveVolumeVector m_volumes; // Probably moved soon to the behavior system since this is a global thing for all groups
	PIBehaviorVector m_behaviors; // AI systems for the AgentGroup
	std::vector<DroneAgent> m_agents; // The agents
	unsigned int m_spriteVertexDeclarationHandle; // VertexDeclHandle for the BehaviorGroup sprite mesh 
	unsigned int m_vertexDeclarationHandle; // VertexDeclHandle for the BehaviorGroup agent mesh 
	std::function<void()> m_changeBufferVertexCount; // A reference to a function on the parent class

	// Tr2Debug 
	std::vector<Vector3> m_forces; // A debug vector that represents the forces applied to the agent 


	// Steering behavior characteristics, this could actually go under the vehicle struct
	float m_maxVelocity;

	// Blend range
	float m_blendRangeMax; // Effectively the distance threshold
	float m_blendRangeMin; // The distance where a drone should stop having a mesh and be fully rendered as a light.
	float m_blendRangeValue; // Normalized 0.0 - 1.0 from blendRangeMin to blendRangeMax;

	// Crossfade blend range
	float m_currentScreenSize;  // READONLY attribute to show artist what the current agent screen size
	float m_renderThreshold;	// Do not render group if all agents have a screen size below this threshold.
	float m_blendScreenSizeMin; // If mesh screen size (in pixels) is smaller than this, it will be drawn as a sprite
	float m_blendScreenSizeMax; // If mesh screen size exceeds this, it will be drawn as mesh
	float m_xfadeValue;			// Normalized 0.0 - 1.0 from m_pixelSizeMin to m_pixelSizeMax;

	void UpdateCurrentScreenSize();

	// Bounding sphere
	Vector3 m_boundingSphereCenter;
	float m_boundingSphereRadius;

	// Spatial partitioning manager/tree
	KDdroneManagementTreePtr m_tree;

};

TYPEDEF_BLUECLASS( BehaviorGroup );

