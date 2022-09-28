/***********************************************************************

 DebugPrint.h
 
 Author: David Petrovic
 
***********************************************************************/

#ifndef INC_DEBUG_H
#define INC_DEBUG_H

#include <Uefi.h>

// APP_DEBUG is global debug enable
// Defines if debug code is compiled into your program or not
#ifndef APP_DEBUG
#define APP_DEBUG 1
#endif

// debug output
#define DO_NONE     0x00
#define DO_CONSOLE  0x01
#define DO_SERIAL   0x02
#define DO_ALL      0xFF

// debug level
#define DL_NONE     0x0000
#define DL_TRACE    0x0001
#define DL_INFO     0x0002
#define DL_WARN     0x0004
#define DL_ERROR    0x0008
#define DL_ALL      0xFFFF

#if APP_DEBUG

VOID DbgInit(UINTN Output, UINT32 Level);
VOID DbgSerialInfo(VOID);
VOID DbgSetLevel(UINT32 Level);
VOID EFIAPI DbgPrint(UINTN Level, CHAR8 *sFormat, ...);
#define TRACE(...) DbgPrint(DL_TRACE, __VA_ARGS__)

#else

#define DbgInit(Output, Level)
#define DbgSetLevel(Level)
#define DbgPrint(Level, sFormat, ...)
#define DbgSerialInfo()
#define TRACE(...)

#endif // APP_DEBUG


#endif // INC_DEBUG_H
