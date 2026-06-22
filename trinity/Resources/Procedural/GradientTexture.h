// Copyright © 2025 CCP ehf.

#pragma once

void RasterizeGradient( const std::string_view& path, ImageIO::HostBitmap& bitmap );
bool IsGradientTexturePath( const wchar_t* path );