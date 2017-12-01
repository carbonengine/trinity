#pragma once

#if TRINITY_PLATFORM == TRINITY_STUB

#include "../Tr2TrackedALObject.h"
#include "../ALResult.h"

class Tr2ResourceSetDescriptionAL;
class Tr2PrimaryRenderContextAL;

namespace TrinityALImpl
{
	class Tr2ResourceSetAL : public Tr2TrackedALObject<Tr2RenderContextEnum::OT_RESOURCE_SET>
	{
	public:
		Tr2ResourceSetAL();

		ALResult Create( const Tr2ResourceSetDescriptionAL& description, Tr2PrimaryRenderContextAL& renderContext );
		bool IsValid() const;

		void Destroy();
		Tr2ALMemoryType GetMemoryClass() const;
	private:
		bool m_isValid;
	};
}

#endif