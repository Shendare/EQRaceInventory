// EQRaceInventory - Inventories race model stats and race availability by zone

#include <windows.h>
#include <string.h>
#include <conio.h>
#include "../EQCommon/Utils.h"
#include "../EQCommon/S3DPackage.h"
#include "../EQCommon/WLDFile.h"
#include "../EQCommon/EQInfo.h"

char EQClientFile[] = "eqgame.exe";

char sDBStringFile[] = "dbstr_us.txt";
char sStringListFile[] = "eqstr_us.txt";

char sGlobalLoadFile[] = "Resources\\GlobalLoad.txt";

char sOutFile_ZoneDB[] = "EQZoneInfo_DB.txt";
char sOutFile_Zones[] = "EQZones.htm";
char sOutFile_Codes[] = "EQRaces_ModelCodes_DB.txt";
char sOutFile_Stats[] = "EQRaces_Appearance_DB.txt";
char sOutFile_Races[] = "EQRaces.htm";
char sOutFile_AvailD[] = "EQRaces_Availability_DB.txt";
char sOutFile_AvailZ[] = "EQRaces_Availability_ByZone.txt";
char sOutFile_AvailR[] = "EQRaces_Availability_ByRace.txt";

char sLogFile[] = "EQRI.log";

char sINIFile[] = "EQRI.ini";

char sWorkPathDefault[] = ".";
char sOutPathDefault[] = "EQRI";

char sWorkPath[_MAX_PATH];
char sOutPath[_MAX_PATH];

LogFile*  Log;

DWORD     iProcessedFiles;
QWORD     iProcessedBytes;

HRESULT   LoadSettings();

HRESULT   ScanFiles(char* Pattern);

HRESULT   LoadGlobalModels();
HRESULT   LoadZoneModels(EQZone* Zone);
HRESULT   LoadModels_S3D(EQSource* Source);
HRESULT   LoadModels_EQG(EQSource* Source);
HRESULT   LoadZoneModels_TXT(EQZone* Zone);
HRESULT   LoadOrphanModels();

HRESULT   ExportRaceInfo();
HRESULT   ExportZoneInfo();
HRESULT   ExportModelVariations2(FILE* File, BYTE Count, BYTE* Variations);
HRESULT   ExportModelVariations(FILE* File, EQModel* Model);
HRESULT   ExportZoneRaceAvailability();

HRESULT   LogOpen();

const BYTE RACE_NO = 0;
const BYTE SEX_MALE = 1;
const BYTE SEX_FEMALE = 2;
const BYTE SEX_NEUTRAL = 3;

const WORD MAX_RACE = 2999;
const WORD MAX_ZONE = 999;

BYTE      Zone_HasRace[MAX_ZONE][MAX_RACE];

int       Quit(int ReturnCode);

void      ProcessedFile(DWORD FileSize);
void      ProcessedFile(char* FilePath);
