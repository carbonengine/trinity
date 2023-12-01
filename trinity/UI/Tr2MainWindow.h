////////////////////////////////////////////////////////////
//
//    Created:   March 2020
//    Copyright: CCP 2020
//

#pragma once

#ifdef __APPLE__
#include <objc/objc-runtime.h>
#endif


BLUE_DECLARE( Tr2MouseCursor );


namespace Tr2WindowMode
{
	enum Type
	{
		FULL_SCREEN = 0,
		WINDOWED = 1,
		FIXED_WINDOW = 2,
        
        _COUNT,
	};
}


namespace Tr2WindowShowState
{
	enum Type
	{
		NORMAL = 0,
		MAXIMIZED = 1,
		MINIMIZED = 2,
	};
}

#ifdef __APPLE__
namespace Tr2ImeState_MacOS
{
	enum Type
	{
		DISABLED = 0,
		READY    = 1,
		BLOCKING = 2,
	};
}
#endif


BLUE_CLASS_IMPL( Tr2MainWindowState );
class Tr2MainWindowState: public IRoot
{
public:
	EXPOSE_TO_BLUE();

	struct State
	{
		State();

		bool operator==( const State& other ) const;
		std::string ToString() const;

		bool RequiresDeviceReset( const State& other ) const;
		ALResult PopulatePresentParameters( Tr2PresentParametersAL& presentParams ) const;

		Tr2MainWindowStatePtr WrapCopy() const;

		Tr2WindowMode::Type windowMode;
		// Video adapter index (see Tr2VideoAdapterInfo)
		uint32_t adapter;
		// Window back buffer width
		uint32_t width;
		// Window back buffer height
		uint32_t height;
		// Present interval
		Tr2RenderContextEnum::PresentInterval presentInterval;

		// Window left coordinate (not the client area), used for Tr2WindowMode::WINDOW mode
		int32_t left;
		// Window left coordinate (not the client area), used for Tr2WindowMode::WINDOW mode
		int32_t top;
		// Window frame state, used for Tr2WindowMode::WINDOW mode
		Tr2WindowShowState::Type showState;
	};

	std::string ToString() const;

	State m_state;
};
TYPEDEF_BLUECLASS( Tr2MainWindowState );


BLUE_CLASS_IMPL( Tr2MainWindow );
class Tr2MainWindow : public IRoot, public IBlueEvents
{
public:
	EXPOSE_TO_BLUE();

	Tr2MainWindow();
	~Tr2MainWindow();

	ALResult SetState( const Tr2MainWindowState::State& state );
	Tr2MainWindowState::State GetState() const;

	ALResult SetStateScript( const Tr2MainWindowState* state );
	Tr2MainWindowStatePtr GetStateScript() const;
	bool IsMinimized() const;

	bool HasFocus() const;

	void SetMinimumSize( uint32_t width, uint32_t height );

	std::wstring GetWindowTitle() const;
	void SetWindowTitle( const wchar_t* title );

	Tr2MouseCursorPtr GetMouseCursor() const;
	void SetMouseCursor( Tr2MouseCursor* cursor );
	void ClipCursor( int32_t left, int32_t top, int32_t right, int32_t bottom );
	void UnclipCursor();
	void SetCursorPos( int32_t x, int32_t y );
	std::pair<int32_t, int32_t> GetCursorPos() const;

	void SanitizeState( Tr2MainWindowState::State& state ) const;
	void SanitizeStateScript( Tr2MainWindowState* state ) const;
	Tr2MainWindowStatePtr GetDefaultState( Tr2WindowMode::Type state ) const;

	uint32_t GetBackBufferWidth() const;
	uint32_t GetBackBufferHeight() const;

	Tr2RenderContextEnum::PixelFormat GetBackBufferFormat( uint32_t adapter ) const;
	std::vector<std::pair<uint32_t, uint32_t>> GetWindowSizeOptions( uint32_t adapter, Tr2WindowMode::Type windowMode ) const;

	bool ProcessMessages();
protected:
    void OnTick( Be::Time realTime, Be::Time simTime, void* cookie ) override;
public:
	bool IsKeyToggled( uint32_t keyCode ) const;
	bool IsKeyPressed( uint32_t keyCode ) const;
	std::string GetKeyName( uint32_t keyCode ) const;
private:
	ALResult SetState( bool adjustWindow, const Tr2MainWindowState::State& state );

