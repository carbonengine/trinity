////////////////////////////////////////////////////////////
//
//    Created:   March 2020
//    Copyright: CCP 2020
//

#include "StdAfx.h"
#include "Tr2MainWindow.h"
#include "TriDevice.h"
#include "Tr2MouseCursor.h"


static CcpLogChannel_t s_moduleChannel = CCP_LOG_DEFINE_CHANNEL( "mainwindow" );


CcpMeanStatisticsEntry g_activeFrametimeMean;
CcpStdDevStatisticsEntry g_activeFrametimeStdDev;

namespace
{
	const Tr2RenderContextEnum::PixelFormat BACK_BUFFER_FORMAT = Tr2RenderContextEnum::PIXEL_FORMAT_B8G8R8A8_UNORM;

	ALResult PopulatePresentParams( Tr2PresentParametersAL& presentParams, Tr2WindowMode::Type windowMode, uint32_t adapter, uint32_t width, uint32_t height, Tr2RenderContextEnum::PresentInterval presentInterval )
	{
		unsigned adapterCount;
		CR_RETURN_HR( Tr2VideoAdapterInfo::GetAdapterCount( adapterCount ) );
		if( adapter >= adapterCount )
		{
			return E_INVALIDARG;
		}

		presentParams.backBufferCount = 1;
		presentParams.presentInterval = presentInterval;
		presentParams.msaaType = 1;
		presentParams.msaaQuality = 0;
		presentParams.software = false;
		presentParams.windowed = windowMode != Tr2WindowMode::FULL_SCREEN;
		presentParams.swapEffect = Tr2RenderContextEnum::SWAP_EFFECT_DISCARD;

		if( windowMode == Tr2WindowMode::FULL_SCREEN )
		{
			bool found = false;
			unsigned modeCount;
			CR_RETURN_HR( Tr2VideoAdapterInfo::GetAdapterModeCount( adapter, BACK_BUFFER_FORMAT, modeCount ) );
			for( unsigned i = 0; i < modeCount; ++i )
			{
				Tr2DisplayModeInfo info;
				CR_RETURN_HR( Tr2VideoAdapterInfo::GetAdapterMode( adapter, BACK_BUFFER_FORMAT, i, info ) );
				if( info.width == width && info.height == height )
				{
					presentParams.mode = info;
					found = true;
					break;
				}
			}
			if( !found )
			{
				return E_INVALIDARG;
			}
		}
		else
		{
			presentParams.mode = Tr2DisplayModeInfo();
			presentParams.mode.format = BACK_BUFFER_FORMAT;
			presentParams.mode.width = width;
			presentParams.mode.height = height;
		}
		return S_OK;
	}

	std::string ToString( Tr2WindowMode::Type windowMode )
	{
		switch( windowMode )
		{
		case Tr2WindowMode::FULL_SCREEN:
			return "full screen";
		case Tr2WindowMode::WINDOWED:
			return "windowed";
		case Tr2WindowMode::FIXED_WINDOW:
			return "fixed window";
		default:
			return "INVALID WINDOW MODE";
		}
	}

	std::string ToString( Tr2WindowShowState::Type windowShowState )
	{
		switch( windowShowState )
		{
		case Tr2WindowShowState::NORMAL:
			return "normal";
		case Tr2WindowShowState::MAXIMIZED:
			return "maximized";
		case Tr2WindowShowState::MINIMIZED:
			return "minimized";
		default:
			return "INVALID WINDOW SHOW STATE";
		}
	}

	std::string ToString( Tr2RenderContextEnum::PresentInterval presentInterval )
	{
		switch( presentInterval )
		{
		case Tr2RenderContextEnum::PRESENT_INTERVAL_IMMEDIATE:
			return "immediate";
		case Tr2RenderContextEnum::PRESENT_INTERVAL_ONE:
			return "one";
		default:
			return "INVALID PRESENT INTERVAL";
		}
	}


	class SetupActiveFrameTimeStats
	{
	public:

		SetupActiveFrameTimeStats()
		{
			g_activeFrametimeMean.SetName( "Trinity/FrameTime/ActiveMean" );
			CcpStatistics::RegisterDerived( &g_activeFrametimeMean );

			g_activeFrametimeStdDev.SetName( "Trinity/FrameTime/ActiveStdDev" );
			CcpStatistics::RegisterDerived( &g_activeFrametimeStdDev );
		}
	};

	static SetupActiveFrameTimeStats s_setupActiveFrameTimeStats;
}


Tr2MainWindowState::State::State()
	:windowMode( Tr2WindowMode::FULL_SCREEN ),
	adapter( 0 ),
	width( 0 ),
	height( 0 ),
	presentInterval( Tr2RenderContextEnum::PRESENT_INTERVAL_ONE ),
	left( 0 ),
	top( 0 ),
	showState( Tr2WindowShowState::NORMAL )
{
}

