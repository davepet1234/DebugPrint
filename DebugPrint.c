/***********************************************************************

 DebugPrint.c
 
 Author: David Petrovic

***********************************************************************/

#include <Uefi.h>
#include <Library/UefiLib.h>
#include <Library/PrintLib.h>
#include <Library/IoLib.h>
#include <Protocol/SerialIo.h>
#include <Library/UefiBootServicesTableLib.h>
#include "DebugPrint.h"

#if APP_DEBUG

#define STRING_SIZE 256

// debug serial support
typedef enum {
    DS_NONE,        // no serial
    DS_DRIVER,      // SerialIo protocol
    DS_DIRECT,      // direct acces to IO port
    DS_AUTO         // auto select; SerialIo first then Direct
} DebugSerialType;

STATIC BOOLEAN DebugInitialised =  FALSE;
STATIC DebugSerialType DebugSerial = DS_AUTO;
STATIC EFI_SERIAL_IO_PROTOCOL *DebugSerialIo = NULL;
STATIC UINTN DebugOutput = DO_NONE;
STATIC UINT16 DebugLevel = DL_NONE;

// Direct access to COM1
// Ref: https://www.lookrs232.com/rs232/registers.htm
#define COM_BASE        0x3F8
#define COM_DLL_OFF         0x00    // Divisor Latch Low Byte (DLAB=1)
#define COM_DLH_OFF         0x01    // Divisor Latch High Byte (DLAB=1)
#define COM_LCR_OFF         0x03    // Line Control Register
#define COM_LCR_DLAB_BIT    0x80    // DLAB bit for accessing the baud rate divisor registers
#define COM_LSR_OFF         0x05    // Line Status Register
#define COM_LSR_EDHR_BIT    0x40    // LSR Bit 6: Empty Data Holding Registers
#define COM_SCRATCH_OFF     0x07    // Scratch Register
// port settings
#define COM_BAUD_DLL        0x01    // 115200 baud
#define COM_BAUD_DLH        0x00
#define COM_LCR_SETUP       0x03    // 8 bits; 1 stop bit, no parity

STATIC BOOLEAN InitSerialDirect()
{
    // check scratch register to see if port present
    IoWrite8(COM_BASE+COM_SCRATCH_OFF, 0x55);
    if (IoRead8(COM_BASE+COM_SCRATCH_OFF)  != 0x55) {
        return FALSE;
    }
    IoWrite8(COM_BASE+COM_SCRATCH_OFF, 0xAA);
    if (IoRead8(COM_BASE+COM_SCRATCH_OFF) != 0xAA) {
        return FALSE;
    }
    // configure port to 115200n81
    IoWrite8(COM_BASE+COM_LCR_OFF, COM_LCR_SETUP | COM_LCR_DLAB_BIT);
    IoWrite8(COM_BASE+COM_DLL_OFF, COM_BAUD_DLL);
    IoWrite8(COM_BASE+COM_DLH_OFF, COM_BAUD_DLH);
    IoWrite8(COM_BASE+COM_LCR_OFF, COM_LCR_SETUP);
    
    return TRUE;
}

STATIC VOID Com1Print(CHAR8 *String)
{
    UINTN i=0;
    while (String[i]) {
        if (String[i] == '\n'){ // do "\r\n"
            while ( (IoRead8(COM_BASE+COM_LSR_OFF) & COM_LSR_EDHR_BIT) == 0 );
            IoWrite8(COM_BASE, '\r');
        }
        while ( (IoRead8(COM_BASE+COM_LSR_OFF) & COM_LSR_EDHR_BIT) == 0 );
        IoWrite8(COM_BASE, String[i]);
        i++;
    }
}

