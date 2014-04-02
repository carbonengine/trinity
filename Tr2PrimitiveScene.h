////////////////////////////////////////////////////////////
//
//    Created:   May 2010
//    Copyright: CCP 2010
//

#pragma once
#ifndef Tr2PrimitiveScene_H
#define Tr2PrimitiveScene_H

#include "include/ITr2Scene.h"
#include "Tr2PickBuffer.h"
#include "TriRenderBatch.h"
#include "ITr2PickableScene.h"

BLUE_DECLARE( TriProjection );
BLUE_DECLARE( TriView );
BLUE_DECLARE( TriViewport );
BLUE_DECLARE( Tr2Effect );
BLUE_DECLARE( Tr2PrimitiveSet );
BLUE_DECLARE_VECTOR( Tr2PrimitiveSet );
BLUE_DECLARE( Tr2ManipulationTool );

// -------------------------------------------------------------
// Description:
//   Tr2PrimitiveScene is a scene that renders a list of primitives
//   (Tr2PrimitiveSet: lines, points, solids).
// SeeAlso:
//   Tr2PrimitiveSet, TriLineSet
// -------------------------------------------------------------
BLUE_CLASS( Tr2PrimitiveScene ) : 
	public ITr2Scene,
	public ITr2PickableScene,
	public Tr2DeviceResource
{
public:
	Tr2PrimitiveScene( IRoot* lockobj = 0 );
	~Tr2PrimitiveScene();

	EXPOSE_TO_BLUE();

	void Render( Tr2RenderContext& renderContext );
	void RenderDebugInfo( Tr2RenderContext& renderContext );
	void Update( Be::Time time );
	
	virtual void ReleaseResources( TriStorage s );
protected:
	virtual bool OnPrepareResources();
private:
    ITriRenderBatchAccumulator* m_opaqueBatches;
	ITriRenderBatchAccumulator* m_pickingBatches;
	TriPoolAllocator* m_allocator;

	// This is a python only wrapper function for just picking an object
	IRoot* PickObjectOnly( int x, int y, TriProjection* proj, TriView* view, TriViewport* vp )
	{
		USE_MAIN_THREAD_RENDER_CONTEXT();
		PickResults results;
		results.components = PICK_OBJECT;
		PickObject( renderContext, x, y, proj, view, vp, results );
		return results.object;
	}
	void SetupTransformsForPicking( float fx, float fy, TriProjection* proj, TriView* view, TriViewport* viewport );
	void SetupPerFrameData();

	void SetPerFrameDataForPicking( void );
	const std::vector<ITr2Renderable*>& GetPickingObjectsToRender( const Vector3& dirWorld );
	const std::vector<ITr2Renderable*>& GetPickingObjectsToRender( const Vector3& dirWorld, float fov, float aspect );
	const std::vector<ITr2Renderable*>& GetObjectsInsideFrustum( );

	// We always render opaque batches for picking
	ITriRenderBatchAccumulator* GetOpaquePickingBatchAccumulator( void );

	// If you need special behaviour for picking, these batches are rendered without a picking override
	ITriRenderBatchAccumulator* GetPickingBatchAccumulator( void );

	ITr2ShaderMaterial* GetPickingEffect( PickComponents pass );
	bool RenderPickingAreasForComponents( PickComponents pass ) const { return true; }
	unsigned int GetRequiredPasses( PickComponents requestedComponents, PickComponents* passes );

	void DecodeBufferPixel( const void* pBuffer, PickComponents pass, BufferResults& results ) const;

	Tr2PickBuffer& GetPickBuffer( void );
private:
	// typedef to shorten the iterator declaration
	typedef Tr2PrimitiveSetVector::const_iterator PrimitiveIterator;
	typedef std::vector<ITr2Renderable*>::const_iterator RenderableIterator;

	PTr2PrimitiveSetVector m_primitives;
	PTr2PrimitiveSetVector m_excludedPickingPrimitives;
	std::vector<ITr2Renderable*> m_visibleRenderObjects;
	std::vector<ITr2Renderable*> m_pickingObjects;

	Tr2ManipulationToolPtr m_manipulator;
	// Pick buffer
	Tr2PickBuffer m_pickBuffer;
	// Picking effect
	Tr2EffectPtr m_pickEffect;

	Tr2ConstantBufferAL	m_vertexConstants;
};

TYPEDEF_BLUECLASS( Tr2PrimitiveScene );

#endif // Tr2PrimitiveScene_H