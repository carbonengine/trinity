#ifndef ITRICURVELENGTH_H
#define ITRICURVELENGTH_H

BLUE_DECLARE_INTERFACE(ITriCurveLength);

BLUE_INTERFACE(ITriCurveLength): public IRoot
{
public:
	// Returns the length of the curve in seconds
	virtual float Length() = 0;		
};

#endif