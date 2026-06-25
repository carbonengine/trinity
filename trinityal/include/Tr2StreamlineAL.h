// Copyright © 2024 CCP ehf.

#pragma once

#if TRINITY_PLATFORM == TRINITY_DIRECTX12 || TRINITY_PLATFORM == TRINITY_DIRECTX11

#include <sl.h>
#include <sl_consts.h>
#include <sl_dlss.h>
#include <sl_nis.h>

#if TRINITY_PLATFORM == TRINITY_DIRECTX12
#include <sl_dlss_g.h>
#include <sl_reflex.h>
#include <sl_pcl.h>
#endif


#if TRINITY_PLATFORM == TRINITY_DIRECTX12
#include <dx12/Tr2RenderContextDx12.h>
#include "dx12/Tr2VideoAdapterInfoALDx12.h"
#elif TRINITY_PLATFORM == TRINITY_DIRECTX11
#include <dx11/Tr2RenderContextDx11.h>
#include "dx11/Tr2VideoAdapterInfoALDx11.h"
#endif


namespace Tr2StreamlineAL
{
const char* GetPluginName( sl::Feature feature );
sl::float4x4 F16AsFloat4x4( const float f[16] );

void Tr2StreamlineLog( sl::LogType type, const char* msg );

sl::Result InitializeStreamline( uint32_t appID );
void ReleaseStreamline();

const char* GetSlResultMessage( sl::Result result );


//Streamline functions

sl::Result SetDevice( void* d3dDevice, uint32_t adapter );
sl::Result UpgradeInterface( void** nativeInterface );

sl::Result SetFeatureLoaded( sl::Feature feature, bool enable );

sl::Result GetNewFrameToken( sl::FrameToken*& m_frameToken );

sl::Result SetTagsForFrame( Tr2RenderContextAL& renderContext, const sl::FrameToken& frame, const sl::ViewportHandle& viewport, const sl::ResourceTag* tags, uint32_t numTags );
sl::Result SetConstants( const sl::Constants& values, const sl::FrameToken& frame, const sl::ViewportHandle& viewport );
sl::Result EvaluateFeature( Tr2RenderContextAL& renderContext, sl::Feature feature, const sl::FrameToken& frame, const sl::BaseStructure** inputs, uint32_t numInputs );


//Feature functions

sl::Result GetDLSSOptimalSettings( const sl::DLSSOptions& options, sl::DLSSOptimalSettings& settings );
sl::Result SetDLSSOptions( const sl::ViewportHandle& viewport, const sl::DLSSOptions& options );

sl::Result SetNISOptions( const sl::ViewportHandle& viewport, const sl::NISOptions& options );


#if TRINITY_PLATFORM == TRINITY_DIRECTX12

sl::Result SetDLSSGOptions( const sl::ViewportHandle& viewport, const sl::DLSSGOptions& options );
sl::Result GetDLSSGState( const sl::ViewportHandle& viewport, sl::DLSSGState& state, const sl::DLSSGOptions* options );

sl::Result SetReflexOptions( const sl::ReflexOptions& options );

void SetPCLMarker( Tr2RenderContextEnum::FrameEvent& frameEvent, sl::FrameToken* m_frameToken );

#endif

void FreeResources( sl::Feature feature, const sl::ViewportHandle& viewport );

bool IsDLSSAvailable();
#if TRINITY_PLATFORM == TRINITY_DIRECTX12
bool IsFrameGenerationAvailable();
#endif



}

#endif
