// EQ - EverQuest Information Handling

#include "EQInfo.h"

// ---------------------
// EQ Static Members
// ---------------------

EQClient     EQ::Client;
EQDBStrings  EQ::DBStrings;
EQStringList EQ::StringList;

WORD         EQ::ModelDefs;
EQModel      EQ::ModelDef[EQMODEL_MAXDEFS];
  
WORD         EQ::ModelSources;
EQSource     EQ::ModelSource[EQMODEL_MAXSOURCES];

BYTE         EQ::Index_Sources[29];
WORD         EQ::Index_Source[29][EQMODEL_MAXSOURCESPERLETTER];

// ---------------------
// EQ Member Functions
// ---------------------

EQModel* EQ::AddModel()
{
  if (EQ::ModelDefs >= EQMODEL_MAXDEFS)
  {
    return NULL;
  }

  return &EQ::ModelDef[EQ::ModelDefs++];
}

EQSource* EQ::AddSource(char* Filename, EQGLOBAL Global, bool Orphan)
{
  if (IsBlank(Filename))
  {
    return NULL;
  }

  if ((Global < EQSA_LOCAL) || (Global > EQSA_GLOBAL_NONLUCLIN))
  {
    return NULL;
  }

  if ((Global != EQSA_LOCAL) && Orphan)
  {
    // Can't be a global source as well as an orphan one!
    
    return NULL;
  }

  size_t _iFilenameLen = strlen(Filename);

  if ((_iFilenameLen < 5) || (_iFilenameLen > EQ_MAXFILENAMELEN))
  {
    // Passing a bad filename! Minimum would be x.eqg!
    
    BreakPoint;
    
    return NULL;
  }

  char _sFilename[EQ_MAXFILENAMELEN + 1];

  memcpy_s(_sFilename, sizeof(_sFilename), Filename, _iFilenameLen + 1);
  TrimString(_sFilename, &_iFilenameLen);

  if (IsBlank(_sFilename))
  {
    return NULL;
  }

  _iFilenameLen++; // From now on, take the null terminator into account
  _strlwr_s(_sFilename, _iFilenameLen);

  BYTE _sCode;
  if (Global != EQSA_LOCAL)
  {
    _sCode = EQSOURCE_CODEGLOBAL;
  }
  else if (Orphan)
  {
    _sCode = EQSOURCE_CODEORPHAN;
  }
  else
  {
    _sCode = EQ::GetSourceCat(_sFilename);
  }
  
  WORD      _iMax = EQ::Index_Sources[_sCode - 1];
  WORD      _i;
  SourceID  _iSource;
  EQSource* _oSource;

  for (_i = 0; _i < _iMax; _i++)
  {
    _oSource = &EQ::ModelSource[EQ::Index_Source[_sCode - 1][_i]];

    if (StringSame(_oSource->Filename, _oSource->FilenameLen, _sFilename, _iFilenameLen - 1, false))
    {
      // Duplicate. Return the previously entered one instead of adding it again.
      
      return _oSource;
    }
  }

  // Doesn't already exist. Add it and return the code!

  if (EQ::Index_Sources[_sCode - 1] >= EQMODEL_MAXSOURCESPERLETTER)
  {
    BreakPoint;
    
    // Crud! Too many sources for this letter. This should never happen!
    return NULL;
  }

  if (EQ::ModelSources >= EQMODEL_MAXSOURCES)
  {
    BreakPoint;
    
    // Whoa! Too many sources total in use. This should never happen!
    return NULL;
  }

  _oSource = &EQ::ModelSource[EQ::ModelSources];

  _oSource->FilenameLen = _iFilenameLen - 1;
  memcpy_s( _oSource->Filename,
            sizeof(_oSource->Filename),
            _sFilename,
            _iFilenameLen);

  // Add the reference to our indexes.
  _i = EQ::Index_Sources[_sCode - 1]++;
  
  _iSource = EQ::MakeSourceID(_sCode, (BYTE)_i);
  
  _oSource->Global = Global;
  _oSource->ID = _iSource;

  EQ::Index_Source[_sCode - 1][_i] = EQ::ModelSources++;

  return _oSource;
}

char* EQ::GetExpansionName(BYTE ExpansionID)
{
  if (EQ::Client.VersionNum != EQC_TITANIUM)
  {
    return EQ::DBStrings.GetText(EQDB_EXPANSION, ExpansionID);
  }

  // Frikkin' frakkin' Titanium
  switch (ExpansionID)
  {
    case 0:
      return "EverQuest";
      break;
    case 1:
      return "Ruins of Kunark";
      break;
    case 2:
      return "Scars of Velious";
      break;
    case 3:
      return "Shadows of Luclin";
      break;
    case 4:
      return "Planes of Power";
      break;
    case 5:
      return "Legacy of Ykesha";
      break;
    case 6:
      return "Lost Dungeons of Norrath";
      break;
    case 7:
      return "Gates of Discord";
      break;
    case 8:
      return "Omens of War";
      break;
    case 9:
      return "Dragons of Norrath";
      break;
    case 10:
      return "Depths of Darkhollow";
      break;
  }

  return "";
}

EQSource* EQ::GetGlobalSource(BYTE Index)
{
  if (Index >= EQ::Index_Sources[EQSOURCE_CODEGLOBAL - 1])
  {
    return NULL;
  }

  return &EQ::ModelSource[EQ::Index_Source[EQSOURCE_CODEGLOBAL - 1][Index]];
}

BYTE EQ::GetGlobalSources()
{
  return EQ::Index_Sources[EQSOURCE_CODEGLOBAL - 1];
}

EQSource* EQ::GetOrphanSource(BYTE Index)
{
  if (Index >= EQ::Index_Sources[EQSOURCE_CODEORPHAN - 1])
  {
    return NULL;
  }

  return &EQ::ModelSource[EQ::Index_Source[EQSOURCE_CODEORPHAN - 1][Index]];
}

BYTE EQ::GetOrphanSources()
{
  return EQ::Index_Sources[EQSOURCE_CODEORPHAN - 1];
}

EQSource* EQ::GetSource(SourceID Source)
{
  if (Source == EQSOURCE_INVALID)
  {
    return NULL;
  }

  BYTE _iCode = ((Source >> 8) & 0xFF);
  BYTE _iIndex = (Source & 0xFF);

  if ((_iCode == EQSOURCE_CODEINVALID) || ((_iCode > 27) && (_iCode != EQSOURCE_CODEGLOBAL) && (_iCode != EQSOURCE_CODEORPHAN)))
  {
    return NULL;
  }

  if (_iIndex >= EQ::Index_Sources[_iCode - 1])
  {
    return NULL;
  }

  return &EQ::ModelSource[EQ::Index_Source[_iCode - 1][_iIndex]];
}

EQSource* EQ::GetSource(char* Filename)
{
  if (IsBlank(Filename))
  {
    return NULL;
  }

  size_t _iFilenameLen = strlen(Filename);
  
  if (_iFilenameLen > EQ_MAXFILENAMELEN)
  {
    return NULL;
  }
  
  char _sFilename[EQ_MAXFILENAMELEN + 1];

  memcpy_s(_sFilename, sizeof(_sFilename), Filename, _iFilenameLen + 1);
  TrimString(_sFilename, &_iFilenameLen);

  if (!_iFilenameLen)
  {
    return NULL;
  }

  _strlwr_s(_sFilename, _iFilenameLen + 1);

  EQSource* _oSource;
  char _iCode;
  WORD _iMax;
  WORD _i;
  WORD _j;

  for (_j = 0; _j < 2; _j++)
  {
    if (_j == 0)
    {
      // First pass, _j == 0, Checking non-global model sources, since they're alphabetically indexed
      if ((_iCode = EQ::GetSourceCat(_sFilename)) == EQSOURCE_CODEINVALID)
      {
        return NULL;
      }
    }
    else
    {
      // Second pass, _j == 1, Checking global model sources
      _iCode = EQSOURCE_CODEGLOBAL;
    }

    _iCode--; // So we don't have to (_iCode - 1) in all our array references

    _iMax = EQ::Index_Sources[_iCode];

    for (_i = 0; _i < _iMax; _i++)
    {
      _oSource = &EQ::ModelSource[EQ::Index_Source[_iCode][_i]];
      
      if (StringSame(_sFilename, _iFilenameLen, _oSource->Filename, _oSource->FilenameLen, false))
      {
        return _oSource;
      }
    }
  }

  return NULL;
}

BYTE EQ::GetSourceCat(char* Filename)
{
  if (IsBlank(Filename))
  {
    return 0;
  }

  char _iChar = Filename[0] & 0xDF;
  
  if ((_iChar >= 'A') && (_iChar <= 'Z')) // A-Z, a-z
  {
    return _iChar - 63;
  }
  else
  {
    return 1;
  }
}

