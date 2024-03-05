#pragma once

BLUE_INTERFACE( IShaderConfigurer ) : public IRoot
{
	virtual void SetShaderOption( const BlueSharedString& name, const BlueSharedString& value ) = 0;
};

