/* 
	*************************************************************************************

	TriDevice.h

	Author:    Hilmar Veigar Pétursson
	Created:   November 2000
	OS:        Win32
	Project:   Trinity

	Description:   

		See TRIDEVICE_Description


	Dependencies:

		Blue, DirectX 9.0

	(c) CCP 2000

	*************************************************************************************
*/
#pragma once

#ifndef _TRIDEVICE_H_
#define _TRIDEVICE_H_

#include "include/ITriDevice.h"
#include "TriViewport.h"
#include "TriDirect3D.h"
#include "TriFrustum.h"

BLUE_DECLARE_INTERFACE( ITr2Scene );
BLUE_DECLARE_INTERFACE( ITr2Updateable );

BLUE_DECLARE( TriCurveSet );
BLUE_DECLARE_VECTOR( TriCurveSet );
BLUE_DECLARE( Tr2RenderJobs );
BLUE_DECLARE( Tr2RenderTargetGrabber );

// Global pointer to device, guaranteed to be a valid device.
// Any rendering code or resource creation code that needs access
// to the device can get it through this variable and does _not_
// need to check for validity.
#if( TRINITY_PLATFORM==TRINITY_DIRECTX9 )
extern bool g_usingEXDevice;
#endif

class TriDevice :
	public ITriDevice,
	public INotify,
	public IBlueEvents,
	public ISimTimeRebaseNotify
{
public:	
	using ITriDevice::Lock;
	using ITriDevice::Unlock;
	
	bool mDisplay;

    ITr2ScenePtr m_scene;

	Tr2WindowHandle mHwnd;	

	TriFrustum& GetFrustum()
	{
		return m_frustum;
	}

	long mWidth;
	long mHeight;

	//device creation parameters
	Tr2RenderContextEnum::SwapEffect mSwapEffect;
	int mBackBufferCount;
	Be::Time mCreationTime;

	// device information parameters
	Tr2DisplayModeInfo mDisplayMode;

	bool mHdrEnable;

	PTriViewport mViewport;

	int mTickInterval;

	void Update( Be::Time time );

	bool Render();

	void Invalidate( long level );

	/////////////////////////////////////////////////////////////////////////////////////
	// ITriDevice
	/////////////////////////////////////////////////////////////////////////////////////

	float AspectRatio();

	// Window handling	
	void GetPresentation( int& adapter, Tr2PresentParametersAL& d3dpp );
	bool SetPresentation( int adapter, const Tr2PresentParametersAL* d3dpp );
	Tr2WindowHandle GetWindow();

	//Transform screen (viewport) coordinates into projection coordinates (clip).
	void ScreenToProjection(	int x,		int y,
								float* fx,	float* fy );

	void ScreenToProjection(	int x,		int y,
								float* fx,	float* fy,
								const TriViewport* viewport );
		
	void GetPickRayFromScreen(
		int x,				// screen coordinates
		int y,
		Vector3* rayWorld,	// The ray in world coordinates
		Vector3* startWorld // Starting point in world coordinates
		);

	void GetPickRayFromViewport( 
		int x,					// screen coordinates
		int y, 
		TriViewport* viewport,	// Viewport
		const Matrix& view,		// View matrix
		const Matrix& proj,		// Projection matrix
		Vector3& rayWorld,		// Out: The ray in world coordinates
		Vector3& startWorld		// Out: Starting point in world coordinates
		);

	void UpdateCursor();

	bool ChangeDevice(
		uint32_t adapter,
		Tr2WindowHandle Tr2WindowHandle,
		const Tr2PresentParametersAL* pp );

	enum DeviceScreenType
	{
		WINDOWED,
		FULLSCREEN,
	};

	bool CreateSimpleDevice( Tr2WindowHandle hwnd, unsigned int width, unsigned int height, DeviceScreenType type );
	// Create a simple windowed or fullscreen device.
	// Parameters:
	//  hwnd -- A window handle
	//  width, height -- The width and height of the window that the device will cover (in pixels).
	//    The should be within the limits of the window size. If a 0 is passed for either
	//    the device will cover the entire 'hwnd' window.	
	bool CreateWindowedDevice( Tr2WindowHandle hwnd, unsigned int width, unsigned int height );
	bool CreateFullScreenDevice( Tr2WindowHandle hwnd, unsigned int width, unsigned int height );

	// Parameter values for ApplicationActivated method
	enum ApplicationActivation
	{
		// Aplication is being activated
		APP_ACTIVATED,
		// Aplication is moved to background
		APP_DEACTIVATED,
	};

	void ApplicationActivated( ApplicationActivation activated );

	static void RegisterResource(Tr2DeviceResource *resource);
	static void UnregisterResource(Tr2DeviceResource *resource);

	static void RegisterForUpdates(ITr2Updateable *item);
	static void UnregisterForUpdates(ITr2Updateable *item);

	void QueueForReload(Tr2DeviceResource *resource);
	void Shutdown();  //shuts the device down, clears all stuff.

	// Temporary function created during re-factoring. This method contains common code
	// that is now re-used in a couple of places through this method. Yay! Now, the difference
	// between this method and 'Shutdown' is subtle and until we determine exactly what is 
	// needed I'm going to refrain from combining those into one (even though this makes
	// sense architecturally and should be possible). <halldor 2007-07-03>  
	void InvalidateAndUnregisterForTicks();

	// Time in seconds, recentered regularly (once per hour)
	float GetAnimationTime() { return m_animationTime; }
	float GetAnimationTimeElapsed( float startTime );

	virtual bool GetWidth( uint32_t& width ) const;
	virtual bool GetHeight( uint32_t& height ) const;

	virtual void RegisterDeviceCallback( Tr2DeviceCallbackTime time, Tr2DeviceCallback callback, void* userData );
	virtual void UnregisterDeviceCallback( Tr2DeviceCallbackTime time, Tr2DeviceCallback callback, void* userData );

	virtual void GetBackBufferGrabber( ITr2RenderTargetGrabber** grabber );

	/////////////////////////////////////////////////////////////////////////////////////
	// TD3DDevice - Device States
	/////////////////////////////////////////////////////////////////////////////////////

	const void SetProjectionMatrix( Matrix* );
	const Matrix* GetProjectionMatrix();
	const Matrix* GetInvProjectionMatrix();
	const Matrix* GetInvViewMatrix();
	const Matrix* GetViewMatrix();

	bool GetCameraPosition(Vector3* out );
	bool GetCameraViewVector(Vector3* out );
	
	/////////////////////////////////////////////////////////////////////////////////////
	// INotify
	/////////////////////////////////////////////////////////////////////////////////////
	bool OnModified( Be::Var* val );

	/////////////////////////////////////////////////////////////////////////////////////
	// IBlueEvents
	/////////////////////////////////////////////////////////////////////////////////////
	void OnTick(
		Be::Time realTime,
		Be::Time simTime,
		void* cookie
		);


	TriDevice(IRoot* lockobj = NULL);
	~TriDevice();

	Be::Time mTime;

	unsigned int CreateScreenVertexDecl();

	void SetRenderJobs( Tr2RenderJobs* renderJobs );

	// Textures and methods for HDR rendering (we use surface level 0 as draw buffer). In order
	// to use the hardware to perform image processing on these buffers we must be able to texture
	// from them. This is why we create them as textures (textures can be used as surfaces but not 
	// vice versa in D3D).
	

	void SetGeometryLoadDisabled( bool value );
	void SetTextureLoadDisabled( bool value );
	void SetEffectLoadDisabled( bool value );
	void SetAsyncLoadDisabled( bool value );
	bool GetGeometryLoadDisabled();
	bool GetTextureLoadDisabled();
	bool GetEffectLoadDisabled();
	bool GetAsyncLoadDisabled();

	//////////////////////////////////
	// Mip level management methods
	/////////////////////////////////
	// Set number of mip levels allowed in a mip map chain. This can be used to 
	// decrease memory used by textures by forcing them be of a lower resolution. 
	// Note that textures that don't have mip-levels in the file are not scaled
	// down.  Default for this value is 2^31+1.
	void SetMipLevelMaxCount( int n );
	unsigned int GetMipLevelMaxCount() const { return m_mipLevelMaxChainLength; }

	// Set the number of mip levels that are chopped of the front of the mip 
	// chain.  I.e. the number of high detail mip levels that are skipped
	// while loading the file.  Default is 0.
	void SetMipLevelSkipCount( int n ) { m_mipLevelSkipCount = (unsigned int)std::max( n, 0 ); }
	unsigned int GetMipLevelSkipCount() const { return m_mipLevelSkipCount; }

	bool IsDeviceLost() const { return mDeviceLost; }
	
	/////////////////////////////////////////////////////////////////////////////////////
	// ISimTimeRebaseNotify
	/////////////////////////////////////////////////////////////////////////////////////
	void OnSimClockRebase( Be::Time oldTime, Be::Time newTime );
#if TRINITYDEV
	static void DumpResources();
#endif

	// Reset the device
	// This is not private, since it can be called from python anyway...
	bool ResetDevice( const Tr2PresentParametersAL* pp );

	int GetAdapter() { return mAdapter; }
	bool DeviceExists();

private:
	bool InitD3DDevice();  //call when a new device has been set
	bool DeviceSupportsVertexTexture();

	void DoShowCursor( bool show );
	void DoReleaseDevice();

	bool ResetDevice( unsigned adapter, const Tr2PresentParametersAL* pp );
	bool DoLowLevelDeviceReset( const Tr2PresentParametersAL& );

	void HandleRenderTick(Be::Time timestamp);

	void DeviceLost();


private:
	int mAdapter;	
	PresentationParameters mPresentParam;

	void SetDefaultRenderStates();

	//Stuff here deals with lost devices.
	bool m_ignoreInvalidate; //to avoid recursion when deleting memory
	// We must use a container class that can survive insertions during iteration without invalidating
	// the iterator.  std::set is such a container and offers fast insertion/removal.
	typedef std::set<Tr2DeviceResource *> ResourceSet;
	static ResourceSet& GetResourcesRegistered();
	static ResourceSet	s_resourcesToBeRemoved;
	static bool			s_iteratingForRelease;
	
	// This is a list of items that needs to be updated each frame, after the updates have been called on the 'scene'
	typedef std::list<ITr2Updateable *> UpdateList;
	static UpdateList& GetUpdateList();

	// This set of resources should be forcibly reloaded before the next render pass
	static ResourceSet& GetResourcesToReload();

	//A dict of blue devices, that will get 
#if BLUE_WITH_PYTHON
	BluePy m_pyResourceSet;  //weakkeydict
#endif

	//Free memory on adapter, as required before a device can be Reset.
	void ReleaseDeviceResources( TriStorage s );
	void PrepareDeviceResources();
	void RebuildDeviceResourcesInPython(); 

	bool mDeviceLost;

	float m_animationTime;
	float m_animationTimeScale;

	unsigned int m_mipLevelMaxChainLength;
	unsigned int m_mipLevelSkipCount;

	TriFrustum m_frustum;

	//////////////////////////////////////////////////////////////////////////
	// Curve sets
	PTriCurveSetVector m_curveSets;

	/////////////////////////////////////
	// Render Jobs
	// Moved to a separate object as a first part in disconnecting this from trinity.device.
	// For python code it already looks like Tr2RenderJobs is on the trinity module, but on
	// the C++ side we keep it in TriDevice for now to make sure its lifetime is still tied
	// to the lifetime of g_d3dDev, until we figure out what to do with this.
	Tr2RenderJobsPtr	m_renderJobs;

	struct DeviceResetContextManager
	{
		DeviceResetContextManager();
		~DeviceResetContextManager();	
	private:
		bool m_originalSetting;
	};

	struct CallbackData
	{
		Tr2DeviceCallback m_callback;
		void* m_userData;
	};
	TrackableStdVector<CallbackData> m_beginFrameCallbacks;
	TrackableStdVector<CallbackData> m_presentCallbacks;

	void CallCallbacks( const TrackableStdVector<CallbackData>& cbs );
public:
	EXPOSE_TO_BLUE();

	/////////////////////////////////////////
	// Python thunkers
	void PyInvalidateAndUnregisterForTicks();
#if BLUE_WITH_PYTHON
	PyObject* PyCreateWindowedDevice ( PyObject* args );
	PyObject* PyCreateFullScreenDevice ( PyObject* args );
	PyObject* PyChangeBackBufferSize ( PyObject* args );
	long PyGetWindow();
	void PyRender();

	PyObject* PyGetPresentParameters ( PyObject* args );

	PyObject* PyRegisterResource ( PyObject* args );
	PyObject* PyResetDeviceResources ( PyObject* args );
	PyObject* PyGetPickRayFromViewport ( PyObject* args );
	void DisableResourceLoad( bool flag );

	void RefreshDeviceResources();

	PyObject* PythonCreateDeviceHelper( PyObject* args, bool windowed );
#endif

#if TRINITYDEV
	void PyDumpResources();
#endif

	//--bpe stupid hack until I can figure out how to export trinity.renderContext similar to trinity.device
	Tr2RenderContext* GetRenderContext();

	unsigned	GetRenderingPlatformID();
};


///////////////////////////////////////////////////////////////////////
// Global pointer so that all nodes don't have to 
// be passed a TriDevice pointer
///////////////////////////////////////////////////////////////////////
extern BlueBasicPtr<TriDevice> gTriDev;

//Here we do some magic.  We want to initialize a global variable with a handle
//to the first class created, and also holding a reference.  But this can only
//be done with the final implementation, when the Lock() / Unlock() virtual functions
//have been created.  It's not possible to do so in the virtual class TriDevice
//So, we do it here.
#pragma warning (disable:4624) //disable warning about destructor
class TriDeviceLock : public RootRefLock<TriDevice>
{
public:
	TriDeviceLock()
	{
		if (!gTriDev)
			gTriDev = this;
	}
};

//TYPEDEF_BLUECLASS(TriDevice);
typedef TriDeviceLock OTriDevice;



#endif