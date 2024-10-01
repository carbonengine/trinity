
#pragma once
#include "Eve/EveComponentRegistry.h"
#include "PostProcess/Tr2PostProcess2.h"

namespace PostProcess
{

enum PostProcessAttributeType
{
	SIGNAL_LOSS_INTENSITY = 0,
	BLOOM_BRIGHTNESS = 1,
	BLOOM_LUMINANCE_THRESHOLD = 2,
	BLOOM_LUMINANCE_SCALE = 3,
	GRIME_INTENSITY = 4,
	GRIME_PATH = 5,
	FILM_GRAIN_INTENSITY = 6,
	FILM_GRAIN_SIZE = 7,
	FILM_GRAIN_DENSITY = 8,
	FILM_GRAIN_CONTRAST = 9,
	FILM_GRAIN_BRIGHTNESS_MODIFIER = 10,
	FILM_GRAIN_COLORED = 11,
	FILM_GRAIN_COLOR_AMOUNT = 12,
	SATURATION = 13,
	FADE_INTENSITY = 14,
	FADE_COLOR = 15,
	LUT_INTENSITY = 16,
	LUT_PATH = 17,
	VIGNETTE_INTENSITY = 18,
	VIGNETTE_OPACITY = 19,
	VIGNETTE_COLOR = 20,
	VIGNETTE_DETAIL1_SIZE = 21,
	VIGNETTE_DETAIL1_SCROLL = 22,
	VIGNETTE_DETAIL2_SIZE = 23,
	VIGNETTE_DETAIL2_SCROLL = 24,
	VIGNETTE_SHAPE_PATH = 25,
	VIGNETTE_DETAIL_PATH = 26,
	VIGNETTE_SINE_FREQUENCY = 27,
	VIGNETTE_MIN_SINE_FREQUENCY = 28,
	VIGNETTE_MAX_SINE_FREQUENCY = 29,
	DEPTH_OF_FIELD_SCALE = 30,
	DEPTH_OF_FIELD_FOCAL_DISTANCE = 31,
	DEPTH_OF_FIELD_FOCAL_LENGTH = 32,
	DEPTH_OF_FIELD_SHAPE = 33,
	POST_PROCESS_ATTRIBUTE_TYPE_COUNT = 34
};

enum Priority
{
	LOW_PRIORITY = 0,
	MEDIUM_PRIORITY = 1,
	HIGH_PRIORITY = 2,
	PRIORITY_COUNT
};

// list of attribute type, intensity, total intensity and index in a attribute
struct WeightRow
{
	WeightRow(): 
	type( POST_PROCESS_ATTRIBUTE_TYPE_COUNT ), 
	intensity( 0.0f ), 
	totalIntensity( 0.0f ), 
	index( 0 ), 
	priority( LOW_PRIORITY ){}

	WeightRow( PostProcessAttributeType type, float intensity, float totalIntensity, size_t index, Priority priority ) :
		type( type ),
		intensity( intensity ),
		totalIntensity( totalIntensity ),
		index( index ),
		priority( priority ){}

	WeightRow( const WeightRow& other ) :
		type( other.type ),
		intensity( other.intensity ),
		totalIntensity( other.totalIntensity ),
		index( other.index ),
		priority( other.priority ){}

	PostProcessAttributeType type;
	float intensity;
	float totalIntensity;
	size_t index;
	Priority priority;
};
typedef std::vector<WeightRow> WeightTable;

template <typename T>
struct PostProcessAttribute
{
	T value;
	bool enabled;
	float currentIntensity;

	PostProcessAttribute( T value ) :
		value( value ),
		enabled( false ),
		currentIntensity( 0.0f ){}
	

	void Blend( const PostProcessAttribute<T>& other, float intensity, float totalIntensity )
	{
		if( other.enabled )
		{
			value += other.value * intensity * intensity / totalIntensity;
			enabled = true;
		}
	}
};


struct Attributes
{
	Attributes();

	void AccumulateIntensity( Attributes& other);
	void SetIntensity( float intensity );

	WeightTable GetWeightTable( size_t index, Priority priority);

	void Merge( const Attributes& other, const WeightTable& table ); 
	float intensity;
	Priority priority;

	PostProcessAttribute<float> signalLossIntensity;

	PostProcessAttribute<float> bloomBrightness;
	PostProcessAttribute<float> bloomLuminanceThreshold;
	PostProcessAttribute<float> bloomLuminanceScale;

	PostProcessAttribute<float> grimeIntensity;
	PostProcessAttribute<BlueSharedString> grimePath;

	PostProcessAttribute<bool> filmGrainColored;
	PostProcessAttribute<float> filmGrainColorAmount;
	PostProcessAttribute<float> filmGrainIntensity;
	PostProcessAttribute<float> filmGrainSize;
	PostProcessAttribute<float> filmGrainDensity;
	PostProcessAttribute<float> filmGrainContrast;
	PostProcessAttribute<float> filmGrainBrightnessModifier;

	// negative is desaturation, positive is saturation
	PostProcessAttribute<float> saturation;

	PostProcessAttribute<float> fadeIntensity;
	PostProcessAttribute<Color> fadeColor;

	PostProcessAttribute<float> lutIntensity;
	PostProcessAttribute<BlueSharedString> lutPath;

	// a container for the accumulation of luts
	std::set<std::pair<float, BlueSharedString>> prioritizedLuts;

	PostProcessAttribute<float> vignetteIntensity;
	PostProcessAttribute<float> vignetteOpacity;
	PostProcessAttribute<Color> vignetteColor;
	PostProcessAttribute<Vector2> vignetteDetail1Size;
	PostProcessAttribute<Vector2> vignetteDetail1Scroll;
	PostProcessAttribute<Vector2> vignetteDetail2Size;
	PostProcessAttribute<Vector2> vignetteDetail2Scroll;
	PostProcessAttribute<BlueSharedString> vignetteShapePath;
	PostProcessAttribute<BlueSharedString> vignetteDetailPath;
	PostProcessAttribute<float> vignetteSineFrequency;
	PostProcessAttribute<float> vignetteMinSineFrequency;
	PostProcessAttribute<float> vignetteMaxSineFrequency;

	PostProcessAttribute<float> depthOfFieldScale;
	PostProcessAttribute<float> depthOfFieldFocalDistance;
	PostProcessAttribute<float> depthOfFieldFocalLength;
	PostProcessAttribute<Tr2Bokeh::Shape> depthOfFieldShape;
};

Tr2PostProcess2Ptr Gather( EveComponentRegistry& componentRegistry );
};