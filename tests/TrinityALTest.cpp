#include "StdAfx.h"
#include "WithWindowFixture.h"
#include "WithRenderContextFixture.h"
#include "WithValidRenderContextFixture.h"
#include "TrinityAL/Tr2DriverUtilities.h"

// Needed by CcpCore
const char* g_moduleName = "TrinityALTest";

// Needed by TrinityAL
bool g_usingEXDevice = false;
bool g_useManagedDX9Buffers = true;
std::vector<void*> g_D3DCreatedHeaps;
bool IsTransgaming() { return false; }

// Interactive test flag (set by --interactive option)
bool g_interactive = false;
// Make screenshots for the first frame of interactive tests (set by --screenshots option)
bool g_makeScreenShots = false;
// Folder to save screenshots (set by --screenshotdir option)
const char* g_screenshotFolder = ".";



#ifdef __ORBIS__


#include <stdlib.h>
#include <mspace.h>
#include <kernel.h>

extern "C"
{

#define HEAP_SIZE (512 * 1024 * 1024)

static SceLibcMspace s_mspace;
static off_t s_memStart;
static size_t s_memLength = HEAP_SIZE;
static size_t s_memAlign = 16 * 1024;

int user_malloc_init(void);
int user_malloc_finalize(void);
void *user_malloc(size_t size);
void user_free(void *ptr);
void *user_calloc(size_t nelem, size_t size);
void *user_realloc(void *ptr, size_t size);
void *user_memalign(size_t boundary, size_t size);
int user_posix_memalign(void **ptr, size_t boundary, size_t size);
void *user_reallocalign(void *ptr, size_t size, size_t boundary);
int user_malloc_stats(SceLibcMallocManagedSize *mmsize);
int user_malloc_stats_fast(SceLibcMallocManagedSize *mmsize);
size_t user_malloc_usable_size(void *ptr);

//E Replace _malloc_init function.
//J _malloc_init 関数と置き換わる
int user_malloc_init(void)
{
	int res;
	void *addr;
	
	//E Allocate direct memory
	//J ダイレクトメモリを割り当てる
	res = sceKernelAllocateDirectMemory(0, SCE_KERNEL_MAIN_DMEM_SIZE, s_memLength, s_memAlign, SCE_KERNEL_WB_ONION, &s_memStart);
	if (res < 0) {
		//E Error handling
		//J エラー処理
		return 1;
	}

	addr = NULL;
	//E Map direct memory to the process address space
	//J ダイレクトメモリをプロセスアドレス空間にマップする
	res = sceKernelMapDirectMemory(&addr, HEAP_SIZE, SCE_KERNEL_PROT_CPU_READ | SCE_KERNEL_PROT_CPU_WRITE, 0, s_memStart, s_memAlign);
	if (res < 0) {
		//E Error handling
		//J エラー処理
		return 1;
	}

	//E Generate mspace
	//J mspace を生成する
	s_mspace = sceLibcMspaceCreate("User Malloc", addr, HEAP_SIZE, 0);
	if (s_mspace == NULL) {
		//E Error handling
		//J エラー処理
		return 1;
	}

	return 0;
}

//E Replace _malloc_finalize function.
//J _malloc_finalize 関数と置き換わる
int user_malloc_finalize(void)
{
	int res;

	if (s_mspace != NULL) {
		//E Free mspace
		//J mspace を解放する
		res = sceLibcMspaceDestroy(s_mspace);
		if (res != 0) {
			//E Error handling
			//J エラー処理
			return 1;
		}
	}

	//E Release direct memory
	//J ダイレクトメモリを解放する
	res = sceKernelReleaseDirectMemory(s_memStart, s_memLength);
	if (res < 0) {
		//E Error handling
		//J エラー処理
		return 1;
	}

	return 0;
}

//E Replace malloc function.
//J malloc 関数と置き換わる
void *user_malloc(size_t size)
{
	return sceLibcMspaceMalloc(s_mspace, size);
}

//E Replace free function.
//J free 関数と置き換わる
void user_free(void *ptr)
{
	sceLibcMspaceFree(s_mspace, ptr);
}

//E Replace calloc function.
//J calloc 関数と置き換わる
void *user_calloc(size_t nelem, size_t size)
{
	return sceLibcMspaceCalloc(s_mspace, nelem, size);
}

//E Replace realloc function.
//J realloc 関数と置き換わる
void *user_realloc(void *ptr, size_t size)
{
	return sceLibcMspaceRealloc(s_mspace, ptr, size);
}

//E Replace memalign function.
//J memalign 関数と置き換わる
void *user_memalign(size_t boundary, size_t size)
{
	return sceLibcMspaceMemalign(s_mspace, boundary, size);
}

//E Replace posix_memalign function.
//J posix_memalign 関数と置き換わる
int user_posix_memalign(void **ptr, size_t boundary, size_t size)
{
	return sceLibcMspacePosixMemalign(s_mspace, ptr, boundary, size);
}

//E Replace reallocalign function.
//J reallocalign 関数と置き換わる
void *user_reallocalign(void *ptr, size_t size, size_t boundary)
{
	return sceLibcMspaceReallocalign(s_mspace, ptr, boundary, size);
}

//E Replace malloc_stats function.
//J malloc_stats 関数と置き換わる
int user_malloc_stats(SceLibcMallocManagedSize *mmsize)
{
	return sceLibcMspaceMallocStats(s_mspace, mmsize);
}

//E Replace malloc_stats_fast function.
//J malloc_stata_fast 関数と置き換わる
int user_malloc_stats_fast(SceLibcMallocManagedSize *mmsize)
{
	return sceLibcMspaceMallocStatsFast(s_mspace, mmsize);
}

//E Replace malloc_usable_size function.
//J malloc_usable_size 関数と置き換わる
size_t user_malloc_usable_size(void *ptr)
{
	return sceLibcMspaceMallocUsableSize(ptr);
}

}

