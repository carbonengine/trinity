#include "StdAfx.h"
#include "App.h"
#include "../TriRect.h"
#include "../TriError.h"
#include "../TriPythonContext.h"
#include "../TriDevice.h"
#include "Tr2MouseCursor.h"
#include "Tr2PresentParameters.h"
#include "WindowIcon.h"

static CcpLogChannel_t s_appChannel = CCP_LOG_DEFINE_CHANNEL( "App" );

App* g_app;


static char TICK_UIAPP[] =  "UI App";

#ifdef _WIN32
extern HINSTANCE gInstance;
ATOM App::mWndClassAtom = NULL;
#elif !defined(__ORBIS__) && !defined(__ANDROID__)
#include "GLFW/glfw3.h"
#endif
static const uint32_t DEFAULT_WINDOW_POSITION = 0x80000000; // CW_DEFAULT on windows

CcpMeanStatisticsEntry s_activeFrametimeMean;
CcpStdDevStatisticsEntry s_activeFrametimeStdDev;

class SetupActiveFrameTimeStats
{
public:

	SetupActiveFrameTimeStats()
	{
		s_activeFrametimeMean.SetName( "Trinity/FrameTime/ActiveMean" );
		CcpStatistics::RegisterDerived( &s_activeFrametimeMean );

		s_activeFrametimeStdDev.SetName( "Trinity/FrameTime/ActiveStdDev" );
		CcpStatistics::RegisterDerived( &s_activeFrametimeStdDev );
	}
};

static SetupActiveFrameTimeStats s_setupActiveFrameTimeStats;

#ifdef _WIN32
// --------------------------------------------------------------------------------
// Description:
//   A callback function that handles event notifications from TransGaming
// Arguments:
//   type - type of notification
//   state - state describing what exactly happened
//   data - additional data
//   context - app defined context
// --------------------------------------------------------------------------------
static void TGNotificationCallback( TG_NOTIFY_TYPE type, DWORD state, int data, void *context )
{
	if( type != TGNOTIFY_ACTIVATION )
	{
		return;
	}

	switch( state )
	{
	case TGAS_MINIMIZE:
		g_app->mIsHidden = TRUE;
		if( gTriDev )
		{
			gTriDev->SetTickInterval( 10 );
		}
		break;
	case TGAS_GAINFOCUS:
		g_app->mIsHidden = FALSE;
		if( gTriDev )
		{
			gTriDev->SetTickInterval( 0 );
		}
		break;
	case TGAS_NONE:
		break;
	case TGAS_LOSEFOCUS:
		break;
	case TGAS_TOGGLE_WINDOWED:
		if( g_app->mTGToggleEventListener )
		{
			g_app->mSendToggleEvent = true;
		}
		break;
	case TGAS_TOGGLE_FULLSCREEN:
		if( g_app->mTGToggleEventListener )
		{
			g_app->mSendToggleEvent = true;
		}
		break;
	default:
		break;
	}
}
#endif

App::App()
	:mWindowTitle( L"CCP 3D Application" ),
	mHwnd( 0 ),
	mCreationLeft( DEFAULT_WINDOW_POSITION ),
	mCreationTop( DEFAULT_WINDOW_POSITION ),
	mCreationWidth( 0 ),
    mCreationHeight( 0 ),
	mRefreshRate( Tr2RenderContextEnum::PRESENT_INTERVAL_ONE ),
	mWindowed( true ),
	mFullscreen( false ),
	mHideTitle( false ),
	mActive( false ),
    mReady( false ),
	mSendResizeEvent( false ),
	mIsResizing( false ),
	mMinimumHeight( 100 ),
	mMinimumWidth( 100 ),
	mIsTransgaming( IsTransgaming() ),
    mFixedWindow( false ),
	mIsMaximized( false ),
	mIsHidden( false ),
	mWindowStyle( 0xffffffff ),
	mWindowClient(),
	mSendToggleEvent( false )
#if BLUE_WITH_PYTHON
	, m_eventHandler( NULL ),
	m_sessionChangeHandler( NULL )
