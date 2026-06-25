// Copyright © 2025 CCP ehf.

#include "StdAfx.h"
#include "Tr2ActionPython.h"
#include "../Tr2Controller.h"
#include "../../ContinueOnMainThread.h"

namespace
{

BluePy CreateInstance( const char* moduleName, const char* className )
{
	BluePy actionName( PyList_New( 1 ) );
	PyList_SET_ITEM( actionName.o, 0, PyUnicode_FromString( className ) );

	BluePy module( PyImport_ImportModuleLevel( (char*)moduleName, nullptr, nullptr, actionName, 0 ) );
	if( !module )
	{
		CCP_LOGERR( "Failed to import python module %s for Tr2ActionPython", moduleName );
		return {};
	}

	BluePy classObj( PyObject_GetAttrString( module, className ) );
	if( !classObj || !PyCallable_Check( classObj ) )
	{
		CCP_LOGERR( "Failed to get %s class from module %s for Tr2ActionPython", className, moduleName );
		return {};
	}
	BluePy instance( PyObject_CallObject( classObj, nullptr ) );
	if( !instance )
	{
		CCP_LOGERR( "Failed to instantiate Action class from module %s for Tr2ActionPython", moduleName );
	}
	return instance;
}

}

Tr2ActionPython::Tr2ActionPython( IRoot* )
{
}

bool Tr2ActionPython::Initialize()
{
	if( !m_instance )
	{
		InstantiateObject();
	}
	return true;
}

bool Tr2ActionPython::OnModified( Be::Var* value )
{
	if( IsMatch( value, m_module ) || IsMatch( value, m_className ) )
	{
		auto controller = m_controller;
		auto isPlaying = m_isPlaying;
		if( m_controller )
		{
			if( m_isPlaying )
			{
				Stop( *m_controller );
			}
			Unlink();
		}
		InstantiateObject();
		if( controller )
		{
			Link( *controller );
			if( isPlaying )
			{
				Start( *controller );
			}
		}
	}
	return true;
}

void Tr2ActionPython::Link( ITr2ActionController& controller )
{
	m_controller = &controller;
	if( !m_vtable.onLink )
	{
		return;
	}
	m_vtable.onLink.CallVoid( controller.GetOwner(), &controller );
}

void Tr2ActionPython::Unlink()
{
	m_controller = nullptr;
	if( !m_vtable.onUnlink )
	{
		return;
	}
	m_vtable.onUnlink.CallVoid();
}

void Tr2ActionPython::Start( ITr2ActionController& controller )
{
	m_isPlaying = true;
	if( m_vtable.onUpdate )
	{
		controller.RegisterUpdateable( *this );
	}
	if( m_vtable.onStart )
	{
		ContinueOnMainThread( [self = Tr2ActionPythonPtr( this ), controllerPtr = ITr2ActionControllerPtr( &controller ), owner = IRootPtr( controller.GetOwner() )]() {
			self->m_vtable.onStart.CallVoid( owner, controllerPtr );
		} );
	}
	m_prevRealTime = BeOS->GetActualTime();
	m_prevSimTime = BeOS->GetCurrentFrameTime();
}

void Tr2ActionPython::Stop( ITr2ActionController& controller )
{
	m_isPlaying = false;
	controller.UnRegisterUpdateable( *this );
	if( !m_vtable.onStop )
	{
		return;
	}
	ContinueOnMainThread( [self = Tr2ActionPythonPtr( this ), controllerPtr = ITr2ActionControllerPtr( &controller ), owner = IRootPtr( controller.GetOwner() )]() {
		self->m_vtable.onStop.CallVoid( owner, controllerPtr );
	} );
}

