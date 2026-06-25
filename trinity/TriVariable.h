// Copyright © 2023 CCP ehf.

#pragma once
#ifndef TRIVARIABLE_H
#define TRIVARIABLE_H

#include "ITr2EffectValue.h"

BLUE_DECLARE_INTERFACE( ITr2TextureProvider );
BLUE_DECLARE_INTERFACE( ITr2GpuBuffer );

enum TriVariableContentType
{
	TRIVARIABLE_INVALID,
	TRIVARIABLE_UNKNOWN_FLOAT,
	TRIVARIABLE_TEXTURE_RES,
	TRIVARIABLE_INT,
	TRIVARIABLE_FLOAT,
	TRIVARIABLE_FLOAT2,
	TRIVARIABLE_FLOAT3,
	TRIVARIABLE_FLOAT4,
	TRIVARIABLE_FLOAT4X4,
	TRIVARIABLE_COLOR,
	TRIVARIABLE_GPUBUFFER,
	TRIVARIABLE_COUNT,
};

inline TriVariableContentType GetVariableType( const ITr2TextureProvider* const )
{
	return TRIVARIABLE_TEXTURE_RES;
}

inline TriVariableContentType GetVariableType( const ITr2GpuBuffer* const )
{
	return TRIVARIABLE_GPUBUFFER;
}

inline TriVariableContentType GetVariableType( const int& )
{
	return TRIVARIABLE_INT;
}

inline TriVariableContentType GetVariableType( const float& )
{
	return TRIVARIABLE_FLOAT;
}

inline TriVariableContentType GetVariableType( const Matrix& )
{
	return TRIVARIABLE_FLOAT4X4;
}

inline TriVariableContentType GetVariableType( const Vector2& )
{
	return TRIVARIABLE_FLOAT2;
}

inline TriVariableContentType GetVariableType( const Vector3& )
{
	return TRIVARIABLE_FLOAT3;
}

inline TriVariableContentType GetVariableType( const Vector4& )
{
	return TRIVARIABLE_FLOAT4;
}

inline TriVariableContentType GetVariableType( const Color& )
{
	return TRIVARIABLE_COLOR;
}

