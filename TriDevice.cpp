#include "StdAfx.h"

#include "TriDevice.h"
#include "TriError.h"
#include "Tr2Renderer.h"

#include "TriPythonContext.h"
#include "RenderJob/Tr2RenderJobs.h"
#include "Curves/TriCurveSet.h"
#include "Tr2RenderTargetGrabber.h"
#include "Include/TriMath.h"

#include "Blue/Include/IBlueCallbackMan.h"

using namespace Tr2RenderContextEnum;

#ifdef _WIN32
extern std::vector<HANDLE> g_D3DCreatedHeaps;
#endif

#if defined(__ANDROID__)
extern int g_windowResized;
#endif

namespace {

	HRESULT CreateDeviceInt(
		uint32_t Adapter,
		Tr2WindowHandle hFocusWindow,
		const Tr2PresentParametersAL& pPresentationParameters )
	{
		USE_MAIN_THREAD_RENDER_CONTEXT();

#ifdef _WIN32
		HANDLE heapsBefore[256];
		const uint32_t countBefore = ::GetProcessHeaps( 256, heapsBefore );	
#endif
	

		const HRESULT hr = renderContext.CreateDevice( Adapter, hFocusWindow, pPresentationParameters );

#if defined(__ANDROID__)
        while( !g_windowResized )
        {
        }
#endif
        
#ifdef _WIN32
		HANDLE heapsAfter[256];
		const uint32_t countAfter = ::GetProcessHeaps( 256, heapsAfter );

		if( countAfter > countBefore )
		{
			const int count = countAfter - countBefore;
			CCP_LOG( "Direct3D::CreateDevice created %d heaps", count );

			for( uint32_t i = countBefore; i != countAfter; ++i )
			{
				g_D3DCreatedHeaps.push_back( heapsAfter[i] );
			}
		}
#endif


		return hr;
	}
}


CCP_STATS_DECLARE( deviceOnTick, "Trinity/device/OnTick", true, CST_TIME, "Time spent in TriDevice::OnTick" );

CCP_STATS_DECLARE( fpsRaw, "FPSRaw", false, CST_COUNTER_LOW, "Frames per second (raw values)");
CCP_STATS_DECLARE( fps, "FPS", false, CST_COUNTER_LOW, "Frames per second");
CCP_STATS_DECLARE( smoothedFrameTime, "Trinity/SmoothedFrameTime", false, CST_TIME, "Frame time smoothed over a number of frames");
CCP_STATS_DECLARE( frameTime, "Trinity/FrameTime", false, CST_TIME, "Frame time" );
CCP_STATS_DECLARE( frameTimeMin, "Trinity/FrameTime/Min", false, CST_TIME, "Frame time (min)" );
CCP_STATS_DECLARE( frameTimeMax, "Trinity/FrameTime/Max", false, CST_TIME, "Frame time (max)" );
CCP_STATS_DECLARE( frameTimeAbove100ms, "Trinity/FrameTimeAbove100ms", false, CST_COUNTER_LOW, "Number of frames where the frame time was above 100ms (but below 200)");
CCP_STATS_DECLARE( frameTimeAbove200ms, "Trinity/FrameTimeAbove200ms", false, CST_COUNTER_LOW, "Number of frames where the frame time was above 200ms (but below 300)");
CCP_STATS_DECLARE( frameTimeAbove300ms, "Trinity/FrameTimeAbove300ms", false, CST_COUNTER_LOW, "Number of frames where the frame time was above 300ms (but below 400)");
CCP_STATS_DECLARE( frameTimeAbove400ms, "Trinity/FrameTimeAbove400ms", false, CST_COUNTER_LOW, "Number of frames where the frame time was above 400ms (but below 500)");
CCP_STATS_DECLARE( frameTimeAbove500ms, "Trinity/FrameTimeAbove500ms", false, CST_COUNTER_LOW, "Number of frames where the frame time was above 500ms");
CCP_STATS_DECLARE( presentTime, "Trinity/PresentTime", true, CST_TIME, "Time spent in Present call");

// NOTE: This is a global pointer to a ROT object, initialized by it
// We never want this rot object to die, so we hold a reference here.
BlueBasicPtr<TriDevice> gTriDev;

unsigned long g_currentFrameCounter = 0;

TriDevice::ResourceSet	TriDevice::s_resourcesToBeRemoved;
bool TriDevice::s_iteratingForRelease = false;

const char* TRINITY = "Trinity"; //cookie for the tick

