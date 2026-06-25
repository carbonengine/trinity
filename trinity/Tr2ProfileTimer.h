// Copyright © 2017 CCP ehf.

#pragma once

#include "Tr2DeviceResource.h"

class Tr2ProfileTimer : public Tr2DeviceResource
{
public:
	Tr2ProfileTimer();

	void Begin( Tr2RenderContext& renderContext );
	void End( Tr2RenderContext& renderContext );

	bool GetCaptureGpuTime() const;
	void SetCaptureGpuTime( bool capture );

	bool GetCaptureCpuTime() const;
	void SetCaptureCpuTime( bool capture );

	float GpuTime() const;
	float CpuTime() const;

	const std::string& GetStatName() const;
	void SetStatName( const char* name );

private:
	virtual void ReleaseResources( TriStorage );
	virtual bool OnPrepareResources();

	mutable Tr2GpuTimerAL m_gpuTimer;
	uint64_t m_beginTime;
	CcpStaticStatisticsEntry* m_statEntryCpu;
	CcpStaticStatisticsEntry* m_statEntryGpu;
	std::string m_statName;
	float m_cpuTime;

	bool m_captureGpuTime;
	bool m_captureCpuTime;
};