////////////////////////////////////////////////////////////
//
//    Created:   2015
//    Copyright: CCP 2015
//
#include "StdAfx.h"
#include "EveChildContainer.h"


Be::VarChooser EveSpaceObjectChildOriginChooser[] =
{
	{ "SPACE", BeCast( IEveSpaceObjectChild::SPACE ), "Origin in Space" },
	{ "SOF", BeCast( IEveSpaceObjectChild::SOF ), "Origin in SOF" },
	{ 0 }
};
BLUE_REGISTER_ENUM_EX( "EveSpaceObjectChildOrigin", IEveSpaceObjectChild::Origin, EveSpaceObjectChildOriginChooser, ENUM_REG_ENUM_OBJECT_ON_MODULE );

Be::VarChooser EveChildContainerDataSetShaderChooser[] =
{
	{ "None_", BeCast(EveChildContainer::SHADER_ALL), "Visible to users with all shader settings" },
	{ "Medium_and_High", BeCast(EveChildContainer::SHADER_HIGHMID), "Visible for users with shader settings on Medium or High" },
	{ "Low_and_Medium", BeCast(EveChildContainer::SHADER_LOWMID), "Visible for users with shader settings on Low or Medium" },
	{ "High", BeCast(EveChildContainer::SHADER_HIGH), "Only visible for users with shader settings on High" },
	{ "Medium", BeCast(EveChildContainer::SHADER_MED), "Only visible for users with shader settings on Medium" },
	{ "Low", BeCast(EveChildContainer::SHADER_LOW), "Only visible for users with shader settings on Low" },
	{ "Only Reflections", BeCast(EveChildContainer::ONLY_REFLECTIONS), "Only visible in the reflections" },
	{ 0 }
};
BLUE_REGISTER_ENUM_EX("SetShader", EveChildContainer::DisplayQualityModifier, EveChildContainerDataSetShaderChooser, ENUM_REG_ENUM_OBJECT_ON_MODULE);


BLUE_DEFINE( EveChildContainer );


const Be::ClassInfo* EveChildContainer::ExposeToBlue()
{
    EXPOSURE_BEGIN( EveChildContainer, "" )
        MAP_INTERFACE( EveChildContainer )
        MAP_INTERFACE( EveEntity )
		MAP_INTERFACE( IEveSpaceObjectChild )
		MAP_INTERFACE( ITr2CurveSetOwner )
		MAP_INTERFACE( IInitialize )
		MAP_INTERFACE( IListNotify )
		MAP_INTERFACE( INotify )
		MAP_INTERFACE( IEveEffectChildrenOwner )
		MAP_INTERFACE( IShaderConfigurer )
		MAP_INTERFACE( ITr2SoundEmitterOwner )
		MAP_INTERFACE( ITr2ControllerOwner )
		MAP_INTERFACE( IEveInheritPropertiesOwner )
		MAP_INTERFACE( IEveSpaceObjectAttachmentOwner )
		MAP_INTERFACE( ITr2Renderable )

		MAP_ATTRIBUTE( "name", m_name, "", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "display", m_display, "", Be::READWRITE | Be::PERSIST | Be::NOTIFY )
		MAP_ATTRIBUTE( "objects", m_objects, "", Be::READ | Be::PERSIST )
		MAP_ATTRIBUTE( "transformModifiers", m_transformModifiers, "", Be::READ | Be::PERSIST )
		MAP_ATTRIBUTE( "curveSets", m_curveSets, "", Be::READ | Be::PERSIST )
		MAP_ATTRIBUTE( "alwaysOn", m_isAlwaysOn, "If false this will be hidden if a spaceobjects activation strength < 0.5. If True then it is always on.", Be::READWRITE | Be::PERSIST )

		MAP_ATTRIBUTE_WITH_CHOOSER("displayFilter", m_displayFilter, "Choose the shader quality settings for users you'd like to display childs to", Be::READWRITE | Be::PERSIST | Be::ENUM | Be::NOTIFY, EveChildContainerDataSetShaderChooser )
		MAP_PROPERTY_READONLY("isRendering", IsRendering, "Are the current childs being rendered with the current filter and shader settings")
		MAP_PROPERTY_READONLY("isUpdating", IsUpdating, "Are the current childs being updated with the current filter and shader settings")

		MAP_ATTRIBUTE( "translation", m_translation, "", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "rotation", m_rotation, "", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "scaling", m_scaling,"", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "localTransform", m_localTransform, "", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "worldTransform", m_worldTransform, "", Be::READ )
		MAP_ATTRIBUTE( "useSRT", m_useSRT, "Should local transform be built from scaling, rotation and translation attributes.", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "staticTransform", m_staticTransform, "Does local transform need to be rebuilt every frame.", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "useStaticRotation", m_useStaticRotation, "Should this container ignore the parent rotation.", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "useStaticScale", m_useStaticScale, "Should this container ignore the parent scale.", Be::READWRITE | Be::PERSIST )
		
		MAP_ATTRIBUTE( "observers", m_observers, "List of audio observers", Be::READ | Be::PERSIST )
		MAP_ATTRIBUTE( "lights", m_lights, "List of dynamic lights", Be::READ | Be::PERSIST )
		MAP_ATTRIBUTE( "fxAttributes", m_fxAttributes, "List of dynamic fxAttributes", Be::READ | Be::PERSIST )
		MAP_ATTRIBUTE( "controllers", m_controllers, "List of object controllers", Be::READ | Be::PERSIST )
		MAP_ATTRIBUTE( "inheritProperties", m_inheritProperties, "Properties inherited from the parent ship when loaded through SOF", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "origin", m_origin, "Where did this effect originate from", Be::READ )
		MAP_ATTRIBUTE( "attachments", m_attachments, "Item sets attached to the object", Be::READ | Be::PERSIST )
		MAP_ATTRIBUTE( "animationOwner", m_animationOwner, "The driving force for the animations", Be::READ )


		MAP_METHOD_AND_WRAP( "RebuildLocalTransform", RebuildLocalTransform, "Rebuilds local transform." )

        MAP_METHOD_AND_WRAP(
            "SetProceduralContainerVariable",
            SetProceduralContainerVariable,
            "Set variable for all applicable ProceduralContainers\n"
            ":param name: variable name\n"
            ":param value: new variable value\n"
        )

		MAP_METHOD_AND_WRAP(
			"SetControllerVariable",
			SetControllerVariable,
			"Set variable for all applicable controllers\n"
			":param name: variable name\n"
			":param value: new variable value\n"
		)

		MAP_METHOD_AND_WRAP(
			"HandleControllerEvent",
			HandleControllerEvent,
			"Pass an event to controllers\n"
			":param name: event name"
		)

		MAP_METHOD_AND_WRAP(
			"StartControllers",
			StartControllers,
			"Start all controllers"
		)

    EXPOSURE_END()
}