TriDevice::TriDevice(IRoot* lockobj) : 
	mHwnd             ( 0 ),
	mHeight           ( 0 ),
	mWidth            ( 0 ),

	// default render states
	mHdrEnable		  ( false ),
	mBackBufferCount ( 1                     ),
	mSwapEffect		 ( SWAP_EFFECT_DISCARD ),
	mDeviceLost( true ),
	m_deviceType( TriDevice::DEVICE_TYPE_HARDWARE ),

	mDisplay( false ),
	mAdapter ( 0 ),
	mTickInterval ( 10 ), // ten ms between ticks
	m_ignoreInvalidate ( false ),
	PARENTLOCK( mViewport ),
	m_animationTime( 0.0f ),
	m_animationTimeScale( 1.0f ),
	m_mipLevelMaxChainLength( 0x7fffffff ),
	m_mipLevelSkipCount( 0U ),
	PARENTLOCK( m_curveSets ),
	m_beginFrameCallbacks( "TriDevice::m_beginFrameCallbacks" ),
	m_presentCallbacks( "TriDevice::m_presentCallbacks" )
{	
	Tr2DisplayModeInfo empty = {0};
	mDisplayMode = empty;
	// At this point, in the constructor, we are just setting defaults.  We really 
	// don't know if the device will require an adapter or not (in the case of
	// software rendering an adapter is not necessary).
	ALResult ignoreThisResult = 
		Tr2VideoAdapterInfo::GetAdapterDisplayMode( mAdapter, mDisplayMode );
	// In the case where an adapter wouldn't be required (we don't know yet) it 
	// is fine for the method above to fail so we must explicitly ignore the result
	// of calling the method (otherwise the results checking, when enabled, would 
	// complain).
	ignoreThisResult.GetResult();

	// Set default presentation parameters
	mPresentParam.mode.width = 0;
	mPresentParam.mode.height = 0;
	mPresentParam.mode.refreshRateNumerator = 0;
	mPresentParam.mode.refreshRateDenominator = 1;
	mPresentParam.mode.format = PIXEL_FORMAT_UNKNOWN;
	mPresentParam.mode.scaling = DISPLAY_SCALING_UNSPECIFIED;
	mPresentParam.mode.scanlineOrdering = SCANLINE_ORDER_UNSPECIFIED;
	mPresentParam.backBufferCount  = mBackBufferCount;

	mPresentParam.msaaType = 0;		
	mPresentParam.msaaQuality = 0;		

	mPresentParam.swapEffect       = mSwapEffect;
	mPresentParam.outputWindow	   = 0;
	mPresentParam.windowed		   = true;
	
#if( TRINITY_PLATFORM==TRINITY_DIRECTX11 )
	mPresentParam.depthStencilFormat = DSFMT_D32F;
#else
	mPresentParam.depthStencilFormat = DSFMT_D24S8;
#endif

	mPresentParam.presentInterval = PRESENT_INTERVAL_ONE;

	mCreationTime = 0;

	BeOS->RegisterForSimTimeRebase( this );

	BeClasses->CreateInstanceFromName("BlueCallbackMan", BlueInterfaceIID<IBlueCallbackMan>(), (void**)&m_postUpdateCallbacks );
}


TriDevice::~TriDevice()
{
    m_scene = (ITr2Scene*)NULL;

	BeOS->UnregisterForSimTimeRebase( this );
	//This is handled in a subclass, so that the IRoot interface is intact when it happens
	//Invalidate(TRIRO_ALL);
}

bool TriDevice::CreateSimpleDevice(
	Tr2WindowHandle hwnd,
	unsigned int width,
	unsigned int height,
	DeviceScreenType type, 
	Tr2RenderContextEnum::PresentInterval presentInterval )
{
	// Clean out old resources and the old device (if exists)
	Invalidate( TRISTORAGE_ALL );

	// Build a windowd device:
	Tr2PresentParametersAL pp = mPresentParam;
	
	//Create the device, using those creation parameters present in the device.
	pp.mode.width  = width;
	pp.mode.height = height;
	pp.outputWindow = hwnd;
	pp.software = (m_deviceType == TriDevice::DEVICE_TYPE_SOFTWARE);

	pp.windowed = false;
	if( type == FULLSCREEN )
	{
		Tr2DisplayModeInfo mode;
		if( FAILED( Tr2VideoAdapterInfo::GetAdapterDisplayMode( Tr2VideoAdapterInfo::DEFAULT_ADAPTER, mode ) ) )
		{
			TriError::ReportError( 0, Clsid(), "Failed to get adapter display mode!" );
			return false;
		}
		pp.mode.format = mode.format;
		pp.mode.refreshRateNumerator = mode.refreshRateNumerator;
		pp.mode.refreshRateDenominator = mode.refreshRateDenominator;
	}
	else if( type == WINDOWED )
	{
		pp.windowed = true;
	}
	pp.presentInterval = presentInterval;

	//take nvperfhud into account!
	// Set default settings
	uint32_t adapterToUse = Tr2VideoAdapterInfo::DEFAULT_ADAPTER;

	CreateDeviceInt( adapterToUse, hwnd, pp );

	if( !DeviceExists() )
	{
		// failed to create a device, forced to give up.
		TriError::ReportError( E_FAIL, Clsid(), "Failed to create compatible device!" );
		return false;
	}

	mPresentParam = pp;

	mWidth = width;
	mHeight = height;
	mHwnd = hwnd;
	mDeviceLost = false;

	if( !InitD3DDevice() )
	{
		return false;
	}

	PrepareDeviceResources();

	mDisplay = true;
	BeOS->RegisterForTicks(this, (void*)TRINITY);

	return true;
}

bool TriDevice::InitD3DDevice()
{
	// Get the default viewport
	// fill out local structures
	if( !DeviceExists() )
	{
		return false;
	}

	if( mPresentParam.IsAdapterRequired()
		&& FAILED( Tr2VideoAdapterInfo::GetAdapterDisplayMode( 
						Tr2VideoAdapterInfo::DEFAULT_ADAPTER, 
						mDisplayMode ) ) 
		)
	{
		return false;
	}

	
	mViewport.x			= 0;
	mViewport.y			= 0;
	mViewport.width		= int( mWidth );
	mViewport.height	= int( mHeight );
	mViewport.minZ		= 0;
	mViewport.maxZ		= 1.0f;

	SetDefaultRenderStates();

	mDeviceLost = false;
	return true;
}

