#include "StdAfx.h"
#include "TriDevice.h"
#include "RenderJob/Tr2RenderJobs.h"

BLUE_DEFINE( TriDevice );

#define TRIDEVICE_Description \
"TriDevice has been a bit of a trash can through the development of Trinity. Now \r\n\
it only handles the assosiation of scenes and the actual D3D Device. Methods \r\n\
such as picking into the scene etc. will probably be added here."

#if BLUE_WITH_PYTHON
void TriDevice::RefreshDeviceResources()
{
	ReleaseDeviceResources( TRISTORAGE_ALL );
	SetPresentParameters( mAdapter, mPresentParam );
	PrepareDeviceResources();
}

PyObject* BuildTuple( const Vector3& v )
{
	PyObject* tuple = PyTuple_New( 3 );
	PyTuple_SET_ITEM(tuple, 0, PyFloat_FromDouble( v.x ) );
	PyTuple_SET_ITEM(tuple, 1, PyFloat_FromDouble( v.y ) );
	PyTuple_SET_ITEM(tuple, 2, PyFloat_FromDouble( v.z ) );

	return tuple;
}

PyObject* TriDevice::PyGetPickRayFromViewport( PyObject* args )
{	
	// Check the arguments
	int x,y;
	PyObject* viewportObj;
	PyObject* viewObj;
	PyObject* projObj;
	if( !PyArg_ParseTuple( args, "iiOOO", &x, &y, &viewportObj, &viewObj, &projObj ) )
	{
		return NULL;
	}

	TriViewport* viewport = BluePythonCast<TriViewport*>( viewportObj );
	if( !viewport )
	{
		return NULL;
	}

	Matrix view;
	if( !BlueExtractArgument( viewObj, view, 3 ) )
	{
		return NULL;
	}

	Matrix proj;
	if( !BlueExtractArgument( projObj, proj, 4 ) )
	{
		return NULL;
	}

	// Call the C++ counterpart
	Vector3 pRay;
	Vector3 pStart;
	GetPickRayFromViewport( x, y, viewport, view, proj, pRay, pStart);

	// Wrap the results a tuple


	PyObject *r = PyTuple_New(2);
	PyTuple_SET_ITEM(r, 0, BuildTuple( pRay ) );
	PyTuple_SET_ITEM(r, 1, BuildTuple( pStart ) );

	return r;
}


PyObject* GetVideoMemoryInfo( PyObject* self, PyObject* args )
{
	if( !PyArg_ParseTuple( args, "" ) )
	{
		return NULL;
	}
	if( !gTriDev )
	{
		PyErr_SetString( PyExc_ValueError, "device is not available yet" );
		return nullptr;
	}

#if TRINITY_PLATFORM == TRINITY_DIRECTX12
	CComPtr<IDXGIAdapter1> dxgiAdapter;
	CComPtr<IDXGIOutput> output;

	if( FAILED( TrinityALImpl::GetVideoAdapter( gTriDev->GetAdapter(), &dxgiAdapter, &output ) ) )
	{
		PyErr_SetString( PyExc_OSError, "failed to query the GPU adapter" );
		return nullptr;
	}

	CComQIPtr<IDXGIAdapter3> adapter( dxgiAdapter );
	if( !adapter )
	{
		PyErr_SetString( PyExc_OSError, "no OS support: old Windows version?" );
		return nullptr;
	}

	auto result = PyDict_New();

	auto ExtractInfo = [&]( auto group, const char* groupName ) {
		DXGI_QUERY_VIDEO_MEMORY_INFO info;
		if( FAILED( adapter->QueryVideoMemoryInfo( 0, group, &info ) ) )
		{
			return;
		}

		auto dict = PyDict_New();
		auto value = PyLong_FromSize_t( info.AvailableForReservation );
		PyDict_SetItemString( dict, "AvailableForReservation", value );
		Py_DecRef( value );
		value = PyLong_FromSize_t( info.Budget );
		PyDict_SetItemString( dict, "Budget", value );
		Py_DecRef( value );
		value = PyLong_FromSize_t( info.CurrentReservation );
		PyDict_SetItemString( dict, "CurrentReservation", value );
		Py_DecRef( value );
		value = PyLong_FromSize_t( info.CurrentUsage );
		PyDict_SetItemString( dict, "CurrentUsage", value );
		Py_DecRef( value );
		PyDict_SetItemString( result, groupName, dict );
		Py_DecRef( dict );
	};

	ExtractInfo( DXGI_MEMORY_SEGMENT_GROUP_LOCAL, "Local" );
	ExtractInfo( DXGI_MEMORY_SEGMENT_GROUP_NON_LOCAL, "NonLocal" );

	return result;
#else
	PyErr_SetString( PyExc_NotImplementedError, "function is not available for this platform" );
	return nullptr;
#endif
}

MAP_FUNCTION(
	"GetVideoMemoryInfo",
	GetVideoMemoryInfo,
	"Returns an API-specific video memory information. Implemented for dx12 only.\n"
	":rtype: dict[str, Dict[str, long]]" );

#endif

