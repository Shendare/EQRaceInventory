// EQInfo - EverQuest Information Handling

#pragma once

#include <windows.h>
#include <stdio.h>
#include <io.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unordered_map>
#include "../EQCommon/Utils.h"

// We need to set some reasonable limit on filename lengths for memory allocation purposes
#define EQ_MAXFILENAMELEN               63

// This shouldn't be changing any time soon.
#define EQRACE_GENDERCOUNT              3

// NOTE: This is so high because there are apparently several RaceID codes
//       from 2253-2259 used internally at SOE for debugging purposes.
//
// Highest as of RoF: 2259 (actual used are up to 732)
#define EQRACE_MAXID                    2500

#define EQRACE_INVALID                  0

// Maximum number of model codes that can start with each letter of the alphabet (+ a nonalpha)
//
// Highest in use as of RoF: 94.
#define EQMODEL_MAXCODESPERLETTER       120

// Maximum number of model definitions allowed (200 bytes each, 3500 = 800kb)
// In use as of RoF: 3649
#define EQMODEL_MAXDEFS                 5000

// Maximum number of model sources allowed (in use as of RoF: 734)
#define EQMODEL_MAXSOURCES              1200

// Highest in use as of RoF is 10 (MINIPOM200). Vast majority are 3 characters long.
#define EQMODEL_MAXCODELEN              13

// Allow up to this many source files (x_chr.s3d, x_chr2.s3d, x.eqg, x_chr.eqg) starting with
// each letter of the alphabet (+ nonalpha).  BYTE value, so max max is 255
// Highest as of SoF is 114
#define EQMODEL_MAXSOURCESPERLETTER     150

// Highest as of RoF is 130
#define EQSOURCE_MAXMODELDEFS           200

#define EQSOURCE_INVALID                0

// Maximum number of variations of these types per model code per source
#define EQMODEL_MAXVARIATIONS_TEXTURE   30 // Highest as of RoF is 23
#define EQMODEL_MAXVARIATIONS_HEAD      20 // Highest as of RoF is 15
#define EQMODEL_MAXVARIATIONS_FACE      15 // Highest as of RoF is 9
#define EQMODEL_MAXVARIATIONS_TATTOO    15 // Highest as of RoF is 8
#define EQMODEL_MAXVARIATIONS_HAIR      15 // Highest as of RoF is 8
#define EQMODEL_MAXVARIATIONS_BEARD     15 // Highest as of RoF is 11
#define EQMODEL_MAXVARIATIONS_DETAIL    15 // Highest as of RoF is 7

#define EQSOURCE_CODEINVALID            0
#define EQSOURCE_CODENONALPHA           1
#define EQSOURCE_CODEGLOBAL             28
#define EQSOURCE_CODEORPHAN             29

// Leaving plenty of room for expansion
// Highest as of RoF is 999
#define EQZONE_MAXID                    1500

// Maximum number of zone nicks that can start with each letter of the alphabet (+ a nonalpha)
// Highest in use as of RoF is 52
#define EQZONE_MAXNICKSPERLETTER        70

// Max models imported via ZoneNick_chr.txt per zone (2 bytes each, 200 = 400 bytes per zone)
// Highest in use as of RoF is 45
#define EQZONE_MAXIMPORTMODELS          80

#define EQZONE_INVALID                  0xFFFF

#define EQDBSTR_INVALID                 0xFFFF

// String List ID Index size in memory will be 2 bytes times this number (65535 = 131070)
// 65535 is the maximum maximum without upgrading to DWORDs at double the size
// Highest as of RoF was 35153
#define EQSTRLIST_MAXID                 65000

// Total number of String List strings to load.
// Total String List Index size will be 4 bytes times this number (30000 = 120KB)
// Highest as of RoF is 7135
#define EQSTRLIST_MAXCOUNT              9999

#define EQSTRLIST_INVALID               0xFFFF

// Maximum number of values allowed on the CPU interpreter's stack. (Don't worry about it.)
#define EQCPU_MAXSTACKSIZE              16

typedef  unsigned __int16 SourceID;

enum EQRACEVARTYPE:BYTE
{
  EQRV_NONE = 0,
  EQRV_TEXTURE = 1,
  EQRV_HEAD = 2,
  EQRV_FACE = 3,
  EQRV_HAIR = 4,
  EQRV_HAIRCOLOR = 5,
  EQRV_BEARD = 6,
  EQRV_TATTOO = 7,
  EQRV_DETAIL = 8,
  EQRV_MAX = 8
};

enum EQGENDER:BYTE
{
  EQRG_INVALID = 0,
  EQRG_MALE = 1,
  EQRG_FEMALE = 2,
  EQRG_NEUTRAL = 3
};

