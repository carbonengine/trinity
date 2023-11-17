R"MSL(
#include <metal_stdlib>
using namespace metal;

#define clip(x) if (any((x) < 0)) discard_fragment()

#define any(x) any((x) != 0)
#define mul(a, b) ((a) * (b))
#define sincos(x, s, c) do { float cc; ( s ) = sincos( ( x ), cc ); ( c ) = cc; } while( false )

// These values must be synchronized with defines in TrinityAL/metal/MetalWorkQueue.h
#define CBUFFER(i) buffer(4 + i)
#define SRV(i) buffer(4 + i)
#define UAV(i) buffer(24 + i)
#define UAVT(i) texture(24 + i)

template< typename T, int n >
vec<float, n> mix( vec<T, n> x, vec<T, n> y, float a )
{ 
	return mix( static_cast< vec<float, n> >( x ), static_cast< vec<float, n> >( y ), vec<float, n>( a ) ); 
}

template< typename T, int n >
vec<float, n> asfloat( vec<T, n> v ) 
{ 
	return as_type< vec<float, n> >( v ); 
}

template< typename T >
float asfloat( T v ) 
{ 
	return as_type< float >( v ); 
}

template< typename T, int n >
vec<uint, n> asuint( vec<T, n> v ) 
{ 
	return as_type< vec<uint, n> >( v ); 
}

template< typename T >
uint asuint( T v ) 
{ 
	return as_type< uint >( v ); 
}

template< typename T, int n >
vec<int, n> asint( vec<T, n> v ) 
{ 
	return as_type< vec<int, n> >( v ); 
}

template< typename T >
int asint( T v ) 
{ 
	return as_type< int >( v ); 
}

void InterlockedCompareExchange( threadgroup uint& dest, uint compare_value, uint value, thread uint& original_value )
{
	original_value = atomic_compare_exchange_weak_explicit( (threadgroup atomic_uint*)&dest, &compare_value, value, memory_order_relaxed, memory_order_relaxed );
}

void InterlockedCompareExchange( threadgroup int& dest, int compare_value, int value, thread int& original_value )
{
	original_value = atomic_compare_exchange_weak_explicit( (threadgroup atomic_int*)&dest, &compare_value, value, memory_order_relaxed, memory_order_relaxed );
}

void InterlockedCompareExchange( device uint& dest, uint compare_value, uint value, thread uint& original_value )
{
	original_value = atomic_compare_exchange_weak_explicit( (device atomic_uint*)&dest, &compare_value, value, memory_order_relaxed, memory_order_relaxed );
}

void InterlockedCompareExchange( device int& dest, int compare_value, int value, thread int& original_value )
{
	original_value = atomic_compare_exchange_weak_explicit( (device atomic_int*)&dest, &compare_value, value, memory_order_relaxed, memory_order_relaxed );
}

void InterlockedCompareStore( threadgroup uint& dest, uint compare_value, uint value )
{
	atomic_compare_exchange_weak_explicit( (threadgroup atomic_uint*)&dest, &compare_value, value, memory_order_relaxed, memory_order_relaxed );
}

void InterlockedCompareStore( threadgroup int& dest, int compare_value, int value )
{
	atomic_compare_exchange_weak_explicit( (threadgroup atomic_int*)&dest, &compare_value, value, memory_order_relaxed, memory_order_relaxed );
}

void InterlockedCompareStore( device uint& dest, uint compare_value, uint value )
{
	atomic_compare_exchange_weak_explicit( (device atomic_uint*)&dest, &compare_value, value, memory_order_relaxed, memory_order_relaxed );
}

void InterlockedCompareStore( device int& dest, int compare_value, int value )
{
	atomic_compare_exchange_weak_explicit( (device atomic_int*)&dest, &compare_value, value, memory_order_relaxed, memory_order_relaxed );
}

void InterlockedExchange( threadgroup uint& dest, uint value, thread uint& original_value )
{
	original_value = atomic_exchange_explicit( (threadgroup atomic_uint*)&dest, value, memory_order_relaxed );
}

void InterlockedExchange( threadgroup int& dest, int value, thread int& original_value )
{
	original_value = atomic_exchange_explicit( (threadgroup atomic_int*)&dest, value, memory_order_relaxed );
}

void InterlockedExchange( device uint& dest, uint value, thread uint& original_value )
{
	original_value = atomic_exchange_explicit( (device atomic_uint*)&dest, value, memory_order_relaxed );
}

void InterlockedExchange( device int& dest, int value, thread int& original_value )
{
	original_value = atomic_exchange_explicit( (device atomic_int*)&dest, value, memory_order_relaxed );
}

