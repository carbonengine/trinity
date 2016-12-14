#include "StdAfx.h"
#include "TriStepSetView.h"
#include "TriRenderStep.h"

BLUE_DEFINE( TriStepSetView );

const Be::ClassInfo* TriStepSetView::ExposeToBlue()
{
	EXPOSURE_BEGIN(TriStepSetView, "" )

		MAP_INTERFACE( TriRenderStep )
		MAP_INTERFACE( TriStepSetView )

		MAP_ATTRIBUTE( "view",   m_view,    "na", Be::READWRITE | Be::PERSIST)
		MAP_ATTRIBUTE( "camera", m_camera, "na", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "cameraParent", m_cameraParent, "", Be::READWRITE | Be::PERSIST )

		MAP_METHOD_AND_WRAP_OPTIONAL_ARGS
		( 
			"__init__", 
			SetViewCameraParent, 
			3,
			"Creates a render step that sets the view to the device"
			"\n:param view: TriView (default None)"
			"\n:param camera: an EveCamera (default None)"
			"\n:param cameraParent: an ITr2Transform that is the parent of the camera (default None)"
		)

	EXPOSURE_CHAINTO( TriRenderStep )
}