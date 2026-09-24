// Copyright © 2026 CCP ehf.

#ifndef CARBON_INTERSECTIONS_FXH
#define CARBON_INTERSECTIONS_FXH



struct Ray
{
	float3 origin;
	float3 dir;
};

Ray InitRay(float3 o, float3 d)
{
	Ray r;
	r.origin = o;
	r.dir = d;
	return r;
}


struct Plane
{
	float3 normal;
	float3 planePoint;
};

Plane InitPlane(float3 normal, float3 planePoint) 
{
	Plane p;
	p.normal = normal;
	p.planePoint = planePoint;
	return p;
}

float GetDistanceToPlane( Plane plane, Ray ray )
{
	float denom = dot( ray.dir, plane.normal );
	float denom2 = dot( ray.dir, -plane.normal );
	float d = max( denom, denom2 );
	if( d == denom2 )
	{
		plane.normal = -plane.normal;
	}

    const float FLT_EPSILON = 0.0000001;
	if( d >= FLT_EPSILON )
	{
		float3 rayToPlanePoint = plane.planePoint - ray.origin;

		return dot( rayToPlanePoint, plane.normal ) / d;		
	}
	return -1;
}

bool PointOnPlane( Ray ray, float t, float3 planePoint, float radius )
{
	if( t < 0 )
	{
		return false;
	}
	float3 pointInPlaneSpace = ray.origin + t * ray.dir;
	float3 dist = planePoint - pointInPlaneSpace;
	float r = radius;

	return dot( dist, dist ) - r * r <= 0;
}

float GetDistanceToCircle(float3 normal, float3 planePoint, float radius, Ray ray)
{
	float t = GetDistanceToPlane( InitPlane( normal, planePoint ), ray);
	if( PointOnPlane( ray, t, planePoint, radius ) )
	{
		return t;
	}
	return -1;
}

// -------------------------------------------------------------------------------
// Sphere
// -------------------------------------------------------------------------------
struct Sphere
{
	float3 center;
	float radius;	
};

Sphere InitSphere( float3 center, float radius )
{
    Sphere s;
    s.center = center;
    s.radius = radius;
    return s;
}

struct SphereHitInfo
{
	bool occurred;
	float4 firstIntersection; // xyz - position, w - distance from eye
	float4 secondIntersection; // xyz - position, w - distance from eye
};

SphereHitInfo SphereRayHitTest( Sphere sphere, Ray ray )
{
	SphereHitInfo info;
	info.occurred = false;
	info.firstIntersection = float4(0, 0, 0, 0);
	info.secondIntersection = float4(0, 0, 0, 0);

	// algorithm taken from Real Time Rendering fourth edition, page 958
	float3 originToSphereCenter = sphere.center - ray.origin;
	float s = dot( originToSphereCenter, ray.dir );
	float originToSphereCenterLengthSq = dot(originToSphereCenter, originToSphereCenter);
	float radSq = sphere.radius * sphere.radius;

	if( s < 0 && originToSphereCenterLengthSq > radSq )
	{
		return info;
	}

	float mSq = originToSphereCenterLengthSq - s*s;
	if( mSq > radSq )
	{
		return info;
	}

	float intersectionPerpendicularToCenter = sqrt( radSq - mSq );
	float minT = max( 0.0, min( s - intersectionPerpendicularToCenter, s + intersectionPerpendicularToCenter ) );
	float maxT = max( s - intersectionPerpendicularToCenter, s + intersectionPerpendicularToCenter );
	
	info.firstIntersection = float4( ray.origin + ray.dir * minT - sphere.center, minT );
	info.secondIntersection = float4( ray.origin + ray.dir * maxT - sphere.center, maxT );
	info.occurred = true;

	return info;
}


// -------------------------------------------------------------------------------
// Cones
// -------------------------------------------------------------------------------
struct Cone
{
	float height;
	float3 tip;
	float cosa; // half cone angle
	// float tipRadius;
	float3 axis;
	float baseRadius;
	float tipOffset;
};

struct ConeHitInfo
{
	float4 firstIntersection; // xyz - position, w - distance from eye
	float4 secondIntersection; // xyz - position, w - distance from eye
	float4 hits; // frontCone, backCone, base, tip
	float shadowConeHit;
	bool occurred;
	float4 tmp;
};

bool PointInCone( Ray ray, float t, Cone cone )
{
	if( t < 0 )
	{
		return false;
	}

	float3 pointInConeSpace = ray.origin + t * ray.dir - cone.tip;
	
	float cone_dist = dot( pointInConeSpace, cone.axis );
    const float FLT_EPSILON = 0.0000001;

	if( cone.tipOffset - cone_dist > 0 || cone_dist > cone.height + FLT_EPSILON )
	{
		return false;
	}

	float cone_radius_at_dist = ( cone_dist / cone.height ) * cone.baseRadius;
	float3 orth_point = pointInConeSpace - cone_dist * cone.axis;
	float orth_distance_sq = dot(orth_point, orth_point);
	return cone.baseRadius * cone.baseRadius - orth_distance_sq >= 0;
}

