// Copyright © 2026 CCP ehf.

#pragma once

#include "TriFrustum.h"
#include "ITr2Renderable.h"
#include "Tr2GpuResourcePool.h"

namespace EveSceneView
{
// Where a view looks from. Create derives the frustum from the other fields, so build a
// new Camera rather than changing one.
struct Camera
{
	Matrix view;
	Matrix projection;
	Matrix jitterMatrix;
	TextureSize2D renderSize;
	float mipLodBias = 0.0f;
	float upscalingAmount = 1.0f;
	// derived from the view, the jittered projection and renderSize, never from the bound viewport
	TriFrustum frustum;

	Camera( const Matrix& view, const Matrix& projection, const TextureSize2D& renderSize, const Matrix& jitterMatrix = IdentityMatrix(), float mipLodBias = 0.0f, float upscalingAmount = 1.0f );
};

// What the scene renders for a view. The scene reads these flags rather than deciding per
// call site what a kind of view gets. The flags are off by default and every factory sets
// every field, so a new field has to be decided for each kind of view.
struct ViewInfo
{
	// Passed on to GetBatches, so renderables can filter themselves (e.g. ShouldReflect)
	Tr2RenderReason reason = TR2RENDERREASON_NORMAL;
	Tr2RenderContextEnum::CullMode cullMode = Tr2RenderContextEnum::CULLMODE_CW;
	bool distortion = false;
	// Gather the scene's objects, camera attachments, static particles and impostors.
	// Otherwise only the reflectable renderables in the component registry are gathered.
	bool sceneObjects = false;
	// The view feeds the next frame: view history for reprojection, temporal volumetrics
	// and lens flare occlusion queries
	bool temporal = false;
	bool subsurfaceScattering = false;
	bool gpuParticles = false;
	bool lensflares = false;

	// Render the scene as a reflection probe sees it: damped reflection intensity,
	// reflection background and the reflection shadow map
	bool IsReflection() const
	{
		return reason == TR2RENDERREASON_REFLECTION;
	}

	static ViewInfo Main( bool distortion );
	static ViewInfo ReflectionFace( bool lensflares );
};
}
