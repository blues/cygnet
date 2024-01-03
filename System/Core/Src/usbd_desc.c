// Copyright 2023 Blues Inc.  All rights reserved.
// Use of this source code is governed by licenses granted by the
// copyright holder including that found in the LICENSE file.

#include "usbd_core.h"
#include "usbd_desc.h"
#include "usbd_conf.h"
#include "product.h"
#include <ctype.h>

#define FRIENDLY_SERIAL_NUMBER

#define USBD_VID                        PRODUCT_USB_VID
#define USBD_PID_FS                     PRODUCT_USB_PID
// (Do NOT add PID numbers within VID 12452 Inca Roads LLC;
// Please use 3 or submit paperwork and register your own)
#define USBD_LANGID_STRING              1033
#define USBD_MANUFACTURER_STRING        PRODUCT_MANUFACTURER
#define USBD_PRODUCT_STRING_FS          PRODUCT_DISPLAY_NAME
#define USBD_CONFIGURATION_STRING_FS    "CDC Config"
#define USBD_INTERFACE_STRING_FS        "CDC Interface"

#define USB_SIZ_BOS_DESC                0x0C

// Forwards
#ifndef FRIENDLY_SERIAL_NUMBER
static void Get_SerialNum(void);
static void IntToUnicode(uint32_t value, uint8_t * pbuf, uint8_t len);
#endif

uint8_t * USBD_FS_DeviceDescriptor(USBD_SpeedTypeDef speed, uint16_t *length);
uint8_t * USBD_FS_LangIDStrDescriptor(USBD_SpeedTypeDef speed, uint16_t *length);
uint8_t * USBD_FS_ManufacturerStrDescriptor(USBD_SpeedTypeDef speed, uint16_t *length);
uint8_t * USBD_FS_ProductStrDescriptor(USBD_SpeedTypeDef speed, uint16_t *length);
uint8_t * USBD_FS_SerialStrDescriptor(USBD_SpeedTypeDef speed, uint16_t *length);
uint8_t * USBD_FS_ConfigStrDescriptor(USBD_SpeedTypeDef speed, uint16_t *length);
uint8_t * USBD_FS_InterfaceStrDescriptor(USBD_SpeedTypeDef speed, uint16_t *length);
#if (USBD_LPM_ENABLED == 1)
uint8_t * USBD_FS_USR_BOSDescriptor(USBD_SpeedTypeDef speed, uint16_t *length);
#endif

USBD_DescriptorsTypeDef FS_Desc = {
    USBD_FS_DeviceDescriptor
    , USBD_FS_LangIDStrDescriptor
    , USBD_FS_ManufacturerStrDescriptor
    , USBD_FS_ProductStrDescriptor
    , USBD_FS_SerialStrDescriptor
    , USBD_FS_ConfigStrDescriptor
    , USBD_FS_InterfaceStrDescriptor
#if (USBD_LPM_ENABLED == 1)
    , USBD_FS_USR_BOSDescriptor
#endif
};

#if defined ( __ICCARM__ ) // IAR Compiler
#pragma data_alignment=4
#endif
// USB standard device descriptor
__ALIGN_BEGIN uint8_t USBD_FS_DeviceDesc[USB_LEN_DEV_DESC] __ALIGN_END = {
    0x12,                       // bLength
    USB_DESC_TYPE_DEVICE,       // bDescriptorType
#if (USBD_LPM_ENABLED == 1)
    0x01,                       // bcdUSB changed to USB version 2.
    // in order to support LPM L1 suspend
    // resume test of USBCV3.0
#else
    0x00,                       // bcdUSB
#endif
    0x02,
    0x02,                       // bDeviceClass
    0x02,                       // bDeviceSubClass
    0x00,                       // bDeviceProtocol
    USB_MAX_EP0_SIZE,           // bMaxPacketSize
    LOBYTE(USBD_VID),           // idVendor
    HIBYTE(USBD_VID),           // idVendor
    LOBYTE(USBD_PID_FS),        // idProduct
    HIBYTE(USBD_PID_FS),        // idProduct
    0x00,                       // bcdDevice rel. 2.00
    0x02,
    USBD_IDX_MFC_STR,           // Index of manufacturer  string
    USBD_IDX_PRODUCT_STR,       // Index of product string
    USBD_IDX_SERIAL_STR,        // Index of serial number string
    USBD_MAX_NUM_CONFIGURATION  // bNumConfigurations
};

