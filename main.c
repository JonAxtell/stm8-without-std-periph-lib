/*
 * main.c
 *
 * Simple blinking LED program for the STM8S105K4 development board from Hobbytronics.
 * Well a little bit more involved that a simple blinky as it includes proper timing.
 * It's simple in that it doesn't have any reliance on the Standard Peripheral Library
 * or anything complicated and buggy like that.
 *
 * Copyright 2018 Jon Axtell GPL
 */

// Some basic macros to make life easy
#ifdef __gcc__
#define __PACKED    __attribute((packed))
#define __IO        volatile
#else
#define __PACKED
#define __IO        volatile
#endif

// Some basic typedefs
typedef int bool;
#define false   0
#define true    (!false)
typedef signed char int8_t;
typedef signed short int16_t;
typedef signed long int32_t;
typedef unsigned char uint8_t;
typedef unsigned short uint16_t;
typedef unsigned long uint32_t;
typedef unsigned int uint_t;

// Some basic in-line assembly
#define disableInterrupts       __asm sim __endasm
#define enableInterrupts        __asm rim __endasm

//#############################################################################
// The peripherals in the STM8
//

//=============================================================================
// System clock
typedef struct
{
  __IO uint8_t ICKR;     /* Internal Clocks Control Register */
  __IO uint8_t ECKR;     /* External Clocks Control Register */
  uint8_t RESERVED1;     /* Reserved byte */
  __IO uint8_t CMSR;     /* Clock Master Status Register */
  __IO uint8_t SWR;      /* Clock Master Switch Register */
  __IO uint8_t SWCR;     /* Switch Control Register */
  __IO uint8_t CKDIVR;   /* Clock Divider Register */
  __IO uint8_t PCKENR1;  /* Peripheral Clock Gating Register 1 */
  __IO uint8_t CSSR;     /* Clock Security System Register */
  __IO uint8_t CCOR;     /* Configurable Clock Output Register */
  __IO uint8_t PCKENR2;  /* Peripheral Clock Gating Register 2 */
  uint8_t RESERVED2;     /* Reserved byte */
  __IO uint8_t HSITRIMR; /* HSI Calibration Trimmer Register */
  __IO uint8_t SWIMCCR;  /* SWIM clock control register */
} stm8_clk_t;

#define CLK_BaseAddress         0x50C0
#define CLK                     ((stm8_clk_t *)CLK_BaseAddress)

#define CLK_ICKR_SWUAH_MASK     ((uint8_t)0x20) /*!< Slow Wake-up from Active Halt/Halt modes */
#define CLK_ICKR_SWUAH_DISABLE  ((uint8_t)0x00)
#define CLK_ICKR_SWUAH_ENABLE   ((uint8_t)0x20)

#define CLK_ICKR_LSIRDY_MASK    ((uint8_t)0x10) /*!< Low speed internal oscillator ready */
#define CLK_ICKR_LSIRDY_NOTREADY ((uint8_t)0x00)
#define CLK_ICKR_LSIRDY_READY   ((uint8_t)0x10)

#define CLK_ICKR_LSIEN_MASK     ((uint8_t)0x08) /*!< Low speed internal RC oscillator enable */
#define CLK_ICKR_LSIEN_DISABLE  ((uint8_t)0x00)
#define CLK_ICKR_LSIEN_ENABLE   ((uint8_t)0x08)

#define CLK_ICKR_FHWU_MASK      ((uint8_t)0x04) /*!< Fast Wake-up from Active Halt/Halt mode */
#define CLK_ICKR_FHWU_DISABLE   ((uint8_t)0x00)
#define CLK_ICKR_FHWU_ENABLE    ((uint8_t)0x04)

#define CLK_ICKR_HSIRDY_MASK    ((uint8_t)0x02) /*!< High speed internal RC oscillator ready */
#define CLK_ICKR_HSIRDY_NOTREADY ((uint8_t)0x00)
#define CLK_ICKR_HSIRDY_READY   ((uint8_t)0x02)

#define CLK_ICKR_HSIEN_MASK     ((uint8_t)0x01) /*!< High speed internal RC oscillator enable */
#define CLK_ICKR_HSIEN_DISABLE  ((uint8_t)0x00)
#define CLK_ICKR_HSIEN_ENABLE   ((uint8_t)0x01)

#define CLK_ECKR_HSERDY_MASK    ((uint8_t)0x02) /*!< High speed external crystal oscillator ready */
#define CLK_ECKR_HSERDY_NOTREADY ((uint8_t)0x00)
#define CLK_ECKR_HSERDY_READY   ((uint8_t)0x02)

#define CLK_ECKR_HSEEN_MASK     ((uint8_t)0x01) /*!< High speed external crystal oscillator enable */
#define CLK_ECKR_HSEEN_DISABLE  ((uint8_t)0x01)
#define CLK_ECKR_HSEEN_ENABLE   ((uint8_t)0x01)

#define CLK_CMSR_CKM_MASK       ((uint8_t)0xFF) /*!< Clock master status bits */
#define CLK_CMSR_CKM_HSI        ((uint8_t)0xE1) /*!< Clock Source HSI. */
#define CLK_CMSR_CKM_LSI        ((uint8_t)0xD2) /*!< Clock Source LSI. */
#define CLK_CMSR_CKM_HSE        ((uint8_t)0xB4) /*!< Clock Source HSE. */

#define CLK_SWR_SWI_MASK        ((uint8_t)0xFF) /*!< Clock master selection bits */
#define CLK_SWR_SWI_HSI         ((uint8_t)0xE1) /*!< Clock Source HSI. */
#define CLK_SWR_SWI_LSI         ((uint8_t)0xD2) /*!< Clock Source LSI. */
#define CLK_SWR_SWI_HSE         ((uint8_t)0xB4) /*!< Clock Source HSE. */

#define CLK_SWCR_SWIF_MASK      ((uint8_t)0x08) /*!< Clock switch interrupt flag */
#define CLK_SWCR_SWIF_NOTREADY  ((uint8_t)0x00) // manual
#define CLK_SWCR_SWIF_READY     ((uint8_t)0x08)
#define CLK_SWCR_SWIF_NOOCCURANCE ((uint8_t)0x00) // auto
#define CLK_SWCR_SWIF_OCCURED   ((uint8_t)0x08)

#define CLK_SWCR_SWIEN_MASK     ((uint8_t)0x04) /*!< Clock switch interrupt enable */
#define CLK_SWCR_SWIEN_DISABLE  ((uint8_t)0x00)
#define CLK_SWCR_SWIEN_ENABLE   ((uint8_t)0x04)

#define CLK_SWCR_SWEN_MASK      ((uint8_t)0x02) /*!< Switch start/stop */
#define CLK_SWCR_SWEN_MANUAL    ((uint8_t)0x00)
#define CLK_SWCR_SWEN_AUTOMATIC ((uint8_t)0x02)

#define CLK_SWCR_SWBSY_MASK     ((uint8_t)0x01) /*!< Switch busy flag*/
#define CLK_SWCR_SWBSY_NOTBUSY  ((uint8_t)0x00)
#define CLK_SWCR_SWBSY_BUSY     ((uint8_t)0x01)

#define CLK_CKDIVR_HSIDIV_MASK  ((uint8_t)0x18) /*!< High speed internal clock prescaler */
#define CLK_CKDIVR_HSIDIV1      ((uint8_t)0x00) /*!< High speed internal clock prescaler: 1 */
#define CLK_CKDIVR_HSIDIV2      ((uint8_t)0x08) /*!< High speed internal clock prescaler: 2 */
#define CLK_CKDIVR_HSIDIV4      ((uint8_t)0x10) /*!< High speed internal clock prescaler: 4 */
#define CLK_CKDIVR_HSIDIV8      ((uint8_t)0x18) /*!< High speed internal clock prescaler: 8 */

#define CLK_CKDIVR_CPUDIV_MASK  ((uint8_t)0x07) /*!< CPU clock prescaler */
#define CLK_CKDIVR_CPUDIV1      ((uint8_t)0x00) /*!< CPU clock division factors 1 */
#define CLK_CKDIVR_CPUDIV2      ((uint8_t)0x01) /*!< CPU clock division factors 2 */
#define CLK_CKDIVR_CPUDIV4      ((uint8_t)0x02) /*!< CPU clock division factors 4 */
#define CLK_CKDIVR_CPUDIV8      ((uint8_t)0x03) /*!< CPU clock division factors 8 */
#define CLK_CKDIVR_CPUDIV16     ((uint8_t)0x04) /*!< CPU clock division factors 16 */
#define CLK_CKDIVR_CPUDIV32     ((uint8_t)0x05) /*!< CPU clock division factors 32 */
#define CLK_CKDIVR_CPUDIV64     ((uint8_t)0x06) /*!< CPU clock division factors 64 */
#define CLK_CKDIVR_CPUDIV128    ((uint8_t)0x07) /*!< CPU clock division factors 128 */

#define CLK_PCKENR1_MASK        ((uint8_t)0xFF)
#define CLK_PCKENR1_TIM1        ((uint8_t)0x80) /*!< Timer 1 clock enable */
#define CLK_PCKENR1_TIM3        ((uint8_t)0x40) /*!< Timer 3 clock enable */
#define CLK_PCKENR1_TIM2        ((uint8_t)0x20) /*!< Timer 2 clock enable */
#define CLK_PCKENR1_TIM5        ((uint8_t)0x20) /*!< Timer 5 clock enable */
#define CLK_PCKENR1_TIM4        ((uint8_t)0x10) /*!< Timer 4 clock enable */
#define CLK_PCKENR1_TIM6        ((uint8_t)0x10) /*!< Timer 6 clock enable */
#define CLK_PCKENR1_UART3       ((uint8_t)0x08) /*!< UART3 clock enable */
#define CLK_PCKENR1_UART2       ((uint8_t)0x08) /*!< UART2 clock enable */
#define CLK_PCKENR1_UART1       ((uint8_t)0x04) /*!< UART1 clock enable */
#define CLK_PCKENR1_SPI         ((uint8_t)0x02) /*!< SPI clock enable */
#define CLK_PCKENR1_I2C         ((uint8_t)0x01) /*!< I2C clock enable */

#define CLK_PCKENR2_MASK        ((uint8_t)0x8C)
#define CLK_PCKENR2_CAN         ((uint8_t)0x80) /*!< CAN clock enable */
#define CLK_PCKENR2_ADC         ((uint8_t)0x08) /*!< ADC clock enable */
#define CLK_PCKENR2_AWU         ((uint8_t)0x04) /*!< AWU clock enable */

#define CLK_CSSR_CSSD        ((uint8_t)0x08) /*!< Clock security system detection */
#define CLK_CSSR_CSSDIE      ((uint8_t)0x04) /*!< Clock security system detection interrupt enable */
#define CLK_CSSR_AUX         ((uint8_t)0x02) /*!< Auxiliary oscillator connected to master clock */
#define CLK_CSSR_CSSEN       ((uint8_t)0x01) /*!< Clock security system enable */

