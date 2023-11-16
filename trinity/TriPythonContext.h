/* 
	*************************************************************************************

	TriPythonContext.h

	Created by:	Halldor Fannar
	Created on:	October 18th, 2006
	Project:	Trinity

	(c) CCP 2006

	*************************************************************************************
*/

#pragma once
#ifndef _TriPythonContext_H_
#define _TriPythonContext_H_

class TriPythonContext
{
public:
	TriPythonContext()
	{
		m_active++;
	}

	~TriPythonContext()
	{
		m_active--;
	}

	static bool IsActive()
	{
		return m_active > 0;
	}

private:
	// Iff 'm_error' > 0 then report errors as Python errors
	static int m_active;
};

#endif