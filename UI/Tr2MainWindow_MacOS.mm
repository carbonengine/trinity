////////////////////////////////////////////////////////////
//
//    Created:   March 2020
//    Copyright: CCP 2020
//

#include "StdAfx.h"

#if __APPLE__
#include "Tr2MainWindow.h"
#include "TriDevice.h"
#include "Scancodes.h"
#include "Tr2MouseCursor.h"
#include "TouchBar.h"
#import <Cocoa/Cocoa.h>
#import <GameController/GameController.h>
#import <QuartzCore/QuartzCore.h>
#import <IOKit/pwr_mgt/IOPMLib.h>

extern CcpMeanStatisticsEntry g_activeFrametimeMean;
extern CcpStdDevStatisticsEntry g_activeFrametimeStdDev;
CCP_STATS_DECLARED_ELSEWHERE( frameTime );


@interface CcpApplication : NSApplication<NSApplicationDelegate>
- (void)clipCursor:(NSRect)rect;
- (void)unclipCursor;
@end

@interface CcpWindow : NSWindow<NSWindowDelegate>
- (void)initialize:(Tr2MainWindow*)mainWindow;
- (BOOL)isFullscreen;
- (BOOL)isInFullscreenTransition;
@end


@interface CcpContentView : NSView<NSTextInputClient>
{
    Tr2MainWindow* m_mainWindow;
    NSTrackingArea* m_trackingArea;
    TouchBar* m_touchBar;
}
- (instancetype)initWithWindow:(Tr2MainWindow*)initWindow;
- (void)updateDrawableSize;
- (void)textInputContextKeyboardSelectionDidChange:(NSNotification*)notification;
@end


namespace
{

bool s_quitRequested = false;
bool s_applicationInitialized = false;


void CreateMenuBar()
{
    NSString* appName = [NSRunningApplication currentApplication].localizedName;

    NSMenu* bar = [[NSMenu alloc] init];
    [NSApp setMainMenu:bar];

    NSMenu* appMenu = [[NSMenu alloc] initWithTitle:appName];
    [[bar addItemWithTitle:@"" action:NULL keyEquivalent:@""] setSubmenu:appMenu];

    NSMenu* servicesMenu = [[NSMenu alloc] init];
    [NSApp setServicesMenu:servicesMenu];
    [[appMenu addItemWithTitle:@"Services"
                       action:NULL
                keyEquivalent:@""] setSubmenu:servicesMenu];

    [appMenu addItem:[NSMenuItem separatorItem]];
    [appMenu addItemWithTitle:[NSString stringWithFormat:@"Hide %@", appName]
                       action:@selector(hide:)
                keyEquivalent:@""];
    [[appMenu addItemWithTitle:@"Hide Others"
                       action:@selector(hideOtherApplications:)
                keyEquivalent:@""]
        setKeyEquivalentModifierMask:NSEventModifierFlagOption | NSEventModifierFlagCommand];
    [appMenu addItemWithTitle:@"Show All"
                       action:@selector(unhideAllApplications:)
                keyEquivalent:@""];
    [appMenu addItem:[NSMenuItem separatorItem]];
    [appMenu addItemWithTitle:[NSString stringWithFormat:@"Quit %@", appName]
                       action:@selector(terminate:)
                keyEquivalent:@""];
}

NSWindowStyleMask GetWindowStyleMask( Tr2WindowMode::Type windowMode )
{
    if( windowMode == Tr2WindowMode::WINDOWED )
    {
        return NSWindowStyleMaskTitled | NSWindowStyleMaskClosable | NSWindowStyleMaskMiniaturizable | NSWindowStyleMaskResizable;
    }
    else
    {
        return NSWindowStyleMaskBorderless;
    }
}

void SetWindowTitle( NSWindow* window, const std::wstring& title )
{
    NSString* nsTitle = [[NSString alloc] initWithBytes: title.c_str()
                                                 length: sizeof(wchar_t) * title.length()
                                               encoding: NSUTF32LittleEndianStringEncoding];
    [window setTitle:nsTitle];
}

NSUInteger KeyCodeToModifierFlag( uint16_t key )
{
    switch( key )
    {
    case kVK_Shift:
    case kVK_RightShift:
        return NSEventModifierFlagShift;
    case kVK_Control:
    case kVK_RightControl:
        return NSEventModifierFlagControl;
    case kVK_Option:
    case kVK_RightOption:
        return NSEventModifierFlagOption;
    case kVK_Command:
    case kVK_RightCommand:
        return NSEventModifierFlagCommand;
    case kVK_CapsLock:
        return NSEventModifierFlagCapsLock;
    }
    return 0;
}

NSDictionary* GetFullScreenModeOptions()
{
    return [NSDictionary dictionaryWithObjectsAndKeys:
            [NSNumber numberWithBool:NO], NSFullScreenModeAllScreens,
            [NSNumber numberWithInt:NSApplicationPresentationHideDock | NSApplicationPresentationHideMenuBar], NSFullScreenModeApplicationPresentationOptions,
            nil];

}
	
void HandleThermalState()
{
	if( gTriDev )
	{
		switch( [NSProcessInfo processInfo].thermalState )
		{
		case NSProcessInfoThermalStateSerious:
		case NSProcessInfoThermalStateCritical:
			if( !gTriDev->GetThrottling( TriDevice::THERMAL_STATE ) )
			{
				CCP_LOGWARN( "Detected serious/critical machine thermal state. Starting throttling" );
				gTriDev->SetThrottling( TriDevice::THERMAL_STATE, true );
			}
			break;
		default:
			if( gTriDev->GetThrottling( TriDevice::THERMAL_STATE ) )
			{
				CCP_LOGNOTICE( "Detected nominal/fair machine thermal state. Stopping throttling" );
				gTriDev->SetThrottling( TriDevice::THERMAL_STATE, false );
			}
			break;
		}
	}
}

bool DisableScreensaver()
{
	IOPMAssertionID assertionID;
	IOReturn success = IOPMAssertionCreateWithName(
						   kIOPMAssertionTypeNoDisplaySleep,
						   kIOPMAssertionLevelOn,
						   static_cast<CFStringRef>( @"Prevent sleep" ),
						   &assertionID );
	return success == kIOReturnSuccess;
}

void FixUpRedScreen()
{
    if( auto mainWindow = [NSApp mainWindow] )
    {
        [mainWindow setDelegate:nil];
        [mainWindow close];
        @autoreleasepool
        {
            while( true )
            {
                NSEvent* event = [NSApp nextEventMatchingMask:NSEventMaskAny
                                                    untilDate:[NSDate distantPast]
                                                       inMode:NSDefaultRunLoopMode
                                                      dequeue:YES];
                if (event == nil)
                {
                    break;
                }
                [NSApp sendEvent:event];
            }
        }
    }
}

void InitializeApplication()
{
    if( s_applicationInitialized )
    {
        return;
    }

    [CcpApplication sharedApplication];
    [NSApp setDelegate:[CcpApplication sharedApplication]];

    [[NSUserDefaults standardUserDefaults] registerDefaults:@{@"ApplePressAndHoldEnabled":@NO}];

    if (![[NSRunningApplication currentApplication] isFinishedLaunching])
    {
        [NSApp run];
    }
    
    BeOS->RegisterIndispensableTerminationStep( &FixUpRedScreen );

    HandleThermalState();
    [NSNotificationCenter.defaultCenter addObserverForName:@"NSProcessInfoThermalStateDidChangeNotification" object:nil queue:nil usingBlock:^(NSNotification * _Nonnull note) {
        HandleThermalState();
    }];
	
	DisableScreensaver();

    if (@available(macOS 10.15, *))
    {
        [NSNotificationCenter.defaultCenter addObserverForName:GCControllerDidConnectNotification object:nil queue:nil usingBlock:^(NSNotification * _Nonnull note) {
            GCController* controller = note.object;
            CCP_LOG("Game controller connected: %s", controller.vendorName.UTF8String);
        }];

        [NSNotificationCenter.defaultCenter addObserverForName:GCControllerDidDisconnectNotification object:nil queue:nil usingBlock:^(NSNotification * _Nonnull note) {
            GCController* controller = note.object;
            CCP_LOG("Game controller disconnected: %s", controller.vendorName.UTF8String);
        }];
    }

    s_applicationInitialized = true;
}

NSScreen* GetAdapterScreen( uint32_t adapter )
{
    void* monitor;
    if( FAILED( Tr2VideoAdapterInfo::GetAdapterMonitor( adapter , monitor ) ) )
    {
        return nullptr;
    }
    auto displayID = CGDirectDisplayID( reinterpret_cast<uintptr_t>( monitor ) );
	CCP_LOG( "Looking for screen with display ID %u for adapter %u", unsigned( displayID ), unsigned( adapter ) );
    for( id screen in [NSScreen screens] )
    {
        CGDirectDisplayID did = [[[screen deviceDescription] objectForKey:@"NSScreenNumber"] unsignedIntValue];
		CCP_LOG( "Found screen with display ID %u", unsigned( did ) );
        if( did == displayID )
        {
            return screen;
        }
    }
	CCP_LOGWARN( "Could not find screen with display ID %u. Falling back to the main screen", unsigned( displayID ) );
    return [NSScreen mainScreen];
}

uint32_t GetScreenAdapter( NSScreen* screen )
{
    CGDirectDisplayID did = [[[screen deviceDescription] objectForKey:@"NSScreenNumber"] unsignedIntValue];
    uint32_t count = 0;
    Tr2VideoAdapterInfo::GetAdapterCount( count );
    for( uint32_t i = 0; i < count; ++i )
    {
        void* monitor;
        if( FAILED( Tr2VideoAdapterInfo::GetAdapterMonitor( i , monitor ) ) )
        {
            continue;
        }
        auto displayID = CGDirectDisplayID( reinterpret_cast<uintptr_t>( monitor ) );
        if( did == displayID )
        {
            return i;
        }
    }
    return 0;
}

// Helper class for emulating ClipCursor functionality on macOS. For clipping a mouse cursor (i.e.
// constraining it to a rectangle), we need to assume manual control of it (using
// CGAssociateMouseAndMouseCursorPosition function), track imaginary mouse location using
// mouse event movement deltas, and actually move the cursor using CGWarpMouseCursorPosition.
// Last bit: we need to undo our manual cursor movements when interpreting mouse deltas in
// incoming events.
class CursorClipper
{
public:
	CursorClipper( const NSRect& clipScreenRect );
	~CursorClipper();
	