#define CLK_CCOR_CCOBSY      ((uint8_t)0x40) /*!< Configurable clock output busy */
#define CLK_CCOR_CCORDY      ((uint8_t)0x20) /*!< Configurable clock output ready */
#define CLK_CCOR_CCOSEL      ((uint8_t)0x1E) /*!< Configurable clock output selection */
#define CLK_CCOR_CCOEN       ((uint8_t)0x01) /*!< Configurable clock output enable */

#define CLK_HSITRIMR_HSITRIM_MASK   ((uint8_t)0x07) /*!< High speed internal oscillator trimmer */
#define CLK_HSITRIMR_HSITRIM_0      ((uint8_t)0x000 /*!< HSI Calibration Value 0 */
#define CLK_HSITRIMR_HSITRIM_1      ((uint8_t)0x01) /*!< HSI Calibration Value 1 */
#define CLK_HSITRIMR_HSITRIM_2      ((uint8_t)0x02) /*!< HSI Calibration Value 2 */
#define CLK_HSITRIMR_HSITRIM_3      ((uint8_t)0x03) /*!< HSI Calibration Value 3 */
#define CLK_HSITRIMR_HSITRIM_4      ((uint8_t)0x04) /*!< HSI Calibration Value 4 */
#define CLK_HSITRIMR_HSITRIM_5      ((uint8_t)0x05) /*!< HSI Calibration Value 5 */
#define CLK_HSITRIMR_HSITRIM_6      ((uint8_t)0x06) /*!< HSI Calibration Value 6 */
#define CLK_HSITRIMR_HSITRIM_7      ((uint8_t)0x07) /*!< HSI Calibration Value 7 */

#define CLK_SWIMCCR_SWIMDIV  ((uint8_t)0x01) /*!< SWIM Clock Dividing Factor */


//=============================================================================
// General Purpose Input/Output
//
typedef struct
{
  __IO uint8_t ODR; /* Output Data Register */
  __IO uint8_t IDR; /* Input Data Register */
  __IO uint8_t DDR; /* Data Direction Register */
  __IO uint8_t CR1; /* Configuration Register 1 */
  __IO uint8_t CR2; /* Configuration Register 2 */
} stm8_gpio_t;

#define GPIOA_BaseAddress       0x5000
#define GPIOB_BaseAddress       0x5005
#define GPIOC_BaseAddress       0x500A
#define GPIOD_BaseAddress       0x500F
#define GPIOE_BaseAddress       0x5014
#define GPIOF_BaseAddress       0x5019
#define GPIOA                   ((stm8_gpio_t *)GPIOA_BaseAddress)
#define GPIOB                   ((stm8_gpio_t *)GPIOB_BaseAddress)
#define GPIOC                   ((stm8_gpio_t *)GPIOC_BaseAddress)
#define GPIOD                   ((stm8_gpio_t *)GPIOD_BaseAddress)
#define GPIOE                   ((stm8_gpio_t *)GPIOE_BaseAddress)
#define GPIOF                   ((stm8_gpio_t *)GPIOF_BaseAddress)

#define GPIO_PIN_0              ((uint8_t)0x01)
#define GPIO_PIN_1              ((uint8_t)0x02)
#define GPIO_PIN_2              ((uint8_t)0x04)
#define GPIO_PIN_3              ((uint8_t)0x08)
#define GPIO_PIN_4              ((uint8_t)0x10)
#define GPIO_PIN_5              ((uint8_t)0x20)
#define GPIO_PIN_6              ((uint8_t)0x40)
#define GPIO_PIN_7              ((uint8_t)0x80)

#define GPIO_ODR_0_MASK         ((uint8_t)0x01)
#define GPIO_ODR_0_LOW          ((uint8_t)0x00)
#define GPIO_ODR_0_HIGH         ((uint8_t)0x01)
#define GPIO_ODR_1_MASK         ((uint8_t)0x02)
#define GPIO_ODR_1_LOW          ((uint8_t)0x00)
#define GPIO_ODR_1_HIGH         ((uint8_t)0x02)
#define GPIO_ODR_2_MASK         ((uint8_t)0x04)
#define GPIO_ODR_2_LOW          ((uint8_t)0x00)
#define GPIO_ODR_2_HIGH         ((uint8_t)0x04)
#define GPIO_ODR_3_MASK         ((uint8_t)0x08)
#define GPIO_ODR_3_LOW          ((uint8_t)0x00)
#define GPIO_ODR_3_HIGH         ((uint8_t)0x08)
#define GPIO_ODR_4_MASK         ((uint8_t)0x10)
#define GPIO_ODR_4_LOW          ((uint8_t)0x00)
#define GPIO_ODR_4_HIGH         ((uint8_t)0x10)
#define GPIO_ODR_5_MASK         ((uint8_t)0x20)
#define GPIO_ODR_5_LOW          ((uint8_t)0x00)
#define GPIO_ODR_5_HIGH         ((uint8_t)0x20)
#define GPIO_ODR_6_MASK         ((uint8_t)0x40)
#define GPIO_ODR_6_LOW          ((uint8_t)0x00)
#define GPIO_ODR_6_HIGH         ((uint8_t)0x40)
#define GPIO_ODR_7_MASK         ((uint8_t)0x80)
#define GPIO_ODR_7_LOW          ((uint8_t)0x00)
#define GPIO_ODR_7_HIGH         ((uint8_t)0x80)

#define GPIO_IDR_0_MASK         ((uint8_t)0x01)
#define GPIO_IDR_0_LOW          ((uint8_t)0x00)
#define GPIO_IDR_0_HIGH         ((uint8_t)0x01)
#define GPIO_IDR_1_MASK         ((uint8_t)0x02)
#define GPIO_IDR_1_LOW          ((uint8_t)0x00)
#define GPIO_IDR_1_HIGH         ((uint8_t)0x02)
#define GPIO_IDR_2_MASK         ((uint8_t)0x04)
#define GPIO_IDR_2_LOW          ((uint8_t)0x00)
#define GPIO_IDR_2_HIGH         ((uint8_t)0x04)
#define GPIO_IDR_3_MASK         ((uint8_t)0x08)
#define GPIO_IDR_3_LOW          ((uint8_t)0x00)
#define GPIO_IDR_3_HIGH         ((uint8_t)0x08)
#define GPIO_IDR_4_MASK         ((uint8_t)0x10)
#define GPIO_IDR_4_LOW          ((uint8_t)0x00)
#define GPIO_IDR_4_HIGH         ((uint8_t)0x10)
#define GPIO_IDR_5_MASK         ((uint8_t)0x20)
#define GPIO_IDR_5_LOW          ((uint8_t)0x00)
#define GPIO_IDR_5_HIGH         ((uint8_t)0x20)
#define GPIO_IDR_6_MASK         ((uint8_t)0x40)
#define GPIO_IDR_6_LOW          ((uint8_t)0x00)
#define GPIO_IDR_6_HIGH         ((uint8_t)0x40)
#define GPIO_IDR_7_MASK         ((uint8_t)0x80)
#define GPIO_IDR_7_LOW          ((uint8_t)0x00)
#define GPIO_IDR_7_HIGH         ((uint8_t)0x80)

#define GPIO_DDR_0_MASK         ((uint8_t)0x01)
#define GPIO_DDR_0_INPUT        ((uint8_t)0x00)
#define GPIO_DDR_0_OUTPUT       ((uint8_t)0x01)
#define GPIO_DDR_1_MASK         ((uint8_t)0x02)
#define GPIO_DDR_1_INPUT        ((uint8_t)0x00)
#define GPIO_DDR_1_OUTPUT       ((uint8_t)0x02)
#define GPIO_DDR_2_MASK         ((uint8_t)0x04)
#define GPIO_DDR_2_INPUT        ((uint8_t)0x00)
#define GPIO_DDR_2_OUTPUT       ((uint8_t)0x04)
#define GPIO_DDR_3_MASK         ((uint8_t)0x08)
#define GPIO_DDR_3_INPUT        ((uint8_t)0x00)
#define GPIO_DDR_3_OUTPUT       ((uint8_t)0x08)
#define GPIO_DDR_4_MASK         ((uint8_t)0x10)
#define GPIO_DDR_4_INPUT        ((uint8_t)0x00)
#define GPIO_DDR_4_OUTPUT       ((uint8_t)0x10)
#define GPIO_DDR_5_MASK         ((uint8_t)0x20)
#define GPIO_DDR_5_INPUT        ((uint8_t)0x00)
#define GPIO_DDR_5_OUTPUT       ((uint8_t)0x20)
#define GPIO_DDR_6_MASK         ((uint8_t)0x40)
#define GPIO_DDR_6_INPUT        ((uint8_t)0x00)
#define GPIO_DDR_6_OUTPUT       ((uint8_t)0x40)
#define GPIO_DDR_7_MASK         ((uint8_t)0x80)
#define GPIO_DDR_7_INPUT        ((uint8_t)0x00)
#define GPIO_DDR_7_OUTPUT       ((uint8_t)0x80)

#define GPIO_CR1_0_INPUT_MASK       ((uint8_t)0x01)
#define GPIO_CR1_0_INPUT_FLOATING   ((uint8_t)0x00)
#define GPIO_CR1_0_INPUT_PULLUP     ((uint8_t)0x01)
#define GPIO_CR1_1_INPUT_MASK       ((uint8_t)0x02)
#define GPIO_CR1_1_INPUT_FLOATING   ((uint8_t)0x00)
#define GPIO_CR1_1_INPUT_PULLUP     ((uint8_t)0x02)
#define GPIO_CR1_2_INPUT_MASK       ((uint8_t)0x04)
#define GPIO_CR1_2_INPUT_FLOATING   ((uint8_t)0x00)
#define GPIO_CR1_2_INPUT_PULLUP     ((uint8_t)0x04)
#define GPIO_CR1_3_INPUT_MASK       ((uint8_t)0x08)
#define GPIO_CR1_3_INPUT_FLOATING   ((uint8_t)0x00)
#define GPIO_CR1_3_INPUT_PULLUP     ((uint8_t)0x08)
#define GPIO_CR1_4_INPUT_MASK       ((uint8_t)0x10)
#define GPIO_CR1_4_INPUT_FLOATING   ((uint8_t)0x00)
#define GPIO_CR1_4_INPUT_PULLUP     ((uint8_t)0x10)
#define GPIO_CR1_5_INPUT_MASK       ((uint8_t)0x20)
#define GPIO_CR1_5_INPUT_FLOATING   ((uint8_t)0x00)
#define GPIO_CR1_5_INPUT_PULLUP     ((uint8_t)0x20)
#define GPIO_CR1_6_INPUT_MASK       ((uint8_t)0x40)
#define GPIO_CR1_6_INPUT_FLOATING   ((uint8_t)0x00)
#define GPIO_CR1_6_INPUT_PULLUP     ((uint8_t)0x40)
#define GPIO_CR1_7_INPUT_MASK       ((uint8_t)0x80)
#define GPIO_CR1_7_INPUT_FLOATING   ((uint8_t)0x00)
#define GPIO_CR1_7_INPUT_PULLUP     ((uint8_t)0x80)

