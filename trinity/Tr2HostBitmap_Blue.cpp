// Copyright © 2023 CCP ehf.

#include "StdAfx.h"
#include "Tr2HostBitmap.h"
#include "Resources/TriTextureRes.h"
#include "Tr2RenderTarget.h"
#include "Tr2TextureAtlas.h"

BLUE_DEFINE( Tr2HostBitmap );

#if BLUE_WITH_PYTHON
static PyObject* PyInit( PyObject* self, PyObject* args )
{
	USE_MAIN_THREAD_RENDER_CONTEXT();

	Tr2HostBitmap* pThis = BluePythonCast<Tr2HostBitmap*>( self );

	PyObject* pyBlueObject = nullptr;
	unsigned dummy1 = 0, dummy2 = 0, dummy3 = 0; // make sure the signature is still up to 4 elements, else blue binding doesn't work with constructor 'overloading' :(
	if( PyArg_ParseTuple( args, "|Oiii", &pyBlueObject, &dummy1, &dummy2, &dummy3 ) && pyBlueObject && !dummy1 && !dummy2 && !dummy3 )
	{
		Tr2RenderTarget* renderTarget = nullptr;
		if( BluePythonCast<Tr2RenderTarget*>( pyBlueObject ) && BlueExtractArgument( pyBlueObject, renderTarget, 1 ) && renderTarget && renderTarget->IsValid() )
		{
			auto& rt = renderTarget->GetRenderTarget();
			pThis->Create( rt.GetWidth(), rt.GetHeight(), rt.GetTrueMipCount(), rt.GetFormat() );
			pThis->CopyFromRenderTarget( *renderTarget, renderContext );
			Py_RETURN_NONE;
		}

		// Try again with TriTextureRes
		TriTextureRes* textureRes = nullptr;
		if( BluePythonCast<TriTextureRes*>( pyBlueObject ) && BlueExtractArgument( pyBlueObject, textureRes, 1 ) && textureRes && textureRes->IsGood() )
		{
			pThis->Create( textureRes->GetWidth(), textureRes->GetHeight(), textureRes->GetTrueMipCount(), textureRes->GetFormat() );
			pThis->CopyFromTextureRes( *textureRes, renderContext );
			Py_RETURN_NONE;
		}

		// ...and with Tr2TextureAtlas
		Tr2TextureAtlas* textureAtlas = nullptr;
		if( BluePythonCast<Tr2TextureAtlas*>( pyBlueObject ) && BlueExtractArgument( pyBlueObject, textureAtlas, 1 ) &&
			textureAtlas && textureAtlas->GetTexture() && textureAtlas->GetTexture()->IsValid() )
		{
			auto& texture = *textureAtlas->GetTexture();
			pThis->Create( texture.GetWidth(), texture.GetHeight(), texture.GetTrueMipCount(), texture.GetFormat() );
			pThis->CopyFromTexture( texture, renderContext );
			Py_RETURN_NONE;
		}

		Py_RETURN_NONE;
	}


	unsigned width = 0, height = 0, mipCount = 0, format = 0;

	if( !PyArg_ParseTuple( args, "|iiii", &width, &height, &mipCount, &format ) )
	{
		return nullptr;
	}

	if( width && height && format )
	{
		pThis->Create( width, height, mipCount, static_cast<Tr2RenderContextEnum::PixelFormat>( format ) );
	}

	Py_RETURN_NONE;
}

PyObject* Tr2HostBitmap::PyGetRawData( PyObject* self, PyObject* args )
{
	Tr2HostBitmap* pThis = BluePythonCast<Tr2HostBitmap*>( self );

	if( pThis->IsValid() )
	{
		PyObject* buffer = PyVerCompat::MemoryViewRW( pThis->GetRawData(), pThis->GetHeight() * pThis->GetPitch() );

		PyObject* result = PyTuple_New( 4 );
		if( !result )
		{
			Py_DECREF( buffer );
			Py_RETURN_FALSE;
		}
		PyTuple_SET_ITEM( result, 0, buffer );
		PyTuple_SET_ITEM( result, 1, ToPython( pThis->GetWidth() ) );
		PyTuple_SET_ITEM( result, 2, ToPython( pThis->GetHeight() ) );
		PyTuple_SET_ITEM( result, 3, ToPython( pThis->GetPitch() ) );

		return result;
	}

	Py_RETURN_FALSE;
}

