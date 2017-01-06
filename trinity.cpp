#include "StdAfx.h"

#include "blue/include/Blue.h"
#ifdef TRINITYBUILD
#include "BlueExposure/include/InterfaceDefinitions.cxx"
#include "blue/include/Blue.cxx"
#include "include/TrinityId.cxx"

//Blue interfaces
BLUE_DEFINE_INTERFACE( IBluePlacementObserver );
BLUE_DEFINE_INTERFACE( IBlueEventListener );
BLUE_DEFINE_INTERFACE( IBlueObjectProxy );
#endif
#include "include/ITr2DebugRenderer.h"

#include "Tr2Renderer.h"

// creatable rendering types
#if APEX_ENABLED
#include "Apex/Apex.h"
#endif

// constants to add to Python
#include "TriConstants.h"

// stubbs for standar interfaces to register to Python
#if BLUE_WITH_PYTHON
#include "TriPythonThunkers.h"
#endif
#include "TriSettings.h"

// constants
#include "UI/Scancodes.h"
#include "UI/UIChoosers.h"
#include "Tr2TextureAtlasMan.h"
#include "Font/Tr2FontManager.h"

#ifdef _WIN32
#include "dxerr.h"
#endif
#include "TriError.h"
#include "TriSettingsRegistrar.h"

#ifndef TRINITYNAME
#error Please add TRINITYNAME=<PythonModuleName> to compiler preprocessor definitions (/D)
#endif

const char* BLUEMODULENAME = CCP_STRINGIZE( TRINITYNAME );

#ifdef TRINITYBUILD
const char* g_moduleName = "trinity";
#endif

#ifdef _WIN32
extern "C"
{
__declspec(dllexport) int AmdPowerXpressRequestHighPerformance = 1;
__declspec(dllexport) uint32_t NvOptimusEnablement = 1;
}
#endif


// reduce CRT link
extern "C" void _setargv(){}
extern "C" void _setenvp(){}

#define BLUETHUNKREG(_Class) \
	PyOS->RegisterThunker(_Class::Defs(), _Class::IID() );

void* Tr2GrannyAllocate( const char* file, granny_int32x line, granny_uintaddrx alignment, granny_uintaddrx size, granny_int32x intent )
{
	return CCPAlignedMallocWithTracking( size, alignment, "Granny", file, line );
}

void Tr2GrannyDeallocate( const char* file, granny_int32x line, void* memory )
{
	return CCPAlignedFreeWithTracking( memory );
}

#if BLUE_WITH_PYTHON
const char* InitializeForPython()
{
	const PyMethodDef dummyMethods[] = {0};

	// put myself into python as a module
	PyObject* module = Py_InitModule((char*)BLUEMODULENAME, (PyMethodDef*)dummyMethods);
	PyObject* dict = PyModule_GetDict(module);

	// Initialize trinity exceptions
	TriError::CreateExceptionObjects(dict);

	// constants
	AddTriConstants(dict);

	// put UI into python as a separate module
	PyObject* uiModule = Py_InitModule("triui", (PyMethodDef*)dummyMethods);
	PyObject* uiDict = PyModule_GetDict(uiModule);

	AddScancodesToDict(uiDict);
	AddUIChoosersToDict(uiDict);

	BLUE_REGISTER_THUNKER(ITriScalarFunction_Thunk::Defs(), ITriScalarFunction_Thunk::IID());
	BLUE_REGISTER_THUNKER(ITriVectorFunction_Thunk::Defs(), ITriVectorFunction_Thunk::IID());
	BLUE_REGISTER_THUNKER(ITriQuaternionFunction_Thunk::Defs(), ITriQuaternionFunction_Thunk::IID());
	BLUE_REGISTER_THUNKER(ITriColorFunction_Thunk::Defs(), ITriColorFunction_Thunk::IID());

	BlueRegisterToModule( module, BlueRegistration::GetClassRegs(), 
						  BlueRegistration::GetFuncRegs(),
						  BlueRegistration::GetEnumRegs(),
						  BlueRegistration::GetTestRegs(),
						  BlueRegistration::GetThunkerRegs(),
						  BlueRegistration::GetFuncSignatures() );

	BlueRegisterObjectsToModule( module, BlueRegistration::GetObjectRegs() );
	BlueRegisterExceptionsToModule( module, BlueRegistration::GetExceptionRegs() );

	PyModule_AddObject( module, "settings", BlueWrapObjectForPython(&Tr2Renderer::GetSettings()) );
	PyModule_AddObject( module, "fontMan", BlueWrapObjectForPython( g_fontManager ) );

	return NULL;
}
#endif

extern bool g_isR10G10B10FormatInverted;
extern bool g_convertA8L8FormatToB8G8R8A8;
extern bool g_requestDeviceDebugLayer;

void InitializeTrinity()
{
	GlobalStore().RegisterVariable( "BlitOriginal", static_cast<ITr2TextureProvider*>( nullptr ) );
	GlobalStore().RegisterVariable( "BlitCurrent", static_cast<ITr2TextureProvider*>( nullptr ) );
	GlobalStore().RegisterVariable( "g_texelSize", Vector4() );

#if( TRINITY_PLATFORM==TRINITY_DIRECTX9 )
	g_isR10G10B10FormatInverted = true;
#else
	g_isR10G10B10FormatInverted = false;
#endif
#if( TRINITY_PLATFORM==TRINITY_DIRECTX11 )
	g_convertA8L8FormatToB8G8R8A8 = true;
#else
	g_convertA8L8FormatToB8G8R8A8 = false;
#endif

	auto debugArg = BeOS->GetStartupArgValue( L"deviceDebug" );
	if( !debugArg.empty() )
	{
		g_requestDeviceDebugLayer = debugArg == L"1";
	}

	GrannySetAllocator( Tr2GrannyAllocate, Tr2GrannyDeallocate );

	Tr2FontManager::Initialize();

	Tr2Renderer::Initialize();
}

