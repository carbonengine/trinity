#pragma once
#ifndef ITr2VisibilityQueryable_H
#define ITr2VisibilityQueryable_H

BLUE_DECLARE_INTERFACE( ITr2VisibilityQueryable );
BLUE_DECLARE( Tr2VisibilityResults );

BLUE_INTERFACE( ITr2VisibilityQueryable ) : public IRoot
{
	virtual void VisibilityQuery( Tr2VisibilityResults* visibilityResults ) = 0;
	virtual void SetVisibilityResults( Tr2VisibilityResults* visibilityResults ) = 0;
};

#endif