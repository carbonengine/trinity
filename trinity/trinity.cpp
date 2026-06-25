// Copyright © 2023 CCP ehf.

#include "StdAfx.h"

#include <Blue.h>
#ifdef TRINITYBUILD

//Blue interfaces
BLUE_DEFINE_INTERFACE( IBluePlacementObserver );
BLUE_DEFINE_INTERFACE( IBlueEventListener );
BLUE_DEFINE_INTERFACE( IBlueObjectProxy );
#endif
#include "include/ITr2DebugRenderer.h"

#include "Tr2Renderer.h"

#include "include/TriMath.h"

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

#include "TriSettingsRegistrar.h"

#include "Eve/IEveSpaceObject2.h"
#include "Eve/SpaceObject/Children/IEveSpaceObjectChild.h"

#ifndef TRINITYNAME
#error Please add TRINITYNAME=<PythonModuleName> to compiler preprocessor definitions (/D)
#endif

#ifdef TRINITYBUILD
const char* g_moduleName = "trinity";
#endif

#ifdef _WIN32
extern "C"
{
	__declspec( dllexport ) int AmdPowerXpressRequestHighPerformance = 1;
	__declspec( dllexport ) uint32_t NvOptimusEnablement = 1;
}
#endif

// reduce CRT link
extern "C" void _setargv()
{
}
extern "C" void _setenvp()
{
}

#define BLUETHUNKREG( _Class ) \
	PyOS->RegisterThunker( _Class::Defs(), _Class::IID() );

#if WITH_GRANNY
void* Tr2GrannyAllocate( const char* file, granny_int32x line, granny_uintaddrx alignment, granny_uintaddrx size, granny_int32x intent )
{
	return CCPAlignedMallocWithTracking( size, alignment, "Granny", file, line );
}

void Tr2GrannyDeallocate( const char* file, granny_int32x line, void* memory )
{
	return CCPAlignedFreeWithTracking( memory );
}
#endif

#if BLUE_WITH_PYTHON