STATIC BOOLEAN InitSerialDriver()
{
    EFI_HANDLE *handles = NULL;
    UINTN count = 0;
    EFI_STATUS status = gBS->LocateHandleBuffer(ByProtocol, &gEfiSerialIoProtocolGuid, NULL, &count, &handles);
    for (UINTN i=0; i<count; i++) {
        EFI_SERIAL_IO_PROTOCOL *SerialIo = NULL;
        status = gBS->HandleProtocol(handles[i], &gEfiSerialIoProtocolGuid, (void **)&SerialIo);
        if (EFI_ERROR(status)) {
            break;
        } else if (SerialIo != NULL) {
            // use first serial port
            DebugSerialIo = SerialIo;
            break;
        }
    }
    if (handles) {
        gBS->FreePool(handles);    
    }
    return DebugSerialIo ? TRUE : FALSE;
}

VOID DbgInit(UINTN Output, UINT32 Level)
{
    DebugInitialised = TRUE;
    DebugOutput = Output;
    DebugLevel = Level;

    switch (DebugSerial) {
    case DS_DRIVER:
        // SerialIo protocol 
        if (!InitSerialDriver()) {
            Print(L"No SerialIo Protocol\n");
            DebugSerial = DS_NONE;
        }
        break;
    case DS_DIRECT:
        // Direct access to COM1 I/O registers
        if (!InitSerialDirect()) {
            Print(L"No COM port found\n");
            DebugSerial = DS_NONE;
        }
        break;
    case DS_AUTO:
        // If SerialIo protocol not available then try direct access to COM1
        if (InitSerialDriver()) {
            DebugSerial = DS_DRIVER;
        } else if (InitSerialDirect()) {
            DebugSerial = DS_DIRECT;
        } else {
            Print(L"No serial output");
            DebugSerial = DS_NONE;
        }
        break;
    default:
        DebugSerial = DS_NONE;
        break;
    }
}

VOID DbgSerialInfo(VOID)
{
    if (!DebugInitialised) {
        return;
    }
    switch (DebugSerial) {
    case DS_DRIVER:
        EFI_SERIAL_IO_MODE *Mode = DebugSerialIo->Mode;
        Print(L"SerialIo Protocol (%p)\n", DebugSerialIo);
        Print(L"├──Timeout          = %u\n", Mode->Timeout);
        Print(L"├──BaudRate         = %lu\n", Mode->BaudRate);
        Print(L"├──ReceiveFifoDepth = %u\n", Mode->ReceiveFifoDepth);
        Print(L"├──DataBits         = %u\n", Mode->DataBits);
        Print(L"├──Parity           = %u\n", Mode->Parity);
        Print(L"└──StopBits         = %u\n", Mode->StopBits);
        break;
    case DS_DIRECT:
        Print(L"Serial Direct\n");
        Print(L"└──COM1 base address: 0x%03x\n", COM_BASE);
        break;
    default:
        Print(L"No serial output available");
        break;
    }
}

VOID DbgSetLevel(UINT32 Level)
{
    DebugLevel = Level;
}

VOID EFIAPI DbgPrint(UINTN Level, CHAR8 *sFormat, ...)
{
    if ( !DebugInitialised || !(Level & DebugLevel) ) {
        return;
    }   
    CHAR8 String[STRING_SIZE];
    if (Level & DL_INFO) {
        AsciiStrCpyS(String, STRING_SIZE, "INFO: ");
    } else if (Level & DL_WARN) {
        AsciiStrCpyS(String, STRING_SIZE, "WARN: ");
    } else if (Level & DL_ERROR) {
        AsciiStrCpyS(String, STRING_SIZE, "ERROR: ");
    } else {
        // includes DL_TRACE
        String[0] = '\0';
    }
    UINTN offset = AsciiStrLen(String);
    VA_LIST vl;
    VA_START(vl, sFormat);
    UINTN Length = AsciiVSPrint(&String[offset], STRING_SIZE, sFormat, vl);
    VA_END(vl);
    Length += offset;
    if (DebugOutput & DO_CONSOLE) {
        Print(L"%a", String);
    }
    if (DebugOutput & DO_SERIAL) {
        switch (DebugSerial) {
        case DS_DRIVER:
            DebugSerialIo->Write(DebugSerialIo, &Length, String);
            break;
        case DS_DIRECT:
            Com1Print(String);
            break;
        default:
            break;
        }
    }
}

#endif // APP_DEBUG
