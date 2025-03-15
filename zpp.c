/******************************************************************************
file:       Zpp
author:     Robbert de Groot
copyright:  2014, Robbert de Groot

description:
Zekaric Pre Processor.
******************************************************************************/

/******************************************************************************
include:
******************************************************************************/
#include <stdio.h>

#include "grl.h"

/******************************************************************************
local:
constant:
******************************************************************************/
#define zsHELP \
   "zpp\n"\
   "===\n\n"\
   "zpp [datFile] [file.c] [[outfile.c]]\n\n"\
   "datFile   - The file the contains the macros.\n"\
   "file.c    - The c file that holds zpp directives.\n"\
   "outfile.c - (optional) The c output file.\n"\

/******************************************************************************
type:
******************************************************************************/
typedef enum
{
   strIndexCREATE,
   strIndexCREATE_CONTENT,
   strIndexDESTROY,
   strIndexDESTROY_CONTENT,
   strIndexFUNC,
   strIndexGET,
   strIndexHEADER,
   strIndexIS,
   strIndexLOCAL,
   strIndexSET,
   strIndexSTART,
   strIndexSTOP,

   strIndexCOUNT
} StrIndex;

typedef enum
{
   partCOMMAND,

   partREPLACE_START,

   partTYPE             = partREPLACE_START, //lint !e960
   partTYPE_VAR,
   partCOPYRIGHT,
   partCOMPANY,
   partAUTHOR,
   partCOPYRIGHT_HOLDER,
   partPARAM,
   partPARAM_DEF,
   partDEF,
   partVAR0,
   partVAR0_NAME,
   partVAR0_TYPE,

   partREPLACE_STOP,

   partVAR1             = partREPLACE_STOP,
   partVAR1_NAME,
   partVAR1_TYPE,
   partVAR2,
   partVAR2_NAME,
   partVAR2_TYPE,
   partVAR3,
   partVAR3_NAME,
   partVAR3_TYPE,
   partVAR4,
   partVAR4_NAME,
   partVAR4_TYPE,
   partVAR5,
   partVAR5_NAME,
   partVAR5_TYPE,
   partVAR6,
   partVAR6_NAME,
   partVAR6_TYPE,
   partVAR7,
   partVAR7_NAME,
   partVAR7_TYPE,
   partVAR8,
   partVAR8_NAME,
   partVAR8_TYPE,
   partVAR9,
   partVAR9_NAME,
   partVAR9_TYPE,

   partCOUNT
} Part;

typedef enum
{
   keywordAUTHOR,
   keywordCOMPANY,
   keywordCOPYRIGHT,
   keywordVAR,
   keywordDEF,

   keywordCOUNT
} Keyword;

/******************************************************************************
variable:
******************************************************************************/
static GsHashKey     *_strTable  = NULL;
static GsKey const   *_part[partCOUNT];            //lint !e956
static GsKey const   *_keyword[keywordCOUNT];

/******************************************************************************
prototype:
******************************************************************************/
static Gb          _Start(             Char const *datFile);
static Gb          _StartLoad(         Gpath const * const path);
static void        _Stop(              void);

static Gs         *_ParseCloc(         GsArray *block, Gs *type, Gs *typeVar, Gb isCloc);
static Gs         *_ParseDloc(         GsArray *block, Gs *type, Gs *typeVar, Gb isDloc);
static Gs         *_ParseFunc(         GsArray *block, Gs *type, Gs *typeVar);
static Gs         *_ParseGet(          GsArray *block, Gs *type, Gs *typeVar);
static Gs         *_ParseIs(           GsArray *block, Gs *type, Gs *typeVar);
static Gs         *_ParseHeader(       GsArray *block, Gs *type, Gs *typeVar);
static Gs         *_ParseLocal(        GsArray *block, Gs *type, Gs *typeVar);
static Gs         *_ParseSet(          GsArray *block, Gs *type, Gs *typeVar);
static Gs         *_ParseStart(        GsArray *block, Gs *type, Gs *typeVar);
static Gs         *_ParseStop(         GsArray *block, Gs *type, Gs *typeVar);
static Gb          _ParseString(       Gs *str, Gs *aword);
static Gb          _ParseWord(         Gs *str, Gs *aword);
static Gb          _Process(           Char *ifile, Char *ofile);
static Gb          _ProcessBlock(      GsArray *block, GsArray *olines);
static Gs         *_ProcessBlockDump(  GsArray *block);
static Gb          _ProcessLines(      GsArray *ilines, GsArray *olines);

