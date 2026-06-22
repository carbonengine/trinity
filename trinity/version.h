// Copyright © 2023 CCP ehf.

#include "windows.h"

#include "CcpCore/include/CcpMacros.h"

#define EVEFILEDESC "CCP Trinity 3D Engine\0"
#define EVEINTFILENAME CCP_STRINGIZE( TARGET_NAME )
#define EVEFILENAME CCP_STRINGIZE( TARGET_FILENAME )
#define EVEFILETYPE VFT_DLL

#include "autoversion.h"
//standard file version thing
#include "../version/evebuildver.h"
