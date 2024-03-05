#pragma once

#ifndef Tr2ShaderBuffer_H
#define Tr2ShaderBuffer_H

BLUE_DECLARE( Tr2ShaderBuffer );
BLUE_DECLARE( Tr2RenderContext );

BLUE_CLASS( Tr2ShaderBuffer ): 
	public IRoot
{
public:
	EXPOSE_TO_BLUE();
	
    Tr2ShaderBuffer( IRoot* lockobj = NULL );
	~Tr2ShaderBuffer();

	void ApplyBuffer( Tr2RenderContext& renderContext );
	void SetData( void* data, int size );
	void SetDataFromScript( const char* data, int size );
	void SetShaderType( Tr2RenderContextEnum::ShaderType shaderType ) { m_shaderType = shaderType; }

private:
	void* m_data;
	int m_size;

	Tr2RenderContextEnum::ShaderType m_shaderType;
};

TYPEDEF_BLUECLASS( Tr2ShaderBuffer );

#endif