#define GPIO_CR1_0_OUTPUT_MASK          ((uint8_t)0x01)
#define GPIO_CR1_0_OUTPUT_OPENDRAIN     ((uint8_t)0x00)
#define GPIO_CR1_0_OUTPUT_PUSHPULL      ((uint8_t)0x01)
#define GPIO_CR1_1_OUTPUT_MASK          ((uint8_t)0x02)
#define GPIO_CR1_1_OUTPUT_OPENDRAIN     ((uint8_t)0x00)
#define GPIO_CR1_1_OUTPUT_PUSHPULL      ((uint8_t)0x02)
#define GPIO_CR1_2_OUTPUT_MASK          ((uint8_t)0x04)
#define GPIO_CR1_2_OUTPUT_OPENDRAIN     ((uint8_t)0x00)
#define GPIO_CR1_2_OUTPUT_PUSHPULL      ((uint8_t)0x04)
#define GPIO_CR1_3_OUTPUT_MASK          ((uint8_t)0x08)
#define GPIO_CR1_3_OUTPUT_OPENDRAIN     ((uint8_t)0x00)
#define GPIO_CR1_3_OUTPUT_PUSHPULL      ((uint8_t)0x08)
#define GPIO_CR1_4_OUTPUT_MASK          ((uint8_t)0x10)
#define GPIO_CR1_4_OUTPUT_OPENDRAIN     ((uint8_t)0x00)
#define GPIO_CR1_4_OUTPUT_PUSHPULL      ((uint8_t)0x10)
#define GPIO_CR1_5_OUTPUT_MASK          ((uint8_t)0x20)
#define GPIO_CR1_5_OUTPUT_OPENDRAIN     ((uint8_t)0x00)
#define GPIO_CR1_5_OUTPUT_PUSHPULL      ((uint8_t)0x20)
#define GPIO_CR1_6_OUTPUT_MASK          ((uint8_t)0x40)
#define GPIO_CR1_6_OUTPUT_OPENDRAIN     ((uint8_t)0x00)
#define GPIO_CR1_6_OUTPUT_PUSHPULL      ((uint8_t)0x40)
#define GPIO_CR1_7_OUTPUT_MASK          ((uint8_t)0x80)
#define GPIO_CR1_7_OUTPUT_OPENDRAIN     ((uint8_t)0x00)
#define GPIO_CR1_7_OUTPUT_PUSHPULL      ((uint8_t)0x80)

#define GPIO_CR2_0_INTERRUPT_MASK       ((uint8_t)0x01)
#define GPIO_CR2_0_INTERRUPT_DISABLE    ((uint8_t)0x00)
#define GPIO_CR2_0_INTERRUPT_ENABLEUP   ((uint8_t)0x01)
#define GPIO_CR2_1_INTERRUPT_MASK       ((uint8_t)0x02)
#define GPIO_CR2_1_INTERRUPT_DISABLE    ((uint8_t)0x00)
#define GPIO_CR2_1_INTERRUPT_ENABLEUP   ((uint8_t)0x02)
#define GPIO_CR2_2_INTERRUPT_MASK       ((uint8_t)0x04)
#define GPIO_CR2_2_INTERRUPT_DISABLE    ((uint8_t)0x00)
#define GPIO_CR2_2_INTERRUPT_ENABLEUP   ((uint8_t)0x04)
#define GPIO_CR2_3_INTERRUPT_MASK       ((uint8_t)0x08)
#define GPIO_CR2_3_INTERRUPT_DISABLE    ((uint8_t)0x00)
#define GPIO_CR2_3_INTERRUPT_ENABLEUP   ((uint8_t)0x08)
#define GPIO_CR2_4_INTERRUPT_MASK       ((uint8_t)0x10)
#define GPIO_CR2_4_INTERRUPT_DISABLE    ((uint8_t)0x00)
#define GPIO_CR2_4_INTERRUPT_ENABLEUP   ((uint8_t)0x10)
#define GPIO_CR2_5_INTERRUPT_MASK       ((uint8_t)0x20)
#define GPIO_CR2_5_INTERRUPT_DISABLE    ((uint8_t)0x00)
#define GPIO_CR2_5_INTERRUPT_ENABLEUP   ((uint8_t)0x20)
#define GPIO_CR2_6_INTERRUPT_MASK       ((uint8_t)0x40)
#define GPIO_CR2_6_INTERRUPT_DISABLE    ((uint8_t)0x00)
#define GPIO_CR2_6_INTERRUPT_ENABLEUP   ((uint8_t)0x40)
#define GPIO_CR2_7_INTERRUPT_MASK       ((uint8_t)0x80)
#define GPIO_CR2_7_INTERRUPT_DISABLE    ((uint8_t)0x00)
#define GPIO_CR2_7_INTERRUPT_ENABLEUP   ((uint8_t)0x80)

#define GPIO_CR2_0_OUTPUT_MASK      ((uint8_t)0x01)
#define GPIO_CR2_0_OUTPUT_2MHZ      ((uint8_t)0x00)
#define GPIO_CR2_0_OUTPUT_10MHZ     ((uint8_t)0x01)
#define GPIO_CR2_1_OUTPUT_MASK      ((uint8_t)0x02)
#define GPIO_CR2_1_OUTPUT_2MHZ      ((uint8_t)0x00)
#define GPIO_CR2_1_OUTPUT_10MHZ     ((uint8_t)0x02)
#define GPIO_CR2_2_OUTPUT_MASK      ((uint8_t)0x04)
#define GPIO_CR2_2_OUTPUT_2MHZ      ((uint8_t)0x00)
#define GPIO_CR2_2_OUTPUT_10MHZ     ((uint8_t)0x04)
#define GPIO_CR2_3_OUTPUT_MASK      ((uint8_t)0x08)
#define GPIO_CR2_3_OUTPUT_2MHZ      ((uint8_t)0x00)
#define GPIO_CR2_3_OUTPUT_10MHZ     ((uint8_t)0x08)
#define GPIO_CR2_4_OUTPUT_MASK      ((uint8_t)0x10)
#define GPIO_CR2_4_OUTPUT_2MHZ      ((uint8_t)0x00)
#define GPIO_CR2_4_OUTPUT_10MHZ     ((uint8_t)0x10)
#define GPIO_CR2_5_OUTPUT_MASK      ((uint8_t)0x20)
#define GPIO_CR2_5_OUTPUT_2MHZ      ((uint8_t)0x00)
#define GPIO_CR2_5_OUTPUT_10MHZ     ((uint8_t)0x20)
#define GPIO_CR2_6_OUTPUT_MASK      ((uint8_t)0x40)
#define GPIO_CR2_6_OUTPUT_2MHZ      ((uint8_t)0x00)
#define GPIO_CR2_6_OUTPUT_10MHZ     ((uint8_t)0x40)
#define GPIO_CR2_7_OUTPUT_MASK      ((uint8_t)0x80)
#define GPIO_CR2_7_OUTPUT_2MHZ      ((uint8_t)0x00)
#define GPIO_CR2_7_OUTPUT_10MHZ     ((uint8_t)0x80)


//=============================================================================
// Timers
//
typedef struct
{
  __IO uint8_t CR1;   /* control register 1 */
  __IO uint8_t CR2;   /* control register 2 */
  __IO uint8_t SMCR;  /* Synchro mode control register */
  __IO uint8_t ETR;   /* external trigger register */
  __IO uint8_t IER;   /* interrupt enable register*/
  __IO uint8_t SR1;   /* status register 1 */
  __IO uint8_t SR2;   /* status register 2 */
  __IO uint8_t EGR;   /* event generation register */
  __IO uint8_t CCMR1; /* CC mode register 1 */
  __IO uint8_t CCMR2; /* CC mode register 2 */
  __IO uint8_t CCMR3; /* CC mode register 3 */
  __IO uint8_t CCMR4; /* CC mode register 4 */
  __IO uint8_t CCER1; /* CC enable register 1 */
  __IO uint8_t CCER2; /* CC enable register 2 */
  __IO uint8_t CNTRH; /* counter high */
  __IO uint8_t CNTRL; /* counter low */
  __IO uint8_t PSCRH; /* prescaler high */
  __IO uint8_t PSCRL; /* prescaler low */
  __IO uint8_t ARRH;  /* auto-reload register high */
  __IO uint8_t ARRL;  /* auto-reload register low */
  __IO uint8_t RCR;   /* Repetition Counter register */
  __IO uint8_t CCR1H; /* capture/compare register 1 high */
  __IO uint8_t CCR1L; /* capture/compare register 1 low */
  __IO uint8_t CCR2H; /* capture/compare register 2 high */
  __IO uint8_t CCR2L; /* capture/compare register 2 low */
  __IO uint8_t CCR3H; /* capture/compare register 3 high */
  __IO uint8_t CCR3L; /* capture/compare register 3 low */
  __IO uint8_t CCR4H; /* capture/compare register 3 high */
  __IO uint8_t CCR4L; /* capture/compare register 3 low */
  __IO uint8_t BKR;   /* Break Register */
  __IO uint8_t DTR;   /* dead-time register */
  __IO uint8_t OISR;  /* Output idle register */
} stm8_tim1_t;

typedef struct
{
  __IO uint8_t CR1;  /* control register 1 */
  __IO uint8_t IER;  /* interrupt enable register */
  __IO uint8_t SR1;  /* status register 1 */
  __IO uint8_t EGR;  /* event generation register */
  __IO uint8_t CNTR; /* counter register */
  __IO uint8_t PSCR; /* prescaler register */
  __IO uint8_t ARR;  /* auto-reload register */
} stm8_tim4_t;

#define TIM1_BaseAddress        0x5250
#define TIM2_BaseAddress        0x5300
#define TIM3_BaseAddress        0x5320
#define TIM4_BaseAddress        0x5340
#define TIM5_BaseAddress        0x5300
#define TIM6_BaseAddress        0x5340
#define TIM1                    ((stm8_tim1_t *)TIM1_BaseAddress)
#define TIM4                    ((stm8_tim4_t *)TIM4_BaseAddress)

#define TIM1_CR1_ARPE_MASK      ((uint8_t)0x80) /*!< Auto-Reload Preload Enable mask. */
#define TIM1_CR1_ARPE_DISABLE   ((uint8_t)0x00)
#define TIM1_CR1_ARPE_ENABLE    ((uint8_t)0x80)

#define TIM1_CR1_CMS_MASK       ((uint8_t)0x60) /*!< Center-aligned Mode Selection mask. */
#define TIM1_CR1_CMS_EDGE       ((uint8_t)0x00)
#define TIM1_CR1_CMS_CENTER1    ((uint8_t)0x20)
#define TIM1_CR1_CMS_CENTER2    ((uint8_t)0x40)
#define TIM1_CR1_CMS_CENTER3    ((uint8_t)0x60)

