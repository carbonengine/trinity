////////////////////////////////////////////////////////////
//
//    Created:   November 2017
//    Copyright: CCP 2017
//
#pragma once
#ifndef EveHazeSet_H
#define EveHazeSet_H

#include "IEveSpaceObjectAttachment.h"
#include "ITr2GeometryProvider.h"
#include "ITr2Renderable.h"
#include "Utilities/BoundingBox.h"
#include "EveHazeSetItem.h"

// forwards
BLUE_DECLARE( Tr2Effect );
BLUE_DECLARE( TriFrustum );
BLUE_DECLARE( Tr2DebugRenderer );
struct ViewDistanceInfo;

class Tr2PerObjectData;

// --------------------------------------------------------------------------------
// Description:
//   This class is for rendering all of one ship's haze sets.
// SeeAlso:
//   EveHazeSetItem
// --------------------------------------------------------------------------------
BLUE_CLASS( EveHazeSet ) :
	public IEveSpaceObjectAttachment,
	public IInitialize,
	public ITr2GeometryProvider,
	public Tr2DeviceResource
{
public:
	EXPOSE_TO_BLUE();

	using IInitialize::Lock;
	using IInitialize::Unlock;

	EveHazeSet( IRoot* lockobj = NULL );
	~EveHazeSet();

	//////////////////////////////////////////////////////////////////////////////////////
	// IInitialize
	bool Initialize();

	//////////////////////////////////////////////////////////////////////////////////////
	// ITr2GeometryProvider
	void SubmitGeometry( Tr2RenderContext& renderContext );

	//////////////////////////////////////////////////////////////////////////////////////
	// ITriDeviceResource
	void ReleaseResources( TriStorage s );

	//////////////////////////////////////////////////////////////////////////////////////
	// IEveSpaceObjectAttachment
	virtual bool UpdateVisibility( const TriFrustum& frustum, const Matrix& parentTransform, const granny_matrix_3x4* bones, size_t boneCount );
	virtual void GetBatches( ITriRenderBatchAccumulator * accumulator, TriBatchType batchType, const Tr2PerObjectData* perObjectData, Tr2RenderReason reason = Tr2RenderReason::TR2RENDERREASON_NORMAL );
	virtual void GetDebugOptions( Tr2DebugRendererOptions& options );
	virtual void RenderDebugInfo( ITr2DebugRenderer2& renderer, const Matrix& parentTransform, const granny_matrix_3x4* bones, size_t boneCount );
	void SetShaderOption( const BlueSharedString& name, const BlueSharedString& value ) override;

	// setup
	void Setup( Tr2EffectPtr effect );

	// access items
	void AddHazeItem( EveHazeSetItemPtr item );

	// rebuild the interal vertexbuffers etc.
	void Rebuild();

	// picking
	void GetPickingBatches( ITriRenderBatchAccumulator* batches, uint16_t& areaIDOffset, const Tr2PerObjectData* perObjectData );

private:
	bool OnPrepareResources();

	// toggle visibility
	bool m_display;
	// keep a name
	std::string m_name;

	// the list of all them haze items
	PEveHazeSetItemVector m_hazes;
	// transforms for each of the hazes
	std::vector<Matrix> m_cachedTransforms;
	// this shader renders them all
	Tr2EffectPtr m_effect;

	// has it's own vertex handle and buffer
	unsigned int m_vertexDeclHandle;
	unsigned int m_vertexCount;
	Tr2BufferAL m_vertexBuffer;

	// bounding box around static items
	AxisAlignedBoundingBox m_aabb;
	std::vector<std::pair<int, CcpMath::AxisAlignedBox>> m_boundingBoxes;

	void CreateBoundingBox();
};

TYPEDEF_BLUECLASS( EveHazeSet );

#endif // EveHazeSet_H
