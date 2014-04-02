#include "StdAfx.h"
#if APEX_ENABLED

#include "Tr2ApexRes.h"

BLUE_DEFINE( Tr2ApexRes );

IBlueResource* CreateTr2ApexRes( const wchar_t* name )
{
	Tr2ApexResPtr p;
	p.CreateInstance();
	return p.Detach();
}

BLUE_REGISTER_RESOURCE_EXTENSION( L"apbapex", CreateTr2ApexRes );
BLUE_REGISTER_RESOURCE_EXTENSION( L"apxapex", CreateTr2ApexRes );

const Be::ClassInfo* Tr2ApexRes::ExposeToBlue()
{
    EXPOSURE_BEGIN( Tr2ApexRes, "" )
        MAP_INTERFACE( Tr2ApexRes )
		MAP_INTERFACE( IBlueResource )
		MAP_INTERFACE( ICacheable )
		MAP_ICACHEABLE_METHODS()

		MAP_ATTRIBUTE( "dataSize", m_dataSize, "Size of the underlying Apex clothing asset", Be::READ )
		
		
	EXPOSURE_CHAINTO( BlueAsyncRes )
}

#endif // APEX_ENABLED
