// Copyright © 2026 Fenris Creations ehf.

#pragma once
#ifndef EveTurretAiming_H
#define EveTurretAiming_H

// system-controlled bones
enum SystemBones
{
	SYSBONE_INVALID = 0,
	SYSBONE_ROTATION,
	SYSBONE_ROTATION01,
	SYSBONE_ROTATION02,
	SYSBONE_COUNTER_ROTATION,
	SYSBONE_PITCH,
	SYSBONE_PITCH1,
	SYSBONE_PITCH2,
	SYSBONE_SCALED_HEIGHT,
	SYSBONE_SCALED_PITCH01,
	SYSBONE_SCALED_PITCH02,
	SYSBONE_SCALED_PITCH03,
	SYSBONE_SCALED_PITCH04,
	SYSBONE_SCALED_PITCH05,
	SYSBONE_SCALED_PITCH06,
	SYSBONE_MAX,
};

// Sysbone aiming math and tuning values shared by EveTurretSet and EveChildTurret.
// Hosts embed this by value and expose the members through their own Blue schema.
class EveTurretAiming
{
public:
	// name of a system bone as authored in the model skeleton
	static const char* GetSystemBoneName( unsigned int bone );

	void ModifySystemBoneTransform( SystemBones bone, const Vector3* target, const Matrix* localTransform, float trackingInfluence, Vector3& position, Quaternion& rotation ) const;

	void CalcTransformForPitchBone( const Vector3* target, float minPitch, float maxPitch, unsigned int boneIndex, const Matrix* localTransform, float trackingInfluence, Quaternion& rotation ) const;

	float GetBonePitchFactor( unsigned int boneIndex ) const;
	float GetBonePitchOffset( unsigned int boneIndex ) const;

	// specific system bone values
	float m_sysBoneHeight = 1.f;
	float m_sysBonePitchOffset = 0.f;
	float m_sysBonePitchFactor = 1.f;
	float m_sysBonePitchMin = 0.f;
	float m_sysBonePitchMax = 90.f;
	float m_sysBonePitch01Offset = 0.f;
	float m_sysBonePitch01Factor = 1.f;
	float m_sysBonePitch02Offset = 0.f;
	float m_sysBonePitch02Factor = 1.f;
	float m_sysBonePitch03Offset = 0.f;
	float m_sysBonePitch03Factor = 1.f;
};

#endif
