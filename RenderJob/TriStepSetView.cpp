#include "StdAfx.h"
#include "TriStepSetView.h"
#include "Tr2Renderer.h"
#include "Eve/EveCamera.h"

TriStepSetView::TriStepSetView( IRoot* lockobj )
{
}

TriStepSetView::~TriStepSetView(void)
{
}

void TriStepSetView::SetViewCameraParent( TriView* view, EveCamera* camera )
{
	m_view = view;
	m_camera = camera;
}

TriStepResult TriStepSetView::Execute( Be::Time realTime, Be::Time simTime, Tr2RenderContext& renderContext )
{
	if( m_view )
	{
		Tr2Renderer::SetViewTransform( m_view->GetTransform() );
	}
	else if( m_camera )
	{
		m_camera->Update( simTime );
		Tr2Renderer::SetViewTransform( m_camera->GetViewMatrix()->GetTransform() );
	}
	return RS_OK;
}