PyObject* Tr2HostBitmap::PyGetMipRawData( PyObject* self, PyObject* args )
{
	Tr2HostBitmap* pThis = BluePythonCast<Tr2HostBitmap*>( self );

	if( pThis->IsValid() )
	{
		// requested miplevel is provided via function arguement
		unsigned int mipLevel = 0;
		if( !PyArg_ParseTuple( args, "i", &mipLevel ) )
			Py_RETURN_FALSE;

		PyObject* buffer = PyVerCompat::MemoryViewRW( pThis->GetMipRawData( mipLevel ), pThis->GetMipHeight( mipLevel ) * pThis->GetMipPitch( mipLevel ) );

		PyObject* result = PyTuple_New( 4 );
		if( !result )
		{
			Py_DECREF( buffer );
			Py_RETURN_FALSE;
		}
		PyTuple_SET_ITEM( result, 0, buffer );
		PyTuple_SET_ITEM( result, 1, ToPython( pThis->GetMipWidth( mipLevel ) ) );
		PyTuple_SET_ITEM( result, 2, ToPython( pThis->GetMipHeight( mipLevel ) ) );
		PyTuple_SET_ITEM( result, 3, ToPython( pThis->GetMipPitch( mipLevel ) ) );

		return result;
	}

	Py_RETURN_FALSE;
}

PyObject* Tr2HostBitmap::PySetMipRawData( PyObject* self, PyObject* args )
{
	unsigned int mipLevel = 0;
	Py_buffer buffer;
	if( !PyArg_ParseTuple( args, "iw*", &mipLevel, &buffer ) )
		return nullptr;
	ON_BLOCK_EXIT( [&] { PyBuffer_Release( &buffer ); } );

	Tr2HostBitmap* pThis = BluePythonCast<Tr2HostBitmap*>( self );
	if( !pThis->IsValid() )
	{
		PyErr_SetString( PyExc_ValueError, "invalid bitmap" );
		return nullptr;
	}

	if( mipLevel >= pThis->GetTrueMipCount() )
	{
		PyErr_SetString( PyExc_ValueError, "mip level out of range" );
		return nullptr;
	}

	if( buffer.len < Py_ssize_t( pThis->GetMipSize( mipLevel ) ) )
	{
		PyErr_SetString( PyExc_ValueError, "buffer too small" );
		return nullptr;
	}

	memcpy( pThis->GetMipRawData( mipLevel ), buffer.buf, pThis->GetMipSize( mipLevel ) );
	Py_RETURN_NONE;
}


PyObject* Tr2HostBitmap::PyCreateFromFile( PyObject* args )
{
	PyObject* file = nullptr;

	if( !PyArg_ParseTuple( args, "O", &file ) )
	{
		PyErr_SetString( PyExc_TypeError, "Expected a filepath string" );
		return nullptr;
	}

	std::wstring wcstr;
	std::string cstr;
	if( PyUnicode_Check( file ) && BlueExtractArgument( file, wcstr, 1 ) )
	{
		CreateFromFile( wcstr );
	}
#if PY_MAJOR_VERSION == 2
	else if( PyString_Check( file ) && BlueExtractArgument( file, cstr, 1 ) )
	{
		CreateFromFile( (const wchar_t*)CA2W( cstr.c_str() ) );
	}
#endif
	else
	{
		PyErr_SetString( PyExc_TypeError, "Expected a filepath string" );
		return nullptr;
	}

	if( !IsValid() )
	{
		Py_RETURN_NONE;
	}

	return Py_BuildValue( "{sisisisisisisi}",
						  "Width",
						  GetWidth(),
						  "Height",
						  GetHeight(),
						  "Depth",
						  GetDepth(),
						  "MipLevels",
						  GetMipCount(),
						  "Format",
						  GetFormat(),
						  "ResourceType",
						  0,
						  "ImageFileFormat",
						  0 );
}