bool EQ::IsPlayableRace(WORD RaceID)
{
  if ((RaceID == 0) || (RaceID > EQ::Client.Race_MaxID))
  {
    return false;
  }

  return EQ::Client.Race[RaceID].IsPlayable();
}

void EQ::LogMaximums(LogFile* Log)
{
  if (!Log)
  {
    return;
  }

  DWORD _i;
  DWORD _m;
  
  // #define EQRACE_MAXID                    3000
  // WORD        Race_MaxID;
  Log->Entry(QUIET, "  MAXIMUM: RaceID = %d", (void*)EQ::Client.Race_MaxID);

  // #define EQMODEL_MAXCODESPERLETTER       150
  //   WORD        Race_ModelCodeCount[27];                              // Number of Race_ModelCodeCheck entries for each letter
  //   WORD        Race_ModelCodeCheck[27][EQMODEL_MAXCODESPERLETTER];   // Up to X model codes per letter of the alphabet (+ nonalpha)
  _m = 0;
  for (_i = 0; _i < 27; _i++)
  {
    if (EQ::Client.Race_ModelCodeCount[_i] > _m)
    {
      _m = EQ::Client.Race_ModelCodeCount[_i];
    }
  }
  Log->Entry(QUIET, "  MAXIMUM: ModelCodesPerLetter = %d", (void*)_m);

  // #define EQMODEL_MAXDEFS                 4000
  // static WORD         ModelDefs;
  // static EQModel      ModelDef[EQMODEL_MAXDEFS];
  Log->Entry(QUIET, "  MAXIMUM: ModelDefs = %d", (void*)EQ::ModelDefs);

  // #define EQMODEL_MAXSOURCES              800
  // static WORD         ModelSources;
  // static EQSource     ModelSource[EQMODEL_MAXSOURCES];
  Log->Entry(QUIET, "  MAXIMUM: ModelSources = %d", (void*)EQ::ModelSources);

  // #define EQMODEL_MAXSOURCESPERLETTER     120
  // static BYTE         Index_Sources[29];
  // static WORD         Index_Source[29][EQMODEL_MAXSOURCESPERLETTER];
  _m = 0;
  for (_i = 0; _i < 29; _i++)
  {
    if (EQ::Index_Sources[_i] > _m)
    {
      _m = EQ::Index_Sources[_i];
    }
  }
  Log->Entry(QUIET, "  MAXIMUM: ModelSourcesPerLetter = %d", (void*)_m);

  // #define EQSOURCE_MAXMODELDEFS           63
  // EQModel*  Model[EQSOURCE_MAXMODELDEFS]; //4*63 = 252
  _m = 0;
  for (_i = 0; _i < EQ::ModelSources; _i++)
  {
    if (EQ::ModelSource[_i].Models > _m)
    {
      _m = EQ::ModelSource[_i].Models;
    }
  }
  Log->Entry(QUIET, "  MAXIMUM: SourceModelDefs = %d", (void*)_m);

  // #define EQMODEL_MAXVARIATIONS_TEXTURE   32
  // BYTE      VarsTexture; //1
  // BYTE      VarTexture[EQMODEL_MAXVARIATIONS_TEXTURE]; //16
  _m = 0;
  for (_i = 0; _i < EQ::ModelDefs; _i++)
  {
    if (EQ::ModelDef[_i].VarsTexture > _m)
    {
      _m = EQ::ModelDef[_i].VarsTexture;
    }
  }
  Log->Entry(QUIET, "  MAXIMUM: VarsTexture = %d", (void*)_m);

  // #define EQMODEL_MAXVARIATIONS_HEAD      32
  // BYTE      VarsHead; //1
  // BYTE      VarHead[EQMODEL_MAXVARIATIONS_HEAD]; //16
  _m = 0;
  for (_i = 0; _i < EQ::ModelDefs; _i++)
  {
    if (EQ::ModelDef[_i].VarsHead > _m)
    {
      _m = EQ::ModelDef[_i].VarsHead;
    }
  }
  Log->Entry(QUIET, "  MAXIMUM: VarsHead = %d", (void*)_m);

  // #define EQMODEL_MAXVARIATIONS_FACE      32
  // BYTE      VarsFace; //1
  // BYTE      VarFace[EQMODEL_MAXVARIATIONS_FACE]; //16
  _m = 0;
  for (_i = 0; _i < EQ::ModelDefs; _i++)
  {
    if (EQ::ModelDef[_i].VarsFace > _m)
    {
      _m = EQ::ModelDef[_i].VarsFace;
    }
  }
  Log->Entry(QUIET, "  MAXIMUM: VarsFace = %d", (void*)_m);

  // #define EQMODEL_MAXVARIATIONS_TATTOO    32
  // BYTE      VarsTattoo; //1
  // BYTE      VarTattoo[EQMODEL_MAXVARIATIONS_TATTOO]; // 16
  _m = 0;
  for (_i = 0; _i < EQ::ModelDefs; _i++)
  {
    if (EQ::ModelDef[_i].VarsTattoo > _m)
    {
      _m = EQ::ModelDef[_i].VarsTattoo;
    }
  }
  Log->Entry(QUIET, "  MAXIMUM: VarsTattoo = %d", (void*)_m);

  // #define EQMODEL_MAXVARIATIONS_HAIR      16
  // BYTE      VarsHair;
  // BYTE      VarHair[EQMODEL_MAXVARIATIONS_HAIR];
  _m = 0;
  for (_i = 0; _i < EQ::ModelDefs; _i++)
  {
    if (EQ::ModelDef[_i].VarsHair > _m)
    {
      _m = EQ::ModelDef[_i].VarsHair;
    }
  }
  Log->Entry(QUIET, "  MAXIMUM: VarsHair = %d", (void*)_m);

  // #define EQMODEL_MAXVARIATIONS_BEARD     16
  // BYTE      VarsBeard;
  // BYTE      VarBeard[EQMODEL_MAXVARIATIONS_BEARD];
  _m = 0;
  for (_i = 0; _i < EQ::ModelDefs; _i++)
  {
    if (EQ::ModelDef[_i].VarsBeard > _m)
    {
      _m = EQ::ModelDef[_i].VarsBeard;
    }
  }
  Log->Entry(QUIET, "  MAXIMUM: VarsBeard = %d", (void*)_m);

  // #define EQMODEL_MAXVARIATIONS_DETAIL    16
  // BYTE      VarsDetail;
  // BYTE      VarDetail[EQMODEL_MAXVARIATIONS_DETAIL];
  _m = 0;
  for (_i = 0; _i < EQ::ModelDefs; _i++)
  {
    if (EQ::ModelDef[_i].VarsDetail > _m)
    {
      _m = EQ::ModelDef[_i].VarsDetail;
    }
  }
  Log->Entry(QUIET, "  MAXIMUM: VarsDetail = %d", (void*)_m);

  // #define EQZONE_MAXID                    2000
  // WORD        Zone_MaxID;
  Log->Entry(QUIET, "  MAXIMUM: ZoneID = %d", (void*)EQ::Client.Zone_MaxID);

  // #define EQZONE_MAXNICKSPERLETTER        100
  // WORD        Zone_NickCount[27];                                // Number of aZoneNick entries for each letter
  // WORD        Zone_NickCheck[27][EQZONE_MAXNICKSPERLETTER];      // Up to X zone nicks per letter of the alphabet (+ nonalpha)
  _m = 0;
  for (_i = 0; _i < 27; _i++)
  {
    if (EQ::Client.Zone_NickCount[_i] > _m)
    {
      _m = EQ::Client.Zone_NickCount[_i];
    }
  }
  Log->Entry(QUIET, "  MAXIMUM: ZoneNicksPerLetter = %d", (void*)_m);

  // #define EQZONE_MAXIMPORTMODELS          200
  // BYTE      ImportModels;
  // EQModel*  ImportModel[EQZONE_MAXIMPORTMODELS];
  _m = 0;
  for (_i = 0; _i <= EQ::Client.Zone_MaxID; _i++)
  {
    if (EQ::Client.Zone[_i].ImportModels > _m)
    {
      _m = EQ::Client.Zone[_i].ImportModels;
    }
  }
  Log->Entry(QUIET, "  MAXIMUM: ZoneImportModels = %d", (void*)_m);

  Log->Entry(QUIET, "  MAXIMUM: DBStrings = %d", (void*)EQ::DBStrings.Count);

  Log->Entry(QUIET, "  MAXIMUM: DBStringID = %d", (void*)EQ::DBStrings.MaxID);

  Log->Entry(QUIET, "  MAXIMUM: List Strings = %d", (void*)EQ::StringList.Count);

  Log->Entry(QUIET, "  MAXIMUM: List StringID = %d", (void*)EQ::StringList.MaxID);
}

