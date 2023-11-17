#pragma once

struct Macro;

void PrintOutOfDateFiles( size_t workerCount );
std::string GetSourceHash( const char* sourcePath, const std::vector<Macro>& defines );