	NSRect GetRect() const;
	NSEvent* ProcessEvent( NSEvent* event );
private:
	struct WarpRec
	{
		CGFloat deltaX;
		CGFloat deltaY;
		NSTimeInterval timestamp;
	};
	
	bool IsMouseEvent( NSEvent* event ) const;
	NSPoint AdjustMouseDeltas( const NSPoint& delta, NSTimeInterval timestamp );
	
	std::vector<WarpRec> m_warps;
	NSPoint m_mouseLocation;
	NSRect m_clipRect;
};

CursorClipper::CursorClipper( const NSRect& clipScreenRect )
:	m_clipRect( clipScreenRect )
{
	m_mouseLocation = [NSEvent mouseLocation];
	CGAssociateMouseAndMouseCursorPosition( false );
}

CursorClipper::~CursorClipper()
{
	CGAssociateMouseAndMouseCursorPosition( true );
}

NSRect CursorClipper::GetRect() const
{
	return m_clipRect;
}

NSEvent* CursorClipper::ProcessEvent( NSEvent* event )
{
	if( !IsMouseEvent( event ) )
	{
		return event;
	}
	
	auto delta = AdjustMouseDeltas( NSMakePoint( event.deltaX, -event.deltaY ), event.timestamp );

	m_mouseLocation.x += delta.x;
	m_mouseLocation.y += delta.y;
	m_mouseLocation.x = std::min( std::max( m_mouseLocation.x, m_clipRect.origin.x ), m_clipRect.origin.x + m_clipRect.size.width );
	m_mouseLocation.y = std::min( std::max( m_mouseLocation.y, m_clipRect.origin.y ), m_clipRect.origin.y + m_clipRect.size.height );

	auto currentLocation = [NSEvent mouseLocation];
	
	CGWarpMouseCursorPosition( CGPointMake( m_mouseLocation.x, NSHeight( NSScreen.screens[0].frame ) - m_mouseLocation.y ) );
	
	WarpRec warpRec = { m_mouseLocation.x - currentLocation.x, m_mouseLocation.y - currentLocation.y, [NSProcessInfo processInfo].systemUptime };
	m_warps.push_back( warpRec );

	auto location = [event.window convertPointFromScreen:m_mouseLocation];
	auto newEvent = [NSEvent mouseEventWithType:event.type
									   location:location
								  modifierFlags:event.modifierFlags
									  timestamp:event.timestamp
								   windowNumber:event.windowNumber
										context:nil
									eventNumber:event.eventNumber
									 clickCount:event.clickCount
									   pressure:event.pressure];
	return newEvent;
}

NSPoint CursorClipper::AdjustMouseDeltas( const NSPoint& delta, NSTimeInterval timestamp )
{
	auto deltaX = delta.x;
	auto deltaY = delta.y;

	size_t consumed = 0;
	for( auto& rec : m_warps )
	{
		if( rec.timestamp < timestamp )
		{
			auto modifiedX = deltaX - rec.deltaX;
			auto modifiedY = deltaY - rec.deltaY;
			// fence in the deltas so they don't change signs!
			deltaX = deltaX == 0.0f || signbit(deltaX) != signbit(modifiedX) ? 0.0f : modifiedX;
			deltaY = deltaY == 0.0f || signbit(deltaY) != signbit(modifiedY) ? 0.0f : modifiedY;
			++consumed;
		}
		else
		{
			break;
		}
	}
	if( consumed > 0 )
	{
		m_warps.erase( begin( m_warps ), begin( m_warps ) + consumed );
	}
	return NSMakePoint( deltaX, deltaY );
}

bool CursorClipper::IsMouseEvent( NSEvent* event ) const
{
	switch( event.type )
	{
	case NSEventTypeMouseMoved:
	case NSEventTypeLeftMouseDragged:
	case NSEventTypeRightMouseDragged:
	case NSEventTypeOtherMouseDragged:
		return true;
	default:
		return false;
	}
}

}

@implementation CcpApplication

CursorClipper* m_cursorClipper = nullptr;

- (void)clipCursor:(NSRect)rect
{
	if( m_cursorClipper )
	{
		if( NSEqualRects( m_cursorClipper->GetRect(), rect ) )
		{
			return;
		}
		delete m_cursorClipper;
		m_cursorClipper = nullptr;
	}
	
	m_cursorClipper = new CursorClipper( rect );
}