static void        _StrTableDloc(      GsKey const * const skey, Gs * const value);

/******************************************************************************
global:
function:
******************************************************************************/
/******************************************************************************
func: main
******************************************************************************/
Gi4 main(Gi4 acount, Char **alist)
{
   Gb result;

   result = gbFALSE;

   returnIf(!grlStart(), 1);

   _Start(alist[1]);

   if (acount < 2)
   {
      printf("%s", zsHELP);
      return 0;
   }

   printf("%s %s %s\n", alist[0], alist[1], alist[2]);

   result = _Process(alist[2], (acount == 4) ? alist[3] : NULL);

   _Stop();

   grlStop();

   return result;
}

/******************************************************************************
local:
function:
******************************************************************************/
/******************************************************************************
func: _Start
******************************************************************************/
Gb _Start(char const *datFile)
{
   Gpath *path;

   genter;

   _part[partCOMMAND]            = gsKeyInternA("[command]");
   _part[partTYPE]               = gsKeyInternA("[type]");
   _part[partTYPE_VAR]           = gsKeyInternA("[typeVar]");
   _part[partCOMPANY]            = gsKeyInternA("[company]");
   _part[partCOPYRIGHT]          = gsKeyInternA("[copyright]");
   _part[partAUTHOR]             = gsKeyInternA("[author]");
   _part[partCOPYRIGHT_HOLDER]   = gsKeyInternA("[name]");
   _part[partPARAM]              = gsKeyInternA("[param]");
   _part[partPARAM_DEF]          = gsKeyInternA("[paramDef]");
   _part[partDEF]                = gsKeyInternA("[def]");
   _part[partVAR0]               = gsKeyInternA("[var]");
   _part[partVAR0_NAME]          = gsKeyInternA("[varName]");
   _part[partVAR0_TYPE]          = gsKeyInternA("[varType]");
   _part[partVAR1]               = gsKeyInternA("[var1]");
   _part[partVAR1_NAME]          = gsKeyInternA("[var1Name]");
   _part[partVAR1_TYPE]          = gsKeyInternA("[var1Type]");
   _part[partVAR2]               = gsKeyInternA("[var2]");
   _part[partVAR2_NAME]          = gsKeyInternA("[var2Name]");
   _part[partVAR2_TYPE]          = gsKeyInternA("[var2Type]");
   _part[partVAR3]               = gsKeyInternA("[var3]");
   _part[partVAR3_NAME]          = gsKeyInternA("[var3Name]");
   _part[partVAR3_TYPE]          = gsKeyInternA("[var3Type]");
   _part[partVAR4]               = gsKeyInternA("[var4]");
   _part[partVAR4_NAME]          = gsKeyInternA("[var4Name]");
   _part[partVAR4_TYPE]          = gsKeyInternA("[var4Type]");
   _part[partVAR5]               = gsKeyInternA("[var5]");
   _part[partVAR5_NAME]          = gsKeyInternA("[var5Name]");
   _part[partVAR5_TYPE]          = gsKeyInternA("[var5Type]");
   _part[partVAR6]               = gsKeyInternA("[var6]");
   _part[partVAR6_NAME]          = gsKeyInternA("[var6Name]");
   _part[partVAR6_TYPE]          = gsKeyInternA("[var6Type]");
   _part[partVAR7]               = gsKeyInternA("[var7]");
   _part[partVAR7_NAME]          = gsKeyInternA("[var7Name]");
   _part[partVAR7_TYPE]          = gsKeyInternA("[var7Type]");
   _part[partVAR8]               = gsKeyInternA("[var8]");
   _part[partVAR8_NAME]          = gsKeyInternA("[var8Name]");
   _part[partVAR8_TYPE]          = gsKeyInternA("[var8Type]");
   _part[partVAR9]               = gsKeyInternA("[var9]");
   _part[partVAR9_NAME]          = gsKeyInternA("[var9Name]");
   _part[partVAR9_TYPE]          = gsKeyInternA("[var9Type]");

   _keyword[keywordAUTHOR]       = gsKeyInternA("author");
   _keyword[keywordCOMPANY]      = gsKeyInternA("company");
   _keyword[keywordCOPYRIGHT]    = gsKeyInternA("copyright");
   _keyword[keywordVAR]          = gsKeyInternA("var");
   _keyword[keywordDEF]          = gsKeyInternA("def");

   path = gsClocFromA(datFile);
   gpathSetFromSystem(path);

   _strTable = gsHashKeyClocLoad(ghashSize100, path);

   gsDloc(path);

   greturnFalseIf(!_strTable);

   greturn gbTRUE;
}