bool GetConeIntersectionDistances(Cone cone, Ray ray, out float t1, out float t2, out float baseT, out float tipT ) 
{
	t1 = -1;
	t2 = -1;
	baseT = -1;
	tipT = -1;

	baseT = GetDistanceToCircle(cone.axis, cone.tip + cone.axis * cone.height, cone.baseRadius, ray);
	
	float tipOffsetRadius = cone.tipOffset / cone.cosa;
	tipOffsetRadius = sqrt(tipOffsetRadius * tipOffsetRadius - cone.tipOffset * cone.tipOffset);	
	tipT = GetDistanceToCircle(cone.axis, cone.tip + cone.axis * cone.tipOffset, tipOffsetRadius, ray);

	// code comes from here http://lousodrome.net/blog/light/2017/01/03/intersection-of-a-ray-and-a-cone/
	float3 co = ray.origin - cone.tip;

	float dirAxisDot = dot(ray.dir, cone.axis);
	float coAxisDot = dot(co, cone.axis);

	float a = dirAxisDot * dirAxisDot - cone.cosa * cone.cosa;
	float b = 2. * (dirAxisDot * coAxisDot - dot(ray.dir, co) * cone.cosa * cone.cosa);
	float c = coAxisDot * coAxisDot - dot(co, co) * cone.cosa * cone.cosa;

	float det = b * b - 4. * a * c;
	
	if( det < 0 ) 
	{
		if( baseT < 0 && tipT < 0 )	
		{
			return false;
		}
		return true;
	}

	det = sqrt(abs(det));

	// if t1 is < 0 then we are will never hit the cone i.e the ray will never intersect
	t1 = ( -b - det ) / ( 2. * a ); 
	// if t2 is < 0 then we are looking down the axis of the cone (in either way)	
	t2 = ( -b + det ) / ( 2. * a ); 
		
	return true;	
}

// Sorts a vector4 in ascending order (x, y, z, w)
float4 sort(float4 values) 
{
	float minv, maxv;
	
	// move the maximum value to w
	minv = min(values.x, values.y);
	maxv = max(values.x, values.y);
	values.x = minv;
	values.y = maxv;

	minv = min(values.y, values.z);
	maxv = max(values.y, values.z);
	values.y = minv;
	values.z = maxv;

	minv = min(values.z, values.w);
	maxv = max(values.z, values.w);
	values.z = minv;
	values.w = maxv;
	// now w has the highest value
	
	// now we can sort xyz easily
	minv = min(values.x, min(values.y, values.z));
	maxv = max(values.x, max(values.y, values.z));
	float midv = values.x + values.y + values.z - minv - maxv;
	values.x = minv;
	values.y = midv;
	values.z = maxv;

	// now values are sorted
	return values;
}

ConeHitInfo ConeFlatBaseHitTest( Cone cone, Ray ray )
{
	ConeHitInfo hitInfo;
	hitInfo.occurred = false;
	hitInfo.firstIntersection = float4(0, 0, 0, 0);
	hitInfo.secondIntersection = float4(0, 0, 0, 0);
	hitInfo.hits = float4(0, 0, 0, 0); 
	hitInfo.tmp = float4(0, 0, 0, 0);
	hitInfo.shadowConeHit = 0;

	float coneT1, coneT2, baseT, tipT = -1;
	if( !GetConeIntersectionDistances(cone, ray, coneT1, coneT2, baseT, tipT) )
	{
		return hitInfo;
	}

	bool frontConeHit = PointInCone( ray, coneT2, cone );
	bool backConeHit = PointInCone( ray, coneT1, cone );
	bool rayOriginConeHit = PointInCone( ray, 0, cone );
	bool baseConeHit = baseT > -1; 
	bool tipConeHit = tipT > -1;

	hitInfo.hits = float4(frontConeHit, backConeHit, baseConeHit, tipConeHit); 
	hitInfo.shadowConeHit = 1;

	if( !any( hitInfo.hits.xyzw ) )
	{
		return hitInfo;
	}

	// for the "distances" that don't create a hit, put in invalid values 
	float4 modifiedT = float4(frontConeHit ? coneT2 : -1, 
							  backConeHit ? coneT1 : -1, 
							  tipConeHit ? tipT : -1, 
							  baseConeHit ? baseT : -1);

	float4 sortedModifiedT = sort(modifiedT);
	float2 tToUse;

	if( rayOriginConeHit ) 
	{
		tToUse.x = 0;		
		tToUse.y = sortedModifiedT.w;
	}
	else 
	{
		tToUse.x = sortedModifiedT.z >= 0 ? sortedModifiedT.z: sortedModifiedT.w;
		tToUse.y = sortedModifiedT.w;
	}

	hitInfo.occurred = true;
	hitInfo.firstIntersection = float4(ray.origin + ray.dir * tToUse.x - cone.tip, tToUse.x);
	hitInfo.secondIntersection = float4(ray.origin + ray.dir * tToUse.y - cone.tip, tToUse.y);

	return hitInfo;
}