bool Tr2MainWindowState::State::operator==( const State& other ) const
{
	if( windowMode != other.windowMode || adapter != other.adapter || width != other.width || height != other.height || presentInterval != other.presentInterval )
	{
		return false;
	}
	if( windowMode == Tr2WindowMode::FULL_SCREEN )
	{
		return true;
	}
	if( left != other.left || top != other.top )
	{
		return false;
	}
	if( windowMode == Tr2WindowMode::FIXED_WINDOW )
	{
		return true;
	}
	return showState == other.showState;
}

std::string Tr2MainWindowState::State::ToString() const
{
	std::string result = ::ToString( windowMode ) + " on adapter " + std::to_string( (long long)( adapter ) ) +
		" " + std::to_string( (long long)( width ) ) + "x" + std::to_string( (long long)( height ) ) + ", present interval " + ::ToString( presentInterval );
	if( windowMode != Tr2WindowMode::FULL_SCREEN )
	{
		result += ", position (" + std::to_string( (long long)( left ) ) + ", " + std::to_string( (long long)( top ) ) + "), " + ::ToString( showState );
	}
	return result;
}

bool Tr2MainWindowState::State::RequiresDeviceReset( const State& other ) const
{
	return windowMode != other.windowMode || adapter != other.adapter || width != other.width || height != other.height || presentInterval != other.presentInterval;
}

ALResult Tr2MainWindowState::State::PopulatePresentParameters( Tr2PresentParametersAL& presentParams ) const
{
	return ::PopulatePresentParams( presentParams, windowMode, adapter, width, height, presentInterval );
}

Tr2MainWindowStatePtr Tr2MainWindowState::State::WrapCopy() const
{
	Tr2MainWindowStatePtr result;
	result.CreateInstance();
	result->m_state = *this;
	return result;
}


std::string Tr2MainWindowState::ToString() const
{
	return m_state.ToString();
}



Tr2MainWindow::Tr2MainWindow()
	:m_hwnd(),
	m_isResizing( false ),
    m_inSetState( false )
{
	m_minimumSize.width = m_minimumSize.height = 100;
    
    BeOS->RegisterForTicks( this, nullptr );
}

Tr2MainWindow::~Tr2MainWindow()
{
    BeOS->UnregisterForTicks( this, nullptr );

    DestroyOSWindow();
}

ALResult Tr2MainWindow::SetState( bool adjustWindow, const Tr2MainWindowState::State& newState )
{
    m_inSetState = true;
    ON_BLOCK_EXIT( [&]{ m_inSetState = false; } );
    
	Tr2MainWindowState::State state = newState;
	SanitizeState( state );

    if( state == m_state )
    {
        return S_OK;
    }

    if( HasWindow() && gTriDev->DeviceExists() && !m_state.RequiresDeviceReset( state ) )
    {
        if( adjustWindow )
        {
            AdjustWindow( state );
        }
		gTriDev->SetThrottling( TriDevice::WINDOW_HIDDEN, state.showState == Tr2WindowShowState::MINIMIZED );
        m_state = state;
        m_onWindowStateChange.CallVoid( state.WrapCopy() );
        return S_OK;
    }

	if( gTriDev->DeviceExists() )
	{
		m_onBeforeSwapChainChange.CallVoid( state.WrapCopy() );
	}

	CCP_LOG_CH( s_moduleChannel, "Trying to change main window to %s", state.ToString().c_str() );

	Tr2PresentParametersAL presentParams;

	m_isResizing = true;
	bool justCreatedWindow = false;

	if( !HasWindow() )
	{
		CCP_LOG_CH( s_moduleChannel, "Creating window" );
		CreateOSWindow( state );
		justCreatedWindow = true;
		// TODO: this SetPresentation thing is not doing what you'd think it would be doing :/
		gTriDev->SetPresentation( state.adapter, &presentParams );
	}
#ifdef _WIN32

#if USE_BORDERLESS_WINDOW
	else if( adjustWindow )
#else
	else if( adjustWindow && m_state.windowMode != Tr2WindowMode::FULL_SCREEN )
#endif

#else
		else if( adjustWindow )
#endif
	{
		AdjustWindow( state );
	}
	CR_RETURN_HR( state.PopulatePresentParameters( presentParams ) );
	presentParams.variableRefreshRateSupported = gTriDev->IsVariableRefreshRateSupported();
	presentParams.outputWindow = GetOutputWindow();

	gTriDev->SetThrottling( TriDevice::WINDOW_HIDDEN, state.showState == Tr2WindowShowState::MINIMIZED );

	if( !gTriDev->ChangeDevice( state.adapter, GetOutputWindow(), &presentParams ) )
	{
		m_isResizing = false;
		CCP_LOGWARN_CH( s_moduleChannel, "ChangeDevice failed" );
		return E_FAIL;
	}

#ifdef _WIN32
	if( adjustWindow && !justCreatedWindow && m_state.windowMode == Tr2WindowMode::FULL_SCREEN && state.windowMode != Tr2WindowMode::FULL_SCREEN )
	{
		AdjustWindow( state );
	}
#endif

	m_isResizing = false;

	m_state = state;

	CCP_LOGNOTICE_CH( s_moduleChannel, "Changed main window to %s", state.ToString().c_str() );
	m_onWindowStateChange.CallVoid( state.WrapCopy() );
	m_onSwapChainChange.CallVoid( state.WrapCopy() );

	return S_OK;
}