bool TriDevice::ChangeDevice(
		uint32_t adapter,
		Tr2WindowHandle hWnd,
		const Tr2PresentParametersAL* pp )
{
	bool resetOnly = true;
	if( !DeviceExists()	||  hWnd != mHwnd || Tr2VideoAdapterInfo::AreAdaptersDifferent( adapter, mAdapter ) )
	{
		resetOnly = false;	// reset is not enough, we need to recreate the device.
	}

	if( !pp )
	{
		pp = &mPresentParam;
	}

	if( resetOnly && ResetDevice( adapter, pp ) )
	{
		return true;  // reset was enough.
	}

	// Okay, we are creating a new device.
	// release all resources etc.
	// We can handle video memory exhaustion gracefully by releasing the device first.
	if( DeviceExists() )
	{
		ReleaseDeviceResources( TRISTORAGE_ALL );
		Tr2RenderContext::DestroyMainThreadRenderContext();	
	}

	if( FAILED( CreateDeviceInt( adapter, hWnd, *pp ) ) )
	{
		return false;
	}

	Tr2Renderer::SetResourceCreationAllowed( true );

	mAdapter = adapter;
	mHwnd = hWnd;
	mWidth = pp->mode.width;
	mHeight = pp->mode.height;
	mPresentParam = *pp;
	// find out the mode we are in now
	if( mPresentParam.IsAdapterRequired() &&
		FAILED( Tr2VideoAdapterInfo::GetAdapterDisplayMode( mAdapter, mDisplayMode ) ) )
	{
		return false;
	}
	
	InitD3DDevice();
		
	PrepareDeviceResources(); //call python to recreate its stuff.

	return true;
}

void TriDevice::SetGeometryLoadDisabled( bool value )	{ Tr2Renderer::m_disableGeometryLoad	= value; }
void TriDevice::SetTextureLoadDisabled( bool value )	{ Tr2Renderer::m_disableTextureLoad		= value; }
void TriDevice::SetEffectLoadDisabled( bool value )		{ Tr2Renderer::m_disableEffectLoad		= value; }
void TriDevice::SetAsyncLoadDisabled( bool value )		{ Tr2Renderer::m_disableAsyncLoad		= value; }
bool TriDevice::GetGeometryLoadDisabled()				{ return  Tr2Renderer::m_disableGeometryLoad; }
bool TriDevice::GetTextureLoadDisabled()				{ return  Tr2Renderer::m_disableTextureLoad; }
bool TriDevice::GetEffectLoadDisabled()					{ return  Tr2Renderer::m_disableEffectLoad; }
bool TriDevice::GetAsyncLoadDisabled()					{ return  Tr2Renderer::m_disableAsyncLoad; }


void TriDevice::Update( Be::Time realTime, Be::Time simTime )
{
#if BLUE_WITH_PYTHON
	AutoTasklet _at( PyOS->GetTaskletTimer(), "TriDevice::Update" );
#endif

	TriSrand( simTime );

	UpdateCursor();

	UpdateList &updateArray = GetUpdateList();
	for( UpdateList::iterator i = updateArray.begin(); i!= updateArray.end(); ++i )
	{
		(*i)->Update( realTime, simTime );
	}

	// Callbacks to Python when curve sets finish may add curve sets to the device curve sets
	// list. This will mess up the iterator unless we iterate over a copy.
	CTriCurveSetVector curveSets;
	for( auto it = m_curveSets.cbegin(); it != m_curveSets.cend(); ++it )
	{
		curveSets.Insert( -1, (*it)->GetRawRoot() );
	}
	for( auto it = curveSets.cbegin(); it != curveSets.cend(); ++it )
	{
		(*it)->Update( realTime, simTime );
	}
	curveSets.Remove(-1);

	// Remember, m_curveSets is a BlueList, must use Remove to get ref-count right.
	for( ssize_t i = 0; i < m_curveSets.GetSize(); )
	{
		TriCurveSet* cs = m_curveSets[i];
		if( !cs->IsPlaying() )
		{
			m_curveSets.Remove( i );
		}
		else
		{
			++i;
		}
	}
}

void TriDevice::Shutdown()
{
#if BLUE_WITH_PYTHON
	m_pyResourceSet.Release();  //must release all python stuff, before python is shut down.
#endif

	//also release children
	m_scene.Unlock();

	Invalidate( TRISTORAGE_ALL );

#if TRINITYDEV
	if( !GetResourcesRegistered().empty() )
	{
		OutputDebugString( "Resources still active at shutdown:\n" );
		DumpResources();
	}
#endif

	gTriDev = 0;
}

void TriDevice::Invalidate( long level )
{
	if( m_ignoreInvalidate )
	{
		return;
	}
	
	mDisplay = false;

	if( DeviceExists() )
	{
		Tr2Renderer::SetResourceCreationAllowed( false );
		ReleaseDeviceResources( (TriStorage)level );  //we are about to release device.
		Tr2Renderer::SetResourceCreationAllowed( true );
		Tr2RenderContext::DestroyMainThreadRenderContext();	
	}
}


