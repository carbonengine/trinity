// Copyright © 2019 CCP ehf.

#pragma once

#include "Tr2DeviceResource.h"


class Tr2ProceduralBuffer
{
public:
	typedef Tr2SuballocatedBuffer::Allocation ResourceType;
	typedef ALResult ( *FactoryFunction )( Tr2SuballocatedBuffer::Allocation& result, Tr2PrimaryRenderContext& renderContext );

	Tr2ProceduralBuffer( const BlueSharedString& name, FactoryFunction factory );

	const Tr2SuballocatedBuffer::Allocation& GetSharedResource() const;

private:
	const Tr2SuballocatedBuffer::Allocation* m_resource;
};
