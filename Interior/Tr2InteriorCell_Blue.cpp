#include "StdAfx.h"

#if INTERIORS_ENABLED

#include "Tr2InteriorCell.h"
#include "Tr2InteriorEnlightenSystem.h"

BLUE_DEFINE( Tr2InteriorCell );

// ------------------------------------------------------------------------------------------------------
#if BLUE_WITH_PYTHON
static PyObject* PyGetLightProbeResolution( PyObject* self, PyObject* args )
{
	Tr2InteriorCell* pThis = BluePythonCast<Tr2InteriorCell*>( self );
	int x = 2;
	int y = 2;
	int z = 2;
	pThis->GetLightProbeResolution( &x, &y, &z );
	return Py_BuildValue( "(iii)", x, y, z );
}
#endif


// ------------------------------------------------------------------------------------------------------
const Be::ClassInfo* Tr2InteriorCell::ExposeToBlue()
{
    EXPOSURE_BEGIN( Tr2InteriorCell, "" )
        MAP_INTERFACE( Tr2InteriorCell )
		MAP_INTERFACE( IInitialize )
		MAP_INTERFACE( INotify )
		MAP_INTERFACE( IListNotify )

		MAP_ATTRIBUTE( "name", m_name, "", Be::READWRITE | Be::PERSIST )

		MAP_ATTRIBUTE( "reflectionMapPath", m_reflectionMapPath, "The path for the reflection map for this cell", Be::READWRITE | Be::PERSIST | Be::NOTIFY )
		MAP_ATTRIBUTE( "reflectionMap", m_reflectionMapRes, "The reflection map for this cell", Be::READ )

		MAP_ATTRIBUTE( "minBounds", m_minBounds, "DEPRECATED: world (x,y,z) minimum bounds for the cell", Be::READ )
		MAP_ATTRIBUTE( "maxBounds", m_maxBounds, "DEPRECATED: world (x,y,z) maximum bounds for the cell", Be::READ )
		MAP_ATTRIBUTE( "boundingBoxes", m_boundingBoxes, "A union of bounding boxes used to determine dynamic object cell membership", Be::READWRITE | Be::PERSIST )
		MAP_METHOD_AND_WRAP( "ContainsPoint", ContainsPoint, "Does this point (in worldspace) exist in the cell?" )
		MAP_METHOD_AND_WRAP( "IntersectsAABB", IntersectsAABB, "Does Axis-aligned bounding box intersect with the cell" )
		MAP_METHOD_AND_WRAP( "IntersectsOBB", IntersectsOBB, "Does this Oriented bounding box intersect with the cell" )
		MAP_METHOD_AND_WRAP( "IntersectsSphere", IntersectsSphere, "Does this sphere intersect with the cell" )

		MAP_ATTRIBUTE( "minBoxGutter", m_minBoxGutter, "Gutter added to minimum bounds of the cell box", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "maxBoxGutter", m_maxBoxGutter, "Gutter added to maximum bounds of the cell box", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "isUnbounded", m_isUnbounded, "Set to true if the cell has no static geometry, but should still be able to contain dynamics and lights", Be::READWRITE | Be::PERSIST | Be::NOTIFY )
		MAP_ATTRIBUTE( "drawBoundingBox", m_drawBoundingBox, "renderDebugInfo must be on!", Be::READWRITE )

		MAP_ATTRIBUTE( "position", m_position, "Vector specifying the position of the cell", Be::READWRITE | Be::PERSIST | Be::NOTIFY )
		MAP_ATTRIBUTE( "rotation", m_rotation, "Quaternion specifying the rotation of the cell", Be::READWRITE | Be::PERSIST | Be::NOTIFY )
		MAP_ATTRIBUTE( "worldTransform", m_worldTransform, "Matrix specifying the position and rotation of the cell in world coordinates", Be::READ )

		MAP_ATTRIBUTE( "occluders", m_occluders, "a list of non-geometry occluders associated with this cell", Be::READWRITE | Be::PERSIST )

		MAP_ATTRIBUTE( "systems", m_systems, "a list of systems", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "probeVolumes", m_probeVolumes, "a list of spherical harmonic probe volumes", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "shProbeResPath", m_shProbeResPath, "a resource path for cached spherical harmonic probe solutions", Be::READWRITE | Be::PERSIST| Be::NOTIFY )
		MAP_ATTRIBUTE( "shProbeResource", m_shProbeResource, "a resource for cached spherical harmonic probe solutions", Be::READ )

		MAP_PROPERTY( "probeVolumeResX", GetDefaultProbeVolumeResolutionX, SetDefaultProbeVolumeResolutionX, "Default probe volume resolution in X direction" )
		MAP_PROPERTY( "probeVolumeResY", GetDefaultProbeVolumeResolutionY, SetDefaultProbeVolumeResolutionY, "Default probe volume resolution in Y direction" )
		MAP_PROPERTY( "probeVolumeResZ", GetDefaultProbeVolumeResolutionZ, SetDefaultProbeVolumeResolutionZ, "Default probe volume resolution in Z direction" )
		MAP_ATTRIBUTE( "drawLightProbes", m_defaultProbeVolume.m_bDrawLightProbes, "draw light probe debug information (renderDebugInfo must be on)", Be::READWRITE )
		MAP_ATTRIBUTE( "resX", m_defaultProbeVolume.m_lightProbeResX, "", Be::PERSISTONLY )
		MAP_ATTRIBUTE( "resY", m_defaultProbeVolume.m_lightProbeResY, "", Be::PERSISTONLY )
		MAP_ATTRIBUTE( "resZ", m_defaultProbeVolume.m_lightProbeResZ, "", Be::PERSISTONLY )

		MAP_ATTRIBUTE( "drawSphereProbes", m_defaultProbeVolume.m_sphereProbes, "Draw colored spheres in probe positions", Be::READWRITE )
		MAP_ATTRIBUTE( "drawProbeCulling", m_defaultProbeVolume.m_drawProbeCulling, "Draw probe cull status (only when drawSphereProbes is on)", Be::READWRITE )
		MAP_ATTRIBUTE( "hasValidProbes", m_defaultProbeVolume.m_validProbes, "True if the volume contains valid (built) probe data", Be::READ )

		MAP_ATTRIBUTE( "drawSocketProbes", m_drawSocketProbes, "Draw colored spheres in portal socket probe positions", Be::READWRITE )
		MAP_ATTRIBUTE( "drawSocketCulling", m_drawSocketProbeCulling, "Draw portal socket probe cull status (only when drawSocketProbes is on)", Be::READWRITE )
		MAP_ATTRIBUTE( "portalSockets", m_portalSockets, "a list of portal sockets", Be::READWRITE | Be::PERSIST | Be::NOTIFY )
		MAP_ATTRIBUTE( "socketSystemID", m_socketSystem.m_systemID, "A unique ID for portal sockets system", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "radSocketSystemPath", m_socketSystem.m_radResPath, "A res path to a cached radiosity socket system for Enlighten (including dusters)", Be::READWRITE | Be::PERSIST| Be::NOTIFY )
		MAP_ATTRIBUTE( "socketRadiosityResource", m_socketSystem.m_radSystemResource, "The resource for the Enlighten Radiosity socket system", Be::READ )

		MAP_ATTRIBUTE( "variableStore", m_variableStore, "Local variable store for this cell", Be::READ )

		MAP_METHOD_AND_WRAP( "RebuildInternalData", RebuildInternalData, "Notify this cell that its content has changed, so it can rebuild itself" )

        MAP_METHOD_AND_WRAP( 
			"AddSystem", 
			AddSystem, 
			"Add an interior Enlighten system to this cell"
			"\n"
			"\nArguments:"
			"\nsystem - The Tr2InteriorEnlightenSystem to add" )

		MAP_METHOD_AND_WRAP( 
			"RemoveSystem", 
			RemoveSystem, 
			"Remove an interior Enlighten system from this cell"
			"\n"
			"\nArguments:"
			"\nsystem - The Tr2InteriorEnlightenSystem to remove" )

		MAP_METHOD_AND_WRAP( "ClearSystems", ClearSystems, "Remove ALL interior static objects from this cell" )

#if defined(ENLIGHTEN_PRECOMPUTE_ENABLED)
		MAP_METHOD_AND_WRAP( "BuildEnlightenSystems", BuildEnlightenSystems, "Build all the systems present in systems list" )
		MAP_METHOD_AND_WRAP( "PreivewEnlightenSystems", PreviewEnlightenSystems, "Build all the systems present in systems list in preview mode" )

		MAP_METHOD_AND_WRAP( 
			"BuildLightVolume", 
			BuildLightVolume, 
			"Builds a light probe sample volume with the specified sampling"
			"\n"
			"\nArguments:"
			"\nxRes - The number of probes that you want in the x Axis"
			"\nyRes - The number of probes that you want in the y Axis"
			"\nzRes -The number of probes that you want in the z Axis" )

		MAP_METHOD_AND_WRAP( "SaveEnlightenSystems", 
			SaveEnlightenSystems, 
			"Save all the systems present in sub-cells to their resource locations." )

		MAP_METHOD_AND_WRAP( "SaveLightProbes", 
			SaveLightProbes, 
			"Save all light probe volume data." )
#endif
		MAP_METHOD_AND_WRAP( "GetLightProbes", GetLightProbeList, "Gets a list of light probe positions" )

		MAP_METHOD_AND_WRAP( "PopulateProbeVolumes", 
			PopulateProbeVolumes, 
			"Populate probe volumes vector with data from SH file." )
		MAP_METHOD( "GetLightProbeResolution", 
			PyGetLightProbeResolution, 
			"Get the current light probe resolution in x, y and z" )
		MAP_METHOD_AND_WRAP( 
			"IsVolumeReady", 
			IsVolumeReady, 
			"Has the volume for the light probes been set up." )
	EXPOSURE_END()
}

#endif