ConeHitInfo ConeRoundBaseHitTest( Cone cone, Ray ray )
{
	ConeHitInfo hitInfo;
	hitInfo.occurred = false;
	hitInfo.firstIntersection = float4(0, 0, 0, 0);
	hitInfo.secondIntersection = float4(0, 0, 0, 0);
	hitInfo.hits = float4(0, 0, 0, 0); 
	hitInfo.shadowConeHit = false;
	hitInfo.tmp = false;

	float coneT1, coneT2, tmp, tipT = -1;
	// are we intersecting the cone (endless and in both directions...)
	if( !GetConeIntersectionDistances(cone, ray, coneT1, coneT2, tmp, tipT ) )
	{
		return hitInfo;
	}
	
	hitInfo.shadowConeHit = true;

	// first do the sphereintersection since that is cheaper
	Sphere s;
	s.radius = cone.height;
	s.center = cone.tip;

	SphereHitInfo si = SphereRayHitTest(s, ray);	
	
	// Early exit, if no spherical intersection occurred
	// either of the intersection points are not within the cone radius
	if( !si.occurred  )
	{
		return hitInfo;
	}

	float firstIntersectionDot = dot(normalize(si.firstIntersection.xyz), cone.axis);
	float secondIntersectionDot = dot(normalize(si.secondIntersection.xyz), cone.axis);

	bool firstBaseHit = firstIntersectionDot - cone.cosa >= 0;
	bool secondBaseHit = secondIntersectionDot - cone.cosa >= 0;

	float baseT1 = si.firstIntersection.w;
	float baseT2 = si.secondIntersection.w;

	// modify the cone so it fits nicely to the sphere
	cone.height *= cone.cosa;
	bool frontConeHit = PointInCone( ray, coneT1, cone );
	bool backConeHit = PointInCone( ray, coneT2, cone );
	bool rayOriginConeHit = PointInCone( ray, 0, cone );
		
	float tipOffsetRadius = cone.tipOffset / cone.cosa;
	tipOffsetRadius = sqrt(tipOffsetRadius * tipOffsetRadius - cone.tipOffset - cone.tipOffset);	
	bool tipConeHit = PointOnPlane( ray, tipT, cone.tip + cone.axis * cone.tipOffset, tipOffsetRadius); 
		
	float4 coneIntersections = float4(frontConeHit, backConeHit, rayOriginConeHit, tipConeHit);
	float2 sphereIntersections = float2(firstBaseHit, secondBaseHit);
	hitInfo.hits = float4(frontConeHit, backConeHit, (any(coneIntersections) && any(sphereIntersections)) || all(sphereIntersections), tipConeHit ); 

	// if we don't hit the cone or have any cone and sphere intersections or don't hit the sphere, we can return
	if( !any(hitInfo.hits.xyzw) )
	{
		return hitInfo;
	}

	float2 tToUse;
	// figure out the t-s to use
	// we have 6 ts to pick from:
	// - front and back cone intersection
	// - front and back sphere 
	// - tip 
	// - 0 (the camera)

	// there are 2 scenarios though
	// if the camera is intersecting with the cone, then we need the next t (or min(coneT1, coneT2, sphereT1, sphereT2))
	// otherwise we need the middle values of (coneT1, coneT2, sphereT1, sphereT2)
	float4 ts = sort(float4(
		hitInfo.hits.x ? coneT1 : -1, 
		hitInfo.hits.y ? coneT2 : -1, 
		(any(coneIntersections) && sphereIntersections.x) || all(sphereIntersections)? baseT1 : -1, 
		(any(coneIntersections) && sphereIntersections.y) || all(sphereIntersections)? baseT2 : -1
		)
	);
		
	if( rayOriginConeHit ) 
	{
		tToUse.x = 0;		
		tToUse.y = max(tipT, ts.w);
	}
	else 
	{
		tToUse.x = ts.z >= 0 ? ts.z: ts.w;
		tToUse.y = ts.w;
	}
	
	hitInfo.occurred = true;
	hitInfo.firstIntersection = float4(ray.origin + ray.dir * tToUse.x - cone.tip, tToUse.x);
	hitInfo.secondIntersection = float4(ray.origin + ray.dir * tToUse.y - cone.tip, tToUse.y);

	return hitInfo;
}

#endif