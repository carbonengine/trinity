// Copyright © 2023 CCP ehf.

#pragma once

struct Matrix;

BLUE_INTERFACE( ITr2AudioEmitter ) :
	public IRoot
{
	virtual void SendEvent() = 0;
	virtual void SetTransform( const Matrix& worldTransform ) = 0;
};
