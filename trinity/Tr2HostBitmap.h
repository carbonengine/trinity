#pragma once
#ifndef Tr2HostBitmap_h_
#define Tr2HostBitmap_h_

#include "Tr2DeviceResource.h"
#include "Resources/Tr2AsyncSave.h"

BLUE_DECLARE( Tr2HostBitmap );
BLUE_DECLARE( Tr2RenderTarget );
BLUE_DECLARE( TriTextureRes );

class Tr2ImageHandler;

// -------------------------------------------------------------
// Description:
//   A class wrapping a bunch of pixels on the CPU plus width, height,
//   pixelformat. Replaces the old DX9 specific "offscreen surface".
// -------------------------------------------------------------
BLUE_CLASS( Tr2HostBitmap ) : public IRoot, public ImageIO::HostBitmap, public Tr2AsyncSave
{
public:
	EXPOSE_TO_BLUE();

    Tr2HostBitmap( IRoot* = 0 );
	~Tr2HostBitmap();

	virtual void Destroy();

	bool CopyFromRenderTarget( Tr2TextureAL& rt, Tr2RenderContext& renderContext );
	bool CopyFromRenderTarget( Tr2TextureAL& rt, const int* srcRect, int offsetX, int offsetY, Tr2RenderContext& renderContext );
	bool CopyFaceFromRenderTarget( Tr2RenderContextEnum::CubemapFace face, Tr2TextureAL& rt, Tr2RenderContext& renderContext );
	bool CopyFromTexture( Tr2TextureAL& texture, Tr2RenderContext& renderContext );
	bool CopyFromTextureRes  ( TriTextureRes& res, Tr2RenderContext& renderContext );
	bool CreateFromHeightData( const std::vector<float>& data, int32_t width, int32_t height );
	bool PopulateMargin( unsigned margin );
	bool Save( const wchar_t* destFile );
	bool SaveAsync( const wchar_t* destFile );
	bool CreateFromFile( const std::wstring& file );

	bool Compress( unsigned compressionFormat, unsigned qualityLevel, TriTextureRes* output );

	uint32_t CountPixelsOfValue( const std::string& channels, uint32_t value ) const;
	bool IsMonochrome() const;

	// Ugly methods to support old code that does ugly things.
	bool SetPixel( int width, int height,  const void* val );
	bool GetPixel( int width, int height,  void* val ) const;

	ALResult ChangeFormatFromScript( Tr2RenderContextEnum::PixelFormat format );

private:
	bool CopyFromRenderTargetPython( Tr2RenderTarget* rt );
	bool CopyFromRenderTargetRegionPython( Tr2RenderTarget* rt, int left, int top, int right, int bottom, unsigned offsetX, unsigned offsetY );

#if BLUE_WITH_PYTHON
	bool		SetPixelPy(PyObject *tuple);
	PyObject*	GetPixelPy(PyObject *typle);
	PyObject* PySetPixel( PyObject* args );
	PyObject* PyGetPixel( PyObject* args );
	PyObject* PySetPixels( PyObject* args );
#endif
	bool CopyFaceFromRenderTargetPython( unsigned face, Tr2RenderTarget* rt );
	bool CopyFromTextureResPython( TriTextureRes* tr );

	bool SharedCopyFaceFromRenderTarget( Tr2RenderContextEnum::CubemapFace face, Tr2TextureAL& rt, const int* srcRect, int offsetX, int offsetY, Tr2RenderContext& renderContext );

#if BLUE_WITH_PYTHON
	PyObject* PyApplyConvFilter ( PyObject* args );
	static PyObject* PyGetRawData( PyObject* self, PyObject* args );
	static PyObject* PyGetMipRawData( PyObject* self, PyObject* args );
	static PyObject* PySetMipRawData( PyObject* self, PyObject* args );
#endif

#if BLUE_WITH_PYTHON
	PyObject* PyCreateFromFile( PyObject* args );
#endif

	// Tr2AsyncSave
	virtual bool DoPrepareAsyncSave();
	virtual bool DoExecuteAsyncSave();
	virtual void DoCleanupAsyncSave();
};

TYPEDEF_BLUECLASS( Tr2HostBitmap );

#endif //Tr2HostBitmap_h_
