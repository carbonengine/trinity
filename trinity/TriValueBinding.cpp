#include "StdAfx.h"
#include "TriValueBinding.h"
#include "ITriReroutable.h"
#include "include/ITriVector.h"
#include "include/ITriColor.h"
#include "include/ITriQuaternion.h"
#include "include/ITriMatrix.h"

TriValueBinding::TriValueBinding( IRoot* lockobj ) :
	m_isValid( false ),
	m_isEnabled( true ),
	m_source( nullptr ),
	m_destination( nullptr ),
	m_copyFunc( NULL ),
	m_scale( 1.0f ),
	m_offset( 0.0f, 0.0f, 0.0f, 0.0f ),
	m_sourceItemOffset( 0 ),
	m_destItemOffset( 0 )
{
}

TriValueBinding::~TriValueBinding()
{
	ITriReroutablePtr rp( BlueCastPtr( m_destinationObject ) );
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

	if( m_copyFunc )
	{
		m_copyFunc( (Be::Var*)m_source, (Be::Var*)m_destination, m_scale, m_offset );
		if( m_notifyPtr )
		{
			m_notifyPtr->OnModified( (Be::Var*)m_destination );
		}
	}
	else if( m_copyValueCallable )
	{
		m_copyValueCallable.CallVoid( m_sourceObject, m_destinationObject );
	}
}

bool TriValueBinding::OnModified( Be::Var* val )
{
	ITriReroutablePtr rp( BlueCastPtr( m_destinationObject ) );
	if( rp )
	{
		rp->UnregisterBinding( this );
	}

	Initialize();
	return true;
}

static void Copy8Bit( void* srcVar, void* dstVar, float scale, const Vector4& offset )
{
	*(uint8_t*)dstVar = (uint8_t)(*static_cast<bool*>( srcVar ) * scale + offset.x);
}

static void Copy16Bit( void* srcVar, void* dstVar, float scale, const Vector4& offset )
{
	*static_cast<int16_t*>( dstVar ) = int16_t( *static_cast<float*>( srcVar ) * scale + offset.x );
}

static void Copy32Bit( void* srcVar, void* dstVar, float scale, const Vector4& offset )
{
	*static_cast<int32_t*>( dstVar ) = int32_t( *static_cast<float*>( srcVar ) * scale + offset.x );
}

static void Copy32BitFloat( void* srcVar, void* dstVar, float scale, const Vector4& offset )
{
	*static_cast<float*>( dstVar ) = *static_cast<float*>( srcVar ) * scale + offset.x;
}

static void Copy32BitFloatToBool( void* srcVar, void* dstVar, float /*scale*/, const Vector4& /*offset*/ )
{
	*static_cast<bool*>( dstVar ) = *static_cast<float*>( srcVar ) != 0.0f;
}

static void Copy64BitFloat( void* srcVar, void* dstVar, float scale, const Vector4& offset )
{
	*static_cast<double*>( dstVar ) = *static_cast<double*>( srcVar ) * static_cast<double>( scale ) + static_cast<double>( offset.x );
}

static void CopyInt64( void* srcVar, void* dstVar, float scale, const Vector4& offset )
{
	// todo scale
	*static_cast<int64_t*>( dstVar ) = *static_cast<int64_t*>( srcVar );
}

static void CopyVector2( void* srcVar, void* dstVar, float scale, const Vector4& offset )
{
	Vector2 &d = *static_cast<Vector2*>( dstVar );
	d = *static_cast<Vector2*>( srcVar ) * scale;
	d.x += offset.x;
	d.y += offset.y;
}

static void CopyVector3( void* srcVar, void* dstVar, float scale, const Vector4& offset )
{
	*static_cast<Vector3*>( dstVar ) = *static_cast<Vector3*>( srcVar ) * scale;
	Vector3 &d = *static_cast<Vector3*>( dstVar );
	d.x += offset.x;
	d.y += offset.y;
	d.z += offset.z;
}

static void CopyVector4( void* srcVar, void* dstVar, float scale, const Vector4& offset )
{
	Vector4 &d = *static_cast<Vector4*>( dstVar );
	d = *static_cast<Vector4*>( srcVar ) * scale;
	d.x += offset.x;
	d.y += offset.y;
	d.z += offset.z;
	d.w += offset.w;
}

static void CopyMatrix( void* srcVar, void* dstVar, float /*scale*/, const Vector4& /*offset*/ )
{
	// todo scale
	memcpy( dstVar, srcVar, 16 * sizeof( float ) );
}

