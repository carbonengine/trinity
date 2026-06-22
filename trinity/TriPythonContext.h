// Copyright © 2006 CCP ehf.

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