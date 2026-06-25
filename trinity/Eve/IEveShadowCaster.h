// Copyright © 2023 CCP ehf.

#ifndef IEveShadowCaster_h
#define IEveShadowCaster_h

#include "Eve/EveComponentRegistry.h"
#include "ITr2Renderable.h"
#include "TriFrustumOrtho.h"
#include "TriFrustum.h"


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

	virtual TriFrustumTestResult SphereTest( const TriFrustum& camera, const CcpMath::Sphere& sphere ) const = 0;
};

class TriShadowOrthoFrustum : public IEveShadowFrustum
{
	TriFrustumOrtho m_shadow;
	uint32_t m_shadowMapSize;
	Vector3 m_sunDir;

public:
	TriShadowOrthoFrustum() :
		m_shadow( {} ),
		m_shadowMapSize( 0 ),
		m_sunDir( {} )
	{
	}

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

	TriFrustumTestResult SphereTest( const TriFrustum& camera, const CcpMath::Sphere& sphere ) const override
	{
		auto orthoResult = m_shadow.SphereTestIgnoreFarPlane( sphere );
		if( orthoResult != TriFrustumTestResult::Outside )
		{
			for( unsigned int j = 0; j < 6; ++j )
			{
				// first check if sun direction is perpendicular of the plane
				float d = DotNormal( camera.m_planes[j], m_sunDir );
				// if it's not perpendicular then check if the object is "behind" the plane
				if( d < 0 )
				{
					auto val = DotCoord( camera.m_planes[j], sphere.center );
					if( val < -sphere.radius )
					{
						return TriFrustumTestResult::Outside;
					}
					if( val < sphere.radius )
					{
						orthoResult = TriFrustumTestResult::Intersect;
					}
				}
			}
		}
		return orthoResult;
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

	TriFrustumTestResult SphereTest( const TriFrustum& camera, const CcpMath::Sphere& sphere ) const override
	{
		return m_shadow.SphereTest( sphere );
	}
};


BLUE_DECLARE( Tr2RaytracingManager );

BLUE_INTERFACE( IEveShadowCaster ) :
	public IRoot
{
	// Used for cascaded shadow map
	virtual bool IsCastingShadow( const TriFrustum& cameraFrustum, const IEveShadowFrustum& shadowFrustum, Tr2RenderReason renderReason, float& sizeInShadow ) const = 0;
	virtual bool IsCastingShadow( const TriFrustum& cameraFrustum, Vector3 position, float radius, Tr2RenderReason renderReason ) const
	{
		return false;
	}
	virtual void GetShadowBatches( ITriRenderBatchAccumulator * batches, const Tr2PerObjectData* perObjectData, float shadowPixelSize ) = 0;
	virtual Tr2PerObjectData* GetShadowPerObjectData( ITriRenderBatchAccumulator * accumulator ) = 0;
	// raytraced shadows
	virtual void PushRtGeometry( Tr2RaytracingManager& ) const
	{
	}
	virtual void MarkRtDirty()
	{
	}
	virtual bool IsShadowCastingDirty() const
	{
		return false;
	}
};

REGISTER_COMPONENT_TYPE( "ShadowCaster", IEveShadowCaster );

#endif