float3x3 to_float3x3( thread const float4x3& m )
{
	return float3x3( m[0].xyz, m[1].xyz, m[2].xyz );
}

float3x3 to_float3x3( thread const float3x4& m )
{
	return float3x3( m[0].xyz, m[1].xyz, m[2].xyz );
}

float3x3 to_float3x3( thread const float4x4& m )
{
	return float3x3( m[0].xyz, m[1].xyz, m[2].xyz );
}

float4x3 to_float4x3( thread const float4x4& m )
{
	return float4x3( m[0].xyz, m[1].xyz, m[2].xyz, m[3].xyz );
}

float3x4 to_float3x4( thread const float4x4& m )
{
	return float3x4( m[0], m[1], m[2] );
}

float3x3 to_float3x3( constant const float4x3& m )
{
	return float3x3( m[0].xyz, m[1].xyz, m[2].xyz );
}

float3x3 to_float3x3( constant const float3x4& m )
{
	return float3x3( m[0].xyz, m[1].xyz, m[2].xyz );
}

float3x3 to_float3x3( constant const float4x4& m )
{
	return float3x3( m[0].xyz, m[1].xyz, m[2].xyz );
}

float4x3 to_float4x3( constant const float4x4& m )
{
	return float4x3( m[0].xyz, m[1].xyz, m[2].xyz, m[3].xyz );
}

float3x4 to_float3x4( constant const float4x4& m )
{
	return float3x4( m[0], m[1], m[2] );
}

float f16tof32( uint x )
{
	return static_cast<float>( as_type<half>( static_cast<uint16_t>( x ) ) );
}

template <size_t N>
vec<float, N> f16tof32( vec<uint, N> x )
{
	return static_cast<vec<float, N>>( as_type<vec<half, N>>( static_cast<vec<uint16_t, N>>( x ) ) );
}

uint f32tof16( float x )
{
	return as_type<uint16_t>( static_cast<half>( x ) );
}

template <size_t N>
vec<uint, N> f32tof16( vec<float, N> x )
{
	return static_cast<vec<uint, N>>( as_type<vec<uint16_t, N>>( static_cast<vec<half, N>>( x ) ) );
}

float3 matrixRow( float3x3 m, int row )
{
	return float3( m[0][row], m[1][row], m[2][row] );
}

float4 matrixRow( float4x4 m, int row )
{
	return float4( m[0][row], m[1][row], m[2][row], m[3][row] );
}

float3 matrixRow( float3x4 m, int row )
{
	return float3( m[0][row], m[1][row], m[2][row] );
}

float4 matrixRow( float4x3 m, int row )
{
	return float4( m[0][row], m[1][row], m[2][row], m[3][row] );
}

template <size_t Rows>
struct MatrixRow2LH
{
	MatrixRow2LH( thread matrix<float, 2, Rows>& m, int row ) :
		m_matrix( m ), m_row( row )
	{
	}
	operator float2() const
	{
		return float2( m_matrix[0][m_row], m_matrix[1][m_row] );
	}
#define ROW_ASSIGN_OP( op )                          \
	thread MatrixRow2LH& operator op( float2 value ) \
	{                                                \
		m_matrix[0][m_row] op value[0];              \
		m_matrix[1][m_row] op value[1];              \
		return *this;                                \
	}

	ROW_ASSIGN_OP( = )
	ROW_ASSIGN_OP( += )
	ROW_ASSIGN_OP( -= )
	ROW_ASSIGN_OP( *= )
	ROW_ASSIGN_OP( /= )

#undef ROW_ASSIGN_OP

	thread matrix<float, 2, Rows>& m_matrix;
	int m_row;
};

template <size_t Rows>
struct MatrixRow3LH
{
	MatrixRow3LH( thread matrix<float, 3, Rows>& m, int row ) :
		m_matrix( m ), m_row( row )
	{
	}
	operator float3() const
	{
		return float3( m_matrix[0][m_row], m_matrix[1][m_row], m_matrix[2][m_row] );
	}
#define ROW_ASSIGN_OP( op )                          \
	thread MatrixRow3LH& operator op( float3 value ) \
	{                                                \
		m_matrix[0][m_row] op value[0];              \
		m_matrix[1][m_row] op value[1];              \
		m_matrix[2][m_row] op value[2];              \
		return *this;                                \
	}

	ROW_ASSIGN_OP( = )
	ROW_ASSIGN_OP( += )
	ROW_ASSIGN_OP( -= )
	ROW_ASSIGN_OP( *= )
	ROW_ASSIGN_OP( /= )

#undef ROW_ASSIGN_OP