#if PY_MAJOR_VERSION == 2
const char* InitializeForPython()
{
	const PyMethodDef dummyMethods[] = { 0 };

	// put myself into python as a module
	PyObject* module = Py_InitModule( CCP_STRINGIZE( CCP_CONCATENATE( TRINITYNAME, CCP_BUILD_FLAVOR ) ), (PyMethodDef*)dummyMethods );
	PyObject* dict = PyModule_GetDict( module );

	// constants
	AddTriConstants( dict );

	// put UI into python as a separate module
	PyObject* uiModule = Py_InitModule( "triui", (PyMethodDef*)dummyMethods );
	PyObject* uiDict = PyModule_GetDict( uiModule );

	AddScancodesToDict( uiDict );
	AddUIChoosersToDict( uiDict );

	BLUE_REGISTER_THUNKER( ITriScalarFunction_Thunk::Defs(), ITriScalarFunction_Thunk::IID() );
	BLUE_REGISTER_THUNKER( ITriVectorFunction_Thunk::Defs(), ITriVectorFunction_Thunk::IID() );
	BLUE_REGISTER_THUNKER( ITriQuaternionFunction_Thunk::Defs(), ITriQuaternionFunction_Thunk::IID() );
	BLUE_REGISTER_THUNKER( ITriColorFunction_Thunk::Defs(), ITriColorFunction_Thunk::IID() );

	BlueRegisterToModule( module, BlueRegistration::GetClassRegs(), BlueRegistration::GetFuncRegs(), BlueRegistration::GetEnumRegs(), BlueRegistration::GetTestRegs(), BlueRegistration::GetThunkerRegs(), BlueRegistration::GetFuncSignatures() );

	BlueRegisterObjectsToModule( module, BlueRegistration::GetObjectRegs() );
	BlueRegisterExceptionsToModule( module, BlueRegistration::GetExceptionRegs() );

	PyModule_AddObject( module, "settings", BlueWrapObjectForPython( &Tr2Renderer::GetSettings() ) );
	PyModule_AddObject( module, "fontMan", BlueWrapObjectForPython( g_fontManager ) );

	return NULL;
}
#else
PyObject* InitializeForPython()
{
	static PyMethodDef dummyMethods[] = { 0 };

	// put myself into python as a module
	static struct PyModuleDef trinityDef = {
		PyModuleDef_HEAD_INIT,
		CCP_STRINGIZE( CCP_CONCATENATE( TRINITYNAME, CCP_BUILD_FLAVOR ) ),
		"",
		-1,
		dummyMethods
	};
	PyObject* module = PyModule_Create( &trinityDef );
	if( module )
	{
		PyObject* dict = PyModule_GetDict( module );

		// constants
		AddTriConstants( dict );

		// put UI into python as a separate module
		static struct PyModuleDef triuiDef = {
			PyModuleDef_HEAD_INIT,
			"triui",
			"",
			-1,
			dummyMethods
		};
		PyObject* uiModule = PyModule_Create( &triuiDef );
		PyObject* uiDict = PyModule_GetDict( uiModule );

		AddScancodesToDict( uiDict );
		AddUIChoosersToDict( uiDict );

		PyObject* sys_modules = PyImport_GetModuleDict();
		if( PyDict_SetItemString( sys_modules, triuiDef.m_name, uiModule ) != 0 )
		{
			return nullptr;
		}

		BLUE_REGISTER_THUNKER( ITriScalarFunction_Thunk::Defs(), ITriScalarFunction_Thunk::IID() );
		BLUE_REGISTER_THUNKER( ITriVectorFunction_Thunk::Defs(), ITriVectorFunction_Thunk::IID() );
		BLUE_REGISTER_THUNKER( ITriQuaternionFunction_Thunk::Defs(), ITriQuaternionFunction_Thunk::IID() );
		BLUE_REGISTER_THUNKER( ITriColorFunction_Thunk::Defs(), ITriColorFunction_Thunk::IID() );

		BlueRegisterToModule( module, BlueRegistration::GetClassRegs(), BlueRegistration::GetFuncRegs(), BlueRegistration::GetEnumRegs(), BlueRegistration::GetTestRegs(), BlueRegistration::GetThunkerRegs(), BlueRegistration::GetFuncSignatures() );

		BlueRegisterObjectsToModule( module, BlueRegistration::GetObjectRegs() );
		BlueRegisterExceptionsToModule( module, BlueRegistration::GetExceptionRegs() );

		PyModule_AddObject( module, "settings", BlueWrapObjectForPython( &Tr2Renderer::GetSettings() ) );
		PyModule_AddObject( module, "fontMan", BlueWrapObjectForPython( g_fontManager ) );
	}

	return module;
}
#endif
#endif

extern bool g_requestDeviceDebugLayer;
extern bool g_requestDebugMarkers;
extern bool g_gpuTimersEnabled;
bool g_bindlessRenderingEnabled = true;
TRI_REGISTER_SETTING( "bindlessRenderingEnabled", g_bindlessRenderingEnabled );
extern bool g_gdrEnabled;
extern bool g_upscalingDebug;
extern ICrashReporter* TrinityALCrashes;

#if TRINITY_PLATFORM == TRINITY_METAL
extern bool g_enableMetalCounters;
TRI_REGISTER_SETTING( "enableMetalCounters", g_enableMetalCounters );
#endif

