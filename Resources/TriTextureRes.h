/* 
	*************************************************************************

	TriTextureRes.h

	Created:   Nov. 2000
	OS:        Win32
	Project:   Yep

	Description:   

		Impl of textureres


	Dependencies:

		Blue, Trinity

	(c) CCP 2000

	*************************************************************************
*/

#if !defined(_TriTextureRes_H_)
#define _TriTextureRes_H_

#include "include/ITriTextureRes.h"
#include "Tr2DeviceResource.h"
#include "ID3DTexture.h"
#include "Tr2AsyncSave.h"

class Tr2RenderTarget;
class Tr2ImageHandler;

BLUE_DECLARE( Tr2RenderTarget );
BLUE_DECLARE( Tr2DepthStencil );
BLUE_DECLARE( Tr2HostBitmap );

/////////////////////////////////////////////////////////////////////////////////////
// TriTextureRes
/////////////////////////////////////////////////////////////////////////////////////
BLUE_CLASS( TriTextureRes ) :
	public BlueAsyncRes,
	public ID3DTexture,
	public ICacheable,
	public ITriTextureRes,
	public Tr2DeviceResource,
	public Tr2BitmapDimensions,
	public Tr2AsyncSave
{
public:
	using BlueAsyncRes::Lock;
	using BlueAsyncRes::Unlock;

	TriTextureRes();
	~TriTextureRes();

	unsigned m_resourceRebuiltCounter;

	void ReloadResources();
	void RebuildCachedData();

	/////////////////////////////////////////////////////////////////////////////////////
	// ITriTextureRes
	Tr2TextureAL* GetTexture();
	unsigned GetWidth()				const { return Tr2BitmapDimensions::GetWidth();    }
	unsigned GetHeight()			const { return Tr2BitmapDimensions::GetHeight();   }
	unsigned GetMipLevelCount()		const { return Tr2BitmapDimensions::GetMipCount(); }
	bool IsValid() const { return IsGood(); }
	Tr2RenderContextEnum::TextureType GetType() const { return Tr2BitmapDimensions::GetType(); }
	long UpdateSubresource( unsigned left, unsigned top, unsigned right, unsigned bottom, const void* source, unsigned sourcePitch );
	const Tr2TextureAL* GetTexture() const;
	
	// Make a TriTextureRes that wraps around an existing texture, renderTarget (that's readable) or depthBuffer (that's readable).
	// RT and DS must stay alive and the textureRes will show a live view on their contents.
	//bool SetTexture( TID3DTextureResource* );
	bool SetTexture( Tr2TextureAL& texture );
	bool SetTextureFromRT( Tr2RenderTarget* renderTarget );
	bool SetTextureFromDS( Tr2DepthStencil* depthStencil );
	
	// Make a TriTextureRes that takes a copy of the renderTarget contents and sticks with it (not a live view).
	bool CreateFromRT( Tr2RenderTarget* renderTarget, unsigned width = 0, unsigned height = 0 );

	// Make an immutable TriTextureRes that takes a copy of the hostBitmap contents and sticks with it (not a live view).
	bool CreateFromHostBitmap( Tr2HostBitmap* bitmap );

	// Create a new TriTextureRes with undefined contents.  It's implied that the usage is dynamic.
	bool Create(	uint32_t width, 
					uint32_t height, 
					uint32_t mipCount, 
					Tr2RenderContextEnum::PixelFormat format, 
					Tr2RenderContextEnum::BufferUsageFlags usage,
					Tr2PrimaryRenderContext& renderContext );

	bool SaveAsync( const wchar_t* filename );
	bool Save( const wchar_t* filename );	// synchronous
	
	
	//////////////////////////////////////////////////////////////////////////
	// IBlueResource
	//
	// Initialize is implemented by the BlueAsyncRes base class, but we
	// need to intercept it to capture mip-skip settings
	void Initialize( const wchar_t* name, const wchar_t* ext );
	void OnShutdown();

	/////////////////////////////////////////////////////////////////////////////////////
	// ITriDeviceResource
private:
	bool OnPrepareResources();
public:				
	void ReleaseResources( TriStorage s );

#if TRINITYDEV
	virtual void GetDescription( std::string& desc );
#endif

	//////////////////////////////////////////////////////////////////////////
	// ICacheable
	bool IsMemoryUsageKnown();
	size_t GetMemoryUsage();

	// Handy name for debugging and memory overviews, otherwise unused.
	std::string m_name;

	virtual void SetName( const char* name )
	{
		m_name = name;
	}
	virtual const char* GetName() const
	{
		return m_name.c_str();
	}

private:
	Tr2TextureAL		m_texture;
	Tr2RenderTargetPtr	m_wrappedRenderTarget;
	Tr2DepthStencilPtr	m_wrappedDepthStencil;

	unsigned ComputeMipSkipCount();

	unsigned int m_memoryUse;

	float	m_cutoutX;
	float	m_cutoutY;
	float	m_cutoutWidth;
	float	m_cutoutHeight;

	ImageIO::Metadata m_metadata;

	std::unique_ptr<ImageIO::HostBitmap> m_loadedBitmap;

    unsigned	m_mipLevelMaxCount;
	bool m_isTextureLoadDisabled		: 1;
	bool m_isTextureResizable			: 1;	// do we listen to the global mipskip setting?
	
private:
	bool Generate( const char *	);
	bool HasALObject( int type, size_t object );

protected:
	// Provide the functions that do the actual work of loading and preparing.
	// The async management itself is done in BlueAsyncRes.
	virtual LoadingResult DoLoad();
	virtual bool DoPrepare();
	virtual void OnCloseStream();

private:
	// These member variables and functions exist to support async texture saving;
	// from Tr2AsyncSave
	virtual bool DoPrepareAsyncSave();
	virtual bool DoExecuteAsyncSave();
	virtual void DoCleanupAsyncSave();

	friend Tr2HostBitmap;

	void* m_data;
	uint32_t m_dataSize;

	Tr2HostBitmapPtr	m_asyncSaveBitmap;
	std::shared_ptr<Tr2ImageHandler> m_asyncSaveImage;

	bool CreateAndCopyFromRenderTargetPython( Tr2RenderTarget* renderTarget ) { return CreateFromRT( renderTarget ); }
	bool CreateAndCopyFromRenderTargetWithSizePython( Tr2RenderTarget* renderTarget, unsigned width, unsigned height )
	{ return CreateFromRT( renderTarget, width, height ); }

	// for python
	bool CreateAndCopyFromRenderTarget( Tr2RenderTarget* renderTarget ) { return CreateFromRT( renderTarget ); }
	bool CreateAndCopyFromRenderTargetWithSize( Tr2RenderTarget* renderTarget, unsigned width, unsigned height ) { return CreateFromRT( renderTarget, width, height ); }

public:
	EXPOSE_TO_BLUE();

	/////////////////////////////////////////
	// Python thunkers
	typedef TriTextureRes _Class;

};

TYPEDEF_BLUECLASS_WR_SHUTDOWN(TriTextureRes);

#endif