void Tr2ActionPython::Update( Be::Time realTime, Be::Time simTime )
{
	ContinueOnMainThread( [self = Tr2ActionPythonPtr( this ),
						   controllerPtr = ITr2ActionControllerPtr( m_controller ),
						   owner = IRootPtr( m_controller->GetOwner() ),
						   realDt = TimeAsFloat( realTime - m_prevRealTime ),
						   simDt = TimeAsFloat( simTime - m_prevSimTime )]() {
		self->m_vtable.onUpdate.CallVoid( owner, controllerPtr, realDt, simDt );
	} );
	m_prevRealTime = realTime;
	m_prevSimTime = simTime;
}

BluePy Tr2ActionPython::GetInstance() const
{
	return m_instance;
}

void Tr2ActionPython::InstantiateObject()
{
	m_vtable = {};
	m_instance = {};

	if( m_module.empty() || m_className.empty() )
	{
		return;
	}

	auto gil = PyGILState_Ensure();
	ON_BLOCK_EXIT( [&gil] { PyGILState_Release( gil ); } );

	m_instance = CreateInstance( m_module.c_str(), m_className.c_str() );

	auto ExtractMethod = [&]( const char* methodName, BlueScriptCallback& callback ) {
		BluePy method( PyObject_GetAttrString( m_instance, methodName ) );
		if( method && PyCallable_Check( method ) )
		{
			BlueExtractArgument( method, callback, 0 );
		}
		else
		{
			PyErr_Clear();
		}
	};

	ExtractMethod( "OnLink", m_vtable.onLink );
	ExtractMethod( "OnUnlink", m_vtable.onUnlink );
	ExtractMethod( "OnStart", m_vtable.onStart );
	ExtractMethod( "OnStop", m_vtable.onStop );
	ExtractMethod( "OnUpdate", m_vtable.onUpdate );
	ExtractMethod( "OnLoad", m_vtable.onLoad );
	ExtractMethod( "OnSave", m_vtable.onSave );
}

void Tr2ActionPython::GetWriteBufferAndSize( const char* memberName, uint8_t** buffer, size_t* bufferSize )
{
	*buffer = nullptr;
	*bufferSize = 0;

	if( !m_vtable.onSave )
	{
		return;
	}
	PyObject* bytesObj = nullptr;
	if( !m_vtable.onSave.Call( bytesObj ) || !bytesObj )
	{
		return;
	}

	auto gil = PyGILState_Ensure();
	ON_BLOCK_EXIT( [&gil] { PyGILState_Release( gil ); } );

	char* str = nullptr;
	Py_ssize_t size = 0;
#if PY_MAJOR_VERSION == 2
	if( PyString_AsStringAndSize( bytesObj, &str, &size ) != 0 )
	{
		CCP_LOGERR( "Tr2ActionPython: OnSave did not return a string" );
		return;
	}
#else
	if( PyBytes_AsStringAndSize( bytesObj, &str, &size ) != 0 )
	{
		CCP_LOGERR( "Tr2ActionPython: OnSave did not return a bytes object" );
		return;
	}
#endif
	if( size <= 0 )
	{
		return;
	}
	uint8_t* buf = new uint8_t[size];
	memcpy( buf, str, size );
	*buffer = buf;
	*bufferSize = size;
}

void Tr2ActionPython::ReleaseWriteBuffer( uint8_t* buffer )
{
	delete[] buffer;
}

unsigned char* Tr2ActionPython::AllocateReadBuffer( const char* memberName, size_t bufferSize )
{
	return new uint8_t[bufferSize];
}

void Tr2ActionPython::SetBufferAndSize( const char* memberName, uint8_t* buffer, size_t bufferSize )
{
	if( !m_instance )
	{
		InstantiateObject();
	}
	if( !m_vtable.onLoad )
	{
		return;
	}

	auto gil = PyGILState_Ensure();
	ON_BLOCK_EXIT( [&gil] { PyGILState_Release( gil ); } );

	std::string state( (const char*)buffer, bufferSize );
	PyObject* bytes = PyVerCompat::ToPyBytes( state.data(), state.size() );
	m_vtable.onLoad.CallVoid( bytes ).ReportException();
	Py_XDECREF( bytes );
	delete[] buffer;
}