static void ExtractMatrixPos3( void* srcVar, void* dstVar, float scale, const Vector4& offset )
{
	CopyVector3( static_cast<float*>( srcVar ) + 3 * 4, dstVar, scale, offset );
}

static void ExtractMatrixPos4( void* srcVar, void* dstVar, float scale, const Vector4& offset )
{
	CopyVector4( static_cast<float*>( srcVar ) + 3 * 4, dstVar, scale, offset );	
}

static void Copy32BitFloatToVector3( void* srcVar, void* dstVar, float scale, const Vector4& offset )
{
	float value = *static_cast<float*>( srcVar ) * scale;
	Vector3* dest = static_cast<Vector3*>( dstVar );
	dest->x = value + offset.x;
	dest->y = value + offset.y;
	dest->z = value + offset.z;
}

static void Copy32BitFloatToVector4( void* srcVar, void* dstVar, float scale, const Vector4& offset )
{
	float value = *static_cast<float*>( srcVar ) * scale;
	Vector4* dest = static_cast<Vector4*>( dstVar );
	dest->x = value + offset.x;
	dest->y = value + offset.y;
	dest->z = value + offset.z;
	dest->w = value + offset.w;
}

static void Copy32BitFloatToDouble( void* srcVar, void* dstVar, float scale, const Vector4& offset )
{
	*static_cast<double*>( dstVar ) = double( *static_cast<float*>( srcVar ) ) * double( scale ) + double( offset.x );
}

static void CopyDoubleTo32BitFloat( void* srcVar, void* dstVar, float scale, const Vector4& offset )
{
	*static_cast<float*>( dstVar ) = float( *static_cast<double*>( srcVar ) * scale + offset.x );
}

static void CopyTriVectorToVector3( void* srcVar, void* dstVar, float scale, const Vector4& offset )
{
	ITriVectorPtr vp;
	if( static_cast<IRoot*>( srcVar )->QueryInterface( BlueInterfaceIID<ITriVector>(), (void**)&vp, BEQI_SILENT ) )
	{
		vp->CopyVector( static_cast<Vector3*>( dstVar ) );
		*static_cast<Vector3*>( dstVar ) = *static_cast<Vector3*>( dstVar ) * scale;
		static_cast<Vector3*>( dstVar )->x += offset.x;
		static_cast<Vector3*>( dstVar )->y += offset.y;
		static_cast<Vector3*>( dstVar )->z += offset.z;
	}
}

static void CopyTriVectorToVector2( void* srcVar, void* dstVar, float scale, const Vector4& offset )
{
	ITriVectorPtr vp;
	if( static_cast<IRoot*>( srcVar )->QueryInterface( BlueInterfaceIID<ITriVector>(), (void**)&vp, BEQI_SILENT ) )
	{
		const Vector3* p = vp->GetVector();
		Vector2* dst = static_cast<Vector2*>( dstVar );
		dst->x = p->x * scale + offset.x;
		dst->y = p->y * scale + offset.y;
	}
}

static void CopyTriVectorToVector4( void* srcVar, void* dstVar, float scale, const Vector4& offset )
{
	ITriVectorPtr vp;
	if( static_cast<IRoot*>( srcVar )->QueryInterface( BlueInterfaceIID<ITriVector>(), (void**)&vp, BEQI_SILENT ) )
	{
		const Vector3* p = vp->GetVector();
		Vector4* dst = static_cast<Vector4*>( dstVar );
		dst->x = p->x * scale + offset.x;
		dst->y = p->y * scale + offset.y;
		dst->z = p->z * scale + offset.z;
	}
}

static void CopyTriColorToFloatArray( void* srcVar, void* dstVar, float scale, const Vector4& offset )
{
	// TODO: Doing this QueryInterface per frame is quite expensive!
	ITriColorPtr cp;
	if( static_cast<IRoot*>( srcVar )->QueryInterface( BlueInterfaceIID<ITriColor>(), (void**)&cp, BEQI_SILENT ) )
	{
		cp->CopyColor( static_cast<Color*>( dstVar ) );
		Vector4 &d = *static_cast<Vector4*>( dstVar );
		d = d * scale + offset;
	}
}

static void CopyTriQuaternionToFloatArray( void* srcVar, void* dstVar, float scale, const Vector4& offset )
{
	ITriQuaternionPtr cp;
	if( static_cast<IRoot*>( srcVar )->QueryInterface( BlueInterfaceIID<ITriQuaternion>(), (void**)&cp, BEQI_SILENT ) )
	{
		cp->CopyQuaternion( static_cast<Quaternion*>( dstVar ) );
		Vector4 &d = *static_cast<Vector4*>( dstVar );
		d = d * scale + offset;
	}
}