- (void)unclipCursor
{
	if( m_cursorClipper )
	{
		delete m_cursorClipper;
		m_cursorClipper = nullptr;
	}
}

- (void)sendEvent:(NSEvent *)event
{
	if( m_cursorClipper )
	{
		event = m_cursorClipper->ProcessEvent( event );
	}
	
    if( [event type] == NSEventTypeKeyUp && ( [event modifierFlags] & NSEventModifierFlagCommand ) )
    {
        [[self keyWindow] sendEvent:event];
    }
	else if( [event type] == NSEventTypeKeyDown && event.keyCode == kVK_Tab && ( [event modifierFlags] & NSEventModifierFlagControl ) )
	{
		[self.keyWindow.contentView keyDown:event];
		[super sendEvent:event];
	}
    else
    {
        [super sendEvent:event];
    }
}

- (void)applicationWillResignActive:(NSNotification *)notification
{
	[self unclipCursor];
}

- (NSApplicationTerminateReply)applicationShouldTerminate:(NSApplication *)sender
{
    s_quitRequested = true;
    return NSTerminateCancel;
}

- (void)applicationWillFinishLaunching:(NSNotification *)notification
{
    CreateMenuBar();
}

- (void)applicationDidFinishLaunching:(NSNotification *)notification
{
    [NSApp setActivationPolicy:NSApplicationActivationPolicyRegular];
    [NSApp stop:nil];

    @autoreleasepool
    {
        NSEvent* event = [NSEvent otherEventWithType:NSEventTypeApplicationDefined
                                            location:NSMakePoint( 0, 0 )
                                       modifierFlags:0
                                           timestamp:0
                                        windowNumber:0
                                             context:nil
                                             subtype:0
                                               data1:0
                                               data2:0];
        [NSApp postEvent:event atStart:YES];
    }
}

@end


@implementation CcpWindow
{
    Tr2MainWindow* m_mainWindow;
    BOOL m_isFullscreen;
    BOOL m_inFullscreenTransition;
}

- (BOOL)canBecomeKeyWindow
{
    return YES;
}

- (BOOL)canBecomeMainWindow
{
    return YES;
}

- (void)initialize:(Tr2MainWindow*)mainWindow
{
    m_mainWindow = mainWindow;
    m_isFullscreen = NO;
}

- (BOOL)isFullscreen
{
    return m_isFullscreen;
}

- (BOOL)isInFullscreenTransition
{
    return m_inFullscreenTransition;
}

- (BOOL)windowShouldClose:(id)sender
{
    return m_mainWindow->OnClose_MacOS() ? YES : NO;
}

- (void)windowWillStartLiveResize:(NSNotification *)notification
{
    m_mainWindow->OnWindowStartedResizing_MacOS();
}

- (void)windowDidEndLiveResize:(NSNotification *)notification
{
	if( !m_inFullscreenTransition )
	{
		m_mainWindow->OnWindowFnishedResizing_MacOS();
	}
}

- (void)windowDidResize:(NSNotification *)notification
{
	if( !m_inFullscreenTransition )
	{
		m_mainWindow->OnWindowResized_MacOS();
	}
}

- (void)windowDidMove:(NSNotification *)notification
{
	if( !m_inFullscreenTransition )
	{
		m_mainWindow->OnWinowMoved_MacOS();
	}
}

- (void)windowDidMiniaturize:(NSNotification *)notification
{
	m_mainWindow->OnWindowResized_MacOS();
}

- (void)windowDidDeminiaturize:(NSNotification *)notification
{
	m_mainWindow->OnWindowResized_MacOS();
}

- (void)windowDidBecomeKey:(NSNotification *)notification
{
	KeyboardHelpers::PlatformKeyChanged( kVK_Shift, false );
	KeyboardHelpers::PlatformKeyChanged( kVK_RightShift, false );
	KeyboardHelpers::PlatformKeyChanged( kVK_Control, false );
	KeyboardHelpers::PlatformKeyChanged( kVK_RightControl, false );
	KeyboardHelpers::PlatformKeyChanged( kVK_Option, false );
	KeyboardHelpers::PlatformKeyChanged( kVK_RightOption, false );
	KeyboardHelpers::PlatformKeyChanged( kVK_Command, false );
	KeyboardHelpers::PlatformKeyChanged( kVK_RightCommand, false );
	auto flags = CGEventSourceFlagsState( kCGEventSourceStateHIDSystemState );
	KeyboardHelpers::PlatformKeyChanged( kVK_CapsLock, ( kCGEventFlagMaskAlphaShift & flags ) != 0 );

    m_mainWindow->OnWindowChangedKey_MacOS( true );
}

- (void)windowDidResignKey:(NSNotification *)notification
{
    m_mainWindow->OnWindowChangedKey_MacOS( false );
}

- (void)windowDidChangeBackingProperties:(NSNotification *)notification
{
    m_mainWindow->OnWindowDidChangeBackingProperties_MacOS();
}

- (void)windowWillExitFullScreen:(NSNotification *)notification
{
    m_inFullscreenTransition = YES;
}

- (void)windowWillEnterFullScreen:(NSNotification *)notification
{
    m_inFullscreenTransition = YES;
}

- (void)windowDidEnterFullScreen:(NSNotification *)notification
{
    m_isFullscreen = YES;
    m_inFullscreenTransition = NO;
    m_mainWindow->OnWindowFullscreenChanged_MacOS( true );
}

- (void)windowDidExitFullScreen:(NSNotification *)notification
{
    m_isFullscreen = NO;
    m_inFullscreenTransition = NO;
    m_mainWindow->OnWindowFullscreenChanged_MacOS( false );
}

- (void)windowDidFailToEnterFullScreen:(NSWindow *)window
{
    m_inFullscreenTransition = NO;
}

- (void)windowDidFailToExitFullScreen:(NSWindow *)window
{
    m_inFullscreenTransition = NO;
}

- (void)windowDidChangeOcclusionState:(NSNotification *)notification
{
	m_mainWindow->OnWindowOcclusionChanged_MacOS();
}

- (NSApplicationPresentationOptions)window:(NSWindow *)window willUseFullScreenPresentationOptions:(NSApplicationPresentationOptions)proposedOptions
{
    return NSApplicationPresentationFullScreen | NSApplicationPresentationHideDock | NSApplicationPresentationHideMenuBar;
}
@end


@implementation CcpContentView

static const NSRange kEmptyRange = {NSNotFound, 0};

- (instancetype)initWithWindow:(Tr2MainWindow*)initWindow
{
    self = [super init];
    if (self != nil)
    {
        m_mainWindow = initWindow;
    }
    self.autoresizingMask = NSViewWidthSizable | NSViewHeightSizable;
    m_trackingArea = nil;
    [[NSNotificationCenter defaultCenter] addObserver:self
                                             selector:@selector(textInputContextKeyboardSelectionDidChange:)
                                                 name:@"NSTextInputContextKeyboardSelectionDidChangeNotification"
                                               object:nil];
    return self;
}

- (BOOL)acceptsFirstMouse:(NSEvent *)event
{
	return YES;
}

- (CALayer *)makeBackingLayer
{
    return [CAMetalLayer layer];
}

- (BOOL)wantsLayer
{
    return YES;
}