static PyObject* PythonSave( PyObject* self, PyObject* args, bool async )
{
	Tr2HostBitmap* pThis = BluePythonCast<Tr2HostBitmap*>( self );

	PyObject* file = nullptr;

	if( !PyArg_ParseTuple( args, "O", &file ) )
	{
		PyErr_SetString( PyExc_TypeError, "Expected a filepath string" );
		return nullptr;
	}

	std::wstring wcstr;
	std::string cstr;
	bool OK = false;
	if( PyUnicode_Check( file ) && BlueExtractArgument( file, wcstr, 1 ) )
	{
		if( async )
		{
			OK = pThis->SaveAsync( wcstr.c_str() );
		}
		else
		{
			OK = pThis->Save( wcstr.c_str() );
		}
	}
#if PY_MAJOR_VERSION == 2
	else if( PyString_Check( file ) && BlueExtractArgument( file, cstr, 1 ) )
	{
		if( async )
		{
			OK = pThis->SaveAsync( (const wchar_t*)CA2W( cstr.c_str() ) );
		}
		else
		{
			OK = pThis->Save( (const wchar_t*)CA2W( cstr.c_str() ) );
		}
	}
#endif
	else
	{
		PyErr_SetString( PyExc_TypeError, "Expected a filepath string" );
		return nullptr;
	}

	return PyBool_FromLong( OK );
}

static PyObject* PySaveAsync( PyObject* self, PyObject* args )
{
	return PythonSave( self, args, true );
}

static PyObject* PySave( PyObject* self, PyObject* args )
{
	return PythonSave( self, args, false );
}

static PyObject* PyCompareBitmaps( PyObject* self, PyObject* args )
{
	using namespace Tr2RenderContextEnum;

	PyObject* img1Obj = nullptr;
	PyObject* img2Obj = nullptr;
	PyObject* diffObj = nullptr;
	unsigned tolerance = 0;
	if( !PyArg_ParseTuple( args, "OO|iO", &img1Obj, &img2Obj, &tolerance, &diffObj ) )
	{
		return NULL;
	}

	Tr2HostBitmap* img1 = BluePythonCast<Tr2HostBitmap*>( img1Obj );
	if( !img1 )
	{
		PyErr_SetString( PyExc_TypeError, "CompareBitmaps: First parameter must be a Tr2HostBitmap object" );
		return NULL;
	}

	Tr2HostBitmap* img2 = BluePythonCast<Tr2HostBitmap*>( img2Obj );
	if( !img2 )
	{
		PyErr_SetString( PyExc_TypeError, "CompareBitmaps: Second parameter must be a Tr2HostBitmap object" );
		return NULL;
	}

	Tr2HostBitmap* diff = nullptr;

	if( diffObj )
	{
		diff = BluePythonCast<Tr2HostBitmap*>( diffObj );
		if( !diff )
		{
			PyErr_SetString( PyExc_TypeError, "CompareBitmaps: Third parameter must be a Tr2HostBitmap object" );
			return NULL;
		}
	}

	if( !img1->IsValid() || !img2->IsValid() )
	{
		PyErr_SetString( PyExc_TypeError, "CompareBitmaps: invalid source/destination bitmaps" );
		return NULL;
	}

	if( img1->GetWidth() != img2->GetWidth() ||
		img1->GetHeight() != img2->GetHeight() ||
		img1->GetFormat() != img2->GetFormat() ||
		img1->GetMipCount() != img2->GetMipCount() ||
		img1->GetType() != TEX_TYPE_2D ||
		img2->GetType() != TEX_TYPE_2D )
	{
		char buffer[2048];
		sprintf_s( buffer,
				   "CompareBitmaps only works between identical layout 2D bitmaps. "
				   " image1: %ux%ux%u %i %i image2: %ux%ux%u %i %i",
				   unsigned( img1->GetWidth() ),
				   unsigned( img1->GetHeight() ),
				   unsigned( img1->GetMipCount() ),
				   int( img1->GetFormat() ),
				   int( img1->GetType() ),
				   unsigned( img2->GetWidth() ),
				   unsigned( img2->GetHeight() ),
				   unsigned( img2->GetMipCount() ),
				   int( img2->GetFormat() ),
				   int( img2->GetType() ) );

		PyErr_SetString( PyExc_TypeError, buffer );
		return NULL;
	}

	if( GetBytesPerPixel( img1->GetFormat() ) != 4 )
	{
		// Do a byte for byte comparison, diff and tolerance don't work
		const uint8_t* d1 = (const uint8_t*)img1->GetRawData();
		const uint8_t* d2 = (const uint8_t*)img2->GetRawData();
		uint32_t count = 0;
		for( unsigned mip = 0; mip != img1->GetTrueMipCount(); ++mip )
		{
			uint32_t size = img1->GetMipSize( mip );
			while( size-- )
			{
				if( *d1++ != *d2++ )
				{
					++count;
				}
			}
		}

		return ToPython( count );
	}

	if( diff )
	{
		if( !diff->Create( img1->GetWidth(), img1->GetHeight(), img1->GetMipCount(), img1->GetFormat() ) )
		{
			PyErr_SetString( PyExc_TypeError, "CompareBitmaps: failed to create difference bitmap" );
			return NULL;
		}
	}

	unsigned count = 0;

	const uint8_t* d1 = (const uint8_t*)img1->GetRawData();
	const uint8_t* d2 = (const uint8_t*)img2->GetRawData();
	uint8_t* d3 = nullptr;
	if( diff )
	{
		d3 = (uint8_t*)diff->GetRawData();
	}

	for( uint32_t mip = 0; mip != img1->GetTrueMipCount(); ++mip )
	{
		for( uint32_t j = 0; j != img1->GetMipHeight( mip ); ++j )
		{
			for( uint32_t i = 0; i != img1->GetMipWidth( mip ); ++i )
			{
				for( uint32_t bpp = 0; bpp != 4; ++bpp, ++d1, ++d2 )
				{
					uint32_t delta = std::max( *d1, *d2 ) - std::min( *d1, *d2 );
					if( delta > tolerance )
					{
						++count;
					}
					if( d3 )
					{
						*d3 = delta;
						++d3;
					}
				}
			}
		}
	}

	return ToPython( count );
}

