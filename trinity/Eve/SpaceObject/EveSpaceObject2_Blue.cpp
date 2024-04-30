#include "StdAfx.h"
#include "EveSpaceObject2.h"
#include "TriConstants.h"
#include "Tr2GrannyAnimation.h"
#include "Utilities/MatrixUtils.h"
#include "Eve/SpaceObject/Utils/EveLocator2.h"
#include "Eve/SpaceObject/Children/EveChildInheritProperties.h"

BLUE_DEFINE_INTERFACE( IEveSpaceObjectChild );
BLUE_DEFINE_INTERFACE( IEveSpaceObject2 );
BLUE_DEFINE_INTERFACE( IEveShadowCaster );
BLUE_DEFINE_INTERFACE( IEveLightReceiver );
BLUE_DEFINE_INTERFACE( IEveEffectChildrenOwner );
BLUE_DEFINE_INTERFACE( IEveSpaceObjectDecalOwner );
BLUE_DEFINE_INTERFACE( IEveInheritPropertiesOwner )
BLUE_DEFINE_INTERFACE( IEveSpaceObjectAttachment );
BLUE_DEFINE_INTERFACE( ITr2SoundEmitterOwner );
BLUE_DEFINE_INTERFACE( IEveSceneRegistrationObject );
BLUE_DEFINE_INTERFACE( IEveReflectionRenderable );
BLUE_DEFINE_INTERFACE( ITr2LightOwner );
BLUE_DEFINE_INTERFACE( IEveSpaceObjectAttachmentOwner );
BLUE_DEFINE_ABSTRACT( EveSpaceObject2 );

#if BLUE_WITH_PYTHON

namespace
{
	void TransformLocator( Vector3& position, Quaternion& rotation, int boneIndex, Tr2GrannyAnimation* animation )
	{
		if( boneIndex > 0 && animation && animation->IsInitialized() )
		{
			size_t boneCount = size_t( animation->GetMeshBoneCount() );
			if( boneCount )
			{
				const granny_matrix_3x4* bones = animation->GetMeshBoneMatrixList();
				Matrix boneTF = IdentityMatrix();
				TriMatrixCopyFrom3x4( &boneTF, &bones[boneIndex] );
				position = XMVector3TransformCoord( position, boneTF );

				rotation = XMQuaternionMultiply( rotation, XMQuaternionRotationMatrix( boneTF ) );
			}
		}
	}

	void ApplyModelTransform( Vector3& position, Quaternion& rotation, ITriVectorFunctionPtr modelTranslationCurve, ITriQuaternionFunctionPtr modelRotationCurve )
	{
		if( modelTranslationCurve )
		{
			Vector3 pos( 0, 0, 0 );
			modelTranslationCurve->GetValueAt( &pos, Be::Time() );
			position += pos;
		}

		if( modelRotationCurve )
		{
			Quaternion quat( 0, 0, 0, 1 );
			modelRotationCurve->GetValueAt( &quat, Be::Time() );
			position = XMVector3Rotate( position, quat );
			rotation = XMQuaternionMultiply( rotation, quat );
		}
	}
}

