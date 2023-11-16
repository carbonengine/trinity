#pragma once
#ifndef ITr2Interior_H
#define ITr2Interior_H

#include "include/ITr2BoundingBox.h"
#include "ITr2Renderable.h"
#include "Tr2PerObjectData.h"

class TriFrustum;
class Tr2InteriorLightSet;
struct Tr2InteriorPerObjectLightData;
struct Tr2PerFrameVSData;
struct ITr2InteriorLight;
class Tr2RenderContext;

enum CullResult
{
	CULLRES_OK = 1,
	CULLRES_NOTREADY,
	CULLRES_FAILED,
};

BLUE_DECLARE( Tr2ApexScene );

BLUE_INTERFACE( ITr2InteriorCullable ) : public IRoot
{
	virtual bool IsInFrustum( const TriFrustum& frustum, Matrix& objectToWorld ) const = 0;
};

BLUE_INTERFACE( ITr2Interior ) : public ITr2InteriorCullable
{
	// Per-object data for instanced lighting
	virtual Tr2PerObjectData* GetPerObjectDataWithPerInstanceLighting( 
		ITriRenderBatchAccumulator* accumulator,
		Tr2InteriorLightSet* lightSet, 
		const Matrix& objectToWorldMatrix 
	) = 0;
};
BLUE_DECLARE_IVECTOR( ITr2Interior );

BLUE_INTERFACE( ITr2InteriorDynamic ) : public ITr2Interior
{
	virtual bool GetWorldBoundingBox( Vector3& min, Vector3& max ) const = 0;
	virtual bool IsBoundingBoxReady( void ) const = 0;

	// Spherical harmonics update
	virtual void PrePhysicsUpdate( Be::Time time ) = 0;
	virtual void PostPhysicsUpdate( Be::Time time, Tr2ApexScene* apexScene ) = 0;

	// Scene add/remove
	virtual bool AddToScene( Tr2ApexScene* apexScene ) = 0;
	virtual void RemoveFromScene( void ) = 0;

	// LOD
	virtual void SetLOD( const TriFrustum* frustum ) = 0;
};
BLUE_DECLARE_IVECTOR( ITr2InteriorDynamic );


// -------------------------------------------------------------
// Description:
//   ITr2InteriorLight represents light source for interior 
//   scene.
// -------------------------------------------------------------
BLUE_INTERFACE( ITr2InteriorLight ) : public ITr2InteriorCullable
{

	// Helper structure for determining shadow caster importance
	// To Do: Rename this guy & maybe unify it with the LightInstance structure
	struct LightSourceItem
	{
		// Pointer to the light source
		ITr2InteriorLight* lightSource;
		// Light importance
		float importance;
		// Shadow map index
		unsigned int shadowMapIndex;

		// Compare light importance for descending sort
		bool operator<( const LightSourceItem& other ) const
		{
			// for equal importance, sort by lightSource.  this way moving the
			// camera around a pair of lights keeps them in the same order.
			// Which is nice for expensive fx that do something with light0 only.
			if( importance > other.importance )
			{
				return true;
			}
			if( importance < other.importance )
			{
				return false;
			}
			return (size_t)lightSource < (size_t)other.lightSource;
		}
	};

	// -------------------------------------------------------------
	// Description:
	//   Copy the light parameters into the per-object data.
	// Arguments:
	//   lightData - Per-object light data
	// -------------------------------------------------------------
	virtual void PopulateLightData( Tr2InteriorPerObjectLightData* lightData ) const = 0;

	// -------------------------------------------------------------
	// Description:
	//   Per-frame update method.
	// Arguments:
	//   time - Current system time.
	// -------------------------------------------------------------
	virtual void Update( Be::Time time ) = 0;
};

BLUE_DECLARE_IVECTOR( ITr2InteriorLight );

class Tr2PerObjectDataPSBuffer;

#endif // ITr2Interior_H