#define TIM1_CR1_DIR_MASK       ((uint8_t)0x10) /*!< Direction mask. */
#define TIM1_CR1_DIR_UP         ((uint8_t)0x00)
#define TIM1_CR1_DIR_DOWN       ((uint8_t)0x10)

#define TIM1_CR1_OPM_MASK       ((uint8_t)0x08) /*!< One Pulse Mode mask. */
#define TIM1_CR1_OPM_DISABLE    ((uint8_t)0x00)
#define TIM1_CR1_OPM_ENABLE     ((uint8_t)0x08)

#define TIM1_CR1_URS_MASK       ((uint8_t)0x04) /*!< Update Request Source mask. */
#define TIM1_CR1_URS_ALL        ((uint8_t)0x00)
#define TIM1_CR1_URS_UPDATE     ((uint8_t)0x04)

#define TIM1_CR1_UDIS_MASK      ((uint8_t)0x02) /*!< Update DIsable mask. */
#define TIM1_CR1_UDIS_DISABLE   ((uint8_t)0x00)
#define TIM1_CR1_UDIS_ENABLE    ((uint8_t)0x02)

#define TIM1_CR1_CEN_MASK       ((uint8_t)0x01) /*!< Counter Enable mask. */
#define TIM1_CR1_CEN_DISABLE    ((uint8_t)0x00)
#define TIM1_CR1_CEN_ENABLE     ((uint8_t)0x01)

#define TIM1_CR2_MMS_MASK       ((uint8_t)0x70) /*!< MMS Selection mask. */
#define TIM1_CR2_MMS_RESET      ((uint8_t)0x00)
#define TIM1_CR2_MMS_ENABLE     ((uint8_t)0x10)
#define TIM1_CR2_MMS_UPDATE     ((uint8_t)0x20)
#define TIM1_CR2_MMS_OC1        ((uint8_t)0x30)
#define TIM1_CR2_MMS_OC1REF     ((uint8_t)0x40)
#define TIM1_CR2_MMS_OC2REF     ((uint8_t)0x50)
#define TIM1_CR2_MMS_OC3REF     ((uint8_t)0x60)
#define TIM1_CR2_MMS_OC4REF     ((uint8_t)0x70)

#define TIM1_CR2_COMS_MASK      ((uint8_t)0x04) /*!< Capture/Compare Control Update Selection mask. */
#define TIM1_CR2_COMS_COMG      ((uint8_t)0x00)
#define TIM1_CR2_COMS_ALL       ((uint8_t)0x04)

#define TIM1_CR2_CCPC_MASK      ((uint8_t)0x01) /*!< Capture/Compare Preloaded Control mask. */
#define TIM1_CR2_CCPC_NOTPRELOAD ((uint8_t)0x00)
#define TIM1_CR2_CCPC_PRELOADED ((uint8_t)0x01)

#define TIM1_SMCR_MSM_MASK      ((uint8_t)0x80) /*!< Master/Slave Mode mask. */
#define TIM1_SMCR_MSM_DISABLE   ((uint8_t)0x00)
#define TIM1_SMCR_MSM_ENABLE    ((uint8_t)0x80)

#define TIM1_SMCR_TS_MASK       ((uint8_t)0x70) /*!< Trigger Selection mask. */
#define TIM1_SMCR_TS_TIM6       ((uint8_t)0x00)  /*!< TRIG Input source =  TIM6 TRIG Output  */
#define TIM1_SMCR_TS_TIM5       ((uint8_t)0x30)  /*!< TRIG Input source =  TIM5 TRIG Output  */
#define TIM1_SMCR_TS_TI1F_ED    ((uint8_t)0x40)
#define TIM1_SMCR_TS_TI1FP1     ((uint8_t)0x50)
#define TIM1_SMCR_TS_TI2FP2     ((uint8_t)0x60)
#define TIM1_SMCR_TS_ETRF       ((uint8_t)0x70)

#define TIM1_SMCR_SMS_MASK      ((uint8_t)0x07) /*!< Slave Mode Selection mask. */
#define TIM1_SMCR_SMS_DISABLE   ((uint8_t)0x00)
#define TIM1_SMCR_SMS_ENCODER1  ((uint8_t)0x01)
#define TIM1_SMCR_SMS_ENCODER2  ((uint8_t)0x02)
#define TIM1_SMCR_SMS_ENCODER3  ((uint8_t)0x03)
#define TIM1_SMCR_SMS_RESET     ((uint8_t)0x04)
#define TIM1_SMCR_SMS_GATED     ((uint8_t)0x05)
#define TIM1_SMCR_SMS_TRIGGER   ((uint8_t)0x06)
#define TIM1_SMCR_SMS_EXTERNAL1 ((uint8_t)0x07)

#define TIM1_ETR_ETP_MASK       ((uint8_t)0x80) /*!< External Trigger Polarity mask. */
#define TIM1_ETR_ETP_NOTINVERTED ((uint8_t)0x00)
#define TIM1_ETR_ETP_INVERTED   ((uint8_t)0x80)

#define TIM1_ETR_ECE_MASK       ((uint8_t)0x40) /*!< External Clock mask. */
#define TIM1_ETR_ECE_DISABLE    ((uint8_t)0x00)
#define TIM1_ETR_ECE_ENABLE     ((uint8_t)0x40)

#define TIM1_ETR_ETPS_MASK      ((uint8_t)0x30) /*!< External Trigger Prescaler mask. */
#define TIM1_ETR_ETPS_OFF       ((uint8_t)0x00)
#define TIM1_ETR_ETPS_DIV2      ((uint8_t)0x10)
#define TIM1_ETR_ETPS_DIV4      ((uint8_t)0x20)
#define TIM1_ETR_ETPS_DIV8      ((uint8_t)0x30)

#define TIM1_ETR_ETF_MASK       ((uint8_t)0x0F) /*!< External Trigger Filter mask. */
#define TIM1_ETR_ETF_DISABLE    ((uint8_t)0x00)
#define TIM1_ETR_ETF_DIV1_N2    ((uint8_t)0x01)
#define TIM1_ETR_ETF_DIV1_N4    ((uint8_t)0x02)
#define TIM1_ETR_ETF_DIV1_N8    ((uint8_t)0x03)
#define TIM1_ETR_ETF_DIV2_N6    ((uint8_t)0x04)
#define TIM1_ETR_ETF_DIV2_N8    ((uint8_t)0x05)
#define TIM1_ETR_ETF_DIV4_N6    ((uint8_t)0x06)
#define TIM1_ETR_ETF_DIV4_N8    ((uint8_t)0x07)
#define TIM1_ETR_ETF_DIV8_N6    ((uint8_t)0x08)
#define TIM1_ETR_ETF_DIV8_N8    ((uint8_t)0x09)
#define TIM1_ETR_ETF_DIV16_N5   ((uint8_t)0x0A)
#define TIM1_ETR_ETF_DIV16_N6   ((uint8_t)0x0B)
#define TIM1_ETR_ETF_DIV16_N8   ((uint8_t)0x0C)
#define TIM1_ETR_ETF_DIV32_N5   ((uint8_t)0x0D)
#define TIM1_ETR_ETF_DIV32_N6   ((uint8_t)0x0E)
#define TIM1_ETR_ETF_DIV32_N8   ((uint8_t)0x0F)

#define TIM1_IER_BIE_MASK       ((uint8_t)0x80) /*!< Break Interrupt Enable mask. */
#define TIM1_IER_BIE_DISABLE    ((uint8_t)0x00)
#define TIM1_IER_BIE_ENABLE     ((uint8_t)0x80)

#define TIM1_IER_TIE_MASK       ((uint8_t)0x40) /*!< Trigger Interrupt Enable mask. */
#define TIM1_IER_TIE_DISABLE    ((uint8_t)0x00)
#define TIM1_IER_TIE_ENABLE     ((uint8_t)0x40)

#define TIM1_IER_COMIE_MASK     ((uint8_t)0x20) /*!<  Commutation Interrupt Enable mask.*/
#define TIM1_IER_COMIE_DISABLE  ((uint8_t)0x00)
#define TIM1_IER_COMIE_ENABLE   ((uint8_t)0x20)

#define TIM1_IER_CC4IE_MASK     ((uint8_t)0x10) /*!< Capture/Compare 4 Interrupt Enable mask. */
#define TIM1_IER_CC4IE_DISABLE  ((uint8_t)0x00)
#define TIM1_IER_CC4IE_ENABLE   ((uint8_t)0x10)

#define TIM1_IER_CC3IE_MASK     ((uint8_t)0x08) /*!< Capture/Compare 3 Interrupt Enable mask. */
#define TIM1_IER_CC3IE_DISABLE  ((uint8_t)0x00)
#define TIM1_IER_CC3IE_ENABLE   ((uint8_t)0x08)

#define TIM1_IER_CC2IE_MASK     ((uint8_t)0x04) /*!< Capture/Compare 2 Interrupt Enable mask. */
#define TIM1_IER_CC2IE_DISABLE  ((uint8_t)0x00)
#define TIM1_IER_CC2IE_ENABLE   ((uint8_t)0x04)

#define TIM1_IER_CC1IE_MASK     ((uint8_t)0x02) /*!< Capture/Compare 1 Interrupt Enable mask. */
#define TIM1_IER_CC1IE_DISABLE  ((uint8_t)0x00)
#define TIM1_IER_CC1IE_ENABLE   ((uint8_t)0x02)

#define TIM1_IER_UIE_MASK       ((uint8_t)0x01) /*!< Update Interrupt Enable mask. */
#define TIM1_IER_UIE_DISABLE    ((uint8_t)0x00)
#define TIM1_IER_UIE_ENABLE     ((uint8_t)0x01)

#define TIM1_SR1_BIF_MASK       ((uint8_t)0x80) /*!< Break Interrupt Flag mask. */
#define TIM1_SR1_BIF_NONE       ((uint8_t)0x00)
#define TIM1_SR1_BIF_DETECTED   ((uint8_t)0x80)

#define TIM1_SR1_TIF_MASK       ((uint8_t)0x40) /*!< Trigger Interrupt Flag mask. */
#define TIM1_SR1_TIF_NONE       ((uint8_t)0x00)
#define TIM1_SR1_TIF_PENDING    ((uint8_t)0x40)

#define TIM1_SR1_COMIF_MASK     ((uint8_t)0x20) /*!< Commutation Interrupt Flag mask. */
#define TIM1_SR1_COMIF_NONE     ((uint8_t)0x00)
#define TIM1_SR1_COMIF_PENDING  ((uint8_t)0x20)

#define TIM1_SR1_CC4IF_MASK     ((uint8_t)0x10) /*!< Capture/Compare 4 Interrupt Flag mask. */
#define TIM1_SR1_CC4IF_NONE     ((uint8_t)0x00)
#define TIM1_SR1_CC4IF_MATCH    ((uint8_t)0x10) // Input
#define TIM1_SR1_CC4IF_CAPTURE  ((uint8_t)0x10) // Output