//  USB_DeviceDescriptor
// * BOS descriptor.
#if (USBD_LPM_ENABLED == 1)
#if defined ( __ICCARM__ ) // IAR Compiler 
#pragma data_alignment=4
#endif
__ALIGN_BEGIN uint8_t USBD_FS_BOSDesc[USB_SIZ_BOS_DESC] __ALIGN_END = {
    0x5,
    USB_DESC_TYPE_BOS,
    0xC,
    0x0,
    0x1,  //  1 device capability
    //  device capability
    0x7,
    USB_DEVICE_CAPABITY_TYPE,
    0x2,
    0x2,  //  LPM capability bit set
    0x0,
    0x0,
    0x0
};
#endif

#if defined ( __ICCARM__ ) // IAR Compiler 
#pragma data_alignment=4
#endif

// * USB lang identifier descriptor.
__ALIGN_BEGIN uint8_t USBD_LangIDDesc[USB_LEN_LANGID_STR_DESC] __ALIGN_END = {
    USB_LEN_LANGID_STR_DESC,
    USB_DESC_TYPE_STRING,
    LOBYTE(USBD_LANGID_STRING),
    HIBYTE(USBD_LANGID_STRING)
};

#if defined ( __ICCARM__ ) // IAR Compiler 
#pragma data_alignment=4
#endif
// Internal string descriptor.
__ALIGN_BEGIN uint8_t USBD_StrDesc[USBD_MAX_STR_DESC_SIZ] __ALIGN_END;

#if defined ( __ICCARM__ ) // IAR Compiler 
#pragma data_alignment=4
#endif
__ALIGN_BEGIN uint8_t USBD_StringSerial[USB_SIZ_STRING_SERIAL] __ALIGN_END = {
    USB_SIZ_STRING_SERIAL,
    USB_DESC_TYPE_STRING,
};

// Return the device descriptor
// speed : Current device speed
// length : Pointer to data length variable
// returns : Pointer to descriptor buffer
uint8_t * USBD_FS_DeviceDescriptor(USBD_SpeedTypeDef speed, uint16_t *length)
{
    UNUSED(speed);
    *length = sizeof(USBD_FS_DeviceDesc);
    return USBD_FS_DeviceDesc;
}

// Return the LangID string descriptor
// speed : Current device speed
// length : Pointer to data length variable
// returns : Pointer to descriptor buffer
uint8_t * USBD_FS_LangIDStrDescriptor(USBD_SpeedTypeDef speed, uint16_t *length)
{
    UNUSED(speed);
    *length = sizeof(USBD_LangIDDesc);
    return USBD_LangIDDesc;
}

// Return the product string descriptor
// speed : Current device speed
// length : Pointer to data length variable
// returns : Pointer to descriptor buffer
uint8_t * USBD_FS_ProductStrDescriptor(USBD_SpeedTypeDef speed, uint16_t *length)
{
    if(speed == 0) {
        USBD_GetString((uint8_t *)USBD_PRODUCT_STRING_FS, USBD_StrDesc, length);
    } else {
        USBD_GetString((uint8_t *)USBD_PRODUCT_STRING_FS, USBD_StrDesc, length);
    }
    return USBD_StrDesc;
}

// Return the manufacturer string descriptor
// speed : Current device speed
// length : Pointer to data length variable
// returns : Pointer to descriptor buffer
uint8_t * USBD_FS_ManufacturerStrDescriptor(USBD_SpeedTypeDef speed, uint16_t *length)
{
    UNUSED(speed);
    USBD_GetString((uint8_t *)USBD_MANUFACTURER_STRING, USBD_StrDesc, length);
    return USBD_StrDesc;
}

