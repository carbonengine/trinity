// Copyright © 2023 CCP ehf.

#pragma once
#ifndef _TRISTEPRENDERFPS_H_
#define _TRISTEPRENDERFPS_H_


#include "TriRenderStep.h"

BLUE_CLASS( TriStepRenderFps ) :
	public TriRenderStep
{
public:
	EXPOSE_TO_BLUE();

	TriStepRenderFps( IRoot* lockobj = 0 );
	~TriStepRenderFps( void );

	//IRenderStep
	TriStepResult Execute( Be::Time realTime, Be::Time simTime, Tr2RenderContext & renderContext );

private:
	double m_sumFPSValues; // collects the differnt fps values to create the avrg
	int m_FPSValuesCount; // counts how many values are collected for the avrg
	double m_nextFPSCalculationTime; //the timer for the avrg
	CcpStaticStatisticsEntry* m_dpCount;

	double m_averageFPS; // average frames per second
	double m_averageMSPerFrame; // average milli seconds per frame

	int m_displayX;
	int m_displayY;

	bool m_alignRight;
	bool m_alignBottom;
};

TYPEDEF_BLUECLASS( TriStepRenderFps );

#endif