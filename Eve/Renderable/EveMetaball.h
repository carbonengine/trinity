////////////////////////////////////////////////////////////
//
//    Created:   January 2014
//    Copyright: CCP 2014
//
#pragma once
#ifndef EveMetaball_H
#define EveMetaball_H

#include "ITr2Renderable.h"
#include "ITr2GeometryProvider.h"
#include "Eve/IEveSpaceObject2.h"
#include "EveMetaballItem.h"
#include "Tr2Renderer.h"
#include "Tr2PersistentPerObjectData.h"

// --------------------------------------------------------------------------------
// Description:
//   This class is for rendering metaballs (aka isosurfaces)
// --------------------------------------------------------------------------------
BLUE_CLASS( EveMetaball ) :
	public IEveSpaceObject2,
	public ITr2Renderable,
	public ITr2GeometryProvider,
	public Tr2DeviceResource,
	public IInitialize
{
public:
	EXPOSE_TO_BLUE();

	EveMetaball(IRoot* lockobj = NULL);
	~EveMetaball();
	
	//////////////////////////////////////////////////////////////////////////////////////
	// IEveSpaceObject2
	void UpdateSyncronous( EveUpdateContext& updateContext );
	void UpdateAsyncronous( EveUpdateContext& updateContext );
	void UpdateVisibility( const TriFrustum& frustum, const Matrix& parentTransform );
	void GetRenderables( std::vector<ITr2Renderable*>& renderables, Tr2ImpostorManager* impostors );
	bool GetBoundingSphere( Vector4& sphere, BoundingSphereQuery query=EVE_BOUNDS_NORMAL ) const;
	void RenderDebugInfo( Tr2RenderContext& renderContext );
	void UpdateModelCenterWorldPosition( Vector3 &position, Be::Time t ) {}
	void GetModelCenterWorldPosition( Vector3 &position ) const {}
	bool GetLocalBoundingBox( Vector3 &min, Vector3 &max ) { return false; }
	void GetLocalToWorldTransform( Matrix &transform ) const { D3DXMatrixIdentity( &transform ); }

	/////////////////////////////////////////////////////////////////////////////////////
	// per-object data
	Tr2PersistentPerObjectData<EveMetaball> m_perObjectDataVs;
	Tr2PersistentPerObjectData<EveMetaball> m_perObjectDataPs;
	uint32_t GetPerObjectDataSize( Tr2RenderContextEnum::ShaderType shaderType ) const;
	void UpdatePerObjectBuffer( Tr2RenderContextEnum::ShaderType shaderType, uint32_t size, void* );

	//////////////////////////////////////////////////////////////////////////////////////
	// ITr2GeometryProvider
	virtual void SubmitGeometry( Tr2RenderContext& renderContext );

	/////////////////////////////////////////////////////////////////////////////////////
	// ITr2Renderable
	virtual void GetBatches( ITriRenderBatchAccumulator* batches, TriBatchType batchType, const Tr2PerObjectData* perObjectData );
	virtual bool HasTransparentBatches();
	virtual float GetSortValue();
	virtual Tr2PerObjectData* GetPerObjectData( ITriRenderBatchAccumulator* accumulator );

	//////////////////////////////////////////////////////////////////////////////////////
	// ITriDeviceResource
	virtual void ReleaseResources( TriStorage s );
private:
	bool OnPrepareResources();
public:

	//////////////////////////////////////////////////////////////////////////
	// IInitialize
	virtual bool Initialize();

	// update all the index and vertex buffers to the latest density field
	void UpdateBuffers();

	struct Cell
	{
		unsigned int mask;
		Vector3 position[8];
		float value[8];
		int x;
		int y;
		int z;
	};

private:
	// vertex
	struct Vertex
	{
		Vector3 position;
		Vector3 normal;
	};

	// metaball triangle
	struct Triangle
	{
		Vector3 position[3];
		Vector3 normal[3];
	};

	float GetGridValue( Vector3 coordinate );
	void Triangulate( const Cell* cell );
	void March();
	Cell* CheckCell( int x, int y, int z, int* sharedVerts, Cell* fromCell );
	void RecursiveCheckCell( int x, int y, int z, int* sharedVerts, Cell* cell );
	Vector3 CalculateNormals( Vector3 position );

	std::vector< Triangle > m_triangles;
	std::map< unsigned int, Cell* > m_cellCache;

	int m_gridSizeX;
	int m_gridSizeY;
	int m_gridSizeZ;

	int m_cellCounter;

	// general
	std::string m_name;
	bool m_display;
	bool m_isVisible;

	// debug
	float m_boxSize;

	float m_isoValue;
	float m_gooValue;

	// metaball data
	unsigned int m_triangleCount;

	// shaders
	Tr2MaterialPtr m_distortionEffect;
	Tr2MaterialPtr m_additiveEffect;
	Tr2MaterialPtr m_transparentEffect;

	// resulting transform
	Matrix m_worldTransform;

	// bounding sphere
	Vector4 m_boundingSphere;

	// bounding box
	Vector3 m_minBounds;
	Vector3 m_maxBounds;

	// vertex buffer
	unsigned int m_vertexDeclHandle;
	Tr2VertexBufferAL m_vertexBuffer;

	// index buffer
	Tr2IndexBufferAL m_indexBuffer;

	// the source data
	PEveMetaballItemVector m_sourceItems;

	// Update bounds
	void AddToBoundingBox( EveMetaballItemPtr item );
};

TYPEDEF_BLUECLASS( EveMetaball );
BLUE_DECLARE_VECTOR( EveMetaball );

#endif