//This function attempts to release all device resources that have been allocated
//using the D3DPOOL_DEFAULT  pool (level == TRIRO_DEFAULT) or all device resources (level == TRIRO_ALL).
//This is necessary prior to resetting the device.
void TriDevice::ReleaseDeviceResources( TriStorage s )
{
	if( m_ignoreInvalidate )
	{
		return;
	}

	m_ignoreInvalidate = true;  //to prevent recursion
	ON_BLOCK_EXIT( [&]{ m_ignoreInvalidate = false; } );

	Tr2AutoResetObjectAL::ReleaseALResources();

	// Call those objects that registered with us.  This is the preferred modus operandi in future.
	//The new resource registry.  Objects put themselves here.
	auto& rs = GetResourcesRegistered();

	s_iteratingForRelease = true;
	for( auto it = rs.begin(); it != rs.end(); ++it )
	{
		if( !*it  )
		{
			CCP_LOGWARN( "NULL TriDeviceResource found in resource set during TriDevice::ReleaseDeviceResources!" );
		}
		else
		if( s_resourcesToBeRemoved.find( *it ) != s_resourcesToBeRemoved.end() )
		{
			// ignore, was Unregistered while iterating. We could erase() right here and now,
			// but then we could leave an object dangling until the next reset: happens if we already
			// saw that object earlier in this loop, but gets unregistered due to a ReleaseResources
			// call that comes next.
		}
		else
		{
			(*it)->ReleaseResources( s );
		}		
	}
	s_iteratingForRelease = false;
	for( auto it = s_resourcesToBeRemoved.cbegin(); it != s_resourcesToBeRemoved.cend(); ++it )
	{
		rs.erase( *it );
	}
	s_resourcesToBeRemoved.clear();

	USE_MAIN_THREAD_RENDER_CONTEXT();

	// Same goes for the effect state manager
	renderContext.m_esm.ReleaseDeviceResources( s );
	Tr2Renderer::ReleaseDeviceResources( s );

#if BLUE_WITH_PYTHON
	//And the python objects too:
	if( m_pyResourceSet )
	{
		BluePySeq keys = BluePy(PyObject_CallMethod( m_pyResourceSet, const_cast<char*>("keys"), 0) );
        if( !keys )
        {
            CCP_LOGERR( "TriDev: Python callback failed in \"ReleaseDeviceResources\"" );
            PyOS->PyError();
        }
		Py_ssize_t n = keys.Size();
		for( Py_ssize_t i = 0; i<n; i++ ) 
		{
			BluePy item(keys.Get(i));
			if( !PyObject_HasAttrString( item.o, "OnInvalidate" ) )
			{
				// OnInvalidate method is optional, often not needed
				continue;
			}
			BluePy result( PyOS->CallMethodWithTrap(item, "OnInvalidate", "OnInvalidate", "(i)", (int)s) );
			if (!result) 
			{
				PyOS->PyError();
			}
		}
	}
#endif

	renderContext.ReleaseDeviceResources();
}

void TriDevice::RebuildDeviceResourcesInPython()
{
	//Call the OnCreate handler on python objects.  This allows them to recreate stuff that they
	//had to release because device became invalid.
#if BLUE_WITH_PYTHON
	if( m_pyResourceSet )
	{
		BluePySeq keys = BluePy(PyObject_CallMethod(m_pyResourceSet, const_cast<char*>( "keys" ), 0));
        if (!keys)
        {
            CCP_LOGERR( "TriDev: Python callback failed in \"RebuildDeviceResourcesInPython\"" );
            PyOS->PyError();
        }
		Py_ssize_t n = keys.Size();
		for( Py_ssize_t i = 0; i<n; i++ )
		{
			BluePy item(keys.Get(i));
			BluePy result(PyOS->CallMethodWithTrap(item, "OnCreate", "OnCreate", "(N)", PyOS->WrapBlueObject(GetRawRoot())));
			if (!result) 
			{
				PyOS->PyError();
			}
		}
	}
#endif
}

const Matrix* TriDevice::GetProjectionMatrix()		{ return &Tr2Renderer::GetProjectionTransform(); }
const Matrix* TriDevice::GetViewMatrix()			{ return &Tr2Renderer::GetViewTransform(); }
const Matrix* TriDevice::GetInvViewMatrix()			{ return &Tr2Renderer::GetInverseViewTransform(); }


float TriDevice::AspectRatio()
{
	if( mViewport.height == 0 )
	{
		CCP_ASSERT(0); // should never get here, programmer or usage error it ir happens
		return 0.0f;
	}
	
	return (float)mViewport.width / (float)mViewport.height;
}

void TriDevice::GetPresentation( int& adapter, Tr2PresentParametersAL& d3dpp )
{
	adapter = mAdapter;
	d3dpp = mPresentParam;
}

/////////////////////////////////////////////////////////////////////////////////////////
// ITriDevice::SetPresentation
// Called from the App to set some attributes
/////////////////////////////////////////////////////////////////////////////////////////
bool TriDevice::SetPresentation( int adapter, const Tr2PresentParametersAL* d3dpp )
{
	if( d3dpp )
	{
		mAdapter = adapter;
		mPresentParam = *d3dpp;
		mHwnd = d3dpp->outputWindow;
		mWidth = d3dpp->mode.width;
		mHeight = d3dpp->mode.height;
		BeOS->RegisterForTicks(this, (void*)TRINITY);
		mDisplay = true;
	} 
	else 
	{
		InvalidateAndUnregisterForTicks();
	}
	return true;
}

void TriDevice::InvalidateAndUnregisterForTicks()
{
	Invalidate( TRISTORAGE_ALL );
	BeOS->UnregisterForTicks(this, (void*)TRINITY);
	mDisplay = false;
	mHwnd = 0;
	mWidth = mHeight = 0;
}

Tr2WindowHandle TriDevice::GetWindow()
{
	return mHwnd;
}


#if BLUE_WITH_PYTHON
BluePyStr UPDATE("Trinity::TriDevice::Update");
BluePyStr RENDER("Trinity::TriDevice::Render");
#endif

