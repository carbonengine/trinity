// Copyright © 2023 CCP ehf.

#include "StdAfx.h"
#include "TriValueBinding.h"
#include "ITriReroutable.h"
#include "include/ITriVector.h"
#include "include/ITriColor.h"
#include "include/ITriQuaternion.h"
#include "include/ITriMatrix.h"

TriValueBinding::TriValueBinding( IRoot* lockobj ) :
	m_isWeak( false ),
	m_isEnabled( true ),
	m_source( nullptr ),
	m_destination( nullptr ),
	m_notifyPtr( nullptr ),
	m_copyFunc( NULL ),
	m_scale( 1.0f ),
	m_offset( 0.0f, 0.0f, 0.0f, 0.0f ),
	m_sourceItemOffset( 0 ),
	m_destItemOffset( 0 )
{
}

TriValueBinding::~TriValueBinding()
{
	ITriReroutablePtr rp( BlueCastPtr( GetCurrentDestinationObject() ) );
	if( rp )
	{
		rp->UnregisterBinding( this );
	}
}

void TriValueBinding::CopyValue()
{
	if( !m_isEnabled )
	{
		return;
	}

	if( m_isWeak )
	{
		if( !m_sourceObjectWeak || !m_destinationObjectWeak )
		{
			return;
		}
	}

	if( m_copyFunc )
	{
		if( m_copyFunc( (Be::Var*)m_source, (Be::Var*)m_destination, m_scale, m_offset ) )
		{
			if( m_notifyPtr )
			{
				m_notifyPtr->OnModified( (Be::Var*)m_destination );
			}
		}
	}
	else if( m_copyValueCallable )
	{
		m_copyValueCallable.CallVoid( GetCurrentSourceObject(), GetCurrentDestinationObject() );
	}
}

bool TriValueBinding::OnModified( Be::Var* val )
{
	ITriReroutablePtr rp( BlueCastPtr( GetCurrentDestinationObject() ) );
	if( rp )
	{
		rp->UnregisterBinding( this );
	}

	Initialize();
	return true;
}

template <typename SrcT, typename DestT = SrcT>
static bool Copy( void* srcVar, void* dstVar, float, const Vector4& )
{
	auto src = static_cast<DestT>( *static_cast<SrcT*>( srcVar ) );
	auto& dst = *static_cast<DestT*>( dstVar );
	if( src != dst )
	{
		dst = src;
		return true;
	}
	return false;
}

template <typename SrcT, typename DestT = SrcT, typename TransformT = float>
static bool TransformAndCopy( void* srcVar, void* dstVar, float scale, const Vector4& offset )
{
	auto src = static_cast<DestT>( static_cast<TransformT>( *static_cast<SrcT*>( srcVar ) ) * scale + offset.x );
	auto& dst = *static_cast<DestT*>( dstVar );
	if( src != dst )
	{
		dst = src;
		return true;
	}
	return false;
}

template <typename T>
static bool TransformAndCopyVector( void* srcVar, void* dstVar, float scale, const Vector4& offset )
{
	auto src = *static_cast<T*>( srcVar ) * scale + *reinterpret_cast<const T*>( &offset );
	auto& dst = *static_cast<T*>( dstVar );
	if( src != dst )
	{
		dst = src;
		return true;
	}
	return false;
}

static bool ExtractMatrixPos3( void* srcVar, void* dstVar, float scale, const Vector4& offset )
{
	return TransformAndCopyVector<Vector3>( &static_cast<Matrix*>( srcVar )->GetTranslation(), dstVar, scale, offset );
}

static bool ExtractMatrixPos4( void* srcVar, void* dstVar, float scale, const Vector4& offset )
{
	return TransformAndCopyVector<Vector4>( &static_cast<Matrix*>( srcVar )->GetTranslation(), dstVar, scale, offset );
}

static bool Copy32BitFloatToVector3( void* srcVar, void* dstVar, float scale, const Vector4& offset )
{
	float value = *static_cast<float*>( srcVar ) * scale;
	Vector3 src( value + offset.x, value + offset.y, value + offset.z );
	Vector3& dest = *static_cast<Vector3*>( dstVar );
	if( src != dest )
	{
		dest = src;
		return true;
	}
	return false;
}