// Return the serial number string descriptor
// speed : Current device speed
// length : Pointer to data length variable
// returns : Pointer to descriptor buffer
uint8_t * USBD_FS_SerialStrDescriptor(USBD_SpeedTypeDef speed, uint16_t *length)
{
    UNUSED(speed);
    // Use limited-length prefix so that device names look good on MacOS
#ifndef FRIENDLY_SERIAL_NUMBER
    *length = USB_SIZ_STRING_SERIAL;
    // Update the serial number string descriptor with the data from the unique ID
    Get_SerialNum();
    return (uint8_t *) USBD_StringSerial;
#else
    char strprefix[5] = {0};
    memset(USBD_StrDesc, 0, sizeof(USBD_StrDesc));
    for (int i=0; i<sizeof(strprefix)-1; i++) {
        uint8_t ch = USBD_PRODUCT_STRING_FS[i];
        if (ch == '\0') {
            break;
        }
        strprefix[i] = toupper(ch);
    }
    USBD_GetString((uint8_t *)strprefix, USBD_StrDesc, length);
    return USBD_StrDesc;
#endif
}

// Return the configuration string descriptor
// speed : Current device speed
// length : Pointer to data length variable
// returns : Pointer to descriptor buffer
uint8_t * USBD_FS_ConfigStrDescriptor(USBD_SpeedTypeDef speed, uint16_t *length)
{
    if (speed == USBD_SPEED_HIGH) {
        USBD_GetString((uint8_t *)USBD_CONFIGURATION_STRING_FS, USBD_StrDesc, length);
    } else {
        USBD_GetString((uint8_t *)USBD_CONFIGURATION_STRING_FS, USBD_StrDesc, length);
    }
    return USBD_StrDesc;
}

// Return the interface string descriptor
// speed : Current device speed
// length : Pointer to data length variable
// returns : Pointer to descriptor buffer
uint8_t * USBD_FS_InterfaceStrDescriptor(USBD_SpeedTypeDef speed, uint16_t *length)
{
    if(speed == 0) {
        USBD_GetString((uint8_t *)USBD_INTERFACE_STRING_FS, USBD_StrDesc, length);
    } else {
        USBD_GetString((uint8_t *)USBD_INTERFACE_STRING_FS, USBD_StrDesc, length);
    }
    return USBD_StrDesc;
}

#if (USBD_LPM_ENABLED == 1)
// Return the BOS descriptor
// speed : Current device speed
// length : Pointer to data length variable
// returns : Pointer to descriptor buffer
uint8_t * USBD_FS_USR_BOSDescriptor(USBD_SpeedTypeDef speed, uint16_t *length)
{
    UNUSED(speed);
    *length = sizeof(USBD_FS_BOSDesc);
    return (uint8_t*)USBD_FS_BOSDesc;
}
#endif

// Create the serial number string descriptor
#ifndef FRIENDLY_SERIAL_NUMBER
static void Get_SerialNum(void)
{
    uint32_t deviceserial0;
    uint32_t deviceserial1;
    uint32_t deviceserial2;

    deviceserial0 = *(uint32_t *) DEVICE_ID1;
    deviceserial1 = *(uint32_t *) DEVICE_ID2;
    deviceserial2 = *(uint32_t *) DEVICE_ID3;

    deviceserial0 += deviceserial2;

    if (deviceserial0 != 0) {
        IntToUnicode(deviceserial0, &USBD_StringSerial[2], 8);
        IntToUnicode(deviceserial1, &USBD_StringSerial[18], 4);
    }

}
#endif

// Convert Hex 32Bits value into char
// value: value to convert
// pbuf: pointer to the buffer
// len: buffer length
#ifndef FRIENDLY_SERIAL_NUMBER
static void IntToUnicode(uint32_t value, uint8_t * pbuf, uint8_t len)
{
    uint8_t idx = 0;

    for (idx = 0; idx < len; idx++) {
        if (((value >> 28)) < 0xA) {
            pbuf[2 * idx] = (value >> 28) + '0';
        } else {
            pbuf[2 * idx] = (value >> 28) + 'A' - 10;
        }

        value = value << 4;

        pbuf[2 * idx + 1] = 0;
    }
}
#endif
