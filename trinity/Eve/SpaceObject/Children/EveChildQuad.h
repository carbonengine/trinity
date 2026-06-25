// Copyright © 2016 CCP ehf.

#pragma once
#ifndef EveChildQuad_H
#define EveChildQuad_H

#include "IEveSpaceObjectChild.h"
#include "EveChildTransform.h"
#include "ITr2Renderable.h"


BLUE_DECLARE( Tr2Effect );

BLUE_CLASS( EveChildQuad ) :
	public IEveSpaceObjectChild,
	public EveChildTransform,
	public ITr2Renderable,
	public IInitialize
{
public:
	EXPOSE_TO_BLUE();

	EveChildQuad( IRoot* lockobj = NULL );
	~EveChildQuad();

	static Tr2VertexDefinition& GetQuadDefinition();

	/////////////////////////////////////////////////////////////////////////////////////
	// IEveSpaceObjectChild
	const char* GetName() const;
	void SetName( const char* name );
	void UpdateVisibility( const EveUpdateContext& updateContext, const Matrix& parentTransform, Tr2Lod parentLod );
	void GetRenderables( std::vector<ITr2Renderable*> & renderables );
	bool GetBoundingSphere( Vector4 & sphere, BoundingSphereQuery query = EVE_BOUNDS_NORMAL ) const;
	void RegisterWithQuadRenderer( Tr2QuadRenderer & quadRenderer );
	void AddQuadsToQuadRenderer( const TriFrustum& frustum, Tr2QuadRenderer& quadRenderer ) const;
	void UpdateSyncronous( const EveUpdateContext& updateContext, const EveChildUpdateParams& params );
	void UpdateAsyncronous( const EveUpdateContext& updateContext, const EveChildUpdateParams& params );
	void GetLocalToWorldTransform( Matrix & transform ) const;
	void ChangeLOD( Tr2Lod lod ) {};
	void Setup( const Vector3* scale, const Quaternion* rotation, const Vector3* translation, Tr2Lod lowestLodVisible );

	/////////////////////////////////////////////////////////////////////////////////////
	// ITr2Renderable
	virtual bool HasTransparentBatches();
	virtual void GetBatches( ITriRenderBatchAccumulator * batches, TriBatchType batchType, const Tr2PerObjectData* perObjectData, Tr2RenderReason reason = TR2RENDERREASON_NORMAL );
	virtual float GetSortValue();
	virtual Tr2PerObjectData* GetPerObjectData( ITriRenderBatchAccumulator * accumulator );

	/////////////////////////////////////////////////////////////////////////////////////
	// IInitialize
	virtual bool Initialize();

private:
	struct Quad
	{
		Vector4 m_parentTransform0;
		Vector4 m_parentTransform1;
		Vector4 m_parentTransform2;

		Vector4 m_localTransform0;
		Vector4 m_localTransform1;
		Vector4 m_localTransform2;

		Float_16 m_color[4];
		Float_16 m_brightness[2];
	};

	BlueSharedString m_name;
	Tr2EffectPtr m_effect;
	unsigned m_effectKey;
	Quad m_quad;

	Vector3 m_position;
	float m_viewRotation;
	Color m_color;
	float m_brightness;
	float m_minScreenSize;
	mutable float m_currentScreenSize;

	bool m_display;
	bool m_isVisible;
	// Has UpdateSyncronous/UpdateAsyncronous been called: until it has, the object cannot be rendered
	bool m_hasUpdated = false;
	// Continiously re-register effect (for editing in Jessica)
	bool m_editMode;
};

TYPEDEF_BLUECLASS( EveChildQuad );

#endif