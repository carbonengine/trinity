// Copyright © 2018 CCP ehf.

#include "StdAfx.h"
#include "EveChildInheritProperties.h"
#include "Eve/SpaceObjectFactory/EveSOFData.h"

using namespace SOFDataFactionColorChooser;

BLUE_DEFINE( EveChildInheritProperties );

const Be::ClassInfo* EveChildInheritProperties::ExposeToBlue()
{
	EXPOSURE_BEGIN( EveChildInheritProperties, "" )
		MAP_INTERFACE( EveChildInheritProperties )

#define COLOR_DEFINE( _Color, _COLOR ) \
	MAP_ATTRIBUTE( #_Color, m_colorSet[TYPE_##_COLOR], ":jessica-group:SOF Faction Glow Colors", Be::READ )

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
		COLOR_DEFINE( PrimarySpotlight, PRIMARY_SPOTLIGHT )
		COLOR_DEFINE( SecondarySpotlight, SECONDARY_SPOTLIGHT )
		COLOR_DEFINE( TertiarySpotlight, TERTIARY_SPOTLIGHT )
		COLOR_DEFINE( PrimaryHologram, PRIMARY_HOLOGRAM )
		COLOR_DEFINE( SecondaryHologram, SECONDARY_HOLOGRAM )
		COLOR_DEFINE( TertiaryHologram, TERTIARY_HOLOGRAM )
		COLOR_DEFINE( State0, STATE_0 )
		COLOR_DEFINE( State1, STATE_1 )
		COLOR_DEFINE( State2, STATE_2 )
		COLOR_DEFINE( State3, STATE_3 )
		COLOR_DEFINE( StateVulnerable, STATE_VULNERABLE )
		COLOR_DEFINE( StateInvulnerable, STATE_INVULNERABLE )
		COLOR_DEFINE( PrimaryForcefield, PRIMARY_FORCEFIELD )
		COLOR_DEFINE( SecondaryForcefield, SECONDARY_FORCEFIELD )
		COLOR_DEFINE( PrimaryBanner, PRIMARY_BANNER )
		COLOR_DEFINE( PrimaryBillboard, PRIMARY_BILLBOARD )
		COLOR_DEFINE( PrimaryFx, PRIMARY_FX )
		COLOR_DEFINE( SecondaryFx, SECONDARY_FX )
		COLOR_DEFINE( PrimaryWarpFx, PRIMARY_WARP_FX )
		COLOR_DEFINE( PrimaryAttackFx, PRIMARY_ATTACK_FX )
		COLOR_DEFINE( PrimarySiegeFx, PRIMARY_SIEGE_FX )
		COLOR_DEFINE( PrimaryDockedFx, PRIMARY_DOCKED_FX )

#undef COLOR_DEFINE

	EXPOSURE_END()
}

using namespace std;