PyObject* EveSpaceObject2::PyTransformLocators( PyObject* self, PyObject* args )
{
	auto pThis = BluePythonCast<EveSpaceObject2*>( self );
	PyObject* pyLocators = nullptr;
	if( !PyArg_ParseTuple( args, "O", &pyLocators ) )
	{
		return nullptr;
	}

	auto modelTranslationCurve = pThis->GetModelTranslationCurve();
	auto modelRotationCurve = pThis->GetModelRotationCurve();

	if( auto locators = BluePythonCast<LocatorStructureList*>( pyLocators ) )
	{
		PyObject* result = PyList_New( ssize_t( locators->GetSize() ) );
		for( size_t i = 0; i < locators->GetSize(); ++i )
		{
			auto& locator = ( *locators )[i];

			Vector3 position = locator.position;
			Quaternion rotation = locator.direction;

			TransformLocator( position, rotation, locator.boneIndex, pThis->m_animationUpdater );
			if( modelTranslationCurve || modelRotationCurve )
				ApplyModelTransform( position, rotation, modelTranslationCurve, modelRotationCurve );

			PyObject* tuple = PyTuple_New( 3 );
			PyTuple_SetItem( tuple, 0, Py_BuildValue( "(fff)", position.x, position.y, position.z ) );
			PyTuple_SetItem( tuple, 1, Py_BuildValue( "(ffff)", rotation.x, rotation.y, rotation.z, rotation.w ) );
			PyTuple_SetItem( tuple, 2, PyInt_FromLong( locator.boneIndex ) );
			PyList_SetItem( result, ssize_t( i ), tuple );
		}
		return result;
	}
	else if( PySequence_Check( pyLocators ) )
	{
		PyObject* result = PyList_New( PySequence_Size( pyLocators ) );
		for( ssize_t i = 0; ; ++i )
		{
			auto item = PySequence_GetItem( pyLocators, i );
			if( !item )
			{
				PyErr_Clear();
				break;
			}
			Vector3 position;
			Quaternion rotation;

			if( !PyTuple_Check( item ) || !BlueExtractVector( PyTuple_GET_ITEM( item, 0 ), &position.x, 3 ) ||
				!BlueExtractVector( PyTuple_GET_ITEM( item, 1 ), &rotation.x, 4 ) || !PyInt_Check( PyTuple_GET_ITEM( item, 2 ) ) )
			{
				Py_DECREF( item );
                PyErr_SetString( PyExc_TypeError, "arument must be a sequence of (position, rotation, boneIndex) tuples" );
				return nullptr;
			}
			int boneIndex = int( PyInt_AsLong( PyTuple_GET_ITEM( item, 2 ) ) );

			TransformLocator( position, rotation, boneIndex, pThis->m_animationUpdater );
			if( modelTranslationCurve || modelRotationCurve )
				ApplyModelTransform( position, rotation, modelTranslationCurve, modelRotationCurve );

			PyObject* tuple = PyTuple_New( 3 );
			PyTuple_SetItem( tuple, 0, Py_BuildValue( "(fff)", position.x, position.y, position.z ) );
			PyTuple_SetItem( tuple, 1, Py_BuildValue( "(ffff)", rotation.x, rotation.y, rotation.z, rotation.w ) );
			PyTuple_SetItem( tuple, 2, PyInt_FromLong( boneIndex ) );
			PyList_SetItem( result, ssize_t( i ), tuple );
			Py_DECREF( item );
		}
		return result;
	}
    PyErr_SetString( PyExc_TypeError, "arument must be a sequence of (position, rotation, boneIndex) tuples" );
	return nullptr;
}
#endif

