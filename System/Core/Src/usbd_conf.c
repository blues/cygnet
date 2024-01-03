// Copyright 2023 Blues Inc.  All rights reserved.
// Use of this source code is governed by licenses granted by the
// copyright holder including that found in the LICENSE file.

#include "stm32l4xx.h"
#include "stm32l4xx_hal.h"
#include "usbd_def.h"
#include "usbd_core.h"
#include "usbd_cdc.h"

PCD_HandleTypeDef hpcd_USB_FS;
void Error_Handler(void);

extern USBD_StatusTypeDef USBD_LL_BatteryCharging(USBD_HandleTypeDef *pdev);

// Forwards
static void SystemClockConfig_Resume(void);
extern void SystemClock_Config(void);

// MSP Init
void HAL_PCD_MspInit(PCD_HandleTypeDef* pcdHandle)
{
    RCC_PeriphCLKInitTypeDef PeriphClkInit = {0};
    if (pcdHandle->Instance==USB) {

        // Initializes the peripherals clock
        PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_USB;
        PeriphClkInit.UsbClockSelection = RCC_USBCLKSOURCE_MSI;
        if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK) {
            Error_Handler();
        }

        // Peripheral clock enable
        __HAL_RCC_USB_CLK_ENABLE();

        // Peripheral interrupt init
        HAL_NVIC_SetPriority(USB_IRQn, 5, 0);
        HAL_NVIC_EnableIRQ(USB_IRQn);

    }
}

// MSP DeInit
void HAL_PCD_MspDeInit(PCD_HandleTypeDef* pcdHandle)
{
    if (pcdHandle->Instance==USB) {

        // Peripheral clock disable
        __HAL_RCC_USB_CLK_DISABLE();

        // Peripheral interrupt Deinit
        HAL_NVIC_DisableIRQ(USB_IRQn);

    }
}

// Setup stage callback
#if (USE_HAL_PCD_REGISTER_CALLBACKS == 1U)
static void PCD_SetupStageCallback(PCD_HandleTypeDef *hpcd)
#else
void HAL_PCD_SetupStageCallback(PCD_HandleTypeDef *hpcd)
#endif
{
    USBD_LL_SetupStage((USBD_HandleTypeDef*)hpcd->pData, (uint8_t *)hpcd->Setup);
}

// Data Out stage callback.
#if (USE_HAL_PCD_REGISTER_CALLBACKS == 1U)
static void PCD_DataOutStageCallback(PCD_HandleTypeDef *hpcd, uint8_t epnum)
#else
void HAL_PCD_DataOutStageCallback(PCD_HandleTypeDef *hpcd, uint8_t epnum)
#endif
{
    USBD_LL_DataOutStage((USBD_HandleTypeDef*)hpcd->pData, epnum, hpcd->OUT_ep[epnum].xfer_buff);
}

// Data In stage callback.
#if (USE_HAL_PCD_REGISTER_CALLBACKS == 1U)
static void PCD_DataInStageCallback(PCD_HandleTypeDef *hpcd, uint8_t epnum)
#else
void HAL_PCD_DataInStageCallback(PCD_HandleTypeDef *hpcd, uint8_t epnum)
#endif
{
    USBD_LL_DataInStage((USBD_HandleTypeDef*)hpcd->pData, epnum, hpcd->IN_ep[epnum].xfer_buff);
}

// SOF callback.
#if (USE_HAL_PCD_REGISTER_CALLBACKS == 1U)
static void PCD_SOFCallback(PCD_HandleTypeDef *hpcd)
#else
void HAL_PCD_SOFCallback(PCD_HandleTypeDef *hpcd)
#endif
{
    USBD_LL_SOF((USBD_HandleTypeDef*)hpcd->pData);
}

// Reset callback.
#if (USE_HAL_PCD_REGISTER_CALLBACKS == 1U)
static void PCD_ResetCallback(PCD_HandleTypeDef *hpcd)
#else
void HAL_PCD_ResetCallback(PCD_HandleTypeDef *hpcd)
#endif
{
    USBD_SpeedTypeDef speed = USBD_SPEED_FULL;

    if ( hpcd->Init.speed != PCD_SPEED_FULL) {
        Error_Handler();
    }

    // Set Speed
    USBD_LL_SetSpeed((USBD_HandleTypeDef*)hpcd->pData, speed);

    // Reset Device
    USBD_LL_Reset((USBD_HandleTypeDef*)hpcd->pData);

}