#define TIM1_SR1_CC3IF_MASK     ((uint8_t)0x08) /*!< Capture/Compare 3 Interrupt Flag mask. */
#define TIM1_SR1_CC3IF_NONE     ((uint8_t)0x00)
#define TIM1_SR1_CC3IF_MATCH    ((uint8_t)0x08) // Input
#define TIM1_SR1_CC3IF_CAPTURE  ((uint8_t)0x08) // Output

#define TIM1_SR1_CC2IF_MASK     ((uint8_t)0x04) /*!< Capture/Compare 2 Interrupt Flag mask. */
#define TIM1_SR1_CC2IF_NONE     ((uint8_t)0x00)
#define TIM1_SR1_CC2IF_MATCH    ((uint8_t)0x04) // Input
#define TIM1_SR1_CC2IF_CAPTURE  ((uint8_t)0x04) // Output

#define TIM1_SR1_CC1IF_MASK     ((uint8_t)0x02) /*!< Capture/Compare 1 Interrupt Flag mask. */
#define TIM1_SR1_CC1IF_NONE     ((uint8_t)0x00)
#define TIM1_SR1_CC1IF_MATCH    ((uint8_t)0x02) // Input
#define TIM1_SR1_CC1IF_CAPTURE  ((uint8_t)0x02) // Output

#define TIM1_SR1_UIF_MASK       ((uint8_t)0x01) /*!< Update Interrupt Flag mask. */
#define TIM1_SR1_UIF_NONE       ((uint8_t)0x00)
#define TIM1_SR1_UIF_PENDING    ((uint8_t)0x01)

#define TIM1_SR2_CC4OF_MASK     ((uint8_t)0x10) /*!< Capture/Compare 4 Overcapture Flag mask. */
#define TIM1_SR2_CC4OF_NONE     ((uint8_t)0x00)
#define TIM1_SR2_CC4OF_DETECTED ((uint8_t)0x10)

#define TIM1_SR2_CC3OF_MASK     ((uint8_t)0x08) /*!< Capture/Compare 3 Overcapture Flag mask. */
#define TIM1_SR2_CC3OF_NONE     ((uint8_t)0x00)
#define TIM1_SR2_CC3OF_DETECTED ((uint8_t)0x08)

#define TIM1_SR2_CC2OF_MASK     ((uint8_t)0x04) /*!< Capture/Compare 2 Overcapture Flag mask. */
#define TIM1_SR2_CC2OF_NONE     ((uint8_t)0x00)
#define TIM1_SR2_CC2OF_DETECTED ((uint8_t)0x04)

#define TIM1_SR2_CC1OF_MASK     ((uint8_t)0x02) /*!< Capture/Compare 1 Overcapture Flag mask. */
#define TIM1_SR2_CC1OF_NONE     ((uint8_t)0x00)
#define TIM1_SR2_CC1OF_DETECTED ((uint8_t)0x02)

#define TIM1_EGR_BG_MASK        ((uint8_t)0x80) /*!< Break Generation mask. */
#define TIM1_EGR_BG_DISABLE     ((uint8_t)0x00)
#define TIM1_EGR_BG_ENABLE      ((uint8_t)0x80)

#define TIM1_EGR_TG_MASK        ((uint8_t)0x40) /*!< Trigger Generation mask. */
#define TIM1_EGR_TG_DISABLE     ((uint8_t)0x00)
#define TIM1_EGR_TG_ENABLE      ((uint8_t)0x40)

#define TIM1_EGR_COMG_MASK      ((uint8_t)0x20) /*!< Capture/Compare Control Update Generation mask. */
#define TIM1_EGR_COMG_DISABLE   ((uint8_t)0x00)
#define TIM1_EGR_COMG_ENABLE    ((uint8_t)0x20)

#define TIM1_EGR_CC4G_MASK      ((uint8_t)0x10) /*!< Capture/Compare 4 Generation mask. */
#define TIM1_EGR_CC4G_DISABLE   ((uint8_t)0x00)
#define TIM1_EGR_CC4G_ENABLE    ((uint8_t)0x10)

#define TIM1_EGR_CC3G_MASK      ((uint8_t)0x08) /*!< Capture/Compare 3 Generation mask. */
#define TIM1_EGR_CC3G_DISABLE   ((uint8_t)0x00)
#define TIM1_EGR_CC3G_ENABLE    ((uint8_t)0x08)

#define TIM1_EGR_CC2G_MASK      ((uint8_t)0x04) /*!< Capture/Compare 2 Generation mask. */
#define TIM1_EGR_CC2G_DISABLE   ((uint8_t)0x00)
#define TIM1_EGR_CC2G_ENABLE    ((uint8_t)0x04)

#define TIM1_EGR_CC1G_MASK      ((uint8_t)0x02) /*!< Capture/Compare 1 Generation mask. */
#define TIM1_EGR_CC1G_DISABLE   ((uint8_t)0x00)
#define TIM1_EGR_CC1G_ENABLE    ((uint8_t)0x02)

#define TIM1_EGR_UG_MASK        ((uint8_t)0x01) /*!< Update Generation mask. */
#define TIM1_EGR_UG_DISABLE     ((uint8_t)0x00)
#define TIM1_EGR_UG_ENABLE      ((uint8_t)0x01)

#define TIM1_CCMR_ICxPSC_MASK   ((uint8_t)0x0C) /*!< Input Capture x Prescaler mask. */
#define TIM1_CCMR_ICxPSC_NONE   ((uint8_t)0x00)
#define TIM1_CCMR_ICxPSC_DIV2   ((uint8_t)0x04)
#define TIM1_CCMR_ICxPSC_DIV4   ((uint8_t)0x08)
#define TIM1_CCMR_ICxPSC_DIV8   ((uint8_t)0x0C)

#define TIM1_CCMR_ICxF_MASK     ((uint8_t)0xF0) /*!< Input Capture x Filter mask. */
#define TIM1_CCMR_ICxF_DISABLE  ((uint8_t)0x00)
#define TIM1_CCMR_ICxF_DIV1_N2  ((uint8_t)0x10)
#define TIM1_CCMR_ICxF_DIV1_N4  ((uint8_t)0x20)
#define TIM1_CCMR_ICxF_DIV1_N8  ((uint8_t)0x30)
#define TIM1_CCMR_ICxF_DIV2_N6  ((uint8_t)0x40)
#define TIM1_CCMR_ICxF_DIV2_N8  ((uint8_t)0x50)
#define TIM1_CCMR_ICxF_DIV4_N6  ((uint8_t)0x60)
#define TIM1_CCMR_ICxF_DIV4_N8  ((uint8_t)0x70)
#define TIM1_CCMR_ICxF_DIV8_N6  ((uint8_t)0x80)
#define TIM1_CCMR_ICxF_DIV8_N8  ((uint8_t)0x90)
#define TIM1_CCMR_ICxF_DIV16_N5 ((uint8_t)0xA0)
#define TIM1_CCMR_ICxF_DIV16_N6 ((uint8_t)0xB0)
#define TIM1_CCMR_ICxF_DIV16_N8 ((uint8_t)0xC0)
#define TIM1_CCMR_ICxF_DIV32_N5 ((uint8_t)0xD0)
#define TIM1_CCMR_ICxF_DIV32_N6 ((uint8_t)0xE0)
#define TIM1_CCMR_ICxF_DIV32_N8 ((uint8_t)0xF0)

#define TIM1_CCMR_OCxCE_MASK    ((uint8_t)0x80) /*!< Output Compare x Clear Enable mask. */
#define TIM1_CCMR_OCxCE_DISABLE ((uint8_t)0x00)
#define TIM1_CCMR_OCxCE_ENABLE  ((uint8_t)0x80)

#define TIM1_CCMR_OCxM_MASK     ((uint8_t)0x70) /*!< Output Compare x Mode mask. */
#define TIM1_CCMR_OCxM_FROZEN   ((uint8_t)0x00)
#define TIM1_CCMR_OCxM_ACTIVE   ((uint8_t)0x10)
#define TIM1_CCMR_OCxM_INACTIVE ((uint8_t)0x20)
#define TIM1_CCMR_OCxM_TOGGLE   ((uint8_t)0x30)
#define TIM1_CCMR_OCxM_FORCELOW ((uint8_t)0x40)
#define TIM1_CCMR_OCxM_FORCEHIGH ((uint8_t)0x50)
#define TIM1_CCMR_OCxM_PWM1     ((uint8_t)0x60)
#define TIM1_CCMR_OCxM_PWM2     ((uint8_t)0x70)

#define TIM1_CCMR_OCxPE_MASK    ((uint8_t)0x08) /*!< Output Compare x Preload Enable mask. */
#define TIM1_CCMR_OCxPE_DISABLE ((uint8_t)0x00)
#define TIM1_CCMR_OCxPE_ENABLE  ((uint8_t)0x08)

#define TIM1_CCMR_OCxFE_MASK    ((uint8_t)0x04) /*!< Output Compare x Fast Enable mask. */
#define TIM1_CCMR_OCxFE_DISABLE ((uint8_t)0x00)
#define TIM1_CCMR_OCxFE_ENABLE  ((uint8_t)0x04)

#define TIM1_CCMR_CCxS_MASK     ((uint8_t)0x03) /*!< Capture/Compare x Selection mask. */
#define TIM1_CCMR_CCxS_OUTPUT   ((uint8_t)0x00)
#define TIM1_CCMR_CCxS_IN_TI1FP2 ((uint8_t)0x01)
#define TIM1_CCMR_CCxS_IN_TI2FP1 ((uint8_t)0x02)
#define TIM1_CCMR_CCxS_IN_TRC   ((uint8_t)0x03)

#define TIM1_CCER1_CC2NP_MASK   ((uint8_t)0x80) /*!< Capture/Compare 2 Complementary output Polarity mask. */
#define TIM1_CCER1_CC2NP_DISABLE ((uint8_t)0x00)
#define TIM1_CCER1_CC2NP_ENABLE ((uint8_t)0x80)

#define TIM1_CCER1_CC2NE_MASK   ((uint8_t)0x40) /*!< Capture/Compare 2 Complementary output enable mask. */
#define TIM1_CCER1_CC2NE_DISABLE ((uint8_t)0x00)
#define TIM1_CCER1_CC2NE_ENABLE ((uint8_t)0x40)

#define TIM1_CCER1_CC2P_MASK    ((uint8_t)0x20) /*!< Capture/Compare 2 output Polarity mask. */
#define TIM1_CCER1_CC2P_DISABLE ((uint8_t)0x00)
#define TIM1_CCER1_CC2P_ENABLE  ((uint8_t)0x20)

#define TIM1_CCER1_CC2E_MASK    ((uint8_t)0x10) /*!< Capture/Compare 2 output enable mask. */
#define TIM1_CCER1_CC2E_DIABLE  ((uint8_t)0x00)
#define TIM1_CCER1_CC2E_ENABLE  ((uint8_t)0x10)

#define TIM1_CCER1_CC1NP_MASK   ((uint8_t)0x08) /*!< Capture/Compare 1 Complementary output Polarity mask. */
#define TIM1_CCER1_CC1NP_DISABLE ((uint8_t)0x00)
#define TIM1_CCER1_CC1NP_ENABLE ((uint8_t)0x08)

