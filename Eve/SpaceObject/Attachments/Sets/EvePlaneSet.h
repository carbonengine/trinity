////////////////////////////////////////////////////////////
//
//    Created:   March 2013
//    Copyright: CCP 2013
//
#pragma once
#ifndef EvePlaneSet_H
#define EvePlaneSet_H

#include "IEveSpaceObjectAttachment.h"
#include "ITr2GeometryProvider.h"
#include "ITr2Renderable.h"
#include "Tr2GrannyAnimation.h"
#include "Utilities/BoundingBox.h"

#include "EvePlaneSetItem.h"

// forwards
BLUE_DECLARE( EvePlaneSet );
BLUE_DECLARE( Tr2Effect );
BLUE_DECLARE( TriFrustum );
BLUE_DECLARE( Tr2DebugRenderer );
struct ViewDistanceInfo;

class Tr2PerObjectData;

// --------------------------------------------------------------------------------
// Description:
//   This class is for rendering all of one ship's trails.
//   The object is part of EveBoosterSet2
// SeeAlso:
//   EveBoosterSet2
// --------------------------------------------------------------------------------
BLUE_CLASS( EvePlaneSet ):
	public IEveSpaceObjectAttachment,
	public IInitialize,
	public ITr2GeometryProvider,
	public Tr2DeviceResource,
	public INotify
{
public:
	EXPOSE_TO_BLUE();

	using IInitialize::Lock;
	using IInitialize::Unlock;

	EvePlaneSet( IRoot* lockobj = NULL );
	~EvePlaneSet();

	//////////////////////////////////////////////////////////////////////////////////////
	// IInitialize
	bool Initialize();
	
	//////////////////////////////////////////////////////////////////////////
	// INotify
	bool OnModified( Be::Var* val );

	//////////////////////////////////////////////////////////////////////////////////////
	// ITr2GeometryProvider
	void SubmitGeometry( Tr2RenderContext& renderContext );

	//////////////////////////////////////////////////////////////////////////////////////
	// ITriDeviceResource
	void ReleaseResources( TriStorage s );
private:
	bool OnPrepareResources();

public:
	//////////////////////////////////////////////////////////////////////////////////////
	// IEveSpaceObjectAttachment
	virtual bool UpdateVisibility( const TriFrustum& frustum, const Matrix& parentTransform, const granny_matrix_3x4* bones, size_t boneCount );
	virtual void GetBatches( ITriRenderBatchAccumulator * accumulator, TriBatchType batchType, const Tr2PerObjectData* perObjectData, Tr2RenderReason reason = Tr2RenderReason::TR2RENDERREASON_NORMAL );
	virtual void GetDebugOptions( Tr2DebugRendererOptions& options );
	virtual void RenderDebugInfo( ITr2DebugRenderer2& renderer, const Matrix& parentTransform, const granny_matrix_3x4* bones, size_t boneCount );

	void SetShaderOption( const BlueSharedString& name, const BlueSharedString& value ) override;

	// access effect
	void SetEffect( Tr2EffectPtr effect );

	// access items
	void AddPlaneItem( EvePlaneSetItemPtr item );

	// access pickBufferID
	void SetPickBufferID( uint8_t pickBufferID );

	// rebuild the interal vertexbuffers etc.
	void Rebuild();

	void GetPickingBatches( ITriRenderBatchAccumulator* batches, uint16_t& areaIDOffset, const Tr2PerObjectData* perObjectData );

	EvePlaneSetItemVector* GetPlanes();
private:
	// toggle visibility
	bool m_display;
	bool m_hideOnLowQuality;
	// pickbuffer ID
	uint8_t m_pickBufferID;
	// keep a name
	std::string m_name;

	// bounding box functions
	AxisAlignedBoundingBox GetAabb( const granny_matrix_3x4* bones, size_t boneCount ) const;
	void CreateBoundingBoxes();
	// bounding boxes that are static
	AxisAlignedBoundingBox m_aabb;
	// bounding boxes are grouped together by bone index
	std::vector<std::pair<int, AxisAlignedBoundingBox>> m_boundingBoxes;

	// the list of all them plane items
	PEvePlaneSetItemVector m_planes;
	// transforms for each of the planes
	std::vector<Matrix> m_cachedTransforms;
	// this shader renders or picks them all
	Tr2EffectPtr m_effect;
	Tr2EffectPtr m_pickEffect;

	// has it's own vertex handle and buffer
	unsigned int m_vertexDeclHandle;
	unsigned int m_vertexCount;
	Tr2BufferAL m_vertexBuffer;
};

TYPEDEF_BLUECLASS( EvePlaneSet );

#endif // EvePlaneSet_H