#pragma once
#ifndef WithWindowFixture_H
#define WithWindowFixture_H

class RenderWindow;

class WithWindow: public ::testing::Test 
{
public:
	static void SetUpTestCase();
	static void TearDownTestCase();

	template <typename T>
	static void RunLoop( T& t )
	{
		extern bool g_interactive;
		if( g_interactive )
		{
			BeginLoopProcessing();
			while( true )
			{
				if( !DoLoopProcessing() )
				{
					return;
				}
				t();
				if( HasFatalFailure() )
				{
					return;
				}
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