// Copyright © 2023 CCP ehf.

#pragma once
#ifndef ITr2Renderable_H
#define ITr2Renderable_H

#include "Eve/EveComponentRegistry.h"

class ITriRenderBatchAccumulator;
class Tr2PerObjectData;
class TriPoolAllocator;
class TriFrustum;
class EveUpdateContext;

BLUE_DECLARE_INTERFACE( ITr2Renderable );
BLUE_DECLARE_IVECTOR( ITr2Renderable );

enum TriBatchType
{
	TRIBATCHTYPE_OPAQUE,
	TRIBATCHTYPE_DECAL,
	TRIBATCHTYPE_TRANSPARENT,
	TRIBATCHTYPE_DEPTH,
	TRIBATCHTYPE_ADDITIVE,
	TRIBATCHTYPE_PICKING,
	TRIBATCHTYPE_MIRROR,
	TRIBATCHTYPE_DECALNORMAL,
	TRIBATCHTYPE_DEPTHNORMAL,
	TRIBATCHTYPE_OPAQUE_PREPASS,
	TRIBATCHTYPE_DECAL_PREPASS,
	TRIBATCHTYPE_GEOMETRY_ERASER,
	TRIBATCHTYPE_FLARE,
	TRIBATCHTYPE_DISTORTION,
	TRIBATCHTYPE_COUNT_OF_BATCH_TYPES // This element must be last!
};

enum Tr2RenderReason
{
	TR2RENDERREASON_NORMAL,
	TR2RENDERREASON_REFLECTION,
	TR2RENDERREASON_VOLUMETRIC,
};

BLUE_INTERFACE( ITr2Renderable ) :
	public IRoot
{
	virtual bool IsVisible( const EveUpdateContext& updateContext ) const
	{
		return true;
	}
	virtual void GetBatches( ITriRenderBatchAccumulator * batches, TriBatchType batchType, const Tr2PerObjectData* perObjectData, Tr2RenderReason reason = TR2RENDERREASON_NORMAL ) = 0;

	virtual bool HasTransparentBatches() = 0;
	virtual float GetSortValue() = 0;

	virtual Tr2PerObjectData* GetPerObjectData( ITriRenderBatchAccumulator * accumulator ) = 0;
};
REGISTER_COMPONENT_TYPE( "ReflectionRenderable", ITr2Renderable );

struct ITr2RenderableEntry
{
	ITr2Renderable* m_object;
	float m_distance;
	bool operator<( const ITr2RenderableEntry& other ) const
	{
		return m_distance < other.m_distance;
	}
	bool operator<=( const ITr2RenderableEntry& other ) const
	{
		return m_distance <= other.m_distance;
	}
};

typedef std::vector<ITr2RenderableEntry> Tr2RenderableSortList;

enum Tr2PickType
{
	PICK_TYPE_PICKING = 1 << 0,
	PICK_TYPE_OPAQUE = 1 << 1,
	PICK_TYPE_TRANSPARENT = 1 << 2,

	PICK_TYPE_ATTACHMENTS = 1 << 3,
	PICK_TYPE_LOCATORS = 1 << 4,
};

typedef uint32_t Tr2PickTypes;

BLUE_INTERFACE( ITr2Pickable ) :
	public IRoot
{
	virtual IRoot* GetID( uint16_t areaId ) = 0;
	virtual void GetPickingBatches( ITriRenderBatchAccumulator * batches, Tr2PickTypes pickTypes, const Tr2PerObjectData* perObjectData ) = 0;
};

BLUE_DECLARE_INTERFACE( ITr2Pickable );


#endif
