#include "stdafx.h"
#include "EffectData.h"

bool SaveEffectData( EffectData& data, ID3DXBuffer** buffer )
{
	SizeCountStream size;
	data.Save( size );

	if( FAILED( D3DXCreateBuffer( size.GetSize(), buffer ) ) )
	{
		return false;
	}

	PackedStream stream( ( *buffer )->GetBufferPointer(), size.GetSize() );
	data.Save( stream );

	//BYTE* bufferStart = reinterpret_cast<BYTE*>( (*buffer)->GetBufferPointer() );
	//BYTE* bufferEnd = bufferStart;
	//data.Save( bufferEnd );
	//if( bufferEnd - bufferStart != size.GetSize() )
	//{
	//	return false;
	//}
	return true;
}