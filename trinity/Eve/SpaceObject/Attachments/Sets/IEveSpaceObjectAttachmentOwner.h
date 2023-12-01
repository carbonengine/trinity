////////////////////////////////////////////////////////////
//
//    Created:   August 2022
//    Copyright: CCP 2022
//
#pragma once

BLUE_DECLARE_INTERFACE( IEveSpaceObjectAttachment );

BLUE_INTERFACE( IEveSpaceObjectAttachmentOwner ) :
	public IRoot
{
	virtual void AddAttachment( IEveSpaceObjectAttachment* attachment ) = 0;
	virtual void ClearAttachments() = 0;
};