static PyObject* PyBgraToRgb( PyObject*, PyObject* args )
{
	Py_buffer buffer;
	if( !PyArg_ParseTuple( args, "s*", &buffer ) )
	{
		return nullptr;
	}
	ON_BLOCK_EXIT( [&] { PyBuffer_Release( &buffer ); } );

	auto count = buffer.len / 4;
	if( count < 1 )
	{
		PyErr_SetString( PyExc_ValueError, "Buffer is too short" );
		return nullptr;
	}

	std::unique_ptr<uint8_t[]> output( new uint8_t[count * 3] );
	auto src = static_cast<const uint8_t*>( buffer.buf );
	auto dst = output.get();
	for( Py_ssize_t i = 0; i < count; ++i )
	{
		dst[0] = src[2];
		dst[1] = src[1];
		dst[2] = src[0];
		dst += 3;
		src += 4;
	}
	return PyVerCompat::ToPyBytes( reinterpret_cast<const char*>( output.get() ), count * 3 );
}

#endif

const Be::ClassInfo* Tr2HostBitmap::ExposeToBlue()
{
	/////////////////////////////////////////
	// Blue class info
	EXPOSURE_BEGIN( Tr2HostBitmap, "" )

		MAP_INTERFACE( IRoot )
		MAP_INTERFACE( Tr2HostBitmap )

		MAP_METHOD_AND_WRAP(
			"IsValid",
			IsValid,
			"No arguments" )

		MAP_METHOD_AND_WRAP(
			"IsCompressed",
			IsCompressed,
			"No arguments" )

		MAP_METHOD_AND_WRAP(
			"Create",
			Create,
			":param width: bitmap width\n"
			":param height: bitmap height\n"
			":param mipCount: number of mip levels\n"
			":param format: pixel format (trinity.PIXEL_FORMAT)" )

		MAP_METHOD_AND_WRAP(
			"CreateCube",
			CreateCube,
			":param width: bitmap width\n"
			":param mipCount: number of mip levels\n"
			":param format: pixel format (trinity.PIXEL_FORMAT)" )

		MAP_METHOD_AND_WRAP(
			"CreateVolume",
			CreateVolume,
			":param width: bitmap width\n"
			":param height: bitmap height\n"
			":param depth: bitmap depth\n"
			":param mipCount: number of mip levels\n"
			":param format: pixel format (trinity.PIXEL_FORMAT)" )

		MAP_METHOD_AND_WRAP(
			"CopyFromRenderTarget",
			CopyFromRenderTargetPython,
			"Copy a non-MSAA renderTarget back to the CPU host.\n"
			":param source: Tr2RenderTarget of the same pixelFormat, and correct width/height\n" )

		MAP_METHOD_AND_WRAP(
			"CopyFromRenderTargetRegion",
			CopyFromRenderTargetRegionPython,
			"Copy a non-MSAA renderTarget back to the CPU host.\n"
			":param source: Tr2RenderTarget of the same pixelFormat, and correct width/height\n"
			":param left: Source rectangle left offset in pixels\n"
			":param top: Source rectangle top offset in pixels\n"
			":param right: Source rectangle right offset in pixels\n"
			":param bottom: Source rectangle bottom offset in pixels\n"
			":param x: X offset in the bitmap in pixels\n"
			":param y: Y offset in the bitmap in pixels" )

		MAP_METHOD_AND_WRAP(
			"CopyFaceFromRenderTarget",
			CopyFaceFromRenderTargetPython,
			"Copy a non-MSAA renderTarget back to the CPU host.\n"
			":param face: cube map face\n"
			":param source: Tr2RenderTarget of the same pixelFormat, and correct width/height\n" )

		MAP_METHOD_AND_WRAP(
			"CopyFromTextureRes",
			CopyFromTextureResPython,
			"Copy a TriTextureRes back to the CPU host.\n"
			":param source: TriTextureRes of the same pixelFormat, and correct width/height\n" )

		MAP_METHOD_AND_WRAP(
			"CreateFromHeightData",
			CreateFromHeightData,
			"Creates a height map by scaling up source data to fill the bitmap.\n"
			":param data: source data, values from 0 to 1\n"
			":param width: width of the source data\n"
			":param height: height of the source data\n" )


		MAP_METHOD(
			"GetRawData",
			PyGetRawData,
			"Returns a tuple with (raw data pointer, width, height, pitch in bytes)\n"
			":rtype: bool | (buffer, int, int, int)" )

		MAP_METHOD(
			"GetMipRawData",
			PyGetMipRawData,
			"Returns a tuple with (raw data pointer, width, height, pitch in bytes) of the specified mip level\n"
			":param level: mip level\n"
			":type level: int\n"
			":rtype: bool | (buffer, int, int, int)" )

		MAP_METHOD(
			"SetMipRawData",
			PySetMipRawData,
			"Copies raw mip data to the bitmap\n"
			":param mip: mip level index\n"
			":type mip: int\n"
			":param data: single-segment buffer\n"
			":type data: buffer\n"
			":rtype: None" )

		MAP_METHOD_AND_WRAP(
			"PopulateMargin",
			PopulateMargin,
			"Assume that the bitmap has a margin-wide border around it, and clamp the border contents of\n"
			"(margin,margin)...(width-margin,height-margin) into this border.\n"
			":param margin: unsigned, must be less than min(width/2, height/2)" )

		MAP_ATTRIBUTE( "name", m_name, "", Be::READWRITE | Be::PERSIST );

		MAP_ATTRIBUTE( "width", m_width, "", Be::READ );
		MAP_ATTRIBUTE( "height", m_height, "", Be::READ );
		MAP_ATTRIBUTE( "mipCount", m_mipCount, "", Be::READ );
		MAP_ATTRIBUTE( "imageType", m_type, "", Be::READ );

		MAP_ATTRIBUTE_WITH_CHOOSER( "format", m_format, "", Be::READ, Tr2RenderContextEnum_PixelFormat_Chooser );

		MAP_PROPERTY_READONLY( "isValid", IsValid, "IsValid" );

		MAP_METHOD(
			"__init__",
			PyInit,
			"Has three possible overloads:\n"
			"(1) Tr2HostBitmap() - creates an empty bitmap, call Create later\n"
			"(2) Tr2HostBitmap(source) - creates a bitmap and copies data from source (RT, texture)\n"
			"(3) Tr2HostBitmap(width, height, mipCount, format) - creates a new 2D bitmap\n"
			":param sourceOrWidth: source for (2) overload or width for (3) overload\n"
			":type sourceOrWidth: Optional[Tr2RenderTarget | TriTextureRes | Tr2AtlasTexture | int]\n"
			":param height: bitmap height\n"
			":type height: Optional[int]\n"
			":param mipCount: number of mip levels\n"
			":type mipCount: Optional[int]\n"
			":param format: pixel format (trinity.PIXEL_FORMAT)\n"
			":type format: Optional[int]\n" )

		MAP_METHOD(
			"Save",
			PySave,
			"Save to a file\n"
			":param path: full path (not a res path)\n"
			":type path: basestring\n"
			":rtype: bool" )

		MAP_METHOD(
			"SaveAsync",
			PySaveAsync,
			"Asynchronous save to a file\n"
			":param path: full path (not a res path)\n"
			":type path: basestring\n"
			":rtype: bool" )

		MAP_METHOD_AND_WRAP(
			"CountPixelsOfValue",
			CountPixelsOfValue,
			"Count number of pixels that have a specific value in selected channels.\n"
			":param channels: String containing one or more of 'r', 'g', 'b', 'a' characters.\n"
			":param value: Value to count for masked pixels" )

		MAP_METHOD_AND_WRAP(
			"IsMonochrome",
			IsMonochrome,
			"Return boolean value representing whether image contains same values in R, G, B channels for all pixels.\n" )

		MAP_METHOD_AS_METHOD(
			"CreateFromFile",
			PyCreateFromFile,
			"Create a hostbitmap with the contents of this image file.\n"
			":param path: full path or res path of an image supported by Trinity" )

		MAP_METHOD_AND_WRAP(
			"Compress",
			Compress,
			"Compress the contents of the hostbitmap into a TriTextureRes.\n"
			":param compressionFormat: see SquishLib\n"
			":param compressionQuality: see SquishLib\n"
			":param output: an empty TriTextureRes" )

		MAP_METHOD_AND_WRAP(
			"Downsample2x2",
			Downsample2x2,
			"Do a destructive, in-place downsampling with a simple 2x2 box filter." )

		MAP_METHOD_AND_WRAP(
			"Crop",
			Crop,
			"Crop a single miplevel 2D bitmap in uncompressed format to a specified rectangle.\n"
			":param left: left coordinate of crop rectangle\n"
			":param top: top coordinate of crop rectangle\n"
			":param right: right coordinate of crop rectangle\n"
			":param bottom: bottom coordinate of crop rectangle" )

		MAP_METHOD_AND_WRAP(
			"ConvertToVolume",
			ConvertToVolume,
			"Special function to convert a 2d representation of a 3d bitmap into a real 3d bitmap." )

		MAP_METHOD_AS_METHOD(
			"SetPixel",
			PySetPixel,
			"[DEPRECATED] Sets a pixel at arg1, arg2 to arg3" )

		MAP_METHOD_AS_METHOD(
			"GetPixel",
			PyGetPixel,
			"[DEPRECATED] Returns the pixel at arg1, arg2" )


		MAP_METHOD_AND_WRAP( "IsSaving", IsSaving, "Is an asynchronous save in progress?" );
		MAP_METHOD_AND_WRAP( "IsSaveCompleted", IsSaveCompleted, "Is an asynchronous save completed?" );
		MAP_METHOD_AND_WRAP( "IsSaveSucceeded", IsSaveSucceeded, "Did an asynchronous save succeed?" );
		MAP_METHOD_AND_WRAP( "WaitForSave", WaitForSave, "Block until an asynchronous save completed." );

		MAP_METHOD_AND_WRAP(
			"ChangeFormat",
			ChangeFormatFromScript,
			"Changes pixel format of a valid bitmap. Both the old and the new format must be\n"
			"uncompressed and BPP of the new format must be the same as for the old one.\n"
			"The function simply changes the format and doesn't do any image conversion.\n"
			":param format: new pixel format (trinity.PIXEL_FORMAT)" );

	EXPOSURE_END()
}

