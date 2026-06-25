// Copyright © 2023 CCP ehf.

#include "StdAfx.h"

#if TRINITY_PLATFORM == TRINITY_DIRECTX12

#include "TriDevice.h"
#include "RenderJob/Tr2RenderJobs.h"
#include "TriSettingsRegistrar.h"
#include <sstream>
#include <iomanip>


bool g_emulateDriverReset = false;
TRI_REGISTER_SETTING( "emulateDriverReset", g_emulateDriverReset );

CCP_STATS_DECLARED_ELSEWHERE( presentTime );
extern unsigned long long g_currentFrameCounter;

namespace
{

std::string GetDeviceRemovedReasonString( HRESULT hr )
{
	std::stringstream reason;
	switch( hr )
	{
	case DXGI_ERROR_DEVICE_HUNG:
		reason << "DXGI_ERROR_DEVICE_HUNG: ";
		break;
	case DXGI_ERROR_DEVICE_REMOVED:
		reason << "DXGI_ERROR_DEVICE_REMOVED: ";
		break;
	case DXGI_ERROR_DEVICE_RESET:
		reason << "DXGI_ERROR_DEVICE_RESET: ";
		break;
	case DXGI_ERROR_DRIVER_INTERNAL_ERROR:
		reason << "DXGI_ERROR_DRIVER_INTERNAL_ERROR: ";
		break;
	case DXGI_ERROR_INVALID_CALL:
		reason << "DXGI_ERROR_INVALID_CALL: ";
		break;
	default:
		reason << "HRESULT: ";
		break;
	}
	reason << "0x" << std::setfill( '0' ) << std::setw( 8 ) << std::hex << (uint32_t)hr;
	return reason.str();
}

void NotifyDeviceRemoved( HRESULT hr, BlueScriptCallback& onDeviceRemoved )
{
	static uint32_t s_deviceLostCount = 0;
	++s_deviceLostCount;

	Tr2RenderContextEnum::RenderContextStatus status = Tr2RenderContextEnum::CONTEXT_STATUS_INVALID;
	std::string marker, markerMessage, offendingShader;
	if( SUCCEEDED( Tr2RenderContext_GetMainThreadRenderContext().GetGpuStateMarker( status, marker, offendingShader ) ) )
	{
		if( !marker.empty() )
		{
			markerMessage = ", GPU marker: " + marker;
		}
		if( !offendingShader.empty() )
		{
			markerMessage = ", shader: " + offendingShader;
		}
	}
	auto reason = GetDeviceRemovedReasonString( hr );

	if( onDeviceRemoved.IsValid() )
	{
		onDeviceRemoved.CallVoid( (uint64_t)(unsigned long)hr, reason, s_deviceLostCount, marker, "", offendingShader );
	}

	CCP_LOGERR( "DX12 device removed, reason: %s%s", reason.c_str(), markerMessage.c_str() );

	if( BeCrashes )
	{
		std::string str;
		BeCrashes->SetCrashKeyValue( "gpuRemovedCount", ( str = std::to_string( s_deviceLostCount ) ).c_str() );
		BeCrashes->SetCrashKeyValue( "gpuRemovedFrame", ( str = std::to_string( g_currentFrameCounter ) ).c_str() );
		BeCrashes->SetCrashKeyValue( "gpuRemovedReason", reason.c_str() );
		if( !marker.empty() )
		{
			BeCrashes->SetCrashKeyValue( "gpuRemovedMarker", marker.c_str() );
		}
	}
};

}
constexpr char const* DredBreadcrumbOpName( D3D12_AUTO_BREADCRUMB_OP op )
{
	switch( op )
	{
	case D3D12_AUTO_BREADCRUMB_OP_SETMARKER:
		return "Set marker";
	case D3D12_AUTO_BREADCRUMB_OP_BEGINEVENT:
		return "Begin event";
	case D3D12_AUTO_BREADCRUMB_OP_ENDEVENT:
		return "End event";
	case D3D12_AUTO_BREADCRUMB_OP_DRAWINSTANCED:
		return "Draw instanced";
	case D3D12_AUTO_BREADCRUMB_OP_DRAWINDEXEDINSTANCED:
		return "Draw indexed instanced";
	case D3D12_AUTO_BREADCRUMB_OP_EXECUTEINDIRECT:
		return "Execute indirect";
	case D3D12_AUTO_BREADCRUMB_OP_DISPATCH:
		return "Dispatch";
	case D3D12_AUTO_BREADCRUMB_OP_COPYBUFFERREGION:
		return "Copy buffer region";
	case D3D12_AUTO_BREADCRUMB_OP_COPYTEXTUREREGION:
		return "Copy texture region";
	case D3D12_AUTO_BREADCRUMB_OP_COPYRESOURCE:
		return "Copy resource";
	case D3D12_AUTO_BREADCRUMB_OP_COPYTILES:
		return "Copy tiles";
	case D3D12_AUTO_BREADCRUMB_OP_RESOLVESUBRESOURCE:
		return "Resolve subresource";
	case D3D12_AUTO_BREADCRUMB_OP_CLEARRENDERTARGETVIEW:
		return "Clear render target view";
	case D3D12_AUTO_BREADCRUMB_OP_CLEARUNORDEREDACCESSVIEW:
		return "Clear unordered access view";
	case D3D12_AUTO_BREADCRUMB_OP_CLEARDEPTHSTENCILVIEW:
		return "Clear depth stencil view";
	case D3D12_AUTO_BREADCRUMB_OP_RESOURCEBARRIER:
		return "Resource barrier";
	case D3D12_AUTO_BREADCRUMB_OP_EXECUTEBUNDLE:
		return "Execute bundle";
	case D3D12_AUTO_BREADCRUMB_OP_PRESENT:
		return "Present";
	case D3D12_AUTO_BREADCRUMB_OP_RESOLVEQUERYDATA:
		return "Resolve query data";
	case D3D12_AUTO_BREADCRUMB_OP_BEGINSUBMISSION:
		return "Begin submission";
	case D3D12_AUTO_BREADCRUMB_OP_ENDSUBMISSION:
		return "End submission";
	case D3D12_AUTO_BREADCRUMB_OP_DECODEFRAME:
		return "Decode frame";
	case D3D12_AUTO_BREADCRUMB_OP_PROCESSFRAMES:
		return "Process frames";
	case D3D12_AUTO_BREADCRUMB_OP_ATOMICCOPYBUFFERUINT:
		return "Atomic copy buffer uint";
	case D3D12_AUTO_BREADCRUMB_OP_ATOMICCOPYBUFFERUINT64:
		return "Atomic copy buffer uint64";
	case D3D12_AUTO_BREADCRUMB_OP_RESOLVESUBRESOURCEREGION:
		return "Resolve subresource region";
	case D3D12_AUTO_BREADCRUMB_OP_WRITEBUFFERIMMEDIATE:
		return "Write buffer immediate";
	case D3D12_AUTO_BREADCRUMB_OP_DECODEFRAME1:
		return "Decode frame 1";
	case D3D12_AUTO_BREADCRUMB_OP_SETPROTECTEDRESOURCESESSION:
		return "Set protected resource session";
	case D3D12_AUTO_BREADCRUMB_OP_DECODEFRAME2:
		return "Decode frame 2";
	case D3D12_AUTO_BREADCRUMB_OP_PROCESSFRAMES1:
		return "Process frames 1";
	case D3D12_AUTO_BREADCRUMB_OP_BUILDRAYTRACINGACCELERATIONSTRUCTURE:
		return "Build raytracing acceleration structure";
	case D3D12_AUTO_BREADCRUMB_OP_EMITRAYTRACINGACCELERATIONSTRUCTUREPOSTBUILDINFO:
		return "Emit raytracing acceleration structure post build info";
	case D3D12_AUTO_BREADCRUMB_OP_COPYRAYTRACINGACCELERATIONSTRUCTURE:
		return "Copy raytracing acceleration structure";
	case D3D12_AUTO_BREADCRUMB_OP_DISPATCHRAYS:
		return "Dispatch rays";
	case D3D12_AUTO_BREADCRUMB_OP_INITIALIZEMETACOMMAND:
		return "Initialize meta command";
	case D3D12_AUTO_BREADCRUMB_OP_EXECUTEMETACOMMAND:
		return "Execute meta command";
	case D3D12_AUTO_BREADCRUMB_OP_ESTIMATEMOTION:
		return "Estimate motion";
	case D3D12_AUTO_BREADCRUMB_OP_RESOLVEMOTIONVECTORHEAP:
		return "Resolve motion vector heap";
	case D3D12_AUTO_BREADCRUMB_OP_SETPIPELINESTATE1:
		return "Set pipeline state 1";
	case D3D12_AUTO_BREADCRUMB_OP_INITIALIZEEXTENSIONCOMMAND:
		return "Initialize extension command";
	case D3D12_AUTO_BREADCRUMB_OP_EXECUTEEXTENSIONCOMMAND:
		return "Execute extension command";
	case D3D12_AUTO_BREADCRUMB_OP_DISPATCHMESH:
		return "Dispatch mesh";
	default:
		return "Unknown";
	}
}