// Suspend callback
// When Low power mode is enabled the debug cannot be used (IAR, Keil doesn't support it)
#if (USE_HAL_PCD_REGISTER_CALLBACKS == 1U)
static void PCD_SuspendCallback(PCD_HandleTypeDef *hpcd)
#else
void HAL_PCD_SuspendCallback(PCD_HandleTypeDef *hpcd)
#endif
{
    // Inform USB library that core enters in suspend Mode.
    USBD_LL_Suspend((USBD_HandleTypeDef*)hpcd->pData);
    // Enter in STOP mode
    if (hpcd->Init.low_power_enable) {
        // Set SLEEPDEEP bit and SleepOnExit of Cortex System Control Register
        SCB->SCR |= (uint32_t)((uint32_t)(SCB_SCR_SLEEPDEEP_Msk | SCB_SCR_SLEEPONEXIT_Msk));
    }
}

// Resume callback
// When Low power mode is enabled the debug cannot be used (IAR, Keil doesn't support it)
#if (USE_HAL_PCD_REGISTER_CALLBACKS == 1U)
static void PCD_ResumeCallback(PCD_HandleTypeDef *hpcd)
#else
void HAL_PCD_ResumeCallback(PCD_HandleTypeDef *hpcd)
#endif
{
    if (hpcd->Init.low_power_enable) {
        // Reset SLEEPDEEP bit of Cortex System Control Register
        SCB->SCR &= (uint32_t)~((uint32_t)(SCB_SCR_SLEEPDEEP_Msk | SCB_SCR_SLEEPONEXIT_Msk));
        SystemClockConfig_Resume();
    }
    USBD_LL_Resume((USBD_HandleTypeDef*)hpcd->pData);
}

// ISOOUTIncomplete callback
#if (USE_HAL_PCD_REGISTER_CALLBACKS == 1U)
static void PCD_ISOOUTIncompleteCallback(PCD_HandleTypeDef *hpcd, uint8_t epnum)
#else
void HAL_PCD_ISOOUTIncompleteCallback(PCD_HandleTypeDef *hpcd, uint8_t epnum)
#endif // USE_HAL_PCD_REGISTER_CALLBACKS
{
    USBD_LL_IsoOUTIncomplete((USBD_HandleTypeDef*)hpcd->pData, epnum);
}

// ISOINIncomplete callback.
#if (USE_HAL_PCD_REGISTER_CALLBACKS == 1U)
static void PCD_ISOINIncompleteCallback(PCD_HandleTypeDef *hpcd, uint8_t epnum)
#else
void HAL_PCD_ISOINIncompleteCallback(PCD_HandleTypeDef *hpcd, uint8_t epnum)
#endif
{
    USBD_LL_IsoINIncomplete((USBD_HandleTypeDef*)hpcd->pData, epnum);
}

// Connect callback.
#if (USE_HAL_PCD_REGISTER_CALLBACKS == 1U)
static void PCD_ConnectCallback(PCD_HandleTypeDef *hpcd)
#else
void HAL_PCD_ConnectCallback(PCD_HandleTypeDef *hpcd)
#endif
{
    USBD_LL_DevConnected((USBD_HandleTypeDef*)hpcd->pData);
}

// Disconnect callback.
#if (USE_HAL_PCD_REGISTER_CALLBACKS == 1U)
static void PCD_DisconnectCallback(PCD_HandleTypeDef *hpcd)
#else
void HAL_PCD_DisconnectCallback(PCD_HandleTypeDef *hpcd)
#endif
{
    USBD_LL_DevDisconnected((USBD_HandleTypeDef*)hpcd->pData);
}

//*******************************************************************************
//                LL Driver Interface (USB Device Library --> PCD)
//*******************************************************************************