enum EQGLOBAL:BYTE
{
  EQSA_LOCAL = 0,           // Local to specific zones (via ZoneName_chr[2].s3d or ZoneName_chr.txt)
  EQSA_GLOBAL_HARDCODE = 1, // Global, hardcoded into eqgame.exe
  EQSA_GLOBAL_TEXTFILE = 2, // Global, referenced in Resources\GlobalLoad.txt
  EQSA_GLOBAL_LUCLIN = 3,   // Global, only loaded when Luclin models are enabled
  EQSA_GLOBAL_NONLUCLIN = 4 // Global, only loaded when Luclin models are NOT enabled (old style models)
};

static const char* EQGlobalName[] =
{
  "Local to zone",
  "Hard-coded",
  "Loaded via GlobalLoad.txt",
  "Loaded with Luclin models",
  "Loaded when Luclin models disabled"
};

enum EQDBSTRTYPE:BYTE
{
  EQDB_INVALID = 0,
  EQDB_AANAME = 1,
  EQDB_SPELLNAME1 = 2,      // First Half
  EQDB_SPELLNAME2 = 3,      // Second Half
  EQDB_AADESC = 4,
  EQDB_SPELLTYPE = 5,
  EQDB_SPELLDESC = 6,
  EQDB_SPECIALITEMTYPE = 7, // Only used by a few items so far
  EQDB_RACEDESC = 8,
  EQDB_CLASSDESC = 9,
  EQDB_STATDESC = 10,
  EQDB_RACENAME = 11,
  EQDB_RACEPLURAL = 12,
  EQDB_CLASSPLURAL = 13,
  EQDB_DEITYDESC = 14,
  EQDB_STARTZONEDESC = 15,
  EQDB_AUGTYPE = 16,
  EQDB_SPECIALCURRENCY = 17,
  EQDB_SPECIALCURRENCYPLURAL = 18,
  EQDB_SKILLTYPES = 19,
  EQDB_EXPANSION = 20
};

enum EQCLIENTVER:BYTE
{
  EQC_INVALID = 0,
  EQC_TITANIUM = 1,
  EQC_SOF = 2,
  EQC_ROF2 = 3,
  EQC_MAXSUPPORTED = 2
};

struct EQModel;
struct EQRace;
struct EQSource;
struct EQZone;

class EQ;
class EQClient;
class EQDBStrings;
class EQStringList;

struct EQClientDef
{
  BYTE   VersionNum;            // Internal version number, see enum EQCLIENTVER
  char*  VersionNick;           // Nick to give this version, for file naming purposes
  char*  VersionName;           // Name to give this version, just for our purposes
  
  DWORD  FileSize;              // Size of eqgame.exe in bytes

  DWORD  RaceBuildCodeStart;    // Where in eqgame.exe to start processing the Race Info machine code
  DWORD  RaceBuildCodeEnd;      // Where in eqgame.exe to stop processing the Race Info machine code
  DWORD  RaceBuildCodeCall;     // The machine code call offset that signifies a Race Info building call

  DWORD  ZoneBuildCodeStart;    // Where in eqgame.exe to start processing the Zone Info machine code
  DWORD  ZoneBuildCodeEnd;      // Where in eqgame.exe to stop processing the Zone Info machine code
  DWORD  ZoneBuildCodeCall;     // The machine code call offset that signifies a Zone Info building call
  DWORD  ZoneBuildCodeCall2;    // The OTHER machine code call offset that signifies a Zone Info building call (RoF2)
};

// This information will not change for any given client version.
// Only new clients may be added.
static const EQClientDef ClientSpecs[] = {
  {
    1,                    // VersionNum
    "Titanium",           // VersionNick
    "Titanium",           // VersionName
    3981312,              // FileSize
    0x08C8CB,             // RaceBuildCodeStart
    0x08F006,             // RaceBuildCodeEnd
    0x08C83A,             // RaceBuildCodeCall
    0x1F7000,             // ZoneBuildCodeStart
    0x1FA1B2,             // ZoneBuildCodeEnd
    0x1F6AFC, 0            // ZoneBuildCodeCall
  },

  {
    2,                    // VersionNum
    "SoF",                // VersionNick
    "Secrets of Faydwer", // VersionName
    3731456,              // FileSize
    0x0B4EAA,             // RaceBuildCodeStart
    0x0B89F2,             // RaceBuildCodeEnd
    0x0B4CFC,             // RaceBuildCodeCall
    0x22A3E0,             // ZoneBuildCodeStart
    0x22E189,             // ZoneBuildCodeEnd
    0x229F9C, 0           // ZoneBuildCodeCall
  },
  {
    3,                    // VersionNum
    "RoF2",               // VersionNick
    "Rain of Fear (v2)",  // VersionName
    8774656,              // FileSize
    0x109A0A,             // RaceBuildCodeStart
    0x10E315,             // RaceBuildCodeEnd
    0x10983C,             // RaceBuildCodeCall
    0x3DBEB9,             // ZoneBuildCodeStart
    0x3E2F50,             // ZoneBuildCodeEnd
    // 0x3E0618,             // ZoneBuildCodeEnd
    0x3DB68C, 0x3DB82C    // ZoneBuildCodeCall
  }
  
    /*
    _oZone.Expansion    = (BYTE)_oCPU.stack[8];
  _oZone.ID           = (WORD)_oCPU.stack[7];
  _oZone.Nick         = (char*)_oCPU.stack[6];
  _oZone.Name         = (char*)_oCPU.stack[5];
  _oZone.DBStrNameID  = _oCPU.stack[4];
  _oZone.Flags        = _oCPU.stack[3];
  _oZone.Unknown7     = _oCPU.stack[2];
  _oZone.Unknown8     = _oCPU.stack[1];
  _oZone.MinLevel     = (BYTE)_oCPU.stack[0];

  */
};

