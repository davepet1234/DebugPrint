/***********************************************************************

 DebugPrintTest.c
 
 Author: David Petrovic

 Test application to test and demonstate debug printing.
 
 Note: How to disable serial console in OVMF?
 https://edk2-devel.narkive.com/zWx0E8ku/edk2-how-to-disable-serial-console-in-ovmf
 
   sermode
   disconnect a2

***********************************************************************/

#include <Uefi.h>
#include <Library/UefiLib.h>
#include <Library/DebugLib.h>
#include <Library/ShellCEntryLib.h>


// set APP_DEBUG to 0 to remove all debug from code
#define APP_DEBUG 1
#include "DebugPrint.h"

STATIC VOID DoOutput(VOID)
{
    Print(L"\n");
    DbgPrint(DL_NONE, "DbgPrint None\n");
    DbgPrint(DL_INFO, "DbgPrint Info\n");
    DbgPrint(DL_WARN, "DbgPrint Warning\n");
    DbgPrint(DL_ERROR, "DbgPrint Error\n");
    DbgPrint(DL_TRACE, "DbgPrint Trace\n");
    TRACE("TRACE\n");
}

INTN
EFIAPI
ShellAppMain (
  IN UINTN Argc,
  IN CHAR16 **Argv
  )
{
    Print(L"DebugPrint Application\n\n");
    DbgInit(DO_ALL, DL_ALL);
    DbgSerialInfo();
    DoOutput();    
    DbgSetLevel(DL_NONE | DL_INFO | DL_WARN);
    DoOutput();    
    DbgSetLevel(DL_ERROR);
    DoOutput();    
   
    return 0;
}
