////////////////////////////////////////////////////////////
//
//    Created:   May 2019
//    Copyright: CCP 2019
//

#pragma once

#ifndef Tr2DynamicBinding_h
#define Tr2DynamicBinding_h

#include "ITr2DynamicBindingOwner.h"

BLUE_DECLARE( TriValueBinding );


BLUE_CLASS( Tr2DynamicBinding ) :
	public INotify,
	public ISimTimeRebaseNotify
{
public:
	EXPOSE_TO_BLUE();
	Tr2DynamicBinding( IRoot* lockobj = NULL );
	~Tr2DynamicBinding();

	void Link();
	void Unlink();
	void SetOwner( ITr2DynamicBindingOwner* owner );
	void Update( Be::Time time );
	virtual void OnSimClockRebase( Be::Time oldTime, Be::Time newTime );

	bool IsDestinationValid() const;
	bool IsSourceValid() const;

	//////////////////////////////////////////////////////////////////////////////////////
	// INotify
	virtual bool OnModified( Be::Var* value );

private:
	BlueSharedString m_name;

	BlueSharedString m_destinationObjectPath;
	BlueSharedString m_destinationObjectAttribute;	
	BlueWeakRef<IRoot> m_destination;

	BlueSharedString m_sourceObjectPath;
	BlueSharedString m_sourceObjectAttribute;
	BlueWeakRef<IRoot> m_source;
	float m_scale;
	int32_t m_bindingDelay;

	Be::Time m_bindingTime;

	TriValueBindingPtr m_binding;
	ITr2DynamicBindingOwner* m_owner;
};
TYPEDEF_BLUECLASS( Tr2DynamicBinding );
BLUE_DECLARE_VECTOR( Tr2DynamicBinding );

#endif // Tr2DynamicBinding