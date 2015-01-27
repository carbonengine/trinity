#include "StdAfx.h"

#include "WodPlaceableRes.h"
#include "TriRenderBatch.h"
#include "Resources/TriGrannyRes.h"
#include "Tr2MeshArea.h"
#include "Utilities/BoundingBox.h"

WodPlaceableRes::WodPlaceableRes( IRoot* lockobj ) : 
	m_farFadeDistance( 100.0f*100.0f ),
	m_nearFadeDistance( 50.0f*50.0f  ),
	m_isShadowCaster( true ),
    m_bIsReady(false),
    PARENTLOCK( m_minBounds ),
    PARENTLOCK( m_maxBounds ),
	PARENTLOCK( m_curveSets )
{

}

WodPlaceableRes::~WodPlaceableRes()
{
}

void WodPlaceableRes::GetBatches( ITriRenderBatchAccumulator* batches,
								  TriBatchType batchType,
								  const Matrix& m , 
								  const Tr2PerObjectData* data )
{
    if( m_visualModel )
    {
        m_visualModel->GetBatches( batches, batchType, m, data );
    }
}

bool WodPlaceableRes::IsReady()
{
    if( m_bIsReady)
    {
        return true;
    }
    Vector3 myMin;
    Vector3 myMax;

    BoundingBoxInitialize( myMin, myMax );
    Vector3 meshMin;
    Vector3 meshMax;

    if( !m_visualModel->GetBoundingBox( meshMin, meshMax ) )
    {
        return false;
    }

    BoundingBoxUpdate( myMin, myMax, meshMin );
    BoundingBoxUpdate( myMin, myMax, meshMax );

    m_minBounds.SetVector( &myMin );
    m_maxBounds.SetVector( &myMax );

    m_bIsReady = true;

	return true;
}

bool WodPlaceableRes::HasTransparency()
{
    if( m_visualModel )
    {
        return m_visualModel->HasTransparency(); 
    }
    return false;
}

