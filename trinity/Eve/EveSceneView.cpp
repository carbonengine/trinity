// Copyright © 2026 CCP ehf.

#include "StdAfx.h"
#include "EveSceneView.h"

namespace EveSceneView
{
Camera::Camera( const Matrix& view, const Matrix& projection, const TextureSize2D& renderSize, const Matrix& jitterMatrix, float mipLodBias, float upscalingAmount )
{
	this->view = view;
	this->projection = projection;
	this->jitterMatrix = jitterMatrix;
	this->renderSize = renderSize;
	this->mipLodBias = mipLodBias;
	this->upscalingAmount = upscalingAmount;

	const Matrix jitteredProjection = projection * jitterMatrix;
	const Matrix inverseView = Inverse( view );
	const Vector3 position( inverseView._41, inverseView._42, inverseView._43 );

	CTriViewport viewport;
	viewport.width = renderSize.width;
	viewport.height = renderSize.height;

	this->frustum.DeriveFrustum( &this->view, &position, &jitteredProjection, viewport );
}

ViewInfo ViewInfo::Main( bool distortion )
{
	ViewInfo view;
	view.reason = TR2RENDERREASON_NORMAL;
	view.cullMode = Tr2RenderContextEnum::CULLMODE_CW;
	view.distortion = distortion;
	view.sceneObjects = true;
	view.temporal = true;
	view.subsurfaceScattering = true;
	view.gpuParticles = true;
	view.lensflares = true;
	return view;
}

ViewInfo ViewInfo::ReflectionFace( bool lensflares )
{
	ViewInfo view;
	view.reason = TR2RENDERREASON_REFLECTION;
	// Faces are rendered into a left handed cubemap, so the caller inverts the cull mode
	// and the scene must not override it
	view.cullMode = Tr2RenderContextEnum::CULLMODE_NONE;
	view.distortion = false;
	view.sceneObjects = false;
	// faces must not disturb the main view's history
	view.temporal = false;
	view.subsurfaceScattering = false;
	view.gpuParticles = false;
	view.lensflares = lensflares;
	return view;
}
}