// Initializes the low level portion of the device driver.
USBD_StatusTypeDef USBD_LL_Init(USBD_HandleTypeDef *pdev)
{
    // Init USB Ip.
    // Enable USB power on Pwrctrl CR2 register.
    HAL_PWREx_EnableVddUSB();
    // Link the driver to the stack.
    hpcd_USB_FS.pData = pdev;
    pdev->pData = &hpcd_USB_FS;

    hpcd_USB_FS.Instance = USB;
    hpcd_USB_FS.Init.dev_endpoints = 8;
    hpcd_USB_FS.Init.speed = PCD_SPEED_FULL;
    hpcd_USB_FS.Init.phy_itface = PCD_PHY_EMBEDDED;
    hpcd_USB_FS.Init.Sof_enable = DISABLE;
    hpcd_USB_FS.Init.low_power_enable = DISABLE;
    hpcd_USB_FS.Init.lpm_enable = DISABLE;
    hpcd_USB_FS.Init.battery_charging_enable = DISABLE;
    if (HAL_PCD_Init(&hpcd_USB_FS) != HAL_OK) {
        Error_Handler( );
    }

#if (USE_HAL_PCD_REGISTER_CALLBACKS == 1U)
    HAL_PCD_RegisterCallback(&hpcd_USB_FS, HAL_PCD_SOF_CB_ID, PCD_SOFCallback);
    HAL_PCD_RegisterCallback(&hpcd_USB_FS, HAL_PCD_SETUPSTAGE_CB_ID, PCD_SetupStageCallback);
    HAL_PCD_RegisterCallback(&hpcd_USB_FS, HAL_PCD_RESET_CB_ID, PCD_ResetCallback);
    HAL_PCD_RegisterCallback(&hpcd_USB_FS, HAL_PCD_SUSPEND_CB_ID, PCD_SuspendCallback);
    HAL_PCD_RegisterCallback(&hpcd_USB_FS, HAL_PCD_RESUME_CB_ID, PCD_ResumeCallback);
    HAL_PCD_RegisterCallback(&hpcd_USB_FS, HAL_PCD_CONNECT_CB_ID, PCD_ConnectCallback);
    HAL_PCD_RegisterCallback(&hpcd_USB_FS, HAL_PCD_DISCONNECT_CB_ID, PCD_DisconnectCallback);

    HAL_PCD_RegisterDataOutStageCallback(&hpcd_USB_FS, PCD_DataOutStageCallback);
    HAL_PCD_RegisterDataInStageCallback(&hpcd_USB_FS, PCD_DataInStageCallback);
    HAL_PCD_RegisterIsoOutIncpltCallback(&hpcd_USB_FS, PCD_ISOOUTIncompleteCallback);
    HAL_PCD_RegisterIsoInIncpltCallback(&hpcd_USB_FS, PCD_ISOINIncompleteCallback);
#endif
    HAL_PCDEx_PMAConfig((PCD_HandleTypeDef*)pdev->pData, 0x00, PCD_SNG_BUF, 0x18);
    HAL_PCDEx_PMAConfig((PCD_HandleTypeDef*)pdev->pData, 0x80, PCD_SNG_BUF, 0x58);
    HAL_PCDEx_PMAConfig((PCD_HandleTypeDef*)pdev->pData, 0x81, PCD_SNG_BUF, 0xC0);
    HAL_PCDEx_PMAConfig((PCD_HandleTypeDef*)pdev->pData, 0x01, PCD_SNG_BUF, 0x110);
    HAL_PCDEx_PMAConfig((PCD_HandleTypeDef*)pdev->pData, 0x82, PCD_SNG_BUF, 0x100);
    return USBD_OK;
}

//  De-Initializes the low level portion of the device driver.
USBD_StatusTypeDef USBD_LL_DeInit(USBD_HandleTypeDef *pdev)
{
    HAL_StatusTypeDef hal_status = HAL_OK;
    USBD_StatusTypeDef usb_status = USBD_OK;

    hal_status = HAL_PCD_DeInit(pdev->pData);

    switch (hal_status) {
    case HAL_OK :
        usb_status = USBD_OK;
        break;
    case HAL_ERROR :
        usb_status = USBD_FAIL;
        break;
    case HAL_BUSY :
        usb_status = USBD_BUSY;
        break;
    case HAL_TIMEOUT :
        usb_status = USBD_FAIL;
        break;
    default :
        usb_status = USBD_FAIL;
        break;
    }
    return usb_status;
}