/******************************************************************************
func: _Stop
******************************************************************************/
void _Stop(void)
{
   genter;

   gsHashKeyForEach(_strTable, (GrlForEachKeyFunc) _StrTableDloc);
   gsHashKeyDloc(_strTable);
   _strTable = NULL;

   greturn;
}

/******************************************************************************
func: _ParseBlockTable
******************************************************************************/
Gs *_ParseBlockTable(GsHashKey *stable)
{
   Gindex  index;
   Gs     *str,
          *valueStr;

   genter;

   // Get the command of the block.
   str = gsHashKeyFind(stable, _part[partCOMMAND]);
   greturnNullIf(!str);

   // Find the subsitute string for the block.
   str = gsHashKeyFind(_strTable, gsKeyIntern(str));
   str = gsClocFrom(str);

   // Process that block
   for (index = partREPLACE_START; index < partREPLACE_STOP; index++)
   {
      valueStr = gsHashKeyFind(stable, _part[index]);
      continueIf(!valueStr);

      gsFindAndReplace(str, _part[index], valueStr, NULL);
   }

   greturn str;
}

/******************************************************************************
func: _ParseString
******************************************************************************/
Gb _ParseString(Gs *str, Gs *aword)
{
   genter;

   gsFlush(aword);
   gsAppend(aword, str);

   loop
   {
      if (*gsGetAt(aword, gsGetCount(aword) - 1) == L'\n' ||
          *gsGetAt(aword, gsGetCount(aword) - 1) == L'\r' ||
          *gsGetAt(aword, gsGetCount(aword) - 1) == L' ')
      {
         gsSetCount(aword, gsGetCount(aword) - 1);
         continue;
      }
      break;
   }

   gsFlush(str);

   greturn gbTRUE;
}

/******************************************************************************
func: _ParseWord
******************************************************************************/
Gb _ParseWord(Gs *str, Gs *aword)
{
   Gindex index;
   Gc2    letter;

   genter;

   // Get the first word.
   forCount(index, gsGetCount(str))
   {
      letter = *gsGetAt(str, index);
      breakIf(
         letter == ' '  ||
         letter == '\n' ||
         letter == '\r');

      gsAddEnd(aword, &letter);
   }

   // skip spaces
   for (; index < gsGetCount(str); index++)
   {
      letter = *gsGetAt(str, index);
      breakIf(
         !(letter == ' '  ||
           letter == '\n' ||
           letter == '\r'));
   }

   // Remove the parsed item.
   gsEraseAt(str, index, 0);

   greturn gbTRUE;
}

