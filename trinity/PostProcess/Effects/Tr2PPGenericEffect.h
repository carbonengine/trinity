// Copyright © 2026 CCP ehf.

#pragma once

#include "PostProcess/Effects/Tr2PPEffect.h"

BLUE_DECLARE( Tr2Effect );
BLUE_DECLARE( Tr2Shader );
BLUE_DECLARE( Tr2FloatParameter );
BLUE_DECLARE( Tr2Vector2Parameter );
BLUE_DECLARE( Tr2Vector3Parameter );
BLUE_DECLARE( Tr2Vector4Parameter );
BLUE_DECLARE( Tr2PostProcessAttributes );
BLUE_DECLARE( Tr2PostProcess2 );

BLUE_CLASS( Tr2PPGenericEffect ) :
	public Tr2PPEffect
{
public:
	EXPOSE_TO_BLUE();

	Tr2PPGenericEffect( IRoot* lockobj = NULL );

	void UpdateEffectParameters();
	bool IsValid() const;
	bool CanBeMerged( const Tr2PPGenericEffect& other ) const;
	bool RequiresSourceTexture() const;

	/// Effect used for post-processing.
	Tr2EffectPtr m_effect;
	/// Post-processing quality threshold for this effect. If the current post-processing quality is lower than this value, the effect will not be executed.
	PostProcess::Quality m_quality = PostProcess::LOW;

	enum ExecutionSlot
	{
		BEFORE_UPSCALING,
		AFTER_TONEMAP,
	};
	static constexpr size_t ExecutionSlotCount = 2;

	/// Place in the post-processing pipeline where this effect should be executed.
	ExecutionSlot m_executionSlot = BEFORE_UPSCALING;
	/// Order of execution for this effect in relation to other generic effects. Lower values are executed first.
	int32_t m_order = 0;

	template <typename Param, typename Value>
	struct BlendableParameter
	{
		using ParameterType = Param;
		using ValueType = Value;

		BluePtr<Param> parameter;
		Value defaultValue;
	};

	std::vector<BlendableParameter<Tr2FloatParameter, float>> blendableFloatParameters;
	std::vector<BlendableParameter<Tr2Vector2Parameter, Vector2>> blendableVector2Parameters;
	std::vector<BlendableParameter<Tr2Vector3Parameter, Vector3>> blendableVector3Parameters;
	std::vector<BlendableParameter<Tr2Vector4Parameter, Vector4>> blendableVector4Parameters;


private:
	unsigned m_lastHashValue = 0;
	unsigned m_nonBlendableHash = 0;
	Tr2ShaderPtr m_lastShader;
	bool m_requiresSourceTexture = false;
};
TYPEDEF_BLUECLASS( Tr2PPGenericEffect );


struct Tr2AccumulatedGenericEffects
{
	struct GenericEffectInstance
	{
		void SetParameters();
		void RestoreParameters();

		Tr2PPGenericEffectPtr effect;
		std::vector<float> floatValues;
		std::vector<Vector2> vector2Values;
		std::vector<Vector3> vector3Values;
		std::vector<Vector4> vector4Values;
	};

	std::array<std::vector<GenericEffectInstance>, Tr2PPGenericEffect::ExecutionSlotCount> effects;
};

void AccumulateGenericEffects( Tr2PostProcess2& postprocess, std::vector<Tr2PostProcessAttributes*>& sources );