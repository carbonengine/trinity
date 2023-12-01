#pragma once
#ifndef ITriObserverLocal_h_
#define ITriObserverLocal_h_

struct IBluePlacementObserver;

BLUE_INTERFACE( ITriObserverLocal ) : public IRoot
{
	virtual IBluePlacementObserver* GetObserver() = 0;
	virtual void SetObserver( IBluePlacementObserver* obs ) = 0;
};

#endif //ITriObserverLocal_h_