ALResult Tr2MainWindow::SetState( const  Tr2MainWindowState::State& state )
{
	return SetState( true, state );
}

Tr2MainWindowState::State Tr2MainWindow::GetState() const
{
	return m_state;
}

ALResult Tr2MainWindow::SetStateScript( const Tr2MainWindowState* state )
{
	return SetState( state->m_state );
}

Tr2MainWindowStatePtr Tr2MainWindow::GetStateScript() const
{
	return m_state.WrapCopy();
}

bool Tr2MainWindow::IsMinimized() const
{
	return m_state.showState == Tr2WindowShowState::MINIMIZED;
}

void Tr2MainWindow::SetMinimumSize( uint32_t width, uint32_t height )
{
	m_minimumSize.width = width;
	m_minimumSize.height = height;
}

uint32_t Tr2MainWindow::GetBackBufferWidth() const
{
	return m_state.width;
}

uint32_t Tr2MainWindow::GetBackBufferHeight() const
{
	return m_state.height;
}

Tr2RenderContextEnum::PixelFormat Tr2MainWindow::GetBackBufferFormat( uint32_t ) const
{
	return BACK_BUFFER_FORMAT;
}

std::vector<std::pair<uint32_t, uint32_t>> Tr2MainWindow::GetWindowSizeOptions( uint32_t adapter, Tr2WindowMode::Type windowMode ) const
{
	std::vector<std::pair<uint32_t, uint32_t>> result;
	SanitizeAdapter( adapter );

	auto Insert = [&result]( const std::pair<uint32_t, uint32_t>& candidate )
	{
		if( std::find( begin( result ), end( result ), candidate ) == end( result ) )
		{
			result.push_back( candidate );
		}
	};

	auto desktopSize = GetClientSize( GetLargestWindowSize( adapter, windowMode ), windowMode );

	unsigned count = 0;
	Tr2VideoAdapterInfo::GetAdapterModeCount( adapter, BACK_BUFFER_FORMAT, count );
	for( unsigned i = 0; i < count; ++i )
	{
		Tr2DisplayModeInfo info;
		if( FAILED( Tr2VideoAdapterInfo::GetAdapterMode( adapter, BACK_BUFFER_FORMAT, i, info ) ) )
		{
			continue;
		}
		if( info.width < m_minimumSize.width || info.height < m_minimumSize.height )
		{
			continue;
		}
		if( windowMode != Tr2WindowMode::FULL_SCREEN )
		{
			if( info.width > desktopSize.width || info.height > desktopSize.height )
			{
				continue;
			}
		}
		Insert( std::make_pair( info.width, info.height ) );
	}
	if( windowMode != Tr2WindowMode::FULL_SCREEN )
	{
		Insert( std::make_pair( m_state.width, m_state.height ) );

		Insert( std::make_pair( desktopSize.width, desktopSize.height ) );
	}
	return result;
}

std::wstring Tr2MainWindow::GetWindowTitle() const
{
	return m_title;
}

Tr2MouseCursorPtr Tr2MainWindow::GetMouseCursor() const
{
	return m_cursor;
}

void Tr2MainWindow::SetMouseCursor( Tr2MouseCursor* cursor )
{
	m_cursor = cursor;
	if( m_cursor )
	{
		m_cursor->Apply();
	}
}

void Tr2MainWindow::SanitizeAdapter( uint32_t& adapter ) const
{
	unsigned count = 0;
	Tr2VideoAdapterInfo::GetAdapterCount( count );
	if( adapter >= count )
	{
		adapter = 0;
	}

	if( Tr2VideoAdapterInfo::SupportsBackBufferFormat( adapter, BACK_BUFFER_FORMAT ) )
	{
		return;
	}


	for( unsigned i = 0; i < count; ++i )
	{
		if( Tr2VideoAdapterInfo::SupportsBackBufferFormat( i, BACK_BUFFER_FORMAT ) )
		{
			adapter = i;
			return;
		}
	}
	adapter = 0;
}

