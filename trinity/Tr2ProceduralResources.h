////////////////////////////////////////////////////////////
//
//    Created:   September 2019
//    Copyright: CCP 2019
//

#pragma once

#include "Tr2DeviceResource.h"


class Tr2ProceduralBuffer
{
public:
	typedef Tr2BufferAL ResourceType;
	typedef ALResult( *FactoryFunction )( Tr2BufferAL& result, Tr2PrimaryRenderContext& renderContext );

	Tr2ProceduralBuffer( const BlueSharedString& name, FactoryFunction factory );

	const Tr2BufferAL& GetSharedResource() const;
private:
	const Tr2BufferAL* m_resource;
};
