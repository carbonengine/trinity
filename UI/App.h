/* 
	*************************************************************************

	App.h

	Created:   May 2001
	OS:        Win32
	Project:   TriUI

	Description:   

		UI stuff in Trinity


	Dependencies:

		Blue, Trinity

	(c) CCP 2000, 2001

	*************************************************************************
*/
#pragma once
#ifndef _APP_H_
#define _APP_H_

#include "include/Rect.h"
#include "include/Point.h"
#include "Tr2DeviceResource.h"

BLUE_DECLARE( Tr2MouseCursor );
BLUE_DECLARE( TriRect );
BLUE_DECLARE( Tr2PresentParameters );

BLUE_CLASS( App ) : 
	public INotify,
	public IBlueEvents,
	public Tr2DeviceResource
{
public:
	using INotify::Lock;
	using INotify::Unlock;

	Tr2WindowHandle GetWindowHandle();

	// The olde python API, to be phased out
	bool Create();
	bool Destroy();
	bool Initialize3DEnvironment();
	

	//The new python api
	bool AdjustWindowForChange(bool windowed, bool fixedWindow);
	bool CreateWin();
	bool ChangeDevice(uint32_t adapter, const Tr2PresentParametersAL* pp);
	bool SetWindowPos(int left, int top);

	int GetVirtualScreenWidth() const;
	int GetVirtualScreenHeight() const;

	// Process messages, return false if WM_QUIT is received
	bool ProcessMessages();


	Be::Result<std::string> CreateDevice( unsigned int adapter, Tr2PresentParameters* pp );
protected:
	void ReleaseResources( TriStorage ) override;
	bool OnPrepareResources() override;
private:
	void CreateImpl();

public:
	EXPOSE_TO_BLUE();

	App();

	~App();

	uint32_t mWindowStyle;
	uint32_t mCreationLeft;
	uint32_t mCreationTop;
	uint32_t mCreationWidth;
	uint32_t mCreationHeight;
	long mRefreshRate;
	std::wstring mWindowTitle;
	bool mHideTitle;

	
	// private members
	Tr2WindowHandle mHwnd;

#ifdef _WIN32
	static ATOM mWndClassAtom;
#endif
	Rect mWindowClient;
	
    // Internal variables for the state of the app
    bool              mFullscreen;
	bool			  mWindowed; // !mFullscreen
    bool              mActive;
    bool              mReady;
	
	bool IsHidden() const { return mIsHidden; }
private:
#ifdef _WIN32
	friend void TGNotificationCallback( TG_NOTIFY_TYPE type, DWORD state, int data, void *context );
#endif

#if BLUE_WITH_PYTHON
	// Python callable for handling Windows events
	PyObject* m_eventHandler;

	// Separate handler for Windows session change - WM_WTSSESSION_CHANGE
	PyObject* m_sessionChangeHandler;
#endif

	bool mIsHidden;
	bool mIsMaximized;
	bool mFixedWindow;
	bool mSendResizeEvent;
	bool mIsResizing;
	IBlueEventListenerPtr mResizeEventListener;

	long mMinimumWidth;
	long mMinimumHeight;

	bool mIsTransgaming;
	IBlueEventListenerPtr mTGToggleEventListener;
	bool mSendToggleEvent;

	Point GetMinBounds();
	Point GetMaxBounds();

	Tr2MouseCursorPtr m_cursor;

	void SetIcon(const wchar_t* filename);

public:
	void Quit();
	void MoveWindow( int x, int y, Be::Optional<int> width, Be::Optional<int> height );
	void Minimize( Be::OptionalWithDefaultValue<bool, true> minimize );
	void Maximize( Be::OptionalWithDefaultValue<bool, true> maximize );
	TriRect* GetClientRect( int );
	TriRect* GetWindowRect();
	bool IsActive();
	void SetActive();

#if BLUE_WITH_PYTHON
	PyObject* PyCreate ( PyObject* args );
	PyObject* PyDestroy ( PyObject* args );
	PyObject* PyRun ( PyObject* args );

	PyObject* PyGetHwnd ( PyObject* args );
	PyObject* PyGetHwndAsLong ( PyObject* args );

	// These three are the new style python control thingies.
	PyObject* PyAdjustWindowForChange ( PyObject* args );
	PyObject* PyChangeDevice ( PyObject* args );
	PyObject* PySetWindowPos ( PyObject* args );
#endif

	Be::Result<std::string> SetKey( unsigned int index, bool keyDown );
	Be::Result<std::string> GetKeyNameText( int vk, std::string& name );
	Be::Result<std::string> ClipCursor( int left, int top, int right, int bottom );
	Be::Result<std::string> UnclipCursor();
	Be::Result<std::string> SetCursorPos( int x, int y );
	
	/////////////////////////////////////////
	// IBlueEvents interface
	void OnTick(
		Be::Time realTime,		// Universal time in seconds
		Be::Time simTime,		// Simulation time
		void* cookie			// user supplied cookie value
		);

	/////////////////////////////////////////////////////////////////////////////////////
	// INotify
	/////////////////////////////////////////////////////////////////////////////////////
	bool OnModified(
		Be::Var* val
		);

#ifdef _WIN32
	static LRESULT CALLBACK _WndProc(HWND hwnd, UINT msg, WPARAM w, LPARAM l);
	LRESULT WndProc(HWND hwnd, UINT msg, WPARAM w, LPARAM l);
#else
    void RegisterWindowCallbacks();
    void OnAppWindowResized( int width, int height );
    void OnAppWindowClosed();
    void OnAppWindowFocused( bool focused );
    void OnAppMouseMove( int x, int y );
    void OnAppMouseButton( uint32_t message, int button, int mods );
    void OnAppMouseScroll( int xoffset, int yoffset );
    void OnAppKeyDown( int key, int mods );
    void OnAppKeyUp( int key, int mods );
    void OnAppChar( unsigned character );
#endif
    uint32_t GetKeyState( uint32_t vKeyCode );
    int IsKeyPressed( uint32_t vKeyCode );
    static bool GetKeyName( uint32_t vKeyCode, char* buffer, size_t bufferSize );
    bool CallEventHandler( uint32_t messageID, uintptr_t wParam, uintptr_t lParam, long& lResult );
};

TYPEDEF_BLUECLASS(App);

extern App* g_app;

#endif