// Copyright © 2023 CCP ehf.

#ifndef Tr2LineSet_H
#define Tr2LineSet_H

#include "Tr2PrimitiveSet.h"
#include "Tr2DeviceResource.h"

// Visual data
struct LineData
{
	Vector3 m_position1;
	Color m_color1;
	Vector3 m_position2;
	Color m_color2;
};

// Picking data
struct Triangle
{
	Vector3 m_position1;
	Vector3 m_position2;
	Vector3 m_position3;
};

BLUE_CLASS( Tr2LineSet ) :
	public IInitialize,
	public Tr2PrimitiveSet,
	public Tr2DeviceResource
{
public:
	EXPOSE_TO_BLUE();

	Tr2LineSet( IRoot* lockobj = 0 );
	~Tr2LineSet();

	//////////////////////////////////////////////////////////////////////////////////////
	// IInitialize
	bool Initialize();

	//////////////////////////////////////////////////////////////////////////////////////
	// ITriDeviceResource
	virtual void ReleaseResources( TriStorage s );
#if TRINITYDEV
	virtual void GetDescription( std::string & desc )
	{
		desc = "<Tr2LineSet>";
	}
#endif

protected:
	void GetBatchesImpl( ITriRenderBatchAccumulator * accumulator, const Tr2PerObjectData* perObjectData, Tr2Material* effect, GetBatchesReason reason ) override;

	std::vector<LineData> m_lines;
	unsigned int m_maxCurrentLineCount;
	unsigned int m_currentSubmittedLineCount;

	// Picking geometry
	unsigned int m_pickingVertexDeclHandle;
	Tr2BufferAL m_pickingVertexBuffer;
	std::vector<Triangle> m_triangles;
	unsigned int m_maxCurrentTriangleCount;
	unsigned int m_currentSubmittedTriangleCount;

public:
	void AddPickingTriangle( const Vector3& position1, const Vector3& position2, const Vector3& position3 );
	void AddLine( const Vector3& position1, const Vector4& color1, const Vector3& position2, const Vector4& color2 );
	void SetCurrentColor( Color & val );
	void ClearLines();
	void ClearPickingTriangles();
	bool SubmitChanges();

private:
	virtual bool OnPrepareResources();
};

TYPEDEF_BLUECLASS( Tr2LineSet );

#endif
