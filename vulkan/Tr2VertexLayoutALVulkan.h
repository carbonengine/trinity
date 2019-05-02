////////////////////////////////////////////////////////////
//
//    Created:   February 2019
//    Copyright: CCP 2019
//

#pragma once

#if TRINITY_PLATFORM == TRINITY_VULKAN
// -------------------------------------------------------------
// Description
//   Class to convert a platform agnostic Tr2VertexDeclaration to a DX11 specific
//   declaration, and build a ID3D11InputLayout out of it.
//   Not intended to be used directly, instead use Tr2VertexDefinition, apply
//   the definition to the Tr2EffectStateManager, and let things happen behind the scenes.
// -------------------------------------------------------------

#include "../ALResult.h"
#include "../Tr2TrackedALObject.h"


class Tr2RenderContextAL;
class Tr2VertexDefinition;
class Tr2ShaderAL;


class Tr2VertexLayoutAL : public Tr2TrackedALObject<Tr2RenderContextEnum::OT_VERTEX_LAYOUT>
{
public:
	Tr2VertexLayoutAL()
	{

	}
	ALResult Create( const Tr2VertexDefinition& definition,
		Tr2RenderContextAL& renderContext )
	{
		return E_NOTIMPL;
	}
	bool IsValid() const
	{
		return false;
	}
	void Destroy()
	{

	}

	ALResult SetLayout( const Tr2ShaderAL* vertexShader, Tr2RenderContextAL& renderContext ) const
	{
		return E_NOTIMPL;
	}

	Tr2ALMemoryType GetMemoryClass() const
	{
		return AL_MEMORY_MANAGED;
	}

private:
	Tr2VertexLayoutAL( const Tr2VertexLayoutAL& )/* = delete */;
	Tr2VertexLayoutAL& operator=( const Tr2VertexLayoutAL& )/* = delete */;
};

#endif
