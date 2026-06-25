// Copyright © 2023 CCP ehf.

#ifndef Tr2SolidSet_H
#define Tr2SolidSet_H

#include "Tr2PrimitiveSet.h"
#include "Tr2DeviceResource.h"

// A single triangle stored locally as they are added by the user
struct TriangleData
{
	Vector3 m_position1;
	Color m_color1;
	Vector3 m_position2;
	Color m_color2;
	Vector3 m_position3;
	Color m_color3;
	Vector3 m_normal;
};

// The data that goes into the vertex buffer
struct TriangleVertex
{
	Vector3 m_position;
	Vector3 m_normal;
	Color m_color;
};

BLUE_CLASS( Tr2SolidSet ) :
	public IInitialize,
	public Tr2PrimitiveSet,
	public Tr2DeviceResource
{
public:
	EXPOSE_TO_BLUE();

	Tr2SolidSet( IRoot* lockobj = 0 );
	~Tr2SolidSet();

	//////////////////////////////////////////////////////////////////////////////////////
	// IInitialize
	bool Initialize();

	//////////////////////////////////////////////////////////////////////////////////////
	// ITriDeviceResource
	virtual void ReleaseResources( TriStorage s );

#if TRINITYDEV
	virtual void GetDescription( std::string & desc )
	{
		desc = "<Tr2SolidSet>";
	}
#endif

	Vector3 GetCenterOfMass( void );

protected:
	void GetBatchesImpl( ITriRenderBatchAccumulator * accumulator, const Tr2PerObjectData* perObjectData, Tr2Material* effect, GetBatchesReason reason ) override;

private:
	virtual bool OnPrepareResources();

	// Hold on to the submitted triangles and the current count
	std::vector<TriangleData> m_triangles;
	unsigned int m_maxCurrentTriangleCount;
	unsigned int m_currentSubmittedTriangleCount;

public:
	// Python interface
	void AddTriangle( const Vector3& position1, const Vector4& color1, const Vector3& position2, const Vector4& color2, const Vector3& position3, const Vector4& color3 );
	void ClearTriangles();
	void SetCurrentColor( Color & val );
	bool SubmitChanges();
};

TYPEDEF_BLUECLASS( Tr2SolidSet );

#endif