#endif
{

#ifdef _WIN32
	if (!mWndClassAtom)
	{
		HICON hIcon = GetWindowIcon();

		//We need to use the wide char version here, so that we get Unicode key input.
		WNDCLASSEXW wclass = {sizeof(wclass)};
		wclass.lpfnWndProc = _WndProc;
		wclass.hInstance = gInstance;
		wclass.lpszClassName = L"triuiScreen";
		wclass.hIcon = hIcon;
		// Register the windows class
		mWndClassAtom = RegisterClassExW(&wclass);
	}
#elif defined( __ORBIS__ )
#elif defined( __ANDROID__ )
#else
    glfwInit();
#endif
    
	g_app = this;
}

App::~App()
{
	g_app = NULL;
}



//--------------------------------------------------------------------
// Create
//--------------------------------------------------------------------
bool App::Create()
{
	if (!Destroy())
		return false;

	// create window and set up, if not already set up.
	if (!AdjustWindowForChange(mWindowed, mFixedWindow))
		return 0;

    // Initialize the 3D environment for the app
    if(!Initialize3DEnvironment())
	{
		BeOS->SetError(BEDEF, Clsid(), "Initialize3DEnvironment() failed.");
#ifdef _WIN32
		DestroyWindow(mHwnd);
#elif defined( __ORBIS__ )
#elif defined( __ANDROID__ )
#else
        glfwDestroyWindow( reinterpret_cast<GLFWwindow*>( mHwnd ) );
#endif
		mHwnd = NULL;
		return false;
	}

	CreateImpl();

#ifdef _WIN32
    // Register for event notifications from cider
	if( mIsTransgaming )
	{
		TGRegisterForNotifications( (TGNotifyCallback_Func)TGNotificationCallback, (void*)this );
	}
#endif
    
	return true;
}

void App::CreateImpl()
{
	// The app is good to go
    mReady = true;


	// The old ticking interface used a strong reference and we mimick this here
	// since the lifetime management model for this class requires it (it's a weakness exposed by the
	// Python WoD code).
	this->Lock();
	BeOS->RegisterForTicks(this, TICK_UIAPP);
}


//--------------------------------------------------------------------
// Destroy
//--------------------------------------------------------------------
bool App::Destroy()
{
	if (!mReady)
		return true;  //don't destroy twice, you twit!

	CCP_LOG_CH( s_appChannel, "Trinity::App Destroy()" );

	mReady = false;

	BeOS->UnregisterForTicks(this, TICK_UIAPP);
	// The old ticking interface used a strong reference and we mimic this here
	// since the lifetime management model for this class requires it (it's a weakness exposed by the
	// Python WoD code).
	this->Unlock();

	if( gTriDev )
	{
		gTriDev->SetPresentation(0, NULL); //invalidates all stuff
	}


	if (mHwnd)
	{
		//if we weren't invoked by the WM_DESTROY handler...  destroy the window.
#ifdef _WIN32
		DestroyWindow(mHwnd);
#elif defined( __ORBIS__ )
#elif defined( __ANDROID__ )
#else
        glfwDestroyWindow( reinterpret_cast<GLFWwindow*>( mHwnd ) );
#endif
		mHwnd = NULL;
	}

	return true;
}


//--------------------------------------------------------------------
// Initialize3DEnvironment
//--------------------------------------------------------------------
bool App::Initialize3DEnvironment()
{
	Tr2DisplayModeInfo mode;
	CR_RETURN_VAL( Tr2VideoAdapterInfo::GetAdapterDisplayMode( Tr2VideoAdapterInfo::DEFAULT_ADAPTER, mode ), false );

	Tr2PresentParametersAL present = {0};
	present.mode = mode;
	present.backBufferCount = 1;

	present.msaaType = 0;		
	present.msaaQuality = 0;		

	present.swapEffect = Tr2RenderContextEnum::SWAP_EFFECT_DISCARD;
	present.outputWindow = mHwnd;
	present.windowed = mWindowed;
	present.software = false;
	
	present.depthStencilFormat = Tr2RenderContextEnum::DSFMT_D16;

	present.presentInterval =
		mWindowed ? Tr2RenderContextEnum::PRESENT_INTERVAL_ONE  : Tr2RenderContextEnum::PRESENT_INTERVAL_IMMEDIATE;


	if (!gTriDev->SetPresentation(Tr2VideoAdapterInfo::DEFAULT_ADAPTER, &present))
	{
		CCP_LOGERR_CH( s_appChannel, "App::Initialize3DEnvironment failed" );

		return false;
	}

    return true;
}


