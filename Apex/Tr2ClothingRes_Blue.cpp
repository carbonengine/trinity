#include "StdAfx.h"

#if APEX_ENABLED

#include "Tr2ClothingRes.h"

BLUE_DEFINE( Tr2ClothingRes );

IBlueResource* CreateTr2ClothingRes( const wchar_t* name )
{
	Tr2ClothingResPtr p;
	p.CreateInstance();
	return p.Detach();
}

BLUE_REGISTER_RESOURCE_EXTENSION( L"apbcloth", CreateTr2ClothingRes );

const Be::ClassInfo* Tr2ClothingRes::ExposeToBlue()
{
    EXPOSURE_BEGIN( Tr2ClothingRes, "" )
        MAP_INTERFACE( Tr2ClothingRes )
		MAP_INTERFACE( IBlueResource )
		MAP_INTERFACE( ICacheable )
		MAP_ICACHEABLE_METHODS()

		MAP_ATTRIBUTE( "dataSize", m_dataSize, "Size of the underlying Apex clothing asset", Be::READ )
		
		MAP_PROPERTY_READONLY( "numGraphicalLodLevels", GetNumGraphicalLodLevels, "returns the number of LOD levels present in this asset" )
		MAP_PROPERTY_READONLY( "maximumSimulationBudget", GetMaximumSimulationBudget, "Returns how much an Actor will cost at maximum" )
		MAP_PROPERTY_READONLY( "biggestMaxDistance", GetBiggestMaxDistance, "returns the biggest max distance of any vertex in any physical mesh" )
		MAP_PROPERTY_READONLY( "numBones", GetNumBones, "Returns the number of bones" )

		MAP_METHOD_AND_WRAP( "GetGraphicalLodValue", GetGraphicalLodValue, "returns the actual LOD value of any particular level, normally this is just the identity map" )
		MAP_METHOD_AND_WRAP( "GetBoneName", GetBoneName, "returns the name of the given bone" )

		MAP_METHOD_AND_WRAP( "SafeReload", SafeReload, "Check if a simulation is in progress, and if so delays the Reload until simulation has stepped." )

	EXPOSURE_CHAINTO( BlueAsyncRes )
}

#endif // APEX_ENABLED