	void SanitizeAdapter( uint32_t& adapter ) const;
	void SanitizeFullScreenResolution( Tr2MainWindowState::State& state ) const;
	void SanitizeWindowedResolution( Tr2MainWindowState::State& state ) const;
	void SanitizeWindowPosition( Tr2MainWindowState::State& state ) const;
    
    void StoreStateSettings( const Tr2MainWindowState* state );
private:
	struct Size
	{
		uint32_t width;
		uint32_t height;
	};
	struct Rect
	{
		int32_t left;
		int32_t top;
		int32_t right;
		int32_t bottom;
	};

	bool HasWindow() const;
	void CreateOSWindow( Tr2MainWindowState::State& state );
	void DestroyOSWindow();
	void AdjustWindow( Tr2MainWindowState::State& state );
	Rect GetDesktopRect() const;
	Rect GetMonitorRect( uint32_t adapter ) const;
	Size GetWindowSize( const Size& clientSize, Tr2WindowMode::Type mode ) const;
	Size GetClientSize( const Size& windowSize, Tr2WindowMode::Type mode ) const;
	bool SupportsFullscreen() const;
    Size GetLargestWindowSize( uint32_t adapter, Tr2WindowMode::Type mode ) const;
    
    uintptr_t GetHwnd() const;
	Tr2WindowHandle GetOutputWindow() const;

	Tr2MainWindowState::State m_state;
    
    Tr2MainWindowState::State m_storedStates[Tr2WindowMode::_COUNT];

	std::wstring m_title;
	Tr2MouseCursorPtr m_cursor;
	Size m_minimumSize;
	Tr2WindowHandle m_hwnd;
	bool m_isResizing;
	bool m_inSetState;

	BlueScriptCallback m_onWindowStateChange;
	BlueScriptCallback m_onBeforeSwapChainChange;
	BlueScriptCallback m_onSwapChainChange;
private:
	BlueScriptCallback m_onMouseMove;
	BlueScriptCallback m_onMouseDown;
	BlueScriptCallback m_onMouseUp;
	BlueScriptCallback m_onMouseWheel;

	BlueScriptCallback m_onKeyDown;
	BlueScriptCallback m_onKeyUp;
	BlueScriptCallback m_onChar;

	BlueScriptCallback m_onFocusChange;
	BlueScriptCallback m_onClose;
private:
#ifdef _WIN32
	static LRESULT CALLBACK StaticWndProc( HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam );
	LRESULT WndProc( HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam );
	DWORD GetWindowStyle( Tr2WindowMode::Type windowMode ) const;

	std::pair<bool, std::vector<UINT>> GetWindowsMessageFilter() const;
	void SetWindowsMessageFilter( bool enabled, const std::vector<UINT>& filter );

	BlueScriptCallback m_onWindowsMessage;
	std::unordered_set<UINT> m_messageFilter;
	bool m_hasMessageFilter = true;
#elif defined( __APPLE__ )
    id GetWindowID() const;
    
	//IME
	BlueScriptCallback m_onInsertTextIME_MacOS;
	BlueScriptCallback m_onSetMarkedTextIME_MacOS;
	BlueScriptCallback m_onKeyboardLayoutChange_MacOS;
public:
    bool OnClose_MacOS();
    void OnWindowStartedResizing_MacOS();
    void OnWindowFnishedResizing_MacOS();
    void OnWindowResized_MacOS();
    void OnWinowMoved_MacOS();
    void OnWindowChangedKey_MacOS( bool isKey );
    void OnWindowDidChangeBackingProperties_MacOS();
    void OnWindowFullscreenChanged_MacOS( bool fullscreen );
	void OnWindowOcclusionChanged_MacOS();

    void OnMouseButton_MacOS( int32_t button, bool down, float left, float top );
    void OnMouseMove_MacOS( float left, float top );
    void OnMouseWheel_MacOS( float delta );

    void OnKey_MacOS( bool isDown, int32_t key, bool repeat = false );
    void OnChar_MacOS( wchar_t charCode );
    
    void OnUpdateCursor_MacOS();
	
	//IME
	void OnInsertTextIME_MacOS( const std::wstring& string );
	void OnSetMarkedTextIME_MacOS( const std::wstring& string, uint64_t selectedLocation, uint64_t selectedLength );
    void OnKeyboardLayoutChange_MacOS();

	Tr2ImeState_MacOS::Type m_imeState_MacOS = Tr2ImeState_MacOS::DISABLED;
#endif
};

TYPEDEF_BLUECLASS( Tr2MainWindow );
