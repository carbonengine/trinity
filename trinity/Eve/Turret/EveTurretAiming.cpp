// Copyright © 2026 Fenris Creations ehf.

#include "StdAfx.h"
#include "EveTurretAiming.h"
#include "TriMath.h"

// names of system bones like they are in the model skeleton
static constexpr const char* s_systemBoneSkeletonNames[] = {
	"invalid", // SYSBONE_INVALID
	"Sys_Rotation_Arm", // SYSBONE_ROTATION
	"Sys_Rotation_Arm01", // SYSBONE_ROTATION1
	"Sys_Rotation_Arm02", // SYSBONE_ROTATION2
	"Sys_CounterRotation", // SYSBONE_COUNTER_ROTATION
	"Sys_Pitch_Barrel", // SYSBONE_PITCH
	"Sys_Pitch_Barrel1", // SYSBONE_PITCH1
	"Sys_Pitch_Barrel2", // SYSBONE_PITCH2
	"Sys_Height", // SYSBONE_SCALED_HEIGHT
	"Sys_Pitch_Arm01", // SYSBONE_SCALED_PITCH01
	"Sys_Pitch_Arm02", // SYSBONE_SCALED_PITCH02
	"Sys_Pitch_Arm03", // SYSBONE_SCALED_PITCH03
	"Sys_Pitch_Arm04", // SYSBONE_SCALED_PITCH04
	"Sys_Pitch_Arm05", // SYSBONE_SCALED_PITCH05
	"Sys_Pitch_Arm06", // SYSBONE_SCALED_PITCH06
};

const char* EveTurretAiming::GetSystemBoneName( unsigned int bone )
{
	return bone < SYSBONE_MAX ? s_systemBoneSkeletonNames[bone] : s_systemBoneSkeletonNames[SYSBONE_INVALID];
}

// --------------------------------------------------------------------------------
// Description:
//   Depending on the type of the system bone, we calculate a new transform for
//   it and apply it. All of this is highly "hard-coded", but there are some
//   variables in there, so we can customize the tracking to individual turrets.
//   The modification should pay attention the amount of modification needed,
//   passed in trackingInfluence
// Arguments:
//   bone - type of system bone
//   target - position of target in "turret"-space
//   position - the bone position that needs to get modified
//   rotation - the bone rotation that needs to get modified
// --------------------------------------------------------------------------------
void EveTurretAiming::ModifySystemBoneTransform( SystemBones bone, const Vector3* target, const Matrix* localTransform, float trackingInfluence, Vector3& position, Quaternion& rotation ) const
{
	switch( bone )
	{
	case SYSBONE_INVALID:
		break;
	case SYSBONE_ROTATION:
	case SYSBONE_ROTATION01:
	case SYSBONE_ROTATION02: {
		// rotation of turret 360 degrees, alpha is between -pi and pi
		float alpha = atan2( target->x, target->z );
		// never forget do apply influence!
		alpha *= trackingInfluence;
		// 1st: make quaternion
		Quaternion quat = RotationQuaternion( alpha, 0.f, 0.f );
		// 2nd: apply this quat after the original one
		quat = rotation * quat;
		rotation = quat;
	}
	break;
	case SYSBONE_COUNTER_ROTATION: {
		// inverse(!!) rotation of turret 360 degrees, alpha is between -pi and pi
		float alpha = -atan2( target->x, target->z );
		// never forget do apply influence!
		alpha *= trackingInfluence;
		// 1st: make quaternion
		Quaternion quat = RotationQuaternion( alpha, 0.f, 0.f );
		// 2nd: apply this quat after the original one
		quat = rotation * quat;
		rotation = quat;
	}
	break;
	case SYSBONE_PITCH:
	case SYSBONE_PITCH1:
	case SYSBONE_PITCH2: {
		CalcTransformForPitchBone( target, XMConvertToRadians( m_sysBonePitchMin ), XMConvertToRadians( m_sysBonePitchMax ), bone, localTransform, trackingInfluence, rotation );
	}
	break;
	case SYSBONE_SCALED_HEIGHT: {
		// pitch of barrel 90 degrees
		Vector3 directionNormal = Normalize( *target );
		float height = TriClamp( directionNormal.y, 0.f, 1.f );
		// never forget do apply influence!
		height *= trackingInfluence;
		// it's a pos extension with a scale
		Vector3 pos = Vector3( 0.f, height * m_sysBoneHeight, 0.f ) + position;
		position = pos;
	}
	break;
	case SYSBONE_SCALED_PITCH01:
	case SYSBONE_SCALED_PITCH02:
	case SYSBONE_SCALED_PITCH03:
	case SYSBONE_SCALED_PITCH04:
	case SYSBONE_SCALED_PITCH05:
	case SYSBONE_SCALED_PITCH06: {
		CalcTransformForPitchBone( target, 0.f, XMConvertToRadians( m_sysBonePitchMax ), bone, nullptr, trackingInfluence, rotation );
	}
	break;
	default:
		break;
	}
}

void EveTurretAiming::CalcTransformForPitchBone( const Vector3* target, float minPitch, float maxPitch, unsigned int boneIndex, const Matrix* localTransform, float trackingInfluence, Quaternion& rotation ) const
{
	float pitchOffset = GetBonePitchOffset( boneIndex );
	float pitchFactor = GetBonePitchFactor( boneIndex );
	// pitch of barrel 90 degrees
	Vector3 bone_position( 0.f, 0.f, 0.f );

	if( localTransform )
	{
		bone_position = localTransform->GetTranslation();
	}

	Vector3 relTarget = *target - bone_position;
	Vector3 dirNrm = Normalize( relTarget );
	float radians = asinf( dirNrm.y );

	if( localTransform )
	{
		Vector3 bone_direction = Normalize( bone_position );
		float d = Dot( bone_direction, *target );
		if( d < Length( bone_position ) )
		{
			// Assuming up is enough for now to avoid cross products
			radians = TriFloatSign( relTarget.y ) * XM_PI - radians;
		}
	}

	float alpha = TriClamp( radians, minPitch, maxPitch );
	// modify!
	alpha = pitchFactor * alpha + XMConvertToRadians( pitchOffset );
	// never forget do apply influence!
	alpha *= trackingInfluence;
	// 1st: make quaternion
	Quaternion quat = RotationQuaternion( 0.f, -alpha, 0.f );
	// 2nd: apply this quat after the original one
	quat = rotation * quat;
	rotation = quat;
}

float EveTurretAiming::GetBonePitchFactor( unsigned int boneIndex ) const
{
	switch( boneIndex )
	{
	case SYSBONE_PITCH:
	case SYSBONE_PITCH1:
	case SYSBONE_PITCH2:
		return m_sysBonePitchFactor;
	case SYSBONE_SCALED_PITCH01:
		return m_sysBonePitch01Factor;
	case SYSBONE_SCALED_PITCH02:
		return m_sysBonePitch02Factor;
	case SYSBONE_SCALED_PITCH03:
		return m_sysBonePitch03Factor;
	default:
		return 1.0f;
	}
}

float EveTurretAiming::GetBonePitchOffset( unsigned int boneIndex ) const
{
	switch( boneIndex )
	{
	case SYSBONE_PITCH:
	case SYSBONE_PITCH1:
	case SYSBONE_PITCH2:
		return m_sysBonePitchOffset;
	case SYSBONE_SCALED_PITCH01:
		return m_sysBonePitch01Offset;
	case SYSBONE_SCALED_PITCH02:
		return m_sysBonePitch02Offset;
	case SYSBONE_SCALED_PITCH03:
		return m_sysBonePitch03Offset;
	default:
		return 0.0f;
	}
}