Tr2WindowHandle App::GetWindowHandle(
	)
{
	return mHwnd;
}


//////////////////////////////////////////////////////////////////////
//
// IBlueEvents interface method
//
//////////////////////////////////////////////////////////////////////

//--------------------------------------------------------------------
// IBlueEvents::OnTick
//--------------------------------------------------------------------
void App::OnTick(
	Be::Time realTime,		// Universal time in seconds
	Be::Time simTime,		// Simulation time
	void* cookie			// user supplied cookie value
	)
{
	CCP_STATS_ZONE( __FUNCTION__ );

	if( mSendResizeEvent && !gTriDev->IsDeviceLost() )
	{
#ifdef _WIN32
		mIsMaximized = ::IsZoomed( mHwnd ) != 0;
#else
        mIsMaximized = false;
#endif
		if( mResizeEventListener )
		{
			mResizeEventListener->HandleEvent( NULL );
		}
		mSendResizeEvent = false;
	}
	if( mSendToggleEvent && !gTriDev->IsDeviceLost() )
	{
		if( mTGToggleEventListener )
		{
			mTGToggleEventListener->HandleEvent( NULL );
		}
		mSendToggleEvent = false;
	}

	CCP_STATS_DECLARED_ELSEWHERE( frameTime );

	if( mActive )
	{
		s_activeFrametimeMean.SetSource( &g_ccpStatistics_frameTime );
		s_activeFrametimeStdDev.SetSource( &g_ccpStatistics_frameTime );
	}
	else
	{
		s_activeFrametimeMean.SetSource( nullptr );
		s_activeFrametimeStdDev.SetSource( nullptr );
	}
}



#ifdef _WIN32
LRESULT App::_WndProc(HWND hwnd, UINT msg, WPARAM w, LPARAM l)
{
	CCP_STATS_ZONE( __FUNCTION__ );

	if (g_app)
	{
		return g_app->WndProc(hwnd, msg, w, l);
	}
	else
	{
		return DefWindowProcW(hwnd, msg, w, l);
	}
}
#endif

#if BLUE_WITH_PYTHON
//////////////////////////////////////////////////////////////////////
//
// Python thunkers
//
//////////////////////////////////////////////////////////////////////


//--------------------------------------------------------------------
// Destroy
//--------------------------------------------------------------------
PyObject* App::PyDestroy(PyObject* args)
{
	if (!PyArg_ParseTuple(args, ""))
		return NULL;

	if (!Destroy())
		return NULL;

	Py_INCREF(Py_None);
	return Py_None;
}
#endif


//--------------------------------------------------------------------
// Quit
//--------------------------------------------------------------------
void App::Quit()
{
	Destroy();
#ifdef _WIN32
	PostQuitMessage(0);
#else
    exit( 0 );
#endif
}


//--------------------------------------------------------------------
// MoveWindow
//--------------------------------------------------------------------
void App::MoveWindow( int x, int y, Be::Optional<int> width, Be::Optional<int> height )
{
#if !defined( __ORBIS__ ) && !defined( __ANDROID__ )
	int w, h;
#ifdef _WIN32
	RECT rc;
	::GetWindowRect(mHwnd, &rc);
	w = rc.right - rc.left;
	h = rc.bottom - rc.top;
#else
    glfwGetWindowSize( reinterpret_cast<GLFWwindow*>( mHwnd ), &w, &h );
#endif
	if( width.IsAssigned() )
	{
		w = width;
	}
	if( height.IsAssigned() )
	{
		h = height;
	}

#ifdef _WIN32
	::MoveWindow(mHwnd, x, y, w, h, TRUE);
#else
    glfwSetWindowPos( reinterpret_cast<GLFWwindow*>( mHwnd ), x, y );
    glfwSetWindowSize( reinterpret_cast<GLFWwindow*>( mHwnd ), w, h );
#endif
#endif
}


