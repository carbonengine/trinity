////////////////////////////////////////////////////////////
//
//    Created:   March 2018
//    Copyright: CCP 2018
//

#pragma once

#include "ITr2ControllerAction.h"
#include "Controllers/Tr2BindingPoint.h"


BLUE_CLASS( Tr2ActionPlayMeshAnimation ) :
	public ITr2ControllerAction,
	public INotify
{
public:
	enum PlayAction
	{
		PLAY,
		ENQUEUE_PLAY,
	};
	enum StopAction
	{
		STOP,
		ENQUEUE_STOP,
		NONE,
	};
	enum class DestinationType
	{
		OWNER,
		CHILD,
	};

	Tr2ActionPlayMeshAnimation( IRoot* lockobj = nullptr );

	EXPOSE_TO_BLUE();

	void Link( Tr2Controller & controller ) override;
	void Unlink() override;
	void Start( Tr2Controller & controller ) override;
	void Stop( Tr2Controller & controller ) override;

	bool OnModified( Be::Var * value ) override;

	bool IsBindingValid() const;

	IRootPtr GetDestination() const;

private:
	void LinkDestination( const Tr2Controller& controller );
	bool HasDelayedBinding() const;

	Tr2BindingPoint m_destination;
	const Tr2Controller* m_controller;
	std::string m_animation;
	std::string m_mask;
	DestinationType m_destinationType;
	PlayAction m_playAction;
	StopAction m_stopAction;
	int32_t m_loops;
	float m_delay;
	float m_speed;
	bool m_delayBinding;
};

TYPEDEF_BLUECLASS( Tr2ActionPlayMeshAnimation );