- (BOOL)canBecomeKeyView
{
    return YES;
}

- (BOOL)acceptsFirstResponder
{
    return YES;
}

- (void)viewDidChangeBackingProperties
{
    [super viewDidChangeBackingProperties];
    [self updateDrawableSize];
}

- (void)setFrame:(NSRect)frame
{
    [super setFrame:frame];
    [self updateDrawableSize];
}

- (void)updateDrawableSize
{
    if( !self.window )
    {
        return;
    }
    CGFloat scale = self.window.backingScaleFactor;
    CGSize drawableSize = self.bounds.size;
    drawableSize.width *= scale;
    drawableSize.height *= scale;
    self.layer.contentsScale = scale;
    ((CAMetalLayer*)self.layer).drawableSize = drawableSize;
}

- (void)textInputContextKeyboardSelectionDidChange:(NSNotification*)notification
{
    m_mainWindow->OnKeyboardLayoutChange_MacOS();
}

- (NSPoint)eventViewPos:(NSEvent*)event
{
    return [self viewPos:[event locationInWindow]];
}

- (NSPoint)viewPos:(NSPoint)point
{
    auto location = [self convertPoint:point fromView:nil];
    auto height = [self frame].size.height;
    location.y = height - location.y;
    return location;
}

- (void)updateTrackingAreas
{
    [super updateTrackingAreas];
    if( m_trackingArea )
    {
        [self removeTrackingArea: m_trackingArea];
    }
    m_trackingArea = [[NSTrackingArea alloc] initWithRect:[self bounds] options:(NSTrackingCursorUpdate | NSTrackingMouseMoved | NSTrackingActiveAlways | NSTrackingInVisibleRect) owner:self userInfo:nil];
    [self addTrackingArea:m_trackingArea];
}

- (void)cursorUpdate:(NSEvent *)event
{
    m_mainWindow->OnUpdateCursor_MacOS();
}

- (void)mouseDown:(NSEvent *)event
{
    auto location = [self eventViewPos:event];
    m_mainWindow->OnMouseButton_MacOS( 0, true, location.x, location.y );
}

- (void)mouseUp:(NSEvent *)event
{
    auto location = [self eventViewPos:event];
    m_mainWindow->OnMouseButton_MacOS( 0, false, location.x, location.y );
}

- (void)rightMouseDown:(NSEvent *)event
{
    auto location = [self eventViewPos:event];
    m_mainWindow->OnMouseButton_MacOS( 1, true, location.x, location.y );
}

- (void)rightMouseUp:(NSEvent *)event
{
    auto location = [self eventViewPos:event];
    m_mainWindow->OnMouseButton_MacOS( 1, false, location.x, location.y );
}

- (void)otherMouseDown:(NSEvent *)event
{
    auto location = [self eventViewPos:event];
    m_mainWindow->OnMouseButton_MacOS( int32_t( [event buttonNumber] ), true, location.x , location.y );
}

- (void)otherMouseUp:(NSEvent *)event
{
    auto location = [self eventViewPos:event];
    m_mainWindow->OnMouseButton_MacOS( int32_t( [event buttonNumber] ), false, location.x, location.y );
}

- (void)mouseMoved:(NSEvent *)event
{
    auto location = [self eventViewPos:event];
    m_mainWindow->OnMouseMove_MacOS( location.x, location.y );
}

- (void)mouseDragged:(NSEvent *)event
{
    [self mouseMoved:event];
}

- (void)rightMouseDragged:(NSEvent *)event
{
    [self mouseMoved:event];
}

- (void)otherMouseDragged:(NSEvent *)event
{
    [self mouseMoved:event];
}

- (void)scrollWheel:(NSEvent *)event
{
	auto dy = event.scrollingDeltaY;
	if( !event.hasPreciseScrollingDeltas )
	{
		dy *= 40;
		if( dy < 0 && dy > -120 )
		{
			dy = -120;
		}
		if( dy > 0 && dy < 120 )
		{
			dy = 120;
		}
	}
	else
	{
		dy *= 6;
	}
    m_mainWindow->OnMouseWheel_MacOS( float( dy ) );
}

- (void)keyDown:(NSEvent *)event
{
    auto modifierFlags = int32_t( [event modifierFlags] );
    
    KeyboardHelpers::PlatformKey modifiers[] = { kVK_Shift, kVK_Control, kVK_Option, kVK_Command };
    for( auto m : modifiers )
    {
        auto down = ( modifierFlags & KeyCodeToModifierFlag( m ) ) != 0;
        if( KeyboardHelpers::IsPlatformKeyPressed( m ) != down )
        {
            KeyboardHelpers::PlatformKeyChanged( m, down );
            m_mainWindow->OnKey_MacOS( down, m );
        }
    }
    
    int32_t keyCode = event.keyCode;
    if (![event isARepeat])
    {
        KeyboardHelpers::PlatformKeyChanged( keyCode, true );
    }
    if( m_mainWindow->m_imeState_MacOS != Tr2ImeState_MacOS::BLOCKING )
    {
        m_mainWindow->OnKey_MacOS( true, keyCode, event.isARepeat );
    }
    const NSUInteger LEFT_OPT = 524576;
    if( ( event.modifierFlags & NSEventModifierFlagCommand ) == 0 )
    {
        if( ( event.modifierFlags & LEFT_OPT ) != LEFT_OPT )
        {
            if( m_mainWindow->m_imeState_MacOS == Tr2ImeState_MacOS::DISABLED )
            {
                [self insertText:event.characters];
            }
            else
            {
                [self interpretKeyEvents:@[event]];
            }
        }
    }
}

- (void)keyUp:(NSEvent *)event
{
    int32_t keyCode = event.keyCode;
    KeyboardHelpers::PlatformKeyChanged( keyCode, false );
    m_mainWindow->OnKey_MacOS( false, keyCode );
}

- (void)insertText:(id)string
{
    NSStringEncoding encoding;
    if( sizeof( wchar_t ) == 2 )
    {
        encoding = NSUTF16LittleEndianStringEncoding;
    }
    else
    {
        encoding = NSUTF32LittleEndianStringEncoding;
    }
    NSData* data = [string dataUsingEncoding:encoding];
    int32_t length = int32_t( [data length] ) / sizeof( wchar_t );
    auto characters = reinterpret_cast<const wchar_t*>( [data bytes] );
    for( int32_t i = 0; i < length; ++i )
    {
        m_mainWindow->OnChar_MacOS( characters[i] );
    }
}

- (void)flagsChanged:(NSEvent *)event
{
    auto key = int32_t( [event keyCode] );
    auto modifierFlags = int32_t( [event modifierFlags] );
    if (key == 0 && modifierFlags == 256)
    {
        // We just got focus after losing it for a while and no modifier flags are active currently.
        KeyboardHelpers::PlatformKeyChanged( kVK_Shift, false );
        KeyboardHelpers::PlatformKeyChanged( kVK_RightShift, false );
        KeyboardHelpers::PlatformKeyChanged( kVK_Control, false );
        KeyboardHelpers::PlatformKeyChanged( kVK_RightControl, false );
        KeyboardHelpers::PlatformKeyChanged( kVK_Option, false );
        KeyboardHelpers::PlatformKeyChanged( kVK_RightOption, false );
        KeyboardHelpers::PlatformKeyChanged( kVK_Command, false );
        KeyboardHelpers::PlatformKeyChanged( kVK_RightCommand, false );
        KeyboardHelpers::PlatformKeyChanged( kVK_CapsLock, false );
    }
	if( key != kVK_Function )
	{
		KeyboardHelpers::PlatformKeyChanged( key, ( modifierFlags & KeyCodeToModifierFlag( key ) ) != 0 );
		m_mainWindow->OnKey_MacOS( ( modifierFlags & KeyCodeToModifierFlag( key ) ) != 0, key );
	}
}