#if BLUE_WITH_PYTHON

MAP_FUNCTION(
	"CompareBitmaps",
	PyCompareBitmaps,
	"Compares two bitmaps and returns the number of pixel elements (not pixels) that exceed\n"
	"a given threshold.\n"
	"count = CompareBitmaps(img1, img2, tolerance, diff)\n\n"
	":param img1: Tr2HostBitmap object\n"
	":type img1: Tr2HostBitmap\n"
	":param img2: Tr2HostBitmap object\n"
	":type img2: Tr2HostBitmap\n"
	":param tolerance: tolerance level. Ignored for non-RGBA8 images.\n"
	":type tolerance: Optional[int]\n"
	":param diff: optional Tr2HostBitmap object for receiving difference image. Ignored for non-RGBA8.\n"
	":type diff: Optional[Tr2HostBitmap]\n"
	":returns: Number of pixel elements that exceed a given threshold.\n"
	"  If diff is passed in, its pixels are set to difference between levels of corresponding pixels\n"
	"  of both bitmaps.\n"
	":rtype: int" );

MAP_FUNCTION(
	"BgraToRgb",
	PyBgraToRgb,
	"Converts a buffer containing B8G8R8A8 pixel values to R8G8B8 buffer\n"
	":param pixels: readable buffer with pixel color values\n"
	":type pixels: buffer | str\n"
	":rtype: str" );

#endif
