#pragma once
#ifndef Tr2InteriorProbeVisualizer_H
#define Tr2InteriorProbeVisualizer_H

#include "IRenderCallback.h"

BLUE_DECLARE( Tr2Effect );

BLUE_CLASS( Tr2InteriorProbeVisualizer ) :
	public IRoot,
    public IRenderCallback
{
public:
	Tr2InteriorProbeVisualizer( IRoot* lockobj = 0 );
	~Tr2InteriorProbeVisualizer();

	EXPOSE_TO_BLUE();

    using IRoot::Lock;
    using IRoot::Unlock;

	void Render( const Vector3& position, const Matrix& redMat, const Matrix& greenMat, const Matrix& blueMat, float brightness );

protected:
	void SubmitGeometry( Tr2RenderContext& renderContext );
private:
	struct Vertex;
	struct PerObjectVS;
	struct PerObjectPS;

    unsigned int m_vertexDeclaration;
    unsigned int m_bytesPerVertex;
    unsigned int m_vertexCount;
    Tr2VertexBufferAL m_vertexBuffer;

    Tr2EffectPtr m_effect;

	Tr2ConstantBufferAL m_VSBuffer, m_PSBuffer;
};

TYPEDEF_BLUECLASS( Tr2InteriorProbeVisualizer );
BLUE_DECLARE_VECTOR( Tr2InteriorProbeVisualizer );

#endif // Tr2InteriorProbeVisualizer_H
