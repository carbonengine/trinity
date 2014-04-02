////////////////////////////////////////////////////////////
//
//    Created:   December 2011
//    Copyright: CCP 2011
//

#pragma once

#include "Tr2ParticleElementDeclaration.h"
#include "Tr2DeviceResource.h"
#include "include/ITr2InstanceData.h"
#include "include/ITr2GpuBuffer.h"

BLUE_DECLARE_INTERFACE( ITr2ParticleForce );
BLUE_DECLARE_IVECTOR( ITr2ParticleForce );
BLUE_DECLARE_INTERFACE( ITr2GenericParticleConstraint );
BLUE_DECLARE_IVECTOR( ITr2GenericParticleConstraint );
BLUE_DECLARE_INTERFACE( ITr2GenericEmitter );
BLUE_DECLARE_IVECTOR( ITr2GenericEmitter );

// --------------------------------------------------------------------------------------
// Description:
//   Tr2ParticleSystem represents a particle system. Similar to Tr2SpriteParticleSystem,
//   but allows arbitrary data for each particle.
// See Also:
//   Tr2SpriteParticleSystem
// --------------------------------------------------------------------------------------
BLUE_CLASS( Tr2ParticleSystem ):
	public IInitialize,
	public INotify,
	public IListNotify,
	public Tr2DeviceResource,
	public ITr2InstanceData,
	public ITr2GpuBuffer,
	public ISimTimeRebaseNotify
{
public:
	EXPOSE_TO_BLUE();

	using IInitialize::Lock;
	using IInitialize::Unlock;

	Tr2ParticleSystem( IRoot* lockobj = 0 );
	~Tr2ParticleSystem();

	//////////////////////////////////////////////////////////////////////////////////////
	// IInitialize
	bool Initialize();

	/////////////////////////////////////////////////////////////////////////////////////
	// INotify
	bool OnModified( Be::Var* val );

	/////////////////////////////////////////////////////////////////////////////////////
	// IListNotify
	void OnListModified(
		long event,
		ssize_t key,
		ssize_t key2,
		IRoot* value,
		const IList* theList
		);

	//////////////////////////////////////////////////////////////////////////////////////////
	// ITriDeviceResource
	void ReleaseResources( TriStorage s );

	//////////////////////////////////////////////////////////////////////////////////////////
	// ISimTimeRebaseNotify
	void OnSimClockRebase( Be::Time oldTime, Be::Time newTime );
private:
	bool OnPrepareResources();
public:
	//////////////////////////////////////////////////////////////////////////

	/////////////////////////////////////////////////////////////////////////////////////
	// ITr2InstanceData
	bool IsInstanceDataReady() const;
	unsigned int GetInstanceBufferCount() const;
	unsigned int GetInstanceBufferVertexDeclaration( unsigned int bufferIndex ) const;
	unsigned int GetInstanceBufferVertexCount( unsigned int bufferIndex ) const;
	void GetVertexBuffer( unsigned int bufferIndex, Tr2VertexBufferAL*& buffer, unsigned& stride );

	/////////////////////////////////////////////////////////////////////////////////////
	// ITr2GpuBuffer
	Tr2GpuBufferAL* GetGpuBuffer( unsigned index );

	void UpdateViewDependentData( const Matrix& worldTransform );

	void UpdateElementDeclaration();
	const Tr2ParticleElementDataMap& GetElementDeclaration() const;
	unsigned GetElementDeclarationHash() const;
	bool InsertParticle( float** particle );
	void DoneInsertingParticle();
	bool IsValid() const;
	bool IsDynamic() const;
	void ClearParticles();
	void SaveToGranny( const char* resPath ) const;

	size_t GetGpuStride() const;

	bool GetBoundingBox( Vector3 &minBounds, Vector3 &maxBounds ) const;

	void RebindConstraints();

	void SetThreadSafeFlag();

	static void UpdateAllSystems( Be::Time time );


	// ----------------------------------------------------------------------------------
	// Description:
	//   A cheap thread-safe lock-free pseudo random number generator based on 
	//   http://www.firstpr.com.au/dsp/rand31/ 
	// Return Value:
	//   Random integer between 0 and 2^31.
	// ----------------------------------------------------------------------------------
	static inline uint32_t RandCheap()
	{
		static CcpAtomic<uint32_t> globalSeed( rand() );
		// We increment global seed here so that if two threads get to this point
		// simultaneously they would still get different results.
		uint32_t seed = globalSeed;
		uint32_t hi, lo;
		lo = 16807 * ( seed & 0xffff );
		hi = 16807 * ( seed >> 16 );
		lo += ( hi & 0x7fff ) << 16;
		lo += hi >> 15;
		lo = (lo & 0x7FFFFFFF) + (lo >> 31);
		globalSeed = static_cast<uint32_t>( lo );
		return lo;
	}

	// ----------------------------------------------------------------------------------
	// Description:
	//   A cheap thread-safe lock-free pseudo random number generator that produces 
	//   random floats in [0, 1] range. Based on Tr2ParticleSystem::RandCheap.
	// Return Value:
	//   Random float between 0 and 1.
	// See also:
	//   Tr2ParticleSystem::RandCheap
	// ----------------------------------------------------------------------------------
	static inline float RandFloat()
	{
		uint32_t r = RandCheap() & 0x7FFFFFFF;
		return float( r ) / float( 0x7FFFFFFF );
	}
private:
	void Update( Be::Time time );
	void UpdateSimulation( float dt );
	void BuildBuffers();
	void DestroyBuffers();
	void RebuildDeclaration();
	bool GetElementStream( Tr2ParticleElementDeclarationName::Type type, float*& element, unsigned& stride );
	bool HasElement( Tr2ParticleElementDeclarationName::Type type ) const;
	void EnsureAligned();
	void ShiftOffsets( Tr2ParticleElementData::BufferType bufferType, unsigned start, int shift );
	bool FASTCALL CompareParticles( unsigned particle1, unsigned particle2 ) const;
	bool CreateVertexBuffer();

	static std::set<Tr2ParticleSystem*>& GetAllSystems();

	// Python-exposed system name
	std::string m_name;

	// Python-exposed list of particle elements
	PTr2ParticleElementDeclarationVector m_elements;
	// Element declaration hash (changes when elements are rebound)
	unsigned m_declarationHash;

	// Particle element declaration filled when elements are rebound
	Tr2ParticleElementDataMap m_elementMap;
	// Particle element declaration for semantic elements
	Tr2ParticleElementData m_semanticElements[Tr2ParticleElementDeclarationName::CUSTOM];

	// Vertex sizes for different buffers
	unsigned m_vertexSizes[Tr2ParticleElementData::COUNT];
	// Buffers with particle data
	float* m_buffers[Tr2ParticleElementData::COUNT];
	// Sorted particle indexes (only used when sorting)
	unsigned* m_indexes;

	// Maximum number of particles in the system
	unsigned m_maxParticleCount;
	void SetMaxParticleCount( unsigned maxParticleCount );
	unsigned GetMaxParticleCount() const;

	// Number of alive particles in the system
	unsigned m_aliveCount;
	// Global alive particles
	static int s_totalParticleCount;

	// Vertex declaration handle
	unsigned m_declaration;
	// Vertex buffer
	Tr2VertexBufferAL m_vertexBuffer;
	Tr2GpuBufferAL m_uavBuffer;
	// If vertex buffer needs to be updated
	bool m_bufferDirty;

	// List of forces
	PITr2ParticleForceVector m_forces;
	// List of particle constraints
	PITr2GenericParticleConstraintVector m_constraints;

	// Emitter to emit stuff along each particle
	ITr2GenericEmitterPtr m_emissionWhileAliveEmitter;
	// Emitter to emit stuff when particle dies
	ITr2GenericEmitterPtr m_emissionOnDeathEmitter;

	// Mutex for inserting/deleting particles
	CcpMutex* m_insertDeleteMutex;

	// Time of the last Update call
	Be::Time m_lastUpdate;

	// Does particle age change
	bool m_applyAging;
	// Do particles move along their velocity vectors
	bool m_updateSimulation;
	// Do we apply forces to particles
	bool m_applyForce;
	// Does the system require view-dependant sorting
	bool m_requiresSorting;
	// Have we temporarily disabled sorting based on frame time?
	bool m_sortingAllowed;
	// Is this a global particle system?
	bool m_isGlobal;

	// Did last call to UpdateElementDeclaration succeeded
	bool m_isValid;
	// Previous sorting reference point (only used during sorting)
	XMVECTOR* m_sortingReferencePoint;

	bool m_updateBoundingBox;
	// Bounding box for the system
	Vector3 m_AabbMin;
	Vector3 m_AabbMax;

	// Allow updates to occur at different frequencies if needed
	unsigned m_updatePeriod;
	unsigned m_updatePeriodClock;

	// Peak live particles
	unsigned m_peakAliveCount;

	bool m_useSimTimeRebase;
	bool m_isUsingSimTimeRebase;
};

TYPEDEF_BLUECLASS( Tr2ParticleSystem );
BLUE_DECLARE_VECTOR( Tr2ParticleSystem );