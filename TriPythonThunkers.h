/* 
	*************************************************************************

	TrinityPythonThunkers.h

	Author:    Hilmar Veigar Pétursson
	Created:   Okt. 2001
	OS:        Win32
	Project:   Trinity

	Description:   

		Generic interface thunkers


	Dependencies:

		Trinity

	(c) CCP 2000

	*************************************************************************
*/

#ifndef _TRIPYTHONTHUNKERS_H_
#define _TRIPYTHONTHUNKERS_H_

#include "include/ITriFunction.h"

//////////////////////////////////////////////////////////////////////
// ITriScalarFunction Thunkers
//////////////////////////////////////////////////////////////////////
class ITriScalarFunction_Thunk : public ITriScalarFunction
{
public:

	typedef ITriScalarFunction_Thunk _Class;
	typedef ITriScalarFunction _Interface;

	static const Be::IID& IID()
	{
		return BlueInterfaceIID<ITriScalarFunction>();
	}

	const Be::Clsid* Clsid()
	{
		return ClassType()->mClassId;
	}

	static const PyMethodDef* Defs()
	{
		THUNKER_BEGIN()
			MAPPYTHON
			( 
				UpdateScalar,                    
				"use:  f = curve.Update( t )\r\n" 
				"pre:  't' is of type double or Be::Time.\r\n" 
				"post: 'curve.value' is now 'f' which is the value at time 't'." 
			)
			MAPPYTHON
			( 
				GetScalarAt,                
				"use:  f = curve.GetValueAt( t )\r\n" 
				"pre:  't' is of type double or Be::Time.\r\n" 
				"post: 'f' is the value of 'curve' at time 't'." 
			)
/*			MAPPYTHON
			( 
				GetValueDotAt,       
				"Get the value of the of the differential function" 
			)
			MAPPYTHON
			( 
				GetValueDoubleDotAt, 
				"Get the value of the of the double differential function" 
			)*/
		THUNKER_END()
	}

	// compatible python methods
	DECLARE_PYMETHODTHUNK( UpdateScalar );
	DECLARE_PYMETHODTHUNK( GetScalarAt );
//	DECLARE_PYMETHODTHUNK( GetValueDotAt );
//	DECLARE_PYMETHODTHUNK( GetValueDoubleDotAt );
};



//////////////////////////////////////////////////////////////////////
// ITriColorFunction Thunkers
//////////////////////////////////////////////////////////////////////
class ITriColorFunction_Thunk : public ITriColorFunction
{
public:

	typedef ITriColorFunction_Thunk _Class;
	typedef ITriColorFunction _Interface;

	static const Be::IID& IID()
	{
		return BlueInterfaceIID<ITriColorFunction>();
	}

	const Be::Clsid* Clsid()
	{
		return ClassType()->mClassId;
	}

	static const PyMethodDef* Defs()
	{
		THUNKER_BEGIN()
			MAPPYTHON
			( 
				UpdateColor,                    
				"use:  f = curve.Update( t )\r\n" 
				"pre:  't' is of type double or Be::Time.\r\n" 
				"post: 'curve.value' is now 'f' which is the value at time 't'." 
			)
			MAPPYTHON
			( 
				GetColorAt,                
				"use:  f = curve.GetValueAt( t )\r\n" 
				"pre:  't' is of type double or Be::Time.\r\n" 
				"post: 'f' is the value of 'curve' at time 't'." 
			)
		THUNKER_END()
	}

	// compatible python methods
	DECLARE_PYMETHODTHUNK( UpdateColor );	
	DECLARE_PYMETHODTHUNK( GetColorAt );
};


//////////////////////////////////////////////////////////////////////
// ITriVectorFunction Thunkers
//////////////////////////////////////////////////////////////////////
class ITriVectorFunction_Thunk : public ITriVectorFunction
{
public:

	typedef ITriVectorFunction_Thunk _Class;
	typedef ITriVectorFunction _Interface;

	static const Be::IID& IID()
	{
		return BlueInterfaceIID<ITriVectorFunction>();
	}

	const Be::Clsid* Clsid()
	{
		return ClassType()->mClassId;
	}

	static const PyMethodDef* Defs()
	{
		THUNKER_BEGIN()
			MAPPYTHON
			( 
				UpdateVector,                    
				"use:  f = curve.Update( t )\r\n" 
				"pre:  't' is of type double or Be::Time.\r\n" 
				"post: 'curve.value' is now 'f' which is the value at time 't'." 
			)
			MAPPYTHON
			( 
				GetVectorAt,                
				"use:  f = curve.GetValueAt( t )\r\n" 
				"pre:  't' is of type double or Be::Time.\r\n" 
				"post: 'f' is the value of 'curve' at time 't'." 
			)
			MAPPYTHON
			( 
				GetVectorDotAt,       
				"Get the value of the of the differential function" 
			)
			MAPPYTHON
			( 
				GetVectorDoubleDotAt, 
				"Get the value of the of the double differential function" 
			)
		THUNKER_END()
	}

	// compatible python methods
	DECLARE_PYMETHODTHUNK( UpdateVector );
	DECLARE_PYMETHODTHUNK( GetVectorIntegralAt );
	DECLARE_PYMETHODTHUNK( GetVectorAt );
	DECLARE_PYMETHODTHUNK( GetVectorDotAt );
	DECLARE_PYMETHODTHUNK( GetVectorDoubleDotAt );
};


//////////////////////////////////////////////////////////////////////
// ITriQuaternionFunction Thunkers
//////////////////////////////////////////////////////////////////////
class ITriQuaternionFunction_Thunk : public ITriQuaternionFunction
{
public:

	typedef ITriQuaternionFunction_Thunk _Class;
	typedef ITriQuaternionFunction _Interface;

	static const Be::IID& IID()
	{
		return BlueInterfaceIID<ITriQuaternionFunction>();
	}

	const Be::Clsid* Clsid()
	{
		return ClassType()->mClassId;
	}

	static const PyMethodDef* Defs()
	{
		THUNKER_BEGIN()
			MAPPYTHON
			( 
				UpdateQuaternion,                    
				"use:  f = curve.Update( t )\r\n" 
				"pre:  't' is of type double or Be::Time.\r\n" 
				"post: 'curve.value' is now 'f' which is the value at time 't'." 
			)
			MAPPYTHON
			( 
				GetQuaternionAt,                
				"use:  f = curve.GetValueAt( t )\r\n" 
				"pre:  't' is of type double or Be::Time.\r\n" 
				"post: 'f' is the value of 'curve' at time 't'." 
			)
			MAPPYTHON
			( 
				GetQuaternionDotAt,       
				"Get the value of the of the differential function" 
			)
			MAPPYTHON
			( 
				GetQuaternionDoubleDotAt, 
				"Get the value of the of the double differential function" 
			)
		THUNKER_END()
	}

	// compatible python methods
	DECLARE_PYMETHODTHUNK( UpdateQuaternion );
	DECLARE_PYMETHODTHUNK( GetQuaternionAt );
	DECLARE_PYMETHODTHUNK( GetQuaternionDotAt );
	DECLARE_PYMETHODTHUNK( GetQuaternionDoubleDotAt );
};





#endif