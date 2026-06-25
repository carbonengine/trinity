// Copyright © 2022 CCP ehf.

#pragma once

BLUE_DECLARE_INTERFACE( IEveSpaceObjectAttachment );

BLUE_INTERFACE( IEveSpaceObjectAttachmentOwner ) :
	public IRoot
{
	virtual void AddAttachment( IEveSpaceObjectAttachment * attachment ) = 0;
	virtual void ClearAttachments() = 0;
};