SourceID EQ::MakeSourceID(BYTE Code, BYTE Index)
{
  if ((Code == 0) || (Index >= EQMODEL_MAXSOURCESPERLETTER))
  {
    return EQSOURCE_INVALID;
  }

  if ((Code > 27) && (Code != EQSOURCE_CODEGLOBAL) && (Code != EQSOURCE_CODEORPHAN))
  {
    return EQSOURCE_INVALID;
  }

  return ((Code << 8) | Index);
}

// ---------------------
// EQClient Member Functions
// ---------------------

EQClient::~EQClient()
{
  _Close();
}

void EQClient::_Close()
{
  if (this->aBytes)
  {
    delete this->aBytes;
  }

  ZeroMemory(this, sizeof(*this));
}

bool EQClient::_FindNextBuildCall(CPUStatus* cpu)
{
  cpu->spos = 0; // Resetting from last BuildCall finding.

  while (cpu->filepos < cpu->fileend)
  {
    // Interpret the machine code to determine code building
    BYTE _b = (this->aBytes[cpu->filepos++]);

    switch (_b)
    {
      case 0x33: // xor ereg, ereg
        switch (this->aBytes[cpu->filepos++])
        {
          case 0xC0: // xor eax, eax
            cpu->eax = 0;
            break;
          case 0xDB: // xor ebx, ebx
            cpu->ebx = 0;
            break;
          case 0xED: // xor ebp, ebp
            cpu->ebp = 0;
            break;
          case 0xFF: // xor edi, edi
            cpu->edi = 0;
            break;
          default:
            BreakPoint;
            break;
        }
        break;
      case 0x45: // inc ebp
        cpu->ebp++;
        break;
      case 0x50: // push eax
        if (cpu->spos < EQCPU_MAXSTACKSIZE)
        {
          cpu->stack[cpu->spos++] = cpu->eax;
        }
        else
        {
          BreakPoint;
        }
        break;
      case 0x53: // push ebx
        if (cpu->spos < EQCPU_MAXSTACKSIZE)
        {
          cpu->stack[cpu->spos++] = cpu->ebx;
        }
        else
        {
          BreakPoint;
        }
        break;
      case 0x55: // push ebp
        if (cpu->spos < EQCPU_MAXSTACKSIZE)
        {
          cpu->stack[cpu->spos++] = cpu->ebp;
        }
        else
        {
          BreakPoint;
        }
        break;
      case 0x56: // push esi
        if (cpu->spos < EQCPU_MAXSTACKSIZE)
        {
          cpu->stack[cpu->spos++] = cpu->esi;
        }
        else
        {
          BreakPoint;
        }
        break;
      case 0x57: // push edi
        if (cpu->spos < EQCPU_MAXSTACKSIZE)
        {
          cpu->stack[cpu->spos++] = cpu->edi;
        }
        else
        {
          BreakPoint;
        }
        break;
      case 0x5F: // pop edi
        if (cpu->spos == 0)
        {
          BreakPoint;
        }
        else
        {
          cpu->edi = cpu->stack[--cpu->spos];
        }
        break;
      case 0x68: // push DWORD
        if (cpu->spos < EQCPU_MAXSTACKSIZE)
        {
          cpu->stack[cpu->spos++] = *(DWORD*)&this->aBytes[cpu->filepos];
        }
        else
        {
          BreakPoint;
        }
        cpu->filepos += 4;
        break;
      case 0x6A: // push BYTE
        if (cpu->spos < EQCPU_MAXSTACKSIZE)
        {
          cpu->stack[cpu->spos++] = this->aBytes[cpu->filepos];
        }
        else
        {
          BreakPoint;
        }
        cpu->filepos++; // Separated out so it executes even after the BreakPoint;
        break;
      case 0x74: // je short - skipped
        cpu->filepos++;
        break;
      case 0x75: // jne short - skipped
        cpu->filepos++;
        break;
      case 0x83: // other reg operations
        switch (this->aBytes[cpu->filepos++])
        {
          case 0x7E:
            // cmp WORD PTR [BP+xx], xx
            cpu->filepos += 2;
            break;
          case 0xBE: // cmp WORD PTR [EBP+xxxxxx], xxxx
            cpu->filepos += 5;
            break;
          case 0xC4: // add sp, i8
            //cpu->spos += (this->aBytes[cpu->filepos] >> 2);
            cpu->filepos++;
            break;
          default:
            BreakPoint;
            break;
        }
        break;
      case 0x84: // test reg, reg - skipped
        switch (this->aBytes[cpu->filepos++])
        {
          case 0xC0: // test al, al
            break;
          default:
            BreakPoint;
            break;
        }
        break;
      case 0x85: // test reg16, reg16
          switch (this->aBytes[cpu->filepos++])
          {
              case 0xC0:
                  // test ax, ax
                  break;
              default:
                  BreakPoint;
                  break;
          }
        break;
      case 0x88: // mov [reg32], reg8 - skipped
        switch (this->aBytes[cpu->filepos++])
        {
          case 0x5E: // mov [esi+xxx], bl
            cpu->filepos++;
            break;
          default:
            BreakPoint;
            break;
        }
        break;
      case 0x89: // mov [DWORD], ereg - skipped
        switch (this->aBytes[cpu->filepos++])
        {
          case 0x3E: // mov [esi], edi (2-byte opcode)
            break;
          case 0x46: // mov [esi+xxx], eax (3-byte opcode)
            cpu->filepos++;
            break;
          case 0x5C: // mov [esp+XXX], ebx (4-byte opcode)
          case 0x74: // mov [esp+XXX], esi (4-byte opcode)
          case 0x7C: // mov [esp+XXX], edi (4-byte opcode)
            cpu->filepos++;
            cpu->filepos++;
            break;
          case 0x7E: // mov [esi+XXX], edi (3-byte opcode)
            cpu->filepos++;
            break;
          case 0x86: // mov [bp+xxxxxxxx], eax
            cpu->filepos += 4;
            break;
          default:
            BreakPoint;
            break;
        }
        break;
      case 0x8B: // mov reg32, XXX
        switch (this->aBytes[cpu->filepos++])
        {
          case 0x46: // mov eax, [ebp+XXX] - skipped
            cpu->eax = 0;
            cpu->filepos += 1;
            break;
          case 0x86: // mov eax, [esi+XXX] - skipped
            cpu->eax = 0;
            cpu->filepos += 4;
            break;
          case 0x8E: // mov ecx, [esi+XXX] - skipped
            cpu->ecx = 0;
            cpu->filepos += 4;
            break;
          case 0xC8: // mov ecx, eax
            cpu->ecx = cpu->eax;
            break;
          case 0xCE: // mov ecx, esi
            cpu->ecx = cpu->esi;
            break;
          case 0xF1: // mov esi, ecx
            cpu->esi = cpu->ecx;
            break;
          default:
            BreakPoint;
            break;
        }
        break;
      case 0x8D: // lea reg32, [reg32]
        switch (this->aBytes[cpu->filepos++])
        {
          case 0x7E: // lea edi, [esi+XXX]
            cpu->edi = cpu->esi + this->aBytes[cpu->filepos++];
            break;
          default:
            BreakPoint;
            break;
        }
        break;
      case 0xB8: // mov eax, DWORD
        cpu->eax = *(DWORD*)&this->aBytes[cpu->filepos];
        cpu->filepos += 4;
        break;
      case 0xB9: // mov ecx, DWORD
        cpu->ecx = *(DWORD*)&this->aBytes[cpu->filepos];
        cpu->filepos += 4;
        break;
      case 0xBB: // mov ebx, DWORD
        cpu->ebx = *(DWORD*)&this->aBytes[cpu->filepos];
        cpu->filepos += 4;
        break;
      case 0xBD: // mov ebp, DWORD
        cpu->ebp = *(DWORD*)&this->aBytes[cpu->filepos];
        cpu->filepos += 4;
        break;
      case 0xBF: // mov edi, DWORD
        cpu->edi = *(DWORD*)&this->aBytes[cpu->filepos];
        cpu->filepos += 4;
        break;
      case 0xC6: // mov [DWORD], BYTE - skipped
        switch (this->aBytes[cpu->filepos++])
        {
          case 0x46: // mov [esi+x], BYTE
            cpu->filepos += 2;
            break;
          default:
            BreakPoint;
            break;
        }
        break;
      case 0xC7: // mov [DWORD], DWORD - skipped
        switch (this->aBytes[cpu->filepos++])
        {
          case 0x06: // mov [esi], DWORD
            cpu->filepos += 4;
            break;
          case 0x80: // mov [eax+XXX], DWORD
          case 0x81: // mov [ecx+XXX], DWORD
            cpu->filepos += 8;
            break;
          default:
            BreakPoint;
            break;
        }
        break;
      case 0xE8: // call [relative_ptr_sint32]
        if (((*(sint32*)&this->aBytes[cpu->filepos]) + cpu->filepos) == cpu->buildcall)
        {
          // We've encountered the buildcall they're looking for

          cpu->filepos += 4;

          return true;
        }
        
        
        if ((cpu->buildcall2 > 0) && (((*(sint32*)&this->aBytes[cpu->filepos]) + cpu->filepos) == cpu->buildcall2))
        {
            // We've encountered the buildcall2 they're looking for

            cpu->filepos += 4;

            return true;
        }

        // Different call. Reset stack and keep going.
        cpu->spos = 0;
        cpu->filepos += 4;
        break;
      case 0xEB: // jump [relative_ptr_sint8]
          cpu->filepos += (this->aBytes[cpu->filepos++]);
          break;
      case 0xF3: // rep stos - memory writing portion skipped
        switch (this->aBytes[cpu->filepos++])
        {
          case 0xAB: // rep stos es:[edi]
            cpu->edi += cpu->ecx;
            cpu->ecx = 0;
            break;
          default:
            BreakPoint;
            break;
        }
        break;
      case 0xFF: // call [DWORD] - skipped
        switch (this->aBytes[cpu->filepos++])
        {
          case 0x15: // call ds:[DWORD]
            cpu->filepos += 4;
            break;
          default:
            BreakPoint;
            break;
        }
        break;
      default:
        BreakPoint;
        break;
    }
  }

  return false;
}

