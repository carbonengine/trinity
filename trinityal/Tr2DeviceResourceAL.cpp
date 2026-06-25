// Copyright © 2023 CCP ehf.

#include "StdAfx.h"
#include "Tr2DeviceResourceAL.h"


namespace
{
typedef std::set<TrinityALImpl::Tr2BaseDeviceResourceAL*> AllResourcesType;

AllResourcesType& GetAllResources()
{
	static AllResourcesType allResources;
	return allResources;
}

CcpMutex& GetAllResourcesMutex()
{
	static CcpMutex mutex( "GetAllResourcesMutex", "mutex" );
	return mutex;
}

bool s_resourcesMutated = false;
}

namespace TrinityALImpl
{
Tr2BaseDeviceResourceAL::Tr2BaseDeviceResourceAL()
{
	CcpAutoMutex lock( GetAllResourcesMutex() );
	GetAllResources().insert( this );
	s_resourcesMutated = true;
}

Tr2BaseDeviceResourceAL::~Tr2BaseDeviceResourceAL()
{
	CcpAutoMutex lock( GetAllResourcesMutex() );
	GetAllResources().erase( this );
	s_resourcesMutated = true;
}

void Tr2BaseDeviceResourceAL::EnumerateResources( ResourceOperation* operation )
{
	CcpAutoMutex lock( GetAllResourcesMutex() );
	auto& allResources = GetAllResources();

	for( auto it = begin( allResources ); it != end( allResources ); ++it )
	{
		( *operation )( *it );
	}
}
}

void DescribeDeviceResources( Tr2DescribeDeviceResourceOperationAL operation )
{
	CcpAutoMutex lock( GetAllResourcesMutex() );
	auto& allResources = GetAllResources();
	for( auto it = begin( allResources ); it != end( allResources ); ++it )
	{
		Tr2DeviceResourceDescriptionAL desc;
		if( ( *it )->IsResourceValid() )
		{
			( *it )->Describe( desc );
			( *operation )( ( *it )->GetResourceMemoryClass(), desc );
		}
	}
}

void DestroyDeviceResources( Tr2ALMemoryTypes memoryTypes )
{
	CcpAutoMutex lock( GetAllResourcesMutex() );
	auto& allResources = GetAllResources();
	do
	{
		s_resourcesMutated = false;
		for( auto resource : allResources )
		{
			if( resource->IsResourceValid() && ( resource->GetResourceMemoryClass() & memoryTypes ) != 0 )
			{
				resource->Destroy();
			}
			if( s_resourcesMutated )
			{
				// We can't trust the iterator anymore, so we need to break out of the loop and start over.
				break;
			}
		}
	} while( s_resourcesMutated );
}
