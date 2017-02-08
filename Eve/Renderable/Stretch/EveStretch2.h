////////////////////////////////////////////////////////////
//
//    Created:   January 2017
//    Copyright: CCP 2017
//

#pragma once

#include "Eve/IEveFiringEffectElement.h"
#include "ITr2Renderable.h"
#include "ITr2GeometryProvider.h"
#include "Tr2DeviceResource.h"


BLUE_DECLARE( Tr2Effect );
BLUE_DECLARE( TriCurveSet );
BLUE_DECLARE( Tr2PointLight );


// --------------------------------------------------------------------------------------
// Description:
//   EveStretch2 is a simplified version of EveStretch. Renders an effect between two
//   points as a set of quads. 
// --------------------------------------------------------------------------------------
BLUE_CLASS( EveStretch2 )
	:public IEveFiringEffectElement,
	public ITr2Renderable,
	public ITr2GeometryProvider,
	public Tr2DeviceResource,
	public IInitialize,
	public INotify
{
public:
	EXPOSE_TO_BLUE();

	EveStretch2( IRoot* lockObj = nullptr );

	virtual bool Initialize();
	virtual bool OnModified( Be::Var* value );

	virtual void SetDestObjectScale( float scale );
	virtual void StartMoving();
	virtual float GetCurveDuration();
	virtual void StartFiring( float delay );
	virtual void StopFiring();

	virtual void SetFiringTransform( const Matrix& source, const Vector3& dest );
	virtual void SetFiringTransform( const Vector3& source, const Vector3& dest );
	virtual void DisplayEndPoints( bool displaySource, bool displayDest );

	virtual void Update( EveUpdateContext& updateContext );
	virtual void UpdateInactive( EveUpdateContext& updateContext );

	virtual void UpdateVisibility( const TriFrustum& frustum, const Matrix& parentTransform );
	virtual void GetRenderables( std::vector<ITr2Renderable*>& renderables );
	virtual void GetLights( Tr2LightManager& lightManager ) const;


	virtual Tr2PerObjectData* GetPerObjectData( ITriRenderBatchAccumulator* accumulator );
	virtual void GetBatches( ITriRenderBatchAccumulator* batches, TriBatchType batchType, const Tr2PerObjectData* perObjectData );
	virtual bool HasTransparentBatches();
	virtual float GetSortValue();

protected:
	virtual void ReleaseResources( TriStorage s );
	virtual bool OnPrepareResources();
	virtual void SubmitGeometry( Tr2RenderContext& renderContext );
private:
	Tr2EffectPtr m_effect;
	TriCurveSetPtr m_start;
	TriCurveSetPtr m_loop;
	TriCurveSetPtr m_end;

	Tr2PointLightPtr m_sourceLight;
	Tr2PointLightPtr m_destinationLight;

	Vector3 m_source;
	float m_currentDestinationScale;
	Vector3 m_destination;
	float m_destinationScale;

	uint32_t m_quadCount;
	Tr2VertexBufferAL m_vb;
	unsigned int m_vertexDeclHandle;

	bool m_visible;
};

TYPEDEF_BLUECLASS( EveStretch2 );