//--------------------------------------------------------------------
// Minimize
//--------------------------------------------------------------------
void App::Minimize( Be::OptionalWithDefaultValue<bool, true> minimize )
{
#ifdef _WIN32
	::ShowWindow(mHwnd, minimize ? SW_MINIMIZE : SW_RESTORE);
#elif defined( __ORBIS__ )
#elif defined( __ANDROID__ )
#else
	if( minimize )
	{
		glfwIconifyWindow( reinterpret_cast<GLFWwindow*>( mHwnd ) );
	}
	else
	{
		glfwRestoreWindow( reinterpret_cast<GLFWwindow*>( mHwnd ) );
	}
#endif
}

void App::Maximize( Be::OptionalWithDefaultValue<bool, true> maximize )
{
#ifdef _WIN32
	::ShowWindow(mHwnd, maximize ? SW_MAXIMIZE : SW_RESTORE);
#elif defined( __ORBIS__ )
#elif defined( __ANDROID__ )
#else
	if( maximize )
	{
		// needs upgrading to GLFW 3.2
		// glfwMaximizeWindow( reinterpret_cast<GLFWwindow*>( mHwnd ) );
	}
	else
	{
		glfwRestoreWindow( reinterpret_cast<GLFWwindow*>( mHwnd ) );
	}
#endif
}

#if BLUE_WITH_PYTHON

//--------------------------------------------------------------------
// GetClientRect  (returns the client rectangle relative to the window rectangle)
//--------------------------------------------------------------------
TriRect* App::GetClientRect( int )
{
#ifdef _WIN32
	RECT cli;
	::GetClientRect(mHwnd, &cli);

	POINT pt = {0, 0};
	ClientToScreen(mHwnd, &pt);

	RECT app;
	::GetWindowRect(mHwnd, &app);
	TriRectPtr r;
	r.CreateInstance();
	int cliLeft = pt.x - app.left;
	int cliTop = pt.y - app.top;
	r->SetDimentions(cliLeft, cliTop, cliLeft + cli.right, cliTop + cli.bottom);
	return r.Detach();
#else
    int width = 0, height = 0;
    glfwGetWindowSize( reinterpret_cast<GLFWwindow*>( mHwnd ), &width, &height );
	TriRectPtr r;
	r.CreateInstance();
	r->SetDimentions( 0, 0, width, height );
	return r.Detach();
#endif
}


//--------------------------------------------------------------------
// GetWindowRect
//--------------------------------------------------------------------
TriRect* App::GetWindowRect()
{
#ifdef _WIN32
	RECT app;
	::GetWindowRect(mHwnd, &app);

	TriRectPtr r;
	r.CreateInstance();
	r->SetRect( reinterpret_cast<Rect*>( &app ) );
	return r.Detach();
#else
    int x = 0, y = 0;
    glfwGetWindowPos( reinterpret_cast<GLFWwindow*>( mHwnd ), &x, &y );
    int width = 0, height = 0;
    glfwGetWindowSize( reinterpret_cast<GLFWwindow*>( mHwnd ), &width, &height );

	TriRectPtr r;
	r.CreateInstance();
	r->SetDimentions( x, y, x + width, y + height );
	return r.Detach();
#endif
}


//--------------------------------------------------------------------
// GetHwnd
//--------------------------------------------------------------------
PyObject* App::PyGetHwnd(PyObject* args)
{
	if (!PyArg_ParseTuple(args, ""))
		return NULL;
	return WrappedHWND::Wrap(mHwnd);
}

PyObject* App::PyGetHwndAsLong(PyObject* args)
{
	if (!PyArg_ParseTuple(args, ""))
		return NULL;

	return PyInt_FromSsize_t( (Py_ssize_t) mHwnd );
}
// --- the new api


//--------------------------------------------------------------------
// Call this before changing between windowed or not windowed
//--------------------------------------------------------------------
PyObject *App::PyAdjustWindowForChange(PyObject *args)
{
	bool windowed = mWindowed;
    bool fixedWindow = mFixedWindow;

	PyObject *o = 0;
	PyObject *oFixed = 0;

	if (!PyArg_ParseTuple(args, "|OO", &o, &oFixed))
		return NULL;
	if (o)
		windowed = PyObject_IsTrue(o) ? true : false;
    if (oFixed)
        fixedWindow = PyObject_IsTrue(oFixed) ? true : false;

	if (!AdjustWindowForChange(windowed, fixedWindow)){
		PyErr_SetString(PyExc_RuntimeError, "failure");
		return 0;
	}

	Py_INCREF(Py_None);
	return Py_None;
}

