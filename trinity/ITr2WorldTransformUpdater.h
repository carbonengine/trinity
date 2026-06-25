// Copyright © 2023 CCP ehf.

#pragma once
#ifndef ITr2WorldTransformUpdater_H
#define ITr2WorldTransformUpdater_H

BLUE_INTERFACE( ITr2WorldTransformUpdater ) :
	public IRoot
{
	virtual void UpdateTransform( Be::Time time, Matrix * transform ) = 0;
};


#endif // ITr2WorldTransformUpdater_H