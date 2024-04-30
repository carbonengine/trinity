#ifndef IEveShadowCaster_h
#define IEveShadowCaster_h

#include "Eve/EveComponentRegistry.h"
#include "TriFrustumOrtho.h"
#include "TriFrustum.h"

CCP_STATS_DECLARED_ELSEWHERE( objectsCulledCount );

class TriFrustum;
struct IEveShadowCaster;

namespace EveShadowCaster
{
	inline bool IsVisible( const TriFrustum& camera, const TriFrustumOrtho& shadow, const Vector3 sunDir, const Vector4 boundingSphere )
	{
		bool sphereIsVisible = shadow.IsSphereVisibleAndInsideNearPlane( &boundingSphere );
		if( sphereIsVisible )
		{
			for( unsigned int j = 0; j < 6; ++j )
			{
				// first check if sun direction is perpendicular of the plane
				float d = DotNormal( camera.m_planes[j], sunDir );
				// if it's not perpendicular then check if the object is "behind" the plane
				if( d < 0 )
				{
					auto val = DotCoord( camera.m_planes[j], -boundingSphere.GetXYZ() );
					if( DotCoord( camera.m_planes[j], boundingSphere.GetXYZ() ) < -boundingSphere.w )
					{
						CCP_STATS_INC( objectsCulledCount );
						return false;
					}
				}
			}
		}
		return sphereIsVisible;
	}

	inline float GetSizeInShadow( const TriFrustumOrtho& shadow, const uint32_t shadowMapSize, const Vector4 boundingSphere )
	{
		return shadow.GetPixelSize( boundingSphere, shadowMapSize );
	}
}

BLUE_INTERFACE( IEveShadowCaster ) :
	public IRoot
{
	// Used for cascaded shadow map
	virtual bool IsCastingShadow( const TriFrustum& cameraFrustum, const TriFrustumOrtho& shadowFrustum, const uint32_t shadowMapSize, const Vector3 sunDir, float& sizeInShadow ) const = 0;
	virtual void GetShadowBatches( ITriRenderBatchAccumulator* batches, const Tr2PerObjectData* perObjectData, float shadowPixelSize ) = 0;
	virtual Tr2PerObjectData* GetShadowPerObjectData( ITriRenderBatchAccumulator* accumulator ) = 0;
};

REGISTER_COMPONENT_TYPE( "ShadowCaster", IEveShadowCaster );

#endif
