////////////////////////////////////////////////////////////
//
//    Created:   September 2018
//    Copyright: CCP 2018
//
#include "StdAfx.h"
#include "EveChildInheritProperties.h"
#include "Eve/SpaceObjectFactory/EveSOFData.h"

using namespace SOFDataFactionColorChooser;

BLUE_DEFINE( EveChildInheritProperties );

const Be::ClassInfo* EveChildInheritProperties::ExposeToBlue()
{
	EXPOSURE_BEGIN( EveChildInheritProperties, "" )
		MAP_INTERFACE( EveChildInheritProperties )

#define COLOR_DEFINE( _Color, _COLOR )\
		MAP_ATTRIBUTE( #_Color, m_colorSet[TYPE_ ## _COLOR], ":jessica-group:SOF Faction Glow Colors", Be::READ )

		COLOR_DEFINE( Primary, PRIMARY )
		COLOR_DEFINE( Secondary, SECONDARY )
		COLOR_DEFINE( Tertiary, TERTIARY )
		COLOR_DEFINE( Black, BLACK )
		COLOR_DEFINE( White, WHITE )
		COLOR_DEFINE( Yellow, YELLOW )
		COLOR_DEFINE( Orange, ORANGE )
		COLOR_DEFINE( Red, RED )
		COLOR_DEFINE( Blue, BLUE )
		COLOR_DEFINE( Green, GREEN )
		COLOR_DEFINE( Cyan, CYAN )
		COLOR_DEFINE( Fire, FIRE )
		COLOR_DEFINE( Hull, HULL )
		COLOR_DEFINE( Glass, GLASS )
		COLOR_DEFINE( Reactor, REACTOR )
		COLOR_DEFINE( Darkhull, DARKHULL )
		COLOR_DEFINE( Booster, BOOSTER )
		COLOR_DEFINE( Killmark, KILLMARK )
		COLOR_DEFINE( PrimaryLight, PRIMARY_LIGHT )
		COLOR_DEFINE( SecondaryLight, SECONDARY_LIGHT )
		COLOR_DEFINE( TertiaryLight, TERTIARY_LIGHT )
		COLOR_DEFINE( WhiteLight, WHITE_LIGHT )

#undef COLOR_DEFINE

	EXPOSURE_END()
}

using namespace std;