void Tr2MainWindow::SanitizeFullScreenResolution( Tr2MainWindowState::State& state ) const
{
	if( state.width == 0 && state.height == 0 )
	{
		// try to use current resolution
		Tr2DisplayModeInfo mi;
		Tr2VideoAdapterInfo::GetAdapterDisplayMode( state.adapter, mi );
		state.width = mi.width;
		state.height = mi.height;
	}

	unsigned count = 0;
	Tr2VideoAdapterInfo::GetAdapterModeCount( state.adapter, BACK_BUFFER_FORMAT, count );

	// Finding the best resolution, in order of priority:
	// 1. exact match for the given width, height
	// 2. the largest resolution that is smaller than given width, height
	// 3. the smallest supported resolution

	Tr2DisplayModeInfo bestMode = {};
	Tr2DisplayModeInfo smallestMode = {};

	for( unsigned i = 0; i < count; ++i )
	{
		Tr2DisplayModeInfo mode;
		if( FAILED( Tr2VideoAdapterInfo::GetAdapterMode( state.adapter, BACK_BUFFER_FORMAT, i, mode ) ) )
		{
			continue;
		}
		if( mode.width < m_minimumSize.width || mode.height < m_minimumSize.height )
		{
			continue;
		}
		if( state.width == mode.width && state.height == mode.height )
		{
			// found exact match
			return;
		}
		if( smallestMode.width == 0 || smallestMode.width > mode.width || smallestMode.height > mode.height )
		{
			smallestMode = mode;
		}
		if( ( state.width == 0 || state.width >= mode.width ) && ( state.height == 0 || state.height >= mode.height ) )
		{
			if( bestMode.width == 0 || bestMode.width < mode.width || bestMode.height < mode.height )
			{
				bestMode = mode;
			}
		}
	}
	if( bestMode.width )
	{
		state.width = bestMode.width;
		state.height = bestMode.height;
	}
	else
	{
		state.width = smallestMode.width;
		state.height = smallestMode.height;
	}
}

void Tr2MainWindow::SanitizeState( Tr2MainWindowState::State& state ) const
{
	SanitizeAdapter( state.adapter );

	if( state.windowMode == Tr2WindowMode::FULL_SCREEN && !SupportsFullscreen() )
	{
		state.windowMode = Tr2WindowMode::FIXED_WINDOW;
	}
	if( state.windowMode == Tr2WindowMode::FULL_SCREEN )
	{
		SanitizeFullScreenResolution( state );
	}
	else
	{
		SanitizeWindowedResolution( state );
	}
	if( state.presentInterval != Tr2RenderContextEnum::PRESENT_INTERVAL_IMMEDIATE && state.presentInterval != Tr2RenderContextEnum::PRESENT_INTERVAL_ONE )
	{
		state.presentInterval = Tr2RenderContextEnum::PRESENT_INTERVAL_ONE;
	}

	SanitizeWindowPosition( state );
}

void Tr2MainWindow::SanitizeStateScript( Tr2MainWindowState* state ) const
{
	SanitizeState( state->m_state );
}

Tr2MainWindowStatePtr Tr2MainWindow::GetDefaultState( Tr2WindowMode::Type mode ) const
{
	Tr2MainWindowState::State result;
	result.windowMode = mode;
	switch( mode )
	{
	case Tr2WindowMode::FULL_SCREEN:
	case Tr2WindowMode::FIXED_WINDOW:
	{
		Tr2DisplayModeInfo mi;
		Tr2VideoAdapterInfo::GetAdapterDisplayMode( result.adapter, mi );
		result.width = mi.width;
		result.height = mi.height;
		break;
	}
	case Tr2WindowMode::WINDOWED:
	{
		Tr2DisplayModeInfo mi;
		Tr2VideoAdapterInfo::GetAdapterDisplayMode( result.adapter, mi );

		Size size = { mi.width, mi.height };
		size = GetWindowSize( size, mode );

		result.width = size.width;
		result.height = size.height;
		break;
	}
	default:
		return nullptr;
	}

	Tr2MainWindowStatePtr ret;
	ret.CreateInstance();
	ret->m_state = result;
	return ret;
}

void Tr2MainWindow::StoreStateSettings( const Tr2MainWindowState* state )
{
    if( state )
    {
        m_storedStates[state->m_state.windowMode] = state->m_state;
    }
}

#ifdef __APPLE__
id Tr2MainWindow::GetWindowID() const
{
    return (id)m_hwnd;
}
#endif