/******************************************************************************
func: _Process
******************************************************************************/
Gb _Process(char *ifile, char *ofile)
{
   Gb           result;
   Gindex       index;
   Gs          *ifilename,
               *ofilename;
   GsArray     *ilines,
               *olines;
   GfileCreate  fileCreateResult;

   genter;

   ifilename    =
      ofilename = NULL;
   result       = gbFALSE;
   ilines       = NULL;
   olines       = gsArrayCloc((GrlCompareFunc) NULL, gbTRUE);
   index        = 0;

   ifilename = gsClocFromA(ifile);
   if (ofile)
   {
      ofilename = gsClocFromA(ofile);
   }
   else
   {
      ofilename = gsClocFrom(ifilename);
   }

   gpathSetFromSystem(ifilename);
   gpathSetFromSystem(ofilename);
   ilines = gsArrayClocLoad(ifilename);
   stopIf(!ilines);

   // Find all the macros and do the replacement.
   stopIf(!_ProcessLines(ilines, olines));

   // There was a macro replacement.
   fileCreateResult = gfileCreateFromStrArray(ofilename, gcTypeA, olines);
   switch(fileCreateResult)
   {
   case gfileCreateSUCCESS:
      result = gbTRUE;
      break;

   case gfileCreateBAD_PATH:
   case gfileCreateFAILED_TO_OPEN_FILE:
   case gfileCreateFAILED_TO_COMPLETELY_WRITE_FILE:
   default:
      break;
   }

STOP:
   // clean up.
   gsDloc(ifilename);
   gsDloc(ofilename);

   gsArrayForEach(ilines, (GrlForEachFunc) gsDlocFunc);
   gsArrayDloc(ilines);
   gsArrayForEach(olines, (GrlForEachFunc) gsDlocFunc);
   gsArrayDloc(olines);

   greturn result;
}

/******************************************************************************
func: _ProcessBlock
******************************************************************************/
Gb _ProcessBlock(GsArray *block, GsArray *olines)
{
   Gi4          var;
   Gindex       line,
                index;
   Gs          *stemp,
               *parsedStr,
               *value,
               *key;
   GsKey const *keyword;
   GsHashKey   *stable;
   Gc2          letter;

   genter;

   // parse the command.
   var     = 0;
   key     = gsCloc();
   stable  = gsHashKeyCloc(ghashSize10);

   stemp   = gsClocFromSub(gsArrayGetAt(block, 0), 1, gsSubStrINDEX_END);

   value = gsCloc();
   _ParseWord(stemp, value);
   gsHashKeyAdd(stable, _part[partCOMMAND], value);

   value = gsCloc();
   _ParseWord(stemp, value);
   gsHashKeyAdd(stable, _part[partTYPE], value);

   gsDloc(stemp);

   value = gsClocFrom(value);
   gsUpdateAt(value, 0, (letter = gcToLowerCase(*gsGetAt(value, 0)), &letter));
   gsHashKeyAdd(stable, _part[partTYPE_VAR], value);

   for (line = 1; line < gsArrayGetCount(block); line++)
   {
      stemp = gsClocFromSub(gsArrayGetAt(block, line), 1, gsSubStrINDEX_END);

      gsFlush(key);
      _ParseWord(stemp, key);

      keyword = gsKeyIntern(key);
      if      (keyword == _keyword[keywordAUTHOR])
      {
         value = gsCloc();

         _ParseString(stemp, value);
         gsHashKeyAdd(stable, _part[partAUTHOR], value);
      }
      else if (keyword == _keyword[keywordCOMPANY])
      {
         value = gsCloc();

         _ParseString(stemp, value);
         gsHashKeyAdd(stable, _part[partCOMPANY], value);
      }
      else if (keyword == _keyword[keywordCOPYRIGHT])
      {
         value = gsCloc();

         _ParseString(stemp, value);
         gsHashKeyAdd(stable, _part[partCOPYRIGHT], value);
      }
      else if (keyword == _keyword[keywordVAR] &&
               var < 10)
      {
         value = gsCloc();

         _ParseWord(stemp, value);
         gsHashKeyAdd(stable, _part[partVAR0 + var * 3], value);

         value = gsClocFrom(value);

         gsUpdateAt(value, 0, (letter = gcToUpperCase(*gsGetAt(value, 0)), &letter));
         gsHashKeyAdd(stable, _part[partVAR0_NAME + var * 3], value);

         value = gsCloc();

         _ParseString(stemp, value);
         gsHashKeyAdd(stable, _part[partVAR0_TYPE + var * 3], value);

         var++;
      }
      else if (keyword == _keyword[keywordDEF])
      {
         value = gsCloc();

         _ParseString(stemp, value);
         gsHashKeyAdd(stable, _part[partDEF], value);
      }

      gsDloc(stemp);
   }

   // Cloc the param list.
   value = gsCloc();
   forCount(index, var)
   {
      if (index)
      {
         gsAppendA(value, ", ");
      }
      gsAppend(
         value,
         gsHashKeyFind(stable, _part[partVAR0 + index * 3]));
   }
   gsHashKeyAdd(stable, _part[partPARAM], value);

   value = gsCloc();
   forCount(index, var)
   {
      if (index)
      {
         gsAppendA(value, ", ");
      }
      gsAppend(
         value,
         gsHashKeyFind(stable, _part[partVAR0_TYPE + index * 3]));
      gsAppendC(value, ' ');
      gsAppend(
         value,
         gsHashKeyFind(stable, _part[partVAR0      + index * 3]));
   }
   gsHashKeyAdd(stable, _part[partPARAM_DEF], value);

   // Set the copyright holder.
   if      (gsHashKeyFind(stable, _part[partCOMPANY]))
   {
      gsHashKeyAdd(
         stable,
         _part[partCOPYRIGHT_HOLDER],
         gsClocFrom(
            gsHashKeyFind(stable, _part[partCOMPANY])));
   }
   else if (gsHashKeyFind(stable, _part[partAUTHOR]))
   {
      gsHashKeyAdd(
         stable,
         _part[partCOPYRIGHT_HOLDER],
         gsClocFrom(
            gsHashKeyFind(stable, _part[partAUTHOR])));
   }

   // figure out the command to process
   parsedStr = _ParseBlockTable(stable);
   if (!parsedStr)
   {
      // Bad processor coommand.
      parsedStr = _ProcessBlockDump(block);
   }

   gsArrayAddEnd(olines, parsedStr);

   // Clean up.
   gsHashKeyForEach(stable, (GrlForEachKeyFunc) _StrTableDloc);
   gsHashKeyDloc(stable);
   gsDloc(key);

   greturn gbTRUE;
}