// IME

void Tr2MainWindow::OnInsertTextIME_MacOS( const std::wstring& string )
{
    m_onInsertTextIME_MacOS.CallVoid( string );
}

void Tr2MainWindow::OnSetMarkedTextIME_MacOS( const std::wstring& string, uint64_t selectedLocation, uint64_t selectedLength)
{
    m_onSetMarkedTextIME_MacOS.CallVoid( string, selectedLocation, selectedLength );
}

void Tr2MainWindow::OnKeyboardLayoutChange_MacOS()
{
    m_onKeyboardLayoutChange_MacOS.CallVoid();
}

std::wstring NSStringToWString( id string )
{
    NSString* str = [string isKindOfClass: [NSAttributedString class]] ? [string string] : string;
    NSStringEncoding pEncode = CFStringConvertEncodingToNSStringEncoding( NSUTF32LittleEndianStringEncoding );
    NSData* pSData = [str dataUsingEncoding: pEncode];
    return std::wstring ( static_cast<const wchar_t*>( [pSData bytes] ), [pSData length] / sizeof( wchar_t ) );
}


// ----------------------------------------------------------------
// NSResponder
// ----------------------------------------------------------------

- (void) insertNewline: (id) sender {
    [self insertText: @"\r\n"];
}

// ----------------------------------------------------------------
// NSTextInputClient (Mac OS X 10.5+)
// ----------------------------------------------------------------
// setMarkedText and insertText are the only methods that seem to
// require proper implementation to get the IME working. The rest
// are stubbed out because they are required by the framework

- (void) doCommandBySelector: (SEL) selector
{
	if( selector == @selector(insertNewline:) )
	{
		[self insertText: @"\r\n"];
	}
	else if( selector == @selector(deleteBackward:) )
	{
		m_mainWindow->OnChar_MacOS( VK_BACK );
	}
}

- (void) unmarkText
{
}

- (BOOL) hasMarkedText
{
    return false;
}

- (NSRange) markedRange
{
    return NSMakeRange(NSNotFound, 1);
}

- (NSRange) selectedRange
{
    return kEmptyRange;
}

- (void) insertText: (id) string replacementRange: (NSRange) replacementRange
{
	m_mainWindow->OnInsertTextIME_MacOS( NSStringToWString( string ) );
}

- (NSUInteger) characterIndexForPoint: (NSPoint) thePoint
{
    return 0;
}

- (NSArray *) validAttributesForMarkedText
{
    return @[];
}

- (void) setMarkedText: (id) string selectedRange: (NSRange) selectedRange replacementRange: (NSRange) replacementRange
{
	m_mainWindow->OnSetMarkedTextIME_MacOS( NSStringToWString( string ), selectedRange.location, selectedRange.length );
}

- (NSAttributedString *) attributedSubstringForProposedRange: (NSRange) aRange actualRange: (NSRangePointer) actualRange
{
	return nullptr;
}

- (NSRect) firstRectForCharacterRange: (NSRange) aRange actualRange: (NSRangePointer) actualRange
{
	return NSZeroRect;
}

- (NSAttributedString *) attributedString
{
	return nullptr;
}

// /ime

- (NSTouchBar *) makeTouchBar {
    m_touchBar = [[TouchBar alloc] initWithWindow:m_mainWindow];
    return [m_touchBar makeFunctionKeyTouchBar];
}
@end


void Tr2MainWindow::SetWindowTitle( const wchar_t* title )
{
    m_title = title;
    if( HasWindow() )
    {
        ::SetWindowTitle( (NSWindow*)GetWindowID(), m_title );
    }
}


bool Tr2MainWindow::HasFocus() const
{
    if( !HasWindow() )
    {
        return false;
    }
    auto window = (NSWindow*)GetWindowID();
    return [window isKeyWindow] && [NSApp isActive];
}

bool Tr2MainWindow::HasWindow() const
{
    return m_hwnd != 0;
}

void Tr2MainWindow::CreateOSWindow( Tr2MainWindowState::State& state )
{
    if( HasWindow() )
    {
        return;
    }
    InitializeApplication();

    auto style = GetWindowStyleMask( state.windowMode );
    NSRect frame = NSMakeRect(state.left, state.top, state.width, state.height);

    CcpWindow* window  = [[CcpWindow alloc] initWithContentRect:frame
                            styleMask:style
                            backing:NSBackingStoreBuffered
                            defer:NO
                            screen:nil
                          ];
    [window initialize:this];
    ::SetWindowTitle( window, m_title );

    [window setCollectionBehavior:NSWindowCollectionBehaviorFullScreenPrimary | NSWindowCollectionBehaviorManaged | NSWindowCollectionBehaviorFullScreenDisallowsTiling];

    auto view = [[CcpContentView alloc] initWithWindow:this];
    [window setContentView:view];

    [window makeKeyAndOrderFront:nil];
    [window makeFirstResponder:[window contentView]];
	
	if( state.windowMode == Tr2WindowMode::WINDOWED )
	{
		window.level = NSNormalWindowLevel;
	}
	else
	{
		window.level = NSMainMenuWindowLevel + 1;
	}

    if( state.windowMode == Tr2WindowMode::FULL_SCREEN )
    {
        auto screen = GetAdapterScreen( state.adapter );
        if( screen != [window screen] )
        {
            [window setFrameOrigin:screen.visibleFrame.origin];
        }
        [window setFrame:screen.visibleFrame display:YES];

        auto factor = [window screen].backingScaleFactor;
        [window setContentMinSize:NSMakeSize( m_minimumSize.width / factor, m_minimumSize.height / factor )];

		[window setDelegate:window];
        [window toggleFullScreen:nullptr];
    }
    else
    {
        // this call will move the window to correct display
        [window setFrameOrigin:frame.origin];
        // after the window is on the correct screen, we can set its size
        auto factor = [window screen].backingScaleFactor;
        [window setContentSize:NSMakeSize( state.width / factor, state.height / factor )];
        [window setContentMinSize:NSMakeSize( m_minimumSize.width / factor, m_minimumSize.height / factor )];
        // and this call will finally set window position to what we requested. isn't that fun
        [window setFrameOrigin:frame.origin];

        if( state.showState == Tr2WindowShowState::MAXIMIZED )
        {
            [window zoom:window];
        }

        auto size = [window contentView].frame.size;
        state.width = uint32_t( size.width * factor );
        state.height = uint32_t( size.height * factor );
        [window setDelegate:window];
    }

    [NSApp activateIgnoringOtherApps:YES];

    // need to flush events, so that the window is fully created after this call
    ProcessMessages();

    // somehow Cocoa manages to fuck up drawable size after attaching the view, so we need to update it here
    [view updateDrawableSize];

    m_hwnd = window;
}

void Tr2MainWindow::DestroyOSWindow()
{
    if( !HasWindow() )
    {
        return;
    }
    auto window = (NSWindow*)GetWindowID();
    [window close];
    m_hwnd = 0;
}

