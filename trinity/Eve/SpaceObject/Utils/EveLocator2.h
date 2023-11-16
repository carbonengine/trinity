#pragma once

#ifndef EVELOCATOR2_H
#define EVELOCATOR2_H



BLUE_CLASS( EveLocator2 ) : public IRoot
{
public:
	EXPOSE_TO_BLUE();

	EveLocator2( IRoot* lockobj = NULL );

	const char* GetName() const { return m_name.c_str(); }
	void SetName( const char* name ) { m_name = BlueSharedString( name ); }
	void SetName( const BlueSharedString& name ) { m_name = name; }

	const Matrix& GetTransform() const { return m_transform; }
	void SetTransform( const Matrix& val) { m_transform = val; }

protected:
	BlueSharedString m_name;
	Matrix m_transform;
};

BLUE_DECLARE_VECTOR( EveLocator2 );
TYPEDEF_BLUECLASS( EveLocator2 );

#endif
