// Copyright © 2019 CCP ehf.

#include "StdAfx.h"
#include "EveMultiEffect.h"

BLUE_DEFINE_INTERFACE( ITr2DynamicBindingOwner );

BLUE_DEFINE( EveMultiEffect );

const Be::ClassInfo* EveMultiEffect::ExposeToBlue()
{
	EXPOSURE_BEGIN( EveMultiEffect, "" )
		MAP_INTERFACE( IEveSpaceObject2 )
		MAP_INTERFACE( ITr2ControllerOwner )
		MAP_INTERFACE( IInitialize )
		MAP_INTERFACE( ITr2DynamicBindingOwner )
		MAP_INTERFACE( ITr2CurveSetOwner )

		MAP_ATTRIBUTE(
			"name",
			m_name,
			"The name of the multi effect",
			Be::READWRITE | Be::PERSIST )

		MAP_ATTRIBUTE(
			"parameters",
			m_parameters,
			"A list of objects that can be set from the python side",
			Be::READ | Be::PERSIST )

		MAP_ATTRIBUTE(
			"controllers",
			m_controllers,
			"A list of controllers that control the multieffect",
			Be::READ | Be::PERSIST )

		MAP_ATTRIBUTE(
			"externalParameters",
			m_externalParameters,
			"A list of external parameters that are linked to multieffect properties",
			Be::READ | Be::PERSIST )

		MAP_ATTRIBUTE(
			"curveSets",
			m_curveSets,
			"A list of curvesets",
			Be::READ | Be::PERSIST )

		MAP_ATTRIBUTE(
			"bindings",
			m_bindings,
			"A list of bindings between parameters/curves",
			Be::READ | Be::PERSIST )

		MAP_METHOD_AND_WRAP(
			"SetParameter",
			SetParameter,
			"Sets a named parameter\n"
			":param parameterName: the name of a defined parameter \n"
			":param object: the object that will be attached to the parameter\n"
			":return : True if it is successful, False otherwise" )

		MAP_METHOD_AND_WRAP(
			"SetControllerVariable",
			SetControllerVariable,
			"Sets a controller variable\n"
			":param name: the name of a controller variable\n"
			":param value: The value of the variable\n" )

		MAP_METHOD_AND_WRAP(
			"HandleControllerEvent",
			HandleControllerEvent,
			"Handles controller event of a particular name\n"
			":param name: the name of a event\n" )

		MAP_METHOD_AND_WRAP(
			"StartControllers",
			StartControllers,
			"Starts the controllers\n" )

		MAP_METHOD_AND_WRAP_OPTIONAL_ARGS(
			"Rebind",
			Rebind,
			1,
			"Rebinds all bindings to parameters\n"
			":param onlyUpdateBindings: true to only update bindings, false to also update controller refs" )

	EXPOSURE_END();
}
