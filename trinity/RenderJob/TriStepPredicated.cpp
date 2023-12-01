////////////////////////////////////////////////////////////
//
//    Created:   July 2019
//    Copyright: CCP 2019
//

#include "StdAfx.h"
#include "TriStepPredicated.h"
#include "Include/ITr2NamedPredicate.h"


void TriStepPredicated::Create( const char* name, ITr2NamedPredicate* predicate, TriRenderStep* step )
{
	m_predicateName = name ? name : "";
	m_predicate = predicate;
	m_step = step;
}

TriStepResult TriStepPredicated::Execute( Be::Time realTime, Be::Time simTime, Tr2RenderContext& renderContext )
{
	if( m_step && m_predicate )
	{
		if( m_predicate->GetPredicate( m_predicateName.c_str() ) )
		{
			return m_step->Execute( realTime, simTime, renderContext );
		}
	}
	return RS_OK;
}

bool TriStepPredicated::GetPredicateValue() const
{
	if( m_predicate )
	{
		return m_predicate->GetPredicate( m_predicateName.c_str() );
	}
	return false;
}