static void CopyMatrixToTriMatrix( void* srcVar, void* dstVar, float /*scale*/, const Vector4& /*offset*/ )
{
	ITriMatrixPtr mp;
	if( static_cast<IRoot*>( dstVar )->QueryInterface( BlueInterfaceIID<ITriMatrix>(), (void**)&mp, BEQI_SILENT ) )
	{
		mp->SetMatrix( static_cast<Matrix*>( srcVar ) );
	}
}

static void CopyVector3ToTriVector( void* srcVar, void* dstVar, float scale, const Vector4& offset )
{
	ITriVectorPtr vp;
	if( static_cast<IRoot*>( dstVar )->QueryInterface( BlueInterfaceIID<ITriVector>(), (void**)&vp, BEQI_SILENT ) )
	{
		Vector3* src = static_cast<Vector3*>( srcVar );
		vp->SetXYZ( src->x * scale, src->y * scale, src->z * scale );
	}
}

static void CopyVector4ToTriVector( void* srcVar, void* dstVar, float scale, const Vector4& offset )
{
	ITriVectorPtr vp;
	if( static_cast<IRoot*>( dstVar )->QueryInterface( BlueInterfaceIID<ITriVector>(), (void**)&vp, BEQI_SILENT ) )
	{
		Vector3* src = static_cast<Vector3*>( srcVar );
		vp->SetXYZ( src->x * scale, src->y * scale, src->z * scale );
	}
}

static void CopyFloatArrayToTriColor( void* srcVar, void* dstVar, float scale, const Vector4& offset )
{
	ITriColorPtr cp;
	if( static_cast<IRoot*>( dstVar )->QueryInterface( BlueInterfaceIID<ITriColor>(), (void**)&cp, BEQI_SILENT ) )
	{
		Color* src = static_cast<Color*>( srcVar );
		cp->SetRGB( src->r * scale, src->g * scale, src->b * scale, src->a * scale );
	}
}

static void CopyFloatArrayToTriQuaternion( void* srcVar, void* dstVar, float scale, const Vector4& offset )
{
	ITriQuaternionPtr cp;
	if( static_cast<IRoot*>( dstVar )->QueryInterface( BlueInterfaceIID<ITriQuaternion>(), (void**)&cp, BEQI_SILENT ) )
	{
		Quaternion* src = static_cast<Quaternion*>( srcVar );
		cp->SetXYZW( src->x * scale, src->y * scale, src->z * scale, src->w * scale );
	}
}

const Be::VarEntry* TriValueBinding::FindEntry( const char* name, const Be::ClassInfo* type, ssize_t& offs )
{
	offs = 0;
	// Loop over all entries - this double loop covers chaining
	for (; type; offs += type->mOffsetToParent, type = type->mParentClassInfo)
	{
		for( const Be::VarEntry* entry = type->mMemberTable; entry->mName; entry++ )
		{
			if( !entry->mGetProperty && strcmp(entry->mName, name) == 0 )
			{
				return entry;
			}
		}
	}

	return NULL;
}

