#include "StdAfx.h"

#include "TriStepRenderFps.h"
#include "Tr2Renderer.h"
#include "TriViewport.h"

TriStepRenderFps::TriStepRenderFps( IRoot* lockobj ) : 
	m_averageFPS( 0 ),
	m_sumFPSValues( 0 ),
	m_averageMSPerFrame( 1 ),
	m_FPSValuesCount( 0 ),
	m_nextFPSCalculationTime( 0 ),
	m_displayX( 0 ),
	m_displayY( 0 ),
	m_alignRight( true ),
	m_alignBottom( true ),
	m_dpCount( nullptr )
{
	auto& entries = CcpStatistics::GetEntryArray();
	auto found = std::find_if( 
		entries.begin(), 
		entries.end(), 
		[&]( CcpStaticStatisticsEntry *entry ) { return entry->GetName() == "Trinity/AL/sceneDrawcallCount"; } );
	if( found != entries.end() )
	{
		m_dpCount = *found;
	}
}

TriStepRenderFps::~TriStepRenderFps(void)
{
}


TriStepResult TriStepRenderFps::Execute( Be::Time realTime, Be::Time simTime, Tr2RenderContext& renderContext )
{
	// these values are static for this implementation, but it should be considered 
	// to change this in a follwing sprint
	const float fpsCalculationTime = 0.25f;
	
	// position of the fps, left top corner
	Rect screenFPSPos;
	const TriViewport& vp = Tr2Renderer::GetViewport();
	screenFPSPos.left = vp.x + m_displayX;
	screenFPSPos.top = vp.y + m_displayY;
	screenFPSPos.right = vp.x + vp.width - m_displayX;
	screenFPSPos.bottom = vp.y + vp.height - m_displayY;
	
	// this enables access to the fps
	BeInfo* beOSInfo = BeOS->GetInfo();

	uint32_t textColor;
	// TimeAsDouble is formating the time from a int64 in nanoseconds to
	// a double where seconds are the first digit before the .
	double nowTime = TimeAsDouble( beOSInfo->mRealTime );

	// to calculate the average the fps of every frame gets collected 
	m_FPSValuesCount += 1;
	m_sumFPSValues += beOSInfo->mFps;

	// This is not the best timing functionality but it is simple and correct enough
	// for these purposes (the timing is most likely a few nanoseconds off, due to the
	// fact that this doesn't check for the exact time)
	if( nowTime >= m_nextFPSCalculationTime )		
	{
		//timer control
		m_nextFPSCalculationTime = ( nowTime + fpsCalculationTime );

		// calculate the avarege fps and ms
		m_averageFPS = m_sumFPSValues/m_FPSValuesCount;
		if( m_averageFPS < 1e-5f ) 
		{		
			m_averageMSPerFrame = 0.0f;
		}
		else 
		{
			m_averageMSPerFrame = 1.0f/m_averageFPS*1000;
		}

		// clean up for the new average calculation
		m_sumFPSValues = 0.0f;
		m_FPSValuesCount = 0;
	}

	// Set the print color 
	// over 60fps green
	// between 60 and 30 fps orange
	// under 30 fps red
	// rgb color calculator http://www.drpeterjones.com/colorcalc/
	if( m_averageFPS > 59.9f ) 
	{
		textColor = 0xff00ff00;
	}
	else if( m_averageFPS > 29.5f ) 
	{
		textColor = 0xffff9900;
	}
	else 
	{
		textColor = 0xffff0000;
	}

	auto bitcount = CCP_STRINGIZE( CCP_CONCATENATE( x, PROCESS_BIT_COUNT ) );
	auto toolset = GetPlatformToolset();
	uint64_t counter = Tr2Renderer::GetCurrentFrameCounter();
	int dpCount = m_dpCount ? int( m_dpCount->GetValue() ) : 0;
	auto str = "\
trinity: %10s\n\
bitness: %10s\n\
toolset: %10s\n\
frame:   %10.0lld\n\
fps:     %10.2f\n\
ms:      %10.2f\n\
dp:      %10.0d\
";

	const int bufferSize = 256; // Make sure to increase this as necessary
	char fpsBuffer[bufferSize];

	sprintf_s( fpsBuffer, str, TRINITY_PLATFORM_NAME, bitcount, toolset, counter, m_averageFPS, m_averageMSPerFrame, dpCount );

	uint32_t flags = 0;
	if( m_alignRight )
	{
		flags |= TRI_DFS_RIGHT;
	}
	if( m_alignBottom )
	{
		flags |= TRI_DFS_BOTTOM;
	}

	Tr2Renderer::PrintfImmediate( TRI_DBG_FONT_LARGE, screenFPSPos, flags, textColor, fpsBuffer );
	
	return RS_OK;
}

