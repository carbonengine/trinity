// Copyright © 2015 CCP ehf.

#include "StdAfx.h"
#include "EveCloudEditableVolume.h"
#include "Tr2HostBitmap.h"

BLUE_DEFINE( EveCloudVolumeBall );

const Be::ClassInfo* EveCloudVolumeBall::ExposeToBlue(){
	EXPOSURE_BEGIN( EveCloudVolumeBall, "Ball for EveCloudEditableVolume" )
		MAP_INTERFACE( EveCloudVolumeBall )
			MAP_INTERFACE( INotify )

				MAP_ATTRIBUTE( "position", m_ballData.m_position, "Ball position", Be::READWRITE | Be::PERSIST | Be::NOTIFY )
					MAP_ATTRIBUTE( "radius", m_ballData.m_radius, "Ball radius", Be::READWRITE | Be::PERSIST | Be::NOTIFY )
						MAP_ATTRIBUTE( "selfIllumination", m_ballData.m_selfIllumination, "Self-illumination per unit volume", Be::READWRITE | Be::PERSIST | Be::NOTIFY )
							MAP_ATTRIBUTE( "opacity", m_ballData.m_opacity, "Opacity per unit volume", Be::READWRITE | Be::PERSIST | Be::NOTIFY )
								MAP_ATTRIBUTE( "falloff", m_ballData.m_falloff, "Opacity/illumination falloff power", Be::READWRITE | Be::PERSIST | Be::NOTIFY )
									EXPOSURE_END()
}

BLUE_DEFINE( EveCloudEditableVolume );

const Be::ClassInfo* EveCloudEditableVolume::ExposeToBlue(){
	EXPOSURE_BEGIN( EveCloudEditableVolume, "Volume texture editor" )
		MAP_INTERFACE( EveCloudEditableVolume )
			MAP_INTERFACE( IListNotify )
				MAP_INTERFACE( IInitialize )
					MAP_INTERFACE( INotify )

						MAP_ATTRIBUTE( "texture", m_texture, "Result texture", Be::READ )
							MAP_ATTRIBUTE( "bitmap", m_bitmap, "Result bitmap", Be::READ )
								MAP_ATTRIBUTE( "balls", m_balls, "List of volume balls", Be::READ | Be::PERSIST )
									MAP_ATTRIBUTE( "width", m_width, "Texture width", Be::READWRITE | Be::PERSIST | Be::NOTIFY )
										MAP_ATTRIBUTE( "height", m_height, "Texture height", Be::READWRITE | Be::PERSIST | Be::NOTIFY )
											MAP_ATTRIBUTE( "depth", m_depth, "Texture depth", Be::READWRITE | Be::PERSIST | Be::NOTIFY )
												MAP_ATTRIBUTE( "renderDebugInfo", m_renderDebugInfo, "Toggle rendering of debuggin information", Be::READWRITE | Be::PERSIST )
													MAP_ATTRIBUTE( "animated", m_animated, "If set than balls are sampled at different time intervals and packed into a texture", Be::READWRITE | Be::PERSIST | Be::NOTIFY )
														MAP_ATTRIBUTE( "curveSets", m_curveSets, "Curve sets for balls", Be::READ | Be::PERSIST )
															MAP_METHOD_AND_WRAP( "OnVolumeModified", OnVolumeModified, "Trigger re-rasterizing of the volume into the texture" )
																MAP_METHOD_AND_WRAP( "Rasterize", Rasterize, "Rasterizes balls into a texture and returns result as a Tr2HostBitmap" )
																	EXPOSURE_END()
}

BLUE_DEFINE( EveCloudVolumeTextureParameter );

const Be::ClassInfo* EveCloudVolumeTextureParameter::ExposeToBlue()
{
	EXPOSURE_BEGIN( EveCloudVolumeTextureParameter, "Cloud volume texture parameter" )
		MAP_INTERFACE( EveCloudVolumeTextureParameter )
		MAP_INTERFACE( ITriEffectResourceParameter )

		MAP_ATTRIBUTE( "name", m_name, "Parameter name", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "volume", m_volume, "Cloud editor volume", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "isUsedByEffect", m_isUsedByEffect, "Is parameter used by effect", Be::READ )
	EXPOSURE_END()
}