#define TIM1_CCER1_CC1NE_MASK   ((uint8_t)0x04) /*!< Capture/Compare 1 Complementary output enable mask. */
#define TIM1_CCER1_CC1NE_DISABLE ((uint8_t)0x00)
#define TIM1_CCER1_CC1NE_ENABLE ((uint8_t)0x04)

#define TIM1_CCER1_CC1P_MASK    ((uint8_t)0x02) /*!< Capture/Compare 1 output Polarity mask. */
#define TIM1_CCER1_CC1P_DISABLE ((uint8_t)0x00)
#define TIM1_CCER1_CC1P_ENABLE  ((uint8_t)0x02)

#define TIM1_CCER1_CC1E_MASK    ((uint8_t)0x01) /*!< Capture/Compare 1 output enable mask. */
#define TIM1_CCER1_CC1E_DISABLE ((uint8_t)0x00)
#define TIM1_CCER1_CC1E_ENABLE  ((uint8_t)0x01)

#define TIM1_CCER2_CC4P_MASK    ((uint8_t)0x20) /*!< Capture/Compare 4 output Polarity mask. */
#define TIM1_CCER2_CC4P_DISABLE  ((uint8_t)0x00)
#define TIM1_CCER2_CC4P_ENABLE  ((uint8_t)0x20)

#define TIM1_CCER2_CC4E_MASK    ((uint8_t)0x10) /*!< Capture/Compare 4 output enable mask. */
#define TIM1_CCER2_CC4E_DISABLE ((uint8_t)0x00)
#define TIM1_CCER2_CC4E_ENABLE  ((uint8_t)0x10)

#define TIM1_CCER2_CC3NP_MASK   ((uint8_t)0x08) /*!< Capture/Compare 3 Complementary output Polarity mask. */
#define TIM1_CCER2_CC3NP_DISABLE ((uint8_t)0x00)
#define TIM1_CCER2_CC3NP_ENABLE ((uint8_t)0x08)

#define TIM1_CCER2_CC3NE_MASK   ((uint8_t)0x04) /*!< Capture/Compare 3 Complementary output enable mask. */
#define TIM1_CCER2_CC3NE_DISABLE ((uint8_t)0x00)
#define TIM1_CCER2_CC3NE_ENABLE ((uint8_t)0x04)

#define TIM1_CCER2_CC3P_MASK    ((uint8_t)0x02) /*!< Capture/Compare 3 output Polarity mask. */
#define TIM1_CCER2_CC3P_DISABLE ((uint8_t)0x00)
#define TIM1_CCER2_CC3P_ENABLE  ((uint8_t)0x02)

#define TIM1_CCER2_CC3E_MASK    ((uint8_t)0x01) /*!< Capture/Compare 3 output enable mask. */
#define TIM1_CCER2_CC3E_DISABLE ((uint8_t)0x00)
#define TIM1_CCER2_CC3E_ENABLE  ((uint8_t)0x01)

#define TIM1_CNTRH_CNT_MASK     ((uint8_t)0xFF) /*!< Counter Value (MSB) mask. */
#define TIM1_CNTRL_CNT_MASK     ((uint8_t)0xFF) /*!< Counter Value (LSB) mask. */

#define TIM1_PSCRH_PSC_MASK     ((uint8_t)0xFF) /*!< Prescaler Value (MSB) mask. */
#define TIM1_PSCRL_PSC_MASK     ((uint8_t)0xFF) /*!< Prescaler Value (LSB) mask. */

#define TIM1_ARRH_ARR_MASK      ((uint8_t)0xFF) /*!< Autoreload Value (MSB) mask. */
#define TIM1_ARRL_ARR_MASK      ((uint8_t)0xFF) /*!< Autoreload Value (LSB) mask. */

#define TIM1_RCR_REP_MASK       ((uint8_t)0xFF) /*!< Repetition Counter Value mask. */

#define TIM1_CCR1H_CCR1_MASK    ((uint8_t)0xFF) /*!< Capture/Compare 1 Value (MSB) mask. */
#define TIM1_CCR1L_CCR1_MASK    ((uint8_t)0xFF) /*!< Capture/Compare 1 Value (LSB) mask. */

#define TIM1_CCR2H_CCR2_MASK    ((uint8_t)0xFF) /*!< Capture/Compare 2 Value (MSB) mask. */
#define TIM1_CCR2L_CCR2_MASK    ((uint8_t)0xFF) /*!< Capture/Compare 2 Value (LSB) mask. */

#define TIM1_CCR3H_CCR3_MASK    ((uint8_t)0xFF) /*!< Capture/Compare 3 Value (MSB) mask. */
#define TIM1_CCR3L_CCR3_MASK    ((uint8_t)0xFF) /*!< Capture/Compare 3 Value (LSB) mask. */

#define TIM1_CCR4H_CCR4_MASK    ((uint8_t)0xFF) /*!< Capture/Compare 4 Value (MSB) mask. */
#define TIM1_CCR4L_CCR4_MASK    ((uint8_t)0xFF) /*!< Capture/Compare 4 Value (LSB) mask. */

#define TIM1_BKR_MOE_MASK       ((uint8_t)0x80) /*!< Main Output Enable mask. */
#define TIM1_BKR_MOE_DISABLE    ((uint8_t)0x00)
#define TIM1_BKR_MOE_ENABLE     ((uint8_t)0x80)

#define TIM1_BKR_AOE_MASK       ((uint8_t)0x40) /*!< Automatic Output Enable mask. */
#define TIM1_BKR_AOE_DISABLE    ((uint8_t)0x00)
#define TIM1_BKR_AOE_ENABLE     ((uint8_t)0x40)

#define TIM1_BKR_BKP_MASK       ((uint8_t)0x20) /*!< Break Polarity mask. */
#define TIM1_BKR_BKP_LOW        ((uint8_t)0x00)
#define TIM1_BKR_BKP_HIGH       ((uint8_t)0x20)

#define TIM1_BKR_BKE_MASK       ((uint8_t)0x10) /*!< Break Enable mask. */
#define TIM1_BKR_BKE_DISABLE    ((uint8_t)0x00)
#define TIM1_BKR_BKE_ENABLE     ((uint8_t)0x10)

#define TIM1_BKR_OSSR_MASK      ((uint8_t)0x08) /*!< Off-State Selection for Run mode mask. */
#define TIM1_BKR_OSSR_DISABLE   ((uint8_t)0x00)
#define TIM1_BKR_OSSR_ENABLE    ((uint8_t)0x08)

#define TIM1_BKR_OSSI_MASK      ((uint8_t)0x04) /*!< Off-State Selection for Idle mode mask. */
#define TIM1_BKR_OSSI_DISABLE   ((uint8_t)0x00)
#define TIM1_BKR_OSSI_ENABLE    ((uint8_t)0x04)

#define TIM1_BKR_LOCK_MASK      ((uint8_t)0x03) /*!< Lock Configuration mask. */
#define TIM1_BKR_LOCK_OFF       ((uint8_t)0x00)
#define TIM1_BKR_LOCK_LEVEL1    ((uint8_t)0x01)
#define TIM1_BKR_LOCK_LEVEL2    ((uint8_t)0x02)
#define TIM1_BKR_LOCK_LEVEL3    ((uint8_t)0x03)

#define TIM1_DTR_DTG_MASK       ((uint8_t)0xFF) /*!< Dead-Time Generator set-up mask. */

#define TIM1_OISR_OIS4_MASK     ((uint8_t)0x40) /*!< Output Idle state 4 (OC4 output) mask. */
#define TIM1_OISR_OIS4_DISABLE  ((uint8_t)0x00)
#define TIM1_OISR_OIS4_ENABLE   ((uint8_t)0x40)

#define TIM1_OISR_OIS3N_MASK    ((uint8_t)0x20) /*!< Output Idle state 3 (OC3N output) mask. */
#define TIM1_OISR_OIS3N_DISABLE ((uint8_t)0x00)
#define TIM1_OISR_OIS3N_ENABLE  ((uint8_t)0x20)

#define TIM1_OISR_OIS3_MASK     ((uint8_t)0x10) /*!< Output Idle state 3 (OC3 output) mask. */
#define TIM1_OISR_OIS3_DISABLE  ((uint8_t)0x00)
#define TIM1_OISR_OIS3_ENABLE   ((uint8_t)0x10)

#define TIM1_OISR_OIS2N_MASK    ((uint8_t)0x08) /*!< Output Idle state 2 (OC2N output) mask. */
#define TIM1_OISR_OIS2N_DISABLE ((uint8_t)0x00)
#define TIM1_OISR_OIS2N_ENABLE  ((uint8_t)0x08)

#define TIM1_OISR_OIS2_MASK     ((uint8_t)0x04) /*!< Output Idle state 2 (OC2 output) mask. */
#define TIM1_OISR_OIS2_DISABLE  ((uint8_t)0x00)
#define TIM1_OISR_OIS2_ENABLE   ((uint8_t)0x04)

#define TIM1_OISR_OIS1N_MASK    ((uint8_t)0x02) /*!< Output Idle state 1 (OC1N output) mask. */
#define TIM1_OISR_OIS1N_DISABLE ((uint8_t)0x00)
#define TIM1_OISR_OIS1N_ENABLE  ((uint8_t)0x02)

#define TIM1_OISR_OIS1_MASK     ((uint8_t)0x01) /*!< Output Idle state 1 (OC1 output) mask. */
#define TIM1_OISR_OIS1_DISABLE  ((uint8_t)0x00)
#define TIM1_OISR_OIS1_ENABLE   ((uint8_t)0x01)

