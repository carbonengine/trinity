// Copyright © 2023 CCP ehf.

#include "StdAfx.h"

#include "TriStepRenderFps.h"
#include "Tr2Renderer.h"
#include "TriViewport.h"
#include "pdm.h"

CCP_STATS_DECLARED_ELSEWHERE( vertexCount );
CCP_STATS_DECLARED_ELSEWHERE( smoothedGeneratedFrames );

#ifdef _WIN32
#include <ctime>

char timeString[9];
char dateString[11];

struct tm GetUnixBuildTime()
{
	auto nt_header = reinterpret_cast<const IMAGE_NT_HEADERS*>( reinterpret_cast<BYTE*>( &__ImageBase ) + __ImageBase.e_lfanew );
	__time64_t t = nt_header->FileHeader.TimeDateStamp;
	struct tm ptm;
	_gmtime64_s( &ptm, &t );

	return ptm;
}

const char* GetTime()
{
	struct tm ptm = GetUnixBuildTime();
	strftime( timeString, sizeof( timeString ), "%H:%M:%S", &ptm );
	return timeString;
}

const char* GetDate()
{
	struct tm ptm = GetUnixBuildTime();
	strftime( dateString, sizeof( dateString ), "%Y-%m-%d", &ptm );
	return dateString;
}

#else

const char* GetTime()
{
	return __TIME__;
}

const char* GetDate()
{
	return __DATE__;
}
#endif

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
		[&]( CcpStaticStatisticsEntry* entry ) { return entry->GetName() == "Trinity/AL/sceneDrawcallCount"; } );
	if( found != entries.end() )
	{
		m_dpCount = *found;
	}
}

TriStepRenderFps::~TriStepRenderFps( void )
{
}


TriStepResult TriStepRenderFps::Execute( Be::Time realTime, Be::Time simTime, Tr2RenderContext& renderContext )
{
	// these values are static for this implementation, but it should be considered
	// to change this in a follwing sprint
	const float fpsCalculationTime = 0.25f;

	// position of the fps, left top corner
	Tr2Rect screenFPSPos;
	const TriViewport& vp = renderContext.m_esm.GetViewport();
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
		m_averageFPS = m_sumFPSValues / m_FPSValuesCount;
		if( m_averageFPS < 1e-5f )
		{
			m_averageMSPerFrame = 0.0f;
		}
		else
		{
			m_averageMSPerFrame = 1.0f / m_averageFPS * 1000;
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

// accommodate for release builds, which use `-DCCP_BUILD_FLAVOR=`, e.g. macro without a value
#if ~( ~CCP_BUILD_FLAVOR + 0 ) == 0 && ~( ~CCP_BUILD_FLAVOR + 1 ) == 1
	std::string flavor{ "release" };
#else
	std::string flavor{ CCP_STRINGIZE( CCP_BUILD_FLAVOR ) };
	flavor = flavor.substr( 1 );
#endif

	int dpCount = m_dpCount ? int( m_dpCount->GetValue() ) : 0;
	int vertCount = static_cast<int>( CCP_STATS_GET( vertexCount ) );

	uint32_t displayWidth, displayHeight;
	Tr2UpscalingAL::UpscalingInfo upscalingInfo;
	{
		USE_MAIN_THREAD_RENDER_CONTEXT();
		renderContext.GetRenderTargetSize( displayWidth, displayHeight );
		upscalingInfo = renderContext.GetUpscalingInfo( Tr2Renderer::GetUpscalingContextID() );
	}
	std::string displayResolution = std::to_string( displayWidth ) + "x" + std::to_string( displayHeight );

	char upscalingBuffer[256];

	char fpsCounterBuffer[32];
	sprintf_s( fpsCounterBuffer, "%11.2f", m_averageFPS );

	if( upscalingInfo.technique != Tr2UpscalingAL::Technique::NONE )
	{
		std::string formattedUpscalingText =
			"   Upscaler: %11s\n"
			"    Setting: %11s\n"
			" Render Res: %11s\n";

		std::string techniqueName( Tr2UpscalingAL::GetTechniqueName( upscalingInfo.technique ) );
		techniqueName = techniqueName.substr( 0, std::min( (size_t)11, techniqueName.length() ) ).c_str();
		std::string renderRes = std::to_string( upscalingInfo.renderWidth ) + "x" + std::to_string( upscalingInfo.renderHeight );
		std::string settingName;
		std::string generatedFrames;
		switch( upscalingInfo.setting )
		{
		case Tr2UpscalingAL::NATIVE:
			settingName = "Native";
			break;
		case Tr2UpscalingAL::ULTRA_QUALITY:
			settingName = "UQuality";
			break;
		case Tr2UpscalingAL::QUALITY:
			settingName = "Quality";
			break;
		case Tr2UpscalingAL::BALANCED:
			settingName = "Balanced";
			break;
		case Tr2UpscalingAL::PERFORMANCE:
			settingName = "Perform";
			break;
		case Tr2UpscalingAL::ULTRA_PERFORMANCE:
			settingName = "UPerform";
			break;
		default:
			settingName = "Unknown";
			break;
		}
		if( upscalingInfo.frameGeneration )
		{
			settingName += "/FG";
			float gen = static_cast<float>( CCP_STATS_GET( smoothedGeneratedFrames ) );
			sprintf_s( fpsCounterBuffer, "%5.1f/%5.1f", m_averageFPS, gen / 100.0f );
		}
		sprintf_s(
			upscalingBuffer,
			formattedUpscalingText.c_str(),
			techniqueName.c_str(),
			settingName.c_str(),
			renderRes.c_str() );
	}
	else
	{
		sprintf_s( upscalingBuffer, "" );
	}

	std::string str =
		" Build Date: %11s\n"
		" Build Time: %11s\n"
		"   Platform: %11s\n"
		"     Flavor: %11s\n"
		"%s" // This is upscaling information
		"Display Res: %11s\n"
		"      Frame: %11.0lld\n"
		"        FPS: %11s\n"
		"         MS: %11.2f\n"
		" Draw Calls: %11.0d\n"
		" Vert Count: %11.0d";

#if __APPLE__
	str += "\n   Rosetta: %11s";
#endif

	if( PDM::IsWine() )
	{
		str += "\n      Wine: %11s";
	}

	const int bufferSize = 512; // Make sure to increase this as necessary
	char fpsBuffer[bufferSize];

	sprintf_s(
		fpsBuffer,
		str.c_str(),
		GetDate(),
		GetTime(),
		TRINITY_PLATFORM_NAME,
		flavor.c_str(),
		upscalingBuffer,
		displayResolution.c_str(),
		Tr2Renderer::GetCurrentFrameCounter(),
		fpsCounterBuffer,
		m_averageMSPerFrame,
		dpCount,
		vertCount,
#if __APPLE__
		PDM::IsRosetta() ? "Yes" : "No",
#endif
		PDM::GetWineVersion() );

	uint32_t flags = 0;
	if( m_alignRight )
	{
		flags |= TRI_DFS_RIGHT;
	}
	if( m_alignBottom )
	{
		flags |= TRI_DFS_BOTTOM;
	}

	Tr2Renderer::PrintfImmediate( renderContext, TRI_DBG_FONT_LARGE, screenFPSPos, flags, textColor, fpsBuffer );

	return RS_OK;
}