void TriValueBinding::Initialize()
{
	m_isValid = false;
	m_source = NULL;
	m_destination = NULL;
	m_copyFunc = NULL;

	if( !m_sourceObject || !m_destinationObject )
	{
		// No point doing anything yet - all copy functions need a source and destination object
		return;
	}

	if( !m_copyValueCallable && ( m_sourceAttribute.empty() || m_destinationAttribute.empty() ) )
	{
		// All copy functions except the Python callback require a source and destination attribute
		return;
	}

	const Be::ClassInfo* srcClassInfo = m_sourceObject->ClassType();
	const Be::ClassInfo* dstClassInfo = m_destinationObject->ClassType();

	size_t dataSize = 0;

	m_sourceItemOffset = 0;
	bool sourceFloatArrayAsFloat = false;

	std::string sourceAttr = m_sourceAttribute.c_str();
	size_t sourceDot = sourceAttr.find( '.' );
	if( sourceDot != std::string::npos )
	{
		std::string sourceItem = sourceAttr.substr( sourceDot + 1 );
		if( strcmp( sourceItem.c_str(), "x" ) == 0 )
		{
			sourceFloatArrayAsFloat = true;
			m_sourceItemOffset = 0;
		}
		else if( strcmp( sourceItem.c_str(), "y" ) == 0 )
		{
			sourceFloatArrayAsFloat = true;
			m_sourceItemOffset = sizeof( float );
		}
		else if( strcmp( sourceItem.c_str(), "z" ) == 0 )
		{
			sourceFloatArrayAsFloat = true;
			m_sourceItemOffset = 2 * sizeof( float );
		}
		else if( strcmp( sourceItem.c_str(), "w" ) == 0 )
		{
			sourceFloatArrayAsFloat = true;
			m_sourceItemOffset = 3 * sizeof( float );
		}
		else if( strcmp( sourceItem.c_str(), "r" ) == 0 )
		{
			sourceFloatArrayAsFloat = true;
			m_sourceItemOffset = 0;
		}
		else if( strcmp( sourceItem.c_str(), "g" ) == 0 )
		{
			sourceFloatArrayAsFloat = true;
			m_sourceItemOffset = sizeof( float );
		}
		else if( strcmp( sourceItem.c_str(), "b" ) == 0 )
		{
			sourceFloatArrayAsFloat = true;
			m_sourceItemOffset = 2 * sizeof( float );
		}
		else if( strcmp( sourceItem.c_str(), "a" ) == 0 )
		{
			sourceFloatArrayAsFloat = true;
			m_sourceItemOffset = 3 * sizeof( float );
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
		if( strcmp( destItem.c_str(), "x" ) == 0 )
		{
			destFloatArrayAsFloat = true;
			m_destItemOffset = 0;
		}
		else if( strcmp( destItem.c_str(), "y" ) == 0 )
		{
			destFloatArrayAsFloat = true;
			m_destItemOffset = sizeof( float );
		}
		else if( strcmp( destItem.c_str(), "z" ) == 0 )
		{
			destFloatArrayAsFloat = true;
			m_destItemOffset = 2 * sizeof( float );
		}
		else if( strcmp( destItem.c_str(), "w" ) == 0 )
		{
			destFloatArrayAsFloat = true;
			m_destItemOffset = 3 * sizeof( float );
		}
		else if( strcmp( destItem.c_str(), "r" ) == 0 )
		{
			destFloatArrayAsFloat = true;
			m_destItemOffset = 0;
		}
		else if( strcmp( destItem.c_str(), "g" ) == 0 )
		{
			destFloatArrayAsFloat = true;
			m_destItemOffset = sizeof( float );
		}
		else if( strcmp( destItem.c_str(), "b" ) == 0 )
		{
			destFloatArrayAsFloat = true;
			m_destItemOffset = 2 * sizeof( float );
		}
		else if( strcmp( destItem.c_str(), "a" ) == 0 )
		{
			destFloatArrayAsFloat = true;
			m_destItemOffset = 3 * sizeof( float );
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
			m_source = (void*)BLUEMAPMEMBEROFFSET( m_sourceObject.p, srcEntry, srcClassInfo, srcOffset );
			m_destination = (void*)BLUEMAPMEMBEROFFSET( m_destinationObject.p, dstEntry, dstClassInfo, dstOffset );
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
			m_notifyPtr = BlueCastPtr( m_destinationObject );
		}
	}

	if( m_copyFunc )
	{
		m_isValid = true;
		ITriReroutablePtr rp( BlueCastPtr( m_destinationObject ) );
		if( rp )
		{
			rp->RegisterBinding( this );
			void* dest = NULL;
			size_t size = 0;
			rp->GetDestination( dest, size );
			if( size >= dataSize )
			{
				m_destination = (void*)((uint8_t*)dest + m_destItemOffset);
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
	m_destination = (void*)((uint8_t*)dest + m_destItemOffset);
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
		switch(srcEntry->mType)
		{
		case Be::BYTE:
		case Be::BOOL:
			m_copyFunc = Copy8Bit;
			dataSize = 1;
			break;

		case Be::SHORT:
			m_copyFunc = Copy16Bit;
			dataSize = 2;
			break;

		case Be::LONG:
			m_copyFunc = Copy32Bit;
			dataSize = 4;
			break;

		case Be::FLOAT:
			m_copyFunc = Copy32BitFloat;
			dataSize = 4;
			break;

		case Be::DOUBLE:
			m_copyFunc = Copy64BitFloat;
			dataSize = 8;
			break;

		case Be::INT64:
			m_copyFunc = CopyInt64;
			dataSize = 8;
			break;

		case Be::FLOATARRAY:
			if( sourceFloatArrayAsFloat )
			{
				if( destFloatArrayAsFloat )
				{
					m_copyFunc = Copy32BitFloat;
					m_source = (void*)((uint8_t*)m_source + m_sourceItemOffset);
					m_destination = (void*)((uint8_t*)m_destination + m_destItemOffset);
					dataSize = 4;
				}
				else if( dstEntry->GetFloatArraySize() == 3 )
				{
					m_source = (void*)((uint8_t*)m_source + m_sourceItemOffset);
					m_copyFunc = Copy32BitFloatToVector3;
					dataSize = 12;
				}
				else if( dstEntry->GetFloatArraySize() == 4 )
				{
					m_source = (void*)((uint8_t*)m_source + m_sourceItemOffset);
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
						m_copyFunc = CopyVector2;
						dataSize = 12;
						break;
					case 3:
						m_copyFunc = CopyVector3;
						dataSize = 12;
						break;
					case 4:
						m_copyFunc = CopyVector4;
						dataSize = 16;
						break;
					case 16:
						m_copyFunc = CopyMatrix;
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
						CCP_LOGWARN( "TriValueBinding: src float array size is bigger than dst, and not a matrix" );
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
	else if( sourceFloatArrayAsFloat && srcEntry->mType == Be::FLOATARRAY && dstEntry->mType == Be::FLOAT  )
	{
		m_copyFunc = Copy32BitFloat;
		m_source = (void*)((uint8_t*)m_source + m_sourceItemOffset);
		dataSize = 4;
	}
	else if( sourceFloatArrayAsFloat && srcEntry->mType == Be::FLOATARRAY && dstEntry->mType == Be::DOUBLE  )
	{
		m_copyFunc = Copy32BitFloatToDouble;
		m_source = (void*)((uint8_t*)m_source + m_sourceItemOffset);
		dataSize = 1;
	}
	else if( srcEntry->mType == Be::FLOAT && dstEntry->mType == Be::FLOATARRAY  )
	{
		if( destFloatArrayAsFloat )
		{
			m_copyFunc = Copy32BitFloat;
			m_destination = (void*)((uint8_t*)m_destination + m_destItemOffset);
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
		m_copyFunc = Copy32BitFloatToBool;
		dataSize = 1;
	}
	else if( srcEntry->mType == Be::FLOAT && dstEntry->mType == Be::DOUBLE )
	{
		m_copyFunc = Copy32BitFloatToDouble;
		dataSize = 1;
	}
	else if( srcEntry->mType == Be::DOUBLE && dstEntry->mType == Be::FLOAT )
	{
		m_copyFunc = CopyDoubleTo32BitFloat;
		dataSize = 1;
	}
	else if( srcEntry->mType == Be::IROOT )
	{
		ITriVectorPtr vp;
		if( ((IRoot*)m_source)->QueryInterface( BlueInterfaceIID<ITriVector>(), (void**)&vp, BEQI_SILENT ) )
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
					m_copyFunc = CopyTriVectorToVector4;
					dataSize = 16;
				}
			}
		}

		ITriColorPtr cp;
		if( ((IRoot*)m_source)->QueryInterface( BlueInterfaceIID<ITriColor>(), (void**)&cp, BEQI_SILENT ) )
		{
			if( dstEntry->mType == Be::FLOATARRAY && dstEntry->GetFloatArraySize() >= 4 )
			{
				m_copyFunc = CopyTriColorToFloatArray;
				dataSize = 16;
			}
		}

		ITriQuaternionPtr qp;
		if( ((IRoot*)m_source)->QueryInterface( BlueInterfaceIID<ITriQuaternion>(), (void**)&qp, BEQI_SILENT ) )
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
		if( ((IRoot*)m_destination)->QueryInterface( BlueInterfaceIID<ITriMatrix>(), (void**)&mp, BEQI_SILENT ) )
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
		if( ((IRoot*)m_destination)->QueryInterface( BlueInterfaceIID<ITriVector>(), (void**)&vp, BEQI_SILENT ) )
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
					m_copyFunc = CopyVector4ToTriVector;
					dataSize = 12;
				}
			}
		}

		ITriColorPtr cp;
		if( ((IRoot*)m_destination)->QueryInterface( BlueInterfaceIID<ITriColor>(), (void**)&cp, BEQI_SILENT ) )
		{
			if( srcEntry->mType == Be::FLOATARRAY && srcEntry->GetFloatArraySize() == 4 )
			{
				m_copyFunc = CopyFloatArrayToTriColor;
				dataSize = 16;
			}
		}

		ITriQuaternionPtr qp;
		if( ((IRoot*)m_destination)->QueryInterface( BlueInterfaceIID<ITriQuaternion>(), (void**)&qp, BEQI_SILENT ) )
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
}

void TriValueBinding::SetDestination( const std::string& destinationAttribute, IRootPtr destinationObject )
{
	m_destinationAttribute = destinationAttribute;
	m_destinationObject = destinationObject;
}

bool TriValueBinding::IsValid() const
{
	return m_isValid;
}

void TriValueBinding::SetScale( float scale )
{
	m_scale = scale;
}