static bool Copy32BitFloatToVector4( void* srcVar, void* dstVar, float scale, const Vector4& offset )
{
	float value = *static_cast<float*>( srcVar ) * scale;
	Vector4 src( value + offset.x, value + offset.y, value + offset.z, value + offset.w );
	Vector4& dest = *static_cast<Vector4*>( dstVar );
	if( src != dest )
	{
		dest = src;
		return true;
	}
	return false;
}

static bool CopyTriVectorToVector3( void* srcVar, void* dstVar, float scale, const Vector4& offset )
{
	ITriVectorPtr vp;
	if( static_cast<IRoot*>( srcVar )->QueryInterface( BlueInterfaceIID<ITriVector>(), (void**)&vp, BEQI_SILENT ) )
	{
		Vector3 src;
		vp->CopyVector( &src );
		src = src * scale + offset.GetXYZ();
		auto& dst = *static_cast<Vector3*>( dstVar );
		if( src != dst )
		{
			dst = src;
			return true;
		}
	}
	return false;
}

static bool CopyTriVectorToVector2( void* srcVar, void* dstVar, float scale, const Vector4& offset )
{
	if( ITriVectorPtr vp = BlueCastPtr( static_cast<IRoot*>( srcVar ) ) )
	{
		const Vector3* p = vp->GetVector();
		Vector2 src( p->x * scale + offset.x, p->y * scale + offset.y );
		Vector2& dst = *static_cast<Vector2*>( dstVar );
		if( src != dst )
		{
			dst = src;
			return true;
		}
	}
	return false;
}

static bool CopyTriColorToFloatArray( void* srcVar, void* dstVar, float scale, const Vector4& offset )
{
	if( ITriColorPtr cp = BlueCastPtr( static_cast<IRoot*>( srcVar ) ) )
	{
		Color color;
		cp->CopyColor( &color );
		Vector4 src = Vector4( color ) * scale + offset;
		Vector4& d = *static_cast<Vector4*>( dstVar );
		if( src != d )
		{
			d = src;
			return true;
		}
	}
	return false;
}

static bool CopyTriQuaternionToFloatArray( void* srcVar, void* dstVar, float scale, const Vector4& offset )
{
	if( ITriQuaternionPtr cp = BlueCastPtr( static_cast<IRoot*>( srcVar ) ) )
	{
		Quaternion q;
		cp->CopyQuaternion( &q );
		Vector4 src( q.x * scale + offset.x, q.y * scale + offset.y, q.z * scale + offset.z, q.w * scale + offset.w );
		Vector4& d = *static_cast<Vector4*>( dstVar );
		if( src != d )
		{
			d = src;
			return true;
		}
	}
	return false;
}

static bool CopyMatrixToTriMatrix( void* srcVar, void* dstVar, float /*scale*/, const Vector4& /*offset*/ )
{
	if( ITriMatrixPtr mp = BlueCastPtr( static_cast<IRoot*>( srcVar ) ) )
	{
		auto src = static_cast<Matrix*>( srcVar );
		auto dst = mp->GetMatrix();
		if( *src != *dst )
		{
			mp->SetMatrix( static_cast<Matrix*>( srcVar ) );
			return true;
		}
	}
	return false;
}

static bool CopyVector3ToTriVector( void* srcVar, void* dstVar, float scale, const Vector4& offset )
{
	if( ITriVectorPtr vp = BlueCastPtr( static_cast<IRoot*>( srcVar ) ) )
	{
		Vector3 src = *static_cast<Vector3*>( srcVar ) * scale + offset.GetXYZ();
		auto dest = *vp->GetVector();
		if( src != dest )
		{
			vp->SetXYZ( src.x, src.y, src.z );
			return true;
		}
	}
	return false;
}