const Be::VarChooser TriDeviceTypeChooser[] = {
	// Name		   Value		    Docstring
	{ "HARDWARE", BeCast( TriDevice::DEVICE_TYPE_HARDWARE ), "Hardware device" },
	{ "SOFTWARE", BeCast( TriDevice::DEVICE_TYPE_SOFTWARE ), "Software device" },
	{ 0 }
};

const Be::VarChooser TriDeviceThrottlingReasonChooser[] = {
	{ "WINDOW_OUT_OF_FOCUS", BeCast( TriDevice::WINDOW_OUT_OF_FOCUS ), "" },
	{ "WINDOW_HIDDEN", BeCast( TriDevice::WINDOW_HIDDEN ), "" },
	{ 0 }
};

BLUE_REGISTER_ENUM_EX( "TriDeviceType", 
					   TriDevice::DeviceType, 
					   TriDeviceTypeChooser, ENUM_REG_ENUM_OBJECT_ON_MODULE );


const Be::ClassInfo* TriDevice::ExposeToBlue()
{
	/////////////////////////////////////////
	// Blue class info
    EXPOSURE_BEGIN( TriDevice, "" )

		MAP_INTERFACE(ITriDevice)

		MAP_ATTRIBUTE_WITH_CHOOSER( "swapEffect", mSwapEffect, "na", Be::READWRITE | Be::NOTIFY | Be::PERSIST | Be::ENUM, Tr2RenderContextEnum_SwapEffect_Chooser )	
		MAP_ATTRIBUTE( "multiSampleType", mPresentParam.msaaType, "na", Be::READWRITE | Be::NOTIFY | Be::PERSIST )	
		MAP_ATTRIBUTE_WITH_CHOOSER( "presentationInterval", mPresentParam.presentInterval, "na", Be::READWRITE | Be::NOTIFY | Be::PERSIST | Be::ENUM, Tr2RenderContextEnum_PresentInterval_Chooser )	

		MAP_ATTRIBUTE( "adapterWidth", mDisplayMode.width, "na", Be::READ ) 
		MAP_ATTRIBUTE( "adapterHeight", mDisplayMode.height, "na", Be::READ )
		MAP_ATTRIBUTE( "adapterRefreshRate", mDisplayMode.refreshRateDenominator, "na", Be::READ )
		MAP_ATTRIBUTE( "adapter", mAdapter, "na", Be::READ )	
		
		MAP_ATTRIBUTE( "multiSampleQuality", mPresentParam.msaaQuality, "na", Be::READWRITE | Be::NOTIFY | Be::PERSIST )

        MAP_ATTRIBUTE ( "scene", m_scene, "na", Be::READWRITE | Be::NOTIFY )	

		//MAP_ATTRIBUTE( "ui", mUi, "na", Be::READ )			
		MAP_ATTRIBUTE( "width", mWidth, "na", Be::READ )
		MAP_ATTRIBUTE( "height", mHeight, "na", Be::READ )
		MAP_ATTRIBUTE( "viewport", mViewport, "na", Be::READ | Be::PERSIST )
		MAP_ATTRIBUTE_WITH_CHOOSER( "deviceType", m_deviceType, "Hardware/Software device", Be::READWRITE| Be::ENUM, TriDeviceTypeChooser )

		MAP_ATTRIBUTE( "backBufferCount", mBackBufferCount, "na", Be::READWRITE | Be::NOTIFY )	
		MAP_ATTRIBUTE( "tickInterval", mTickInterval, "na", Be::READWRITE )

		MAP_ATTRIBUTE_WITH_CHOOSER( "throttlingState", m_throttlingState, "", Be::READ, TriDeviceThrottlingReasonChooser )
		MAP_ATTRIBUTE( "allowThrottling", m_allowThrottling, "", Be::READWRITE )

		MAP_PROPERTY
		(
			"disableGeometryLoad",
			GetGeometryLoadDisabled,
			SetGeometryLoadDisabled,
			"Set to true to disable loading of external geometry - useful for batch processing of blue files"
		)
		MAP_PROPERTY
		(
			"disableTextureLoad",
			GetTextureLoadDisabled,
			SetTextureLoadDisabled,
			"Set to true to disable loading of external textures - useful for batch processing of blue files"
		)
		MAP_PROPERTY
		(
			"disableAsyncLoad",
			GetAsyncLoadDisabled,
			SetAsyncLoadDisabled,
			"Set to true to make resource loads synchronous"
		)
		MAP_ATTRIBUTE
		( 
			"mipLevelSkipCount", 
			m_mipLevelSkipCount, 
			"Number of high detail mip levels we skip, i.e. chop of the front of the mip chain.",
			Be::READWRITE 
		)

		MAP_ATTRIBUTE( "curveSets", m_curveSets, "Curve sets that are update each frame. Finished curve sets are removed automatically.", Be::READ | Be::PERSIST ) 
		
		MAP_ATTRIBUTE( "animationTime", m_animationTime, "Time (in seconds) used for animations", Be::READWRITE ) 
		MAP_ATTRIBUTE ( "animationTimeScale", m_animationTimeScale, "Scale on animation time. Set to 0 to freeze all animations", Be::READWRITE )

		MAP_ATTRIBUTE( 
			"onDeviceRemoved", 
			m_onDeviceRemoved,
			"Callback function that is called when a GPU device is removed (driver crash, etc.).\n"
			"The function is called with arguments (hr, hrMessage, removedCount, marker) where:\n"
			"hr - HRESULT code for removal reason\n"
			"hrMessage - HRESULT message string\n"
			"lostCount - number of device removed events since the process started\n"
			"maker - GPU marker name last seen before device removal (may not always be available)",
			Be::READWRITE
		)

		MAP_METHOD_AS_METHOD
		( 
			"CreateWindowedDevice",
			PyCreateWindowedDevice, 
			"Create a simple windowed device.\n" 
			":param hwnd: A window handle\n"
			":type hwnd: int\n"
			":param width: window area width\n"
			":type width: Optional[int]\n"
			":param height: [0,widht]x[0,height] is the area within the window\n"
			"       that the device will render to. If omitted or either set to 0 the\n"
			"       device will render to the entire window area.\n"
			":type height: Optional[int]\n"
			":param presentInterval: presentation interval\n"
			":type presentInterval: Optional[int]\n"
			":param adapter: video adapter index\n"
			":type adapter: Optional[int]\n"
			":rtype: None"
		)
		MAP_METHOD_AS_METHOD
		( 
			"CreateFullScreenDevice",
			PyCreateFullScreenDevice, 
			"Create a simple full screen device.\n" 
			":param hwnd: A window handle\n"
			":type hwnd: int\n"
			":param width: window area width\n"
			":type width: Optional[int]\n"
			":param height: [0,widht]x[0,height] is the area within the window\n"
			"       that the device will render to. If omitted or either set to 0 the\n"
			"       device will render to the entire window area.\n"
			":type height: Optional[int]\n"
			":param presentInterval: presentation interval"
			":type presentInterval: Optional[int]\n"
			":rtype: None"
		)
		MAP_METHOD_AS_METHOD
		( 
			"CreateWindowlessDevice",
			PyCreateWindowlessDevice, 
			"Create a simple device with no swap chain.\n" 
			":rtype: None"
		)
#if BLUE_WITH_PYTHON
		MAP_METHOD_AND_WRAP
		( 
			"Render",
			PyRender,
			"Renders the device." 
		)		

		MAP_METHOD_AS_METHOD
		(
			"RegisterResource",
			PyRegisterResource,
			"Register a Python device resource which will get"
			"\n 1. OnInvalidate() call when device is lost or destroyed"
			"\n 2. OnCreate( device ) call for device reset or newly created device"
			"\nNOTE: Only implement the methods you need. Neither of these methods are"
			"\nrequired. Normally only OnCreate is needed. Also note that there is"
			"\nno unregister call because the object will automatically unregister"
			"\nitself when it is destroyed in Python.\n"
			":param resource: resource object\n"
			":rtype: None"
		)
		MAP_METHOD_AS_METHOD
		(
			"GetPickRayFromViewport",
			PyGetPickRayFromViewport,
			"Get a ray for picking in world coordinates from screen space, using the given viewport\n"
			"and view/projection transforms.\n"
			":param x: mouse X position\n"
			":type x: int\n"
			":param y: mouse Y position\n"
			":type y: int\n"
			":param viewport: viewport\n"
			":type viewport: TriViewport\n"
			":param view: view transform\n"
			":type view: tuple[tuple[float]]\n"
			":param proj: projection transform\n"
			":type proj: tuple[tuple[float]]\n"
			":rtype: ((float, float, float), (float, float, float))"
		)
#endif

		MAP_METHOD_AND_WRAP
		(
			"DoesD3DDeviceExist",
			DeviceExists,
			"Returns true if TriDevice currently has a valid D3D Device."
		)
#if BLUE_WITH_PYTHON
		MAP_METHOD_AND_WRAP( 
			"RefreshDeviceResources", 
			RefreshDeviceResources,
			"Releases all D3D resources from memory and recreates them from source."
		)
#endif

		MAP_METHOD_AND_WRAP( "GetRenderContext", GetRenderContext, "TODO DEBUG" )

		MAP_METHOD_AND_WRAP
		(
			"GetRenderingPlatformID",
			GetRenderingPlatformID,
			"Returns an ID identifying the current rendering backend."
		)

		MAP_METHOD_AND_WRAP
		(
			"SetRenderJobs",
			SetRenderJobs,
			"Set the Tr2RenderJobs objects on the device -- debug helper until we sort this out.\n"
			":param renderJobs: render jobs object"
		)

		MAP_METHOD_AND_WRAP
		(
			"SupportsRenderTargetFormat",
			SupportsRenderTargetFormat,
			"Returns True if the device support render targets with the given pixel format\n"
			":param format: render target pixel format"
		)

		MAP_METHOD_AND_WRAP(
			"IsVariableRefreshRateSupported",
			IsVariableRefreshRateSupported,
			"Returns True if the device support variable refresh rate (gsync, freesync)"
		)

    EXPOSURE_END()
}
