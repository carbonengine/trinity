////////////////////////////////////////////////////////////
//
//    Created:   June 2019
//    Copyright: CCP 2019
//

#pragma once

#include "IEveSpaceObjectChild.h"
#include "EveChildTransform.h"
#include "Tr2DebugRenderer.h"
#include "TriRenderBatch.h"
#include "Tr2Mesh.h"
#include "Eve/SpaceObject/EveSpaceObject2.h"
#include "Tr2PerObjectData.h"
#include "Eve/UI/EveCurveLineSet.h"
#include "LineSetPaths/IEveLineSetPath.h"

class ChildLineSetInstancingBatch;



BLUE_CLASS( EveChildLineSet ) :
	public IInitialize,
	public IEveSpaceObjectChild,
	public Tr2DeviceResource,
	public ITr2Renderable,
	public EveChildTransform,
	public INotify,
	public ITr2DebugRenderable
{
public:
	EXPOSE_TO_BLUE();

	EveChildLineSet( IRoot* lockobj = NULL );
	~EveChildLineSet();

	//////////////////////////////////////////////////////////////////////////////////////
	// IInitialize
	bool Initialize() override;

	//////////////////////////////////////////////////////////////////////////////////////
	// INotify
	bool OnModified( Be::Var* value ) override;

	//////////////////////////////////////////////////////////////////////////////////////
	// IEveSpaceObjectChild
	const char* GetName() const override;
	void SetName( const char* name ) override;

	void SetShaderOption( const BlueSharedString& name, const BlueSharedString& value ) override;
	bool IsAlwaysOn() const override;
	void Setup( const Vector3* scale, const Quaternion* rotation, const Vector3* translation, Tr2Lod lowestLodVisible ) override;
	void GetLights( Tr2LightManager & lightManager ) const override {};
	void GetLocalToWorldTransform( Matrix & transform ) const override;

	bool GetBoundingSphere( Vector4 & sphere, BoundingSphereQuery query = EVE_BOUNDS_NORMAL ) const override;
	void UpdateVisibility( const TriFrustum& frustum, const Matrix& parentTransform, Tr2Lod parentLod ) override;
	void AddQuadsToQuadRenderer( const TriFrustum& frustum, Tr2QuadRenderer& quadRenderer ) const override {};
	void GetRenderables( std::vector<ITr2Renderable*> & renderables ) override;
	void RegisterWithQuadRenderer( Tr2QuadRenderer & quadRenderer ) override {};

	/////////////////////////////////////////////////////////////////////////////////////
	// Tr2DeviceResource
	void ReleaseResources( TriStorage s ) override;
	bool OnPrepareResources() override;

	/////////////////////////////////////////////////////////////////////////////////////
	// ITr2Renderable
	Tr2PerObjectData* GetPerObjectData( ITriRenderBatchAccumulator * accumulator ) override;
	virtual void GetBatches( ITriRenderBatchAccumulator * batches, TriBatchType batchType, const Tr2PerObjectData* perObjectData, Tr2RenderReason reason = TR2RENDERREASON_NORMAL ) override;
	bool HasTransparentBatches() override;


	/////////////////////////////////////////////////////////////////////////////////////
	// update
	void UpdateSyncronous( EveUpdateContext & updateContext, const EveChildUpdateParams& params ) override;
	void UpdateAsyncronous( EveUpdateContext & updateContext, const EveChildUpdateParams& params ) override;
	void ChangeLOD( Tr2Lod lod ) override;


	/////////////////////////////////////////////////////////////////////////////////////
	// Debug
	void GetDebugOptions( Tr2DebugRendererOptions& options ) override;
	void RenderDebugInfo( ITr2DebugRenderer2& renderer ) override;

	void GetWorldVelocity( Vector3& velocity ) const;

	/////////////////////////////////////////////////////////////////////////////////////
	// EveChildLineSet
	Tr2MeshPtr GetMesh() const;
	float GetOwnerMaxSpeed() const;
	void CreateSpriteVertexDeclaration();
	float GetSortValue() { return 0.f; };
	void UpdateBuffer( Tr2RenderContext& renderContext );
	void Draw( ChildLineSetInstancingBatch* batch, Tr2RenderContext& renderContext );
	std::vector<std::pair<int, int>> GetVertexElementAddedThroughCode() const;

	enum lineSetType { OBJECT_RENDER, LINE_RENDER, BOTH };

private:

	void InitializeLineSet();
	void GenerateManagedPoints();
	void UpdateBoundingSphere( bool reCalculateChildren = true );

	BlueSharedString m_name;
	Vector3 m_worldVelocity;
	float m_ownerMaxSpeed;
	
	bool m_display;
	bool m_isAlwaysOn;
	bool m_updateLineSet;
	bool m_scaleSegmentsByCompleteness;

	EveCurveLineSetPtr m_lineSet;
	lineSetType m_type;

	// line render settings
	Vector4 m_baseColor;
	Vector4 m_animColor;
	float m_brightness;
	bool m_additiveBatch;

	// Instance data
	Tr2BufferAL m_vertexBuffer;
	unsigned const m_stride;
	unsigned m_vertexCount;

	Tr2MeshPtr m_mesh;
	unsigned int m_vertexDeclarationHandle;
	unsigned int m_cachedSVD;
	EveSpaceObjectPSData m_psData;
	EveSpaceObjectVSData m_vsData;
	unsigned m_totalObjectCount;

	//animate - lineRender
	float m_scrollSpeed;
	
	// visibility culls
	Vector4 m_boundingSphere;
	float m_currentScreenSize;
	float m_minScreenSize;
	bool m_isVisible;

	PIEveLineSetPathVector m_lines;
};

TYPEDEF_BLUECLASS( EveChildLineSet );