// Starts the low level portion of the device driver.
USBD_StatusTypeDef USBD_LL_Start(USBD_HandleTypeDef *pdev)
{
    HAL_StatusTypeDef hal_status = HAL_OK;
    USBD_StatusTypeDef usb_status = USBD_OK;

    hal_status = HAL_PCD_Start(pdev->pData);

    switch (hal_status) {
    case HAL_OK :
        usb_status = USBD_OK;
        break;
    case HAL_ERROR :
        usb_status = USBD_FAIL;
        break;
    case HAL_BUSY :
        usb_status = USBD_BUSY;
        break;
    case HAL_TIMEOUT :
        usb_status = USBD_FAIL;
        break;
    default :
        usb_status = USBD_FAIL;
        break;
    }
    return usb_status;
}

// Stops the low level portion of the device driver.
USBD_StatusTypeDef USBD_LL_Stop(USBD_HandleTypeDef *pdev)
{
    HAL_StatusTypeDef hal_status = HAL_OK;
    USBD_StatusTypeDef usb_status = USBD_OK;

    hal_status = HAL_PCD_Stop(pdev->pData);

    switch (hal_status) {
    case HAL_OK :
        usb_status = USBD_OK;
        break;
    case HAL_ERROR :
        usb_status = USBD_FAIL;
        break;
    case HAL_BUSY :
        usb_status = USBD_BUSY;
        break;
    case HAL_TIMEOUT :
        usb_status = USBD_FAIL;
        break;
    default :
        usb_status = USBD_FAIL;
        break;
    }
    return usb_status;
}

// Opens an endpoint of the low level driver.
// pdev: Device handle
// ep_addr: Endpoint number
// ep_type: Endpoint type
// ep_mps: Endpoint max packet size
USBD_StatusTypeDef USBD_LL_OpenEP(USBD_HandleTypeDef *pdev, uint8_t ep_addr, uint8_t ep_type, uint16_t ep_mps)
{
    HAL_StatusTypeDef hal_status = HAL_OK;
    USBD_StatusTypeDef usb_status = USBD_OK;

    hal_status = HAL_PCD_EP_Open(pdev->pData, ep_addr, ep_mps, ep_type);

    switch (hal_status) {
    case HAL_OK :
        usb_status = USBD_OK;
        break;
    case HAL_ERROR :
        usb_status = USBD_FAIL;
        break;
    case HAL_BUSY :
        usb_status = USBD_BUSY;
        break;
    case HAL_TIMEOUT :
        usb_status = USBD_FAIL;
        break;
    default :
        usb_status = USBD_FAIL;
        break;
    }
    return usb_status;
}

// Closes an endpoint of the low level driver.
// pdev: Device handle
// ep_addr: Endpoint number
// USBD status
USBD_StatusTypeDef USBD_LL_CloseEP(USBD_HandleTypeDef *pdev, uint8_t ep_addr)
{
    HAL_StatusTypeDef hal_status = HAL_OK;
    USBD_StatusTypeDef usb_status = USBD_OK;

    hal_status = HAL_PCD_EP_Close(pdev->pData, ep_addr);

    switch (hal_status) {
    case HAL_OK :
        usb_status = USBD_OK;
        break;
    case HAL_ERROR :
        usb_status = USBD_FAIL;
        break;
    case HAL_BUSY :
        usb_status = USBD_BUSY;
        break;
    case HAL_TIMEOUT :
        usb_status = USBD_FAIL;
        break;
    default :
        usb_status = USBD_FAIL;
        break;
    }
    return usb_status;
}