void InitializeTrinity()
{
	TrinityALCrashes = BeCrashes;

	GlobalStore().RegisterVariable( "BlitOriginal", static_cast<ITr2TextureProvider*>( nullptr ) );
	GlobalStore().RegisterVariable( "BlitCurrent", static_cast<ITr2TextureProvider*>( nullptr ) );
	GlobalStore().RegisterVariable( "g_texelSize", Vector4() );

#if TRINITY_PLATFORM == TRINITY_METAL
	bool isUsingMetalValidation = false;
	if( auto type = getenv( "METAL_DEVICE_WRAPPER_TYPE" ) )
	{
		isUsingMetalValidation = strcmp( type, "0" ) != 0;
	}
#endif

#if TRINITY_PLATFORM_SUPPORTS_PARALLEL_CONTEXTS
	extern bool g_useParallelEncoding;
	auto parallelRenderArg = BeOS->GetStartupArgValue( L"parallelrender" );
	if( parallelRenderArg == L"1" )
	{
		g_useParallelEncoding = true;
	}
	else if( parallelRenderArg == L"0" )
	{
		g_useParallelEncoding = false;
	}
#if TRINITY_PLATFORM == TRINITY_METAL
	if( g_useParallelEncoding && isUsingMetalValidation )
	{
		g_useParallelEncoding = false;
		CCP_LOGWARN( "Disabled parallel encoding because of metal validation is present and that may cause stalls during parallel encoding" );
	}
#endif
	if( g_useParallelEncoding )
	{
		CCP_LOGNOTICE( "trinity is using parallel encoding" );
	}
	else
	{
		CCP_LOGNOTICE( "trinity is not using parallel encoding" );
	}
#endif

	auto upsclingDebugArg = BeOS->GetStartupArgValue( L"upscalingDebug" );
	if( !upsclingDebugArg.empty() )
	{
		g_upscalingDebug = upsclingDebugArg == L"1";
	}

	auto debugArg = BeOS->GetStartupArgValue( L"deviceDebug" );
	if( !debugArg.empty() )
	{
		g_requestDeviceDebugLayer = debugArg == L"1";
	}

	auto markersArg = BeOS->GetStartupArgValue( L"gpuMarkers" );
	if( !markersArg.empty() )
	{
		g_requestDebugMarkers = markersArg == L"1";
	}

	auto timers = BeOS->GetStartupArgValue( L"gpuTimers" );
	if( !timers.empty() )
	{
		g_gpuTimersEnabled = timers != L"0";
	}

	auto bindlessRendering = BeOS->GetStartupArgValue( L"bindlessRendering" );
	if( !bindlessRendering.empty() )
	{
		g_bindlessRenderingEnabled = bindlessRendering != L"0";
	}

	auto gdpr = BeOS->GetStartupArgValue( L"gdpr" );
	if( !gdpr.empty() )
	{
		g_gdrEnabled = gdpr != L"0";
	}

#if WITH_GRANNY
	GrannySetAllocator( Tr2GrannyAllocate, Tr2GrannyDeallocate );
#endif

	Tr2FontManager::Initialize();

	Tr2Renderer::Initialize();

	// Make sure noise table is initialized before we start calling noise functions from multiple threads
	PerlinNoise1D( 0.0, 1.0, 1.0, 1 );
}

static void StartDLL()
{
	CCP_LOG( "Trinity (%s) module starting", CCP_STRINGIZE( TRINITYNAME ) );
	BeClasses->RegisterClasses( BlueRegistration::GetClassRegs() );

	InitializeTrinity();
}

#if BLUE_WITH_PYTHON

//--------------------------------------------------------------------
// inittrinity - python dll module entry function
//--------------------------------------------------------------------
#if PY_MAJOR_VERSION == 2
extern "C" void
#ifdef _MSC_VER
	__declspec( dllexport )
#else
	__attribute__( ( visibility( "default" ) ) )
#endif
	CCP_CONCATENATE( CCP_CONCATENATE( init, TRINITYNAME ), CCP_BUILD_FLAVOR )()
{
	StartDLL();
	InitializeForPython();
}

#else
PyMODINIT_FUNC
	CCP_CONCATENATE( CCP_CONCATENATE( PyInit_, TRINITYNAME ), CCP_BUILD_FLAVOR )()
{
	StartDLL();
	return InitializeForPython();
}
#endif
#endif


#ifndef _WIN32
static void emptySignalHandler( int )
{
}
#endif

#if BLUE_WITH_PYTHON

#ifdef _WIN32
static void DoDebugBreak()
{
	__try
	{
		// This breakpoint exception is used by several D3D return value checking functions
		// If you get, here, go up the stack and see what D3D function failed
		DebugBreak();
	}
	__except( GetExceptionCode() == EXCEPTION_BREAKPOINT ? EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH )
	{
	}
}
#endif

