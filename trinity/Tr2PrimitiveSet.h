#pragma once
#ifndef Tr2PrimitiveSet_h
#define Tr2PrimitiveSet_h

#include "ITr2Renderable.h"
#include "ITr2GeometryProvider.h"

BLUE_DECLARE( Tr2Effect );

/*
	The base class of Tr2 debugging primitive sets.

	============================================================
	Primitive sets are used to draw bounding boxes and tools for
	Jessica and other authoring tools.
*/

BLUE_CLASS( Tr2PrimitiveSet ):
	public ITr2Renderable,	// An interface for batches, per user data and sorting
	public ITr2Pickable,	// Make this a pickable object
	public ITr2GeometryProvider, // We do our own submit of geometry for rendering
	public INotify			// We want to be able to monitor edits through python
{
public:
	EXPOSE_TO_BLUE();
	Tr2PrimitiveSet( IRoot* lockobj = NULL );
	virtual ~Tr2PrimitiveSet();
	virtual const Matrix& GetWorldTransform( void ) const { return m_worldTransform; }
	virtual Vector4 GetBoundingSphere( void );
	virtual void UpdateTransform( void );

	void GetBatchesImpl( ITriRenderBatchAccumulator* accumulator, const Tr2PerObjectData* perObjectData, Tr2Effect* effect );

	/////////////////////////////////////////////////////////////////////////////////////
	// ITr2Pickable
	virtual IRoot* GetID( uint16_t ) { return this->GetRawRoot(); }
	virtual void GetPickingBatches( ITriRenderBatchAccumulator* batches, Tr2PickTypes pickTypes, const Tr2PerObjectData* perObjectData );

	//////////////////////////////////////////////////////////////////////////
	// INotify
	virtual bool OnModified( Be::Var* value );

	/////////////////////////////////////////////////////////////////////////////////////
	// ITr2Renderable
	virtual void GetBatches( ITriRenderBatchAccumulator* batches, TriBatchType batchType, const Tr2PerObjectData* perObjectData, Tr2RenderReason reason = TR2RENDERREASON_NORMAL );	
	virtual bool HasTransparentBatches();
	virtual float GetSortValue();
	virtual Tr2PerObjectData* GetPerObjectData( ITriRenderBatchAccumulator* accumulator );
	virtual void SetCurrentColor( Color& val ) { }

	//////////////////////////////////////////////////////////////////////////////////////
	// ITr2GeometryProvider
	virtual void SubmitGeometry( Tr2RenderContext& renderContext ) {}

	// keeps score of the current scale that is applied to the model
	float		m_scale; 
	// should we keep the size of the primitive always the same on screen
	bool		m_scaleByDistanceToView; 
	// Should we orient the primitive towards the view
	bool		m_viewOriented; 
	// --
	Vector4		m_boundingSphere; 
	// the end result of the internal calculations
	Matrix		m_worldTransform; 
	// for positioning and rotating the view
	Matrix		m_localTransform; 

#if BLUE_WITH_PYTHON
	// for latching on any python objects that might be of use to the programmers
	PyObject*	m_pythonUserData; 
#endif

	// when we are drawing for picking
	bool		m_isDrawingForPicking; 
	// What is the main solid color of our primitive
	Color		m_color;	

	// Just a name to help identify the primitive
	std::string m_name;	

	// Vertex buffer and effects
	unsigned int m_vertexDeclHandle; 
	Tr2BufferAL m_vertexBuffer;
	Tr2EffectPtr m_effect;
	Tr2EffectPtr m_pickEffect;
};

TYPEDEF_BLUECLASS( Tr2PrimitiveSet );

#endif