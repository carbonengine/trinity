#pragma once
#ifndef ITr2Renderable_H
#define ITr2Renderable_H

class ITriRenderBatchAccumulator;
class Tr2PerObjectData;
class TriPoolAllocator;

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

BLUE_INTERFACE( ITr2Renderable ) : public  IRoot
{
	virtual void GetBatches( ITriRenderBatchAccumulator* batches, TriBatchType batchType, const Tr2PerObjectData* perObjectData ) = 0;
	virtual void GetShadowBatches( ITriRenderBatchAccumulator* batches, const Tr2PerObjectData* perObjectData ) {}

    virtual bool HasTransparentBatches() = 0;
    virtual float GetSortValue() = 0; 

	virtual Tr2PerObjectData* GetPerObjectData( ITriRenderBatchAccumulator* accumulator ) = 0;
};

struct ITr2RenderableEntry
{
    ITr2RenderablePtr m_object;
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

BLUE_INTERFACE( ITr2Pickable ) : public IRoot
{
    virtual IRoot* GetID() = 0;
};

BLUE_DECLARE_INTERFACE( ITr2Pickable );


#endif
