// Copyright © 2023 CCP ehf.

#pragma once
#ifndef TRIFRUSTUMORTHO_H
#define TRIFRUSTUMORTHO_H

#include "TriFrustum.h"

class TriFrustumOrtho
{
public:
	TriFrustumOrtho();

	void DeriveFrustum( const Matrix& view, const Vector3& minBounds, const Vector3& maxBounds );
	bool IsSphereVisibleAndInsideNearPlane( const Vector4* sphere ) const;
	bool IsSphereVisibleAndInsideNearPlane( const Vector3& center, float radius ) const;
	bool IsSphereVisibleIgnoreFarPlane( const Vector3& center, float radius ) const;
	float GetPixelSize( Vector4 sphere, uint16_t textureSize ) const;
	const Vector3& GetEyePos() const;

	TriFrustumTestResult SphereTestIgnoreFarPlane( const CcpMath::Sphere& sphere ) const;

private:
	Matrix m_view;
	Vector3 m_boundsMin;
	Vector3 m_boundsMax;
};

#endif
