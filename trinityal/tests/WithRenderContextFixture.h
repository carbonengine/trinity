// Copyright © 2023 CCP ehf.

#pragma once
#ifndef WithRenderContextFixture_H
#define WithRenderContextFixture_H

#include "WithWindowFixture.h"

struct WithRenderContext : public WithWindow
{
public:
	void SetUp()
	{
		renderContext = new Tr2PrimaryRenderContextAL();
		Tr2PrimaryRenderContextAL::SetPrimaryRenderContext( renderContext );
	}

	void TearDown()
	{
		delete renderContext;
		Tr2PrimaryRenderContextAL::SetPrimaryRenderContext( nullptr );
	}

	static bool MachineHasGfxAdapter()
	{
		unsigned count = 0;
		Tr2VideoAdapterInfo::GetAdapterCount( count );
		return count > 0;
	}

	Tr2PrimaryRenderContextAL* renderContext;
};

#endif