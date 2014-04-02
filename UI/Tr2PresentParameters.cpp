////////////////////////////////////////////////////////////////////////////////
//
// Created:		August 2013
// Copyright:	CCP 2013
//

#include "StdAfx.h"
#include "Tr2PresentParameters.h"


Tr2PresentParameters::Tr2PresentParameters( IRoot* lockobj /*= nullptr */ )
{
	backBufferCount = 0;
	msaaType = 0;
	msaaQuality = 0;
	swapEffect = Tr2RenderContextEnum::SWAP_EFFECT_DISCARD;
	depthStencilFormat = Tr2RenderContextEnum::DSFMT_AUTO;
	outputWindow = 0;
	windowed = false;
	presentInterval = Tr2RenderContextEnum::PRESENT_INTERVAL_ONE;
}
