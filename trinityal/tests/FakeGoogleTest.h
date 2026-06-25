// Copyright © 2023 CCP ehf.

#pragma once
#ifndef FakeGoogleTest_H
#define FakeGoogleTest_H

#if defined( TRINITY_AL_MOBILE )

#if defined( __ANDROID__ )
#include <android/log.h>
#endif

namespace testing
{


struct TestInfo
{
	const char* name() const
	{
		return m_name.c_str();
	}

	const char* test_case_name() const
	{
		return m_category.c_str();
	}

	std::string m_category;
	std::string m_name;
};

struct Test : public TestInfo
{
	static void SetUpTestCase()
	{
	}
	static void TearDownTestCase()
	{
	}
	virtual void SetUp()
	{
	}
	virtual void TearDown()
	{
	}

	virtual void Run()
	{
	}

	typedef void ( *SetUpTestCase_t )();

	bool m_succeeded;

	SetUpTestCase_t m_setUpTestCase;
	SetUpTestCase_t m_tearDownTestCase;
};

struct UnitTest
{
	static UnitTest* GetInstance()
	{
		static UnitTest instance;
		return &instance;
	}

	static Test*& GetCurrentTest()
	{
		static Test* test = nullptr;
		return test;
	}

	TestInfo* current_test_info()
	{
		return GetCurrentTest();
	}
};

inline void InitGoogleTest( int* argc, char** argv )
{
}

namespace internal
{

struct strcomp
{
	bool operator()( const std::string& a, const std::string& b ) const
	{
		return a.compare( b ) < 0;
	}
};

inline std::map<std::string, std::vector<Test*>, strcomp>& GetAllTests()
{
	static std::map<std::string, std::vector<Test*>, strcomp> allTests;
	return allTests;
}

inline void AddTest( const char* parent, Test* newTest )
{
	GetAllTests()[parent].push_back( newTest );
}

#if defined( __ANDROID__ )
#define PrintMessage( ... ) __android_log_print( ANDROID_LOG_ERROR, "TrinityALTest", __VA_ARGS__ )
#else
#define PrintMessage( ... ) printf( __VA_ARGS__ )
#endif

}

}

inline int RUN_ALL_TESTS()
{
	printf( "\n\n\n" );
	auto& all = testing::internal::GetAllTests();
	int totalSucceeded = 0;
	int totalFailed = 0;
	for( auto it = std::begin( all ); it != std::end( all ); ++it )
	{
		if( !it->second.empty() )
		{
			( *it->second.front()->m_setUpTestCase )();
			for( auto jt = std::begin( it->second ); jt != std::end( it->second ); ++jt )
			{
				PrintMessage(
					"%s %s\t\t\tRUN\n--------------------------------------------------------\n",
					( *jt )->m_category.c_str(),
					( *jt )->m_name.c_str() );
				::testing::UnitTest::GetCurrentTest() = *jt;
				( *jt )->SetUp();
				( *jt )->m_succeeded = true;
				( *jt )->Run();
				( *jt )->TearDown();
				::testing::UnitTest::GetCurrentTest() = nullptr;
				if( ( *jt )->m_succeeded )
				{
					totalSucceeded++;
				}
				else
				{
					totalFailed++;
				}
				PrintMessage(
					"%s %s\t\t\t%s\n--------------------------------------------------------\n",
					( *jt )->m_category.c_str(),
					( *jt )->m_name.c_str(),
					( *jt )->m_succeeded ? "SUCCESS" : "FAIL" );
			}
			( *it->second.front()->m_tearDownTestCase )();
		}
	}
	PrintMessage(
		"\n\n\nRun total %i tests; %i succeeded; %i failed\n\n\n",
		totalSucceeded + totalFailed,
		totalSucceeded,
		totalFailed );
	return 0;
}

