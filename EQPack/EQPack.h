#include <windows.h>
#include <string.h>
#include <conio.h>
#include <direct.h>
#include <errno.h>
#include "../EQCommon/S3DPackage.h"
#include "../EQCommon/WLDFile.h"
#include "../EQCommon/Utils.h"

void DisplayUsage();

HRESULT UnpackS3D(char* Filename);
HRESULT UnpackWLD(char* Filename);
