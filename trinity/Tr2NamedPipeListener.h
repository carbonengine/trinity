#pragma once
#ifndef Tr2NamedPipeListener_h
#define Tr2NamedPipeListener_h


BLUE_CLASS( Tr2NamedPipeListener ):
     public IRoot
{
public:
    EXPOSE_TO_BLUE();
    Tr2NamedPipeListener( IRoot* lockobj = NULL );
	~Tr2NamedPipeListener();

	bool Listen( std::string pipeName );
	void Clear( );	

	void SetCallback( const BlueScriptCallback& callback );

	std::string		m_pipeName;
	HANDLE			m_pipeHandle;	
	BYTE*			m_buffer;
	unsigned int	m_size;
	LONG			m_waiting;

	BlueScriptCallback m_callback;

};

TYPEDEF_BLUECLASS( Tr2NamedPipeListener );
#endif //Tr2NamedPipeListener_h
