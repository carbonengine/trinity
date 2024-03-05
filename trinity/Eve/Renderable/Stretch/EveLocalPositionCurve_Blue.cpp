#include "StdAfx.h"
#include "EveLocalPositionCurve.h"

BLUE_DEFINE( EveLocalPositionCurve );

Be::VarChooser EveLocalPositionChooser[] =
{
	{
		"none",     
		BeCast( EveLocalPositionCurve::POS_NONE ),     
		"No position."
	},
	{
		"nearestBounds",
		BeCast( EveLocalPositionCurve::POS_NEAREST_BOUNDING_POINT ),     
		"use the closest point on the ellyptical bounding sphere of the parent object."
	},
	{
		"centerBounds",
		BeCast( EveLocalPositionCurve::POS_CENTER_BOUNDING_POINT),
		"Use the center of the bounding sphere of the parent object."
	},
	{
		"damageLocator",
		BeCast( EveLocalPositionCurve::POS_TARGET_DMG_LOCATOR ),
		"Use a damage locator of the target object."
	},
	{
		"damageLocatorImpact",
		BeCast( EveLocalPositionCurve::POS_TARGET_DMG_LOCATOR_IMPACT ),
		"Use a damage locator of the target object, but with impact effect"
	},
	{
		"offsetPosition",
		BeCast( EveLocalPositionCurve::POS_OFFSET_POSITION ),
		"Calculate a position based on an offset from parent position"
	},
	{
		"offsetPlaneRotation",
		BeCast( EveLocalPositionCurve::POS_OFFSET_PLANE_ROTATION ),
		"Moves the vector from parentPositionCurve(or 0, 0, 0) to alignedPositionCurve OR positionOffset to the xz plane "
		"containing parentPositionCurve, maintaining the vector's length."
	},
	{
		"nearestFiringLocator",
		BeCast( EveLocalPositionCurve::POS_NEAREST_FIRING_LOCATOR ),
		"Use the nearest firing locator of source object"
	},
	{
		"activeTurret",
		BeCast( EveLocalPositionCurve::POS_ACTIVE_TURRET),
		"Use the active turret world position"
	},
	{ 0 }
};

BLUE_REGISTER_ENUM_EX( 
    "EveLocalPositionBehavior", 
	EveLocalPositionCurve::LocalPositionBehavior, 
    EveLocalPositionChooser,
    ENUM_REG_ENUM_OBJECT_ON_MODULE
);

const Be::ClassInfo* EveLocalPositionCurve::ExposeToBlue()
{
	EXPOSURE_BEGIN( EveLocalPositionCurve, "" )
		MAP_INTERFACE( ITriVectorFunction )
		
		MAP_ATTRIBUTE_WITH_CHOOSER
		( 
			"behavior",
			m_behavior,
			"Chooses between position calculation behaviour.",
			Be::READWRITE | Be::PERSIST | Be::ENUM, 
			EveLocalPositionChooser
		)

		MAP_ATTRIBUTE
		( 
			"parentPositionCurve",        
			m_parentPositionCurve,
			"na", 
			Be::READWRITE | Be::PERSIST
		)
		MAP_ATTRIBUTE
		(    
			"parentRotationCurve",
			m_parentRotationCurve,
			"na",
			Be::READWRITE
		)

		MAP_ATTRIBUTE
		( 
			"alignPositionCurve",        
			m_alignPositionCurve,
			"na", 
			Be::READWRITE | Be::PERSIST
		)

		MAP_ATTRIBUTE
		( 
			"parent",
			m_parentObject,
			"na",
			Be::READWRITE
		)

		MAP_ATTRIBUTE
		(
			"turretSetObject",
			m_turretSetObject,
			"na",
			Be::READWRITE
		)

		MAP_ATTRIBUTE
		(
			"muzzleIndex",
			m_muzzleIndex,
			"na",
			Be::READWRITE
		)		

		MAP_ATTRIBUTE
		(  
			"value",         
			m_value,         
			"na", 
			Be::READWRITE | Be::PERSIST
		)

		MAP_ATTRIBUTE
		(  
			"boundingSize",         
			m_boundingBoxSize,         
			"na", 
			Be::READWRITE | Be::PERSIST
		)

		MAP_ATTRIBUTE
		(  
			"offset",         
			m_offset,         
			"na", 
			Be::READWRITE | Be::PERSIST
		)

		MAP_ATTRIBUTE
		(
			"locatorSetName",
			m_locatorSetName,
			"The locatorSet name that is being used by the curve",
			Be::READWRITE
		)

		MAP_ATTRIBUTE
		(
			"locatorIndex",
			m_locatorIndex,
			"The locator index that is being used by the curve",
			Be::READWRITE
		)

		MAP_ATTRIBUTE
		(  
			"positionOffset",         
			m_positionOffset,         
			"na", 
			Be::READWRITE | Be::PERSIST
		)

		MAP_ATTRIBUTE
		(  
			"damageLocatorIndex",         
			m_damageLocatorIndex,         
			"The damage locator index that is being used by the curve", 
			Be::READ
		)

		MAP_ATTRIBUTE( "impactSize", m_impactSize, "Holds the size of the impact effect", Be::READWRITE )

		MAP_METHOD_AND_WRAP_OPTIONAL_ARGS
		(
			"__init__",
			SetBehavior,
			1,
			"Create a EveLocalPostitionCurve with a render step that issues a Python callback\n"
			":param cb: a Python callable (default None)"
		)

	EXPOSURE_END()
}
