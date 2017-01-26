#include "StdAfx.h"
#include "EveSpaceObject2.h"
#include "TriConstants.h"
#include "Vector3.h"
#include "Tr2GrannyAnimation.h"
#include "Utilities/MatrixUtils.h"

BLUE_DEFINE_INTERFACE( IEveSpaceObjectChild );
BLUE_DEFINE_INTERFACE( IEveSpaceObject2 );
BLUE_DEFINE_INTERFACE( IEveShadowCaster );
BLUE_DEFINE_INTERFACE( IEveLightReceiver );
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
				Matrix boneTF;
				D3DXMatrixIdentity( &boneTF );
				TriMatrixCopyFrom3x4( &boneTF, &bones[boneIndex] );
				position = XMVector3TransformCoord( position, boneTF );

				rotation = XMQuaternionMultiply( rotation, XMQuaternionRotationMatrix( boneTF ) );
			}
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
	if( auto locators = BluePythonCast<LocatorStructureList*>( pyLocators ) )
	{
		PyObject* result = PyList_New( ssize_t( locators->GetSize() ) );
		for( size_t i = 0; i < locators->GetSize(); ++i )
		{
			auto& locator = ( *locators )[i];

			Vector3 position = locator.position;
			Quaternion rotation = locator.direction;
			TransformLocator( position, rotation, locator.boneIndex, pThis->m_animationUpdater );

			PyObject* tuple = PyTuple_New( 3 );
			PyTuple_SetItem( tuple, 0, Py_BuildValue( "(fff)", position.x, position.y, position.z ) );
			PyTuple_SetItem( tuple, 1, Py_BuildValue( "(ffff)", rotation.x, rotation.y, rotation.z, rotation.w ) );
			PyTuple_SetItem( tuple, 2, PyInt_FromLong( locator.boneIndex ) );
			PyList_SetItem( result, ssize_t( i ), tuple );
		}
		return result;
	}
	else if( auto locators = BluePythonCast<EveDamageLocatorStructureList*>( pyLocators ) )
	{
		PyObject* result = PyList_New( ssize_t( locators->GetSize() ) );
		for( size_t i = 0; i < locators->GetSize(); ++i )
		{
			auto& locator = ( *locators )[i];

			Vector3 position = locator.m_position;
			Quaternion rotation = locator.m_impactDirection;
			TransformLocator( position, rotation, locator.m_boneIndex, pThis->m_animationUpdater );

			PyObject* tuple = PyTuple_New( 3 );
			PyTuple_SetItem( tuple, 0, Py_BuildValue( "(fff)", position.x, position.y, position.z ) );
			PyTuple_SetItem( tuple, 1, Py_BuildValue( "(ffff)", rotation.x, rotation.y, rotation.z, rotation.w ) );
			PyTuple_SetItem( tuple, 2, PyInt_FromLong( locator.m_boneIndex ) );
			PyList_SetItem( result, ssize_t( i ), tuple );
		}
		return result;
	}
	else if( PySequence_Check( pyLocators ) )
	{
		PyObject* result = PyList_New( ssize_t( locators->GetSize() ) );
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

			if( !PyTuple_Check( item ) || !BlueExtractVector( PyTuple_GET_ITEM( item, 0 ), position, 3 ) || 
				!BlueExtractVector( PyTuple_GET_ITEM( item, 1 ), rotation, 4 ) || !PyInt_Check( PyTuple_GET_ITEM( item, 2 ) ) )
			{
				Py_DECREF( item );
				return PyErr_SetString( PyExc_TypeError, "arument must be a sequence of (position, rotation, boneIndex) tuples" ), nullptr;
			}
			int boneIndex = int( PyInt_AsLong( PyTuple_GET_ITEM( item, 2 ) ) );
			
			TransformLocator( position, rotation, boneIndex, pThis->m_animationUpdater );

			PyObject* tuple = PyTuple_New( 3 );
			PyTuple_SetItem( tuple, 0, Py_BuildValue( "(fff)", position.x, position.y, position.z ) );
			PyTuple_SetItem( tuple, 1, Py_BuildValue( "(ffff)", rotation.x, rotation.y, rotation.z, rotation.w ) );
			PyTuple_SetItem( tuple, 2, PyInt_FromLong( boneIndex ) );
			PyList_SetItem( result, ssize_t( i ), tuple );
			Py_DECREF( item );
		}
		return result;
	}
	return PyErr_SetString( PyExc_TypeError, "arument must be a sequence of (position, rotation, boneIndex) tuples" ), nullptr;
}
#endif