#if 0
typedef enum
{
  TIM1_OCMODE_TIMING     = ((uint8_t)0x00),
  TIM1_OCMODE_ACTIVE     = ((uint8_t)0x10),
  TIM1_OCMODE_INACTIVE   = ((uint8_t)0x20),
  TIM1_OCMODE_TOGGLE     = ((uint8_t)0x30),
  TIM1_OCMODE_PWM1       = ((uint8_t)0x60),
  TIM1_OCMODE_PWM2       = ((uint8_t)0x70)
}TIM1_OCMode_TypeDef;
typedef enum
{
  TIM1_OPMODE_SINGLE                 = ((uint8_t)0x01),
  TIM1_OPMODE_REPETITIVE             = ((uint8_t)0x00)
}TIM1_OPMode_TypeDef;
typedef enum
{
  TIM1_CHANNEL_1                     = ((uint8_t)0x00),
  TIM1_CHANNEL_2                     = ((uint8_t)0x01),
  TIM1_CHANNEL_3                     = ((uint8_t)0x02),
  TIM1_CHANNEL_4                     = ((uint8_t)0x03)
}TIM1_Channel_TypeDef;
typedef enum
{
  TIM1_COUNTERMODE_UP                = ((uint8_t)0x00),
  TIM1_COUNTERMODE_DOWN              = ((uint8_t)0x10),
  TIM1_COUNTERMODE_CENTERALIGNED1    = ((uint8_t)0x20),
  TIM1_COUNTERMODE_CENTERALIGNED2    = ((uint8_t)0x40),
  TIM1_COUNTERMODE_CENTERALIGNED3    = ((uint8_t)0x60)
}TIM1_CounterMode_TypeDef;
typedef enum
{
  TIM1_OCPOLARITY_HIGH               = ((uint8_t)0x00),
  TIM1_OCPOLARITY_LOW                = ((uint8_t)0x22)
}TIM1_OCPolarity_TypeDef;
typedef enum
{
  TIM1_OCNPOLARITY_HIGH              = ((uint8_t)0x00),
  TIM1_OCNPOLARITY_LOW               = ((uint8_t)0x88)
}TIM1_OCNPolarity_TypeDef;
typedef enum
{
  TIM1_OUTPUTSTATE_DISABLE           = ((uint8_t)0x00),
  TIM1_OUTPUTSTATE_ENABLE            = ((uint8_t)0x11)
}TIM1_OutputState_TypeDef;
typedef enum
{
  TIM1_OUTPUTNSTATE_DISABLE = ((uint8_t)0x00),
  TIM1_OUTPUTNSTATE_ENABLE  = ((uint8_t)0x44)
} TIM1_OutputNState_TypeDef;
typedef enum
{
  TIM1_BREAK_ENABLE                  = ((uint8_t)0x10),
  TIM1_BREAK_DISABLE                 = ((uint8_t)0x00)
}TIM1_BreakState_TypeDef;
typedef enum
{
  TIM1_BREAKPOLARITY_LOW             = ((uint8_t)0x00),
  TIM1_BREAKPOLARITY_HIGH            = ((uint8_t)0x20)
}TIM1_BreakPolarity_TypeDef;
typedef enum
{
  TIM1_AUTOMATICOUTPUT_ENABLE        = ((uint8_t)0x40),
  TIM1_AUTOMATICOUTPUT_DISABLE       = ((uint8_t)0x00)
}TIM1_AutomaticOutput_TypeDef;
typedef enum
{
  TIM1_LOCKLEVEL_OFF                 = ((uint8_t)0x00),
  TIM1_LOCKLEVEL_1                   = ((uint8_t)0x01),
  TIM1_LOCKLEVEL_2                   = ((uint8_t)0x02),
  TIM1_LOCKLEVEL_3                   = ((uint8_t)0x03)
}TIM1_LockLevel_TypeDef;
typedef enum
{
  TIM1_OSSISTATE_ENABLE              = ((uint8_t)0x04),
  TIM1_OSSISTATE_DISABLE             = ((uint8_t)0x00)
}TIM1_OSSIState_TypeDef;
typedef enum
{
  TIM1_OCIDLESTATE_SET               = ((uint8_t)0x55),
  TIM1_OCIDLESTATE_RESET             = ((uint8_t)0x00)
}TIM1_OCIdleState_TypeDef;
typedef enum
{
  TIM1_OCNIDLESTATE_SET             = ((uint8_t)0x2A),
  TIM1_OCNIDLESTATE_RESET           = ((uint8_t)0x00)
}TIM1_OCNIdleState_TypeDef;
typedef enum
{
  TIM1_ICPOLARITY_RISING            = ((uint8_t)0x00),
  TIM1_ICPOLARITY_FALLING           = ((uint8_t)0x01)
}TIM1_ICPolarity_TypeDef;
typedef enum
{
  TIM1_ICSELECTION_DIRECTTI          = ((uint8_t)0x01),
  TIM1_ICSELECTION_INDIRECTTI        = ((uint8_t)0x02),
  TIM1_ICSELECTION_TRGI              = ((uint8_t)0x03)
}TIM1_ICSelection_TypeDef;
typedef enum
{
  TIM1_ICPSC_DIV1                    = ((uint8_t)0x00),
  TIM1_ICPSC_DIV2                    = ((uint8_t)0x04),
  TIM1_ICPSC_DIV4                    = ((uint8_t)0x08),
  TIM1_ICPSC_DIV8                    = ((uint8_t)0x0C)
}TIM1_ICPSC_TypeDef;
typedef enum
{
  TIM1_IT_UPDATE                     = ((uint8_t)0x01),
  TIM1_IT_CC1                        = ((uint8_t)0x02),
  TIM1_IT_CC2                        = ((uint8_t)0x04),
  TIM1_IT_CC3                        = ((uint8_t)0x08),
  TIM1_IT_CC4                        = ((uint8_t)0x10),
  TIM1_IT_COM                        = ((uint8_t)0x20),
  TIM1_IT_TRIGGER                    = ((uint8_t)0x40),
  TIM1_IT_BREAK                      = ((uint8_t)0x80)
}TIM1_IT_TypeDef;
typedef enum
{
  TIM1_EXTTRGPSC_OFF                 = ((uint8_t)0x00),
  TIM1_EXTTRGPSC_DIV2                = ((uint8_t)0x10),
  TIM1_EXTTRGPSC_DIV4                = ((uint8_t)0x20),
  TIM1_EXTTRGPSC_DIV8                = ((uint8_t)0x30)
}TIM1_ExtTRGPSC_TypeDef;
typedef enum
{
  TIM1_TS_TIM6                       = ((uint8_t)0x00),  /*!< TRIG Input source =  TIM6 TRIG Output  */
  TIM1_TS_TIM5                       = ((uint8_t)0x30),  /*!< TRIG Input source =  TIM5 TRIG Output  */
  TIM1_TS_TI1F_ED                    = ((uint8_t)0x40),
  TIM1_TS_TI1FP1                     = ((uint8_t)0x50),
  TIM1_TS_TI2FP2                     = ((uint8_t)0x60),
  TIM1_TS_ETRF                       = ((uint8_t)0x70)
}TIM1_TS_TypeDef;
typedef enum
{
  TIM1_TIXEXTERNALCLK1SOURCE_TI1ED   = ((uint8_t)0x40),
  TIM1_TIXEXTERNALCLK1SOURCE_TI1     = ((uint8_t)0x50),
  TIM1_TIXEXTERNALCLK1SOURCE_TI2     = ((uint8_t)0x60)
}TIM1_TIxExternalCLK1Source_TypeDef;
typedef enum
{
  TIM1_EXTTRGPOLARITY_INVERTED       = ((uint8_t)0x80),
  TIM1_EXTTRGPOLARITY_NONINVERTED    = ((uint8_t)0x00)
}TIM1_ExtTRGPolarity_TypeDef;
typedef enum
{
  TIM1_PSCRELOADMODE_UPDATE          = ((uint8_t)0x00),
  TIM1_PSCRELOADMODE_IMMEDIATE       = ((uint8_t)0x01)
}TIM1_PSCReloadMode_TypeDef;
typedef enum
{
  TIM1_ENCODERMODE_TI1               = ((uint8_t)0x01),
  TIM1_ENCODERMODE_TI2               = ((uint8_t)0x02),
  TIM1_ENCODERMODE_TI12              = ((uint8_t)0x03)
}TIM1_EncoderMode_TypeDef;
typedef enum
{
  TIM1_EVENTSOURCE_UPDATE            = ((uint8_t)0x01),
  TIM1_EVENTSOURCE_CC1               = ((uint8_t)0x02),
  TIM1_EVENTSOURCE_CC2               = ((uint8_t)0x04),
  TIM1_EVENTSOURCE_CC3               = ((uint8_t)0x08),
  TIM1_EVENTSOURCE_CC4               = ((uint8_t)0x10),
  TIM1_EVENTSOURCE_COM               = ((uint8_t)0x20),
  TIM1_EVENTSOURCE_TRIGGER           = ((uint8_t)0x40),
  TIM1_EVENTSOURCE_BREAK             = ((uint8_t)0x80)
}TIM1_EventSource_TypeDef;
typedef enum
{
  TIM1_UPDATESOURCE_GLOBAL           = ((uint8_t)0x00),
  TIM1_UPDATESOURCE_REGULAR          = ((uint8_t)0x01)
}TIM1_UpdateSource_TypeDef;
typedef enum
{
  TIM1_TRGOSOURCE_RESET              = ((uint8_t)0x00),
  TIM1_TRGOSOURCE_ENABLE             = ((uint8_t)0x10),
  TIM1_TRGOSOURCE_UPDATE             = ((uint8_t)0x20),
  TIM1_TRGOSource_OC1                = ((uint8_t)0x30),
  TIM1_TRGOSOURCE_OC1REF             = ((uint8_t)0x40),
  TIM1_TRGOSOURCE_OC2REF             = ((uint8_t)0x50),
  TIM1_TRGOSOURCE_OC3REF             = ((uint8_t)0x60)
}TIM1_TRGOSource_TypeDef;
typedef enum
{
  TIM1_SLAVEMODE_RESET               = ((uint8_t)0x04),
  TIM1_SLAVEMODE_GATED               = ((uint8_t)0x05),
  TIM1_SLAVEMODE_TRIGGER             = ((uint8_t)0x06),
  TIM1_SLAVEMODE_EXTERNAL1           = ((uint8_t)0x07)
}TIM1_SlaveMode_TypeDef;
typedef enum
{
  TIM1_FLAG_UPDATE                   = ((uint16_t)0x0001),
  TIM1_FLAG_CC1                      = ((uint16_t)0x0002),
  TIM1_FLAG_CC2                      = ((uint16_t)0x0004),
  TIM1_FLAG_CC3                      = ((uint16_t)0x0008),
  TIM1_FLAG_CC4                      = ((uint16_t)0x0010),
  TIM1_FLAG_COM                      = ((uint16_t)0x0020),
  TIM1_FLAG_TRIGGER                  = ((uint16_t)0x0040),
  TIM1_FLAG_BREAK                    = ((uint16_t)0x0080),
  TIM1_FLAG_CC1OF                    = ((uint16_t)0x0200),
  TIM1_FLAG_CC2OF                    = ((uint16_t)0x0400),
  TIM1_FLAG_CC3OF                    = ((uint16_t)0x0800),
  TIM1_FLAG_CC4OF                    = ((uint16_t)0x1000)
}TIM1_FLAG_TypeDef;
typedef enum
{
  TIM1_FORCEDACTION_ACTIVE           = ((uint8_t)0x50),
  TIM1_FORCEDACTION_INACTIVE         = ((uint8_t)0x40)
}TIM1_ForcedAction_TypeDef;
#endif

#define TIM4_CR1_ARPE_MASK  ((uint8_t)0x80) /*!< Auto-Reload Preload Enable mask. */
#define TIM4_CR1_OPM_MASK   ((uint8_t)0x08) /*!< One Pulse Mode mask. */
#define TIM4_CR1_URS_MASK   ((uint8_t)0x04) /*!< Update Request Source mask. */
#define TIM4_CR1_UDIS_MASK  ((uint8_t)0x02) /*!< Update DIsable mask. */
#define TIM4_CR1_CEN_MASK   ((uint8_t)0x01) /*!< Counter Enable mask. */
#define TIM4_IER_UIE_MASK   ((uint8_t)0x01) /*!< Update Interrupt Enable mask. */
#define TIM4_SR1_UIF_MASK   ((uint8_t)0x01) /*!< Update Interrupt Flag mask. */
#define TIM4_EGR_UG_MASK    ((uint8_t)0x01) /*!< Update Generation mask. */
#define TIM4_CNTR_CNT_MASK  ((uint8_t)0xFF) /*!< Counter Value (LSB) mask. */
#define TIM4_PSCR_PSC_MASK  ((uint8_t)0x07) /*!< Prescaler Value  mask. */
#define TIM4_ARR_ARR_MASK   ((uint8_t)0xFF) /*!< Autoreload Value mask. */