	thread matrix<float, 3, Rows>& m_matrix;
	int m_row;
};

template <size_t Rows>
struct MatrixRow4LH
{
	MatrixRow4LH( thread matrix<float, 4, Rows>& m, int row ) :
		m_matrix( m ), m_row( row )
	{
	}
	operator float4() const
	{
		return float4( m_matrix[0][m_row], m_matrix[1][m_row], m_matrix[2][m_row], m_matrix[3][m_row] );
	}
#define ROW_ASSIGN_OP( op )                          \
	thread MatrixRow4LH& operator op( float4 value ) \
	{                                                \
		m_matrix[0][m_row] op value[0];              \
		m_matrix[1][m_row] op value[1];              \
		m_matrix[2][m_row] op value[2];              \
		m_matrix[3][m_row] op value[3];              \
		return *this;                                \
	}

	ROW_ASSIGN_OP( = )
	ROW_ASSIGN_OP( += )
	ROW_ASSIGN_OP( -= )
	ROW_ASSIGN_OP( *= )
	ROW_ASSIGN_OP( /= )

#undef ROW_ASSIGN_OP

	thread matrix<float, 4, Rows>& m_matrix;
	int m_row;
};

float radians( float degrees )
{
	return degrees * 0.0174532924;
}
 
// We can be fairly confident that this will be ok, since dx11/dx12 shader compilers will complain if we use unaccepted types in here...
template <typename T>
T rcp( T v )
{
	return (T)1.0 / v;
}

template <typename T, typename W, typename H>
void GetDimensions( texture2d<T> tex, thread W& w, thread H& h )
{
	w = W( tex.get_width() );
	h = H( tex.get_height() );
}

template <typename T, typename W, typename H>
void GetDimensions( texturecube<T> tex, thread W& w, thread H& h )
{
	w = W( tex.get_width() );
	h = H( tex.get_height() );
}

template <typename T, typename W, typename H, typename D>
void GetDimensions( texture3d<T> tex, thread W& w, thread H& h, thread D& d )
{
	w = W( tex.get_width() );
	h = H( tex.get_height() );
	d = D( tex.get_depth() );
}

template <typename T, typename W, typename H>
void GetDimensions( texture2d_ms<T> tex, thread W& w, thread H& h )
{
	w = W( tex.get_width() );
	h = H( tex.get_height() );
}

template <typename T, typename W, typename H, typename S>
void GetDimensions( texture2d_ms<T> tex, thread W& w, thread H& h, thread S& s )
{
	w = W( tex.get_width() );
	h = H( tex.get_height() );
	s = S( tex.get_num_samples() );
}

template <typename TOut, typename TIn>
struct VectorReference1
{
	VectorReference1( TOut reference, int swizzles ) :
		m_reference( reference ),
		m_swizzles( swizzles )
	{
		m_value = reference[swizzles];
	}
	~VectorReference1()
	{
		m_reference[m_swizzles] = m_value;
	}
	operator TIn()
	{
		return m_value;
	}
	TOut m_reference;
	int m_swizzles;
	typename metal::remove_reference<TIn>::type m_value;
};
template <typename TOut, typename TIn>
struct VectorReference2
{
	VectorReference2( TOut reference, int2 swizzles ) :
		m_reference( reference ),
		m_swizzles( swizzles )
	{
		m_value[0] = reference[swizzles[0]];
		m_value[1] = reference[swizzles[1]];
	}
	~VectorReference2()
	{
		m_reference[m_swizzles[0]] = m_value[0];
		m_reference[m_swizzles[1]] = m_value[1];
	}
	operator TIn()
	{
		return m_value;
	}
	TOut m_reference;
	int2 m_swizzles;
	typename metal::remove_reference<TIn>::type m_value;
};
template <typename TOut, typename TIn>
struct VectorReference3
{
	VectorReference3( TOut reference, int3 swizzles ) :
		m_reference( reference ),
		m_swizzles( swizzles )
	{
		m_value[0] = reference[swizzles[0]];
		m_value[1] = reference[swizzles[1]];
		m_value[2] = reference[swizzles[2]];
	}
	~VectorReference3()
	{
		m_reference[m_swizzles[0]] = m_value[0];
		m_reference[m_swizzles[1]] = m_value[1];
		m_reference[m_swizzles[2]] = m_value[2];
	}
	operator TIn()
	{
		return m_value;
	}
	TOut m_reference;
	int3 m_swizzles;
	typename metal::remove_reference<TIn>::type m_value;
};
)MSL"
