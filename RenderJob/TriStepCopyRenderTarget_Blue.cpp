#include "StdAfx.h"
#include "TriStepCopyRenderTarget.h"
#include "Tr2RenderTarget.h"
#include "Resources/TriTextureRes.h"
#include "TriViewport.h"

BLUE_DEFINE( TriStepCopyRenderTarget );

// We can't use MAP_METHOD_AND_WRAP as this is expecting an interface
// pointer as an argument
#if BLUE_WITH_PYTHON
PyObject* TriStepCopyRenderTarget::PyInitLowLevel( PyObject* self, PyObject* args )
{
	TriStepCopyRenderTarget* pThis = BluePythonCast<TriStepCopyRenderTarget*>( self );

	PyObject* src = NULL;
	PyObject* dest = NULL;
	PyObject* vpSrc = NULL;
	PyObject* vpDst = NULL;

	if( !PyArg_ParseTuple( args, "|OOOO", &dest, &src, &vpDst, &vpSrc ) )
	{
		return nullptr;
	}

	Tr2RenderTarget* destRT;
	TriTextureRes* destRes;
	if( dest )
	{
		if( BluePythonCast<Tr2RenderTarget*>( dest ) && BlueExtractArgument( dest, destRT, 1 ) )
		{
			pThis->m_destinationRT = destRT;
		}
		else if( BluePythonCast<TriTextureRes*>( dest ) && BlueExtractArgument( dest, destRes, 1 ) )
		{
			pThis->m_destinationTexture = destRes;
		}
	}

	Tr2RenderTarget* srcRT;
	if( src )
	{
		if( !BlueExtractArgument( src, srcRT, 2 ) )
		{
			return NULL;
		}
		else
		{
			pThis->m_sourceRT = srcRT;
		}
	}

	TriViewport* viewport;
	if( vpDst )
	{
		if( !BlueExtractArgument( vpDst, viewport, 3 ) )
		{
			return nullptr;
		}
		else
		{
			pThis->m_destinationViewport = viewport;
		}
	}
	if( vpSrc )
	{
		if( !BlueExtractArgument( vpSrc, viewport, 4 ) )
		{
			return nullptr;
		}
		else
		{
			pThis->m_sourceViewport = viewport;
		}
	}
	

	Py_RETURN_NONE;
}
#endif

const Be::ClassInfo* TriStepCopyRenderTarget::ExposeToBlue()
{
	EXPOSURE_BEGIN( TriStepCopyRenderTarget, "" )
		MAP_INTERFACE( TriStepCopyRenderTarget )

		MAP_ATTRIBUTE( "Destination", m_destinationRT, "", Be::READWRITE)
		MAP_ATTRIBUTE( "destinationTexture", m_destinationTexture, "", Be::READWRITE)
		MAP_ATTRIBUTE( "Source", m_sourceRT, "", Be::READWRITE)
		MAP_ATTRIBUTE( "sourceViewport", m_sourceViewport, "", Be::READWRITE)
		MAP_ATTRIBUTE( "destinationViewport", m_destinationViewport, "", Be::READWRITE)

		MAP_METHOD( 
			"__init__", 
			PyInitLowLevel, 
			":param dest: copy destination\n" 
			":type dest: Optional[Tr2RenderTarget | TriTextureRes]\n"
			":param source: copy source\n" 
			":type source: Optional[Tr2RenderTarget]\n"
			":param destViewport: destination rectangle\n" 
			":type destViewport: Optional[TriViewport]\n"
			":param sourceViewport: source rectangle\n" 
			":type sourceViewport: Optional[TriViewport]\n"
			)

	EXPOSURE_CHAINTO( TriRenderStep )
}
