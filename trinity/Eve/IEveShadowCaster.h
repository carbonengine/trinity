#ifndef IEveShadowCaster_h
#define IEveShadowCaster_h

#include "Eve/EveComponentRegistry.h"
#include "ITr2Renderable.h"
#include "TriFrustumOrtho.h"
#include "TriFrustum.h"


CCP_STATS_DECLARED_ELSEWHERE( objectsCulledCount );

class TriFrustum;
struct IEveShadowCaster;

class IEveShadowFrustum
{
public:
	virtual ~IEveShadowFrustum()
	{
	}
	virtual bool IsVisible( const TriFrustum& camera, const Vector4& boundingSphere ) const = 0;
	virtual float GetSizeInShadow( const Vector4& boundingSphere ) const = 0;
	virtual const Vector3& GetEyePos() const = 0;
};

class TriShadowOrthoFrustum : public IEveShadowFrustum
{
	TriFrustumOrtho m_shadow;
	uint32_t m_shadowMapSize;
	Vector3 m_sunDir;

public:
	TriShadowOrthoFrustum( const TriFrustumOrtho& shadow, uint32_t shadowMapSize, const Vector3& sunDir ) :
		m_shadow( shadow ),
		m_shadowMapSize( shadowMapSize ),
		m_sunDir( sunDir )
	{
	}
	bool IsVisible( const TriFrustum& camera, const Vector4& boundingSphere ) const override
	{
		bool sphereIsVisible = m_shadow.IsSphereVisibleIgnoreFarPlane( boundingSphere.GetXYZ(), boundingSphere.w );
		if( sphereIsVisible )
		{
			for( unsigned int j = 0; j < 6; ++j )
			{
				// first check if sun direction is perpendicular of the plane
				float d = DotNormal( camera.m_planes[j], m_sunDir );
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
	float GetSizeInShadow( const Vector4& boundingSphere ) const override
	{
		return m_shadow.GetPixelSize( boundingSphere, m_shadowMapSize );
	}
	const Vector3& GetEyePos() const override
	{
		return m_shadow.GetEyePos();
	}
};

class TriShadowFrustum : public IEveShadowFrustum
{
	TriFrustum m_shadow;

public:
	TriShadowFrustum( const TriFrustum& shadow ) :
		m_shadow( shadow )
	{
	}
	bool IsVisible( const TriFrustum& camera, const Vector4& boundingSphere ) const override
	{
		bool sphereIsVisible = m_shadow.IsSphereVisible( &boundingSphere );
		// TODO: intern, do something smart to cull the shadowcasting sphere using the camera frustum
		return sphereIsVisible;
	}
	float GetSizeInShadow( const Vector4& boundingSphere ) const override
	{
		return m_shadow.GetPixelSizeAccross( &boundingSphere );
	}
	const Vector3& GetEyePos() const override
	{
		return m_shadow.m_viewPos;
	}
};


BLUE_DECLARE( Tr2RaytracingManager );

BLUE_INTERFACE( IEveShadowCaster ) :
	public IRoot
{
	// Used for cascaded shadow map
	virtual bool IsCastingShadow( const TriFrustum& cameraFrustum, const IEveShadowFrustum& shadowFrustum, Tr2RenderReason renderReason, float& sizeInShadow ) const = 0;
	virtual void GetShadowBatches( ITriRenderBatchAccumulator * batches, const Tr2PerObjectData* perObjectData, float shadowPixelSize ) = 0;
	virtual Tr2PerObjectData* GetShadowPerObjectData( ITriRenderBatchAccumulator * accumulator ) = 0;
	// raytraced shadows
	virtual void PushRtGeometry( Tr2RaytracingManager& ) const{ }
};

REGISTER_COMPONENT_TYPE( "ShadowCaster", IEveShadowCaster );

#endif