char* EQClient::GetModelCode(WORD RaceID, EQGENDER Gender)
{
  if ((RaceID == EQRACE_INVALID) || (RaceID > this->Race_MaxID) ||
      (Gender < EQRG_MALE) || (Gender > EQRG_NEUTRAL))
  {
    return NULL;
  }

  return this->Race[RaceID].ModelCode[Gender - EQRG_MALE];
}

HRESULT EQClient::GetRaceFromModelCode(char* ModelCode, WORD* RaceID, EQGENDER* Gender)
{
  if (!ModelCode || !RaceID || !Gender)
  {
    if (RaceID)
    {
      *RaceID = EQRACE_INVALID;
    }

    if (Gender)
    {
      *Gender = EQRG_INVALID;
    }
    
    return E_INVALIDARG;
  }

  int   _iModelCodeLen = strlen(ModelCode);
  if (_iModelCodeLen > EQMODEL_MAXCODELEN)
  {
    if (RaceID)
    {
      *RaceID = EQRACE_INVALID;
    }

    if (Gender)
    {
      *Gender = EQRG_INVALID;
    }
    
    return E_FAIL;
  }

  WORD  _iRaceID;
  EQGENDER  _iGender;
  char  _iCode;
  int   _i;
  int   _r;
  int   _g;
  char  _sModelCode[EQMODEL_MAXCODELEN + 1];

  memcpy_s(_sModelCode, sizeof(_sModelCode), ModelCode, _iModelCodeLen + 1);
  _strupr_s(_sModelCode, _iModelCodeLen + 1);

  _iRaceID = EQRACE_INVALID;
  _iGender = EQRG_INVALID;

  if (_sModelCode[0] != NULL)
  {
    _iCode = EQ::GetSourceCat(_sModelCode) - 1;

    for (_i = 0; (_iRaceID == EQRACE_INVALID) && (_i < this->Race_ModelCodeCount[_iCode]); _i++)
    {
      // Reverse order of checking. Allow newer races that re-use old model codes (TEM/TEF, for example) to override old ones.
      _r = this->Race_ModelCodeCheck[_iCode][this->Race_ModelCodeCount[_iCode] - _i - 1];

      if (_r <= this->Race_MaxID) // Sanity check
      {
        for (_g = 0; (_iRaceID == EQRACE_INVALID) && (_g < EQRACE_GENDERCOUNT); _g++)
        {
          if (this->Race[_r].ModelCode[_g])
          {
            if (StringSame(this->Race[_r].ModelCode[_g], this->Race[_r].ModelCodeLen[_g], _sModelCode, _iModelCodeLen, false))
            {
              _iRaceID = _r;
              _iGender = (EQGENDER)(_g + EQRG_MALE);
            }
          }
        }
      }
    }
  }

  *RaceID = _iRaceID;
  *Gender = _iGender;

  return (_iRaceID == EQRACE_INVALID) ? E_FAIL : S_OK;
}

EQCLIENTVER EQClient::GetVersionOf(char* Filename)
{
  size_t  _iFileSize;
  int     _iClient;
  int     _iNumClients;

  if (!Filename)
  {
    return EQC_INVALID;
  }

  if (!(_iFileSize = GetFileSize(Filename)))
  {
    return EQC_INVALID;
  }

  _iNumClients = sizeof(ClientSpecs) / sizeof(ClientSpecs[0]);

  for (_iClient = 0; _iClient < _iNumClients; _iClient++)
  {
    if (_iFileSize == ClientSpecs[_iClient].FileSize)
    {
      return (EQCLIENTVER)ClientSpecs[_iClient].VersionNum;
    }
  }

  return EQC_INVALID;
}

EQZone* EQClient::GetZone(WORD ZoneID)
{
  if (ZoneID > this->Zone_MaxID)
  {
    return NULL;
  }

  return &this->Zone[ZoneID];
}

EQZone* EQClient::GetZone(char* ZoneNick)
{
  EQZone* _oZone = NULL;
  char    _Code;
  int     _i;
  int     _z;
  char    _sZoneNick[33];
  size_t  _iZoneNickLen;

  
  if (IsBlank(ZoneNick))
  {
    return _oZone;
  }

  _iZoneNickLen = strlen(ZoneNick);
  if (_iZoneNickLen > 32)
  {
    return _oZone;
  }

  memcpy_s(_sZoneNick, sizeof(_sZoneNick), ZoneNick, _iZoneNickLen + 1);
  _strlwr_s(_sZoneNick, _iZoneNickLen + 1);

  if (_sZoneNick[0] != NULL)
  {
    _Code = EQ::GetSourceCat(_sZoneNick);

    for (_i = 0; (_oZone == NULL) && (_i < this->Zone_NickCount[_Code]); _i++)
    {
      _z = this->Zone_NickCheck[_Code][_i];

      if (_z <= this->Zone_MaxID) // Sanity check
      {
        if (StringSame(_sZoneNick, _iZoneNickLen, this->Zone[_z].Nick, false))
        {
          _oZone = &this->Zone[_z];
        }
      }
    }
  }

  return _oZone;
}

HRESULT EQClient::Load(char* ClientEXE)
{
  FILE*   _oFile;
  size_t  _iFileSize;
  DWORD   _i;
  DWORD   _iFilePos;
  
  if (IsBlank(ClientEXE))
  {
    return E_INVALIDARG;
  }

  this->_Close();

  // Not needed. It's already 0 from the ZeroMemory() in _Close()
  //this->oClient.Version = EQC_INVALID;
  
  strncpy_s(this->sFilePath, sizeof(this->sFilePath), ClientEXE, _TRUNCATE);
  
  if ((this->VersionNum = this->GetVersionOf(this->sFilePath)) == EQC_INVALID)
  {
    return E_FAIL;
  }

  if (fopen_s(&_oFile, this->sFilePath, "rb"))
  {
    this->_Close();

    return E_FAIL;
  }

  _iFileSize = _filelength(_fileno(_oFile));

  this->aBytes = new BYTE[_iFileSize];

  if (!this->aBytes)
  {
    this->_Close();

    return E_OUTOFMEMORY;
  }

  if (fread(this->aBytes, _iFileSize, 1, _oFile) != 1)
  {
    fclose(_oFile);

    this->_Close();
    
    return E_FAIL;
  }

  fclose(_oFile);

  this->FileSize = _iFileSize;

  // Load memory mapping data from the EXE File Header
  
  IMAGE_DOS_HEADER*     _oHeaderDOS;
  IMAGE_NT_HEADERS*     _oHeaderWin;
  IMAGE_SECTION_HEADER* _oHeaderSec;

  _oHeaderDOS = (IMAGE_DOS_HEADER*)&this->aBytes[0];

  if (_oHeaderDOS->e_magic != IMAGE_DOS_SIGNATURE)
  {
    this->_Close();
    
    return E_FAIL; // Not an EXE!
  }

  _oHeaderWin = (IMAGE_NT_HEADERS*)&this->aBytes[_oHeaderDOS->e_lfanew];

  if (_oHeaderWin->Signature != IMAGE_NT_SIGNATURE)
  {
    this->_Close();
    
    return E_FAIL; // Not an EXE!
  }

  WORD _iSections = _oHeaderWin->FileHeader.NumberOfSections;

  _iFilePos = _oHeaderDOS->e_lfanew + sizeof(IMAGE_NT_HEADERS);

  // Now situated at the section entries

  for (_i = 0; _i < _iSections; _i++)
  {
    _oHeaderSec = (IMAGE_SECTION_HEADER*)&this->aBytes[_iFilePos];

    if (StringSame(".rdata", 6, (char*)_oHeaderSec->Name, true))
    {
      break;
    }

    _iFilePos += sizeof(IMAGE_SECTION_HEADER);
  }

  if (_i >= _iSections)
  {
    this->_Close();

    return E_FAIL; // Didn't find .rdata, required for processing!
  }

  if (false) //(this->VersionNum == 64-bit-exe-version)
  {
      IMAGE_NT_HEADERS64* _oHeaderW64 = (IMAGE_NT_HEADERS64*)&this->aBytes[_oHeaderDOS->e_lfanew];

      this->iRDataMemoryOffset = _oHeaderSec->VirtualAddress + _oHeaderW64->OptionalHeader.ImageBase;
      this->iRDataMemoryEnd = this->iRDataMemoryOffset + _oHeaderSec->SizeOfRawData - 1;
      this->iRDataFileOffset = _oHeaderSec->PointerToRawData;
  }
  else
  {
      this->iRDataMemoryOffset = _oHeaderSec->VirtualAddress + _oHeaderWin->OptionalHeader.ImageBase;
      this->iRDataMemoryEnd = this->iRDataMemoryOffset + _oHeaderSec->SizeOfRawData - 1;
      this->iRDataFileOffset = _oHeaderSec->PointerToRawData;
  }
  
  DWORD _j = sizeof(ClientSpecs) / sizeof(ClientSpecs[0]);

  for (_i = 0; _i < _j; _i++)
  {
    if (ClientSpecs[_i].VersionNum == this->VersionNum)
    {
      this->VersionNick = ClientSpecs[_i].VersionNick;
      this->VersionName = ClientSpecs[_i].VersionName;
      
      this->iRaceBuildCodeStart = ClientSpecs[_i].RaceBuildCodeStart;
      this->iRaceBuildCodeEnd = ClientSpecs[_i].RaceBuildCodeEnd;
      this->iRaceBuildCodeCall = ClientSpecs[_i].RaceBuildCodeCall;

      this->iZoneBuildCodeStart = ClientSpecs[_i].ZoneBuildCodeStart;
      this->iZoneBuildCodeEnd = ClientSpecs[_i].ZoneBuildCodeEnd;
      this->iZoneBuildCodeCall = ClientSpecs[_i].ZoneBuildCodeCall;
      this->iZoneBuildCodeCall2 = ClientSpecs[_i].ZoneBuildCodeCall2;

      break;
    }
  }

  return S_OK;
}

