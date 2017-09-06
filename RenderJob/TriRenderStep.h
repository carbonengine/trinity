#pragma once
#ifndef _TRIRENDERSTEP_H_
#define _TRIRENDERSTEP_H_



enum TriStepResult
{
	RS_OK,
	RS_FAILED,
	RS_IN_PROGRESS,
	RS_TERMINATE
};

class Tr2RenderContext;

BLUE_CLASS( TriRenderStep ) : public IRoot
{
public:
	EXPOSE_TO_BLUE();

	TriRenderStep( IRoot* lockobj = NULL );
	virtual ~TriRenderStep();

	bool IsEnabled() const;
	virtual TriStepResult Execute( Be::Time realTime, Be::Time simTime, Tr2RenderContext& renderContext ) = 0;

	void BeginExecute( Tr2RenderContext& renderContext );
	void EndExecute( Tr2RenderContext& renderContext );
protected:
	bool GetCaptureGpuTime() const;
	void SetCaptureGpuTime( bool capture );
	float GpuTime() const;
	float CpuTime() const;

	std::string m_name;
	mutable Tr2GpuTimerAL m_gpuTimer;
	uint64_t m_beginTime;
	float m_cpuTime;
	std::string m_statName;
	CcpStaticStatisticsEntry* m_statEntryCpu;
	CcpStaticStatisticsEntry* m_statEntryGpu;

	// Enabled/disabled flag: disabled steps are not executed
	bool m_enabled;
	bool m_captureGpuTime;
	bool m_captureCpuTime;
};

BLUE_DECLARE_VECTOR( TriRenderStep );
TYPEDEF_BLUECLASS( TriRenderStep );

#endif