#endif

void PrintAdapterInfo( unsigned index )
{
	Tr2AdapterInfo info;
	if( FAILED( Tr2VideoAdapterInfo::GetAdapterInfo( index, info ) ) )
	{
		fprintf( stderr, "Failed to get video adapter information for adapter %u\n", index );
		return;
	}
	Tr2VideoDriverInfo driverInfo;
	if( FAILED( Tr2DriverUtilities::GetDriverVersion( info.deviceID, driverInfo ) ) )
	{
		fprintf( stderr, "Failed to get video driver information for adapter %u\n", index );
		return;
	}

	printf( 
		"Device name: %s\nDescription: %ls\nVendor ID: %u\nDevice ID: %u\n", 
		info.deviceName.c_str(), 
		info.description.c_str(), 
		info.vendorID, 
		info.deviceID );
	printf(
		"Driver version: %s\nDriver date: %s\nDriver vendor: %s\nIs Optimus: %s\nIs AMD Dynamic Switchable: %s\n\n",
		driverInfo.driverVersionString.c_str(),
		driverInfo.driverDate.c_str(),
		driverInfo.driverVendor.c_str(),
		driverInfo.isOptimus ? "yes" : "no",
		driverInfo.isAmdDynamicSwitchable ? "yes" : "no" );
}

void PrintAllAdapterInfo()
{
	unsigned count = 0;
	if( FAILED( Tr2VideoAdapterInfo::GetAdapterCount( count ) ) )
	{
		fprintf( stderr, "Failed to get video adapter count\n" );
		return;
	}
	for( unsigned i = 0; i < count; ++i )
	{
		PrintAdapterInfo( i );
	}
}

int main( int argc, char **argv ) 
{
	CCP::SetLogMainThreadId();

	for( int i = 1; i < argc; ++i )
	{
		if( strcmp( argv[i], "--interactive" ) == 0 )
		{
			g_interactive = true;
		}
		else if( strcmp( argv[i], "--screenshots" ) == 0 )
		{
			g_makeScreenShots = true;
		}
		else if( strcmp( argv[i], "--screenshotdir" ) == 0 )
		{
			if( i + 1 >= argc )
			{
				printf( "Error parsing arguments: --screenshotdir should be followed by directory path" );
				return 1;
			}
			g_screenshotFolder = argv[++i];
		}
		else if( strcmp( argv[i], "--adapterinfo" ) == 0 )
		{
			if( i + 1 >= argc )
			{
				printf( "Error parsing arguments: --adapterinfo should be followed by adapter index or \"all\"" );
				return 1;
			}
			if( strcmp( argv[i + 1], "all" ) == 0 )
			{
				PrintAllAdapterInfo();
			}
			else
			{
				PrintAdapterInfo( atoi( argv[i + 1] ) );
			}
		}
	}
	::testing::InitGoogleTest( &argc, argv );
	int result = RUN_ALL_TESTS();
	WithValidRenderContext::SaveImageSetReport();
	return result;
}

#if defined(__ANDROID__)

#include <android/native_activity.h>

ANativeActivity* g_androidActivity = nullptr;
ANativeWindow* g_androidWindow = nullptr;
bool g_windowResized = false;

uint32_t mainThread( void* )
{
    char* args[] = {
        strdup( "TrinityALTest" ),
        strdup( "--interactive" ),
    };
    main( 2, args );
    ANativeActivity_finish( g_androidActivity );
    return 0;
}

static void onNativeWindowCreated( ANativeActivity*, ANativeWindow* window )
{
    g_androidWindow = window;
    CcpCreateThread( &mainThread, nullptr, CCP_THREAD_PRIORITY_NORMAL );
    
}

static void onNativeWindowResized( ANativeActivity*, ANativeWindow* window )
{
    g_windowResized = true;
}

void ANativeActivity_onCreate( ANativeActivity* activity, void*, size_t )
{
    g_androidActivity = activity;
    activity->callbacks->onNativeWindowCreated = onNativeWindowCreated;
    activity->callbacks->onNativeWindowResized = onNativeWindowResized;
    
}

#endif