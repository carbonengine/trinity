////////////////////////////////////////////////////////////
//
//    Created:   March 2013
//    Copyright: CCP 2013
//
#pragma once
#ifndef EvePlaneSet_H
#define EvePlaneSet_H

//#include "Tr2DeviceResource.h"
#include "ITr2GeometryProvider.h"
#include "ITr2Renderable.h"

#include "EvePlaneSetItem.h"

// forwards
BLUE_DECLARE( EvePlaneSet );
BLUE_DECLARE( Tr2Effect );
BLUE_DECLARE( TriFrustum );
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
	public IInitialize,
	public ITr2GeometryProvider,
	public Tr2DeviceResource
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
	
	//////////////////////////////////////////////////////////////////////////////////////
	// ITr2GeometryProvider
	void SubmitGeometry( Tr2RenderContext& renderContext );

	//////////////////////////////////////////////////////////////////////////////////////
	// ITriDeviceResource
	void ReleaseResources( TriStorage s );
private:
	bool OnPrepareResources();

public:
	// hand out batches
	void GetBatches( ITriRenderBatchAccumulator* accumulator, const Tr2PerObjectData* perObjectData );

	// access effect
	void SetEffect( Tr2EffectPtr effect );

	// access items
	void AddPlaneItem( EvePlaneSetItemPtr item );

	// rebuild the interal vertexbuffers etc.
	void Rebuild();

	void GetPickingBatches( ITriRenderBatchAccumulator* batches, uint16_t& areaIDOffset, const Tr2PerObjectData* perObjectData );

	EvePlaneSetItemVector* GetPlanes();
private:
	// toggle visibility
	bool m_display;
	bool m_hideOnLowQuality;
	// keep a name
	std::string m_name;

	// the list of all them plane items
	PEvePlaneSetItemVector m_planes;
	// transforms for each of the planes
	std::vector<Matrix> m_cachedTransforms;
	// this shader renders them all
	Tr2EffectPtr m_effect;

	// has it's own vertex handle and buffer
	unsigned int m_vertexDeclHandle;
	unsigned int m_vertexCount;
	Tr2VertexBufferAL m_vertexBuffer;
};

TYPEDEF_BLUECLASS( EvePlaneSet );

#endif // EvePlaneSet_H