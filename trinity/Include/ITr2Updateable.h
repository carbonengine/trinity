#pragma once
#ifndef ITr2Updatable_H
#define ITr2Updatable_H

BLUE_INTERFACE( ITr2Updateable ) : public IRoot
{
	virtual void Update( Be::Time realTime, Be::Time simTime ) = 0;
};

BLUE_DECLARE_IVECTOR( ITr2Updateable );

#endif