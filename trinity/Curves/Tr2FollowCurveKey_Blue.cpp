////////////////////////////////////////////////////////////
//
//    Created:   June 2019
//    Copyright: CCP 2019
//
#include "StdAfx.h"
#include "Tr2FollowCurveKey.h"

BLUE_DEFINE_INTERFACE( ITr2FollowCurveKey );
BLUE_DEFINE( Tr2ObjectFollowCurveKey );
BLUE_DEFINE( Tr2CameraFollowCurveKey );


Be::VarChooser RotationChooser[] =
{
	{
		"NO_ROTATION",
		BeCast( Tr2ObjectFollowCurveKey::NO_ROTATION ),
		"No Rotation"
	},
	{
		"LOCATOR_ROTATION",
		BeCast( Tr2ObjectFollowCurveKey::LOCATOR_ROTATION ),
		"Locator rotation"
	},
	{
		"MODEL_ROTATION",
		BeCast( Tr2ObjectFollowCurveKey::MODEL_ROTATION ),
		"Model rotation"
	},
	{ 0 }
};

Be::VarChooser Tr2FollowCurveKeyInterpolationChooser[] =
{
	{
		"CONSTANT",
		BeCast( Tr2FollowCurveKeyInterpolation::CONSTANT ),
		"Performs a constant interpolation"
	},
	{
		"LINEAR",
		BeCast( Tr2FollowCurveKeyInterpolation::LINEAR ),
		"Performs a linear interpolation"
	},
	{
		"HERMITE",
		BeCast( Tr2FollowCurveKeyInterpolation::HERMITE ),
		"Performs a hermite interpolation"
	},
	{ 0 }
};


BLUE_REGISTER_ENUM_EX( "Tr2ObjectFollowCurveKeyRotationSetting", Tr2ObjectFollowCurveKey::RotationSetting, RotationChooser, ENUM_REG_ENUM_OBJECT_ON_MODULE );
BLUE_REGISTER_ENUM_EX( "Tr2FollowCurveKeyInterpolation", Tr2FollowCurveKeyInterpolation::Type, Tr2FollowCurveKeyInterpolationChooser, ENUM_REG_ENUM_OBJECT_ON_MODULE );

const Be::ClassInfo* Tr2ObjectFollowCurveKey::ExposeToBlue()
{
	EXPOSURE_BEGIN( Tr2ObjectFollowCurveKey, "" )
		MAP_INTERFACE( ITr2FollowCurveKey )
		MAP_INTERFACE( INotify )
		MAP_INTERFACE( IInitialize)

		MAP_ATTRIBUTE( "name", m_name, "name of the key", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "object", m_object, "the object that is used", Be::READWRITE | Be::NOTIFY )
		MAP_ATTRIBUTE( "time", m_time, "The start time of the key", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE_WITH_CHOOSER( "interpolation", m_interpolation, "The interpolation of the key", Be::READWRITE | Be::PERSIST | Be::ENUM, Tr2FollowCurveKeyInterpolationChooser )
		MAP_ATTRIBUTE( "leftTangent", m_leftTangent, "The left tangent of the key", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "rightTangent", m_rightTangent, "The right tangent of the key", Be::READWRITE | Be::PERSIST )		
		MAP_ATTRIBUTE( "rotatedLeftTangent", m_rotatedLeftTangent, "The rotated left tangent of the key", Be::READ )
		MAP_ATTRIBUTE( "rotatedRightTangent", m_rotatedRightTangent, "The rotated right tangent of the key", Be::READ )
		MAP_ATTRIBUTE( "offsetLocatorName", m_offsetLocatorName, "The name of the offset locator", Be::READWRITE | Be::PERSIST | Be::NOTIFY )
		MAP_ATTRIBUTE( "offset", m_offset, "The offset from the object (and locator)", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE_WITH_CHOOSER( "rotationSetting", m_rotationSetting, "The type of rotation we want", Be::READWRITE | Be::PERSIST | Be::ENUM, RotationChooser )
	EXPOSURE_END()
}


const Be::ClassInfo* Tr2CameraFollowCurveKey::ExposeToBlue()
{
	EXPOSURE_BEGIN( Tr2CameraFollowCurveKey, "" )
		MAP_INTERFACE( ITr2FollowCurveKey )
		MAP_INTERFACE( INotify )
		MAP_INTERFACE( IInitialize )

		MAP_ATTRIBUTE( "name", m_name, "name of the key", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "time", m_time, "The start time of the key", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE_WITH_CHOOSER( "interpolation", m_interpolation, "The interpolation of the key", Be::READWRITE | Be::PERSIST | Be::ENUM, Tr2FollowCurveKeyInterpolationChooser )
		MAP_ATTRIBUTE( "leftTangent", m_leftTangent, "The left tangent of the key", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "rightTangent", m_rightTangent, "The right tangent of the key", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "rotatedLeftTangent", m_rotatedLeftTangent, "The rotated left tangent of the key", Be::READ )
		MAP_ATTRIBUTE( "rotatedRightTangent", m_rotatedRightTangent, "The rotated right tangent of the key", Be::READ )
		MAP_ATTRIBUTE( "objectBounds", m_objectBounds, "The bounds of the object (bounding sphere, box or ellipsoid)", Be::READWRITE )
		MAP_ATTRIBUTE( "angle", m_angle, "The angle which the box is oriented around the unblockedRadius", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "angleZero", m_angleZero, "The angle which is the 'Zero' position of the angle", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "fovMultiplication", m_fovMultiplication, "A fraction of the fov where the box is not allowed to enter (0.25 - 0.75)", Be::READWRITE | Be::PERSIST | Be::NOTIFY )
		MAP_ATTRIBUTE( "offset", m_offset, "An offset of the bounding box position", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "boxPosition", m_boxPosition, "The position of the box", Be::READ )
		MAP_ATTRIBUTE( "enabled", m_enabled, "Enables/Disables the camera (stores the last active camera)", Be::READWRITE | Be::NOTIFY ) 
	EXPOSURE_END()
}