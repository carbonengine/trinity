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
#endif

const Be::ClassInfo* TriDevice::ExposeToBlue()
{
	/////////////////////////////////////////
	// Blue class info
    EXPOSURE_BEGIN( TriDevice, "" )

		MAP_INTERFACE(ITriDevice)
		MAP_INTERFACE(INotify)

		MAP_ATTRIBUTE_WITH_CHOOSER( "depthStencilFormat", mPresentParam.depthStencilFormat, "na", Be::READ | Be::ENUM, Tr2RenderContextEnum_DepthStencilFormat_Chooser )	
		MAP_ATTRIBUTE_WITH_CHOOSER( "swapEffect", mSwapEffect, "na", Be::READWRITE | Be::NOTIFY | Be::PERSIST | Be::ENUM, Tr2RenderContextEnum_SwapEffect_Chooser )	
		MAP_ATTRIBUTE( "multiSampleType", mPresentParam.msaaType, "na", Be::READWRITE | Be::NOTIFY | Be::PERSIST )	
		MAP_ATTRIBUTE_WITH_CHOOSER( "presentationInterval", mPresentParam.presentInterval, "na", Be::READWRITE | Be::NOTIFY | Be::PERSIST | Be::ENUM, Tr2RenderContextEnum_PresentInterval_Chooser )	
		
		MAP_ATTRIBUTE( "hdrEnable", mHdrEnable, "Enable High Dynamic Range rendering", Be::READWRITE | Be::PERSIST )

		MAP_ATTRIBUTE( "adapterWidth", mDisplayMode.width, "na", Be::READ ) 
		MAP_ATTRIBUTE( "adapterHeight", mDisplayMode.height, "na", Be::READ )
		MAP_ATTRIBUTE( "adapterRefreshRate", mDisplayMode.refreshRateDenominator, "na", Be::READ )
		MAP_ATTRIBUTE( "adapter", mAdapter, "na", Be::READ )	
		
		MAP_ATTRIBUTE( "multiSampleQuality", mPresentParam.msaaQuality, "na", Be::READWRITE | Be::NOTIFY | Be::PERSIST )

        MAP_ATTRIBUTE ( "scene", m_scene, "na", Be::READWRITE | Be::NOTIFY )	

		//MAP_ATTRIBUTE( "ui", mUi, "na", Be::READ )			
		MAP_ATTRIBUTE( "width", mWidth, "na", Be::READ | Be::PERSIST )
		MAP_ATTRIBUTE( "height", mHeight, "na", Be::READ | Be::PERSIST )
		MAP_ATTRIBUTE( "viewport", mViewport, "na", Be::READ | Be::PERSIST )

		MAP_ATTRIBUTE( "backBufferCount", mBackBufferCount, "na", Be::READWRITE | Be::NOTIFY | Be::PERSIST )	
		MAP_ATTRIBUTE( "tickInterval", mTickInterval, "na", Be::READWRITE )

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
			"disableEffectLoad",
			GetEffectLoadDisabled,
			SetEffectLoadDisabled,
			"Set to true to disable loading of external effects - useful for batch processing of blue files"
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
			"mipLevelMaxChainLength",
			m_mipLevelMaxChainLength, 
			"Length of longest mip chain, i.e. largest mip level will have resolution of 2^(n-1) where n is this value", 
			Be::READWRITE | Be::NOTIFY
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

		MAP_METHOD_AND_WRAP
		( 
			"InvalidateAndUnregisterForTicks",
			PyInvalidateAndUnregisterForTicks,
			"Releases the device and all associate device resources.\n" 
			"Unregisters the device from Blue ticks.\n"
		)
		MAP_METHOD_AS_METHOD
		( 
			"CreateWindowedDevice",
			PyCreateWindowedDevice, 
			"Create a simple windowed device.\n" 
			"Parameters:\n"
			"  hwnd -- A window handle\n"
			"  width, height -- [0,widht]x[0,height] is the area within the window\n"
			"       that the device will render to. If omitted or either set to 0 the\n"
			"       device will render to the entire window area.\n"
			"( hwnd, width = 0, height = 0 )"
		)
		MAP_METHOD_AS_METHOD
		( 
			"CreateFullScreenDevice",
			PyCreateFullScreenDevice, 
			"Create a simple full screen device.\n" 
			"Parameters:\n"
			"  hwnd -- A window handle\n"
			"  width, height -- [0,widht]x[0,height] is the area within the window\n"
			"       that the device will render to. If omitted or either set to 0 the\n"
			"       device will render to the entire window area.\n"
			"( hwnd, width = 0, height = 0 )"
		)
		MAP_METHOD_AS_METHOD
		(
			"ChangeBackBufferSize",
			PyChangeBackBufferSize,
			"Changes the drawable area.\n"
			"Parameters:\n"
			" width, height -- The width and height (in pixels) of the back buffers(s)\n"
			"      for the device.\n"
			"( width, height )"
		)
#if BLUE_WITH_PYTHON
		MAP_METHOD_AND_WRAP
		( 
			"DisableResourceLoad",
			DisableResourceLoad, 
			"Disable reloading of all external resources." 
		)
		MAP_METHOD_AND_WRAP
		( 
			"GetWindow",
			PyGetWindow, 
			"no comment" 
		)		
		MAP_METHOD_AND_WRAP
		( 
			"Render",
			PyRender,
			"Renders the device." 
		)		

		MAP_METHOD_AS_METHOD
		(
			"GetPresentParameters",
			PyGetPresentParameters,
			"retrieves a dict with the current presentation parameters"
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
			"\nitself when it is destroyed in Python."
		)
		MAP_METHOD_AS_METHOD
		(
			"ResetDeviceResources",
			PyResetDeviceResources,
			"Requests that all device resources be reloaded immediately. Used for chaging HDR settings etc."
			"\nMay raise a trinity.D3DERR_INVALIDCALL if a device reset is already in progress"
		)
		MAP_METHOD_AS_METHOD
		(
			"GetPickRayFromViewport",
			PyGetPickRayFromViewport,
			"Get a ray for picking in world coordinates from screen space, using the given viewport\n"
			"and view/projection transforms."
		)
#endif

		MAP_METHOD_AND_WRAP
		(
			"ShowDeviceCursor",
			DoShowCursor,
			"( show ): Show the window cursor."
		)

		MAP_METHOD_AND_WRAP
		(
			"DoesD3DDeviceExist",
			DeviceExists,
			"Returns true if TriDevice currently has a valid D3D Device."
		)
#if TRINITYDEV
		MAP_METHOD_AND_WRAP
		(
			"DumpResources",
			PyDumpResources,
			"Dumps a list of all resources currently live in the system"
		)
#endif
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
			"Set the Tr2RenderJobs objects on the device -- debug helper until we sort this out."
		)
			
    EXPOSURE_END()
}
