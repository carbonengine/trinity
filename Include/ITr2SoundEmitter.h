////////////////////////////////////////////////////////////
//
//    Created:   October 2018
//    Copyright: CCP 2018
//

#pragma once


BLUE_INTERFACE( ITr2SoundEmitter ): public IRoot
{
	virtual void Initialize( const char* name, const wchar_t* prefix ) = 0;
	virtual void SendSoundEvent( const wchar_t* eventName ) = 0;
	virtual void SetSwitch( const wchar_t* switchGroup, const wchar_t* switchState ) = 0;
};