static bool CopyFloatArrayToTriColor( void* srcVar, void* dstVar, float scale, const Vector4& offset )
{
	if( ITriColorPtr cp = BlueCastPtr( static_cast<IRoot*>( srcVar ) ) )
	{
		Vector4 src = *static_cast<Color*>( srcVar ) * scale + offset;
		Vector4 dst = *cp->GetColor();
		if( src != dst )
		{
			cp->SetRGB( src.x, src.y, src.z, src.w );
			return true;
		}
	}
	return false;
}

static bool CopyFloatArrayToTriQuaternion( void* srcVar, void* dstVar, float scale, const Vector4& offset )
{
	if( ITriQuaternionPtr cp = BlueCastPtr( static_cast<IRoot*>( srcVar ) ) )
	{
		Vector4 src = *static_cast<Vector4*>( srcVar ) * scale + offset;
		Vector4 dst = *reinterpret_cast<const Vector4*>( cp->GetQuaternion() );
		if( src != dst )
		{
			cp->SetXYZW( src.x, src.y, src.z, src.w );
			return true;
		}
	}
	return false;
}

const Be::VarEntry* TriValueBinding::FindEntry( const char* name, const Be::ClassInfo* type, ssize_t& offs )
{
	offs = 0;
	// Loop over all entries - this double loop covers chaining
	for( ; type; offs += type->mOffsetToParent, type = type->mParentClassInfo )
	{
		for( const Be::VarEntry* entry = type->mMemberTable; entry->mName; entry++ )
		{
			if( !entry->mGetProperty && strcmp( entry->mName, name ) == 0 )
			{
				return entry;
			}
		}
	}

	return NULL;
}

namespace
{
std::optional<uint8_t> GetItemOffset( const std::string& item )
{
	if( item == "x" || item == "r" )
	{
		return 0;
	}
	if( item == "y" || item == "g" )
	{
		return 1;
	}
	if( item == "z" || item == "b" )
	{
		return 2;
	}
	if( item == "w" || item == "a" )
	{
		return 3;
	}
	return std::nullopt;
}
}

void TriValueBinding::Initialize()
{
	m_source = NULL;
	m_destination = NULL;
	m_copyFunc = NULL;
	m_notifyPtr = nullptr;

	IRoot* sourceObject = GetCurrentSourceObject();
	IRoot* destinationObject = GetCurrentDestinationObject();

	if( !sourceObject || !destinationObject )
	{
		// No point doing anything yet - all copy functions need a source and destination object
		return;
	}

	if( !m_copyValueCallable && ( m_sourceAttribute.empty() || m_destinationAttribute.empty() ) )
	{
		// All copy functions except the Python callback require a source and destination attribute
		return;
	}

	const Be::ClassInfo* srcClassInfo = sourceObject->ClassType();
	const Be::ClassInfo* dstClassInfo = destinationObject->ClassType();

	size_t dataSize = 0;

	m_sourceItemOffset = 0;
	bool sourceFloatArrayAsFloat = false;

	std::string sourceAttr = m_sourceAttribute;
	size_t sourceDot = sourceAttr.find( '.' );
	if( sourceDot != std::string::npos )
	{
		std::string sourceItem = sourceAttr.substr( sourceDot + 1 );
		if( auto offset = GetItemOffset( sourceItem ) )
		{
			sourceFloatArrayAsFloat = true;
			m_sourceItemOffset = *offset * sizeof( float );
		}
		sourceAttr = sourceAttr.substr( 0, sourceDot );
	}

	m_destItemOffset = 0;
	bool destFloatArrayAsFloat = false;

	std::string destAttr = m_destinationAttribute.c_str();
	size_t destDot = destAttr.find( '.' );
	if( destDot != std::string::npos )
	{
		std::string destItem = destAttr.substr( destDot + 1 );
		if( auto offset = GetItemOffset( destItem ) )
		{
			destFloatArrayAsFloat = true;
			m_destItemOffset = *offset * sizeof( float );
		}
		destAttr = destAttr.substr( 0, destDot );
	}

	if( !m_copyValueCallable )
	{
		ssize_t srcOffset = 0;
		ssize_t dstOffset = 0;

		const Be::VarEntry* srcEntry = FindEntry( sourceAttr.c_str(), srcClassInfo, srcOffset );
		const Be::VarEntry* dstEntry = FindEntry( destAttr.c_str(), dstClassInfo, dstOffset );
		if( srcEntry && dstEntry )
		{
			m_source = (void*)BLUEMAPMEMBEROFFSET( sourceObject, srcEntry, srcClassInfo, srcOffset );
			m_destination = (void*)BLUEMAPMEMBEROFFSET( destinationObject, dstEntry, dstClassInfo, dstOffset );
			dataSize = DetermineCopyFunc( srcEntry, dstEntry, dataSize, sourceFloatArrayAsFloat, destFloatArrayAsFloat );
		}
		else
		{
			if( !srcEntry )
			{
				CCP_LOGWARN( "TriValueBinding: Source attribute '%s' not valid", m_sourceAttribute.c_str() );
			}
			if( !dstEntry )
			{
				CCP_LOGWARN( "TriValueBinding: Destination attribute '%s' not valid", m_destinationAttribute.c_str() );
			}
		}

		if( dstEntry && ( dstEntry->mEditFlags & Be::NOTIFY ) )
		{
			m_notifyPtr = INotifyPtr( BlueCastPtr( destinationObject ) );
		}
	}

	if( m_copyFunc )
	{
		ITriReroutablePtr rp( BlueCastPtr( destinationObject ) );
		if( rp )
		{
			rp->RegisterBinding( this );
			void* dest = NULL;
			size_t size = 0;
			rp->GetDestination( dest, size );
			if( size >= dataSize )
			{
				m_destination = (void*)( (uint8_t*)dest + m_destItemOffset );
			}
			else
			{
				CCP_LOGWARN( "TriValueBinding: Rerouted destination too small!" );
			}
		}
	}
}

