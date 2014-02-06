#include "stdafx.h"
#include "EffectData.h"

bool SaveEffectData( EffectData& data, ID3DXBuffer** buffer )
{
	size_t size = data.GetPackedSize();
	if( FAILED( D3DXCreateBuffer( size, buffer ) ) )
	{
		return false;
	}
	BYTE* bufferStart = reinterpret_cast<BYTE*>( (*buffer)->GetBufferPointer() );
	BYTE* bufferEnd = bufferStart;
	data.Save( bufferEnd );
	if( bufferEnd - bufferStart != size )
	{
		return false;
	}
	return true;
}