#include "StdAfx.h"
#include "TriStepRenderTexture.h"
#include "Tr2AtlasTexture.h"
#include "Tr2RenderTarget.h"
#include "Tr2DepthStencil.h"

BLUE_DEFINE( TriStepRenderTexture );

#if BLUE_WITH_PYTHON
static PyObject* py__init__( PyObject* self, PyObject* args )
{
	TriStepRenderTexture* pThis = BluePythonCast<TriStepRenderTexture*>( self );

	PyObject* pyTexture = NULL;

	if( !PyArg_ParseTuple( args, "|O", &pyTexture ) )
	{
		return NULL;
	}

	if( pyTexture && pyTexture != Py_None )
	{
		Tr2RenderTarget* rt = BluePythonCast<Tr2RenderTarget*>( pyTexture );
		if( rt )
		{
			pThis->SetTexture( rt );
			Py_RETURN_NONE;
		}

		Tr2DepthStencil* ds = BluePythonCast<Tr2DepthStencil*>( pyTexture );
		if( ds )
		{
			pThis->SetTexture( ds );
			Py_RETURN_NONE;
		}

		ITr2TextureProvider* texture = BluePythonCast<ITr2TextureProvider*>( pyTexture );
		if( texture )
		{
			pThis->SetTexture( texture );
			Py_RETURN_NONE;
		}

		Tr2AtlasTexture* atlasTexture = BluePythonCast<Tr2AtlasTexture*>( pyTexture );
		if( atlasTexture )
		{
			pThis->SetTexture( atlasTexture );
			Py_RETURN_NONE;
		}

		PyErr_SetString( PyExc_TypeError, "Expected a Tr2RenderTarget, Tr2AtlasTexture, or TriTextureRes" );
		return NULL;
	}

	Py_RETURN_NONE;
}
#endif

const Be::ClassInfo* TriStepRenderTexture::ExposeToBlue()
{
	EXPOSURE_BEGIN(TriStepRenderTexture, "" )

		MAP_INTERFACE( TriRenderStep )
		MAP_INTERFACE( TriStepRenderTexture )

		MAP_ATTRIBUTE( "texture", m_texture,"TriTextureRes that will be rendered", Be::READWRITE )
		MAP_ATTRIBUTE( "renderTarget", m_texture,"Tr2RenderTarget that will be rendered", Be::READWRITE )
		MAP_ATTRIBUTE( "depthStencil", m_texture,"Tr2DepthStencil that will be rendered", Be::READWRITE )
		MAP_ATTRIBUTE( "tlTexCoord", m_tlTexCoord, "top left texture coordinate", Be::READWRITE )
		MAP_ATTRIBUTE( "brTexCoord", m_brTexCoord, "bottom right texture coordinate", Be::READWRITE )

		MAP_ATTRIBUTE( "textureSize", m_textureSize, "texture size", Be::READ )

		MAP_ATTRIBUTE( "failClearColor", m_failClearColor, "color to use when clearing due to a failed blit", Be::READWRITE )

		MAP_METHOD
		(
			"__init__", 
			py__init__, 
			"Creates a render step that renders a texture\n" 
			":param texture: texture to render\n"
			":type texture: Optional[ITr2TextureProvider | Tr2AtlasTexture]"
		)

	EXPOSURE_CHAINTO( TriRenderStep )
}
