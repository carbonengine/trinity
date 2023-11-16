#include "StdAfx.h"
#ifndef GAMEWORLD_64

#include "GrannyBoneOffset.h"

BLUE_DEFINE( GrannyBoneOffset );

const Be::ClassInfo* GrannyBoneOffset::ExposeToBlue()
{
    EXPOSURE_BEGIN( GrannyBoneOffset, "" )
		MAP_INTERFACE( GrannyBoneOffset )
		MAP_INTERFACE( IInitialize )

		MAP_METHOD_AND_WRAP( 
			"SetOffset",
			SetOffset,
			"Set an offset for the bone with the given name\n" 
			":param boneName: Name of the bone; not existing in the current skeleton is OK\n"
			":param x: x offset in bindpose/worldspace coordinates (ie. post-animation)\n"
			":param y: y offset in bindpose/worldspace coordinates (ie. post-animation)\n"
			":param z: z offset in bindpose/worldspace coordinates (ie. post-animation)\n"
			)

		MAP_METHOD_AND_WRAP( 
			"SetRotation", 
			SetRotation,
			"Set a quaternion rotation offset for the bone with the given name\n" 
			":param boneName: Name of the bone; not existing in the current skeleton is OK\n"
			":param r: 1-st component of the rotation quaternion\n"
			":param i: 2-nd component of the rotation quaternion\n"
			":param j: 3-rd component of the rotation quaternion\n"
			":param k: 4-th component of the rotation quaternion\n"
			)

		MAP_METHOD_AND_WRAP( 
			"ClearTransforms", 
			ClearTransforms,
			"Clear out all offsets and rotations" 
			"\n"				
			)	
    EXPOSURE_END()
}

#endif /* GAMEWORLD_64 */