static const float ANIMATION_TIME_MAX = 3600.0f; // One hour
#include <cwchar>
void TriDevice::OnTick( Be::Time realTime, Be::Time simTime, void* cookie )
{
	CCP_STATS_SCOPED_TIME( deviceOnTick );

	// Start with statistics on frame time
	static BeTimer s_frameTimer;
	static const int s_fpsValuesCount = 64;
	static double s_frameTimeValues[s_fpsValuesCount] = {0.0};
	static double s_frameTimeSum = 0.0;
	static int s_currentFpsValue = 0;

	double frameTime = s_frameTimer.GetSeconds();
#if CCP_STATS_ENABLED
	g_ccpStatistics_frameTime.Set( frameTime );
	if( g_ccpStatistics_frameTimeMin.GetValue() > 0 )
	{
		g_ccpStatistics_frameTimeMin.Set( std::min( frameTime, g_ccpStatistics_frameTimeMin.GetValue() ) );
	}
	else
	{
		g_ccpStatistics_frameTimeMin.Set( frameTime );
	}
	g_ccpStatistics_frameTimeMax.Set( std::max( frameTime, g_ccpStatistics_frameTimeMax.GetValue() ) );
#endif

	if( frameTime > 0.5f )
	{
		CCP_STATS_INC( frameTimeAbove500ms );
	}
	else if( frameTime > 0.4f )
	{
		CCP_STATS_INC( frameTimeAbove400ms );
	}
	else if( frameTime > 0.3f )
	{
		CCP_STATS_INC( frameTimeAbove300ms );
	}
	else if( frameTime > 0.2f )
	{
		CCP_STATS_INC( frameTimeAbove200ms );
	}
	else if( frameTime > 0.1f )
	{
		CCP_STATS_INC( frameTimeAbove100ms );
	}

	double fps = 0.0;
	if( frameTime > 0.0 )
	{
		fps = 1.0 / frameTime;
	}

	CCP_STATS_SET( fpsRaw, fps );

	if( frameTime > 0.5f )
	{
		// If frame time is very high, clear out the values to better show stalls
		// on fps graphs.
		s_frameTimeSum = frameTime * s_fpsValuesCount;
		for( int i = 0; i < s_fpsValuesCount; ++i )
		{
			s_frameTimeValues[i] = frameTime;
		}
	}

	s_frameTimeSum -= s_frameTimeValues[s_currentFpsValue];
	s_frameTimeValues[s_currentFpsValue] = frameTime;
	s_frameTimeSum += frameTime;
	++s_currentFpsValue;
	s_currentFpsValue %= s_fpsValuesCount;

	double avgFps = 0.0;
	// cppcheck-suppress variableScope
	double avgFrametime = 0.0;
	if( frameTime > 0.0 )
	{
		avgFrametime = s_frameTimeSum / (double)s_fpsValuesCount;
		avgFps = 1.0 / avgFrametime;
	}

	CCP_STATS_SET( fps, 100.0 * avgFps );
#if CCP_STATS_ENABLED
	g_ccpStatistics_smoothedFrameTime.Set( avgFrametime );
#endif

	s_frameTimer.Reset();

	++g_currentFrameCounter;

	if( BeCrashes )
	{
		wchar_t buffer[32];
        swprintf( buffer, 32, L"%lu", g_currentFrameCounter );
		BeCrashes->SetCrashKeyValueW( (wchar_t*)L"frameCounter", buffer );
	}

	Be::Time delta = simTime - m_simTime;
	if( delta < 0L )
	{
		delta = 0L;
	}
	float fDelta = (float)((double)delta / 10000000.0);
	if( fDelta > 1.0f )
	{
		fDelta = 1.0f;
	}
	m_animationTime += fDelta * m_animationTimeScale;
	
	if( m_animationTime > ANIMATION_TIME_MAX )
	{
		m_animationTime -= ANIMATION_TIME_MAX;
		GrannyRecenterAllControlClocks( -ANIMATION_TIME_MAX );
	}

	BeOS->NextScheduledEvent( mTickInterval );
	m_simTime = simTime;
	m_realTime = realTime;

	if( mCreationTime == 0 )
	{
		mCreationTime = realTime;
	}

	// Check if the artist has asked for any resources to be reloaded
	// If they have, we currently invalidate as much as possible. This may not work for Granny etc.
	ResourceSet& reloadSet = GetResourcesToReload();
	if( !reloadSet.empty() )
	{
		// First, throw out as much as possible
		for ( auto i = reloadSet.begin(); i != reloadSet.end(); i++)
		{
			(*i)->ReleaseResources(TRISTORAGE_ALL);
		}

		// Then reload stuff (so you're not invalidating dependent resources as you reload them)
		for ( auto i = reloadSet.begin(); i != reloadSet.end(); i++)
		{
			(*i)->PrepareResources();
		}

		reloadSet.clear();
	}
   
	Update( realTime, simTime );
	HandleRenderTick( realTime, simTime );
}

void TriDevice::OnSimClockRebase( Be::Time oldTime, Be::Time newTime )
{
	m_simTime += newTime - oldTime;
}

float TriDevice::GetAnimationTimeElapsed( float startTime )
{
	// This is temporary until we make an animation clock class that can be recentered globally
	// from TriDevice::Update (like granny does it)
	float elapsed = m_animationTime - startTime;
	if( elapsed < 0.0f )
	{
		elapsed += ANIMATION_TIME_MAX;
	}
	return elapsed;
}

//Device has been lost.  Send an event here to free up memory.  We repeat these messages when the
//device comes back up, to release anything that gets created after this.
void TriDevice::DeviceLost()
{
	ReleaseDeviceResources( TRISTORAGE_VIDEOMEMORY );
}

// Convenience function.
bool TriDevice::ResetDevice( const Tr2PresentParametersAL* pp )
{
	return ResetDevice( mAdapter, pp );
}