#define TEST_( category, name, parent )                        \
	struct Test_##category##name : public parent               \
	{                                                          \
		void Run();                                            \
	};                                                         \
	struct Test_##category##name##_Factory                     \
	{                                                          \
		Test_##category##name##_Factory()                      \
		{                                                      \
			auto p = new Test_##category##name();              \
			p->m_category = #category;                         \
			p->m_name = #name;                                 \
			p->m_setUpTestCase = &parent::SetUpTestCase;       \
			p->m_tearDownTestCase = &parent::TearDownTestCase; \
			testing::internal::AddTest( #category, p );        \
		}                                                      \
	} s_Test_##category##name##_Factory;                       \
	void Test_##category##name::Run()

#define TEST( category, name ) TEST_( category, name, ::testing::Test )
#define TEST_F( parent, name ) TEST_( parent, name, parent )

inline void FailCurrentTest()
{
	if( ::testing::UnitTest::GetCurrentTest() )
	{
		::testing::UnitTest::GetCurrentTest()->m_succeeded = false;
	}
}

#define ASSERT_EQ( val1, val2 )                                                     \
	{                                                                               \
		if( ( val1 ) != ( val2 ) )                                                  \
		{                                                                           \
			FailCurrentTest();                                                      \
			PrintMessage( "%s (%i): expected val1 == val2\n", __FILE__, __LINE__ ); \
			return;                                                                 \
		}                                                                           \
	};
#define ASSERT_NE( val1, val2 )                                                     \
	{                                                                               \
		if( ( val1 ) == ( val2 ) )                                                  \
		{                                                                           \
			FailCurrentTest();                                                      \
			PrintMessage( "%s (%i): expected val1 != val2\n", __FILE__, __LINE__ ); \
			return;                                                                 \
		}                                                                           \
	};
#define ASSERT_TRUE( val1 )                                                         \
	{                                                                               \
		if( !( val1 ) )                                                             \
		{                                                                           \
			FailCurrentTest();                                                      \
			PrintMessage( "%s (%i): expected val1 == true\n", __FILE__, __LINE__ ); \
			return;                                                                 \
		}                                                                           \
	};
#define ASSERT_FALSE( val1 )                                                         \
	{                                                                                \
		if( ( val1 ) )                                                               \
		{                                                                            \
			FailCurrentTest();                                                       \
			PrintMessage( "%s (%i): expected val1 == false\n", __FILE__, __LINE__ ); \
			return;                                                                  \
		}                                                                            \
	};

#define EXPECT_EQ( val1, val2 )                                                     \
	{                                                                               \
		if( ( val1 ) != ( val2 ) )                                                  \
		{                                                                           \
			FailCurrentTest();                                                      \
			PrintMessage( "%s (%i): expected val1 == val2\n", __FILE__, __LINE__ ); \
		}                                                                           \
	};
#define EXPECT_NE( val1, val2 )                                                     \
	{                                                                               \
		if( ( val1 ) == ( val2 ) )                                                  \
		{                                                                           \
			FailCurrentTest();                                                      \
			PrintMessage( "%s (%i): expected val1 == val2\n", __FILE__, __LINE__ ); \
		}                                                                           \
	};
#define EXPECT_GT( val1, val2 )                                                    \
	{                                                                              \
		if( ( val1 ) <= ( val2 ) )                                                 \
		{                                                                          \
			FailCurrentTest();                                                     \
			PrintMessage( "%s (%i): expected val1 > val2\n", __FILE__, __LINE__ ); \
		}                                                                          \
	};
#define EXPECT_GE( val1, val2 )                                                     \
	{                                                                               \
		if( ( val1 ) < ( val2 ) )                                                   \
		{                                                                           \
			FailCurrentTest();                                                      \
			PrintMessage( "%s (%i): expected val1 >= val2\n", __FILE__, __LINE__ ); \
		}                                                                           \
	};
#define EXPECT_LE( val1, val2 )                                                     \
	{                                                                               \
		if( ( val1 ) > ( val2 ) )                                                   \
		{                                                                           \
			FailCurrentTest();                                                      \
			PrintMessage( "%s (%i): expected val1 <= val2\n", __FILE__, __LINE__ ); \
		}                                                                           \
	};
#define EXPECT_TRUE( val )                                                             \
	{                                                                                  \
		if( !( val ) )                                                                 \
		{                                                                              \
			FailCurrentTest();                                                         \
			PrintMessage( "%s (%i): expected true, got false\n", __FILE__, __LINE__ ); \
		}                                                                              \
	};
#define EXPECT_FALSE( val )                                                            \
	{                                                                                  \
		if( val )                                                                      \
		{                                                                              \
			FailCurrentTest();                                                         \
			PrintMessage( "%s (%i): expected false, got true\n", __FILE__, __LINE__ ); \
		}                                                                              \
	};

#define ASSERT_HRESULT_SUCCEEDED( hr )                                                                                  \
	{                                                                                                                   \
		HRESULT ___hr = (HRESULT)( hr );                                                                                \
		if( ___hr < 0 )                                                                                                 \
		{                                                                                                               \
			FailCurrentTest();                                                                                          \
			PrintMessage( "%s (%i): expected SUCCEEDED HRESULT, got FAILED (%i)\n", __FILE__, __LINE__, int( ___hr ) ); \
			return;                                                                                                     \
		}                                                                                                               \
	};
#define ASSERT_HRESULT_FAILED( hr )                                                                                     \
	{                                                                                                                   \
		HRESULT ___hr = (HRESULT)( hr );                                                                                \
		if( ___hr >= 0 )                                                                                                \
		{                                                                                                               \
			FailCurrentTest();                                                                                          \
			PrintMessage( "%s (%i): expected FAILED HRESULT, got SUCCEEDED (%i)\n", __FILE__, __LINE__, int( ___hr ) ); \
			return;                                                                                                     \
		}                                                                                                               \
	};

#endif

#endif