static PyObject* PyBreakInDebugger( PyObject* module, PyObject* args )
{
#ifdef _WIN32

	if( PyTuple_GET_SIZE( args ) == 1 )
	{
		PyObject* o = PyTuple_GetItem( args, 0 );
		if( PyVerCompat::IsPyString( o ) )
		{
			auto context = FromPython<std::string>( o );
			OutputDebugString( "Python Triggered Breakpoint: " );
			OutputDebugString( context.c_str() );
			OutputDebugString( "\n" );
		}
	}
	DoDebugBreak();
#else
	struct sigaction action, oldAction;
	memset( &action, 0, sizeof( action ) );
	action.sa_handler = &emptySignalHandler;
	sigaction( SIGTRAP, &action, &oldAction );
	raise( SIGTRAP );
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

#if WITH_GRANNY
static const char* GetGrannyProductVersion()
{
	return GrannyProductVersion;
}

MAP_FUNCTION_AND_WRAP( "GetGrannyProductVersion", GetGrannyProductVersion, "Returns the 'GrannyProductVersion' string as defined by Granny" );
#endif

static BlueStdResult GetObjectWorldTransform( IRoot* object, Matrix& result )
{
	if( IEveSpaceObjectChildPtr child = BlueCastPtr( object ) )
	{
		child->GetLocalToWorldTransform( result );
		return BlueStdResult( BLUE_STD_RESULT_OK );
	}
	else if( IEveSpaceObject2Ptr spaceObject = BlueCastPtr( object ) )
	{
		spaceObject->GetLocalToWorldTransform( result );
		return BlueStdResult( BLUE_STD_RESULT_OK );
	}

	return BlueStdResult( BLUE_STD_RESULT_TYPE_ERROR );
}

MAP_FUNCTION_AND_WRAP(
	"GetObjectWorldTransform",
	GetObjectWorldTransform,
	"Returns world transform for some supported object interfaces. Currently only\n"
	"IEveSpaceObject2 and IEveSpaceObjectChild interfaces are supported.\n"
	":param obj: blue object to get world transform from\n"
	":raies TypeError: if the function does not support the object type" );

// Interface definitions
BLUE_DEFINE_INTERFACE( IWorldPosition );
BLUE_DEFINE_INTERFACE( ITr2AnimationUpdater );
BLUE_DEFINE_INTERFACE( ITr2WorldTransformUpdater );
BLUE_DEFINE_INTERFACE( ITr2BoundingBox );

BLUE_DEFINE_INTERFACE( ITriDevice );

BLUE_DEFINE_INTERFACE( ITriColor );
BLUE_DEFINE_INTERFACE( ITriVector );
BLUE_DEFINE_INTERFACE( ITriQuaternion );
BLUE_DEFINE_INTERFACE( ITriMatrix );

BLUE_DEFINE_INTERFACE( ITriFunction );
BLUE_DEFINE_INTERFACE( ITriDuration );
BLUE_DEFINE_INTERFACE( ITriCurveLength );

BLUE_DEFINE_INTERFACE( ITriScalarFunction );
BLUE_DEFINE_INTERFACE( ITriQuaternionFunction );
BLUE_DEFINE_INTERFACE( ITriVectorFunction );
BLUE_DEFINE_INTERFACE( ITriColorFunction );

BLUE_DEFINE_INTERFACE( ITriTextureRes );

BLUE_DEFINE_INTERFACE( ITriEffectParameter );
BLUE_DEFINE_INTERFACE( ITriEffectResourceParameter );
BLUE_DEFINE_INTERFACE( ITriEffectTextureParameter );

BLUE_DEFINE_INTERFACE( ITriTargetable );

BLUE_DEFINE_INTERFACE( ITr2Renderable );
BLUE_DEFINE_INTERFACE( ITr2Updateable );
BLUE_DEFINE_INTERFACE( ITr2Pickable );

BLUE_DEFINE_INTERFACE( ITr2Scene );

BLUE_DEFINE_INTERFACE( ITr2VisualizationModeRenderer );

BLUE_DEFINE_INTERFACE( ITr2MultiPassScene );

BLUE_DEFINE_INTERFACE( ITr2DebugRenderer );