void Tr2MainWindow::AdjustWindow( Tr2MainWindowState::State& state )
{
    if( !HasWindow() )
    {
        return;
    }
    auto window = (CcpWindow*)GetWindowID();

	if( state.windowMode == Tr2WindowMode::WINDOWED )
	{
		window.level = NSNormalWindowLevel;
	}
	else
	{
		window.level = NSMainMenuWindowLevel + 1;
	}

	bool wasFullscreen = [window isFullscreen];//m_state.windowMode == Tr2WindowMode::FULL_SCREEN;
    bool wantsFullscreen = state.windowMode == Tr2WindowMode::FULL_SCREEN;
    if( wantsFullscreen != wasFullscreen )
    {
        if( wantsFullscreen )
        {
            auto screen = GetAdapterScreen( state.adapter );
            if( screen != [window screen] )
            {
                [window setFrameOrigin:screen.visibleFrame.origin];
            }
            [window setStyleMask:GetWindowStyleMask( state.windowMode )];
            [window setFrame:screen.visibleFrame display:YES];
            [window toggleFullScreen:nil];
        }
        else
        {
            [window toggleFullScreen:nil];
            // We need to flush all events here because either the client or OS gets very confused
            // when changing monitors
            ProcessMessages();
            [window setStyleMask:GetWindowStyleMask( state.windowMode )];

            // Move the window to the specified position first, then deal with size because
            // here it may change the monitor and we need its backing scale
            [window setFrameOrigin:NSMakePoint(state.left, state.top)];

            auto factor = [window screen].backingScaleFactor;
            [window setContentSize:NSMakeSize( state.width / factor, state.height / factor )];
            auto size = [window contentView].frame.size;
            state.width = uint32_t( size.width * factor );
            state.height = uint32_t( size.height * factor );

            // Move the window again because its size may have changed
            [window setFrameOrigin:NSMakePoint(state.left, state.top)];
        }
        [window makeKeyWindow];
        [window makeFirstResponder:[window contentView]];
    }
    else if( wantsFullscreen && state.adapter != m_state.adapter )
    {
        [window toggleFullScreen:nil];
        while( [window isFullscreen])
        {
            ProcessMessages();
        }
        auto screen = GetAdapterScreen( state.adapter );
        [window setFrameOrigin:screen.visibleFrame.origin];
        [window setFrame:screen.visibleFrame display:YES];
        [window toggleFullScreen:nil];
    }
    else if( !wantsFullscreen )
    {
        auto newStyle = GetWindowStyleMask( state.windowMode );
        if( [window styleMask] != newStyle )
        {
            [window setStyleMask:newStyle];
        }
        [window setFrameOrigin:NSMakePoint(state.left, state.top)];
        auto factor = [window screen].backingScaleFactor;
        [window setContentSize:NSMakeSize( state.width / factor, state.height / factor )];
		[window setFrameOrigin:NSMakePoint(state.left, state.top)];
    }
}

Tr2MainWindow::Rect Tr2MainWindow::GetDesktopRect() const
{
    NSArray *screenArray = [NSScreen screens];
    uint32_t screenCount = uint32_t( [screenArray count] );

    NSRect desktop = NSZeroRect;

    for( uint32_t index = 0; index < screenCount; index++ )
    {
        NSScreen *screen = [screenArray objectAtIndex: index];
        NSRect screenRect = [screen visibleFrame];
        desktop = NSUnionRect(desktop, screenRect);
    }

    return {
        int32_t( desktop.origin.x ),
        int32_t( desktop.origin.y ),
        int32_t( desktop.origin.x + desktop.size.width ),
        int32_t( desktop.origin.y + desktop.size.height )
    };
}

Tr2MainWindow::Rect Tr2MainWindow::GetMonitorRect( uint32_t adapter ) const
{
    return { 0, 0, 100, 100 };
}

Tr2MainWindow::Size Tr2MainWindow::GetWindowSize( const Size& clientSize, Tr2WindowMode::Type mode ) const
{
    auto size = [NSWindow frameRectForContentRect:NSMakeRect(0, 0, clientSize.width, clientSize.height) styleMask:GetWindowStyleMask(mode)];
    return { uint32_t( size.size.width ), uint32_t( size.size.height ) };
}

Tr2MainWindow::Size Tr2MainWindow::GetClientSize( const Size& windowSize, Tr2WindowMode::Type mode ) const
{
    auto size = [NSWindow contentRectForFrameRect:NSMakeRect(0, 0, windowSize.width, windowSize.height) styleMask:GetWindowStyleMask(mode)];
    return { uint32_t( size.size.width ), uint32_t( size.size.height ) };
}

bool Tr2MainWindow::SupportsFullscreen() const
{
    return true;
}

void Tr2MainWindow::ClipCursor( int32_t left, int32_t top, int32_t right, int32_t bottom )
{
	if( !HasWindow() )
	{
		return;
	}

	id delegate = [NSApp delegate];
	if( ![delegate respondsToSelector:@selector(clipCursor:)] )
	{
		return;
	}

	auto window = (NSWindow*)GetWindowID();
	auto content = window.contentView.frame;
	
	auto l = CGFloat( left ) / m_state.width * content.size.width;
	auto t = content.size.height - CGFloat( top ) / m_state.height * content.size.height;
	auto r = CGFloat( right ) / m_state.width * content.size.width;
	auto b = content.size.height - CGFloat( bottom ) / m_state.height * content.size.height;
	
	auto rect = [window convertRectToScreen:NSMakeRect( l, b, r - l, t - b)];

	[(CcpApplication*)delegate clipCursor:rect];
}

void Tr2MainWindow::UnclipCursor()
{
	id delegate = [NSApp delegate];
	if( ![delegate respondsToSelector:@selector(unclipCursor)] )
	{
		return;
	}

	[(CcpApplication*)delegate unclipCursor];
}

void Tr2MainWindow::SetCursorPos( int32_t x, int32_t y )
{
    if( !HasWindow() )
    {
        return;
    }
    auto window = (NSWindow*)GetWindowID();
    float sx = float( x );
    float sy = float( y );
    auto content = [[window contentView] frame];
    if( m_state.width )
    {
        sx = sx / m_state.width * content.size.width;
    }
    if( m_state.height )
    {
        sy = sy / m_state.height * content.size.height;
    }
    sy = content.size.height - sy;
    NSPoint location = [window convertPointToScreen:NSMakePoint( sx, sy )];
    auto height = CGDisplayBounds(CGMainDisplayID()).size.height;
    CGWarpMouseCursorPosition( CGPointMake( location.x, height - location.y ) );
}

std::pair<int32_t, int32_t> Tr2MainWindow::GetCursorPos() const
{
    if( !HasWindow() )
    {
        return { 0, 0 };
    }
    auto window = (NSWindow*)GetWindowID();
    NSPoint mouseLoc = [NSEvent mouseLocation];
    mouseLoc = [window convertPointFromScreen:mouseLoc];
    auto x = mouseLoc.x;
    auto y = mouseLoc.y;
    auto content = [[window contentView] frame];
    x -= content.origin.x;
    y -= content.origin.y;
    y = content.size.height - y;

    int32_t l  = int32_t( x / content.size.width * m_state.width + 0.5 );
    int32_t t  = int32_t( y / content.size.height * m_state.height + 0.5 );

    return { l, t };
}

