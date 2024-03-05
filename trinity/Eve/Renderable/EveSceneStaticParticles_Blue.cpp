////////////////////////////////////////////////////////////
//
//    Created:   November 2014
//    Copyright: CCP 2014
//
#include "StdAfx.h"
#include "EveSceneStaticParticles.h"

BLUE_DEFINE( EveSceneStaticParticles );

const Be::ClassInfo* EveSceneStaticParticles::ExposeToBlue()
{
    EXPOSURE_BEGIN( EveSceneStaticParticles, "" )
        MAP_INTERFACE( EveSceneStaticParticles )
		MAP_INTERFACE( IInitialize )

		MAP_ATTRIBUTE( "maxParticleCount", m_maxParticleCount, "The total particle count this module can handle", Be::READWRITE )
		MAP_ATTRIBUTE( "minSize", m_minSize, "Minimum size of particles", Be::READWRITE )
		MAP_ATTRIBUTE( "maxSize", m_maxSize, "Maximum size of particles", Be::READWRITE )
		MAP_ATTRIBUTE( "clusterParticleDensity", m_clusterParticleDensity, "How many particles per radius", Be::READWRITE )
		MAP_ATTRIBUTE( "clusterParticleDensityAdjust", m_clusterParticleDensityAdjust, "DEBUG: how much do we have to reduce", Be::READ )

		MAP_ATTRIBUTE( "transform", m_transform, "The particle system", Be::READWRITE )

		MAP_METHOD_AND_WRAP(
			"AddCluster",
			AddCluster,
			"Add a whole cluster of particles\n"
			"\n:param position: The center position of the cluster in global 3d space (double precision)"
			"\n:param radius: The radius of the cluster"
			"\n:param color1: First color for the cluster to interpolate from"
			"\n:param color2: Second color for the cluster to interpolate from"
			"\n:param randomSeed: seeding integer for the randomness of this cluster (default is 0)" )

		MAP_METHOD_AND_WRAP( "ClearClusters", ClearClusters, "Remove all clusters" )
		MAP_METHOD_AND_WRAP( "Rebuild", Rebuild, "Once finished adding clusters, we need to build internal buffers etc." )

    EXPOSURE_END()
}
