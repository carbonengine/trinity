// Copyright © 2023 CCP ehf.

#pragma once
#ifndef WithValidRenderContextFixture_H
#define WithValidRenderContextFixture_H

#include "WithWindowFixture.h"

#define ENSURE_GPU_OR_SKIP                                                 \
	if( !MachineHasGfxAdapter() )                                          \
	{                                                                      \
		GTEST_SKIP() << "Test Skipped as no adapters present on machine."; \
	}

struct WithValidRenderContext : public WithWindow
{
public:
	WithValidRenderContext();

	static void SetUpTestCase();
	static void TearDownTestCase();

	static bool MachineHasGfxAdapter();
	static void MakeScreenShot( const char* outFilePath );
	void MakeTestScreenShot();

	static Tr2PresentParametersAL presentParameters;
	static Tr2PrimaryRenderContextAL* renderContext;

private:
	bool m_madeScreenshot;
};

#endif