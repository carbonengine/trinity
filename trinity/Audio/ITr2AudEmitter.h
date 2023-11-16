////////////////////////////////////////////////////////////
//
//    Created:   April 2020
//    Copyright: CCP 2020
//
//    Description:
//      An interface intended for use by audio2 to expose relevant functions
//      related to audio emitters.

#pragma once

#ifndef ITr2AudEmitter_h_
#define ITr2AudEmitter_h_

BLUE_INTERFACE( ITr2AudEmitter ) : public IRoot
{
	virtual void Initialize( const std::string& name, const std::wstring& prefix, const Vector3& position ) = 0;
	virtual int SetPosition( const Vector3& front, const Vector3& top, const Vector3& pos ) = 0;
	virtual void SetName( const std::string& name ) = 0;
	virtual void SetPrefix( const std::wstring& prefix ) = 0;
	virtual unsigned int SendEvent( const std::wstring& name, bool bypassPrefix = false ) = 0;
	virtual bool SetSwitch( const std::wstring& switchGroup, const std::wstring& switchState ) = 0;
	virtual bool SetRTPC( const std::wstring& rtpcName, float rtpcValue ) = 0;
	virtual bool SetAttenuationScalingFactor( const float scalingFactor ) = 0;
	virtual std::string GetName() = 0;
	virtual void SetVisibility( bool visible ) = 0;
};

#endif
