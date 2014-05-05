////////////////////////////////////////////////////////////
//
//    Created:   August 2013
//    Copyright: CCP 2013
//
#include "StdAfx.h"
#include "EveSOFDataMgr.h"

#include "TriPythonContext.h"

BLUE_DEFINE( EveSOFDataMgr );

PyObject* PySetData( PyObject* self, PyObject* args )
{
	TriPythonContext pythonCtx;
	EveSOFDataMgr* pThis = BluePythonCast<EveSOFDataMgr*>( self );

	PyObject* pyDB = NULL;
	if( !PyArg_ParseTuple( args, "O", &pyDB ) )
	{
		return NULL;
	}

	IRoot* db = NULL;
	if( !BlueExtractArgument( pyDB, db, 1 ) )
	{
		return NULL;
	}

	if( db == NULL )
	{
		return NULL;
	}

	pThis->SetData( db );

	Py_RETURN_NONE;
}


const Be::ClassInfo* EveSOFDataMgr::ExposeToBlue()
{
    EXPOSURE_BEGIN( EveSOFDataMgr, "" )
        MAP_INTERFACE( EveSOFDataMgr )

		MAP_METHOD_AND_WRAP( "LoadData", LoadData, "Inject all the data into this mgr, providing a redfile path" )
		MAP_METHOD( "SetData", PySetData, "Inject all the data into this mgr, providing a blue object" )

		MAP_METHOD_AND_WRAP( "HasFactionData", HasFactionData, "Does this faction exist?" )
		MAP_METHOD_AND_WRAP( "HasHullData", HasHullData, "Does this hull exist?" )
		MAP_METHOD_AND_WRAP( "HasRaceData", HasRaceData, "Does this race exist?" )

    EXPOSURE_END()
}