//--------------------------------------------------------------------
// Then call this with the new presentation parameters:
//--------------------------------------------------------------------
PyObject *App::PyChangeDevice(PyObject *args)
{
	TriPythonContext pythonCtx; // Errors reported here and deeper in the callstack will be Python errors

	PyObject *adapterO;
	uint32_t behaviorFlags;
	int32_t dType;
	PyObject *presentation;
	if (!PyArg_ParseTuple(args, "OiiO", &adapterO, &behaviorFlags, &dType, &presentation))
		return NULL;
	uint32_t adapter = Tr2VideoAdapterInfo::DEFAULT_ADAPTER;
	if (adapterO != Py_None){
		if (!PyInt_Check(adapterO)) {
			PyErr_SetString(PyExc_TypeError, "invalid 'adapter'");
			return 0;
		}
		adapter = uint32_t( PyInt_AsLong(adapterO) );
	}

	PresentationParameters pp;
	if (!pp.Set(presentation))
		return 0;
	if (pp.windowed != mWindowed) {
		PyErr_SetString(PyExc_RuntimeError, "'Windowed' flag doesn't match triapp setting");
		return 0;
	}

	bool result = ChangeDevice(adapter, &pp);
	if (!result)
		return 0;
	//percolate device changes back into the dict given
	BluePyDict dict(BluePy(presentation, true));
	dict.Update(pp.Get());

	Py_INCREF(Py_None);
	return Py_None;
}


//--------------------------------------------------------------------
// Call this after changing the device.  It moves the window (if applicable)
// and sends UILib a resize request.
//--------------------------------------------------------------------
PyObject *App::PySetWindowPos(PyObject *args)
{
	int left = mCreationLeft, top=mCreationTop;
	if (!PyArg_ParseTuple(args, "|ii", &left, &top))
		return 0;

	if (!SetWindowPos(left, top)) {
		PyErr_SetString(PyExc_RuntimeError, "failure");
		return 0;
	}

	Py_INCREF(Py_None);
	return Py_None;
}
#endif

// C implementations
bool App::AdjustWindowForChange(bool windowed, bool fixedWindow)
{
	mWindowed = windowed;
	mFullscreen = !mWindowed;
    mFixedWindow = fixedWindow;

#ifdef _WIN32
	DWORD windowStyle = WS_POPUP | WS_SYSMENU | WS_VISIBLE | WS_MINIMIZEBOX;
	if( mWindowed && !mFixedWindow )
    {
        if (!mHideTitle)
        {
			windowStyle = windowStyle | WS_CAPTION;
            if( !mIsTransgaming )
            {
                windowStyle |= WS_THICKFRAME | WS_MAXIMIZEBOX;
            }
        }
	}

	if( !mWindowed || mFixedWindow )
	{
		mIsMaximized = false;
	}

	if( mWindowStyle != windowStyle )
	{
		mWindowStyle = windowStyle;

		if (!mHwnd && !CreateWin())
			return false;

		mActive = mHwnd == ::GetForegroundWindow();

		SetWindowLong( mHwnd, GWL_STYLE, mWindowStyle );
	}
	else
	{
		if (!mHwnd && !CreateWin())
			return false;

		mActive = mHwnd == ::GetForegroundWindow();
	}
#elif defined( __ORBIS__ )
#elif defined( __ANDROID__ )
    if( !mHwnd && !CreateWin() )
    {
        return false;
    }
#else
    if( !mHwnd && !CreateWin() )
    {
        return false;
    }
    mActive = glfwGetWindowAttrib( reinterpret_cast<GLFWwindow*>( mHwnd ), GLFW_FOCUSED ) != 0;
#endif
	return true;
}

