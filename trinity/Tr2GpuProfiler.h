// Copyright © 2021 CCP ehf.

#pragma once

BLUE_DECLARE( Tr2GpuProfiler );
BLUE_CLASS_IMPL( Tr2GpuProfiler );

class Tr2GpuProfiler : public IRoot
{
public:
	EXPOSE_TO_BLUE();

	Tr2GpuProfiler( IRoot* lockobj = nullptr );

	static Tr2GpuProfiler& GetProfiler();

	// Requests profile capture for the next frame
	void Capture();
	bool IsCapturing();

	// Should be called in the beginning of the frame, before any GPU work
	void BeginFrame( uint64_t frameNumber );
	// Should be called after all the GPU work for the frame is submitted
	void EndFrame();

	// Starts a profile zone
	void Begin( IRoot* owner, const char* label, Tr2RenderContextAL& renderContext );
	// Ends the previously started profile zone
	void End( Tr2RenderContextAL& renderContext );

	bool IsDataReady();
	BlueScriptValue GetFrameReport();
	void ClearData();

private:
	struct Zone
	{
		enum Type
		{
			REGION_BEGIN,
			REGION_END,
		} type;
		Tr2PipelineStatsQueryAL query;
		Tr2GpuTimerAL timer;
		IRootPtr owner;
		std::string message;
	};

	BlueScriptValue GetRegionReport( size_t& index );

	std::vector<Zone> m_zones;
	std::vector<size_t> m_stack;

	Tr2FenceAL m_frameFence;
	bool m_pendingCapture;
	bool m_capturing;
};

// RAII wrapper for the global Tr2GpuProfiler instance Begin/End calls
class Tr2GpuProfilerZone
{
public:
	Tr2GpuProfilerZone( IRoot* owner, const char* label, Tr2RenderContextAL& renderContext );
	Tr2GpuProfilerZone( const Tr2GpuProfilerZone& ) = delete;
	Tr2GpuProfilerZone& operator=( const Tr2GpuProfilerZone& ) = delete;
	~Tr2GpuProfilerZone();

private:
	Tr2RenderContextAL& context;
};

TYPEDEF_BLUECLASS( Tr2GpuProfiler );
