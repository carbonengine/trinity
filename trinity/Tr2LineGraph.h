// Copyright © 2023 CCP ehf.

#pragma once
#ifndef Tr2LineGraph_h
#define Tr2LineGraph_h


#include "Tr2DeviceResource.h"

BLUE_DECLARE( Tr2LineGraph );
BLUE_DECLARE_VECTOR( Tr2LineGraph );

class Tr2LineGraph : public ICcpStatisticsAccumulator,
					 public Tr2DeviceResource
{
public:
	EXPOSE_TO_BLUE();
	Tr2LineGraph( IRoot* lockobj = NULL );
	virtual ~Tr2LineGraph();

	// For saving stats out
	std::vector<float> GetStatsHistory() const;

	unsigned int GetSize();
	void SetSize( unsigned int size );

	const Color& GetColor() const;
	void SetColor( const Color& val );

	const std::string& GetName() const;

	void Add( float value );
	void AddMarker( const std::string& name );

	float GetMaxValue() const;

	void Render( float scale, Tr2RenderContext& renderContext );

	//////////////////////////////////////////////////////////////////////////
	// ICcpStatisticsAccumulator
	void Add( double value );

	/////////////////////////////////////////////////////////////////////////////
	// ITriDeviceResource
	void ReleaseResources( TriStorage s );

private:
	bool OnPrepareResources();

	typedef TrackableStdVector<float> FloatVector_t;

	struct Marker
	{
		unsigned int m_index;
		unsigned int m_ticksLeft;
		std::list<std::string> m_values;
	};

	typedef TrackableStdList<Marker> MarkerList_t;

	std::string m_name;
	FloatVector_t m_data;
	unsigned int m_currentIx;
	Color m_color;

	MarkerList_t m_markers;

	bool m_isPrepared;
	float m_maxValue;

	unsigned int m_vertexDeclaration;
	Tr2BufferAL m_vertexBuffer;
	unsigned int m_primitiveCount;
};

TYPEDEF_BLUECLASS( Tr2LineGraph );

#endif //Tr2LineGraph_h