bool App::CreateWin()
{
	if (mHwnd)
		return true;
#ifdef _WIN32
	RECT rc;
	if (mFullscreen)
	{
		rc.left = 0;
		rc.top = 0;
		rc.right = mCreationWidth;
		rc.bottom = mCreationHeight;
	}
	else
	{
		if (mCreationLeft == CW_USEDEFAULT)
			mCreationLeft = 0;
		if (mCreationTop == CW_USEDEFAULT)
			mCreationTop = 0;

		SetRect(&rc, 0, 0, mCreationWidth, mCreationHeight); //clientsize
		AdjustWindowRect(&rc, mWindowStyle, FALSE);
		OffsetRect(&rc, mCreationLeft-rc.left, mCreationTop-rc.top); // move to mCreationLeft
	}
	mHwnd = ::CreateWindowW(
		(LPCWSTR)mWndClassAtom,
		mWindowTitle.c_str(),
		mWindowStyle,
		rc.left, rc.top,
		(rc.right-rc.left), (rc.bottom-rc.top),
		0L,
		NULL,
		gInstance,
		0L);

	if (!mHwnd) {
		BeOS->SetError(BE32, Clsid(), "Couldn't create window?!?");
		return false;
	}

    // Save window properties
    mWindowStyle = GetWindowLong(mHwnd, GWL_STYLE);
    ::GetClientRect(mHwnd, reinterpret_cast<RECT*>( &mWindowClient ) );
#elif defined( __ORBIS__ )
#elif defined( __ANDROID__ )
    extern ANativeWindow* g_androidWindow;
    mHwnd = reinterpret_cast<Tr2WindowHandle>( g_androidWindow );
#else
    if (mCreationLeft == DEFAULT_WINDOW_POSITION)
        mCreationLeft = 0;
    if (mCreationTop == DEFAULT_WINDOW_POSITION)
        mCreationTop = 0;
    GLFWmonitor* monitor = nullptr;
    if( mFullscreen )
    {
        monitor = glfwGetPrimaryMonitor();
    }
    mHwnd = uintptr_t( glfwCreateWindow(
        mCreationWidth <= 0 ? 640 : mCreationWidth,
        mCreationHeight <= 0 ? 480 : mCreationHeight,
        CW2A( mWindowTitle.c_str() ),
        monitor,
        nullptr ) );
	if (!mHwnd) {
		BeOS->SetError(BE32, Clsid(), "Couldn't create window?!?");
		return false;
	}
    RegisterWindowCallbacks();
    glfwMakeContextCurrent( reinterpret_cast<GLFWwindow*>( mHwnd ) );
    mWindowClient.left = 0;
    mWindowClient.top = 0;
    int width, height;
    glfwGetWindowSize( reinterpret_cast<GLFWwindow*>( mHwnd ), &width, &height );
    mWindowClient.right = width;
    mWindowClient.bottom = height;
#endif
	return true;
}


bool App::ChangeDevice(uint32_t adapter, const Tr2PresentParametersAL* pp)
{
	//We take the client width and height from this:
	bool result = gTriDev->ChangeDevice(adapter, mHwnd, pp);
	if (!result)
		return false;

	if (pp->mode.width)
		mCreationWidth = pp->mode.width;
	if (pp->mode.height)
		mCreationHeight = pp->mode.height;
	if( !mActive )
	{
		gTriDev->ApplicationActivated( TriDevice::APP_DEACTIVATED );
	}
	return true;
}