BLUE_CLASS( TriVariable ) :
	public ITr2EffectValue
{
public:
	// This class is not actually exposed
	EXPOSE_TO_BLUE();

	static const char* GetTypeName( TriVariableContentType t )
	{
		static const char* typeNames[TRIVARIABLE_COUNT] = {
			"INVALID TYPE!",
			// A float variable type of some sort. May be converted into any known float type
			"TRIVARIABLE_UNKNOWN_FLOAT",
			"TRIVARIABLE_TEXTURE_RES",
			"TRIVARIABLE_INT",
			"TRIVARIABLE_FLOAT",
			"TRIVARIABLE_FLOAT2",
			"TRIVARIABLE_FLOAT3",
			"TRIVARIABLE_FLOAT4",
			"TRIVARIABLE_FLOAT4X4",
			"TRIVARIABLE_COLOR",
			"TRIVARIABLE_GPUBUFFER"
		};

		return typeNames[t];
	}

	static size_t GetTypeSize( TriVariableContentType t )
	{
		static size_t typeSizes[TRIVARIABLE_COUNT] = {
			sizeof( float ) * 16, // INVALID may be converted to another type, must register as largest type
			sizeof( float ) * 16, // UNKNOWN_FLOAT may be converted to another type, must register as largest type
			sizeof( ITr2TextureProvider* ),
			sizeof( int ),
			sizeof( float ),
			sizeof( float ) * 2,
			sizeof( float ) * 3,
			sizeof( float ) * 4,
			sizeof( float ) * 16,
			sizeof( Color ),
			sizeof( ITr2GpuBuffer* ),
		};

		return typeSizes[t];
	}

	const std::string& GetName() const
	{
		return m_name;
	}

private:
	template <typename T>
	void GetValue_( T & value ) const
	{
		CCP_ASSERT( m_type == GetVariableType( value ) );
		value = *(T*)m_value;
	}

	void GetValueTextureRes( ITr2TextureProvider * &value ) const
	{
		CCP_ASSERT( m_type == GetVariableType( value ) );
		value = m_texture;
	}

	void GetValueGpuBuffer( ITr2GpuBuffer * &value ) const
	{
		CCP_ASSERT( m_type == GetVariableType( value ) );
		value = m_gpuBuffer;
	}

	template <typename T>
	void SetValue_( const T& value )
	{
		CCP_ASSERT( m_type == GetVariableType( value ) );
		*(T*)m_value = value;
	}

	// Storing a texture res pointer
	void SetValueTextureRes( ITr2TextureProvider * &value )
	{
		CCP_ASSERT( m_type == GetVariableType( value ) );
		m_texture = value;
		m_type = TRIVARIABLE_TEXTURE_RES;
	}

	void SetValueGpuBuffer( ITr2GpuBuffer * &value )
	{
		CCP_ASSERT( m_type == GetVariableType( value ) );
		m_gpuBuffer = value;
		m_type = TRIVARIABLE_GPUBUFFER;
	}

	// Variables contain their payload starting at 'value'[0].  No size is
	// stored since 'type' is enough.
	TriVariableContentType m_type;
	std::string m_name;

	ITr2TextureProviderPtr m_texture;
	ITr2GpuBufferPtr m_gpuBuffer;

	// This variable must be at the end of the class, due to malloc magic in
	// Tr2VariableStore::RegisterVariableType
	uint8_t m_value[sizeof( Matrix )];

protected:
	// We intentionally disallow construction - only Tr2VariableStore can
	// create instances!
	TriVariable() :
		m_type( TRIVARIABLE_INVALID )
	{
		m_value[0] = 0;
	}

public:
	TriVariableContentType GetType() const
	{
		return m_type;
	}

	const char* GetTypeName() const
	{
		return GetTypeName( m_type );
	}

	size_t GetTypeSize() const
	{
		return GetTypeSize( m_type );
	}

	void GetValue( float& value ) const
	{
		GetValue_( value );
	}
	void GetValue( int& value ) const
	{
		GetValue_( value );
	}
	void GetValue( Vector2 & value ) const
	{
		GetValue_( value );
	}
	void GetValue( Vector3 & value ) const
	{
		GetValue_( value );
	}
	void GetValue( Vector4 & value ) const
	{
		GetValue_( value );
	}
	void GetValue( Color & value ) const
	{
		GetValue_( value );
	}
	void GetValue( Matrix & value ) const
	{
		GetValue_( value );
	}
	void GetValue( ITr2TextureProvider * &value ) const
	{
		GetValueTextureRes( value );
	}
	void GetValue( ITr2GpuBuffer * &value ) const
	{
		GetValueGpuBuffer( value );
	}

	/////////////////////////////////////////////
	// ITr2EffectValue methods

	size_t GetValueSize() const
	{
		return GetTypeSize();
	}

	virtual bool CopyToResourceSet(
		Tr2ResourceSetDescriptionAL & resourceDesc,
		Tr2RenderContextEnum::ShaderType stage,
		uint32_t registerIndex,
		ResourceFlags flags ) const;
	virtual bool ApplyUav(
		Tr2ResourceSetDescriptionAL & resourceDesc,
		Tr2RenderContextEnum::ShaderType stage,
		uint32_t registerIndex ) const;
	virtual void CopyValueToEffect(
		Tr2RenderContextEnum::ShaderType inputType,
		unsigned char* destHandle,
		size_t size,
		Tr2RenderContext& renderContext ) const;
	//
	/////////////////////////////////////////////

	void SetValue( float value )
	{
		SetValue_( value );
	}
	void SetValue( int value )
	{
		SetValue_( value );
	}
	void SetValue( const Vector2& value )
	{
		SetValue_( value );
	}
	void SetValue( const Vector3& value )
	{
		SetValue_( value );
	}
	void SetValue( const Vector4& value )
	{
		SetValue_( value );
	}
	void SetValue( const Color& value )
	{
		SetValue_( value );
	}
	void SetValue( const Matrix& value )
	{
		SetValue_( value );
	}
	void SetValue( ITr2TextureProvider * value )
	{
		SetValueTextureRes( value );
	}
	void SetValue( ITr2GpuBuffer * value )
	{
		SetValueGpuBuffer( value );
	}

	// Invalidate the variable
	void Invalidate();

	// Clear the value, but leave the type alone, so a new SetValue will still work.
	void Clear();


	friend class Tr2VariableStore;
};

TYPEDEF_BLUECLASS( TriVariable );


#endif // TRIVARIABLE_H