void TriValueBinding::RerouteDestination( void* dest )
{
	m_destination = (void*)( (uint8_t*)dest + m_destItemOffset );
}

size_t TriValueBinding::DetermineCopyFunc( const Be::VarEntry* srcEntry, const Be::VarEntry* dstEntry, size_t dataSize, bool sourceFloatArrayAsFloat, bool destFloatArrayAsFloat )
{
	if( sourceFloatArrayAsFloat && srcEntry->mType != Be::FLOATARRAY )
	{
		CCP_LOGERR( "TriValueBinding \"%s\": float swizzle (.xyzw) on a non-array source value \"%s\"", m_name.c_str(), srcEntry->mName );
		return dataSize;
	}
	if( destFloatArrayAsFloat && dstEntry->mType != Be::FLOATARRAY )
	{
		CCP_LOGERR( "TriValueBinding \"%s\": float swizzle (.xyzw) on a non-array destination value \"%s\"", m_name.c_str(), dstEntry->mName );
		return dataSize;
	}

	if( srcEntry->mType == dstEntry->mType )
	{
		switch( srcEntry->mType )
		{
		case Be::BYTE:
			m_copyFunc = TransformAndCopy<uint8_t>;
			dataSize = 1;
			break;

		case Be::BOOL:
			m_copyFunc = TransformAndCopy<bool>;
			dataSize = 1;
			break;

		case Be::SHORT:
			m_copyFunc = TransformAndCopy<int16_t>;
			dataSize = 2;
			break;

		case Be::LONG:
			m_copyFunc = TransformAndCopy<int32_t>;
			dataSize = 4;
			break;

		case Be::FLOAT:
			m_copyFunc = TransformAndCopy<float>;
			dataSize = 4;
			break;

		case Be::DOUBLE:
			m_copyFunc = TransformAndCopy<double, double, double>;
			dataSize = 8;
			break;

		case Be::INT64:
			m_copyFunc = Copy<int64_t>;
			dataSize = 8;
			break;

		case Be::FLOATARRAY:
			if( sourceFloatArrayAsFloat )
			{
				if( destFloatArrayAsFloat )
				{
					m_copyFunc = TransformAndCopy<float>;
					m_source = (void*)( (uint8_t*)m_source + m_sourceItemOffset );
					m_destination = (void*)( (uint8_t*)m_destination + m_destItemOffset );
					dataSize = 4;
				}
				else if( dstEntry->GetFloatArraySize() == 3 )
				{
					m_source = (void*)( (uint8_t*)m_source + m_sourceItemOffset );
					m_copyFunc = Copy32BitFloatToVector3;
					dataSize = 12;
				}
				else if( dstEntry->GetFloatArraySize() == 4 )
				{
					m_source = (void*)( (uint8_t*)m_source + m_sourceItemOffset );
					m_copyFunc = Copy32BitFloatToVector4;
					dataSize = 16;
				}
			}
			else
			{
				if( dstEntry->GetFloatArraySize() >= srcEntry->GetFloatArraySize() )
				{
					switch( srcEntry->GetFloatArraySize() )
					{
					case 2:
						m_copyFunc = TransformAndCopyVector<Vector2>;
						dataSize = 12;
						break;
					case 3:
						m_copyFunc = TransformAndCopyVector<Vector3>;
						dataSize = 12;
						break;
					case 4:
						m_copyFunc = TransformAndCopyVector<Vector4>;
						dataSize = 16;
						break;
					case 16:
						m_copyFunc = Copy<Matrix>;
						dataSize = 64;
						break;

					default:
						CCP_LOGWARN( "TriValueBinding: Float array size not handled" );
					}
				}
				else
				{
					if( srcEntry->GetFloatArraySize() == 16 )
					{
						// if going from a full matrix to size 3 or 4, assume we're extracting position
						switch( dstEntry->GetFloatArraySize() )
						{
						case 3:
							m_copyFunc = ExtractMatrixPos3;
							dataSize = 12;
							break;
						case 4:
							m_copyFunc = ExtractMatrixPos4;
							dataSize = 16;
							break;

						default:
							CCP_LOGWARN( "TriValueBinding: src float array size is bigger than dst, and don't recognize dst" );
						}
					}
					else
					{
						switch( dstEntry->GetFloatArraySize() )
						{
						case 2:
							m_copyFunc = TransformAndCopyVector<Vector2>;
							dataSize = 8;
							break;
						case 3:
							m_copyFunc = TransformAndCopyVector<Vector3>;
							dataSize = 12;
							break;
						case 4:
							m_copyFunc = TransformAndCopyVector<Vector4>;
							dataSize = 16;
							break;
						default:
							CCP_LOGWARN( "TriValueBinding: Float array size not handled" );
						}
					}
				}
			}
			break;

		case Be::IROOT:
		case Be::IROOTPTR:
		case Be::CHARARRAY:
		case Be::CSTRING:
		case Be::REFERENCE:
		case Be::WCSTRING:
		case Be::WREFERENCE:
		case Be::PYOBJECTPTR:
			// TODO: Do we want to handle those types?
			CCP_LOGWARN( "TriValueBinding: Unsupported value type" );
			break;

		default:
			CCP_LOGERR( "TriValueBinding: Unknown blue type in member %s", srcEntry->mName );
			break;
		}
	}
	else if( sourceFloatArrayAsFloat && srcEntry->mType == Be::FLOATARRAY && dstEntry->mType == Be::FLOAT )
	{
		m_copyFunc = TransformAndCopy<float>;
		m_source = (void*)( (uint8_t*)m_source + m_sourceItemOffset );
		dataSize = 4;
	}
	else if( sourceFloatArrayAsFloat && srcEntry->mType == Be::FLOATARRAY && dstEntry->mType == Be::DOUBLE )
	{
		m_copyFunc = TransformAndCopy<float, double, double>;
		m_source = (void*)( (uint8_t*)m_source + m_sourceItemOffset );
		dataSize = 8;
	}
	else if( srcEntry->mType == Be::FLOAT && dstEntry->mType == Be::FLOATARRAY )
	{
		if( destFloatArrayAsFloat )
		{
			m_copyFunc = TransformAndCopy<float>;
			m_destination = (void*)( (uint8_t*)m_destination + m_destItemOffset );
			dataSize = 4;
		}
		else if( dstEntry->GetFloatArraySize() == 3 )
		{
			m_copyFunc = Copy32BitFloatToVector3;
			dataSize = 12;
		}
		else if( dstEntry->GetFloatArraySize() == 4 )
		{
			m_copyFunc = Copy32BitFloatToVector4;
			dataSize = 16;
		}
	}
	else if( srcEntry->mType == Be::FLOAT && dstEntry->mType == Be::BOOL )
	{
		m_copyFunc = Copy<float, bool>;
		dataSize = 1;
	}
	else if( srcEntry->mType == Be::FLOAT && dstEntry->mType == Be::DOUBLE )
	{
		m_copyFunc = TransformAndCopy<float, double, double>;
		dataSize = 1;
	}
	else if( srcEntry->mType == Be::DOUBLE && dstEntry->mType == Be::FLOAT )
	{
		m_copyFunc = TransformAndCopy<double, float>;
		dataSize = 1;
	}
	else if( srcEntry->mType == Be::IROOT )
	{
		ITriVectorPtr vp;
		if( ( (IRoot*)m_source )->QueryInterface( BlueInterfaceIID<ITriVector>(), (void**)&vp, BEQI_SILENT ) )
		{
			if( dstEntry->mType == Be::FLOATARRAY )
			{
				if( dstEntry->GetFloatArraySize() == 2 )
				{
					m_copyFunc = CopyTriVectorToVector2;
					dataSize = 8;
				}
				else if( dstEntry->GetFloatArraySize() == 3 )
				{
					m_copyFunc = CopyTriVectorToVector3;
					dataSize = 12;
				}
				else if( dstEntry->GetFloatArraySize() == 4 )
				{
					// The source is 3-component, so we can reuse the 3-component copy function
					m_copyFunc = CopyTriVectorToVector3;
					dataSize = 16;
				}
			}
		}

		ITriColorPtr cp;
		if( ( (IRoot*)m_source )->QueryInterface( BlueInterfaceIID<ITriColor>(), (void**)&cp, BEQI_SILENT ) )
		{
			if( dstEntry->mType == Be::FLOATARRAY && dstEntry->GetFloatArraySize() >= 4 )
			{
				m_copyFunc = CopyTriColorToFloatArray;
				dataSize = 16;
			}
		}

		ITriQuaternionPtr qp;
		if( ( (IRoot*)m_source )->QueryInterface( BlueInterfaceIID<ITriQuaternion>(), (void**)&qp, BEQI_SILENT ) )
		{
			if( dstEntry->mType == Be::FLOATARRAY && dstEntry->GetFloatArraySize() >= 4 )
			{
				m_copyFunc = CopyTriQuaternionToFloatArray;
				dataSize = 16;
			}
		}

		if( !m_copyFunc )
		{
			CCP_LOGWARN( "TriValueBinding: No suitable mapping from '%s' to '%s'", srcEntry->mName, dstEntry->mName );
		}
	}
	else if( dstEntry->mType == Be::IROOT )
	{
		ITriMatrixPtr mp;
		if( ( (IRoot*)m_destination )->QueryInterface( BlueInterfaceIID<ITriMatrix>(), (void**)&mp, BEQI_SILENT ) )
		{
			if( srcEntry->mType == Be::FLOATARRAY )
			{
				if( srcEntry->GetFloatArraySize() == 16 )
				{
					m_copyFunc = CopyMatrixToTriMatrix;
					dataSize = 64;
				}
			}
		}
		ITriVectorPtr vp;
		if( ( (IRoot*)m_destination )->QueryInterface( BlueInterfaceIID<ITriVector>(), (void**)&vp, BEQI_SILENT ) )
		{
			if( srcEntry->mType == Be::FLOATARRAY )
			{
				if( srcEntry->GetFloatArraySize() == 3 )
				{
					m_copyFunc = CopyVector3ToTriVector;
					dataSize = 12;
				}
				else if( srcEntry->GetFloatArraySize() == 4 )
				{
					m_copyFunc = CopyVector3ToTriVector;
					dataSize = 12;
				}
			}
		}

		ITriColorPtr cp;
		if( ( (IRoot*)m_destination )->QueryInterface( BlueInterfaceIID<ITriColor>(), (void**)&cp, BEQI_SILENT ) )
		{
			if( srcEntry->mType == Be::FLOATARRAY && srcEntry->GetFloatArraySize() == 4 )
			{
				m_copyFunc = CopyFloatArrayToTriColor;
				dataSize = 16;
			}
		}

		ITriQuaternionPtr qp;
		if( ( (IRoot*)m_destination )->QueryInterface( BlueInterfaceIID<ITriQuaternion>(), (void**)&qp, BEQI_SILENT ) )
		{
			if( srcEntry->mType == Be::FLOATARRAY && srcEntry->GetFloatArraySize() == 4 )
			{
				m_copyFunc = CopyFloatArrayToTriQuaternion;
				dataSize = 16;
			}
		}
		if( !m_copyFunc )
		{
			CCP_LOGWARN( "TriValueBinding: No suitable mapping from '%s' to '%s'", srcEntry->mName, dstEntry->mName );
		}
	}
	else
	{
		CCP_LOGWARN( "TriValueBinding: Source and destination types don't match" );
	}
	return dataSize;
}

