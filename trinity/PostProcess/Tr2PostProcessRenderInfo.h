////////////////////////////////////////////////////////////////////////////////
//
// Created:		January 2019
// Copyright:	CCP 2019
//

#pragma once

#ifndef Tr2PostProcessRenderInfo_H
#define Tr2PostProcessRenderInfo_H

#include "StdAfx.h"
#include "Shader/Tr2ShaderBuffer.h"
#include "Tr2RenderTarget.h"
#include "Tr2RenderContext.h"
#include "Tr2GpuBuffer.h"


BLUE_DECLARE( Tr2RenderTarget );
BLUE_DECLARE( Tr2RenderContext );
BLUE_DECLARE( Tr2ShaderBuffer );
BLUE_DECLARE( Tr2GpuBuffer );

BLUE_CLASS( Tr2PostProcessRenderInfo ) :
	public INotify
{
public:
	EXPOSE_TO_BLUE();

	class Texture
	{
	public:
		Texture();
		Texture( const Texture& other );
		Texture& operator=( const Texture& other );
		~Texture();

		Tr2RenderTarget* GetRenderTarget() const;
		operator Tr2RenderTarget*() const;
		Tr2RenderTarget* operator->() const;
	private:
		Texture( Tr2PostProcessRenderInfo* renderInfo, Tr2RenderTarget* renderTarget );

		Tr2RenderTarget* m_renderTarget;
		Tr2PostProcessRenderInfo* m_renderInfo;

		friend class Tr2PostProcessRenderInfo;
	};

	Tr2PostProcessRenderInfo( IRoot* lockobj = NULL );
	~Tr2PostProcessRenderInfo();

	bool OnModified( Be::Var* value );

	bool Setup( Tr2RenderContext & renderContext );
	void SetSourceBuffer( Tr2RenderTarget* sourceBuffer );

	Texture GetTempTexture( float sizeFactor = 1.f, Tr2RenderContextEnum::ExFlag exFlag = Tr2RenderContextEnum::EX_NONE, Tr2RenderContextEnum::PixelFormat pixelFormat = Tr2RenderContextEnum::PIXEL_FORMAT_UNKNOWN );
	Texture GetSourceBuffer();
	Texture GetBlackTexture();

	std::vector<Tr2RenderTargetPtr> GetAllTempTextures() const;
private:
	friend class Tr2PostProcessRenderInfo::Texture;

	void LockTempTexture( Tr2RenderTarget * tempTexture );
	void ReleaseTempTexture( Tr2RenderTarget * tempTexture );

	struct TempTexture
	{
		Tr2RenderTargetPtr texture;
		float sizeFactor;
		Tr2RenderContextEnum::ExFlag exFlag;
		int32_t lockCount;
		Tr2RenderContextEnum::PixelFormat pixelFormat;
	};

	std::vector<TempTexture> m_tempTextures;
	Tr2RenderTargetPtr m_sourceBuffer;
	Tr2RenderTargetPtr m_black;
};

TYPEDEF_BLUECLASS( Tr2PostProcessRenderInfo );

#endif
