#pragma once
#ifndef StringTable_H
#define StringTable_H

typedef unsigned StringReference;

class StringTable
{
public:
	StringTable();
	~StringTable();

	StringReference AddString( const char* string );
	const char* GetString( StringReference ref );
	bool Write( HANDLE file ) const;
	size_t GetSize() const;
private:
	CRITICAL_SECTION m_CS;
	std::map<std::string, StringReference> m_table;
	size_t m_size;
};

#endif // StringTable_H