bool App::SetWindowPos(int left, int top)
{
#ifdef _WIN32
	if( mFixedWindow )
	{
		MONITORINFO monitorInfo;
		monitorInfo.cbSize = sizeof( MONITORINFO );
		int adapter = gTriDev->GetAdapter();

		HMONITOR hMonitor = nullptr;
		if( SUCCEEDED( Tr2VideoAdapterInfo::GetAdapterMonitor( adapter, (void*&)hMonitor ) ) )
		{
			GetMonitorInfo( hMonitor, &monitorInfo );
			left = monitorInfo.rcMonitor.left;
			top = monitorInfo.rcMonitor.top;
		}
	}

	if (mWindowed) {
		mCreationLeft = left;
		mCreationTop = top;

		RECT rc;
		SetRect(&rc, 0, 0, mCreationWidth, mCreationHeight); //clientsize
		AdjustWindowRect(&rc, mWindowStyle, FALSE);
		OffsetRect(&rc, mCreationLeft-rc.left, mCreationTop-rc.top); // move to mCreationLeft

		if (!mHwnd && !CreateWin())
			return false;
		// We can't move a window that's minimized.  In order to get the correct coordinates,
		// We need to briefly restore it.
		int minimized = IsIconic(mHwnd);
		if (minimized)
			ShowWindow(mHwnd, SW_RESTORE);
		// Use this rather than MoveWindow. After being fullscreen, the window sometimes becomes topmost
		::SetWindowPos( mHwnd, HWND_NOTOPMOST, rc.left, rc.top, rc.right-rc.left, rc.bottom-rc.top, SWP_SHOWWINDOW);
		::GetClientRect(mHwnd, reinterpret_cast<RECT*>( &mWindowClient ) );\
		if (minimized)
			ShowWindow(mHwnd, SW_MINIMIZE);
	}
    else
	{
		mCreationLeft = 0;
		mCreationTop = 0;
		SetRect( reinterpret_cast<RECT*>( &mWindowClient ), 0, 0, mCreationWidth, mCreationHeight );
	}
#elif defined( __ORBIS__ )
#elif defined( __ANDROID__ )
#else
    mCreationLeft = left;
    mCreationTop = top;
    if (!mHwnd && !CreateWin())
        return false;
    // We can't move a window that's minimized.  In order to get the correct coordinates,
    // We need to briefly restore it.
    int minimized = glfwGetWindowAttrib( reinterpret_cast<GLFWwindow*>( mHwnd ), GLFW_ICONIFIED );
    if (minimized)
        glfwRestoreWindow( reinterpret_cast<GLFWwindow*>( mHwnd ) );
    glfwSetWindowPos( reinterpret_cast<GLFWwindow*>( mHwnd ), left, top );
    if (minimized)
        glfwIconifyWindow( reinterpret_cast<GLFWwindow*>( mHwnd ) );
#endif
	return true;
}

bool App::OnModified( Be::Var* val )
{
	if( IsMatch( val, mFullscreen ) )
	{
		mWindowed = !mFullscreen;
	}
	else if( IsMatch( val, mWindowed ) )
	{
		mFullscreen = !mWindowed;
	}
	else if( IsMatch( val, mWindowTitle ) )
	{
#ifdef _WIN32
		SetWindowTextW( mHwnd, mWindowTitle.c_str() );
#elif defined( __ORBIS__ )
#elif defined( __ANDROID__ )
#else
        glfwSetWindowTitle( reinterpret_cast<GLFWwindow*>( mHwnd ), CW2A( mWindowTitle.c_str() ) );
#endif
	}
	else if( IsMatch( val, m_cursor ) )
	{
		m_cursor->Apply();
	}
	return true;
}

bool App::IsActive()
{
#ifdef _WIN32
	mActive = mHwnd == ::GetForegroundWindow();
#elif defined( __ORBIS__ )
#elif defined( __ANDROID__ )
#else
    mActive = glfwGetWindowAttrib( reinterpret_cast<GLFWwindow*>( mHwnd ), GLFW_FOCUSED ) != 0;
#endif
	return mActive;
}

void App::SetActive()
{
#ifdef _WIN32
	HWND foregroundWindow = ::GetForegroundWindow();
	mActive = mHwnd == foregroundWindow;

	if( !mActive )
	{
		// In order for SetForegroundWindow to work we need to be getting input from the current foreground window thread
		DWORD foregroundThreadID = GetWindowThreadProcessId( foregroundWindow, NULL );
		DWORD ourThreadID		 = GetWindowThreadProcessId( mHwnd, NULL );
		if( ourThreadID != foregroundThreadID )
		{
			AttachThreadInput( foregroundThreadID, ourThreadID, true);
			SetForegroundWindow( mHwnd);
			AttachThreadInput( foregroundThreadID, ourThreadID, false);
		}
		else
		{
			SetForegroundWindow( mHwnd );
		}

		SetFocus( mHwnd );
		ShowWindow( mHwnd, SW_RESTORE );
	}
#endif
}

bool App::CallEventHandler( uint32_t messageID, uintptr_t wParam, uintptr_t lParam, long& lResult )
{
#if BLUE_WITH_PYTHON
	if( m_eventHandler )
	{
		if( PyCallable_Check( m_eventHandler ) )
		{
			PyObject* result = PyObject_CallFunction( m_eventHandler, (char*)"(iii)", int( messageID ), int( wParam ), int( lParam ) );
            
			lResult = 0L;
			bool bHandled;
			if( result && result != Py_None )
			{
				bHandled = true;
			}
			else
			{
				bHandled = false;
			}
            
			if( bHandled )
			{
				lResult = PyInt_AS_LONG( result );
			}
            
			if( result )
			{
				Py_DECREF( result );
			}
			else
			{
				PyErr_Print();
			}
            return bHandled;
		}
	}
#endif
    return false;
}

