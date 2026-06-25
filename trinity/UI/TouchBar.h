// Copyright © 2023 CCP ehf.

//
// Created by Hrafn Jökull Geirsson on 15.12.2020.
//

#pragma once

#if __APPLE__
#import <Cocoa/Cocoa.h>
#import "Tr2MainWindow.h"

// On some versions of the MacBook Pro(TM) there is a little row of touchscreen where
// the F1-F12 keys ought to be. This is used to create a function-key toolbar which
// shows as many F-keys as the users touch-bar has room for.
// There is a "Control Strip" on the right that has some standard stuff like Siri,
// that we have no control over from the application perspective.
@interface TouchBar : NSObject
- (id)initWithWindow:(Tr2MainWindow*)pMainWindow;
- (NSTouchBar*)makeFunctionKeyTouchBar;
@end

#endif // __APPLE__
