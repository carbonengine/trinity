////////////////////////////////////////////////////////////
//
//    Created:   January 2021
//    Copyright: CCP 2021
//

#include "StdAfx.h"
#include "Tr2LoadPrepareFence.h"


Tr2LoadPrepareFence::Tr2LoadPrepareFence() :
	m_resourceLoadCbId( 0 ),
	m_resourcePrepCbId( 0 ),
	m_reached( true )
{
	BeResMan->AddToQueue( BRMQ_BACKGROUND, StaticResourceLoadFinished, this, IBlueCallbackMan::BCBF_FENCE, &m_resourceLoadCbId );
}

Tr2LoadPrepareFence::~Tr2LoadPrepareFence()
{
	Cancel();
}

void Tr2LoadPrepareFence::Put( const std::function<void()>& onPrepared )
{
	Cancel();
	m_reached = false;
	m_onPrepared = onPrepared;
	BeResMan->AddToQueue( BRMQ_BACKGROUND, StaticResourceLoadFinished, this, IBlueCallbackMan::BCBF_FENCE, &m_resourceLoadCbId );
}

bool Tr2LoadPrepareFence::IsReached() const
{
	return m_reached;
}

void Tr2LoadPrepareFence::Cancel()
{
	if( m_resourceLoadCbId )
	{
		BeResMan->CancelFromQueue( BRMQ_BACKGROUND, m_resourceLoadCbId );
		m_resourceLoadCbId = 0;
	}

	if( m_resourcePrepCbId )
	{
		BeResMan->CancelFromQueue( BRMQ_MAIN, m_resourcePrepCbId );
		m_resourcePrepCbId = 0;
	}
	m_reached = true;
	m_onPrepared = {};
}

void Tr2LoadPrepareFence::StaticResourceLoadFinished( void* pContext )
{
	auto self = static_cast<Tr2LoadPrepareFence*>( pContext );
	BeResMan->AddToQueue( BRMQ_MAIN, StaticResourcePrepFinished, pContext, 0, &self->m_resourcePrepCbId );
	self->m_resourceLoadCbId = 0;
}

void Tr2LoadPrepareFence::StaticResourcePrepFinished( void* pContext )
{
	auto self = static_cast<Tr2LoadPrepareFence*>( pContext );
	self->m_resourcePrepCbId = 0;
	self->m_reached = true;
	if( self->m_onPrepared )
	{
		self->m_onPrepared();
	}
}