void TriValueBinding::SetSource( const std::string& sourceAttribute, IRootPtr sourceObject )
{
	m_sourceAttribute = sourceAttribute;
	m_sourceObject = sourceObject;
	m_sourceObjectWeak = nullptr;
	m_isWeak = false;
}

void TriValueBinding::SetDestination( const std::string& destinationAttribute, IRootPtr destinationObject )
{
	m_destinationAttribute = destinationAttribute;
	m_destinationObject = destinationObject;
	m_destinationObjectWeak = nullptr;
	m_isWeak = false;
}

void TriValueBinding::CreateWeakBinding( IRoot* source, const char* sourceAttr, IRoot* dest, const char* destAttr, float scale, const Vector4& offset )
{
	if( ITriReroutablePtr rp = BlueCastPtr( GetCurrentDestinationObject() ) )
	{
		rp->UnregisterBinding( this );
	}

	m_sourceAttribute = sourceAttr;
	m_sourceObject = nullptr;
	m_sourceObjectWeak = source;
	m_destinationAttribute = destAttr;
	m_destinationObject = nullptr;
	m_destinationObjectWeak = dest;
	m_scale = scale;
	m_offset = offset;
	m_isWeak = true;
	m_notifyPtr = nullptr;
	Initialize();
}

bool TriValueBinding::IsValid() const
{
	return m_copyFunc != nullptr;
}

