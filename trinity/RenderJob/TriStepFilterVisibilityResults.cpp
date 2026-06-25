// Copyright © 2010 CCP ehf.

#include "StdAfx.h"
#include "TriStepFilterVisibilityResults.h"
#include "Tr2VisibilityResults.h"

TriStepFilterVisibilityResults::TriStepFilterVisibilityResults( IRoot* lockobj ) :
	m_eventFilter( -1 ),
	m_filterType( EXCLUDE_OBJECTS_IN_LIST ),
	PARENTLOCK( m_objects )
{
}

TriStepFilterVisibilityResults::~TriStepFilterVisibilityResults( void )
{
}

// --------------------------------------------------------------------------------------
// Description:
//   Blue-exposed initializer.
// --------------------------------------------------------------------------------------
void TriStepFilterVisibilityResults::py__init__(
	Tr2VisibilityResults* input,
	Tr2VisibilityResults* output,
	Be::Optional<unsigned> eventFilter,
	Be::Optional<unsigned> filter )
{
	SetInputResults( input );
	SetOutputResults( output );
	if( eventFilter.IsAssigned() )
	{
		SetEventFilter( eventFilter );
	}
	if( filter.IsAssigned() )
	{
		SetFilterType( FilterType( filter.GetValue() ) );
	}
}

// -------------------------------------------------------------
// Description:
//   Implements TriRenderStep method. Processes input
//   visibility result list to produce a new one filtering
//   visibility events. Removes events not present on m_eventFilter
//   bitfield filter. For events that specifies object instances
//   does filtering according to filter type and objects list.
// Arguments:
//   time - System time (not used)
// Return value:
//   RS_OK always
// -------------------------------------------------------------
TriStepResult TriStepFilterVisibilityResults::Execute( Be::Time realTime, Be::Time simTime, Tr2RenderContext& renderContext )
{
	if( m_inputResults && m_outputResults )
	{
		m_outputResults->Clear();

		const std::vector<Tr2VisibilityEvent>& events = m_inputResults->GetEvents();
		for( std::vector<Tr2VisibilityEvent>::const_iterator i = events.begin(); i != events.end(); ++i )
		{
			if( ( i->m_eventType & m_eventFilter ) == 0 )
			{
				continue;
			}

			if( i->m_userData )
			{
				if( m_filterType == EXCLUDE_OBJECTS_IN_LIST )
				{
					if( m_objects.FindKey( i->m_userData ) != -1 )
					{
						continue;
					}
				}
				else
				{
					if( m_objects.FindKey( i->m_userData ) == -1 )
					{
						continue;
					}
				}
			}
			m_outputResults->AddVisibilityEvent( *i );
		}
	}
	return RS_OK;
}

// -------------------------------------------------------------
// Description:
//   Sets visibility event type filter.
// Arguments:
//   eventFilter - Visibility event type filter composed of
//                 Tr2VisibilityEvent::EventType bits. Events that
//                 don't have corresponding bit set in eventFilter
//                 will be ommited during VisibilityQuery call.
// -------------------------------------------------------------
void TriStepFilterVisibilityResults::SetEventFilter( unsigned int eventFilter )
{
	m_eventFilter = eventFilter;
}

// -------------------------------------------------------------
// Description:
//   Sets the type of per-object filter. Can be ONLY_OBJECTS_IN_LIST
//   to include events with objects that are only present in the
//   objects list or can be EXCLUDE_OBJECTS_IN_LIST to omit
//   objects present in the objects list.
// Arguments:
//   filterType - type of per-object filter.
// -------------------------------------------------------------
void TriStepFilterVisibilityResults::SetFilterType( FilterType filterType )
{
	m_filterType = filterType;
}
