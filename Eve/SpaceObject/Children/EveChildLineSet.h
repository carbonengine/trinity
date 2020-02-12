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
	const char* GetName() const;
	void SetName( const char* name );
	
	void SetShaderOption( const BlueSharedString& name, const BlueSharedString& value ) override;
	bool IsAlwaysOn() const override;
	void Setup( const Vector3* scale, const Quaternion* rotation, const Vector3* translation, Tr2Lod lowestLodVisible );
	void GetLights( Tr2LightManager& lightManager ) const {};
	void GetLocalToWorldTransform( Matrix& transform ) const;
	void AddQuadsToQuadRenderer( const TriFrustum& frustum, Tr2QuadRenderer& quadRenderer ) const {};
	bool GetBoundingSphere( Vector4& sphere, BoundingSphereQuery query = EVE_BOUNDS_NORMAL ) const;
	void UpdateVisibility( const TriFrustum& frustum, const Matrix& parentTransform, Tr2Lod parentLod );
	void GetRenderables( std::vector<ITr2Renderable*>& renderables );
	void RegisterWithQuadRenderer( Tr2QuadRenderer& quadRenderer ) {};

	/////////////////////////////////////////////////////////////////////////////////////
	// Tr2DeviceResource
	void ReleaseResources( TriStorage s ) {};
	bool OnPrepareResources();

	/////////////////////////////////////////////////////////////////////////////////////
	// ITr2Renderable
	Tr2PerObjectData* GetPerObjectData( ITriRenderBatchAccumulator* accumulator );
	virtual void GetBatches( ITriRenderBatchAccumulator* batches, TriBatchType batchType, const Tr2PerObjectData* perObjectData );
	bool HasTransparentBatches();

	
	/////////////////////////////////////////////////////////////////////////////////////
	// update
	void UpdateSyncronous( EveUpdateContext& updateContext, const EveChildUpdateParams& params );
	void UpdateAsyncronous( EveUpdateContext& updateContext, const EveChildUpdateParams& params );
	void UpdateAsyncronous( EveUpdateContext& updateContext, Matrix& parentTransform );
	void ChangeLOD( Tr2Lod lod );

	
	/////////////////////////////////////////////////////////////////////////////////////
	// Debug
	void GetDebugOptions( Tr2DebugRendererOptions& options );
	void RenderDebugInfo( ITr2DebugRenderer2& renderer );

	void GetWorldVelocity( Vector3& velocity ) const;

	/////////////////////////////////////////////////////////////////////////////////////
	// EveChildLineSet
	// 
	Tr2MeshPtr GetMesh() const;
	float GetOwnerMaxSpeed() const;
	void CreateSpriteVertexDeclaration();
	float GetSortValue() { return 0.f; };
	void UpdateBuffer( Tr2RenderContext& renderContext );
	void Draw( ChildLineSetInstancingBatch* batch, Tr2RenderContext& renderContext );

	enum lineSetType { OBJECT_RENDER, LINE_RENDER, BOTH };
	enum lineSetObjType { CIRCLE, BEZIER_CURVE };
	
private:

	void InitializeLineSet();
	void GenerateManagedPoints();
	void InitializeLineSetForCurves();
	void GenerateManagedPointsForCurve();

	BlueSharedString m_name;
	Vector3 m_worldVelocity;
	float m_ownerMaxSpeed;
	bool m_display;
	bool m_isAlwaysOn;

	// circle attributes
	float m_circleRadius;
	int m_numSegments;
	float m_exposedNumSegments;
	Vector4 m_circleDistort;
	float m_completeness;
	
	EveCurveLineSetPtr m_lineSet;
	lineSetType m_type;
	lineSetObjType m_objType;
	std::vector<Vector3> m_managedPoints;
	
	//lines
	float m_lineWidth;
	Vector4 m_baseColor;
	Vector4 m_animColor;
	bool m_additiveBatch;
	
	//obj
	bool m_billboardObject;
	
	// Instance data
	Tr2BufferAL m_vertexBuffer;
	unsigned const m_stride;
	unsigned m_vertexCount;

	Tr2MeshPtr m_mesh;
	unsigned int m_vertexDeclarationHandle;
	unsigned int m_cachedSVD;
	EveSpaceObjectPSData m_psData;
	EveSpaceObjectVSData m_vsData;
	Vector3 m_objectScale;
	
	//animate the scene
	float m_animValue;
	float m_animSpeed;
	float m_scrollSpeed;
	float m_scrollSegmenting;
	float m_brightness;

	// BezierCurve
	Vector3 m_point1;
	Vector3 m_point2;
	Vector3 m_bezierPoint;
	int m_curveSegments;
	float m_exposedCurveSegments;
};

TYPEDEF_BLUECLASS( EveChildLineSet );