// Flushes an endpoint of the Low Level Driver.
// pdev: Device handle
// ep_addr: Endpoint number
USBD_StatusTypeDef USBD_LL_FlushEP(USBD_HandleTypeDef *pdev, uint8_t ep_addr)
{
    HAL_StatusTypeDef hal_status = HAL_OK;
    USBD_StatusTypeDef usb_status = USBD_OK;

    hal_status = HAL_PCD_EP_Flush(pdev->pData, ep_addr);

    switch (hal_status) {
    case HAL_OK :
        usb_status = USBD_OK;
        break;
    case HAL_ERROR :
        usb_status = USBD_FAIL;
        break;
    case HAL_BUSY :
        usb_status = USBD_BUSY;
        break;
    case HAL_TIMEOUT :
        usb_status = USBD_FAIL;
        break;
    default :
        usb_status = USBD_FAIL;
        break;
    }
    return usb_status;
}

// Sets a Stall condition on an endpoint of the Low Level Driver.
// pdev: Device handle
// ep_addr: Endpoint number
USBD_StatusTypeDef USBD_LL_StallEP(USBD_HandleTypeDef *pdev, uint8_t ep_addr)
{
    HAL_StatusTypeDef hal_status = HAL_OK;
    USBD_StatusTypeDef usb_status = USBD_OK;

    hal_status = HAL_PCD_EP_SetStall(pdev->pData, ep_addr);

    switch (hal_status) {
    case HAL_OK :
        usb_status = USBD_OK;
        break;
    case HAL_ERROR :
        usb_status = USBD_FAIL;
        break;
    case HAL_BUSY :
        usb_status = USBD_BUSY;
        break;
    case HAL_TIMEOUT :
        usb_status = USBD_FAIL;
        break;
    default :
        usb_status = USBD_FAIL;
        break;
    }
    return usb_status;
}

// Clears a Stall condition on an endpoint of the Low Level Driver.
// pdev: Device handle
// ep_addr: Endpoint number
USBD_StatusTypeDef USBD_LL_ClearStallEP(USBD_HandleTypeDef *pdev, uint8_t ep_addr)
{
    HAL_StatusTypeDef hal_status = HAL_OK;
    USBD_StatusTypeDef usb_status = USBD_OK;

    hal_status = HAL_PCD_EP_ClrStall(pdev->pData, ep_addr);

    switch (hal_status) {
    case HAL_OK :
        usb_status = USBD_OK;
        break;
    case HAL_ERROR :
        usb_status = USBD_FAIL;
        break;
    case HAL_BUSY :
        usb_status = USBD_BUSY;
        break;
    case HAL_TIMEOUT :
        usb_status = USBD_FAIL;
        break;
    default :
        usb_status = USBD_FAIL;
        break;
    }
    return usb_status;
}

// Returns Stall condition.
// pdev: Device handle
// ep_addr: Endpoint number
// Stall (1: Yes, 0: No)
uint8_t USBD_LL_IsStallEP(USBD_HandleTypeDef *pdev, uint8_t ep_addr)
{
    PCD_HandleTypeDef *hpcd = (PCD_HandleTypeDef*) pdev->pData;

    if((ep_addr & 0x80) == 0x80) {
        return hpcd->IN_ep[ep_addr & 0x7F].is_stall;
    } else {
        return hpcd->OUT_ep[ep_addr & 0x7F].is_stall;
    }
}

// Assigns a USB address to the device.
// pdev: Device handle
// dev_addr: Device address
USBD_StatusTypeDef USBD_LL_SetUSBAddress(USBD_HandleTypeDef *pdev, uint8_t dev_addr)
{
    HAL_StatusTypeDef hal_status = HAL_OK;
    USBD_StatusTypeDef usb_status = USBD_OK;

    hal_status = HAL_PCD_SetAddress(pdev->pData, dev_addr);

    switch (hal_status) {
    case HAL_OK :
        usb_status = USBD_OK;
        break;
    case HAL_ERROR :
        usb_status = USBD_FAIL;
        break;
    case HAL_BUSY :
        usb_status = USBD_BUSY;
        break;
    case HAL_TIMEOUT :
        usb_status = USBD_FAIL;
        break;
    default :
        usb_status = USBD_FAIL;
        break;
    }
    return usb_status;
}

