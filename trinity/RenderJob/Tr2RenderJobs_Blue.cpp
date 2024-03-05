#include "StdAfx.h"

#include "Tr2RenderJobs.h"

BLUE_DEFINE( Tr2RenderJobs );

const Be::ClassInfo* Tr2RenderJobs::ExposeToBlue()
{
	EXPOSURE_BEGIN(Tr2RenderJobs, "" )

		MAP_INTERFACE( IRoot )
		MAP_INTERFACE( Tr2RenderJobs )

		MAP_ATTRIBUTE
		(
			"recurring",
			m_scheduledRecurring,
			"Recurring render jobs are run each frame",
			Be::READ | Be::PERSIST
		)

		MAP_ATTRIBUTE
		( 
			"once",
			m_scheduledOnce,
			"Render jobs scheduled to run once - every job on this list is given a chance to run."
			"\nJobs may extend over multiple frames.",
			Be::READ | Be::PERSIST
		)

		MAP_ATTRIBUTE
		( 
			"chained",
			m_scheduledChained,
			"Render jobs scheduled to run once - jobs are run until the first one returns in progress."
			"\nJobs may extend over multiple frames.",
			Be::READ | Be::PERSIST
		)

		MAP_ATTRIBUTE
		( 
			"updateRecurring",
			m_updateRecurring,
			"Update jobs are special jobs that are not supposed to do any rendering, but rather do CPU-intensive"
			"\ncomputations. They run every frame and are scheduled right before device present to hide stalls"
			"\nfor GPU-bount cases.",
			Be::READ | Be::PERSIST
		)

	EXPOSURE_END()
}

