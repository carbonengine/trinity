#include "StdAfx.h"
#include "TriRigidOrientation.h"

BLUE_DEFINE( TriTorque );
BLUE_DEFINE( TriRigidOrientation );

const Be::ClassInfo* TriTorque::ExposeToBlue()
{
	EXPOSURE_BEGIN(TriTorque, "no comment")

		MAP_ATTRIBUTE( "time",  mTime,  "na", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "rot0", mRot0, "na", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "omega0", mOmega0, "na", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "torque", mTorque, "na", Be::READWRITE | Be::PERSIST )

	EXPOSURE_END()
}

const Be::ClassInfo* TriRigidOrientation::ExposeToBlue()
{
	EXPOSURE_BEGIN(TriRigidOrientation, "no comment\n:jessica-deprecated:")
		MAP_INTERFACE(ITriFunction)
		MAP_INTERFACE(ITriQuaternionFunction)

		////////////////////////////////////////////////////////////////////////////
		//               name
		MAP_ATTRIBUTE
		(  
			"name",          
			mName,     
			"Yes you can name your rigid rotation", 
			Be::READWRITE | Be::PERSIST 
		)
		
		////////////////////////////////////////////////////////////////////////////
		//               I
		MAP_ATTRIBUTE
		( 
			"I", 
			mI, 
			"moment of inertia", 
			Be::READWRITE | Be::PERSIST 
		)		

		////////////////////////////////////////////////////////////////////////////
		//               drag
		MAP_ATTRIBUTE
		( 
			"drag", 
			mDrag, 
			"", 
			Be::READWRITE | Be::PERSIST 
		)		

		////////////////////////////////////////////////////////////////////////////
		//               value
		MAP_ATTRIBUTE
		( 
			"value",       
			mValue,       
			"na", 
			Be::READWRITE | Be::PERSIST
		)	

		////////////////////////////////////////////////////////////////////////////
		//               start
		MAP_ATTRIBUTE
		( 
			"start",         
			mStart,        
			"", 
			Be::READWRITE | Be::PERSIST 
		)		

		////////////////////////////////////////////////////////////////////////////
		//               forces
		MAP_ATTRIBUTE
		(  
			"states",          
			mStates,          
			"na", 
			Be::READ | Be::PERSIST
		)

		////////////////////////////////////////////////////////////////////////////
		//               Sort
		MAP_METHOD_AND_WRAP
		(
            "Sort",
			Sort,
			"Sorts the force list, based on time" 
		)

	EXPOSURE_END()

}