const Be::ClassInfo* EveSpaceObject2::ExposeToBlue()
{
	EXPOSURE_BEGIN( EveSpaceObject2, "" )
		MAP_INTERFACE( EveSpaceObject2 )
		MAP_INTERFACE( EveEntity )
		MAP_INTERFACE( IEveShadowCaster )
		MAP_INTERFACE( IInitialize )
		MAP_INTERFACE( ITr2Pickable )
		MAP_INTERFACE( ITriTargetable )
		MAP_INTERFACE( IWorldPosition )
		MAP_INTERFACE( ITr2ShLightingReceiver )
		MAP_INTERFACE( INotify )
		MAP_INTERFACE( ITr2SecondaryLightSource )
		MAP_INTERFACE( ITr2ImpostorSource )
		MAP_INTERFACE( ITr2CurveSetOwner )
		MAP_INTERFACE( IEveEffectChildrenOwner )
		MAP_INTERFACE( IShaderConfigurer )
		MAP_INTERFACE( ITr2SoundEmitterOwner )
		MAP_INTERFACE( ITr2ControllerOwner )
		MAP_INTERFACE( ITr2GrannyAnimationOwner )
		MAP_INTERFACE( IEveInheritPropertiesOwner )
		MAP_INTERFACE( IEveSpaceObjectDecalOwner )
		MAP_INTERFACE( ITr2LightOwner )
		MAP_INTERFACE( IEveSpaceObjectAttachmentOwner )

		MAP_ATTRIBUTE( "name", m_name, "", Be::READWRITE | Be::NOTIFY | Be::PERSIST )
		MAP_ATTRIBUTE( "dna", m_dna, "If created by the SOF, this is the DNA string", Be::READ | Be::PERSIST )
		MAP_ATTRIBUTE( "activationStrength", m_activationStrength, "Ship's activation strength", Be::READWRITE )

		MAP_ATTRIBUTE
		(
			"display",
			m_display,
			"Specifies whether to render the object or not",
			Be::READWRITE | Be::PERSIST | Be::NOTIFY
		)

		MAP_ATTRIBUTE
		(
			"update",
			m_update,
			"Specifies whether to update the object or not",
			Be::READWRITE | Be::PERSIST
		)

		MAP_ATTRIBUTE
		(
			"castShadow",
			m_castShadow,
			"If set, shadow is enabled",
			Be::READWRITE | Be::NOTIFY | Be::PERSIST
		)

		MAP_ATTRIBUTE
		(
			"isAnimated",
			m_isAnimated,
			"If set, we have animations",
			Be::READWRITE | Be::PERSIST 
		)

		MAP_ATTRIBUTE
		(
			"isPickable",
			m_isPickable,
			"Enables/disables picking for the object",
			Be::READWRITE
		)

		MAP_ATTRIBUTE
		(
			"mesh",
			m_mesh,
			"Mesh for rendering space object",
			Be::READWRITE | Be::PERSIST
		)

		MAP_ATTRIBUTE
		(
			"meshLod",
			m_mesh,
			"Mesh with levels-of-detail for rendering space object\n:jessica-hidden: True",
			Be::READWRITE | Be::PERSIST
		)

		MAP_ATTRIBUTE
		(
			"locatorSets",
			m_locatorSets,
			"Set of Blue structure lists of locators identified by a name",
			Be::READ | Be::PERSIST
		)

		MAP_ATTRIBUTE
		(
			"translationCurve",
			m_ballPosition,
			"Vector function slot for attaching a destiny ball to set the position of a ship",
			Be::READWRITE | Be::PERSIST
		)

		MAP_ATTRIBUTE
		(
			"rotationCurve",
			m_ballRotation,
			"Quaternion function slot for attaching a destiny ball to set the rotation of a ship",
			Be::READWRITE | Be::PERSIST
		)

		MAP_ATTRIBUTE
		(
			"modelRotationCurve",
			m_modelRotation,
			"Used to add rotations to the basic rotation curve",
			Be::READWRITE | Be::PERSIST
		)

		MAP_ATTRIBUTE
		(
			"modelTranslationCurve",
			m_modelTranslation,
			"Used to add animated translations to ships",
			Be::READWRITE | Be::PERSIST
		)

		MAP_ATTRIBUTE
		(
			"worldPosition",
			m_worldPosition,
			"Position in world space",
			Be::READ
		)

		MAP_ATTRIBUTE
		(
			"worldRotation",
			m_worldRotation,
			"Rotation in world space",
			Be::READ 
		)

		MAP_ATTRIBUTE
		(
			"modelScale",
			m_modelScale,
			"Scaling of this object (ONLY USED BY ASTEROIDS!)",
			Be::READWRITE | Be::PERSIST
		)

		MAP_ATTRIBUTE
		(
			"attachments",
			m_attachments,
			"Item sets attached to the object",
			Be::READ | Be::PERSIST
		)

		MAP_ATTRIBUTE
		(
			"locators",
			m_locators,
			"Locators for things such as turrets, boosters, etc. Locators can also come from a skeleton.",
			Be::READ | Be::PERSIST
		)
		MAP_ATTRIBUTE
		(
			"observers",
			m_observers,
			"Observers for pushing data between modules every frame. Currently used to push locator data out to the audio2 module.",
			Be::READ | Be::PERSIST
		)

		MAP_ATTRIBUTE( "impactOverlay", m_impactOverlay, "object for rendering damage/impact fx on this space object.", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "decals", m_decals, "list of all decals on this space object.", Be::READ | Be::PERSIST )

		MAP_ATTRIBUTE
		(
			"estimatedPixelDiameter",
			m_estimatedPixelDiameter,
			"Estimated pixel diameter given the current view/projection transforms.\n",
			Be::READ
		)
		MAP_ATTRIBUTE
		(
			"estimatedPixelDiameterWithChildren",
			m_estimatedPixelDiameterWithChildren,
			"Estimated pixel diameter given the current view/projection transforms.\n",
			Be::READ
		)

		MAP_ATTRIBUTE( "clipSphereFactor", m_clipSphereFactor, "Object's clip state", Be::READWRITE | Be::NOTIFY )
		MAP_ATTRIBUTE( "clipSphereCenter", m_clipSphereCenter, "Object's clip sphere center", Be::READWRITE | Be::PERSIST )

		MAP_ATTRIBUTE( "boundingSphereCenter", m_boundingSphereCenter, "The center of the minimum bounding sphere of the model in local coordinates", Be::READWRITE | Be::PERSIST )

		MAP_ATTRIBUTE( "boundingSphereRadius", m_boundingSphereRadius, "The radius of the minimum bounding sphere of the model", Be::READWRITE | Be::PERSIST )

		MAP_ATTRIBUTE( "lodLevel", m_lodLevel, "Current lod-level of this spaceobject\n", Be::READ )

		MAP_ATTRIBUTE( "customMasks", m_customMasks, "\n", Be::READ | Be::PERSIST )
		MAP_ATTRIBUTE( "dirtLevel", m_dirtLevel, "The dirt amount of the object", Be::READWRITE | Be::NOTIFY )
		MAP_ATTRIBUTE( "shapeEllipsoidCenter", m_shapeEllipsoidCenter, "User-authored ellipsoid data for center", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "shapeEllipsoidRadius", m_shapeEllipsoidRadius, "User-authored ellipsoid data for radii", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "generatedShapeEllipsoidCenter", m_generatedShapeEllipsoidCenter, "Generated or User-authored ellipsoid data for center", Be::READ )
		MAP_ATTRIBUTE( "generatedShapeEllipsoidRadius", m_generatedShapeEllipsoidRadius, "Generated or User-authored ellipsoid data for radii", Be::READ )

		MAP_ATTRIBUTE
		(
			"effectChildren",
			m_effectChildren,
			"",
			Be::READ | Be::PERSIST
		)
		MAP_ATTRIBUTE
		(
			"children",
			m_children,
			"",
			Be::READ | Be::PERSIST
		)
		MAP_ATTRIBUTE
		(
			"curveSets",
			m_curveSets,
			"Curvesets for animating things",
			Be::READ | Be::PERSIST
		)

		MAP_ATTRIBUTE
		(
			"inheritProperties",
			m_inheritProperties,
			"",
			Be::READWRITE
		)

		MAP_ATTRIBUTE( "dynamicBoundingSphereEnabled", m_dynamicBoundingSphereEnabled, "Indicate if object uses dynamic bounding spheres", Be::READ | Be::PERSIST )

		MAP_METHOD_AND_WRAP( "GetBoneCount", GetBoneCount, "Returns the number of bones in the granny." )
		MAP_METHOD_AND_WRAP
		(
			"GetBoundingSphereRadius",
			GetBoundingSphereRadius,
			"Returns the bounding sphere radius."
		)
		MAP_METHOD_AND_WRAP
		(
			"GetBoundingSphereCenter",
			GetBoundingSphereCenter,
			"Returns the bounding sphere center."
		)

		MAP_METHOD_AND_WRAP
		(
			"CalculateSkinnedBoundingSphere",
			CalculateSkinnedBoundingSphere,
			"Calculate and return skinned bounding sphere."
		)
		MAP_METHOD_AND_WRAP
		(
			"CalculateSkinnedBoundingBoxFromTransform",
			CalculateSkinnedBoundingBoxFromTransform,
			"Calculate and return bounding sphere of the object projected by the transform provided.\n"
			":param transform: object transform matrix"
		)

		MAP_METHOD_AND_WRAP
		(
			"RebuildBoundingSphereInformation",
			RebuildBoundingSphereInformation,
			"Rebuilds the bounding data."
		)
		MAP_METHOD_AND_WRAP
		(
			"GetLocalBoundingBox",
			GetLocalBoundingBoxFromScript,
			"Returns the bounding box of the object in local coordinates."
		)
		MAP_METHOD_AND_WRAP
		(
			"PlayAnimation",
			PlayAnimationOnce,
			"Plays the given animation, replacing whatever animation was playing before.\n"
			":param name: animation name"
		)
		MAP_METHOD_AND_WRAP
		(
			"PlayAnimationEx",
			PlayAnimationEx,
			"Plays the given animation, replacing whatever animation was playing before.\n"
			":param name: animation name\n"
			":param loopCount: can be 0 to loop forever.\n"
			":param delay: time (in seconds) from now before animation should start playing.\n"
			":param speed: can be used speed up or slow down playback - use negative values to play backwards.\n"
		)
		MAP_METHOD_AND_WRAP
		(
			"ChainAnimation",
			ChainAnimation,
			"Plays the given animation, starting when currently playing animation finishes.\n"
			"If it is looping then it is replaced at the end of the current loop.\n"
			":param name: animation name"
		)
		MAP_METHOD_AND_WRAP
		(
			"ChainAnimationEx",
			ChainAnimationEx,
			"Plays the given animation, starting when currently playing animation finishes.\n"
			"If it is looping then it is replaced at the end of the current loop.\n"
			":param name: animation name\n"
			":param loopCount: can be 0 to loop forever.\n"
			":param delay: time (in seconds) from now before animation should start playing.\n"
			":param speed: can be used speed up or slow down playback - use negative values to play backwards.\n"
		)
		MAP_METHOD_AND_WRAP
		(
			"EndAnimation",
			EndAnimation,
			"EndAnimation()\n\n"
			"Stops currently playing animation at the end of the current loop iteration."
		)
		MAP_METHOD_AND_WRAP
		(
			"ClearAnimations",
			ClearAnimations,
			"ClearAnimations()\n\n"
			"Abruptly ends all animations."
		)
		MAP_METHOD_AND_WRAP
		(
			"FreezeHighDetailMesh",
			FreezeHighDetailMesh,
			"Freezes the high detail mesh and prevents LOD selection or resource unloading."
		)

		MAP_ATTRIBUTE
		(
			"overlayEffects",
			m_overlayEffects,
			"A list of effects that are added to the current LOD. Rendered additive.",
			Be::READ | Be::PERSIST
		)

		MAP_ATTRIBUTE
		(
			"animationUpdater",
			m_animationUpdater,
			"Granny animation exposure",
			Be::READ
		)

		MAP_ATTRIBUTE
		(
			"positionDelta",
			m_positionDelta,
			"Change in global position of the object during the frame",
			Be::READ
		)

		MAP_ATTRIBUTE
		(
			"albedoColor",
			m_albedoColor,
			"Space object overall albedo color. Used for secondary lighting. In linear RGB space.",
			Be::READWRITE
		)

		MAP_ATTRIBUTE
		(
			"secondaryLightingSphereRadius",
			m_secondaryLightingSphereRadius,
			"Sphere radius used to approximate space object for secondary lighting.",
			Be::READ
		)

		MAP_ATTRIBUTE
		(
			"lastDamageLocatorHit",
			m_lastDamageLocatorHit,
			"The last damagelocator hit.",
			Be::READ
		)

		MAP_METHOD_AND_WRAP( "GetDamageLocatorCount", GetDamageLocatorCount, "Get number of damage locators on this ship" )
		MAP_METHOD_AND_WRAP(
			"GetLocatorCount",
			GetLocatorCount,
			"Get number of locators on this model. \n"
			":param locatorSetName: name of locatorSet" )
		MAP_METHOD_AND_WRAP(
			"GetDamageLocator",
			GetDamageLocator,
			"Get the position of indexed damage locator, (0,0,0) is returned for indices out of range.\n"
			":param idx: locator index" )
		MAP_METHOD_AND_WRAP(
			"GetDamageLocatorDirection",
			GetDamageLocatorDirectionLocal,
			"Get the direction of indexed damage locator, (0,0,0) is returned for indices out of range.\n"
			":param idx: locator index" )
		MAP_METHOD_AND_WRAP(
			"GetTransformedDamageLocator",
			GetTransformedDamageLocator,
			"Get the position of indexed damage locator, (0,0,0) is returned for indices out of range.\n"
			":param idx: locator index" )
		MAP_METHOD_AND_WRAP(
			"SetImpactDamageState",
			SetImpactDamageState,
			"Set the damage states of shield and armor and hull.\n"
			":param shield: shield damage\n"
			":param armor: armor damage\n"
			":param hull: hull damage\n"
			":param createArmorImpacts: True to create armor impacts\n"
		)
		MAP_METHOD_AND_WRAP(
			"SetImpactAnimation",
			SetImpactAnimation,
			"Set the impact's animations.\n"
			":param name: animation name\n"
			":param enable: enable/disable animation\n"
			":param duration: animation duration\n"
		)
		MAP_METHOD_AND_WRAP(
			"GetGoodLocatorIndex",
			GetCloseLocatorIndex,
			"Get the closest locator in set to target.\n"
			":param position: position of target\n"
			":param locatorSetName: name of locator set \n"
		)MAP_METHOD_AND_WRAP(
			"GetCloseLocatorIndex",
			GetCloseLocatorIndex,
			"Get the closest locator in set to target, does not mind about direction of locator.\n"
			":param position: position of target\n"
			":param locatorSetName: name of locator set \n"
		)
		MAP_METHOD_AND_WRAP(
			"GetLocatorPositionFromSet",
			GetLocatorPositionFromSet,
			"locator position from a set Specified by name\n"
			":param index: locator index\n"
			":param inWorldSpace: position of target\n"
			":param locatorSetName: name of locator set \n" 
		)
		MAP_METHOD_AND_WRAP(
			"GetLocatorRotationFromSet",
			GetLocatorRotationFromSet,
			"locator rotation in worldspace\n"
			":param index: locator index\n"
			":param inWorldSpace: position of target\n"
			":param locatorSetName: name of locator set \n" 
		)
		MAP_METHOD_AND_WRAP( "ClearImpactDamage", ClearImpactDamage, "Clear all the impact/damage effects." )
		MAP_METHOD_AND_WRAP(
			"CreateImpact",
			CreateImpact,
			"debug only\n"
			":param idx: damage locator index\n"
			":param direction: incoming damage direction\n"
			":param lifeTime: effect time\n"
			":param size: effect size"
		);
	MAP_METHOD_AND_WRAP(
		"CreateImpactFromPosition",
		CreateImpactFromPosition,
		"Creates an impact facing a position\n"
		":param position: damage source position\n"
		":param direction: incoming damage direction\n"
		":param lifeTime: effect time\n"
		":param size: effect size"
	);
	MAP_METHOD_AND_WRAP( "IsImpostor", IsImpostor, "Is this object in the impostor mode?" );

	MAP_ATTRIBUTE
	(
		"modelWorldPosition",
		m_boundingSphereWorldCenter,
		"The spaceobject's model world position",
		Be::READ
	)

	MAP_ATTRIBUTE_WITH_CHOOSER( "reflectionMode", m_reflectionMode, "When is this object rendered into the cubemap", Be::READWRITE | Be::PERSIST | Be::NOTIFY | Be::ENUM, EntityComponents::ReflectionModeChooser );

	MAP_ATTRIBUTE( "lights", m_lights, "List of dynamic lights", Be::READ | Be::PERSIST | Be::NOTIFY );

	MAP_ATTRIBUTE( "externalParameters", m_externalParameters, "List of external parameters to bind to object elements", Be::READ | Be::PERSIST )
		MAP_ATTRIBUTE( "controllers", m_controllers, "List of object controllers", Be::READ | Be::PERSIST )

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

		MAP_METHOD_AND_WRAP(
			"GetControllerVariables",
			GetControllerVariables,
			"Returns all previously set contrller variables"
		)

		MAP_METHOD_AND_WRAP(
			"GetLocatorTransform",
			GetEveLocatorTransform,
			"Returns locator to object transform (taking bone bindings in account)\n"
			":param locator: locator name belonging to this object"
		)

		MAP_METHOD_AND_WRAP(
			"GetLastUsedMeshLod",
			GetLastUsedMeshLod,
			"Returns last used mesh LOD index. For debugging purposes" )

        MAP_METHOD_AND_WRAP(
            "SetProceduralContainerVariable",
            SetProceduralContainerVariable,
            "Set variable for all applicable ProceduralContainer\n"
            ":param name: variable name\n"
            ":param value: new variable value\n"
        )

#if BLUE_WITH_PYTHON
		MAP_METHOD(
			"TransformLocators",
			PyTransformLocators,
			"Transforms a sequence of locators using current bone setup\n"
			":param locators: Locator data (position, rotation, boneIndex)\n"
			":type locators: sequence[((float, float, float), (float, float, float, float), int)]" )
#endif

	EXPOSURE_END()
}
