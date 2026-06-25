// Copyright © 2021 CCP ehf.

#pragma once
#include "Tr2Light.h"
#include "Include/TriMath.h"
#include "Eve/SpaceObjectFactory/EveSOFData.h"

class Tr2LightManager;

BLUE_CLASS( Tr2FactionLight ) :
	public Tr2Light,
	public IEveInheritPropertiesOwner
{
public:
	EXPOSE_TO_BLUE();
	Tr2FactionLight( IRoot* lockobj = nullptr );

	bool OnModified( Be::Var * value ) override;

	void SetInheritProperties( const Color* colorSet ) override;
	void RenderDebugInfo( ITr2DebugRenderer2 & renderer, const Matrix& worldMatrix, const Float4x3* bones = nullptr, size_t boneCount = 0 ) override;
	Color GetSelectedColor() const;

private:
	void SetLightColorFromFactionColor();

	float m_saturation;
	bool m_isSpotlight;
	const Color* m_parentColorSet;
	int m_selectedColor;
};

TYPEDEF_BLUECLASS( Tr2FactionLight );