bool Tr2MainWindow::IsKeyToggled( uint32_t keyCode ) const
{
    if( keyCode != 0x14 )
    {
        // we only report CapsLock
        return false;
    }
    return ( [NSEvent modifierFlags] & NSEventModifierFlagCapsLock ) != 0;
}

bool Tr2MainWindow::IsKeyPressed( uint32_t keyCode ) const
{
    switch( keyCode )
    {
    case VK_LBUTTON:
        return ( [NSEvent pressedMouseButtons] & 1 ) != 0;
    case VK_RBUTTON:
        return ( [NSEvent pressedMouseButtons] & 2 ) != 0;
    case VK_MBUTTON:
        return ( [NSEvent pressedMouseButtons] & 4 ) != 0;
    case VK_SHIFT:
        return KeyboardHelpers::IsAppKeyPressed( VK_LSHIFT ) || KeyboardHelpers::IsAppKeyPressed( VK_RSHIFT  );
    case VK_CONTROL:
        return KeyboardHelpers::IsAppKeyPressed( VK_LCONTROL ) || KeyboardHelpers::IsAppKeyPressed( VK_RCONTROL );
    case VK_MENU:
        return KeyboardHelpers::IsAppKeyPressed( VK_LMENU ) || KeyboardHelpers::IsAppKeyPressed( VK_RMENU );
	case VK_LWIN:
		return KeyboardHelpers::IsAppKeyPressed( VK_LWIN ) || KeyboardHelpers::IsAppKeyPressed( VK_RWIN );
	default:
        return KeyboardHelpers::IsAppKeyPressed( keyCode );
    }
}

std::string Tr2MainWindow::GetKeyName( uint32_t keyCode ) const
{
    return KeyboardHelpers::GetAppKeyName( keyCode );
}

bool Tr2MainWindow::ProcessMessages()
{
    @autoreleasepool
    {
        while( true )
        {
            NSEvent* event = [NSApp nextEventMatchingMask:NSEventMaskAny
                                                untilDate:[NSDate distantPast]
                                                   inMode:NSDefaultRunLoopMode
                                                  dequeue:YES];
            if (event == nil)
            {
                break;
            }
            [NSApp sendEvent:event];
        }
    }
    return !s_quitRequested;
}


bool Tr2MainWindow::OnClose_MacOS()
{
    bool canClose = true;
    m_onClose.Call( canClose );
    return canClose;
}

void Tr2MainWindow::OnWindowStartedResizing_MacOS()
{
    m_isResizing = true;
}

void Tr2MainWindow::OnWindowFnishedResizing_MacOS()
{
    m_isResizing = false;
    OnWindowResized_MacOS();
}

void Tr2MainWindow::OnWindowResized_MacOS()
{
    if( m_isResizing || m_inSetState || m_state.windowMode == Tr2WindowMode::FULL_SCREEN )
    {
        return;
    }
    auto window = (NSWindow*)GetWindowID();
    auto size = [window contentView].frame.size;

    auto prevWidth = m_state.width;
    auto prevHeight = m_state.height;
    auto newMode = m_state;
    auto factor = [window screen].backingScaleFactor;
    newMode.width = uint32_t( size.width * factor );
    newMode.height = uint32_t( size.height * factor );

    if( [window isZoomed] )
    {
        newMode.showState = Tr2WindowShowState::MAXIMIZED;
    }
    else if( [window isMiniaturized] )
    {
        newMode.showState = Tr2WindowShowState::MINIMIZED;
        newMode.width = prevWidth;
        newMode.height = prevHeight;
    }
    else
    {
        newMode.showState = Tr2WindowShowState::NORMAL;
    }
    SetState( false, newMode );

}

void Tr2MainWindow::OnWinowMoved_MacOS()
{
    if( m_isResizing || m_inSetState )
    {
        return;
    }
    if( m_state.windowMode == Tr2WindowMode::WINDOWED )
    {
        auto window = (NSWindow*)GetWindowID();

        auto height = [[window screen] frame].size.height;
        auto location = [window frame].origin;

        auto newState = m_state;
        newState.left = int32_t( location.x );
        newState.top = int32_t( location.y );
        SetState( false, newState );
    }
}

void Tr2MainWindow::OnWindowChangedKey_MacOS( bool isKey )
{
	auto window = (NSWindow*)GetWindowID();
    if( isKey )
    {
		if( m_state.windowMode != Tr2WindowMode::WINDOWED )
		{
			window.level = NSMainMenuWindowLevel + 1;
		}
        if( gTriDev )
        {
            gTriDev->ApplicationActivated( TriDevice::APP_ACTIVATED );
			gTriDev->SetThrottling( TriDevice::WINDOW_OUT_OF_FOCUS, false );
        }
        g_activeFrametimeMean.SetSource( &g_ccpStatistics_frameTime );
        g_activeFrametimeStdDev.SetSource( &g_ccpStatistics_frameTime );
    }
    else
    {
		window.level = NSNormalWindowLevel;
        if( gTriDev )
        {
            gTriDev->ApplicationActivated( TriDevice::APP_DEACTIVATED );
			gTriDev->SetThrottling( TriDevice::WINDOW_OUT_OF_FOCUS, true );
        }
        g_activeFrametimeMean.SetSource( nullptr );
        g_activeFrametimeStdDev.SetSource( nullptr );
    }
    m_onFocusChange.CallVoid( isKey );
}

void Tr2MainWindow::OnMouseButton_MacOS( int32_t button, bool down, float left, float top )
{
    auto size = [(NSWindow*)GetWindowID() contentView].frame.size;
    int32_t l  = int32_t( left / size.width * m_state.width + 0.5 );
    int32_t t  = int32_t( top / size.height * m_state.height + 0.5 );

    if( down )
    {
        m_onMouseDown.CallVoid( button, l, t );
    }
    else
    {
        m_onMouseUp.CallVoid( button, l, t );
    }
}

void Tr2MainWindow::OnMouseMove_MacOS( float left, float top )
{
    auto size = [(NSWindow*)GetWindowID() contentView].frame.size;
    int32_t l  = int32_t( left / size.width * m_state.width + 0.5 );
    int32_t t  = int32_t( top / size.height * m_state.height + 0.5 );

    m_onMouseMove.CallVoid( l, t );
}

void Tr2MainWindow::OnMouseWheel_MacOS(float delta)
{
    auto intDelta = int32_t( delta );
    if( intDelta != 0 )
    {
        m_onMouseWheel.CallVoid( intDelta );
    }
}

void Tr2MainWindow::OnKey_MacOS( bool isDown, int32_t key, bool repeat )
{
    auto k = KeyboardHelpers::PlatformKeyToAppKey( key );
    if (k == VK_LSHIFT || k == VK_RSHIFT)
    {
        k = VK_SHIFT;
    }
    else if (k == VK_LCONTROL || k == VK_RCONTROL)
    {
        k = VK_CONTROL;
    }
    else if (k == VK_LMENU || k == VK_RMENU)
    {
        k = VK_MENU;
    }
    if( isDown )
    {
        m_onKeyDown.CallVoid( k, repeat ? 0x40000000u : 0u );
    }
    else
    {
        m_onKeyUp.CallVoid( k, 0 );
    }
}

void Tr2MainWindow::OnChar_MacOS( wchar_t charCode )
{
    m_onChar.CallVoid( int32_t( charCode ), 0, false );
}

