/**
  ******************************************************************************
  * @file    usbd_ioreq.c
  * @author  MCD Application Team
  * @brief   This file provides the IO requests APIs for control endpoints.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2015 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include "usbd_ioreq.h"

/** @addtogroup STM32_USB_DEVICE_LIBRARY
  * @{
  */


/** @defgroup USBD_IOREQ
  * @brief control I/O requests module
  * @{
  */

/** @defgroup USBD_IOREQ_Private_TypesDefinitions
  * @{
  */
/**
  * @}
  */


/** @defgroup USBD_IOREQ_Private_Defines
  * @{
  */

/**
  * @}
  */


/** @defgroup USBD_IOREQ_Private_Macros
  * @{
  */
/**
  * @}
  */


/** @defgroup USBD_IOREQ_Private_Variables
  * @{
  */

/* One aligned EP0 packet per USB controller (device IDs 0 and 1). */
#if defined(__ICCARM__)
#pragma data_alignment=4
#endif
__ALIGN_BEGIN static uint8_t ctl_dma_buffer[2U][USB_MAX_EP0_SIZE] __ALIGN_END;

/**
  * @}
  */


/** @defgroup USBD_IOREQ_Private_FunctionPrototypes
  * @{
  */
/**
  * @}
  */


/** @defgroup USBD_IOREQ_Private_Functions
  * @{
  */

/**
  * @brief  USBD_CtlStageData
  *         Stage one unaligned packet, or snapshot a short status response.
  * @param  pdev: device instance
  * @param  pbuf: pointer to data buffer
  * @param  len: remaining length, updated to the staged packet length
  * @retval transmit buffer, or NULL for invalid non-empty transfers
  */
static uint8_t *USBD_CtlStageData(USBD_HandleTypeDef *pdev, uint8_t *pbuf, uint32_t *len)
{
  uint32_t idx;
  uint8_t *staging;

  if ((*len != 0U) && (pbuf == NULL))
  {
    return NULL;
  }

  if ((*len == 0U) || ((*len > 4U) && (((uint32_t)pbuf & 0x03U) == 0U)))
  {
    return pbuf;
  }

  if ((pdev->id >= 2U) ||
      (pdev->ep_in[0].maxpacket == 0U) || (pdev->ep_in[0].maxpacket > USB_MAX_EP0_SIZE))
  {
    return NULL;
  }

  staging = ctl_dma_buffer[pdev->id];
  *len = MIN(*len, (uint32_t)pdev->ep_in[0].maxpacket);

  for (idx = 0U; idx < *len; idx++)
  {
    staging[idx] = pbuf[idx];
  }

  return staging;
}

/**
  * @brief  USBD_CtlSendData
  *         send data on the ctl pipe
  * @param  pdev: device instance
  * @param  buff: pointer to data buffer
  * @param  len: length of data to be sent
  * @retval status
  */
USBD_StatusTypeDef USBD_CtlSendData(USBD_HandleTypeDef *pdev,
                                    uint8_t *pbuf, uint32_t len)
{
  uint32_t packet_length = len;
  uint8_t *aligned_buffer = USBD_CtlStageData(pdev, pbuf, &packet_length);

  if ((len != 0U) && (aligned_buffer == NULL))
  {
    return USBD_FAIL;
  }

  /* Set EP0 State */
  pdev->ep0_state = USBD_EP0_DATA_IN;
  pdev->ep_in[0].total_length = len;
  pdev->ep_in[0].pbuffer = pbuf;

#ifdef USBD_AVOID_PACKET_SPLIT_MPS
  pdev->ep_in[0].rem_length = 0U;
#else
  pdev->ep_in[0].rem_length = len;
#endif /* USBD_AVOID_PACKET_SPLIT_MPS */

  /* Start the transfer */
  return USBD_LL_Transmit(pdev, 0x00U, aligned_buffer, packet_length);
}

/**
  * @brief  USBD_CtlContinueSendData
  *         continue sending data on the ctl pipe
  * @param  pdev: device instance
  * @param  buff: pointer to data buffer
  * @param  len: length of data to be sent
  * @retval status
  */
USBD_StatusTypeDef USBD_CtlContinueSendData(USBD_HandleTypeDef *pdev,
                                            uint8_t *pbuf, uint32_t len)
{
  uint8_t *aligned_buffer = USBD_CtlStageData(pdev, pbuf, &len);

  if ((len != 0U) && (aligned_buffer == NULL))
  {
    return USBD_FAIL;
  }

  /* Start the next transfer */
  return USBD_LL_Transmit(pdev, 0x00U, aligned_buffer, len);
}

/**
  * @brief  USBD_CtlPrepareRx
  *         receive data on the ctl pipe
  * @param  pdev: device instance
  * @param  buff: pointer to data buffer
  * @param  len: length of data to be received
  * @retval status
  */
USBD_StatusTypeDef USBD_CtlPrepareRx(USBD_HandleTypeDef *pdev,
                                     uint8_t *pbuf, uint32_t len)
{
  /* Set EP0 State */
  pdev->ep0_state = USBD_EP0_DATA_OUT;
  pdev->ep_out[0].total_length = len;
  pdev->ep_out[0].pbuffer = pbuf;

#ifdef USBD_AVOID_PACKET_SPLIT_MPS
  pdev->ep_out[0].rem_length = 0U;
#else
  pdev->ep_out[0].rem_length = len;
#endif /* USBD_AVOID_PACKET_SPLIT_MPS */

  /* Start the transfer */
  (void)USBD_LL_PrepareReceive(pdev, 0U, pbuf, len);

  return USBD_OK;
}

/**
  * @brief  USBD_CtlContinueRx
  *         continue receive data on the ctl pipe
  * @param  pdev: device instance
  * @param  buff: pointer to data buffer
  * @param  len: length of data to be received
  * @retval status
  */
USBD_StatusTypeDef USBD_CtlContinueRx(USBD_HandleTypeDef *pdev,
                                      uint8_t *pbuf, uint32_t len)
{
  (void)USBD_LL_PrepareReceive(pdev, 0U, pbuf, len);

  return USBD_OK;
}

/**
  * @brief  USBD_CtlSendStatus
  *         send zero lzngth packet on the ctl pipe
  * @param  pdev: device instance
  * @retval status
  */
USBD_StatusTypeDef USBD_CtlSendStatus(USBD_HandleTypeDef *pdev)
{
  /* Set EP0 State */
  pdev->ep0_state = USBD_EP0_STATUS_IN;

  /* Start the transfer */
  (void)USBD_LL_Transmit(pdev, 0x00U, NULL, 0U);

  return USBD_OK;
}

/**
  * @brief  USBD_CtlReceiveStatus
  *         receive zero lzngth packet on the ctl pipe
  * @param  pdev: device instance
  * @retval status
  */
USBD_StatusTypeDef USBD_CtlReceiveStatus(USBD_HandleTypeDef *pdev)
{
  /* Set EP0 State */
  pdev->ep0_state = USBD_EP0_STATUS_OUT;

  /* Start the transfer */
  (void)USBD_LL_PrepareReceive(pdev, 0U, NULL, 0U);

  return USBD_OK;
}

/**
  * @brief  USBD_GetRxCount
  *         returns the received data length
  * @param  pdev: device instance
  * @param  ep_addr: endpoint address
  * @retval Rx Data blength
  */
uint32_t USBD_GetRxCount(USBD_HandleTypeDef *pdev, uint8_t ep_addr)
{
  return USBD_LL_GetRxDataSize(pdev, ep_addr);
}

/**
  * @}
  */


/**
  * @}
  */


/**
  * @}
  */
