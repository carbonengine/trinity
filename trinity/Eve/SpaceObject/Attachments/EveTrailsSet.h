#pragma once
#ifndef EveTrailsSet_H
#define EveTrailsSet_H



#include "ITr2GeometryProvider.h"
#include "Tr2DeviceResource.h"

// forwards
class ITriRenderBatchAccumulator;
class Tr2PerObjectData;
BLUE_DECLARE( Tr2Effect );
BLUE_DECLARE( TriGeometryRes );

// --------------------------------------------------------------------------------
// Description:
//   This class is for rendering all of one ship's trails.
//   The object is part of EveBoosterSet2
// SeeAlso:
//   EveBoosterSet2
// --------------------------------------------------------------------------------
BLUE_CLASS( EveTrailsSet ):
	public IInitialize,
	public INotify,
	public ITr2GeometryProvider,
	public IBlueAsyncResNotifyTarget,
	public Tr2DeviceResource
{
public:
	EXPOSE_TO_BLUE();

	using IInitialize::Lock;
	using IInitialize::Unlock;

	EveTrailsSet( IRoot* lockobj = NULL );
	~EveTrailsSet();

	/////////////////////////////////////////////////////////////////////////////////////
	// IInitialize
	bool Initialize();

	//////////////////////////////////////////////////////////////////////////
	// INotify
	bool OnModified( Be::Var* val );
	
	//////////////////////////////////////////////////////////////////////////////////////
	// ITr2GeometryProvider
	void SubmitGeometry( Tr2RenderContext& renderContext );

	/////////////////////////////////////////////////////////////////////////////////////
	// IBlueAsyncResNotifyTarget
	void ReleaseCachedData( BlueAsyncRes* p );
	void RebuildCachedData( BlueAsyncRes* p );

	//////////////////////////////////////////////////////////////////////////////////////////
	// ITriDeviceResource
	void ReleaseResources( TriStorage s );
private:
	bool OnPrepareResources();

public:
	// vertex data for stream1: transform
	struct InstanceVertex
	{
		Vector4 transform;
	};

	// cleanup
	void Cleanup();
	// timing
	void Update( Be::Time t );
	// manage individual trails
	void Clear();
	void Add( const Matrix* localMatrix, float size );
	// rendering
	void GetBatches( ITriRenderBatchAccumulator* accumulator, const Tr2PerObjectData* perObjectData );

	// query fade speed
	float GetFadeSpeed()			const	{	return m_fadeSpeed;		}
	// set internal visual data
	void SetEffect( Tr2EffectPtr effect );
	void SetMeshResPath( const char* path );


private:
	// all data on a single trail
	struct SingleTrailData
	{
		// where?
		Matrix transform;
		// size
		float size;
	};

	// geom res load
	void InitializeGeometryResource();
	// instance vertex buffer
	void InitializeInstanceBuffer();

	// toggle display
	bool m_display;
	// the shader used for rendering the instanced boosters
	Tr2EffectPtr m_effect;
	// base trail geometry
	TriGeometryResPtr m_geometryResource;
	// base trail geometry res path
	std::string m_geometryResPath;

	// need special vertex declaration for multi-stream rendering
	unsigned int m_trailVertexDeclElementCount;
	Tr2VertexDefinition m_trailVertexDecl;
	unsigned int m_vertexDeclHandle;
	// vertex buffers for multi-stream rendering
	Tr2BufferAL m_instanceBuffer;

	// fade in/out co-efficient
	float m_fadeSpeed;

	// indivual data of each trail
	std::vector<SingleTrailData> m_trailData;
};

TYPEDEF_BLUECLASS( EveTrailsSet );

#endif // EveTrailsSet_H