void TriValueBinding::SetScale( float scale )
{
	m_scale = scale;
}

IRoot* TriValueBinding::GetCurrentSourceObject() const
{
	return m_isWeak ? static_cast<IRoot*>( m_sourceObjectWeak ) : m_sourceObject.p;
}

IRoot* TriValueBinding::GetCurrentDestinationObject() const
{
	return m_isWeak ? static_cast<IRoot*>( m_destinationObjectWeak ) : m_destinationObject.p;
}

IRootPtr TriValueBinding::GetSourceObject() const
{
	return GetCurrentSourceObject();
}

void TriValueBinding::SetSourceObject( IRoot* sourceObject )
{
	if( m_isWeak )
	{
		m_sourceObjectWeak = sourceObject;
	}
	else
	{
		m_sourceObject = sourceObject;
	}
	if( ITriReroutablePtr rp = BlueCastPtr( GetCurrentDestinationObject() ) )
	{
		rp->UnregisterBinding( this );
	}
	Initialize();
}

IRootPtr TriValueBinding::GetDestinationObject() const
{
	return GetCurrentDestinationObject();
}

void TriValueBinding::SetDestinationObject( IRoot* destinationObject )
{
	if( ITriReroutablePtr rp = BlueCastPtr( GetCurrentDestinationObject() ) )
	{
		rp->UnregisterBinding( this );
	}

	if( m_isWeak )
	{
		m_destinationObjectWeak = destinationObject;
	}
	else
	{
		m_destinationObject = destinationObject;
	}
	Initialize();
}