static void StartDLL()
{
	CCP_LOG( "Trinity (%s) module starting", BLUEMODULENAME );
	BeClasses->RegisterClasses( BlueRegistration::GetClassRegs() );

	InitializeTrinity();
}


#ifdef _WIN32 
HINSTANCE gInstance = NULL;

#ifdef _WINDLL
BOOL APIENTRY DllMain(HINSTANCE instance, DWORD reason, LPVOID)
{
	if (reason == DLL_PROCESS_ATTACH)
	{
		gInstance = instance;
		DisableThreadLibraryCalls(gInstance);
	}
	else if (reason == DLL_PROCESS_DETACH)
	{
		;
	}
	return TRUE;
}
#endif
#endif

#if BLUE_WITH_PYTHON

//--------------------------------------------------------------------
// inittrinity - python dll module entry function
//--------------------------------------------------------------------
extern "C" void
#ifdef _MSC_VER
	__declspec(dllexport)
#endif
CCP_CONCATENATE( init, TRINITYNAME ) ()
{
	StartDLL();
	InitializeForPython();
}

#elif BLUE_WITH_LUA
extern "C" int
#if defined(_MSC_VER) || defined(__ORBIS__)
	__declspec(dllexport)
#endif
CCP_CONCATENATE( luaopen_, TRINITYNAME ) ( lua_State* ls )
{
	StartDLL();

	BlueRegisterClasses( ls, g_moduleName, BlueRegistration::GetClassRegs() );
	BlueRegisterFunctions( ls, g_moduleName, BlueRegistration::GetFuncRegs() );
	BlueRegisterObjectsToModule( ls, g_moduleName, BlueRegistration::GetObjectRegs() );

	return 1;
}

#endif


#ifndef _WIN32
static void emptySignalHandler(int)
{
}
#endif

#if BLUE_WITH_PYTHON
static PyObject* PyBreakInDebugger( PyObject* module, PyObject* args )
{
#ifdef _WIN32

	char* context = NULL;
	if( PyTuple_GET_SIZE(args) == 1 )
	{
		PyObject* o = PyTuple_GetItem( args, 0 );
		if( PyString_Check( o ) )
		{
			context = PyString_AsString( o );
			OutputDebugString( "Python Triggered Breakpoint: " );
			OutputDebugString( context );
			OutputDebugString( "\n" );
		}
	}	

	__try 
	{
		// This breakpoint exception is used by several D3D return value checking functions
		// If you get, here, go up the stack and see what D3D function failed
		DebugBreak();
	}
	__except(GetExceptionCode() == EXCEPTION_BREAKPOINT ? EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH) 
	{
	}
#else
    struct sigaction action, oldAction;
    memset( &action, 0, sizeof( action ) );
    action.sa_handler = &emptySignalHandler;
    sigaction( SIGTRAP, &action, &oldAction );
    raise(SIGTRAP);
    sigaction( SIGTRAP, &oldAction, &action );
#endif

	Py_RETURN_NONE;
}
MAP_FUNCTION( 
	"BreakInDebugger", 
	PyBreakInDebugger, 
	"BreakInDebugger( [contextString] )\nBreaks in the debugger, if one is attached, allowing you to look at the program state at a point determined from Python.\n"
	":param contextString: string that is dumped into the debugger output\n"
	":type contextString: str\n"
	":rtype: None" );
#endif

ITr2DebugRendererPtr g_debugRenderer;

static void SetDebugRenderer( ITr2DebugRenderer* renderer )
{
	g_debugRenderer = renderer;
}

MAP_FUNCTION_AND_WRAP( "SetDebugRenderer", SetDebugRenderer, "Sets the debug renderer for Trinity\n:param renderer: new debug renderer" );

static ITr2DebugRenderer* GetDebugRenderer()
{
	return g_debugRenderer;
}

MAP_FUNCTION_AND_WRAP( "GetDebugRenderer", GetDebugRenderer, "Returns the debug renderer for Trinity" );

#if APEX_ENABLED

extern Tr2Apex *g_Tr2Apex;

static Tr2Apex* GetApex()
{
	if( !g_Tr2Apex )
	{
		BeClasses->CreateInstance( GetTr2ApexClsid(), BlueInterfaceIID<Tr2Apex>(), (void**)&g_Tr2Apex );
	}

	return g_Tr2Apex;
}

MAP_FUNCTION_AND_WRAP( "GetApex", GetApex, "Gets a global apex pointer" );

#endif


static const char* PyDXGetErrorString( int hr )
{
#ifdef _WIN32
	return DXGetErrorString( hr );
#else
	return "";
#endif
}

MAP_FUNCTION_AND_WRAP( "DXGetErrorString", PyDXGetErrorString, "Convert an D3D hr code to string\n:param hr: DX HRESULT" );

static const char* PyDXGetErrorDescription( int hr )
{
#ifdef _WIN32
	return DXGetErrorDescription( hr );
#else
	return "";
#endif
}

MAP_FUNCTION_AND_WRAP( "DXGetErrorDescription", PyDXGetErrorDescription, "Convert an D3D hr code to an error description\n:param hr: DX HRESULT" );

static const char* GetGrannyProductVersion()
{
	return GrannyProductVersion;
}

MAP_FUNCTION_AND_WRAP( "GetGrannyProductVersion", GetGrannyProductVersion, "Returns the 'GrannyProductVersion' string as defined by Granny" );

TRI_REGISTER_SETTING( "gpuTelemetryEnabled", g_gpuTelemetryEnabled );
