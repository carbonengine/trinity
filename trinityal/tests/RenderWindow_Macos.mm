// Copyright © 2023 CCP ehf.

#include "StdAfx.h"
#if defined( __APPLE__ ) && TRINITY_PLATFORM != TRINITY_STUB

#include "RenderWindow.h"
#import <Cocoa/Cocoa.h>
#import <QuartzCore/QuartzCore.h>

bool s_keyPressed = false;

@interface TrinityALTestWindow : NSWindow
@end

@implementation TrinityALTestWindow

- (BOOL)canBecomeKeyWindow
{
	return YES;
}

- (BOOL)canBecomeMainWindow
{
	return YES;
}

@end

@interface TrinityALTestContentView : NSView
@end

@implementation TrinityALTestContentView

- (CALayer*)makeBackingLayer
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

- (void)keyDown:(NSEvent*)event
{
	s_keyPressed = true;
}

- (void)mouseDown:(NSEvent*)event
{
	s_keyPressed = true;
}

@end


RenderWindow::RenderWindow( uint32_t width, uint32_t height )
{
	NSRect frame = NSMakeRect( 0, 0, width, height );
	TrinityALTestWindow* window =
		[[TrinityALTestWindow alloc] initWithContentRect:frame
											   styleMask:NSWindowStyleMaskTitled | NSWindowStyleMaskClosable
												 backing:NSBackingStoreBuffered
												   defer:NO];

	TrinityALTestContentView* view = [[TrinityALTestContentView alloc] init];
	[window setContentView:view];

	// Explicitly set this to force creation of backing layer (makeBackingLayer) for our view.
	// By default backing layer is created only when the window is shown, which is not
	// the default behavior for TrinityALTest.
	view.wantsLayer = YES;

	[window makeKeyAndOrderFront:nil];
	m_handle = window;
}

RenderWindow::~RenderWindow()
{
	if( m_handle )
	{
		[(TrinityALTestWindow*)m_handle close];
	}
	m_handle = nullptr;
}

uint32_t RenderWindow::GetClientWidth() const
{
	auto size = [(TrinityALTestWindow*)m_handle contentView].frame.size;
	return uint32_t( size.width );
}

uint32_t RenderWindow::GetClientHeight() const
{
	auto size = [(TrinityALTestWindow*)m_handle contentView].frame.size;
	return uint32_t( size.height );
}

bool RenderWindow::Resize( uint32_t width, uint32_t height )
{
	[(TrinityALTestWindow*)m_handle setContentSize:NSMakeSize( width, height )];
	return true;
}

Tr2WindowHandle RenderWindow::GetHandle() const
{
	return [(NSWindow*)m_handle contentView];
}


#endif
