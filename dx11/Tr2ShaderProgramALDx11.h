#pragma once

#include "../ALResult.h"
#include "../Tr2TrackedALObject.h"
#include "../include/Tr2ShaderAL.h"

#if( TRINITY_PLATFORM==TRINITY_DIRECTX11 )

class Tr2ShaderProgramAL : public Tr2TrackedALObject<Tr2RenderContextEnum::OT_SHADER_PROGRAM>
{
public:
	Tr2ShaderProgramAL();
	
	ALResult Create( Tr2ShaderAL* shaders, size_t count, Tr2PrimaryRenderContextAL& renderContext );
	void Destroy();

	bool IsValid() const;

	Tr2ALMemoryType GetMemoryClass() const;

private:
	struct Shaders
	{
		CComPtr<ID3D11VertexShader> vertexShader;
		CComPtr<ID3D11PixelShader> pixelShader;
		CComPtr<ID3D11ComputeShader> computeShader;
		CComPtr<ID3D11GeometryShader> geometryShader;
		CComPtr<ID3D11HullShader> hullShader;
		CComPtr<ID3D11DomainShader> domainShader;

		CComPtr<ID3D11PixelShader> patchedPixelShader;
	};

	Tr2ShaderAL m_vertexShader;
	Shaders m_shaders;

	//Tr2ShaderAL m_shaders[Tr2RenderContextEnum::SHADER_TYPE_COUNT];
	bool m_isValid;

	friend class Tr2RenderContextAL;
};

#endif