#define TIM4_PSCR_1     ((uint8_t)0x00)
#define TIM4_PSCR_2     ((uint8_t)0x01)
#define TIM4_PSCR_4     ((uint8_t)0x02)
#define TIM4_PSCR_8     ((uint8_t)0x03)
#define TIM4_PSCR_16    ((uint8_t)0x04)
#define TIM4_PSCR_32    ((uint8_t)0x05)
#define TIM4_PSCR_64    ((uint8_t)0x06)
#define TIM4_PSCR_128   ((uint8_t)0x07)

#define TIM4_SR1_UIF
typedef enum
{
  TIM4_FLAG_UPDATE                   = ((uint8_t)0x01)
}TIM4_FLAG_TypeDef;
typedef enum
{
  TIM4_IT_UPDATE                     = ((uint8_t)0x01)
}TIM4_IT_TypeDef;


//=============================================================================
// System clock functions
//
// Only one of the following function should be used. The other can either be
// conditionally compiled out or the compiler might optimise them out if they
// aren't used.
//

//-----------------------------------------------------------------------------
// Set the system clock to the Low Speed Internal clock.
//
// The system will run at 128Khz
//
// TODO: Check if the LSI is enabled in the option byte as its required.
//
void SysClock_LSI(void)
{
    // Enable automatic clock switching
    CLK->SWCR = CLK_SWCR_SWEN_AUTOMATIC;

    // Start the low speed internal clock
    CLK->ICKR = CLK_ICKR_LSIEN_ENABLE;

    // Wait for the LSI to be ready
    while ((CLK->ICKR & CLK_ICKR_LSIRDY_MASK) == CLK_ICKR_LSIRDY_NOTREADY)
    {
    }

    // Switch to the LSI
    CLK->SWR = CLK_SWR_SWI_LSI;

    // Wait for the switch to the LSI to be completed
    while (CLK->CMSR != CLK_CMSR_CKM_LSI)
    {
    }
}

//-----------------------------------------------------------------------------
// Set the system clock to the High Speed Internal clock.
//
// The system will run at 16Mhz
//
void SysClock_HSI(void)
{
    // Set the clock prescaler for the HSI
    CLK->CKDIVR = CLK_CKDIVR_CPUDIV1 | CLK_CKDIVR_HSIDIV1;

    // Enable automatic clock switching
    CLK->SWCR = CLK_SWCR_SWEN_AUTOMATIC;

    // Start the high speed internal clock
    CLK->ICKR = CLK_ICKR_HSIEN_ENABLE;

    // Wait for the HSI to be ready
    while ((CLK->ICKR & CLK_ICKR_HSIRDY_MASK) == CLK_ICKR_HSIRDY_NOTREADY)
    {
    }

    // Switch to the HSI
    CLK->SWR = CLK_SWR_SWI_HSI;

    // Wait for the switch to the HSI to be completed
    while (CLK->CMSR != CLK_CMSR_CKM_HSI)
    {
    }
}

//-----------------------------------------------------------------------------
// Set the system clock to the High Speed Internal clock.
//
// The system will run at 1-24Mhz
//
void SysClock_HSE(void)
{
    // Enable automatic clock switching
    CLK->SWCR = CLK_SWCR_SWEN_AUTOMATIC;

    // Start the high speed external clock
    CLK->ECKR = CLK_ECKR_HSEEN_ENABLE;

    // Wait for the HSE to be ready
    while ((CLK->ECKR & CLK_ECKR_HSERDY_MASK) == CLK_ECKR_HSERDY_NOTREADY)
    {
    }

    // Switch to the HSE
    CLK->SWR = CLK_SWR_SWI_HSE;

    // Wait for the switch to the HSE to be completed
    while (CLK->CMSR != CLK_CMSR_CKM_HSE)
    {
    }
}

//=============================================================================
// Timer functions
//
// Initialise timer 1 with a frequency of 50Khz
// (16000000 / 1) / (319 + 1) = 0.00002s = 50Khz
//

#define TIM1_PERIOD         320
#define TIM1_CH3_DUTY       ((TIM1_PERIOD * 0) / 100)

//-----------------------------------------------------------------------------
// Set the auto-reload value
void Tim1_SetAutoReload(uint16_t period)
{
    TIM1->ARRH = ((period - 1) >> 8) & 0xFF;
    TIM1->ARRL = ((period - 1) >> 0) & 0xFF;
}

//-----------------------------------------------------------------------------
// Set the prescaler value
void Tim1_SetPrescaler(uint16_t division)
{
    TIM1->PSCRH = ((division - 1) >> 8) & 0xFF;
    TIM1->PSCRL = ((division - 1) >> 0) & 0xFF;
}

//-----------------------------------------------------------------------------
// Initialise timer 1
//
void Tim1_Config(void)
{
    // Disable to setup
    TIM1->CR1 = (TIM1->CR1 & ~TIM1_CR1_CEN_MASK) | TIM1_CR1_CEN_DISABLE;

    // Set the auto-reload value
    Tim1_SetAutoReload(TIM1_PERIOD);

    // Set the prescaler to division by 1
    Tim1_SetPrescaler(1);

    // Set the counter mode to up
    TIM1->CR1 = (TIM1->CR1 & ~(TIM1_CR1_CMS_MASK | TIM1_CR1_DIR_MASK)) |
                TIM1_CR1_CMS_EDGE | TIM1_CR1_DIR_UP;

    // Set the repetition counter to zero
    TIM1->RCR = 0;

    TIM1->CCER2 = (TIM1->CCER2 & ~(TIM1_CCER2_CC3E_MASK | TIM1_CCER2_CC3NE_MASK | TIM1_CCER2_CC3P_MASK | TIM1_CCER2_CC3NP_MASK)) |
                    (TIM1_CCER2_CC3E_ENABLE) |
                    (TIM1_CCER2_CC3P_ENABLE) |
                    (TIM1_CCER2_CC3NE_ENABLE) |
                    (TIM1_CCER2_CC3NP_ENABLE);
    TIM1->CCMR3 = (TIM1->CCMR3 & ~(TIM1_CCMR_OCxM_MASK | TIM1_CCMR_OCxPE_MASK)) | TIM1_CCMR_OCxM_PWM2 | TIM1_CCMR_OCxPE_ENABLE;
    TIM1->OISR &= ~(TIM1_OISR_OIS3_MASK | TIM1_OISR_OIS3N_MASK);
    TIM1->OISR |= (TIM1_OISR_OIS3_ENABLE) | (TIM1_OISR_OIS3N_ENABLE);
    TIM1->CCR3H = (TIM1_CH3_DUTY >> 8) & 0xFF;
    TIM1->CCR3L = (TIM1_CH3_DUTY >> 0) & 0xFF;
    TIM1->BKR |= TIM1_BKR_MOE_ENABLE;

    TIM1->CR1 = (TIM1->CR1 & ~TIM1_CR1_CEN_MASK) | TIM1_CR1_CEN_ENABLE;
}

// Setup 8-bit basic timer as 1ms system tick using interrupts to update the tick counter
// With 16Mhz HSI prescaler is 128 and reload value is 124. freq = 16000000/128 is 125Khz, therefore reload value is 125 - 1 or 124. Period is 125/125000 which is 0.001s or 1ms
// With 8Mhz HSE prescaler is 64 and reload value is 124. freq is 8000000/64 is 125Khz.
// With 128Khz LSI prescaler is 16 and reload value is 128. freq is 128000/16 is 8Khz.
void Tim4_Config(uint8_t prescaler, uint8_t reload)
{
    TIM4->CR1 &= ~(unsigned int)TIM4_CR1_CEN_MASK;
    TIM4->PSCR = prescaler;
    TIM4->ARR = reload;
    TIM4->SR1 = TIM4_FLAG_UPDATE;
    TIM4->IER |= TIM4_IT_UPDATE;
    TIM4->CNTR = 0;
    TIM4->CR1 |= TIM4_CR1_ARPE_MASK | TIM4_CR1_CEN_MASK;
}

//=============================================================================
// System Tick functions
//
// The system tick is a 1ms timer. It uses the basic 8-bit timer TIM4. Counter
// is 16 bit so maximum period of time that can be handled is 65s.
//

__IO uint16_t systick;

//-----------------------------------------------------------------------------
// Initialise
//
void Systick_Init(void)
{
    //Tim4_Config(TIM4_PSCR_1, 128);
    Tim4_Config(TIM4_PSCR_64, 124);
    //Tim4_Config(TIM4_PSCR_128, 124);
}

//-----------------------------------------------------------------------------
// Check if a period has passed since the start time.
//
// If the timeout has expired, the start value is reset to the current time
// so that it can be reused.
//
bool Systick_Timeout(uint16_t *start, uint16_t period)
{
    if (systick - *start >= period)
    {
        *start = systick;
        return true;
    }
    return false;
}

//-----------------------------------------------------------------------------
// Wait a period of time
//
void Systick_Wait(uint16_t period)
{
    uint16_t start = systick;
    while (!Systick_Timeout(&start, period))
    {
    }
}

//-----------------------------------------------------------------------------
// Interrupt handler for the system tick counter
//
void TIM4_UPD_OVF_IRQHandler(void) __interrupt(23)
{
    ++systick;

    // Clear Interrupt Pending bit
    TIM4->SR1 = (uint8_t)(~(unsigned int)TIM4_IT_UPDATE);
}

//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
// Main super loop
//
void main(void)
{
    uint16_t value = 1;

    SysClock_HSE();
    Systick_Init();
    Tim1_Config();

    enableInterrupts;

    GPIOD->DDR |= GPIO_DDR_7_OUTPUT;
    GPIOD->CR1 |= GPIO_CR1_7_OUTPUT_PUSHPULL;
    GPIOD->CR2 |= GPIO_CR2_7_OUTPUT_2MHZ;

    for (;;)
    {
        Systick_Wait(100);
        GPIOD->ODR = (GPIOD->ODR & ~GPIO_ODR_7_MASK) | GPIO_ODR_7_HIGH;

        Systick_Wait(100);
        GPIOD->ODR = (GPIOD->ODR & ~GPIO_ODR_7_MASK) | GPIO_ODR_7_LOW;

        TIM1->CCR3H = (value >> 8) & 0xFF;
        TIM1->CCR3L = (value >> 0) & 0xFF;

        value += 10;
        if (value > TIM1_PERIOD)
        {
            value = 0;
        }
    }
}