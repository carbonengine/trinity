// Copyright © 2026 CCP ehf.

#include "StdAfx.h"
#include "EveChildPartData.h"
#include <numeric>


EveChildPartData::EveChildPartData( IRoot* )
{
}

EveSpaceObjectChild::PartTag EveChildPartData::GetUnusedPartID() const
{
	return std::accumulate( m_parts.begin(), m_parts.end(), 1u, []( EveSpaceObjectChild::PartTag maxId, const PartData& part ) {
		return std::max( maxId, part.partId + 1 );
	} );
}