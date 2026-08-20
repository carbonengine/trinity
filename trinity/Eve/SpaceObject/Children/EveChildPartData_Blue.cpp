// Copyright © 2026 CCP ehf.

#include "StdAfx.h"
#include "EveChildPartData.h"


BLUE_DEFINE( EveChildPartData );

const Be::ClassInfo* EveChildPartData::ExposeToBlue()
{
	EXPOSURE_BEGIN( EveChildPartData, "Persistent state of a modular space object (per-part transforms and bounds). Edit through EveModularObjectModifier" )
		MAP_INTERFACE( EveSpaceObjectChild );
		MAP_INTERFACE( IEveSpaceObjectChild )
	EXPOSURE_END()
}


BLUE_DEFINE_NONEXPOSED( EveModularObjectModifier );

const Be::ClassInfo* EveModularObjectModifier::ExposeToBlue()
{
	EXPOSURE_BEGIN( EveModularObjectModifier, "Edit session for a modular space object, from CreateModularObject or ModifyModularObject. Culling bounds update on ApplyBounds; dropping the last reference also applies them" )
		MAP_METHOD_AND_WRAP( "AddHull", AddHull, "Builds a SOF hull as a new part at the given transform. Empty faction/race fall back to the seeds given to CreateModularObject. Returns the new part tag, or GetInvalidPartTag() on failure" );
		MAP_METHOD_AND_WRAP( "AddChild", AddChild, "Loads a space object child resource and adds it as a new part at the given transform. Returns the new part tag, or GetInvalidPartTag() on failure" )
		MAP_METHOD_AND_WRAP( "Remove", Remove, "Removes a part: its effect children, mesh instances and locators. Clears accumulated impact damage. Key error if the part tag is unknown" )
		MAP_METHOD_AND_WRAP( "ApplyBounds", ApplyBounds, "Recomputes the object's bounding sphere and shape ellipsoid from the current parts. Call after a batch of edits to keep culling in sync" )
		MAP_METHOD_AND_WRAP( "SetTransform", SetTransform, "Moves a part: re-derives its effect children, locators and bounding sphere from the new transform. Key error if the part tag is unknown" )
		MAP_METHOD_AND_WRAP( "GetPosition", GetPosition, "Returns the part's position. Key error if the part tag is unknown" )
		MAP_METHOD_AND_WRAP( "GetRotation", GetRotation, "Returns the part's rotation. Key error if the part tag is unknown" )
		MAP_METHOD_AND_WRAP( "GetScale", GetScale, "Returns the part's scale. Key error if the part tag is unknown" )

	EXPOSURE_END()
}

MAP_FUNCTION_AND_WRAP( "CreateModularObject", CreateModularObject,
	"Creates an empty modular space object and an edit session for it. Returns (object, modifier).\n"
	"\n"
	"    obj, mod = trinity.CreateModularObject(sof, 'faction', 'race')\n"
	"    tag = mod.AddHull('hullname', '', '', pos, rot, scale)  # '' -> seed faction/race\n"
	"    mod.SetTransform(tag, pos2, rot2, scale2)\n"
	"    mod.ApplyBounds()  # apply culling bounds after a batch of edits\n"
	"    mod = None  # end the session (also applies bounds)" );
MAP_FUNCTION_AND_WRAP( "ModifyModularObject", ModifyModularObject, "Opens an edit session on an existing modular space object" );

MAP_FUNCTION_AND_WRAP( "GetInvalidPartTag", GetInvalidPartTag, "Gets the INVALID_PART_TAG constant" );
