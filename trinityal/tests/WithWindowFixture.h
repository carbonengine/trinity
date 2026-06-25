// Copyright © 2023 CCP ehf.

#pragma once
#ifndef WithWindowFixture_H
#define WithWindowFixture_H

class RenderWindow;

class WithWindow : public ::testing::Test
{
public:
	static void SetUpTestCase();
	static void TearDownTestCase();

	template <typename T> static void RunLoop( T& t )
	{
		extern bool g_interactive;
		if( g_interactive )
		{
			BeginLoopProcessing();
			while( true )
			{
#if __OBJC__
				@autoreleasepool
				{
#endif
					if( !DoLoopProcessing() )
					{
						return;
					}
					t();
					if( HasFatalFailure() )
					{
						return;
					}
#if __OBJC__
				}
#endif
			}
		}
		else
		{
			t();
		}
	}

	static Tr2WindowHandle GetWindowHandle();
	static RenderWindow* GetWindow();

private:
	static void BeginLoopProcessing();
	static bool DoLoopProcessing();
};

#endif
