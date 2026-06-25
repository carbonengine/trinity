// Copyright © 2023 CCP ehf.

#pragma once
#ifndef Tr2GrannyPrimitiveSet_h
#define Tr2GrannyPrimitiveSet_h

#include "Tr2PrimitiveSet.h"
#include "Tr2DeviceResource.h"

BLUE_DECLARE( TriGrannyRes );

struct TriangleVertex
{
	Vector3 m_position;
	Vector3 m_normal;
	Color m_color;
};

BLUE_CLASS( Tr2GrannyPrimitiveSet ) :
	public IInitialize,
	public Tr2PrimitiveSet,
	public Tr2DeviceResource,
	public IBlueAsyncResNotifyTarget
{
public:
	EXPOSE_TO_BLUE();
	Tr2GrannyPrimitiveSet( IRoot* lockobj = NULL );
	~Tr2GrannyPrimitiveSet();
	using IInitialize::Lock;
	using IInitialize::Unlock;

	//////////////////////////////////////////////////////////////////////////
	// IBlueAsyncResNotifyTarget
	void ReleaseCachedData( BlueAsyncRes * p );
	void RebuildCachedData( BlueAsyncRes * p );

	//////////////////////////////////////////////////////////////////////////
	// IInitialize
	bool Initialize();

	//////////////////////////////////////////////////////////////////////////
	// INotify
	virtual bool OnModified( Be::Var * value );

	//////////////////////////////////////////////////////////////////////////////////////
	// ITriDeviceResource
	virtual void ReleaseResources( TriStorage s );

#if TRINITYDEV
	virtual void GetDescription( std::string & desc )
	{
		desc = "<Tr2GrannyPrimitiveSet>";
	}
#endif

	void SetGrannyResource();
	void CreatePrimitive();
	void CleanUp( void );

protected:
	void GetBatchesImpl( ITriRenderBatchAccumulator * accumulator, const Tr2PerObjectData* perObjectData, Tr2Material* effect, GetBatchesReason reason ) override;

private:
	virtual bool OnPrepareResources();

	void SetCurrentColor( Color & val );

#if WITH_GRANNY
	void CreatePrimitiveFromGranny();
#endif
	void CreatePrimitiveFromCMF();

	// We use indices to draw the solids and the lines,
	// because we want to use the same vertex buffer
	Tr2BufferAL m_triangleIndexBuffer;
	Tr2BufferAL m_lineIndexBuffer;

	// How do we want to render this granny res
	bool m_renderSolid;

	// The gr2 data
	std::string m_grannyResPath;
	TriGrannyResPtr m_grannyRes;
	unsigned int m_primitiveCount;
	unsigned int m_pickingPrimitiveCount;
	unsigned int m_pickingIndexOffset;

	std::vector<TriangleVertex> m_points;
	std::vector<unsigned int> m_lineIndices;
	std::vector<unsigned int> m_triangleIndices;
};

TYPEDEF_BLUECLASS( Tr2GrannyPrimitiveSet );
#endif //Tr2GrannyPrimitiveSet_h