// Transmits data over an endpoint.
// pdev: Device handle
// ep_addr: Endpoint number
// pbuf: Pointer to data to be sent
// size: Data size
USBD_StatusTypeDef USBD_LL_Transmit(USBD_HandleTypeDef *pdev, uint8_t ep_addr, uint8_t *pbuf, uint32_t size)
{
    HAL_StatusTypeDef hal_status = HAL_OK;
    USBD_StatusTypeDef usb_status = USBD_OK;

    hal_status = HAL_PCD_EP_Transmit(pdev->pData, ep_addr, pbuf, size);

    switch (hal_status) {
    case HAL_OK :
        usb_status = USBD_OK;
        break;
    case HAL_ERROR :
        usb_status = USBD_FAIL;
        break;
    case HAL_BUSY :
        usb_status = USBD_BUSY;
        break;
    case HAL_TIMEOUT :
        usb_status = USBD_FAIL;
        break;
    default :
        usb_status = USBD_FAIL;
        break;
    }
    return usb_status;
}

// Prepares an endpoint for reception.
// pdev: Device handle
// ep_addr: Endpoint number
// pbuf: Pointer to data to be received
// size: Data size
USBD_StatusTypeDef USBD_LL_PrepareReceive(USBD_HandleTypeDef *pdev, uint8_t ep_addr, uint8_t *pbuf, uint32_t size)
{
    HAL_StatusTypeDef hal_status = HAL_OK;
    USBD_StatusTypeDef usb_status = USBD_OK;

    hal_status = HAL_PCD_EP_Receive(pdev->pData, ep_addr, pbuf, size);

    switch (hal_status) {
    case HAL_OK :
        usb_status = USBD_OK;
        break;
    case HAL_ERROR :
        usb_status = USBD_FAIL;
        break;
    case HAL_BUSY :
        usb_status = USBD_BUSY;
        break;
    case HAL_TIMEOUT :
        usb_status = USBD_FAIL;
        break;
    default :
        usb_status = USBD_FAIL;
        break;
    }
    return usb_status;
}

// Returns the last transferred packet size.
// pdev: Device handle
// ep_addr: Endpoint number
// Received Data Size
uint32_t USBD_LL_GetRxDataSize(USBD_HandleTypeDef *pdev, uint8_t ep_addr)
{
    return HAL_PCD_EP_GetRxCount((PCD_HandleTypeDef*) pdev->pData, ep_addr);
}

// Send LPM message to user layer
// hpcd: PCD handle
// msg: LPM message
void HAL_PCDEx_LPM_Callback(PCD_HandleTypeDef *hpcd, PCD_LPM_MsgTypeDef msg)
{
    switch (msg) {
    case PCD_LPM_L0_ACTIVE:
        if (hpcd->Init.low_power_enable) {
            SystemClockConfig_Resume();

            // Reset SLEEPDEEP bit of Cortex System Control Register.
            SCB->SCR &= (uint32_t)~((uint32_t)(SCB_SCR_SLEEPDEEP_Msk | SCB_SCR_SLEEPONEXIT_Msk));
        }
        USBD_LL_Resume(hpcd->pData);
        break;

    case PCD_LPM_L1_ACTIVE:
        USBD_LL_Suspend(hpcd->pData);

        // Enter in STOP mode.
        if (hpcd->Init.low_power_enable) {
            // Set SLEEPDEEP bit and SleepOnExit of Cortex System Control Register.
            SCB->SCR |= (uint32_t)((uint32_t)(SCB_SCR_SLEEPDEEP_Msk | SCB_SCR_SLEEPONEXIT_Msk));
        }
        break;
    }
}

// Delays routine for the USB Device Library.
// Delay: Delay in ms
void USBD_LL_Delay(uint32_t Delay)
{
    HAL_Delay(Delay);
}

// Static single allocation.
// size: Size of allocated memory
void *USBD_static_malloc(uint32_t size)
{
    static uint32_t mem[(sizeof(USBD_CDC_HandleTypeDef)/4)+1];/* On 32-bit boundary */
    return mem;
}

// Dummy memory free
// p: Pointer to allocated  memory address
void USBD_static_free(void *p)
{
}

// Configures system clock after wake-up from USB resume callBack:
// enable HSI, PLL and select PLL as system clock source.
static void SystemClockConfig_Resume(void)
{
    SystemClock_Config();
}

