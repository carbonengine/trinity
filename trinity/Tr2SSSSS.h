// Copyright © 2024 CCP ehf.

#pragma once

#include "Tr2DeviceResource.h"


BLUE_DECLARE( Tr2Effect );
BLUE_DECLARE( Tr2TextureReference );
BLUE_DECLARE( Tr2DepthStencil );
BLUE_DECLARE( Tr2GpuBuffer );
BLUE_DECLARE( TriTextureRes );
BLUE_DECLARE( Tr2GpuStructuredBuffer );
BLUE_DECLARE_INTERFACE( ITriTextureRes );
class Tr2GpuResourcePool;

BLUE_CLASS( Tr2SSSSS ) :
	public IRoot
{
public:
	static const int MAX_BLUR_KERNALS = 25;

	struct PerSubSurfaceFrontScatterData
	{
		Vector4 kernalInfo[MAX_BLUR_KERNALS];
	};

	EXPOSE_TO_BLUE();

	Tr2SSSSS( IRoot* lockobj = NULL );

	void SetupScreenSpaceSubSurfaceScattering( Tr2RenderContext & renderContext, ITriRenderBatchAccumulator * batches, const Tr2TextureAL& colorMap, const Tr2TextureAL& opaqueColorMap, const Tr2TextureAL& depthMap, Tr2GpuResourcePool& gpuResourcePool );
	void UpdateSubSurfaceFrontScatterData( Tr2RenderContext & renderContext );

private:
	bool m_enabled;
	bool m_hasSSSSSInScene;

	Color m_subSurfaceFrontScatterColor;
	float m_subSurfaceScatteringWidth;

	Tr2EffectPtr m_screenSpaceSubSurfaceScatteringEffect;
	Tr2EffectPtr m_specularRecombineEffect;

	Tr2GpuStructuredBufferPtr m_subSurfaceFrontScatterColorBuffer;
};

TYPEDEF_BLUECLASS( Tr2SSSSS );