/******************************************************************************
func: _ProcessBlockDump
******************************************************************************/
Gs *_ProcessBlockDump(GsArray *block)
{
   Gindex index;
   Gs    *str;

   genter;

   str = gsCloc();

   forCount(index, gsArrayGetCount(block))
   {
      gsAppend(str, gsArrayGetAt(block, index));
   }

   greturn str;
}

/******************************************************************************
func: _ProcessLines
******************************************************************************/
Gb _ProcessLines(GsArray *ilines, GsArray *olines)
{
   Gindex     index;
   Gs const  *str;
   Gb         result;
   GsArray   *block;

   genter;

   result = gbFALSE;
   forCount(index, gsArrayGetCount(ilines))
   {
      str = gsArrayGetAt(ilines, index);
      breakIf(!str);

      // Found a preprocessor block.
      if (*gsGetAt(str, 0) == L'`')
      {
         block = gsArrayCloc(NULL, gbTRUE);

         // Get all the lines in the preprocessor block.
         for (; index < gsArrayGetCount(ilines); index++)
         {
            str = gsArrayGetAt(ilines, index);
            breakIf(
               !str ||
               *gsGetAt(str, 0) != L'`');

            gsArrayAddEnd(block, gsClocFrom(str));
         }

         // Process the block.
         index--;
         _ProcessBlock(block, olines);

         gsArrayForEach(block, gsDlocFunc);
         gsArrayDloc(block);

         result = gbTRUE;
      }
      // Not a preprocessor block, copy the string over verbatum.
      else
      {
         gsArrayAddEnd(olines, gsClocFrom(str));
      }
   }

   greturn result;
}

/******************************************************************************
func: _StrTableDloc
******************************************************************************/
static void _StrTableDloc(GsKey const * const skey, Gs * const value)
{
   genter;
   skey;
   gsDloc(value);
   greturn;
}

//////////////////////////////
/*

///
`header Zpp
`author Robbert de Groot
`copyright 2014
`company Zekaric
///
`start Zpp
///
`stop Zpp
///
`get Zpp
`var dog Point *
`def NULL
///
`is Zpp
`var isSet
`def FALSE
///
`set Zpp
`var dog Point *
///
`create Zpp
`var cat Gxyz *
`var doc Gxyz *
`var isSet Gb
///
`createcontent Zpp
`var cat Gxyz *
`var doc Gxyz *
`var isSet Gb
///
`destroy Zpp
///
`destroycontent Zpp
///
`func Zpp
///
`local
///

*/
