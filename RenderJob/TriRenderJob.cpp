#include "StdAfx.h"
#include "TriRenderJob.h"
#include "TriRenderStep.h"


TriRenderJob::TriRenderJob( IRoot* lockobj )
	: PARENTLOCK( m_renderSteps )
	, m_threadAffinity( 0 )	
	, m_enabled( true )
	, m_status( RJ_INIT )
	, m_stackGuard( true )
{
	m_currentStep = 0;
}

TriRenderJob::~TriRenderJob(void)
{
}

/*Tr2RenderContext& TriRenderJob::GetRenderContext()
{
	USE_MAIN_THREAD_RENDER_CONTEXT();
	return m_renderContext ? *m_renderContext : renderContext;
}*/

bool TriRenderJob::SetThreadAffinity( uint32_t affinity )
{
	if( affinity == m_threadAffinity )
	{
		return true;
	}
#if 0
	if( !affinity )
	{
		m_renderContext.Unlock();
		m_threadAffinity = 0;
		return true;
	}

	m_renderContext.CreateInstance();
	if( FAILED( m_renderContext->CreateSecondaryContext() ) )
	{
		m_renderContext.Unlock();
		m_threadAffinity = 0;
		return false;
	}
#endif

	m_threadAffinity = affinity;
	return true;
}

uint32_t TriRenderJob::GetThreadAffinity() const
{
	return m_threadAffinity;
}

TriRenderJobStatus TriRenderJob::Run( Be::Time realTime, Be::Time simTime, Tr2RenderContext *renderContextPtr )
{
	D3DPERF_EVENT1( L"TriRenderJob::%S", CA2W(m_name.c_str()) );

	if( !m_enabled )
	{
		return RJ_DONE;
	}

	typedef std::vector<TriRenderStepPtr>	RenderStepVector;
	
	RenderStepVector copyOfSteps( m_renderSteps.size() );  // we need to copy steps because lists can change when executed

	for( size_t i = 0; i != m_renderSteps.size(); ++i )
	{
		copyOfSteps[i] = m_renderSteps[i];
	}

	// Note that we don't always initialize the cursor here - it is persisted 
	// across runs, in case a step does not finish its execution. Then we need to
	// continue where we left off last frame.
	// Changing the list may however result in an out-of-bounds cursor.
	if( m_status != RJ_IN_PROGRESS || m_currentStep >= copyOfSteps.size() )
	{
		m_currentStep = 0;
	}

	Tr2RenderContext &renderContext = 
			renderContextPtr	? *renderContextPtr
								: Tr2RenderContext_GetMainThreadRenderContext();

	const size_t preRT = renderContext.GetStackSizeRT();
	const size_t preDS = renderContext.GetStackSizeDS();

	TriStepResult result = RS_OK;
	for( ; m_currentStep < copyOfSteps.size(); ++m_currentStep )
	{
		if( copyOfSteps[m_currentStep] && copyOfSteps[m_currentStep]->IsEnabled() )
		{
			copyOfSteps[m_currentStep]->BeginExecute( renderContext );
			result = copyOfSteps[m_currentStep]->Execute( realTime, simTime, renderContext );
			copyOfSteps[m_currentStep]->EndExecute( renderContext );

			// We almost never want a job that leaves the renderTarget/depthStencil stack in a changed state.
			if( m_stackGuard )
			{
				CCP_ASSERT( preRT <= renderContext.GetStackSizeRT() && "Too many Pops (RT)" );
				CCP_ASSERT( preDS <= renderContext.GetStackSizeDS() && "Too many Pops (DS)" );
			}

			if( result != RS_OK )
			{
				break;
			}
		}
	}

	if( m_stackGuard )
	{
		if( result == RS_OK )
		{
			CCP_ASSERT( preRT >= renderContext.GetStackSizeRT() && "Push without a Pop (RT)" );	
			CCP_ASSERT( preDS >= renderContext.GetStackSizeDS() && "Push without a Pop (DS)");
		}
		else
		{
			// 'repair' any damage to the stack from aborted jobs
			while( renderContext.GetStackSizeRT() > preRT )
			{
				renderContext.PopRenderTarget();
			}
			while( renderContext.GetStackSizeDS() > preDS )
			{
				renderContext.PopDepthStencil();
			}
		}
	}

	switch( result )
	{
		case RS_TERMINATE:
			m_status = RJ_DONE;
			break;

		case RS_OK:
			m_status = RJ_DONE;
			break;

		case RS_FAILED:
			m_status = RJ_FAILED;
			break;

		case RS_IN_PROGRESS:
			m_status = RJ_IN_PROGRESS;
			break;

		default:
			CCP_LOGERR( "TriRenderJob::Run: Invalid TriStepResult" );
			break;
	}

	return m_status;
}

bool TriRenderJob::OnPrepareResources()
{
#if BLUE_WITH_PYTHON
	// Perform a callback to a python decorator, if one exists
	PyOS->SendEvent( this, 
					 "TriRenderJob::PrepareResources::DoPrepareResources", 
					 "DoPrepareResources", 
					 NULL, 
					 "()" );

#endif
	return true;
}

void TriRenderJob::ReleaseResources( TriStorage s )
{
#if BLUE_WITH_PYTHON
	// Perform a callback to a python decorator, if one exists
	PyOS->SendEvent( this, 
					 "TriRenderJob::ReleaseResources::DoReleaseResources", 
					 "DoReleaseResources", 
					 NULL, 
					 "(i)", 
					 s );
#endif
}