#pragma once

#include "../ALResult.h"
#include "../Tr2TrackedALObject.h"
#include "../include/Tr2ShaderAL.h"

#if TRINITY_PLATFORM==TRINITY_DIRECTX9

class Tr2ShaderProgramAL : public Tr2TrackedALObject<Tr2RenderContextEnum::OT_SHADER_PROGRAM>
{
public:
	Tr2ShaderProgramAL();

	ALResult Create( Tr2ShaderAL** shaders, size_t count, Tr2PrimaryRenderContextAL& renderContext );
	void Destroy();

	bool IsValid() const;

	Tr2ALMemoryType GetMemoryClass() const;

private:
	const Tr2ShaderAL* m_vertexShader;
	const Tr2ShaderAL* m_pixelShader;
	bool m_isValid;

	friend class Tr2RenderContextAL;
};

#endif