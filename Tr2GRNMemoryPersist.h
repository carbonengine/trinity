#pragma once
#ifndef Tr2GRNMemoryPersist_h
#define Tr2GRNMemoryPersist_h

struct Tr2GRNMemoryPersist
{
	granny_int32x m_bufferSize;
	granny_int32  m_blockSize;
	granny_uint8* m_buffer;

	Tr2GRNMemoryPersist() : m_buffer( NULL ), m_bufferSize( 0 ), m_blockSize( 1024 ){}
	bool ConvertGrannyTypeToContiguousBuffer( granny_data_type_definition* grannyType, void* object );

	static void* RebaseBuffer( granny_data_type_definition* grannyType, void* object );
	static void ReleaseWriteBuffer( granny_uint8* buffer );
};

#endif //Tr2GRNMemoryPersist_h