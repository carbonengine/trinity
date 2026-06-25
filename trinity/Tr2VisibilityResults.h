// Copyright © 2023 CCP ehf.

#pragma once

#ifndef Tr2VisibilityResults_H
#define Tr2VisibilityResults_H

#include "include/Rect.h"

struct Tr2VisibilityEvent;

BLUE_CLASS( Tr2VisibilityResults ) :
	public IRoot
{
public:
	Tr2VisibilityResults( IRoot* lockobj = 0 );
	~Tr2VisibilityResults();

	EXPOSE_TO_BLUE();

	// Adds a visibility event to the set
	void AddVisibilityEvent( const Tr2VisibilityEvent& event );
	// Gets the number of visibility events in the set
	size_t GetNumVisibilityEvents( void ) const
	{
		return m_events.size();
	}
	// Clears the result set
	void Clear( void );
	// Get the list of events
	const std::vector<Tr2VisibilityEvent>& GetEvents( void ) const
	{
		return m_events;
	}

private:
	typedef std::vector<Tr2VisibilityEvent> EventVector;
	EventVector m_events;
};
TYPEDEF_BLUECLASS( Tr2VisibilityResults );

struct Tr2VisibilityEvent
{
	enum EventType
	{
		QUERY_BEGIN = 1 << 0,
		QUERY_END = 1 << 1,
		PORTAL_ENTER = 1 << 2,
		PORTAL_EXIT = 1 << 3,
		PORTAL_PRE_EXIT = 1 << 4,
		CELL_IMMEDIATE_REPORT = 1 << 5,
		VIEW_PARAMETERS_CHANGED = 1 << 6,
		INSTANCE_VISIBLE = 1 << 7,
		REMOVAL_SUGGESTED = 1 << 8,
		INSTANCE_IMMEDIATE_REPORT = 1 << 9,
		REGION_OF_INFLUENCE_ACTIVE = 1 << 10,
		REGION_OF_INFLUENCE_INACTIVE = 1 << 11,
		STENCIL_MASK = 1 << 12,
		TEXT_MESSAGE = 1 << 13,
		DRAW_LINE_2D = 1 << 14,
		DRAW_LINE_3D = 1 << 15,
		DRAW_BUFFER = 1 << 16,
		OCCLUSION_QUERY_BEGIN = 1 << 17,
		OCCLUSION_QUERY_END = 1 << 18,
		OCCLUSION_QUERY_GET_RESULT = 1 << 19,
		OCCLUSION_QUERY_DRAW_DEPTH_TEST = 1 << 20,
		INSTANCE_DRAW_DEPTH = 1 << 21,
		FLUSH_DEPTH = 1 << 22,
		DEPTH_PASS_BEGIN = 1 << 23,
		DEPTH_PASS_END = 1 << 24,
		COLOR_PASS_BEGIN = 1 << 25,
		COLOR_PASS_END = 1 << 26,
		TILE_BEGIN = 1 << 27,
		TILE_END = 1 << 28,
		FLUSH_GPU_COMMAND_BUFFER = 1 << 29
	};

	Tr2VisibilityEvent() :
		m_userData( NULL ),
		m_objectToWorldMatrix(),
		m_clipPlane( 0.0f, 0.0f, 0.0f, 0.0f ),
		m_scissorRect(),
		m_stencilWrite( 0 ),
		m_stencilTest( 0 ),
		m_mirrorIndex( -1 ),
		m_eventType( INSTANCE_VISIBLE ),
		m_useClipPlane( false ),
		m_isMirroredInLeftHandedSpace( false )
	{
	}

	IRootPtr m_userData;
	Matrix m_objectToWorldMatrix;
	Vector4 m_clipPlane;
	Tr2Rect m_scissorRect;
	int m_stencilWrite, m_stencilTest;
	int m_mirrorIndex;
	EventType m_eventType;
	bool m_useClipPlane, m_isMirroredInLeftHandedSpace;
};

#endif