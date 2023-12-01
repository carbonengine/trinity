#include "StdAfx.h"
#include "Tr2GRNMemoryPersist.h"


// --------------------------------------------------------------------------------------
// Description:
//		Re-bases all the internal pointers relative to the object. 
//		Needed for when loading a converted block from disk to make the pointers valid.
// Arguments:
//		granny_data_type_definition*	- The granny type describing the object
//		void*							- The object defined by the granny type
// Return Value:
//		The object with all the pointers re-based. NULL on failure.
// --------------------------------------------------------------------------------------	
void* Tr2GRNMemoryPersist::RebaseBuffer( granny_data_type_definition* grannyType, void* object )
{
	if( GrannyRebasePointers( grannyType, object, (granny_intaddrx)object, true ) )
	{
		return object;
	}
	else
	{
		CCP_LOGERR("Tr2GRNMemoryPersist::RebaseBuffer Failed");
		return NULL;
	}
}

// --------------------------------------------------------------------------------------
// Description:
//		Converts an object that has a granny type definition to a contiguous memory block.
//		Callers are responsible for cleaning up the memory when they are done with it.
//		
// Arguments:
//		granny_data_type_definition*	- The granny type describing the object
//		void*							- The object defined by the granny type
// Return Value:
//		true, if we were able to convert the data to the memory block
//		false, otherwise
// --------------------------------------------------------------------------------------
bool Tr2GRNMemoryPersist::ConvertGrannyTypeToContiguousBuffer( granny_data_type_definition* grannyType, void* object )
{
	if( object == NULL )
	{
		CCP_LOGWARN("Tr2GRNMemoryPersist::ConvertGrannyTypeToContiguousBuffer Can't convert a NULL pointer!");
		return false;
	}

	// We want to write the data to memory
	granny_file_builder *builder = GrannyBeginFileInMemory( 1, 
		GrannyCurrentGRNStandardTag,
		GrannyGRNFileMV_ThisPlatform, 
		m_blockSize );

	// Write out our structure to the data tree and write it to the builder
	granny_file_data_tree_writer *writer = GrannyBeginFileDataTreeWriting( grannyType, object, 0, 0 );
	if( !GrannyWriteDataTreeToFileBuilder( writer, builder ) )
	{
		CCP_LOGERR("Tr2GRNMemoryPersist::ConvertGrannyTypeToContiguousBuffer Failed to write data tree to file builder!");
		return false;
	}
	GrannyEndFileDataTreeWriting( writer );

	// The internal buffer is a flat expandable array, so if the block size is not enough it will fix it for us
	granny_file_writer *memoryWriter = GrannyCreateMemoryFileWriter( m_blockSize );
	// Use the type data from the builder to write our object to a contiguous block in memory
	if( !GrannyEndFileRawToWriter( builder, memoryWriter ) )
	{
		CCP_LOGERR("Tr2GRNMemoryPersist::ConvertGrannyTypeToContiguousBuffer Failed to move raw data to writer!");
		return false;
	}

	// Get the block we just wrote out
	if( !GrannyStealMemoryWriterBuffer( memoryWriter, &m_buffer, &m_bufferSize ) )
	{
		CCP_LOGERR("Tr2GRNMemoryPersist::ConvertGrannyTypeToContiguousBuffer Failed to steal memory buffer!");
		return false;
	}

	return true; // We managed
}

// --------------------------------------------------------------------------------------
// Description:
//		Release the memory writer buffer allocated by ConvertGrannyTypeToContiguousBuffer.
//		
// Arguments:
//		granny_uint8*	- The write buffer
// --------------------------------------------------------------------------------------
void Tr2GRNMemoryPersist::ReleaseWriteBuffer( granny_uint8* buffer )
{
	GrannyFreeMemoryWriterBuffer( buffer );
}