HRESULT EQClient::LoadRaces()
{
  if (!this->aBytes)
  {
    return E_FAIL;
  }

  DWORD     _iRace;
  DWORD     _iGender;
  DWORD     _iCodePointer;
  char*     _sModelCode;
  char      _iCodeChar;
  size_t    _iModelCodeLen;
  bool      _bFoundNewRace;
  DWORD     _i;
  CPUStatus _oCPU;

  ZeroMemory(&_oCPU, sizeof(_oCPU));

  this->Races = 0;
  this->Race_MaxID = 0;
  this->Models = 0;

  _oCPU.buildcall = this->iRaceBuildCodeCall;
  _oCPU.buildcall2 = 0;
  _oCPU.fileend = this->iRaceBuildCodeEnd;
  _oCPU.filepos = this->iRaceBuildCodeStart;

  while (this->_FindNextBuildCall(&_oCPU))
  {
    switch (this->VersionNum)
    {
      case EQC_TITANIUM:
        _iGender      = _oCPU.stack[0];
        _iRace        = _oCPU.stack[1];
        _iCodePointer = _oCPU.stack[2];
        break;
      case EQC_SOF:
        _iCodePointer = _oCPU.stack[2];
        _iGender      = _oCPU.stack[3];
        _iRace        = _oCPU.stack[4];
        break;
      case EQC_ROF2:
        _iCodePointer = _oCPU.stack[2];
        _iGender = _oCPU.stack[3];
        _iRace = _oCPU.stack[4];
        break;
      default:
        BreakPoint;
        break;
    }
    
    // GenderID
    if (_iGender >= EQRACE_GENDERCOUNT)
    {
      BreakPoint;

      continue; // Can't work with this gender. Skip it.
    }

    // RaceID
    if (_iRace > EQRACE_MAXID)
    {
      BreakPoint;

      continue; // Can't work with this race. Skip it.
    }

    // Mapped memory pointer to model code
    if ((_iCodePointer < this->iRDataMemoryOffset) || (_iCodePointer > this->iRDataMemoryEnd))
    {
      BreakPoint;

      continue; // Can't work with this code. Skip it.
    }

    _sModelCode = (char*)&this->aBytes[_iCodePointer - this->iRDataMemoryOffset + this->iRDataFileOffset];
    _iModelCodeLen = strlen(_sModelCode) + 1;

    // Model codes are standardized to upper case.
    // They're supposed to come upper-cased from the client, but we have to enforce it.
    _strupr_s(_sModelCode, _iModelCodeLen);

    _bFoundNewRace = true;
    for (_i = 0; _i < EQRACE_GENDERCOUNT; _i++)
    {
      if (this->Race[_iRace].ModelCode[_i])
      {
        _bFoundNewRace = false;
        
        break;
      }
    }

    if (_bFoundNewRace)
    {
      this->Races++;
    }

    this->Race[_iRace].ModelCode[_iGender] = _sModelCode;
    this->Race[_iRace].ModelCodeLen[_iGender] = _iModelCodeLen - 1;
    this->Race[_iRace].ID = (WORD)_iRace;

    if (_iRace > this->Race_MaxID)
    {
      this->Race_MaxID = (WORD)_iRace;
    }

    this->Models++;
  }

  // Build index for ModelCode-to-RaceID lookups
  for (_iRace = 0; _iRace <= this->Race_MaxID; _iRace++)
  {
    for (_iGender = 0; _iGender < EQRACE_GENDERCOUNT; _iGender++)
    {
      if (this->Race[_iRace].ModelCode[_iGender])
      {
        if (_iCodeChar = EQ::GetSourceCat(this->Race[_iRace].ModelCode[_iGender]))
        {
          _iCodeChar--;

          if ((this->Race_ModelCodeCount[_iCodeChar] == 0) ||
              (this->Race_ModelCodeCheck[_iCodeChar][this->Race_ModelCodeCount[_iCodeChar] - 1] != _iRace))
          {
            // Sanity check. Should never hit this boundary, though, since we've set it so high.
            if (this->Race_ModelCodeCount[_iCodeChar] < EQMODEL_MAXCODESPERLETTER)
            {
              // TODO: Alphabetize Race_ModelCodeCheck[][]?
              this->Race_ModelCodeCheck[_iCodeChar][this->Race_ModelCodeCount[_iCodeChar]++] = (WORD)_iRace;
            }
            else
            {
              BreakPoint;
            }
          }
        }
      }
    }
  }
  
  return S_OK;
}

HRESULT EQClient::LoadZones()
{
  CPUStatus _oCPU;
  QWORD     _iMemoryOffset;
  DWORD     _iZoneID;
  EQZone    _oZone;

  if (!this->aBytes)
  {
    return E_FAIL;
  }

  ZeroMemory(&_oCPU, sizeof(_oCPU));
  ZeroMemory(&_oZone, sizeof(_oZone));

  this->Zones = 0;
  this->Zone_MaxID = 0;

  _oCPU.buildcall = this->iZoneBuildCodeCall;
  _oCPU.buildcall2 = this->iZoneBuildCodeCall2;
  _oCPU.fileend = this->iZoneBuildCodeEnd;
  _oCPU.filepos = this->iZoneBuildCodeStart;

  while (this->_FindNextBuildCall(&_oCPU))
  {
    // New Zone Information!

    switch (this->VersionNum)
    {
      case EQC_TITANIUM:
      case EQC_SOF:
      case EQC_ROF2:
        // Both appear to use the same parameters for zone processing
        if (_oCPU.spos < 9)
        {
          BreakPoint;

          continue;
        }
        else
        {
          if ((_iZoneID = _oCPU.stack[7]) > EQZONE_MAXID)
          {
            BreakPoint;

            continue;
          }

          _oZone.Expansion    = (BYTE)_oCPU.stack[8];
          _oZone.ID           = (WORD)_oCPU.stack[7];
          _oZone.Nick         = (char*)_oCPU.stack[6];
          _oZone.Name         = (char*)_oCPU.stack[5];
          _oZone.DBStrNameID  = _oCPU.stack[4];
          _oZone.Flags        = _oCPU.stack[3];
          _oZone.Unknown7     = _oCPU.stack[2];
          _oZone.Unknown8     = _oCPU.stack[1];
          _oZone.MinLevel     = (BYTE)_oCPU.stack[0];
        }
        break;
      default:
        BreakPoint;

        continue;
        break;
    }
    
    if (_iZoneID > EQZONE_MAXID)
    {
      BreakPoint;

      continue; // Can't work with this zone. Skip it.
    }

    if (((DWORD)_oZone.Nick < this->iRDataMemoryOffset) || ((DWORD)_oZone.Nick > this->iRDataMemoryEnd))
    {
      BreakPoint;

      continue; // Can't work with this nick. Skip it.
    }

    if (((DWORD)_oZone.Name < this->iRDataMemoryOffset) || ((DWORD)_oZone.Name > this->iRDataMemoryEnd))
    {
      BreakPoint;

      continue; // Can't work with this name. Skip it.
    }

    _iMemoryOffset = (DWORD)this->aBytes - this->iRDataMemoryOffset + this->iRDataFileOffset;

    _oZone.Nick += _iMemoryOffset;
    _oZone.Name += _iMemoryOffset;

    // Zone nicks are standardized to lower case
    _strlwr_s(_oZone.Nick, strlen(_oZone.Nick) + 1);

    this->Zone[_iZoneID] = _oZone;

    if (_iZoneID > this->Zone_MaxID)
    {
      this->Zone_MaxID = (WORD)_iZoneID;
    }

    this->Zones++;
  }

  char _iCodeChar;
  
  // Build index for ZoneNick-to-ZoneID lookups
  for (_iZoneID = 0; _iZoneID <= this->Zone_MaxID; _iZoneID++)
  {
    if (this->Zone[_iZoneID].Nick)
    {
      if (_iCodeChar = EQ::GetSourceCat(this->Zone[_iZoneID].Nick))
      {
        _iCodeChar--;

        // Sanity check. Should never hit this boundary, though, since we've set it so high.
        if (this->Zone_NickCount[_iCodeChar] < EQZONE_MAXNICKSPERLETTER)
        {
          // TODO: Alphabetize Zone_NickCheck[][] for quicker index checking
          this->Zone_NickCheck[_iCodeChar][this->Zone_NickCount[_iCodeChar]++] = (WORD)_iZoneID;
        }
        else
        {
          BreakPoint;
        }
      }
    }
  }
  
  return S_OK;
}

