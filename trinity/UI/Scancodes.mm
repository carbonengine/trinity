// Functions in this file deal with keyboard layouts on macOS. We need to be able to convert between physical
// key codes (as sent in Cocoa keyboard messages) and "virtual" key codes as dictated by keyboard layouts, so
// that, for example "select all" Cmd+A command on French AZERTY keyboard would use the phycial Q button.
// We also need reversed lookups from a virtual key code to the physical one (for "is key pressed" queries for
// example). And lastly we need to be able to figure out labels for keys to show them in UI.
// In order to do all these interesting things we have to use old well documented Carbon functions like
// TISCopyCurrentKeyboardInputSource and UCKeyTranslate because Cocoa does not provide enough functionality.
// Now, Carbon and blue have name conflicts, so we can't include Carbon.h along side StdAfx.h, hence all this
// code is in a separate file set up to skip using PCH.
#if __APPLE__
#import <Carbon/Carbon.h>
#include <string>
#include <cstdint>

namespace
{

// Characters resulting from key presses for ANSI keyboard, indexed by kVK_ANSI_... constants
static const char s_ansiChars[][2] = {
	{ 'a', 'A' }, // kVK_ANSI_A
	{ 's', 'S' }, // kVK_ANSI_S
	{ 'd', 'D' }, // kVK_ANSI_D
	{ 'f', 'F' }, // kVK_ANSI_F
	{ 'h', 'H' }, // kVK_ANSI_H
	{ 'g', 'G' }, // kVK_ANSI_G
	{ 'z', 'Z' }, // kVK_ANSI_Z
	{ 'x', 'X' }, // kVK_ANSI_X
	{ 'c', 'C' }, // kVK_ANSI_C
	{ 'v', 'V' }, // kVK_ANSI_V
	{ 0, 0 },
	{ 'b', 'B' }, // kVK_ANSI_B
	{ 'q', 'Q' }, // kVK_ANSI_Q
	{ 'w', 'W' }, // kVK_ANSI_W
	{ 'e', 'E' }, // kVK_ANSI_E
	{ 'r', 'R' }, // kVK_ANSI_R
	{ 'y', 'Y' }, // kVK_ANSI_Y
	{ 't', 'T' }, // kVK_ANSI_T
	{ '1', '!' }, // kVK_ANSI_1
	{ '2', '@' }, // kVK_ANSI_2
	{ '3', '#' }, // kVK_ANSI_3
	{ '4', '$' }, // kVK_ANSI_4
	{ '5', '%' }, // kVK_ANSI_6
	{ '6', '^' }, // kVK_ANSI_5
	{ '=', '+' }, // kVK_ANSI_Equal
	{ '9', '(' }, // kVK_ANSI_9
	{ '7', '&' }, // kVK_ANSI_7
	{ '-', '_' }, // kVK_ANSI_Minus
	{ '8', '*' }, // kVK_ANSI_8
	{ '0', ')' }, // kVK_ANSI_0
	{ ']', '}' }, // kVK_ANSI_RightBracket
	{ 'o', 'O' }, // kVK_ANSI_O
	{ 'u', 'U' }, // kVK_ANSI_U
	{ '[', '{' }, // kVK_ANSI_LeftBracket
	{ 'i', 'I' }, // kVK_ANSI_I
	{ 'p', 'P' }, // kVK_ANSI_P
	{ 0, 0 },
	{ 'l', 'L' }, // kVK_ANSI_L
	{ 'j', 'J' }, // kVK_ANSI_J
	{ '\'', '\"' }, // kVK_ANSI_Quote
	{ 'k', 'K' }, // kVK_ANSI_K
	{ ';', ':' }, // kVK_ANSI_Semicolon
	{ '\\', '|' }, // kVK_ANSI_Backslash
	{ ',', '<' }, // kVK_ANSI_Comma
	{ '/', '?' }, // kVK_ANSI_Slash
	{ 'n', 'N' }, // kVK_ANSI_N
	{ 'm', 'M' }, // kVK_ANSI_M
	{ '.', '>' }, // kVK_ANSI_Period
	{ 0, 0 },
	{ 0, 0 },
	{ '`', '~' }, // kVK_ANSI_Grave
};
	
class KeyReversedLookup
{
public:
	KeyReversedLookup( bool shift )
	{
		memset( m_keyForChar, 0xff, sizeof( m_keyForChar ) );
		for( int i = 0; i < 0x32; ++i )
		{
			auto c = s_ansiChars[i][shift ? 1 : 0];
			if( c )
			{
				m_keyForChar[int( c )] = i;
			}
		}
	}
	