void Tr2MainWindow::OnWindowDidChangeBackingProperties_MacOS()
{
    if( m_inSetState )
    {
        return;
    }

    auto window = (NSWindow*)GetWindowID();
    auto factor = [window screen].backingScaleFactor;
    auto size = [window contentView].frame.size;
    auto newMode = m_state;
    newMode.width = uint32_t( size.width * factor );
    newMode.height = uint32_t( size.height * factor );
    [window setMinSize:NSMakeSize( m_minimumSize.width / factor, m_minimumSize.height / factor )];
    if( newMode.width < m_minimumSize.width || newMode.height < m_minimumSize.height )
    {
        [window setContentSize:NSMakeSize( std::max( newMode.width, m_minimumSize.width ) * factor, std::max( newMode.height, m_minimumSize.height ) * factor )];
    }
    else
    {
        SetState( false, newMode );
    }
}

void Tr2MainWindow::OnUpdateCursor_MacOS()
{
    if( m_cursor )
    {
        m_cursor->Apply();
    }
}


void Tr2MainWindow::OnTick( Be::Time, Be::Time, void* )
{
    if( !ProcessMessages() )
    {
        BeOS->Terminate();
    }
}

void Tr2MainWindow::SanitizeWindowedResolution( Tr2MainWindowState::State& state ) const
{
    if( state.width == 0 && state.height == 0 )
    {
        // use window size that spans default display, adjust for window frame
        Tr2DisplayModeInfo mi;
        Tr2VideoAdapterInfo::GetAdapterDisplayMode( state.adapter, mi );

        Size size = { mi.width, mi.height };
        size = GetClientSize( size, state.windowMode );

        state.width = size.width;
        state.height = size.height;
    }
    else
    {
        state.width = std::max( m_minimumSize.width, state.width );
        state.height = std::max( m_minimumSize.height, state.height );
    }
}

void Tr2MainWindow::SanitizeWindowPosition( Tr2MainWindowState::State& state ) const
{
    if( state.windowMode == Tr2WindowMode::FIXED_WINDOW )
    {
        // fixed windows are always at the top left corner of the display
        auto screen = GetAdapterScreen( state.adapter );
        auto frame = screen.frame;
        CCP_LOG( "Tr2MainWindow::SanitizeWindowPosition: screen frame position is (%f, %f), size (%f, %f), backing scale factor %f", frame.origin.x, frame.origin.y, frame.size.width, frame.size.height, screen.backingScaleFactor );

        state.left = int32_t( frame.origin.x );
        state.top = int32_t( frame.origin.y + frame.size.height - state.height / screen.backingScaleFactor );
    }
    else
    {
        // window should intersect desktop with some margin
        Size size = { state.width, state.height };
        size = GetWindowSize( size, state.windowMode );

        auto margin = int32_t( std::max( std::min( size.width / 2, size.height / 2 ), 50u ) );

        auto desktop = GetDesktopRect();

        state.left = std::max( state.left, int32_t( desktop.left - size.width + margin ) );
        state.top = std::max( state.top, int32_t( desktop.top - size.height + margin ) );
        state.left = std::min( state.left, int32_t( desktop.right - margin ) );
        state.top = std::min( state.top, int32_t( desktop.bottom - margin ) );
    }
}

Tr2MainWindow::Size Tr2MainWindow::GetLargestWindowSize( uint32_t adapter, Tr2WindowMode::Type mode ) const
{
    if( [NSScreen screensHaveSeparateSpaces] )
    {
        auto screen = GetAdapterScreen( adapter );
        switch( mode )
        {
        case Tr2WindowMode::WINDOWED:
            return { uint32_t( screen.visibleFrame.size.width * screen.backingScaleFactor ), uint32_t( screen.visibleFrame.size.height * screen.backingScaleFactor ) };
        case Tr2WindowMode::FULL_SCREEN:
            {
                auto screenFrame = screen.visibleFrame;
                return { uint32_t( screenFrame.size.width * screen.backingScaleFactor ), uint32_t( screenFrame.size.height * screen.backingScaleFactor ) };
            }
        default:
            {
                auto screenFrame = screen.frame;
                return { uint32_t( screenFrame.size.width * screen.backingScaleFactor ), uint32_t( screenFrame.size.height * screen.backingScaleFactor ) };
            }
        }
    }
    else
    {
        auto desktopRect = GetDesktopRect();
        return { uint32_t( desktopRect.right - desktopRect.left ), uint32_t( desktopRect.bottom - desktopRect.top ) };
    }
}

void Tr2MainWindow::OnWindowFullscreenChanged_MacOS( bool fullscreen )
{
    bool isFullscreen = m_state.windowMode == Tr2WindowMode::FULL_SCREEN;
    auto window = (NSWindow*)GetWindowID();
    if (fullscreen)
    {
        // We have to resize here again because now that the dock and menu bar is hidden
        // we get the real "visible frame"
        auto screen = [window screen];
        [window setFrame:screen.visibleFrame display:YES];
    }
    if( isFullscreen != fullscreen )
    {
        auto screen = [window screen];
        auto newState = m_state;
        newState.windowMode = fullscreen ? Tr2WindowMode::FULL_SCREEN : Tr2WindowMode::WINDOWED;
        newState.adapter = GetScreenAdapter( screen );
        if( fullscreen )
        {
            const auto& stored = m_storedStates[Tr2WindowMode::FULL_SCREEN];
            if( stored.width > 0 && stored.adapter == newState.adapter )
            {
                newState = stored;
            }
            else
            {
                newState.width = uint32_t( screen.frame.size.width * screen.backingScaleFactor );
                newState.height = uint32_t( screen.frame.size.height * screen.backingScaleFactor );
                newState.left = 0;
                newState.top = 0;
            }
        }
        else
        {
            const auto& stored = m_storedStates[Tr2WindowMode::WINDOWED];
            if( stored.width > 0 && stored.adapter == newState.adapter )
            {
                newState = stored;
            }
            else
            {
                newState.width = uint32_t( [window contentView].frame.size.width * screen.backingScaleFactor );
                newState.height = uint32_t( [window contentView].frame.size.height * screen.backingScaleFactor );
                newState.left = int32_t( [window frame].origin.x );
                newState.top = int32_t( [window frame].origin.y );
            }
        }
		auto newStyle = GetWindowStyleMask( newState.windowMode );
		if( fullscreen )
		{
			newStyle |= NSWindowStyleMaskFullScreen;
		}
        if( window.styleMask != newStyle )
        {
            [window setStyleMask:newStyle];
        }
        SetState( false, newState );
    }
    if( fullscreen )
    {
        [window makeKeyWindow];
        [window makeFirstResponder:[window contentView]];
    }
}

void Tr2MainWindow::OnWindowOcclusionChanged_MacOS()
{
	auto window = (NSWindow*)GetWindowID();
	auto visible = ( window.occlusionState & NSWindowOcclusionStateVisible ) != 0;
	CCP_LOG( "Main window ccclusion changed to %s", visible ? "visible" : "hidden" );
	gTriDev->SetThrottling( TriDevice::WINDOW_HIDDEN, !visible );
}

uintptr_t Tr2MainWindow::GetHwnd() const
{
    return uintptr_t( m_hwnd );
}

Tr2WindowHandle Tr2MainWindow::GetOutputWindow() const
{
    return [(NSWindow*)GetWindowID() contentView];
}

#endif