// ---------------------
// EQDBStrings Member Functions
// ---------------------

EQDBStrings::~EQDBStrings()
{
  this->_Close();
}

void EQDBStrings::_Close()
{
  SAFE_DELETE(this->aDBString_Bytes);

  Count = 0;
  MaxID = 0;
  MaxType = 0;
  iDBString_Size = 0;
  aDBString_Bytes = NULL;

  //ZeroMemory(this, sizeof(*this));
}

char* EQDBStrings::GetText(EQDBSTRTYPE TypeID, DWORD StringID)
{
    /*
  if (this->Count == 0)
  {
    return NULL; // We haven't loaded strings yet!
  }

  if ((TypeID < 0) || (TypeID > this->MaxType))
  {
    return NULL; // Invalid TypeID
  }

  if (StringID > this->MaxID)
  {
    return NULL; // Invalid StringID
  }
  */
  
  DWORD _iStringHash = StringID | ((DWORD)TypeID << 24);
  
  if (this->sDBString.count(_iStringHash) == 1)
  {
      return (char*)&this->aDBString_Bytes[this->sDBString[_iStringHash]];
  }
  else
  {
      return NULL;
  }
  
  /*
  if (this->iDBString_Line[StringID] == EQDBSTR_INVALID)
  {
    return NULL; // StringID not found
  }

  WORD _i;

  for (_i = this->iDBString_Line[StringID]; (_i < this->Count) && (this->iDBString_ID[_i] == StringID); _i++)
  {
    if (this->iDBString_Type[_i] == TypeID)
    {
      // We found our StringID and TypeID!

      return this->aDBString_Text[_i];
    }
  }

  return NULL; // StringID found, but not with that TypeID
  */
}

HRESULT EQDBStrings::Load(char* Filename)
{
  if (IsBlank(Filename))
  {
    return E_INVALIDARG;
  }

  FILE*   _oFile;
  size_t  _iFileSize;

  if (this->aDBString_Bytes)
  {
    delete this->aDBString_Bytes;

    this->aDBString_Bytes = NULL;
  }

  this->Count = 0;
  this->MaxType = 0;
  this->MaxID = 0;

  if (fopen_s(&_oFile, Filename, "rb"))
  {
    return E_FAIL;
  }
  
  _iFileSize = _filelength(_fileno(_oFile));
  
  if (!(this->aDBString_Bytes = new char[_iFileSize + 1]))
  {
    fclose(_oFile);
    
    return E_OUTOFMEMORY;
  }

  if (fread_s(this->aDBString_Bytes, _iFileSize, _iFileSize, 1, _oFile) != 1)
  {
    fclose(_oFile);

    delete this->aDBString_Bytes;

    this->aDBString_Bytes = NULL;

    return E_FAIL;
  }
  
  fclose(_oFile);

  char* _sStringID = NULL;
  char* _sTypeID = NULL;
  char* _sString = NULL;
  char* _sNextStringID = this->aDBString_Bytes;
  char* _sStringEnd = NULL;
  char* _pEndOfFile = &this->aDBString_Bytes[_iFileSize];
  bool _bDone = false;

  _pEndOfFile[0] = 0x00;
  
  this->Count = 0;

  DWORD _iStringID;
  DWORD _iTypeID;

  //memset(this->iDBString_Line, 0xFF, sizeof(this->iDBString_Line));

  while (!_bDone)
  {
    _sStringID = _sNextStringID;
    _bDone = (_sTypeID = (char*)memchr(_sStringID, '^', _pEndOfFile - _sStringID)) == NULL;

    if (!_bDone)
    {
      _bDone = (_sString = (char*)memchr(&_sTypeID[1], '^', _pEndOfFile - _sTypeID - 1)) == NULL;
    }

    if (!_bDone)
    {
      // Split the strings and set things accordingly.

      // _sStringiD points to our StringID

      // Set _sTypeID to our TypeID
      _sTypeID[0] = 0x00;
      _sTypeID++;

      // Set _sString to our String Text
      _sString[0] = 0x00;
      _sString++;

      if (_bDone = !(_sStringEnd = (char*)memchr(_sString, '\n', _pEndOfFile - _sString)))
      {
        // End of file = End of string!

        _sNextStringID = NULL;
      }
      else
      {
        _bDone = (_sNextStringID = (char*)memchr(_sStringEnd, '^', _pEndOfFile - _sStringEnd)) == NULL;
      }

      if (_sNextStringID)
      {
        // _sNextStringID1 now points to the first ^ after our String Text starts (or NULL if we're at the end of the file)
        // Adjust it backwards to make it more useful
        while (_sNextStringID[0] != '\n')
        {
          _sNextStringID--;
        }

        // Set _sStringEnd to the real end of our String Text, taking multi-line strings into account
        // (Though _sStringEnd may not be needed anymore)
        _sStringEnd = _sNextStringID;

        // Set _sNextStringID to our next StringID
        _sNextStringID[0] = 0x00;
        _sNextStringID++;
      }
      else
      {
        // Set _sStringEnd to the real end of our String Text, taking multi-line strings into account
        // (Though _sStringEnd may not be needed anymore)
        _sStringEnd = _pEndOfFile;
      }

      TrimString(_sStringID);
      _iStringID = atoi(_sStringID);
      if ((_iStringID == 0) && ((_sStringID[0] != '0') || (_sStringID[1] != 0x00)))
      {
        BreakPoint; // Invalid StringID found
        
        continue; // Continue processing, just skip this string.
      }

      if (true) //_iStringID <= EQDBSTR_MAXID)
      {
        TrimString(_sTypeID);
        _iTypeID = atoi(_sTypeID);
        if (_iTypeID > 0xFF)
        {
          BreakPoint; // Invalid TypeID found

          continue; // Continue processing, just skip this string
        }

        // We have a new DB String!

        char* _caret = (char*)memchr(_sString, '^', _sStringEnd - _sString);
        if (_caret != NULL)
        {
            _caret[0] = 0x00;
        }


        TrimString(_sString);

        DWORD _iStringHash = _iStringID + ((DWORD)_iTypeID << 24);

        this->sDBString[_iStringHash] = (_sString - this->aDBString_Bytes);

        /*
        if (this->iDBString_Line[_iStringID] == EQDBSTR_INVALID)
        {
          // Add this line to index as first line with this StringID
          this->iDBString_Line[_iStringID] = this->Count;
        }

        this->iDBString_ID[this->Count] = _iStringID;
        this->iDBString_Type[this->Count] = (BYTE)_iTypeID;
        this->aDBString_Text[this->Count] = _sString;
        */

        if ((BYTE)_iTypeID > this->MaxType)
        {
          this->MaxType = (BYTE)_iTypeID;
        }

        if (_iStringID > this->MaxID)
        {
          this->MaxID = _iStringID;
        }

        this->Count++;

        _bDone |= (this->Count == 0);// || (this->Count >= EQDBSTR_MAXCOUNT);
      }
      else
      {
        BreakPoint; // StringID higher than our max! Ruh roh, Raggy!
      }
    }
  }
  
  return S_OK;
}