	uint32_t operator[]( char c ) const
	{
		return m_keyForChar[int( (uint8_t)c )];
	}
private:
	uint32_t m_keyForChar[256];
};
	
static KeyReversedLookup s_ansiKeyForChar = KeyReversedLookup( false );

// A list of ANSI key codes - the ones that can be re-assigned in the keyboard layout
const CGKeyCode s_ansiKeys[] = {
	kVK_ANSI_A,
	kVK_ANSI_S,
	kVK_ANSI_D,
	kVK_ANSI_F,
	kVK_ANSI_H,
	kVK_ANSI_G,
	kVK_ANSI_Z,
	kVK_ANSI_X,
	kVK_ANSI_C,
	kVK_ANSI_V,
	kVK_ANSI_B,
	kVK_ANSI_Q,
	kVK_ANSI_W,
	kVK_ANSI_E,
	kVK_ANSI_R,
	kVK_ANSI_Y,
	kVK_ANSI_T,
	kVK_ANSI_1,
	kVK_ANSI_2,
	kVK_ANSI_3,
	kVK_ANSI_4,
	kVK_ANSI_6,
	kVK_ANSI_5,
	kVK_ANSI_Equal,
	kVK_ANSI_9,
	kVK_ANSI_7,
	kVK_ANSI_Minus,
	kVK_ANSI_8,
	kVK_ANSI_0,
	kVK_ANSI_RightBracket,
	kVK_ANSI_O,
	kVK_ANSI_U,
	kVK_ANSI_LeftBracket,
	kVK_ANSI_I,
	kVK_ANSI_P,
	kVK_ANSI_L,
	kVK_ANSI_J,
	kVK_ANSI_Quote,
	kVK_ANSI_K,
	kVK_ANSI_Semicolon,
	kVK_ANSI_Backslash,
	kVK_ANSI_Comma,
	kVK_ANSI_Slash,
	kVK_ANSI_N,
	kVK_ANSI_M,
	kVK_ANSI_Period,
	kVK_ANSI_Grave,

    10 // OEM 102
};

CGKeyCode s_physicalToVirtual[0xff];
CGKeyCode s_virtualToPhysical[0xff];
char s_keyLabels[0xff];
TISInputSourceRef s_activeKeyboard = nullptr;

void ProcessKeyboardLayout()
{
	TISInputSourceRef currentKeyboard = TISCopyCurrentKeyboardInputSource();
	if( s_activeKeyboard && CFEqual( currentKeyboard, s_activeKeyboard ) )
	{
		CFRelease( currentKeyboard );
		return;
	}
	
	if( s_activeKeyboard )
	{
		CFRelease( s_activeKeyboard );
		s_activeKeyboard = nullptr;
	}
	s_activeKeyboard = currentKeyboard;
	
	for( int keyCode = 0; keyCode < 0xff; ++keyCode )
	{
		s_physicalToVirtual[keyCode] = CGKeyCode( keyCode );
		s_virtualToPhysical[keyCode] = CGKeyCode( keyCode );
		s_keyLabels[keyCode] = 0;
	}
	
	if( CFDataRef layoutData = (CFDataRef)TISGetInputSourceProperty( currentKeyboard, kTISPropertyUnicodeKeyLayoutData ) )
	{
		const UCKeyboardLayout *keyboardLayout = (const UCKeyboardLayout *)CFDataGetBytePtr( layoutData );
		UInt32 keysDown = 0;
		UniChar chars[4];
		UniCharCount realLength;
		
		for( auto keyCode : s_ansiKeys )
		{
			// For each of re-assignable keys, we query it's label (what would account for the
			// keyboard layout. Given this label, we find the key in ANSI layout that corresponds to
			// that label. This gives us the corresponding "virtual" key for the physical key.
			UCKeyTranslate(keyboardLayout,
						   keyCode,
						   kUCKeyActionDisplay,
						   0,
						   LMGetKbdType(),
						   kUCKeyTranslateNoDeadKeysBit,
						   &keysDown,
						   sizeof( chars ) / sizeof( chars[0] ),
						   &realLength,
						   chars);
			if( chars[0] > 31 && chars[0] < 256 )
			{
				uint32_t key = s_ansiKeyForChar[char( chars[0] )];
				if( key < 0xff )
				{
					s_physicalToVirtual[keyCode] = CGKeyCode( key );
					s_virtualToPhysical[key] = keyCode;
				}
				s_keyLabels[keyCode] = toupper( char( chars[0] ) );
			}
		}
	}
}

}

namespace KeyboardHelpers
{

CGKeyCode ApplyKeyboardLayout( CGKeyCode keyCode )
{
	ProcessKeyboardLayout();
	return s_physicalToVirtual[keyCode];
}

CGKeyCode UnapplyKeyboardLayout( CGKeyCode keyCode )
{
	ProcessKeyboardLayout();
	return s_virtualToPhysical[keyCode];
}
	
std::string CreateStringForKey( CGKeyCode keyCode )
{
	ProcessKeyboardLayout();
	if( s_keyLabels[keyCode] )
	{
		return std::string( 1, s_keyLabels[keyCode] );
	}
	return "";
}

}
#endif
