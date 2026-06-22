// Copyright © 2023 CCP ehf.

#include "StdAfx.h"

#include "Tr2VisibilityResults.h"

Tr2VisibilityResults::Tr2VisibilityResults( IRoot* lockobj )
{
	m_events.reserve( 500 );
}

Tr2VisibilityResults::~Tr2VisibilityResults()
{
	// Clear the set
	Clear();
}

void Tr2VisibilityResults::AddVisibilityEvent( const Tr2VisibilityEvent& event )
{
	m_events.push_back( event );
}

void Tr2VisibilityResults::Clear( void )
{
	m_events.clear();
}