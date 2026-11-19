// Copyright © 2023 CCP ehf.

#pragma once
#ifndef TriValueBinding_h
#define TriValueBinding_h


#include "include/ITr2ValueBinding.h"
#include "Curves/TriCurveSampler.h"

BLUE_DECLARE( TriValueBinding );
BLUE_DECLARE_VECTOR( TriValueBinding );

BLUE_CLASS( TriValueBinding ) :
	public INotify,
	public ITr2ValueBinding
{
public:
	EXPOSE_TO_BLUE();

	TriValueBinding( IRoot* lockobj = NULL );
	~TriValueBinding();

	virtual void Initialize();

	void RerouteDestination( void* dest );

	//////////////////////////////////////////////////////////////////////////
	// INotify
	virtual bool OnModified( Be::Var * val );

	//////////////////////////////////////////////////////////////////////////
	// ITr2ValueBinding
	virtual void CopyValue();
	virtual void CopyValueAt( const TriCurveSetTime& time );

	static const Be::VarEntry* FindEntry( const char* name, const Be::ClassInfo* type, ssize_t& offs );

	const std::string& GetDestinationAttributeName() const
	{
		return m_destinationAttribute;
	}

	void SetSource( const std::string& sourceAttribute, IRootPtr sourceObject );
	void SetDestination( const std::string& destAttribute, IRootPtr destination );
	void SetScale( float scale );

	void CreateWeakBinding( IRoot * source, const char* sourceAttr, IRoot* dest, const char* destAttr, float scale = 1.0f, const Vector4& offset = Vector4( 0.0f, 0.0f, 0.0f, 0.0f ) );

	bool IsValid() const;

protected:
	bool CanCopy() const;
	void CopyFrom( void* source );

	IRoot* GetCurrentSourceObject() const;
	IRoot* GetCurrentDestinationObject() const;

	IRootPtr GetSourceObject() const;
	void SetSourceObject( IRoot * sourceObject );

	IRootPtr GetDestinationObject() const;
	void SetDestinationObject( IRoot * destinationObject );

	std::string m_name;
	IRootPtr m_sourceObject;
	BlueWeakRef<IRoot> m_sourceObjectWeak;
	IRootPtr m_destinationObject;
	BlueWeakRef<IRoot> m_destinationObjectWeak;

	std::string m_sourceAttribute;
	std::string m_destinationAttribute;

	void* m_source;
	void* m_destination;

	// Bindings can put values in individuial components of vectors
	// with destination attribute as something like 'value.x'. We
	// need to store this offset to properly handle rerouting destination
	// after initial setup.
	uint8_t m_sourceItemOffset;
	uint8_t m_destItemOffset;
	bool m_isWeak;
	bool m_isEnabled;

	float m_scale;
	Vector4 m_offset;

	// Shifts the time this binding samples its source curve at, without
	// touching the curve's own keys or time offset, so that several bindings
	// can drive different targets from the same curve at different times.
	float m_timeOffset;

	// Set when the source is a curve bound through its currentValue, so an
	// offset binding can evaluate it at its own time without moving the curve.
	// m_sampleOffset is where m_source sits inside that value.
	TriCurveSampler m_sampler;
	uint8_t m_sampleOffset;

	INotify* m_notifyPtr;

	typedef bool ( *CopyFunc )( void* srcVar, void* dstVar, float scale, const Vector4& offset );
	CopyFunc m_copyFunc;

	BlueScriptCallback m_copyValueCallable;

	virtual size_t DetermineCopyFunc( const Be::VarEntry* srcEntry, const Be::VarEntry* dstEntry, size_t dataSize, bool sourceFloatArrayAsFloat, bool destFloatArrayAsFloat );
};

TYPEDEF_BLUECLASS( TriValueBinding );
#endif //TriValueBinding_h
