////////////////////////////////////////////////////////////
//
//    Created:   March 2019
//    Copyright: CCP 2019
//
#pragma once

#include "Tr2LightManager.h"
#include "Tr2DebugRenderer.h"


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
};

/*
	This class contains all different forms of lights that can be added to the light manager.
	The reason why I did it this way instead of using virtual functions is that this is faster (no vtable).
	Another reason is because the information in this ends up as PerLightData which needs to contain everything regardless of light type
*/
BLUE_CLASS( Tr2Light ):
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
	void AddLight( Tr2LightManager& lightManager, CXMMATRIX transform, float scale );
	void GetLight( Vector3& position, float& radius, Color& color );

	virtual void Update();
	virtual void RenderDebugInfo( Tr2DebugRenderer& renderer, const Matrix& worldMatrix );
	virtual void SetLightData( LightData& baseData );

	// INotify
	virtual bool OnModified( Be::Var* value );

protected:
	LightData m_lightData;
	LIGHT_TYPE m_type;

	std::string m_name;
	Be::Time m_startTime;
	bool m_isDynamic;
};

TYPEDEF_BLUECLASS( Tr2Light );
