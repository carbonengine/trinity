#include "StdAfx.h"
#include "TriStepClear.h"


TriStepClear::TriStepClear( IRoot* lockobj ) :
	m_depth( 1.0f ),
	m_color( 0.0f, 0.0f, 0.0f, 1.0f ),
	m_stencil( 0 ),
	m_isColorCleared( true ),
	m_isDepthCleared( true ),
	m_isStencilCleared( false )
{

}

TriStepClear::~TriStepClear(void)
{
}

TriStepResult TriStepClear::Execute( Be::Time realTime, Be::Time simTime, Tr2RenderContext& renderContext )
{
	unsigned int flags = 0;

	using namespace Tr2RenderContextEnum;
	if( m_isColorCleared )
	{
		flags |= CLEARFLAGS_TARGET;
	}

	if( m_isDepthCleared )
	{
		flags |= CLEARFLAGS_ZBUFFER;
	}

	if( m_isStencilCleared )
	{
		flags |= CLEARFLAGS_STENCIL;
	}

	uint32_t c = Color(
		max(min(m_color.r, 1.0f), 0.0f),
		max(min(m_color.g, 1.0f), 0.0f),
		max(min(m_color.b, 1.0f), 0.0f),
		max(min(m_color.a, 1.0f), 0.0f)
	);

	HRESULT hr = renderContext.Clear( flags, c, m_depth, m_stencil );
	if(!SUCCEEDED(hr))
	{
		CCP_LOGERR( "Clear failed" );
	}
	return RS_OK;
}
