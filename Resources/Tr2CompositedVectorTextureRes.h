////////////////////////////////////////////////////////////
//
//    Created:   July 2018
//    Copyright: CCP 2018
//

#pragma once

#include "ITr2TextureProvider.h"


BLUE_DECLARE( Tr2CairoScriptSourceRes );


BLUE_CLASS( Tr2CompositedVectorTextureRes ): 
	public BlueAsyncRes, 
	public Tr2DeviceResource,
	public ICacheable,
	public ITr2TextureProvider
{
public:
	EXPOSE_TO_BLUE();

	struct Source
	{
		Tr2CairoScriptSourceResPtr source;
		Vector2 position;
		float rotation;
		float scale;
		Color color;
	};

	Tr2CompositedVectorTextureRes( IRoot* lockobj = nullptr );
	~Tr2CompositedVectorTextureRes();

	void ComposeSync( uint32_t width, uint32_t height, bool premultipliedAlpha, const std::vector<Source>& sources );
	void ComposeAsync( uint32_t width, uint32_t height, bool premultipliedAlpha, const std::vector<Source>& sources );

	bool Save( const wchar_t* path );

	virtual bool IsMemoryUsageKnown();
	virtual size_t GetMemoryUsage();

	virtual Tr2TextureAL* GetTexture();

	virtual void ReleaseResources( TriStorage s );

	static void RegisterDynamicConstructor( const wchar_t* name, const wchar_t* directory );
protected:
	virtual bool OnPrepareResources();

	virtual LoadingResult DoLoad();
	virtual bool DoPrepare();
	virtual bool DoOpenStream();
private:
	static void StaticResourceLoadFinished( void* context );
	static void StaticResourcePrepFinished( void* context );
	void StartLoading();

	Tr2RenderContextEnum::TextureType GetTextureType() const;
	Tr2RenderContextEnum::PixelFormat GetFormat() const;
	uint32_t GetWidth() const;
	uint32_t GetHeight() const;
	uint32_t GetOne() const;

	ImageIO::HostBitmap m_bitmap;
	Tr2TextureAL m_texture;
	TrackableStdVector<Source> m_sources;
	CcpAtomic<uint32_t> m_resourceLoadCbId;
	CcpAtomic<uint32_t> m_resourcePrepCbId;
	uint32_t m_width;
	uint32_t m_height;
	bool m_premultipliedAlpha;
};

TYPEDEF_BLUECLASS_WR_SHUTDOWN( Tr2CompositedVectorTextureRes );