// Reset the device.  Much required.  Note, the device should be NOTRESET here.
bool TriDevice::ResetDevice( unsigned adapter, const Tr2PresentParametersAL* pp )
{
	CCP_LOGNOTICE( "Starting Device Reset Resource Release" );

	// Prevent a second device reset from being triggered by python code within this context
	DeviceResetContextManager resetContext;

	// Setting this flag should disable all creation of device resources that would
	// prevent a reset
	Tr2Renderer::SetResourceCreationAllowed( false );

	ReleaseDeviceResources( TRISTORAGE_VIDEOMEMORY );

	if( !DeviceExists() )
	{
		CCP_LOGERR( "Device Reset: No device available" );
		return false;
	}

	if (!pp)
	{
		pp = &mPresentParam;
	}

	if( !DoLowLevelDeviceReset( *pp ) )
	{
		return false;
	}
		
	// success! 
	mWidth = pp->mode.width;
	mHeight = pp->mode.height;
	mPresentParam = *pp;
	
	// We may have changed display modes
	if( mPresentParam.IsAdapterRequired()
		&& FAILED( Tr2VideoAdapterInfo::GetAdapterDisplayMode( mAdapter, mDisplayMode ) ) )
	{
		CCP_LOGNOTICE( "Device Reset: Adapter Display Mode Changed" );
		Tr2Renderer::SetResourceCreationAllowed( true );
		return false;	
	}

	if( !InitD3DDevice() )
	{
		CCP_LOGERR( "Device Reset: Failed to initialize device" );
		return false;
	}

	mAdapter = adapter;

	// This must be unset to allow us to prepare resources
	Tr2Renderer::SetResourceCreationAllowed( true );

	CCP_LOGNOTICE( "Device Reset: Re-Preparing Resources" );
	PrepareDeviceResources();

	CCP_LOGNOTICE( "Device Reset: Completed Successfully" );
	return true;
}

/////////////////////////////////////////////////////////////////////////////////////////
// INotify Impl
/////////////////////////////////////////////////////////////////////////////////////////

bool TriDevice::OnModified( Be::Var* value )
{
	if( IsMatch( value, m_mipLevelMaxChainLength ) )
	{
		SetMipLevelMaxCount( m_mipLevelMaxChainLength );
	}
	else if( IsMatch( value, m_mipLevelSkipCount ) )
	{
		SetMipLevelSkipCount( m_mipLevelSkipCount );
	}

	return true;
}

#if BLUE_WITH_PYTHON
void TriDevice::DisableResourceLoad( bool flag )
{
	Tr2Renderer::DisableResourceLoad( flag );
}

PyObject* TriDevice::PythonCreateDeviceHelper( PyObject* args, DeviceScreenType screenType )
{
	TriPythonContext pythonCtx;

	Tr2WindowHandle hwnd = 0;
	int width = 0;
	int height = 0;
	int presentInterval = Tr2RenderContextEnum::PRESENT_INTERVAL_IMMEDIATE;

	if( screenType != NO_ADAPTER && !PyArg_ParseTuple( args, "i|iii", &hwnd, &width, &height, &presentInterval ) )
	{
		return nullptr;
	}

	width  = std::max( 0, width );
	height = std::max( 0, height );

	bool OK = CreateSimpleDevice( hwnd, width, height, screenType, Tr2RenderContextEnum::PresentInterval( presentInterval ) );
		
	if( !OK )
	{
		return nullptr;
	}

	Py_RETURN_NONE;
}

PyObject* TriDevice::PyCreateWindowedDevice( PyObject* args )
{
	return PythonCreateDeviceHelper( args, WINDOWED );	
}

PyObject* TriDevice::PyCreateFullScreenDevice( PyObject* args )
{
	return PythonCreateDeviceHelper( args, FULLSCREEN );	
}

PyObject* TriDevice::PyCreateWindowlessDevice( PyObject* args )
{
	return PythonCreateDeviceHelper( args, NO_ADAPTER );	
}

long TriDevice::PyGetWindow()
{
	return (long)GetWindow();
}

PyObject *TriDevice::PyGetPresentParameters( PyObject *args)
{
	TriPythonContext pythonCtx;

	if( !PyArg_ParseTuple( args, "" ) )
	{
		return nullptr;
	}

	return mPresentParam.Get().Detach();
}

PyObject *TriDevice::PyRegisterResource( PyObject *args )
{
	PyObject *obj;
	if (!PyArg_ParseTuple(args, "O:RegisterResource", &obj))
	{
		return nullptr;
	}

	if( !m_pyResourceSet )
	{
		//Initialize a weakref.WeakKeyDictionary object
		BluePy module( PyImport_Import( BluePyStr( "weakref" ) ) );
		if( !module )
		{
			return nullptr;
		}
		m_pyResourceSet = BluePy( PyObject_CallMethod( module, const_cast<char*>("WeakKeyDictionary"), 0 ) );
		if( !m_pyResourceSet )
		{ 
			return nullptr;
		}
	}

	if( !PyObject_HasAttrString( obj, "OnCreate" ) )
	{
		PyErr_SetString( PyExc_TypeError, "Object has no OnCreate method" );
		return nullptr;
	}

	//Add the object as a key in the dictionary
	return PyObject_CallMethod( m_pyResourceSet, const_cast<char*>("__setitem__"), const_cast<char*>("OO"), obj, Py_None );
}

PyObject* TriDevice::PyResetDeviceResources( PyObject* args )
{
	if( Tr2Renderer::IsDeviceResetting() )
	{
		TriError::ReportError(E_INVALIDCALL, NULL, "Device reset is already in progress" );
		return nullptr;
	}
	
	ResetDevice( NULL );
	Py_RETURN_NONE;
}
#endif

void TriDevice::PrepareDeviceResources()
{
	USE_MAIN_THREAD_RENDER_CONTEXT();

	// NB: Unchecked return.
	// Also, why are we calling this?
	renderContext.SetPresentParameters( mAdapter, mPresentParam );

	Tr2AutoResetObjectAL::PrepareALResources( renderContext );

	// Rebuild C++ device resources
	const ResourceSet& rs = GetResourcesRegistered();
	for( auto i = rs.cbegin(); i!= rs.cend(); ++i )
	{
		Tr2DeviceResource* resource = *i;
		if( resource )
		{
			resource->PrepareResources();
		}
		else
		{
			CCP_LOGWARN( "NULL ITriDeviceResource found in resource set during TriDevice::PrepareDeviceResources!" );
		}
	}

	Tr2Renderer::PrepareDeviceResources();
	
	// Rebuild Python device resources
	RebuildDeviceResourcesInPython();
}