struct CPUStatus
{
  DWORD eax;
  DWORD ebx;
  DWORD ecx;
  DWORD edx;
  DWORD esi;
  DWORD edi;
  DWORD ebp;

  DWORD stack[EQCPU_MAXSTACKSIZE];
  DWORD spos;

  DWORD  filepos;
  DWORD  fileend;
  DWORD  buildcall;
  DWORD  buildcall2;
};

struct EQRace
{
  char* GetName();
  char* GetDesc();
  char* GetPlural();
  
  bool  IsPlayable();
  bool  IsGlobal(EQModel** Model);

  WORD  ID;
  char* ModelCode[EQRACE_GENDERCOUNT];
  BYTE  ModelCodeLen[EQRACE_GENDERCOUNT];
};

struct EQZone
{
  HRESULT   AddImportModel(EQModel* Model);
  
  char*     GetDBStrName();
  char*     GetStartDesc();
  
  bool      HasSource(EQSource* Source);
  bool      HasModel(WORD RaceID, EQGENDER Gender, EQModel** ModelDef);
  bool      HasRace(WORD RaceID);
  bool      IsImport(EQModel* Model);

  WORD      ID;
  char*     Nick;
  char*     Name;
  BYTE      Expansion;
  DWORD     DBStrNameID;
  DWORD     Flags;
  DWORD     Unknown7;
  DWORD     Unknown8;
  BYTE      MinLevel;
  EQSource* ModelSource1;
  EQSource* ModelSource2;
  BYTE      ImportModels;
  EQModel*  ImportModel[EQZONE_MAXIMPORTMODELS];
};

struct EQSource
{
  bool      HasRace(WORD RaceID);
  bool      HasModel(WORD RaceID, EQGENDER Gender, EQModel** ModelDef);
  EQModel*  AddModel(char* ModelCode);

  SourceID  ID; //2
  BYTE      FilenameLen; //1
  char      Filename[EQ_MAXFILENAMELEN + 1]; //64
  BYTE      Loaded; // 1
  EQGLOBAL  Global; //1
  BYTE      Models; //1
  EQModel*  Model[EQSOURCE_MAXMODELDEFS]; //4*63 = 252
  BYTE      Reserved[2]; // Pad to 320 bytes (QWORD align)
};

struct EQModel
{
  HRESULT   AddVariation(BYTE VariationType, BYTE Variation);
  HRESULT   AddVariations(BYTE VariationType, BYTE MaxVariation);
  static bool IsGlobal(WORD RaceID, EQGENDER Gender, EQModel** Model);

  BYTE      CodeLen; //1
  char      Code[EQMODEL_MAXCODELEN + 1]; //11
  WORD      RaceID; //2
  BYTE      Gender; //1
  EQSource* Source; //4
  BYTE      IndexInSource; // 1
  BYTE      VarsTexture; //1
  BYTE      VarTexture[EQMODEL_MAXVARIATIONS_TEXTURE]; //16
  BYTE      VarsHead; //1
  BYTE      VarHead[EQMODEL_MAXVARIATIONS_HEAD]; //16
  BYTE      VarsFace; //1
  BYTE      VarFace[EQMODEL_MAXVARIATIONS_FACE]; //16
  BYTE      VarsTattoo; //1
  BYTE      VarTattoo[EQMODEL_MAXVARIATIONS_TATTOO]; // 16
  BYTE      VarsHair;
  BYTE      VarHair[EQMODEL_MAXVARIATIONS_HAIR];
  BYTE      VarsBeard;
  BYTE      VarBeard[EQMODEL_MAXVARIATIONS_BEARD];
  BYTE      VarsDetail;
  BYTE      VarDetail[EQMODEL_MAXVARIATIONS_DETAIL];
};

class EQDBStrings
{
public:
  ~EQDBStrings(void);
  
  HRESULT Load(char* Filename);
  char*   GetText(EQDBSTRTYPE TypeID, DWORD StringID);

