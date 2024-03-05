#pragma once
#ifndef ITr2ValueBinding_H
#define ITr2ValueBinding_H

BLUE_INTERFACE( ITr2ValueBinding ) : public IRoot
{
	virtual void CopyValue() = 0;
};

#endif