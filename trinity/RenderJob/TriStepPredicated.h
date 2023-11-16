////////////////////////////////////////////////////////////
//
//    Created:   July 2019
//    Copyright: CCP 2019
//

#pragma once

#include "TriRenderStep.h"

BLUE_DECLARE_INTERFACE( ITr2NamedPredicate );

BLUE_CLASS( TriStepPredicated ): public TriRenderStep
{
public:
	EXPOSE_TO_BLUE();

	void Create( const char* name, ITr2NamedPredicate* predicate, TriRenderStep* step );
	TriStepResult Execute( Be::Time realTime, Be::Time simTime, Tr2RenderContext& renderContext ) override;
	bool GetPredicateValue() const;
private:
	std::string m_predicateName;
	ITr2NamedPredicatePtr m_predicate;
	TriRenderStepPtr m_step;
};

TYPEDEF_BLUECLASS( TriStepPredicated );