// ---------------------
// EQModel Member Functions
// ---------------------

HRESULT EQModel::AddVariation(BYTE VariationType, BYTE Variation)
{
  if (!Variation)
  {
    // Skipping Variation #0. We're assuming THAT one always exists.
    
    return S_OK;
  }

  BYTE* _pVariations;
  BYTE  _iMaxVars;
  BYTE* _pCount;
  
  switch (VariationType)
  {
    case EQRV_TEXTURE:
      _pVariations = &this->VarTexture[0];
      _pCount = &this->VarsTexture;
      _iMaxVars = EQMODEL_MAXVARIATIONS_TEXTURE;
      break;
    case EQRV_HEAD:
      _pVariations = &this->VarHead[0];
      _pCount = &this->VarsHead;
      _iMaxVars = EQMODEL_MAXVARIATIONS_HEAD;
      break;
    case EQRV_FACE:
      _pVariations = &this->VarFace[0];
      _pCount = &this->VarsFace;
      _iMaxVars = EQMODEL_MAXVARIATIONS_FACE;
      break;
    case EQRV_HAIR:
      _pVariations = &this->VarHair[0];
      _pCount = &this->VarsHair;
      _iMaxVars = EQMODEL_MAXVARIATIONS_HAIR;
      break;
    case EQRV_BEARD:
      _pVariations = &this->VarBeard[0];
      _pCount = &this->VarsBeard;
      _iMaxVars = EQMODEL_MAXVARIATIONS_BEARD;
      break;
    case EQRV_DETAIL:
      _pVariations = &this->VarDetail[0];
      _pCount = &this->VarsDetail;
      _iMaxVars = EQMODEL_MAXVARIATIONS_DETAIL;
      break;
    case EQRV_TATTOO:
      _pVariations = &this->VarTattoo[0];
      _pCount = &this->VarsTattoo;
      _iMaxVars = EQMODEL_MAXVARIATIONS_TATTOO;
      break;
    default:
      return E_INVALIDARG;
      break;
  }

  BYTE* _pLast = &_pVariations[*_pCount - 1];
  BYTE* _pVariation = _pVariations;

  // Check for existing record of this variation
  for (; _pVariation <= _pLast; _pVariation++)
  {
    if (*_pVariation == Variation)
    {
      return S_OK; // Duplicate. Ignore.
    }
    else if (*_pVariation > Variation)
    {
      break; // Ascending sorted. It's not here.
    }
  }

  // As long as we're not at our limit, add the new one in!
  if (*_pCount < _iMaxVars)
  {
    if (_pVariation <= _pLast)
    {
      // Inserting it between others
      memmove_s(_pVariation + 1, _iMaxVars, _pVariation, _pLast - _pVariation + 1);
    }
    *_pVariation = Variation;
    (*_pCount)++;

    return S_OK;
  }

  return E_OUTOFMEMORY;
}

HRESULT EQModel::AddVariations(BYTE VariationType, BYTE MaxVariation)
{
  if ((VariationType == EQRV_NONE) || (VariationType > EQRV_MAX))
  {
    return E_INVALIDARG;
  }

  BYTE _v;

  for (_v = 1; _v <= MaxVariation; _v++)
  {
    this->AddVariation(VariationType, _v);
  }

  return S_OK;
}

bool EQModel::IsGlobal(WORD RaceID, EQGENDER Gender, EQModel** Model)
{
  if (Model)
  {
    *Model = NULL;
  }

  if ((RaceID == EQRACE_INVALID) || (RaceID > EQ::Client.Race_MaxID) ||
     (Gender < EQRG_MALE) || (Gender > EQRG_NEUTRAL))
  {
    return false;
  }

  WORD _iMax = EQ::GetGlobalSources();
  WORD _i;
  EQSource* _oSource;

  // See if this model is in any of our global sources
  for (_i = 0; _i < _iMax; _i++)
  {
    _oSource = EQ::GetGlobalSource((BYTE)_i);

    if (_oSource->HasModel(RaceID, Gender, Model))
    {
      return true;
    }
  }

  return false;
}

// ---------------------
// EQRace Member Functions
// ---------------------

char* EQRace::GetDesc()
{
  return EQ::DBStrings.GetText(EQDB_RACEDESC, this->ID);
}

char* EQRace::GetName()
{
  return EQ::DBStrings.GetText(EQDB_RACENAME, this->ID);
}

char* EQRace::GetPlural()
{
  return EQ::DBStrings.GetText(EQDB_RACEPLURAL, this->ID);
}

bool EQRace::IsGlobal(EQModel** Model)
{
  BYTE _iGender;

  if (Model)
  {
    *Model = NULL;
  }
  
  if ((this->ID == EQRACE_INVALID) || (this->ID > EQ::Client.Race_MaxID))
  {
    return false;
  }
  
  // There really isn't a much faster way.
  // Checking each global source for all of a race's model codes is going to take time no matter what.
  for (_iGender = 0; _iGender <= EQRACE_GENDERCOUNT; _iGender++)
  {
    if (EQ::Client.Race[this->ID].ModelCode[_iGender])
    {
      if (EQModel::IsGlobal(this->ID, (EQGENDER)(_iGender + EQRG_MALE), Model))
      {
        return true;
      }
    }
  }

  return false;
}

bool EQRace::IsPlayable()
{
  switch (this->ID)
  {
    case 1:
    case 2:
    case 3:
    case 4:
    case 5:
    case 6:
    case 7:
    case 8:
    case 9:
    case 10:
    case 11:
    case 12:
    case 128:
    case 130:
    case 330:
    case 522:
      return true;
  }

  return false;
  
  // Bit of a cheat. A race is playable if it has a "Character
  // Creation Description" configured in dbstr_us.txt. :P
  //return (EQ::DBStrings.GetText(EQDB_RACEDESC, this->ID) != NULL);
  //
  // Never mind. Titanium doesn't have these strings. Time to hardcode.
}

// ---------------------
// EQSource Member Functions
// ---------------------

EQModel* EQSource::AddModel(char* ModelCode)
{
  if (IsBlank(ModelCode))
  {
    return NULL;
  }
  
  char _sModelCode[EQMODEL_MAXCODELEN + 1];
  size_t _iModelCodeLen;
  WORD _i;
  EQModel* _oModel;

  _iModelCodeLen = strlen(ModelCode);
  if (_iModelCodeLen > EQMODEL_MAXCODELEN)
  {
    return NULL;
  }

  memcpy_s(_sModelCode, sizeof(_sModelCode), ModelCode, _iModelCodeLen + 1);
  TrimString(_sModelCode, &_iModelCodeLen);

  if (!_iModelCodeLen)
  {
    return NULL;
  }

  _strupr_s(_sModelCode, _iModelCodeLen + 1);

  for (_i = 0; _i < this->Models; _i++)
  {
    _oModel = this->Model[_i];
    
    if (StringSame(_oModel->Code, _oModel->CodeLen, _sModelCode, _iModelCodeLen, false))
    {
      // Found duplicate. Return the original.
      
      return _oModel;
    }
  }

  if (this->Models >= EQSOURCE_MAXMODELDEFS)
  {
    // Too many model definitions for this source!
    BreakPoint;
    
    return NULL;
  }

  if (!(_oModel = EQ::AddModel()))
  {
    return NULL;
  }

  WORD _iRaceID;
  EQGENDER _iGender;

  EQ::Client.GetRaceFromModelCode(_sModelCode, &_iRaceID, &_iGender);

  this->Model[this->Models] = _oModel;

  _oModel->RaceID = _iRaceID;
  _oModel->Gender = _iGender;
  _oModel->CodeLen = _iModelCodeLen;
  _oModel->IndexInSource = this->Models++;
  _oModel->Source = this;

  memcpy_s(_oModel->Code, sizeof(_oModel->Code), _sModelCode, _iModelCodeLen + 1);

  return _oModel;
}

bool EQSource::HasModel(WORD RaceID, EQGENDER Gender, EQModel** ModelDef)
{
  if (ModelDef)
  {
    *ModelDef = NULL;
  }

  if ((RaceID == EQRACE_INVALID) || (RaceID > EQ::Client.Race_MaxID) ||
      (Gender < EQRG_MALE) || (Gender > EQRG_NEUTRAL))
  {
    return false;
  }

  WORD _i;
  EQModel* _oModel;
  char* _sModelCode;
  BYTE _iModelCodeLen;

  _sModelCode = EQ::Client.GetModelCode(RaceID, Gender);
  if (IsBlank(_sModelCode))
  {
    return false;
  }

  _iModelCodeLen = (BYTE)strlen(_sModelCode);

  for (_i = 0; _i < this->Models; _i++)
  {
    _oModel = this->Model[_i];

    // Originally checked with RaceID==RaceID & Gender==Gender, but some codes are used
    // for multiple Race/Gender combos. Grr.
    if (StringSame(_oModel->Code, _oModel->CodeLen, _sModelCode, _iModelCodeLen, false))
    {
      if (ModelDef)
      {
        _oModel->RaceID = RaceID;
        _oModel->Gender = Gender;
        
        *ModelDef = _oModel;
      }
    
      return true;
    }
  }

  return false;
}