#if BLUE_WITH_PYTHON
PyObject* TriDevice::PyChangeBackBufferSize( PyObject* args )
{
	TriPythonContext pythonCtx;

	if( Tr2Renderer::IsDeviceResetting() )
	{
		TriError::ReportError(E_INVALIDCALL, NULL, "Device reset is already in progress" );
		return nullptr;
	}

	int width, height;

	if( !PyArg_ParseTuple( args, "ii", &width, &height ) )
	{
		return nullptr;
	}

	width  = std::max( 0, width );
	height = std::max( 0, height );

	PresentationParameters pp = mPresentParam;
	pp.mode.height = height;
	pp.mode.width = width;
	if( !ResetDevice( &pp ) )
	{
		return nullptr;
	}

	Py_RETURN_NONE;
}
#endif

void TriDevice::PyInvalidateAndUnregisterForTicks()
{
	TriPythonContext pythonCtx;
	InvalidateAndUnregisterForTicks();
}

TriDevice::ResourceSet& TriDevice::GetResourcesRegistered()
{
	static NeverEndingSingleton<TriDevice::ResourceSet> registered;
	return registered.GetInstance();
}

TriDevice::UpdateList& TriDevice::GetUpdateList()
{
	static NeverEndingSingleton<TriDevice::UpdateList> updateList;
	return updateList.GetInstance();
}

TriDevice::ResourceSet& TriDevice::GetResourcesToReload()
{	
	static NeverEndingSingleton<TriDevice::ResourceSet> reloadSet;
	return reloadSet.GetInstance();
}

#if TRINITYDEV
void TriDevice::DumpResources()
{
	const ResourceSet& resources = GetResourcesRegistered();
	for( ResourceSet::const_iterator it = resources.begin(); it != resources.end(); ++it)
	{
		std::string desc;
		(*it)->GetDescription( desc );
		desc += "\n";
		OutputDebugString( desc.c_str() );
		fprintf( stdout, desc.c_str() );
	}
}

#if BLUE_WITH_PYTHON
void TriDevice::PyDumpResources()
{
	DumpResources();
}
#endif
#endif

#if BLUE_WITH_PYTHON
void TriDevice::PyRender()
{	
	TriPythonContext pythonCtx;
	Render();	
}
#endif

void TriDevice::SetMipLevelMaxCount( int n )
{
	m_mipLevelMaxChainLength = (unsigned int)std::max( n, 3 ); 
}

// We need to guard access to the resource set. As an example, the EveShip2Builder
// copies sprite sets on a background thread - these sprite sets are registered
// here, and the builder might happen to do that just as something is being
// created on the main thread.
CcpMutex& GetResourcesRegisteredMutex()
{
	static CcpMutex s_resourcesRegisteredMutex( "TriDevice", "s_resourcesRegisteredMutex" );
	return s_resourcesRegisteredMutex;
}

// We manage a set of weak references to objects that have registered themselves
// to be notified for an Invalidate call.  To release device resources when required.
void TriDevice::RegisterResource( Tr2DeviceResource* resource )
{
	CcpAutoMutex guard( GetResourcesRegisteredMutex() );
	GetResourcesRegistered().insert( resource );
}

void TriDevice::UnregisterResource( Tr2DeviceResource* resource )
{
	CcpAutoMutex guard( GetResourcesRegisteredMutex() );
	auto& reg = GetResourcesRegistered();

	if( !s_iteratingForRelease )
	{
		reg.erase( resource );
	}
	else
	{
		s_resourcesToBeRemoved.insert( resource) ;
	}
	
	GetResourcesToReload().erase( resource );
}

void TriDevice::RegisterForUpdates( ITr2Updateable* item )
{
	UpdateList &updateArray = GetUpdateList();
	
	if( std::find( updateArray.begin(), updateArray.end(), item ) == updateArray.end() )
	{
		updateArray.insert( updateArray.end(), item );
	}
}

void TriDevice::UnregisterForUpdates( ITr2Updateable* item )
{
	UpdateList &updateArray = GetUpdateList();
	auto findResult = std::find( updateArray.begin(), updateArray.end(), item );
	if( findResult != updateArray.end() )
	{
		// We have the item that we were looking for registered
		ITr2Updateable* backItem = updateArray.back();
		if ( *findResult != backItem )
		{
			*findResult = backItem;
		}

		updateArray.pop_back();
	}
}

void TriDevice::QueueForReload( Tr2DeviceResource* resource )
{
	GetResourcesToReload().insert(resource);
}

Tr2RenderContext* TriDevice::GetRenderContext()
{
	USE_MAIN_THREAD_RENDER_CONTEXT();
	return &static_cast<Tr2RenderContext&>( renderContext );
}

unsigned TriDevice::GetRenderingPlatformID()
{
	return TRINITY_PLATFORM;
}

void TriDevice::SetRenderJobs( Tr2RenderJobs* renderJobs )
{
	m_renderJobs = renderJobs;
}

unsigned TriDevice::CreateScreenVertexDecl()
{
	Tr2VertexDefinition vd;
	vd.Add( vd.FLOAT32_4, vd.POSITION );
	vd.Add( vd.FLOAT32_2, vd.TEXCOORD );
	
	return Tr2EffectStateManager::GetVertexDeclarationHandle( vd );
}

