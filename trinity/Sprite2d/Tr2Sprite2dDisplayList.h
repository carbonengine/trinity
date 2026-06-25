// Copyright © 2012 CCP ehf.

#pragma once

#ifndef Tr2Sprite2dDisplayList_h
#define Tr2Sprite2dDisplayList_h

#include "Tr2DeviceResource.h"
#include "ITr2SpriteObject.h"
#include "IRenderCallback.h"

const unsigned int TR2_SS_MAX_TRANSFORM_COUNT = 32;

BLUE_DECLARE( TriRenderJob );
BLUE_DECLARE( Tr2AtlasTexture );
BLUE_DECLARE( Tr2Effect );

struct Tr2Sprite2dDisplayList : public Tr2DeviceResource
{
	Tr2BufferAL vertexBuffer;
	Tr2BufferAL indexBuffer;

	struct Entry final : public IRenderCallback
	{
		// Render job to run, rather than issuing a draw call
		TriRenderJobPtr job;

		uint32_t numVertices;
		uint32_t startIndex;
		uint32_t primitiveCount;
		Vector4 texelSize0;
		Vector4 texelSize1;
		Tr2AtlasTexturePtr texture0;
		Tr2AtlasTexturePtr texture1;
		Tr2EffectPtr effect;
		//hardcoded due to need for CB slot//Tr2EffectConstant transformsHandle;
		Matrix transformArray[TR2_SS_MAX_TRANSFORM_COUNT];
		Tr2ConstantBufferAL* m_uiTransformsCb;

		//////////////////////////////////////////////////////////////////////////
		// IRenderCallback
		void SubmitGeometry( Tr2RenderContext& renderContext ) override;
	};

	std::list<Entry> entries;

	Tr2Sprite2dDisplayList( ITr2SpriteObject* owner );
	~Tr2Sprite2dDisplayList() override;

	//////////////////////////////////////////////////////////////////////////
	// ITriDeviceResource
	void ReleaseResources( TriStorage s ) override;

private:
	bool OnPrepareResources();

private:
	ITr2SpriteObject* m_owner;
};

#endif // Tr2Sprite2dDisplayList_h
