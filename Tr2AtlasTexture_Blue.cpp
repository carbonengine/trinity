#include "StdAfx.h"
#include "Tr2AtlasTexture.h"
#include "Resources/TriTextureRes.h"
#include "Tr2HostBitmap.h"

BLUE_DEFINE( Tr2AtlasTexture );

IBlueResource* CreateTr2AtlasTextureRes( const wchar_t* name )
{
	Tr2AtlasTexturePtr p;
	p.CreateInstance();
	return p.Detach();
}

BLUE_REGISTER_RESOURCE_EXTENSION( L"ddsatlas", CreateTr2AtlasTextureRes );
BLUE_REGISTER_RESOURCE_EXTENSION( L"sddatlas", CreateTr2AtlasTextureRes );
BLUE_REGISTER_RESOURCE_EXTENSION( L"pngatlas", CreateTr2AtlasTextureRes );
BLUE_REGISTER_RESOURCE_EXTENSION( L"jpgatlas", CreateTr2AtlasTextureRes );
BLUE_REGISTER_RESOURCE_EXTENSION( L"jpegatlas", CreateTr2AtlasTextureRes );

#if BLUE_WITH_PYTHON
static PyObject* PyLockBuffer( PyObject* self, PyObject* args )
{
	Tr2AtlasTexture* pThis = BluePythonCast<Tr2AtlasTexture*>( self );

	void* pData = NULL;
	unsigned int pitch = 0;
	if( pThis->LockBuffer( pData, pitch ) )
	{
		PyObject *buffer = PyBuffer_FromReadWriteMemory( pData, pThis->GetHeight() * pitch);

		PyObject *result = PyTuple_New(4);
		if (!result) {
			Py_DECREF(buffer);
			return nullptr;
		}
		PyTuple_SET_ITEM( result, 0, buffer );
		PyTuple_SET_ITEM( result, 1, PyInt_FromLong( pThis->GetWidth() ) );
		PyTuple_SET_ITEM( result, 2, PyInt_FromLong( pThis->GetHeight() ) );
		PyTuple_SET_ITEM( result, 3, PyInt_FromLong( pitch ) );

		return result;
	}

	Py_RETURN_FALSE;
}


static PyObject* PyLockBufferAndMargin( PyObject* self, PyObject* args )
{
	Tr2AtlasTexture* pThis = BluePythonCast<Tr2AtlasTexture*>( self );

	void* data = nullptr;
	unsigned pitch = 0;
	unsigned margin = 0;
	if( pThis->LockBufferAndMargin( data, pitch, margin ) )
	{
		const int height = pThis->GetHeight() + margin * 2;
		const int width = pThis->GetWidth() + margin * 2;

		PyObject *buffer = PyBuffer_FromReadWriteMemory( data, height * pitch);

		PyObject *result = PyTuple_New(5);
		if (!result) {
			Py_DECREF(buffer);
			return nullptr;
		}
		PyTuple_SET_ITEM( result, 0, buffer );
		PyTuple_SET_ITEM( result, 1, PyInt_FromLong( width ) );
		PyTuple_SET_ITEM( result, 2, PyInt_FromLong( height ) );
		PyTuple_SET_ITEM( result, 3, PyInt_FromLong( pitch ) );
		PyTuple_SET_ITEM( result, 4, PyInt_FromLong( margin ) );

		return result;
	}

	Py_RETURN_FALSE;
}

static PyObject* PyGetTextureAtlas( PyObject* self, PyObject* args )
{
	Tr2AtlasTexture* pThis = BluePythonCast<Tr2AtlasTexture*>( self );

	TriTextureResPtr tmpRes;
	tmpRes.CreateInstance();
	if( pThis->GetTexture() )
	{
		tmpRes->SetTexture( *pThis->GetTexture() );
	}
	
	BluePythonObject* obj = PyOS->WrapBlueObject( tmpRes->GetRawRoot() );
	return obj;
}
#endif

const Be::ClassInfo* Tr2AtlasTexture::ExposeToBlue()
{
    EXPOSURE_BEGIN( Tr2AtlasTexture, "" )
        MAP_INTERFACE( Tr2AtlasTexture )
		MAP_INTERFACE( IBlueResource )
		MAP_INTERFACE( ICacheable )
		MAP_ICACHEABLE_METHODS()


		MAP_METHOD
		(
			"LockBuffer",
			PyLockBuffer,
			"Locks the surface behind the atlas texture.\n"
			"Returns a tuple with (address, width, height, pitch)"
		)
		
		MAP_METHOD
		(
			"LockBufferAndMargin",
			PyLockBufferAndMargin,
			"Locks the surface behind the atlas texture, including margin.\n"
			"Returns a tuple with (address, width, height, pitch, margin)"
		)

		MAP_METHOD_AND_WRAP
		(
			"UnlockBuffer",
			UnlockBuffer,
			"Unlocks the surface previously locked with LockBuffer"
		)

		MAP_METHOD
		(
			"GetTextureAtlas",
			PyGetTextureAtlas,
			"Returns a handle to the texture atlas that this atlas texture is a part of."
		)

		MAP_METHOD_AND_WRAP
		(
			"CopyFromHostBitmap",
			CopyFromHostBitmap,
			"Copies the contents of the host bitmap into the atlas texture.\n"
			"Arguments:\n"
			"hostBitmap - Tr2HostBitmap of exactly the same pixelformat as the atlas texture.\n"
			"             The size needs to be either an exact match, in which case the texture is updated;\n"
			"             _or_ it can be 2xmargin pixels wider and taller, in which case the margin in the\n"
			"			 parent texture atlas is also overwritten."
		)

		MAP_ATTRIBUTE
		(
			"name", 
			m_name, 
			"Name of this texture atlas entry", 
			Be::READWRITE | Be::PERSIST
		)

		MAP_ATTRIBUTE
		(
			"resPath", 
			m_resPath, 
			"Resource path behind this texture atlas entry.", 
			Be::READ
		)

		MAP_ATTRIBUTE
		(
			"width",
			m_width,
			"Width of this texture.",
			Be::READ
		)

		MAP_ATTRIBUTE
		(
			"height",
			m_height,
			"Height of this texture.",
			Be::READ
		)

		MAP_ATTRIBUTE
		(
			"atlasWidth",
			m_textureWidth,
				"Width of the entire atlas this texture is a part of.",
			Be::READ
		)

		MAP_ATTRIBUTE
		(
			"atlasHeight",
			m_textureHeight,
			"Height of the entire atlas this texture is a part of.",
			Be::READ
		)

		MAP_ATTRIBUTE
		(
			"x",
			m_x,
			"x coordinate offset into the atlas.",
			Be::READ
		)

		MAP_ATTRIBUTE
		(
			"y",
			m_y,
			"y coordinate offset into the atlas.",
			Be::READ
		)

		MAP_ATTRIBUTE
		(
			"atlas",
			m_textureAtlas,
			"Texture atlas containing this texture",
			Be::READ
		)

		MAP_PROPERTY
		(
			"isStandAlone",
			IsStandAlone, SetStandAlone,
			"A stand-alone atlas texture is really not an atlas texture - it's a permanent outsider.\n"
			"This is to support textures that need texture transformations - clamping and repeat\n"
			"is currently too expensive to support for atlas textures."
		)

    EXPOSURE_CHAINTO( BlueAsyncRes )
}
