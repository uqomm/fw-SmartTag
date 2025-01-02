#ifndef _UWB_H
#define _UWB_H

#include "deca_types.h"
#include "main.h"

#define DWT_SUCCESS                 	(0)
#define DWT_ERROR                   	(-1)

#define DW3000_IRQ_EXTI_IRQn           EXTI3_IRQn

#define DW3000_CS_PIN           GPIO_PIN_4
#define DW3000_RST_PIN          GPIO_PIN_0

#define DW3000_CS1_PIN           GPIO_PIN_12
#define DW3000_RST1_PIN          GPIO_PIN_8

#define DW3000_WUP_PIN          GPIO_PIN_4
#define DW3000_IRQ_PIN          GPIO_PIN_3

#define writetospi                  	writetospi_serial
#define readfromspi                		readfromspi_serial

#define port_GetEXT_IRQStatus()         EXTI_GetITEnStatus(DW3000_IRQ_EXTI_IRQn)
#define port_DisableEXT_IRQ()           NVIC_DisableIRQ(DW3000_IRQ_EXTI_IRQn)
#define port_EnableEXT_IRQ()            NVIC_EnableIRQ(DW3000_IRQ_EXTI_IRQn)
#define port_CheckEXT_IRQ()             HAL_GPIO_ReadPin(GPIOB, DW3000_IRQ_PIN)


#define port_SPIx_set_chip_select()     HAL_GPIO_WritePin(GPIOA, DW3000_CS_PIN, GPIO_PIN_RESET)
#define port_SPIx_clear_chip_select()	HAL_GPIO_WritePin(GPIOA, DW3000_CS_PIN, GPIO_PIN_SET)

#define port_SPI2_set_chip_select()     HAL_GPIO_WritePin(GPIOB, DW3000_CS1_PIN, GPIO_PIN_RESET)
#define port_SPI2_clear_chip_select()	HAL_GPIO_WritePin(GPIOB, DW3000_CS1_PIN, GPIO_PIN_SET)

void Sleep(uint32_t time_ms);

void reset_DWIC(void);

void wakeup_device_with_io(void);

void port_set_dw_ic_spi_slowrate(void);

void port_set_dw_ic_spi_fastrate(void);

int writetospi_serial( uint16_t headerLength,
			   	    const uint8_t *headerBuffer,
					uint32_t bodylength,
					const uint8_t *bodyBuffer
				  );
int readfromspi_serial( uint16_t	headerLength,
			    	 const uint8_t *headerBuffer,
					 uint32_t readlength,
					 uint8_t *readBuffer );
                     
ITStatus EXTI_GetITEnStatus(uint32_t EXTI_Line);
                   
void process_deca_irq(void);
                     
extern __IO unsigned long time32_incr;
                     
#endif
