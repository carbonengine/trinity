////////////////////////////////////////////////////////////
//
//    Created:   October 2018
//    Copyright: CCP 2018
//

#include "StdAfx.h"
#include "Tr2ControllerEventHandler.h"
#include "Actions/ITr2ControllerAction.h"


Tr2ControllerEventHandler::Tr2ControllerEventHandler( IRoot* lockobj )
	:PARENTLOCK( m_actions ),
	m_controller( nullptr )
{
	m_actions.SetNotify( this );
}

void Tr2ControllerEventHandler::OnListModified( long event, ssize_t key, ssize_t key2, IRoot* value, const IList* list )
{
	if( list == &m_actions )
	{
		switch( event & BELIST_EVENTMASK )
		{
		case BELIST_INSERTED:
			if( m_controller )
			{
				if( ITr2ControllerActionPtr action = BlueCastPtr( value ) )
				{
					action->Link( *m_controller );
				}
			}
			break;
		case BELIST_REMOVED:
			if( ITr2ControllerActionPtr action = BlueCastPtr( value ) )
			{
				action->Unlink();
			}
			break;
		default:
			break;
		}
	}
}

void Tr2ControllerEventHandler::Link( Tr2Controller& controller )
{
	Unlink();

	m_controller = &controller;
	for( auto it = begin( m_actions ); it != end( m_actions ); ++it )
	{
		( *it )->Link( controller );
	}
}

void Tr2ControllerEventHandler::Unlink()
{
	if( !m_controller )
	{
		return;
	}
	for( auto it = begin( m_actions ); it != end( m_actions ); ++it )
	{
		( *it )->Unlink();
	}
}

const char* Tr2ControllerEventHandler::GetName() const
{
	return m_name.c_str();
}

void Tr2ControllerEventHandler::Execute( Tr2Controller& controller )
{
	for( auto it = begin( m_actions ); it != end( m_actions ); ++it )
	{
		( *it )->Start( controller );
	}
	for( auto it = begin( m_actions ); it != end( m_actions ); ++it )
	{
		( *it )->Stop( controller );
	}
}