void TriDevice::HandleRenderTick( Be::Time realTime, Be::Time simTime )
{
#if BLUE_WITH_PYTHON
	AutoTasklet _at( PyOS->GetTaskletTimer(), "TriDevice::HandleRenderTick" );
#endif

	auto TearDownDevice = [&]() {
		ReleaseDeviceResources( TRISTORAGE_ALL );

		mDeviceLost = true;
		Tr2RenderContext::DestroyMainThreadRenderContext();

		LogAllLiveResources();
	};

	auto HandleDREDDebugging = [&]() {
		// Example sourced from https://github.com/NANAnoo/Adria/blob/f25306f66fab1bba35fc0622880ff876d230a8c8/Adria/Graphics/GfxDevice.cpp

		CComPtr<ID3D12DeviceRemovedExtendedData1> pDred;
		if( SUCCEEDED( Tr2RenderContext_GetMainThreadRenderContext().m_device->QueryInterface( IID_PPV_ARGS( &pDred ) ) ) )
		{
			D3D12_DRED_AUTO_BREADCRUMBS_OUTPUT1 dredAutoBreadcrumbsOutput;
			D3D12_DRED_PAGE_FAULT_OUTPUT1 dredPageFaultOutput;
			if( SUCCEEDED( pDred->GetAutoBreadcrumbsOutput1( &dredAutoBreadcrumbsOutput ) ) )
			{

				CCP_LOGERR( "[DRED] Last tracked GPU operations:" );
				std::map<UINT, std::wstring> contextStrings;
				D3D12_AUTO_BREADCRUMB_NODE1 const* pNode = dredAutoBreadcrumbsOutput.pHeadAutoBreadcrumbNode;
				while( pNode && pNode->pLastBreadcrumbValue )
				{
					UINT lastCompletedOp = *pNode->pLastBreadcrumbValue;
					if( lastCompletedOp != (int)pNode->BreadcrumbCount && lastCompletedOp != 0 )
					{
						CCP_LOGERR( "[DRED] Commandlist completed %d of %d commands", lastCompletedOp, pNode->BreadcrumbCount );

						UINT firstOp = std::max<UINT>( lastCompletedOp - 100, 0 );
						UINT lastOp = std::min<UINT>( lastCompletedOp + 20, UINT( pNode->BreadcrumbCount ) - 1 );

						contextStrings.clear();
						for( UINT breadcrumbContext = firstOp; breadcrumbContext < pNode->BreadcrumbContextsCount; ++breadcrumbContext )
						{
							const D3D12_DRED_BREADCRUMB_CONTEXT& context = pNode->pBreadcrumbContexts[breadcrumbContext];
							contextStrings[context.BreadcrumbIndex] = context.pContextString;
						}

						for( UINT op = firstOp; op <= lastOp; ++op )
						{
							D3D12_AUTO_BREADCRUMB_OP breadcrumbOp = pNode->pCommandHistory[op];

							std::wstring contextString;
							auto it = contextStrings.find( op );
							if( it != contextStrings.end() )
							{
								contextString = it->second;
							}

							char const* opName = DredBreadcrumbOpName( breadcrumbOp );
							CCP_LOGERR( "\tOp: %d, %s%ls%s", op, opName, contextString.c_str(), ( op + 1 == lastCompletedOp ) ? " - Last completed" : "" );
						}
					}
					pNode = pNode->pNext;
				}
			}
			if( SUCCEEDED( pDred->GetPageFaultAllocationOutput1( &dredPageFaultOutput ) ) )
			{
				for( auto node = dredPageFaultOutput.pHeadExistingAllocationNode; node != nullptr; node = node->pNext )
				{
					if( node->ObjectNameW )
					{
						CCP_LOGERR( "Page Fault Allocation on: %ls", node->ObjectNameW );
					}
				}
				for( auto node = dredPageFaultOutput.pHeadRecentFreedAllocationNode; node != nullptr; node = node->pNext )
				{
					if( node->ObjectNameW )
					{
						CCP_LOGERR( "Page Fault Free on: %ls", node->ObjectNameW );
					}
				}
			}
		}
	};

	auto HandleLostDevice = [&]() {
		HRESULT hr = Tr2RenderContext_GetMainThreadRenderContext().m_device->GetDeviceRemovedReason();
		if( FAILED( hr ) || g_emulateDriverReset )
		{
			HandleDREDDebugging();
			g_emulateDriverReset = false;
			if( !mDeviceLost )
			{
				NotifyDeviceRemoved( hr, m_onDeviceRemoved );
				TearDownDevice();
				return true;
			}
		}
		return false;
	};

	if( mDeviceLost )
	{
		if( !ChangeDevice( mAdapter, mHwnd, nullptr ) )
		{
			CCP_LOGERR( "Failed to create D3D device" );
		}

		return;
	}

	if( !Tr2RenderContext_GetMainThreadRenderContext().m_device )
	{
		return;
	}

	if( ShouldSkipFrame() )
	{
		Throttle();
		return;
	}
	Tr2RenderContext_GetMainThreadRenderContext().MarkFrameEvent( Tr2RenderContextEnum::FRAME_EVENT_UPDATE_STARTED );
	if( HandleLostDevice() )
	{
		return;
	}

	if( m_upscalingChanged )
	{
		CCP_LOGNOTICE( "DX12 device removed, reason: upscaler changed" );
		TearDownDevice();
		return;
	}

	if( mDeviceLost )
	{
		return;
	}

	if( m_renderJobs )
	{
		m_renderJobs->RunUpdate( realTime, simTime );
	}

	m_postUpdateCallbacks->Update();
	Tr2RenderContext_GetMainThreadRenderContext().MarkFrameEvent( Tr2RenderContextEnum::FRAME_EVENT_UPDATE_FINISHED );

	// Present the backbuffer from the last renderering to the front buffer.
	// it is more efficient to do it like this (revers order), because there is some
	// acyncrounicy between EndScene() and Present(). So if we pump Python
	// and do all the other stuff between we get a degree of paralization
	Tr2RenderContext_GetMainThreadRenderContext().MarkFrameEvent( Tr2RenderContextEnum::FRAME_EVENT_PRESENT_STARTED );
	{
		CCP_STATS_SCOPED_TIME( presentTime );
		if( FAILED( Tr2RenderContext_GetMainThreadRenderContext().Present() ) )
		{
			if( HandleLostDevice() )
			{
				return;
			}
		}
	}
	if( HandleLostDevice() )
	{
		return;
	}
	Tr2RenderContext_GetMainThreadRenderContext().MarkFrameEvent( Tr2RenderContextEnum::FRAME_EVENT_PRESENT_FINISHED );

	Tr2RenderContext_GetMainThreadRenderContext().MarkFrameEvent( Tr2RenderContextEnum::FRAME_EVENT_RENDERING_STARTED );
	if( !Render() )
	{
		CCP_LOGERR( "Failed to render a frame" );
	}
	Tr2RenderContext_GetMainThreadRenderContext().MarkFrameEvent( Tr2RenderContextEnum::FRAME_EVENT_RENDERING_FINISHED );
}

