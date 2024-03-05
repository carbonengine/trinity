#include "StdAfx.h"

#ifdef _WIN32

#include "Tr2NamedPipeListener.h"
#include "Resources/TriGeometryRes.h"

BLUE_DEFINE( Tr2NamedPipeListener );

#if BLUE_WITH_PYTHON
static PyObject * PyApplyAsGeometry( PyObject * self, PyObject * args )    
{
	Tr2NamedPipeListener* pThis = BluePythonCast<Tr2NamedPipeListener*>( self );

	TriGeometryRes* geoRes = nullptr;
	
	PyObject *pyRes;
	if( !PyArg_ParseTuple( args, "O", &pyRes ))
	{
		return NULL;
	}

	if( pyRes )
	{
		if( !BlueExtractArgument( pyRes, geoRes, 0 ))
		{
			return NULL;
		}
	}

	// Extract trigeometryres
	granny_file* gfile = GrannyReadEntireFileFromMemory( pThis->m_size, pThis->m_buffer );
	if( !gfile )
	{
		return NULL;
	}

	granny_file_info* ginfo = GrannyGetFileInfo( gfile );
	geoRes->InitializeFromMemory( ginfo );
	geoRes->DoPrepareFromMemory();
	Py_RETURN_NONE;
}
#endif

const Be::ClassInfo* Tr2NamedPipeListener::ExposeToBlue()
{
    EXPOSURE_BEGIN( Tr2NamedPipeListener, "" )
        MAP_INTERFACE( Tr2NamedPipeListener )
		MAP_ATTRIBUTE( "pipeName", m_pipeName, "", Be::READ )
		MAP_ATTRIBUTE( "size", m_size, "", Be::READ )
		MAP_ATTRIBUTE( "waiting", m_waiting, "", Be::READ )
		MAP_METHOD_AND_WRAP( "Listen", Listen, 
		"Listen for data on a target named pipe"
		"\n"
		"\n:param name: The name of the pipe");
	MAP_METHOD_AND_WRAP( "Clear", Clear, 
		"Clear the internal data of listener");

	MAP_METHOD_AND_WRAP( "SetCallback", SetCallback, 
		"Add a python callback for when the pipe has been read.\n"
		":param cb: callback function"
		);
	MAP_METHOD( 
		"ApplyAsGeometry", 
		PyApplyAsGeometry, 
		"( trigeores )\n"
		"Apply the inner buffer to a gr2 resource.\n"
		":param trigeores: geometry resource\n"
		":type trigeores: None | TriGeometryRes\n"
		":rtype: None"
	);
    EXPOSURE_END()
}

#endif
