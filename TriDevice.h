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

BLUE_DECLARE_INTERFACE( ITr2Scene );
BLUE_DECLARE_INTERFACE( ITr2Updateable );

BLUE_DECLARE( TriCurveSet );
BLUE_DECLARE_VECTOR( TriCurveSet );
BLUE_DECLARE( Tr2RenderJobs );
BLUE_DECLARE( Tr2RenderTargetGrabber );

extern const Be::VarChooser TriDeviceTypeChooser[];

// Global pointer to device, guaranteed to be a valid device.
// Any rendering code or resource creation code that needs access
// to the device can get it through this variable and does _not_
// need to check for validity.
#if( TRINITY_PLATFORM==TRINITY_DIRECTX9 )
extern bool g_usingEXDevice;
#endif

BLUE_CLASS( TriDevice ):
	public ITriDevice,
	public IBlueEvents,
	public ISimTimeRebaseNotify
{
public:	
	using ITriDevice::Lock;
	using ITriDevice::Unlock;

    ITr2ScenePtr m_scene;

	Tr2WindowHandle mHwnd;	

	long mWidth;
	long mHeight;

	//device creation parameters
	Tr2RenderContextEnum::SwapEffect mSwapEffect;
	int mBackBufferCount;

	// device information parameters
	Tr2DisplayModeInfo mDisplayMode;

	PTriViewport mViewport;

	int mTickInterval;

	void Update( Be::Time realTime, Be::Time simTime );

	bool Render();

	/////////////////////////////////////////////////////////////////////////////////////
	// ITriDevice
	/////////////////////////////////////////////////////////////////////////////////////

	float AspectRatio();

	// Window handling	
	bool SetPresentation( int adapter, const Tr2PresentParametersAL* d3dpp );
	Tr2WindowHandle GetWindow();

	//Transform screen (viewport) coordinates into projection coordinates (clip).
	void ScreenToProjection(	int x,		int y,
								float* fx,	float* fy );

	void ScreenToProjection(	int x,		int y,
								float* fx,	float* fy,
								const TriViewport* viewport );
		
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
		NO_ADAPTER,
	};

	enum DeviceType
	{
		DEVICE_TYPE_HARDWARE,
		DEVICE_TYPE_SOFTWARE,
	};

	bool CreateSimpleDevice( 
		Tr2WindowHandle hwnd, 
		unsigned int width, 
		unsigned int height, 
		DeviceScreenType type, 
		Tr2RenderContextEnum::PresentInterval presentInterval,
		unsigned int adapter = Tr2VideoAdapterInfo::DEFAULT_ADAPTER );

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

	// Temporary function created during re-factoring. This method contains common code
	// that is now re-used in a couple of places through this method. Yay! Now, the difference
	// between this method and 'Shutdown' is subtle and until we determine exactly what is 
	// needed I'm going to refrain from combining those into one (even though this makes
	// sense architecturally and should be possible). <halldor 2007-07-03>  
	void InvalidateAndUnregisterForTicks();

	// Time in seconds, recentered regularly (once per hour)
	float GetAnimationTime() { return m_animationTime; }
	float GetAnimationTimeElapsed( float startTime );

	virtual void SetTickInterval( int value );


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

	Be::Time m_realTime;
	Be::Time m_simTime;

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

	// Set the number of mip levels that are chopped of the front of the mip 
	// chain.  I.e. the number of high detail mip levels that are skipped
	// while loading the file.  Default is 0.
	unsigned int GetMipLevelSkipCount() const { return m_mipLevelSkipCount; }

	bool IsDeviceLost() const { return mDeviceLost; }
	
	/////////////////////////////////////////////////////////////////////////////////////
	// ISimTimeRebaseNotify
	/////////////////////////////////////////////////////////////////////////////////////
	void OnSimClockRebase( Be::Time oldTime, Be::Time newTime );

	bool ResetDevice();

	int GetAdapter() { return mAdapter; }
	bool DeviceExists();

	// Add a callback to be processed after update render jobs are processed. This
	// can for example be used for routing callbacks from worker threads into
	// Python at some well defined point.
	void AddPostUpdateCallback( IBlueCallbackMan::CallbackFunc cb, void* context );

private:
	bool InitD3DDevice();  //call when a new device has been set
	void DestroyRenderContext();

	void DoReleaseDevice();

	bool ResetDevice( unsigned adapter, const Tr2PresentParametersAL* pp );

	void HandleRenderTick( Be::Time realTime, Be::Time simTime );

	bool SetPresentParameters( unsigned adapter, const Tr2PresentParametersAL& pp );

	static void LogAllLiveResources( Tr2ALMemoryTypes flags = AL_MEMORY_VIDEO | AL_MEMORY_MANAGED );


private:
	BlueScriptCallback m_onDeviceRemoved;

	DeviceType m_deviceType;
	
	int mAdapter;	
	PresentationParameters mPresentParam;

	// We must use a container class that can survive insertions during iteration without invalidating
	// the iterator.  std::set is such a container and offers fast insertion/removal.
	typedef std::set<Tr2DeviceResource *> ResourceSet;
	static ResourceSet& GetResourcesRegistered();
	static ResourceSet	s_resourcesToBeRemoved;
	static bool			s_iteratingForRelease;
	
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

	unsigned int m_mipLevelSkipCount;

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

	IBlueCallbackManPtr m_postUpdateCallbacks;

public:
	EXPOSE_TO_BLUE();

	/////////////////////////////////////////
	// Python thunkers
#if BLUE_WITH_PYTHON
	PyObject* PyCreateWindowedDevice ( PyObject* args );
	PyObject* PyCreateFullScreenDevice ( PyObject* args );
	PyObject* PyCreateWindowlessDevice ( PyObject* args );
	
	size_t PyGetWindow();
	void PyRender();

	PyObject* PyGetPresentParameters ( PyObject* args );

	PyObject* PyRegisterResource ( PyObject* args );
	PyObject* PyGetPickRayFromViewport ( PyObject* args );

	void RefreshDeviceResources();

	PyObject* PythonCreateDeviceHelper( PyObject* args, DeviceScreenType screenType );
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