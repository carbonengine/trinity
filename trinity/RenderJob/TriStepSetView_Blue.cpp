// Copyright © 2023 CCP ehf.

#include "StdAfx.h"
#include "TriStepSetView.h"
#include "TriRenderStep.h"
#include "Eve/EveCamera.h"

BLUE_DEFINE( TriStepSetView );

const Be::ClassInfo* TriStepSetView::ExposeToBlue()
{
	EXPOSURE_BEGIN( TriStepSetView, "" )
		MAP_INTERFACE( TriRenderStep )
		MAP_INTERFACE( TriStepSetView )

		MAP_ATTRIBUTE( "view", m_view, "", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "camera", m_camera, "", Be::READWRITE | Be::PERSIST )

		MAP_METHOD_AND_WRAP_OPTIONAL_ARGS(
			"__init__",
			SetViewCameraParent,
			2,
			"Creates a render step that sets the view to the device"
			"\n:param view: TriView (default None)"
			"\n:param camera: an EveCamera (default None)" )

	EXPOSURE_CHAINTO( TriRenderStep )
}