// Copyright © 2019 CCP ehf.

#pragma once

#include "Tr2FollowCurveKey.h"

#include <ITriFunction.h>

BLUE_DECLARE_IVECTOR( ITr2FollowCurveKey );

BLUE_CLASS( Tr2FollowCurve ) :
	public ITriVectorFunction,
	public IListNotify
{
public:
	Tr2FollowCurve( IRoot* lockobj = nullptr );
	~Tr2FollowCurve();

	EXPOSE_TO_BLUE();

	virtual void OnListModified( long event, ssize_t key, ssize_t key2, IRoot* value, const IList* list );
	virtual void UpdateValue( double time );

	Vector3 GetValue( double time ) const;

	virtual Vector3* Update( Vector3 * in, Be::Time time );
	virtual Vector3* Update( Vector3 * in, double time );
	virtual Vector3* GetValueAt( Vector3 * in, Be::Time time );
	virtual Vector3* GetValueAt( Vector3 * in, double time );
	virtual Vector3* GetValueDotAt( Vector3 * in, Be::Time time );
	virtual Vector3* GetValueDotAt( Vector3 * in, double time );
	virtual Vector3* GetValueDoubleDotAt( Vector3 * in, Be::Time time );
	virtual Vector3* GetValueDoubleDotAt( Vector3 * in, double time );
	virtual Vector3d* InterpolatedPosition( Vector3d * out, Be::Time time );

private:
	Vector3 GetSegmentValue( float time, ITr2FollowCurveKey& k0, ITr2FollowCurveKey& k1 ) const;

	std::string m_name;

	PITr2FollowCurveKeyVector m_keys;
	Vector3 m_currentValue;
};

TYPEDEF_BLUECLASS( Tr2FollowCurve );
