#include "StdAfx.h"
#include "Tr2ShadowMap.h"

BLUE_DEFINE( Tr2ShadowMap );

const Be::ClassInfo* Tr2ShadowMap::ExposeToBlue()
{
	EXPOSURE_BEGIN( Tr2ShadowMap, "" )
		MAP_INTERFACE( Tr2ShadowMap )
		MAP_INTERFACE( INotify )

		MAP_ATTRIBUTE(
			"shadowQuality",
			m_quality,
			"Shadow quality",
			Be::READWRITE | Be::NOTIFY )

		MAP_ATTRIBUTE(
			"size",
			m_size,
			"The size of the shadow map, is always quadric",
			Be::READWRITE | Be::NOTIFY )

		MAP_ATTRIBUTE(
			"disableShimmer",
			m_disableShimmer,
			"The size of the shadow map, is always quadric",
			Be::READWRITE | Be::NOTIFY )

		MAP_ATTRIBUTE(
			"splitCount",
			m_splitCount,
			"How many splits",
			Be::READ | Be::NOTIFY )

		MAP_ATTRIBUTE(
			"cascadeEffect",
			m_shadowEffect,
			"Effect used for calculating which split the pixel belongs to and determing the shadow factor.",
			Be::READWRITE )
		
		MAP_ATTRIBUTE( 
			"cascadedShadowMapDS", 
			m_cascadedShadowMapDS, 
			"Depth stencil used for shadows.", 
			Be::READWRITE | Be::NOTIFY )

		MAP_ATTRIBUTE( 
			"shadowMapResultRT", 
			m_shadowMapResultRT,
			"", 
			Be::READWRITE )

	
		MAP_ATTRIBUTE( 
			"denoiser", 
			m_denoiser, 
			"", 
			Be::READWRITE )

		MAP_ATTRIBUTE(
			"debugColorSplit",
			m_debugColorSplit,
			"If enabled color pixels based on splits",
			Be::READWRITE | Be::NOTIFY )

		MAP_METHOD_AND_WRAP(
			"SetNoShadow",
			SetNoShadow,
			"Set to disable shadows" )

		MAP_ATTRIBUTE(
			"SplitNr0",
			m_splitValues[0],
			"Set z far value of split"
			"'shadowThreshold' in pixel diameter. (debug color = red)\n"
			":jessica-group: Split Info",
			Be::READWRITE )
		MAP_ATTRIBUTE(
			"SplitNr1",
			m_splitValues[1],
			"Set z far value of split"
			"'shadowThreshold' in pixel diameter. (debug color = orange)\n"
			":jessica-group: Split Info",
			Be::READWRITE )
		MAP_ATTRIBUTE(
			"SplitNr2",
			m_splitValues[2],
			"Set z far value of split"
			"'shadowThreshold' in pixel diameter. (debug color = yellow)\n"
			":jessica-group: Split Info",
			Be::READWRITE )
		MAP_ATTRIBUTE(
			"SplitNr3",
			m_splitValues[3],
			"Set z far value of split"
			"'shadowThreshold' in pixel diameter. (debug color = lime)\n"
			":jessica-group: Split Info",
			Be::READWRITE )
		MAP_ATTRIBUTE(
			"SplitNr4",
			m_splitValues[4],
			"Set z far value of split"
			"'shadowThreshold' in pixel diameter. (debug color = light green)\n"
			":jessica-group: Split Info",
			Be::READWRITE )
		MAP_ATTRIBUTE(
			"SplitNr5",
			m_splitValues[5],
			"Set z far value of split"
			"'shadowThreshold' in pixel diameter. (debug color = cyan)\n"
			":jessica-group: Split Info",
			Be::READWRITE )
		MAP_ATTRIBUTE(
			"SplitNr6",
			m_splitValues[6],
			"Set z far value of split"
			"'shadowThreshold' in pixel diameter. (debug color = blue)\n"
			":jessica-group: Split Info",
			Be::READWRITE )
		MAP_ATTRIBUTE(
			"SplitNr7",
			m_splitValues[7],
			"Set z far value of split"
			"'shadowThreshold' in pixel diameter. (debug color = navy blue)\n"
			":jessica-group: Split Info",
			Be::READWRITE )
		MAP_ATTRIBUTE(
			"SplitNr8",
			m_splitValues[8],
			"Set z far value of split"
			"'shadowThreshold' in pixel diameter. (debug color = blue violet)\n"
			":jessica-group: Split Info",
			Be::READWRITE )
		MAP_ATTRIBUTE(
			"SplitNr9",
			m_splitValues[9],
			"Set z far value of split"
			"'shadowThreshold' in pixel diameter. (debug color = pink)\n"
			":jessica-group: Split Info",
			Be::READWRITE )
		MAP_ATTRIBUTE(
			"SplitNr10",
			m_splitValues[10],
			"Set z far value of split"
			"'shadowThreshold' in pixel diameter. (debug color = dark magenta)\n"
			":jessica-group: Split Info",
			Be::READWRITE )
		MAP_ATTRIBUTE(
			"SplitNr11",
			m_splitValues[11],
			"Set z far value of split"
			"'shadowThreshold' in pixel diameter. (debug color = rosy brown)\n"
			":jessica-group: Split Info",
			Be::READWRITE )
		MAP_ATTRIBUTE(
			"SplitNr12",
			m_splitValues[12],
			"Set z far value of split"
			"'shadowThreshold' in pixel diameter. (debug color = white)\n"
			":jessica-group: Split Info",
			Be::READWRITE )
		MAP_ATTRIBUTE(
			"SplitNr13",
			m_splitValues[13],
			"Set z far value of split"
			"'shadowThreshold' in pixel diameter. (debug color = light gray)\n"
			":jessica-group: Split Info",
			Be::READWRITE )
		MAP_ATTRIBUTE(
			"SplitNr14",
			m_splitValues[14],
			"Set z far value of split"
			"'shadowThreshold' in pixel diameter. (debug color = dark gray)\n"
			":jessica-group: Split Info",
			Be::READWRITE )
		MAP_ATTRIBUTE(
			"SplitNr15",
			m_splitValues[15],
			"Set z far value of split"
			"'shadowThreshold' in pixel diameter. (debug color = black)\n"
			":jessica-group: Split Info",
			Be::READWRITE )


	EXPOSURE_END()
}
