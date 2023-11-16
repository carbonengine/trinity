////////////////////////////////////////////////////////////
//
//    Created:   March 2019
//    Copyright: CCP 2019
//
#pragma once

#include "Tr2LightManager.h"
#include "Tr2DebugRenderer.h"
#include "Utilities/MatrixUtils.h"

BLUE_DECLARE( Tr2LightProfileRes );

struct LightData {
	LightData();

	Vector3 position;
	Color color;
	float brightness;
	float noiseAmplitude;
	float noiseFrequency;
	uint32_t noiseOctaves;

	float radius;
	float innerRadius;

	// Spotlight specifics
	Quaternion rotation;
	float outerAngle;
	float innerAngle;

	// Textured light specifics
	std::wstring texturePath;
	int32_t boneIndex;

	uint16_t flags;
};

/*
	This class contains all different forms of lights that can be added to the light manager.
	The reason why I did it this way instead of using virtual functions is that this is faster (no vtable).
	Another reason is because the information in this ends up as PerLightData which needs to contain everything regardless of light type
*/
BLUE_CLASS( Tr2Light ):
	public IInitialize,
	public INotify
{
public:
	enum LIGHT_TYPE {
		UNDEFINED_LIGHT,
		POINT_LIGHT,
		SPOT_LIGHT,

		COUNT
	};

	EXPOSE_TO_BLUE();
	Tr2Light( IRoot* lockobj = nullptr );
	void AddLight( Tr2LightManager& lightManager, CXMMATRIX transform, float scale, const granny_matrix_3x4* bones = nullptr, size_t boneCount = 0 );
	void GetLight( Vector3& position, float& radius, Color& color );
	void ChangeLightColor( Color c );

	virtual void Update();
	virtual void RenderDebugInfo( ITr2DebugRenderer2& renderer, const Matrix& worldMatrix, const granny_matrix_3x4* bones = nullptr, size_t boneCount = 0 );
	virtual void SetLightData( LightData& baseData );
	
	void SetBoneMatrix( const granny_matrix_3x4* bones, size_t boneCount );
	void SetBrightnessMultiplier( float multi );

	bool Initialize() override;

	// INotify
	virtual bool OnModified( Be::Var* value );

	const LightData& GetLightData() const;
	float GetBrightnessMultiplier() const;

protected:
	LightData m_lightData;
	LIGHT_TYPE m_type;

	std::string m_name;
	Be::Time m_startTime;
	bool m_isDynamic;
	float m_brightnessMultiplier;
	Matrix m_boneTransform; // used for lights that have boneIndices

	Tr2LightProfileResPtr m_lightProfile;
	std::wstring m_lightProfilePath;
};

TYPEDEF_BLUECLASS( Tr2Light );

extern const Be::VarChooser Tr2LightFlagChooser[];
