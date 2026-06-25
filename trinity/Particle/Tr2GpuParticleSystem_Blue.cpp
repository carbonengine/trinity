// Copyright © 2015 CCP ehf.

#include "StdAfx.h"
#include "Tr2GpuParticleSystem.h"


BLUE_DEFINE( Tr2GpuParticleSystem );

const Be::ClassInfo* Tr2GpuParticleSystem::ExposeToBlue()
{
	EXPOSURE_BEGIN( Tr2GpuParticleSystem, "GPU-managed particle system" )
		MAP_INTERFACE( Tr2GpuParticleSystem )
		MAP_INTERFACE( IInitialize )
		MAP_INTERFACE( INotify )

		MAP_ATTRIBUTE(
			"emit",
			m_emit,
			"Shader for emitting particles",
			Be::READWRITE | Be::PERSIST | Be::NOTIFY )
		MAP_ATTRIBUTE(
			"update",
			m_update,
			"Shader for updating particles",
			Be::READWRITE | Be::PERSIST | Be::NOTIFY )
		MAP_ATTRIBUTE(
			"render",
			m_render,
			"Shader for rendering particles",
			Be::READWRITE | Be::PERSIST | Be::NOTIFY )
		MAP_ATTRIBUTE(
			"clear",
			m_clear,
			"Shader for clearing particle system",
			Be::READWRITE | Be::PERSIST | Be::NOTIFY )
		MAP_ATTRIBUTE(
			"setDrawParameters",
			m_setDrawParameters,
			"Shader to assign draw call parameters",
			Be::READWRITE | Be::PERSIST | Be::NOTIFY )
		MAP_ATTRIBUTE(
			"setSortParameters",
			m_setSortParameters,
			"Shader to assign sort dispatch parameters",
			Be::READWRITE | Be::PERSIST | Be::NOTIFY )
		MAP_ATTRIBUTE(
			"sort",
			m_sort,
			"Shader for initial sorting of particles",
			Be::READWRITE | Be::PERSIST | Be::NOTIFY )
		MAP_ATTRIBUTE(
			"sortStep",
			m_sortStep,
			"Shader for incremental sorting of particles",
			Be::READWRITE | Be::PERSIST | Be::NOTIFY )
		MAP_ATTRIBUTE(
			"sortInner",
			m_sortInner,
			"Shader for incremental sorting of particles (merge phase)",
			Be::READWRITE | Be::PERSIST | Be::NOTIFY )

		MAP_ATTRIBUTE(
			"enableEmit",
			m_enableEmit,
			"Enable/disable emitting new particles (for debugging)",
			Be::READWRITE )
		MAP_ATTRIBUTE(
			"enableUpdate",
			m_enableUpdate,
			"Enable/disable updating existing particles (for debugging)",
			Be::READWRITE )
		MAP_ATTRIBUTE(
			"enableSort",
			m_enableSort,
			"Enable/disable sorting of particles (for debugging)",
			Be::READWRITE )
		MAP_ATTRIBUTE(
			"display",
			m_enableRender,
			"Enable/disable rendering of particles",
			Be::READWRITE )

		MAP_ATTRIBUTE(
			"maxParticles",
			m_maxParticles,
			"Maximum number of particles in the system",
			Be::READWRITE | Be::PERSIST | Be::NOTIFY )

		MAP_PROPERTY_READONLY(
			"emitTime",
			GetEmitTime,
			"GPU emit particles time in milliseconds" )
		MAP_PROPERTY_READONLY(
			"updateTime",
			GetUpdateTime,
			"GPU particle update time in milliseconds" )
		MAP_PROPERTY_READONLY(
			"sortTime",
			GetSortTime,
			"GPU particle sort time in milliseconds" )
		MAP_PROPERTY_READONLY(
			"renderTime",
			GetRenderTime,
			"GPU render time in milliseconds" )

		MAP_ATTRIBUTE(
			"updateVisibleCount",
			m_updateVisibleCount,
			"Enable/disable updating visibleCount attribute (for debug only)",
			Be::READWRITE )
		MAP_ATTRIBUTE(
			"visibleCount",
			m_visibleCount,
			"Number of visible particles (when updateVisibleCount is on; for debug only)",
			Be::READ )

		MAP_PROPERTY_READONLY(
			"hasParticles",
			HasParticles,
			"If the system contains no alive particles; acts as an estimate and can return false positives" )

		MAP_METHOD_AND_WRAP(
			"Clear",
			Clear,
			"Removes all alive particles from the system" )

	EXPOSURE_END()
}