bool EQSource::HasRace(WORD RaceID)
{
  WORD _i;

  // Originally worked off of RaceID, which was quicker, but that
  // doesn't take into account re-used model codes for multiple races.

  BYTE  _iModelCodeLen[EQRACE_GENDERCOUNT];
  char* _sModelCode[EQRACE_GENDERCOUNT];
  BYTE _g;
  EQModel* _oModel;

  for (_g = 0; _g < EQRACE_GENDERCOUNT; _g++)
  {
    _sModelCode[_g] = EQ::Client.GetModelCode(RaceID, (EQGENDER)(_g + EQRG_MALE));

    if (!_sModelCode[_g])
    {
      _sModelCode[_g] = "";
      _iModelCodeLen[_g] = 0;
    }
    else
    {
      _iModelCodeLen[_g] = strlen(_sModelCode[_g]);
    }
  }
  
  for (_i = 0; _i < this->Models; _i++)
  {
    _oModel = this->Model[_i];

    if (_oModel->RaceID == RaceID)
    {
      return true;
    }

    for (_g = 0; _g < EQRACE_GENDERCOUNT; _g++)
    {
      if (StringSame(_sModelCode[_g], _iModelCodeLen[_g], _oModel->Code, _oModel->CodeLen, false))
      {
        return true;
      }
    }
  }

  return false;
}

// ---------------------
// EQStringList Member Functions
// ---------------------

EQStringList::~EQStringList()
{
  this->_Close();
}

void EQStringList::_Close()
{
  SAFE_DELETE(this->aStringList_Bytes);

  ZeroMemory(this, sizeof(*this));
}

char* EQStringList::GetText(WORD StringID)
{
  if (this->Count == 0)
  {
    return NULL; // We haven't loaded strings yet!
  }

  if (StringID > this->MaxID)
  {
    return NULL; // Invalid StringID
  }

  if (this->iStringList_Line[StringID] == EQSTRLIST_INVALID)
  {
    return NULL; // StringID not found
  }

  return this->aStringList_Text[this->iStringList_Line[StringID]];
}

HRESULT EQStringList::Load(char* Filename)
{
  if (!Filename)
  {
    return E_INVALIDARG;
  }

  FILE* _oFile;
  DWORD _iFileSize;

  if (fopen_s(&_oFile, Filename, "rb"))
  {
    return E_FAIL;
  }
  
  if (this->aStringList_Bytes)
  {
    delete this->aStringList_Bytes;
  }

  this->Count = 0;
  this->MaxID = 0;

  _iFileSize = _filelength(_fileno(_oFile));
  
  if (!(this->aStringList_Bytes = new char[_iFileSize + 1]))
  {
    fclose(_oFile);
    
    return E_OUTOFMEMORY;
  }

  if (fread_s(this->aStringList_Bytes, _iFileSize, _iFileSize, 1, _oFile) != 1)
  {
    fclose(_oFile);

    delete this->aStringList_Bytes;

    this->aStringList_Bytes = NULL;

    return E_FAIL;
  }
  
  fclose(_oFile);

  char* _sStringID = NULL;
  char* _sString = NULL;
  char* _sNextStringID = this->aStringList_Bytes;
  char* _sLastFileByte = &this->aStringList_Bytes[_iFileSize];
  bool _bDone = false;

  _sLastFileByte[0] = 0x00;
  
  this->Count = 0;

  DWORD _iStringID;

  memset(this->iStringList_Line, 0xFF, sizeof(this->iStringList_Line));

  while (!_bDone)
  {
    _sStringID = _sNextStringID;
    _bDone = (_sNextStringID = (char*)memchr(_sStringID, '\n', _sLastFileByte - _sStringID)) == NULL;

    if (!_sNextStringID)
    {
      _sNextStringID = _sLastFileByte;
    }

    _sString = (char*)memchr(_sStringID, ' ', _sNextStringID - _sStringID);
    if (_sString)
    {
      _sString[0] = 0x00;
      _sString++;
    }
    else
    {
      _sString = _sNextStringID; // No string for this ID. Skip!
    }

    _sNextStringID[0] = 0x00;
    _sNextStringID++;

    // _sStringID now has our string ID, null-terminated
    // _sString now has our string text, null-terminated
    // _sNextStringID now has our next line of text (unless EOF)

    TrimString(_sStringID);
    _iStringID = atoi(_sStringID);

    // Skip invalid StringIDs or StringID 0 (which doesn't matter anyway)
    if ((_iStringID > 0) && (_iStringID < EQSTRLIST_MAXID))
    {
      TrimString(_sString);

      // Add this string to index
      this->iStringList_Line[_iStringID] = this->Count;
      this->aStringList_Text[this->Count] = _sString;

      if (_iStringID > this->MaxID)
      {
        this->MaxID = (WORD)_iStringID;
      }

      this->Count++;

      _bDone |= (this->Count == 0) || (this->Count >= EQSTRLIST_MAXCOUNT);
    }
  }
  
  return S_OK;
}

// ---------------------
// EQZone Member Functions
// ---------------------

HRESULT EQZone::AddImportModel(EQModel* Model)
{
  if (!Model)
  {
    return E_INVALIDARG;
  }

  WORD _i;

  for (_i = 0; _i < this->ImportModels; _i++)
  {
    if (this->ImportModel[_i] == Model)
    {
      // Duplicate. Ignore.
      
      return S_OK;
    }
  }

  if (this->ImportModels >= EQZONE_MAXIMPORTMODELS)
  {
    // Too many models imported into this zone. This should not be happening!
    
    return E_OUTOFMEMORY;
  }

  this->ImportModel[this->ImportModels++] = Model;

  return S_OK;
}


char* EQZone::GetStartDesc()
{
  return EQ::DBStrings.GetText(EQDB_STARTZONEDESC, this->ID);
}

bool EQZone::HasModel(WORD RaceID, EQGENDER Gender, EQModel** ModelDef)
{
  if (ModelDef)
  {
    *ModelDef = NULL; // Preliminary, in case of errors.
  }

  char* _sModelCode;
  BYTE  _iModelCodeLen;

  if (IsBlank(_sModelCode = EQ::Client.GetModelCode(RaceID, Gender)))
  {
    return false;
  }
      
  if ((this->ModelSource1) && (this->ModelSource1->HasModel(RaceID, Gender, ModelDef)))
  {
    // It's in ZoneName_chr.s3d
    return true;
  }

  if ((this->ModelSource2) && (this->ModelSource2->HasModel(RaceID, Gender, ModelDef)))
  {
    // It's in ZoneName_chr2.s3d
    return true;
  }

  _iModelCodeLen = (BYTE)strlen(_sModelCode);

  // We'll have to see if it's been imported via ZoneName_chr.txt
  
  WORD      _i;
  EQModel*  _oModelDef;

  for (_i = 0; _i < this->ImportModels; _i++)
  {
    _oModelDef = this->ImportModel[_i];

    // Originally checked with RaceID==RaceID & Gender==Gender, but some codes are used
    // for multiple Race/Gender combos. Grr.
    if (StringSame(_oModelDef->Code, _oModelDef->CodeLen, _sModelCode, _iModelCodeLen, false))
    {
      if (ModelDef)
      {
        *ModelDef = _oModelDef;
      }
    
      return true;
    }
  }
  
  return false;
}

bool EQZone::IsImport(EQModel* Model)
{
  if (!Model)
  {
    return false;
  }

  if ((Model->Source == this->ModelSource1) || (Model->Source == this->ModelSource2))
  {
    return false;
  }
      
  WORD _i;

  for (_i = 0; _i < this->ImportModels; _i++)
  {
    if (Model == this->ImportModel[_i])
    {
      return true;
    }
  }
  
  return false;
}

bool EQZone::HasRace(WORD RaceID)
{
  if ((RaceID == EQRACE_INVALID) || (RaceID > EQ::Client.Race_MaxID))
  {
    return false;
  }

  BYTE _g;

  for (_g = 0; _g <= EQRACE_GENDERCOUNT; _g++)
  {
    if (this->HasModel(RaceID, (EQGENDER)(_g + EQRG_MALE), NULL))
    {
      return true;
    }
  }

  return false;
}

bool EQZone::HasSource(EQSource* Source)
{
  if (!Source)
  {
    return false;
  }

  if (this->ModelSource1 == Source)
  {
    return true;
  }

  if (this->ModelSource2 == Source)
  {
    return true;
  }

  BYTE _i;

  for (_i = 0; _i < this->ImportModels; _i++)
  {
    if (this->ImportModel[_i]->Source == Source)
    {
      return true;
    }
  }

  return false;
}