  WORD    Count;
  DWORD   MaxID;
  BYTE    MaxType;

private:
  void    _Close();
  DWORD   iDBString_Size;
  char*   aDBString_Bytes;

  std::unordered_map <DWORD, DWORD> sDBString;
};

class EQStringList
{
public:
  ~EQStringList(void);

  HRESULT Load(char* Filename);
  char*   GetText(WORD StringID);

  WORD    Count;
  WORD    MaxID;

private:
  void    _Close();
  char*   aStringList_Bytes;
  WORD    iStringList_Line[EQSTRLIST_MAXID];
  char*   aStringList_Text[EQSTRLIST_MAXCOUNT];
};

class EQClient
{
public:
  ~EQClient(void);
  
  static EQCLIENTVER GetVersionOf(char* Filename);

  HRESULT     GetRaceFromModelCode(char* ModelCode, WORD* RaceID, EQGENDER* Gender);
  EQZone*     GetZone(WORD ZoneID);
  EQZone*     GetZone(char* ZoneNick);
  char*       GetModelCode(WORD RaceID, EQGENDER Gender);
  
  HRESULT     Load(char* ClientEXE);
  HRESULT     LoadRaces();
  HRESULT     LoadZones();

  WORD        Races;
  WORD        Race_MaxID;

  EQRace      Race[EQRACE_MAXID + 1];
  EQZone      Zone[EQZONE_MAXID + 1];

  WORD        Zones;
  WORD        Zone_MaxID;

  size_t      FileSize;              // Size of eqgame.exe in bytes

  WORD        Models;

  EQCLIENTVER VersionNum;            // Internal version number, see enum EQCLIENTVER
  char*       VersionNick;
  char*       VersionName;           // Name to give this version, just for our purposes
  
  WORD        Race_ModelCodeCount[27];                              // Number of Race_ModelCodeCheck entries for each letter
  WORD        Race_ModelCodeCheck[27][EQMODEL_MAXCODESPERLETTER];   // Up to X model codes per letter of the alphabet (+ nonalpha)
  
  WORD        Zone_NickCount[27];                                // Number of aZoneNick entries for each letter
  WORD        Zone_NickCheck[27][EQZONE_MAXNICKSPERLETTER];      // Up to X zone nicks per letter of the alphabet (+ nonalpha)

private:
  void        _Close();
  bool        _FindNextBuildCall(CPUStatus* cpu);
  
  DWORD       iRaceBuildCodeStart;    // Where in eqgame.exe to start processing the Race Info machine code
  DWORD       iRaceBuildCodeEnd;      // Where in eqgame.exe to stop processing the Race Info machine code
  DWORD       iRaceBuildCodeCall;     // The machine code call offset that signifies a Race Info building call

  DWORD       iZoneBuildCodeStart;    // Where in eqgame.exe to start processing the Zone Info machine code
  DWORD       iZoneBuildCodeEnd;      // Where in eqgame.exe to stop processing the Zone Info machine code
  DWORD       iZoneBuildCodeCall;     // The machine code call offset that signifies a Zone Info building call
  DWORD       iZoneBuildCodeCall2;     // The machine code call offset that signifies a Zone Info building call

  char        sFilePath[_MAX_PATH];
  BYTE*       aBytes;
  DWORD       iRDataFileOffset;     // Where in eqgame.exe the .rdata section begins
  QWORD       iRDataMemoryOffset;   // Where in memory the .rdata section gets mapped to
  QWORD       iRDataMemoryEnd;      // Where in memory the .rdata section ends
};

class EQ
{
public:
  static EQClient     Client;
  static EQDBStrings  DBStrings;
  static EQStringList StringList;

  static char*        GetExpansionName(BYTE ExpansionID);

  static bool         IsPlayableRace(WORD RaceID);
  
  static EQSource*    GetSource(SourceID ID);
  static EQSource*    GetSource(char* Filename);
  static EQSource*    AddSource(char* Filename, EQGLOBAL Global, bool Orphan);

  static BYTE         GetGlobalSources();
  static EQSource*    GetGlobalSource(BYTE Index);

  static BYTE         GetOrphanSources();
  static EQSource*    GetOrphanSource(BYTE Index);

  static EQModel*     AddModel();

  static BYTE         GetSourceCat(char* Filename);
  static SourceID     MakeSourceID(BYTE Code, BYTE Index);

  static WORD         ModelDefs;
  static EQModel      ModelDef[EQMODEL_MAXDEFS];
  
  static WORD         ModelSources;
  static EQSource     ModelSource[EQMODEL_MAXSOURCES];

  static BYTE         Index_Sources[29];
  static WORD         Index_Source[29][EQMODEL_MAXSOURCESPERLETTER];

  static void         LogMaximums(LogFile* Log);
};