void TriDevice::GetPickRayFromViewport( int x, int y, TriViewport* viewport, const Matrix& view, const Matrix& proj, Vector3& rayWorld, Vector3& startWorld )
{
	float xp, yp;
	ScreenToProjection( x, y, &xp, &yp, viewport );

	ConvertProjectionCoordToWorldPickRay( xp, yp, &proj, &view, &startWorld, &rayWorld ); 
}

void TriDevice::ScreenToProjection(		int x,		int y,
										float* fx,	float* fy )
{
	ScreenToProjection( x, y, fx, fy, &mViewport );
}

void TriDevice::ScreenToProjection(		int x,		int y,
										float* fx,	float* fy,
										const TriViewport* viewport )
{
	x = x - viewport->x;
	y = y - viewport->y;
	const int w = viewport->width;
	const int h = viewport->height;

	//DX maps viewport pixel _centres_ to the view space.  So, for four pixels, pixel no 3 maps to 1 and no 0 to -1
	//(some other implementations map pixel _edges_ to the screen, this would give different results.
	*fx =   (float)(2*x)/(w-1) - 1.0f;
	*fy =   (float)(2*y)/(h-1) - 1.0f;
	*fy = -*fy;
}

void TriDevice::CallCallbacks( const TrackableStdVector<CallbackData>& cbs )
{
	auto cbCopy = cbs;
	for( auto it = std::begin( cbCopy ); it != std::end( cbCopy ); ++it )
	{
		( *it->m_callback )( this, it->m_userData );
	}
}

bool TriDevice::Render()
{
	D3DPERF_EVENT(L"TriDevice::Render");

	CallCallbacks( m_beginFrameCallbacks );

	USE_MAIN_THREAD_RENDER_CONTEXT();
	Tr2Viewport vp;
	mViewport.ConvertToTr2Viewport( vp );
	renderContext.SetViewport( vp );
	
	// start rendering
	Tr2Renderer::BeginFrame();

	HRESULT hr = Tr2Renderer::BeginRenderContext();
	if( FAILED( hr ) )
	{
#if TRINITY_PLATFORM == TRINITY_DIRECTX9
		if( hr == D3DERR_OUTOFVIDEOMEMORY )
		{
			ReleaseDeviceResources( TRISTORAGE_VIDEOMEMORY );
			hr = Tr2Renderer::BeginRenderContext();
		}
		CHECKDXFAIL("BeginScene Failed");
#endif
	}

	if( m_renderJobs )
	{		
		m_renderJobs->Run( m_realTime, m_simTime );
	}

	CallCallbacks( m_presentCallbacks );

	Tr2Renderer::EndRenderContext();
	Tr2Renderer::EndFrame();

	return true;
}

bool TriDevice::GetWidth( uint32_t& width ) const
{
	unsigned w, h;
	Tr2Renderer::GetBackBufferDimensions( w, h );
	width = w;
	return true;
}

bool TriDevice::GetHeight( uint32_t& height ) const
{
	unsigned w, h;
	Tr2Renderer::GetBackBufferDimensions( w, h );
	height = h;
	return true;
}

void TriDevice::RegisterDeviceCallback( Tr2DeviceCallbackTime time, Tr2DeviceCallback callback, void* userData )
{
	auto& cbs = time == DEVICE_CALLBACK_FRAME_BEGIN ? m_beginFrameCallbacks : m_presentCallbacks;
	CallbackData cb = { callback, userData };
	cbs.push_back( cb );
}

void TriDevice::UnregisterDeviceCallback( Tr2DeviceCallbackTime time, Tr2DeviceCallback callback, void* userData )
{
	auto& cbs = time == DEVICE_CALLBACK_FRAME_BEGIN ? m_beginFrameCallbacks : m_presentCallbacks;
	for( auto it = std::begin( cbs ); it != std::end( cbs ); ++it )
	{
		if( it->m_callback == callback && it->m_userData == userData )
		{
			cbs.erase( it );
			break;
		}
	}
}

void TriDevice::GetBackBufferGrabber( ITr2RenderTargetGrabber** grabber )
{
	Tr2RenderTargetGrabberPtr backBufferGrabber;
	backBufferGrabber.CreateInstance();
	if( backBufferGrabber )
	{
		USE_MAIN_THREAD_RENDER_CONTEXT();
		if( !backBufferGrabber->Create( renderContext.GetDefaultBackBuffer(), this ) )
		{
			backBufferGrabber = nullptr;
		}
	}

	( *grabber ) = backBufferGrabber.Detach();
}

void TriDevice::AddPostUpdateCallback( IBlueCallbackMan::CallbackFunc cb, void* context )
{
	m_postUpdateCallbacks->Add( cb, context, IBlueCallbackMan::BCBF_NONE, nullptr );
}

void TriDevice::SetTickInterval( int value )
{
	mTickInterval = value;
}

//  Description:
//    This class is an RAII implementation of management for an 'IsDeviceResetting' state
//    It is intended to prevent python from calling functions that would trigger a device
//    reset, while we're inside a device reset context (through python callbacks)
TriDevice::DeviceResetContextManager::DeviceResetContextManager()
{
	m_originalSetting = Tr2Renderer::IsDeviceResetting();
	// If the device was originally resetting, we may have gotten in here because
	// we needed to have this scope in two places. This isn't necessarily an error
	// in C++ (if, for example, we need to put it in SetShaderModel, which calls
	// ResetDevice())
	if( !m_originalSetting )
	{
		Tr2Renderer::SetIsDeviceResetting( true );
	}
}

TriDevice::DeviceResetContextManager::~DeviceResetContextManager()
{
	// We don't unset the flag, unless we were the context to set it 
	if( !m_originalSetting )
	{
		Tr2Renderer::SetIsDeviceResetting( false );
	}
}