int App::IsKeyPressed( uint32_t vKeyCode )
{
    return ( GetKeyState( vKeyCode ) & 0x8000 ) ? 1 : 0;
}

bool App::ProcessMessages()
{
#ifdef _WIN32
	MSG msg;
	bool returnValue = true;
	while(PeekMessageW(&msg, 0, 0, 0, PM_REMOVE))
	{
		if (msg.message ==	WM_QUIT)
		{				
			returnValue = false;
		}
		TranslateMessage(&msg);
		DispatchMessageW(&msg);
	}

	return returnValue;
#elif defined( __ORBIS__ )
    return true;
#elif defined( __ANDROID__ )
    return true;
#else
    glfwPollEvents();
    return true;
#endif
}

Be::Result<std::string> App::SetKey( unsigned int index, bool keyDown )
{
#ifdef _WIN32
	KEYBDINPUT  keyInput={0};
	INPUT    input={0};

	keyInput.wVk = index;
	if(!keyDown)
	{
		keyInput.dwFlags = KEYEVENTF_KEYUP;
	}

	input.type = INPUT_KEYBOARD;
	input.ki = keyInput;

	SendInput(1, &input, sizeof(input));

	return Be::Result<std::string>();
#else
	return Be::Result<std::string>( "SetKey is not (yet) available on non-windows platforms" );
#endif
}

Be::Result<std::string> App::GetKeyNameText( int vk, std::string& name )
{
	char buffer[64];
	if( App::GetKeyName( uint32_t( vk ), buffer, sizeof( buffer ) ) )
	{
		name = buffer;
	}
	else
	{
		name = "";
	}

	return Be::Result<std::string>();
}

Be::Result<std::string> App::ClipCursor( int left, int top, int right, int bottom )
{
#ifdef _WIN32
	RECT rc;
	rc.left = left;
	rc.top = top;
	rc.right = right;
	rc.bottom = bottom;

	POINT offs = {0,0};
	ClientToScreen( gTriDev->GetWindow(), &offs );
	OffsetRect( &rc, offs.x, offs.y );

	::ClipCursor(&rc);

	return Be::Result<std::string>();
#else
	return Be::Result<std::string>( "ClipCursor is not (yet) available on non-windows platforms" );
#endif
}

Be::Result<std::string> App::UnclipCursor()
{
#ifdef _WIN32
	::ClipCursor( nullptr );

	return Be::Result<std::string>();
#else
	return Be::Result<std::string>( "UnclipCursor is not (yet) available on non-windows platforms" );
#endif
}

Be::Result<std::string> App::SetCursorPos( int x, int y )
{
#ifdef _WIN32
	POINT offs = {0,0};
	ClientToScreen(gTriDev->GetWindow(), &offs);

	// Ignore errors here - this can only fail if our desktop isn't active,
	// see http://msdn.microsoft.com/en-us/library/windows/desktop/ms682573(v=vs.85).aspx
	// In that case we don't care about the results anyway.
	::SetCursorPos( x + offs.x , y + offs.y );

	return Be::Result<std::string>();
#else
	return Be::Result<std::string>( "SetCursorPos is not (yet) available on non-windows platforms" );
#endif
}

Be::Result<std::string> App::CreateDevice( unsigned int adapter, Tr2PresentParameters* pp )
{
	bool result = ChangeDevice( adapter, pp );
	if( !result )
	{
		return Be::Result<std::string>( "Couldn't create device" );
	}

	return Be::Result<std::string>();
}

void App::SetIcon(const wchar_t* filename)
{
#ifdef _WIN32
	HANDLE hIcon = LoadImageW(NULL, filename, IMAGE_ICON, 0, 0, LR_LOADFROMFILE);
	::SendMessage( mHwnd, WM_SETICON, ICON_BIG, (LPARAM)hIcon);
#endif
}

void App::ReleaseResources( TriStorage )
{
#if TRINITY_PLATFORM == TRINITY_DIRECTX9
	::ReleaseCapture();
#endif
}

bool App::OnPrepareResources()
{
	return true;
}
