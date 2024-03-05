//
// Created by Hrafn Jökull Geirsson on 15.12.2020.
//

#if __APPLE__
#import "TouchBar.h"
#import "Scancodes.h"


@implementation TouchBar
{
    Tr2MainWindow * m_mainWindow;
}

- (id) initWithWindow: (Tr2MainWindow *) pMainWindow
{
    self = [super init];
    if(self)
    {
        m_mainWindow = pMainWindow;
    }
    return self;
}

- (NSTouchBar *) makeFunctionKeyTouchBar
{
    NSTouchBar* bar = [[NSTouchBar alloc] init];

    NSCustomTouchBarItem* itemF1 = [self addToolBarButton:@"F1" itemIdentifier:@"F1CustomTouchBarItem" action:@selector(pressF1)];
    NSCustomTouchBarItem* itemF2 = [self addToolBarButton:@"F2" itemIdentifier:@"F2CustomTouchBarItem" action:@selector(pressF2)];
    NSCustomTouchBarItem* itemF3 = [self addToolBarButton:@"F3" itemIdentifier:@"F3CustomTouchBarItem" action:@selector(pressF3)];
    NSCustomTouchBarItem* itemF4 = [self addToolBarButton:@"F4" itemIdentifier:@"F4CustomTouchBarItem" action:@selector(pressF4)];

    NSCustomTouchBarItem* itemF5 = [self addToolBarButton:@"F5" itemIdentifier:@"F5CustomTouchBarItem" action:@selector(pressF5)];
    NSCustomTouchBarItem* itemF6 = [self addToolBarButton:@"F6" itemIdentifier:@"F6CustomTouchBarItem" action:@selector(pressF6)];
    NSCustomTouchBarItem* itemF7 = [self addToolBarButton:@"F7" itemIdentifier:@"F7CustomTouchBarItem" action:@selector(pressF7)];
    NSCustomTouchBarItem* itemF8 = [self addToolBarButton:@"F8" itemIdentifier:@"F8CustomTouchBarItem" action:@selector(pressF8)];

    NSCustomTouchBarItem* itemF9 = [self addToolBarButton:@"F9" itemIdentifier:@"F9CustomTouchBarItem" action:@selector(pressF9)];
    NSCustomTouchBarItem* itemF10 = [self addToolBarButton:@"F10" itemIdentifier:@"F10CustomTouchBarItem" action:@selector(pressF10)];
    NSCustomTouchBarItem* itemF11 = [self addToolBarButton:@"F11" itemIdentifier:@"F11CustomTouchBarItem" action:@selector(pressF11)];
    NSCustomTouchBarItem* itemF12 = [self addToolBarButton:@"F12" itemIdentifier:@"F12CustomTouchBarItem" action:@selector(pressF12)];

    bar.templateItems = [NSSet setWithArray:@[
            itemF1, itemF2, itemF3, itemF4,
            itemF5, itemF6, itemF7, itemF8,
            itemF9, itemF10, itemF11, itemF12,
    ]];

    bar.defaultItemIdentifiers = @[
            @"F1CustomTouchBarItem",
            @"F2CustomTouchBarItem",
            @"F3CustomTouchBarItem",
            @"F4CustomTouchBarItem",
            @"F5CustomTouchBarItem",
            @"F6CustomTouchBarItem",
            @"F7CustomTouchBarItem",
            @"F8CustomTouchBarItem",
            @"F9CustomTouchBarItem",
            @"F10CustomTouchBarItem",
            @"F11CustomTouchBarItem",
            @"F12CustomTouchBarItem",
    ];

    return bar;
}

- (NSCustomTouchBarItem *) addToolBarButton: (NSString *) title
                             itemIdentifier: (NSString *) itemIdentifier
                                     action: (SEL) action
{
    NSCustomTouchBarItem* item = [[NSCustomTouchBarItem alloc] initWithIdentifier:itemIdentifier];
    NSButton* b = [NSButton buttonWithTitle:title target:self action:action];
    item.view = b;
    return item;
}

- (void) pressKey:(int32_t) key
{
    m_mainWindow->OnKey_MacOS(true, key);
    m_mainWindow->OnKey_MacOS(false, key);
}

- (void) pressF1
{
    [self pressKey: kVK_F1];
}

- (void) pressF2
{
    [self pressKey: kVK_F2];
}

- (void) pressF3
{
    [self pressKey: kVK_F3];
}

- (void) pressF4
{
    [self pressKey: kVK_F4];
}

- (void) pressF5
{
    [self pressKey: kVK_F5];
}

- (void) pressF6
{
    [self pressKey: kVK_F6];
}

- (void) pressF7
{
    [self pressKey: kVK_F7];
}

- (void) pressF8
{
    [self pressKey: kVK_F8];
}

- (void) pressF9
{
    [self pressKey: kVK_F9];
}

- (void) pressF10
{
    [self pressKey: kVK_F10];
}

- (void) pressF11
{
    [self pressKey: kVK_F11];
}

- (void) pressF12
{
    [self pressKey: kVK_F12];
}

@end

#endif