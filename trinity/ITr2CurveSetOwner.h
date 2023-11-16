#pragma once


BLUE_DECLARE( TriCurveSet );


BLUE_INTERFACE( ITr2CurveSetOwner ): public IRoot
{
	virtual void PlayCurveSet( const std::string& name, const std::string& rangeName ) = 0;
	virtual void StopCurveSet( const std::string& name ) = 0;
	virtual void UpdateCurveSet( const std::string& name, Be::Time time ) {};
	virtual float GetCurveSetDuration( const std::string& name ) const = 0;
	virtual float GetRangeDuration( const std::string& name, const std::string& rangeName ) const = 0;
	virtual void PlayAllCurveSets() {};
	virtual void StopAllCurveSets() {};
};