// Copyright © 2023 CCP ehf.

#pragma once
#ifndef EVELINESET_H
#define EVELINESET_H


#include "ITr2Renderable.h"
#include "Eve/IEveSpaceObject2.h"
#include "Eve/IEveTransform.h"
#include "Tr2DeviceResource.h"

#include <ITriFunction.h>

BLUE_DECLARE( EveLineSet );
BLUE_DECLARE( Tr2Effect );

class Tr2PerObjectData;
class Tr2PerObjectDataStandard;

// Each element of this struct is actually two vertices
struct EveLineData
{
	Vector3 m_position1;
	Color m_color1;
	Vector3 m_position2;
	Color m_color2;
};

class EveLineSet : public IInitialize,
				   public ITr2Renderable,
				   public IEveTransform,
				   public IEveSpaceObject2,
				   public Tr2DeviceResource
{
public:
	EXPOSE_TO_BLUE();

	EveLineSet( IRoot* lockobj = NULL );
	~EveLineSet();

	//////////////////////////////////////////////////////////////////////////////////////
	// IInitialize
	bool Initialize();

	//////////////////////////////////////////////////////////////////////////////////////
	// IEveSpaceObject2
	virtual void UpdateSyncronous( const EveUpdateContext& updateContext );
	virtual void UpdateAsyncronous( const EveUpdateContext& updateContext );
	void UpdateVisibility( const EveUpdateContext& updateContext, const Matrix& parentTransform );
	virtual void GetRenderables( std::vector<ITr2Renderable*>& renderables );
	virtual void GetRenderables( std::vector<ITr2Renderable*>& renderables, Tr2ImpostorManager* impostors );
	virtual bool GetBoundingSphere( Vector4& sphere, BoundingSphereQuery query = EVE_BOUNDS_NORMAL ) const;

	/////////////////////////////////////////////////////////////////////////////////////
	// IEveTransform
	virtual void Update( const EveUpdateContext& updateContext );
	virtual void UpdateViewDependentData( const TriFrustum& frustum, const Matrix& parentTransform );

	// No sensible implementation?
	virtual void UpdateModelCenterWorldPosition( Vector3& position, Be::Time t )
	{
	}
	virtual void GetModelCenterWorldPosition( Vector3& position ) const
	{
	}
	virtual bool GetLocalBoundingBox( Vector3& min, Vector3& max )
	{
		return false;
	}
	virtual void GetLocalToWorldTransform( Matrix& transform ) const
	{
		transform = IdentityMatrix();
	}

	//////////////////////////////////////////////////////////////////////////////////////
	// ITriDeviceResource
	virtual void ReleaseResources( TriStorage s );

private:
	bool OnPrepareResources();

public:
	/////////////////////////////////////////////////////////////////////////////////////
	// ITr2Renderable
	virtual void GetBatches( ITriRenderBatchAccumulator* batches, TriBatchType batchType, const Tr2PerObjectData* perObjectData, Tr2RenderReason reason = TR2RENDERREASON_NORMAL );

	virtual bool HasTransparentBatches();
	virtual float GetSortValue();
	virtual Tr2PerObjectData* GetPerObjectData( ITriRenderBatchAccumulator* accumulator );

	Tr2Lod GetLODLevel() const
	{
		return TR2_LOD_HIGH;
	}

private:
	// name
	std::string m_name;
	// display
	bool m_display;
	// bounding sphere
	Vector4 m_boundingSphere;

	unsigned int m_vertexDeclHandle;
	Tr2EffectPtr m_effect;
	Tr2BufferAL m_vertexBuffer;

	std::vector<EveLineData> m_lines;
	unsigned int m_maxCurrentLineCount;
	unsigned int m_currentSubmittedLineCount;

	bool m_isRenderedAsTransparent;

	// The ability to attach the LineSet to a Ball
	ITriVectorFunctionPtr m_ballPosition;
	ITriQuaternionFunctionPtr m_ballRotation;

	Matrix m_worldTransform;
	Vector3 m_scaling;

	// Python interface
	unsigned int AddLine( const Vector3& position1, const Vector4& color1, const Vector3& position2, const Vector4& color2 );
	void RemoveLine( unsigned int id );
	;

	void ChangeLine( unsigned int id, const Vector3& position1, const Vector4& color1, const Vector3& position2, const Vector4& color2 );
	void ChangeLineColor( unsigned int id, const Vector4& color1, const Vector4& color2 );
	void ChangeLinePosition( unsigned int id, const Vector3& position1, const Vector3& position2 );
	;

	void ClearLines();

	bool SubmitChanges();
};

TYPEDEF_BLUECLASS( EveLineSet );
BLUE_DECLARE_VECTOR( EveLineSet );

#endif