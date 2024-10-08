
#pragma once

#include "Eve/EveComponentRegistry.h"
#include "Tr2PostProcess2.h"
#include "Tr2PostProcessEnums.h"

BLUE_DECLARE( Tr2PostProcessAttributes );
BLUE_DECLARE_VECTOR( Tr2PostProcessAttributes );

namespace PostProcess
{
	template <typename T>
	struct Attribute
	{
		T value;
		bool enabled;
		float currentIntensity;

		Attribute( T value ) :
			value( value ),
			enabled( false ),
			currentIntensity( 0.0f ){}
	
		Attribute( T value, bool enabled ) :
			value( value ),
			enabled( enabled ),
			currentIntensity( 0.0f ){}

		void Blend( const Attribute<T>& other, float intensity, float totalIntensity )
		{
			if( other.enabled )
			{
				value += other.value * intensity * intensity / totalIntensity;
				enabled = true;
			}
		}
	};
}

struct WeightRow
{
	WeightRow();
	WeightRow( PostProcessEnums::AttributeType type, float intensity );
	WeightRow( PostProcessEnums::AttributeType type, float intensity, float totalIntensity );
	WeightRow( const WeightRow& other );

	float intensity;
	float totalIntensity;
	uint32_t type;
};
BLUE_DECLARE_STRUCTURE_LIST( WeightRow );


BLUE_CLASS( Tr2PostProcessAttributes ) :
	public IRoot
{
public:
	EXPOSE_TO_BLUE();

	Tr2PostProcessAttributes( IRoot* lockobj = NULL );
	~Tr2PostProcessAttributes();

	void Reset();

	void FromPostProcess( Tr2PostProcess2* postprocess, PostProcessEnums::Priority priority, float intensity );
	void ToPostProcess( Tr2PostProcess2Ptr postprocess ) const;
	void Blend( std::vector<Tr2PostProcessAttributes*> attributes );
	
	PWeightRowStructureList GetWeightTable( ) const;
	void SetOwner( IRoot* owner );

	void Merge( const Tr2PostProcessAttributes* other ); 

	// public attributes, so we can access them from the outside
	float intensity;
	PostProcessEnums::Priority priority;
	PostProcess::Attribute<float> signalLossIntensity;

	PostProcess::Attribute<float> bloomBrightness;
	PostProcess::Attribute<float> bloomLuminanceThreshold;
	PostProcess::Attribute<float> bloomLuminanceScale;

	PostProcess::Attribute<float> grimeIntensity;
	PostProcess::Attribute<BlueSharedString> grimePath;

	PostProcess::Attribute<bool> filmGrainColored;
	PostProcess::Attribute<float> filmGrainColorAmount;
	PostProcess::Attribute<float> filmGrainIntensity;
	PostProcess::Attribute<float> filmGrainSize;
	PostProcess::Attribute<float> filmGrainDensity;
	PostProcess::Attribute<float> filmGrainContrast;
	PostProcess::Attribute<float> filmGrainBrightnessModifier;

	// negative is desaturation, positive is saturation
	PostProcess::Attribute<float> saturation;

	PostProcess::Attribute<float> fadeIntensity;
	PostProcess::Attribute<Color> fadeColor;

	PostProcess::Attribute<float> lutIntensity;
	PostProcess::Attribute<BlueSharedString> lutPath;
	
	// a container for the accumulation of luts
	std::set<std::pair<float, BlueSharedString>> prioritizedLuts;

	PostProcess::Attribute<float> vignetteIntensity;
	PostProcess::Attribute<float> vignetteOpacity;
	PostProcess::Attribute<Color> vignetteColor;
	PostProcess::Attribute<Vector2> vignetteDetail1Size;
	PostProcess::Attribute<Vector2> vignetteDetail1Scroll;
	PostProcess::Attribute<Vector2> vignetteDetail2Size;
	PostProcess::Attribute<Vector2> vignetteDetail2Scroll;
	PostProcess::Attribute<BlueSharedString> vignetteShapePath;
	PostProcess::Attribute<BlueSharedString> vignetteDetailPath;
	PostProcess::Attribute<float> vignetteSineFrequency;
	PostProcess::Attribute<float> vignetteMinSineFrequency;
	PostProcess::Attribute<float> vignetteMaxSineFrequency;

	PostProcess::Attribute<float> depthOfFieldScale;
	PostProcess::Attribute<float> depthOfFieldFocalDistance;
	PostProcess::Attribute<float> depthOfFieldFocalLength;
	PostProcess::Attribute<Tr2Bokeh::Shape> depthOfFieldShape;
private:
	void ResetWeights();
	void UpdateWeightIntensity( std::vector<WeightRow> & currentPriorityWeights, std::vector<WeightRow> & higherPriorityWeights );

	PWeightRowStructureList m_weights;

	// the owner of this attribute, used for knowing where the blended attribute comes from
	IRoot* m_owner;

	// when attributes are blended we maintain a list of sources, for debugging
	PTr2PostProcessAttributesVector m_sources;
};

TYPEDEF_BLUECLASS( Tr2PostProcessAttributes );
