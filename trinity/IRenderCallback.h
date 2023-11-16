////////////////////////////////////////////////////////////////////////////////
//
// Created:		March 2014
// Copyright:	CCP 2014
//

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