const Be::ClassInfo* EveSpaceObject2::ExposeToBlue()
{
    EXPOSURE_BEGIN( EveSpaceObject2, "" )
        MAP_INTERFACE( EveSpaceObject2 )
		MAP_INTERFACE( IEveShadowCaster )
		MAP_INTERFACE( IInitialize )
		MAP_INTERFACE( ITr2Pickable )
		MAP_INTERFACE( ITriTargetable )
		MAP_INTERFACE( IWorldPosition )
		MAP_INTERFACE( ITr2ShLightingReceiver )
		MAP_INTERFACE( INotify )
		MAP_INTERFACE( ITr2SecondaryLightSource )
		MAP_INTERFACE( ITr2ImpostorSource )

		MAP_ATTRIBUTE( "name", m_name, "", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "dna", m_dna, "If created by the SOF, this is the DNA string", Be::READ | Be::PERSIST )

		MAP_ATTRIBUTE
		( 
			"display", 
			m_display, 
			"Specifies whether to render the object or not", 
			Be::READWRITE | Be::PERSIST
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
			"enableShadow", 
			m_enableShadow, 
			"If set, shadow is enabled",
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
			m_meshLod, 
			"Mesh with levels-of-detail for rendering space object", 
			Be::READWRITE | Be::PERSIST
		)

		MAP_ATTRIBUTE
		(
			"shadowEffect",  
			m_shadowEffect, 
			"Effect used to render into shadow map. If not set, object can't cast shadows.", 
			Be::READWRITE | Be::PERSIST
		)

		MAP_ATTRIBUTE
		( 
			"damageLocators",    
			m_persistedDamageLocators,
			"Blue structure list of EveDamageLocators (Vector3 and a Quaternion) that are read\n"
			"into the class and transformed with the ship frame-by-frame to give new turrets\n"
			"locations to shoot at on it.", 
			Be::READWRITE | Be::PERSIST
		)

		MAP_ATTRIBUTE
		(
			"locatorSets",
			m_locatorSets,
			"Set of Blue structure lists of locators identified by a name",
			Be::READWRITE | Be::PERSIST
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
			"modelScale", 
			m_modelScale, 
			"Scaling of this object (ONLY USED BY ASTEROIDS!)",
			Be::READWRITE | Be::PERSIST
		)

		MAP_ATTRIBUTE
		(
			"spriteSets",
			m_spriteSets,
			"Sprite sets attached to the object",
			Be::READWRITE | Be::PERSIST
		)

		MAP_ATTRIBUTE
		(
			"spotlightSets",
			m_spotlightSets,
			"Spotlight sets attached to the object",
			Be::READWRITE | Be::PERSIST
		)

		MAP_ATTRIBUTE
		(
			"planeSets",
			m_planeSets,
			"Plane sets attached to the object",
			Be::READWRITE | Be::PERSIST
		)

		MAP_ATTRIBUTE
		(
			"spriteLineSets",
			m_spriteLineSets,
			"Sprite line sets attached to the object",
			Be::READWRITE | Be::PERSIST
		)

		MAP_ATTRIBUTE
		( 
			"locators", 
			m_locators, 
			"Locators for things such as turrets, boosters, etc. Locators can also come from a skeleton.",
			Be::READWRITE | Be::PERSIST
		)
		MAP_ATTRIBUTE
		( 
			"observers", 
			m_observers, 
			"Observers for pushing data between modules every frame. Currently used to push locator data out to the audio2 module.",
			Be::READWRITE | Be::PERSIST
		)

		MAP_ATTRIBUTE( "impactOverlay", m_impactOverlay, "object for rendering damage/impact fx on this space object.", Be::READWRITE )
		MAP_ATTRIBUTE( "decals", m_decals, "list of all decals on this space object.", Be::READWRITE | Be::PERSIST )

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

		MAP_ATTRIBUTE( "boundingSphereCenter", m_boundingSphereCenter, "The center of the minimum bounding sphere of the model in local coordinates", Be::READWRITE | Be::PERSIST )
		
		MAP_ATTRIBUTE( "boundingSphereRadius", m_boundingSphereRadius, "The radius of the minimum bounding sphere of the model", Be::READWRITE | Be::PERSIST )

		MAP_ATTRIBUTE( "lodLevel", m_lodLevel, "Current lod-level of this spaceobject\n", Be::READ )

		MAP_ATTRIBUTE( "customMasks", m_customMasks, "\n", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "dirtLevel", m_dirtLevel, "The dirt amount of the object", Be::READWRITE )
		MAP_ATTRIBUTE( "shapeEllipsoidCenter", m_shapeEllipsoidCenter, "User-authored ellipsoid data for center", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "shapeEllipsoidRadius", m_shapeEllipsoidRadius, "User-authored ellipsoid data for radii", Be::READWRITE | Be::PERSIST )

		MAP_ATTRIBUTE
		(
			"effectChildren",
			m_effectChildren,
			"",
			Be::READWRITE | Be::PERSIST
		)
		MAP_ATTRIBUTE
		(
			"children",
			m_children,
			"",
			Be::READWRITE | Be::PERSIST
		)
		MAP_ATTRIBUTE
		( 
			"curveSets", 
			m_curveSets, 
			"Curvesets for animating things", 
			Be::READWRITE | Be::PERSIST
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
			Be::READWRITE | Be::PERSIST
		)

		MAP_ATTRIBUTE
		(
			"animationSequencer",
			m_animationSequencer,
			"",
			Be::READWRITE | Be::NOTIFY
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

		MAP_ATTRIBUTE( "lights", m_lights, "List of dynamic lights", Be::READ | Be::PERSIST );

		MAP_ATTRIBUTE( "externalParameters", m_externalParameters, "List of external parameters to bind to object elements", Be::READ | Be::PERSIST )

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