// -- Smaller helpers to enable big methods like TriDevice::Render to be mostly API neutral.

// Do we have a valid device pointer? Lower level question than "do we have a valid renderContext",
// so first question may be true while second one is still false.
bool TriDevice::DeviceExists()
{
	USE_MAIN_THREAD_RENDER_CONTEXT();
	return renderContext.IsValid();
}

// --------------------------------------------------------------------------------------
// Description:
//   A chance for device to respond to application window being activated/deactivated.
//   For DX11 platform we minimize fullscreen window when user alt-tabs from application
//   and restore it back when he returns. This makes DX11 window behavior being
//   consistent with DX9.
// Arguments:
//   activated - true if application was activated; false otherwise
// --------------------------------------------------------------------------------------
void TriDevice::ApplicationActivated( ApplicationActivation activated )
{
	CCP_STATS_ZONE( __FUNCTION__ );

	if( !mPresentParam.windowed && mHwnd )
	{
		USE_MAIN_THREAD_RENDER_CONTEXT();
		if( activated == APP_ACTIVATED )
		{
#if !USE_BORDERLESS_WINDOW
			ChangeDevice( mAdapter, mHwnd, nullptr );
#endif
		}
		else
		{
			ShowWindow( mHwnd, SW_MINIMIZE );
#if !USE_BORDERLESS_WINDOW
			ChangeDevice( mAdapter, mHwnd, nullptr );
#endif
		}
	}
}

#endif
