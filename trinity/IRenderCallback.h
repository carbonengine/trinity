// Copyright © 2014 CCP ehf.

#pragma once
#ifndef IRenderCallback_h
#define IRenderCallback_h

class Tr2RenderContext;

class IRenderCallback
{
public:
	virtual void SubmitGeometry( Tr2RenderContext& renderContext ) = 0;
};



#endif // IRenderCallback_h
