// Copyright © 2005 CCP ehf.

#ifndef _ITRITARGETABLE_H_
#define _ITRITARGETABLE_H_

BLUE_INTERFACE( ITriTargetable ) :
	IRoot
{
	enum ImpactConfiguration
	{
		IMPACT_INVALID = 0,
		IMPACT_SHIELD,
		IMPACT_ARMOR,
		IMPACT_HULL,
	};

	virtual unsigned int GetDamageLocatorCount() const = 0;
	virtual int GetClosestDamageLocatorIndex( const Vector3* position ) = 0;
	virtual bool GetDamageLocatorPosition( Vector3 * out, int index, bool inWorldSpace ) = 0;
	virtual bool GetDamageLocatorDirection( Vector3 * out, int index, bool inWorldSpace ) = 0;
	virtual void GetMissPosition( const Vector3* hit, const Vector3* source, Vector3* out ) = 0;
	virtual int GetGoodDamageLocatorIndex( const Vector3& position ) = 0;
	virtual float GetRadius() const = 0;
	virtual int CreateImpact( int damageLocatorIndex, const Vector3& direction, float lifeTime, float size ) = 0;
	virtual bool UpdateImpact( Vector3 & out, const Vector3& direction, int impactIndex ) = 0;
	virtual ImpactConfiguration GetImpactConfiguration() const
	{
		return ImpactConfiguration::IMPACT_INVALID;
	}
	virtual bool GetImpactPosition( Vector3 & out, int locator, const Vector3& posPrev, const Vector3& posNow, float epsilon ) = 0;
	virtual bool HasImpactConfigurationShield() const = 0;
	virtual void SetLastDamageLocatorHit( unsigned int locator )
	{
	}
};


#endif