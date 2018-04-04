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

//#define FLASHER
//#define FADER
#define SERIALIZER

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
// Circular buffer functions
//
typedef struct
{
    __IO uint8_t in;
    __IO uint8_t out;
    uint8_t size;
    uint8_t *buffer;
} circular_buffer_t;

//=============================================================================
// Initialise circular buffer
//
// Size must be a power of 2, eg. 2, 4, 8, 16, 32, 64, 128, 256.
//
void CircBuf_Init(circular_buffer_t *buf, uint8_t *buffer, uint8_t size)
{
    buf->in = 0;
    buf->out = 0;
    buf->size = size - 1;
    buf->buffer = buffer;
}

//=============================================================================
// Put byte in circular buffer
//
// Does not check if buffer is full, use CircBuf_IsFull if this is important.
//
void CircBuf_Put(circular_buffer_t *buf, uint8_t byte)
{
    buf->buffer[buf->in] = byte;
    buf->in = (buf->in + 1) & buf->size;
}

//=============================================================================
// Get byte from circular buffer
//
// Does not check if buffer is empty, use CircBuf_IsEmpty if this is important.
//
uint8_t CircBuf_Get(circular_buffer_t *buf)
{
    uint8_t byte;
    byte = buf->buffer[buf->out];
    buf->out = (buf->out + 1) & buf->size;
    return byte;
}

//=============================================================================
// Indicates if buffer is empty
//
bool CircBuf_IsEmpty(circular_buffer_t *buf)
{
    return buf->in == buf->out;
}

//=============================================================================
// Indicates if buffer is full
//
bool CircBuf_IsFull(circular_buffer_t *buf)
{
    return ((buf->in + 1) & buf->size) == buf->out;
}

//=============================================================================
// Returns how much of buffer is used
//
uint8_t CircBuf_Used(circular_buffer_t *buf)
{
    return (buf->in - buf->out) & buf->size;
}

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

#define CLK_ICKR_SWUAH_MASK     ((uint8_t)0x20) /* Slow Wake-up from Active Halt/Halt modes */
#define CLK_ICKR_SWUAH_DISABLE  ((uint8_t)0x00)
#define CLK_ICKR_SWUAH_ENABLE   ((uint8_t)0x20)

#define CLK_ICKR_LSIRDY_MASK    ((uint8_t)0x10) /* Low speed internal oscillator ready */
#define CLK_ICKR_LSIRDY_NOTREADY ((uint8_t)0x00)
#define CLK_ICKR_LSIRDY_READY   ((uint8_t)0x10)

#define CLK_ICKR_LSIEN_MASK     ((uint8_t)0x08) /* Low speed internal RC oscillator enable */
#define CLK_ICKR_LSIEN_DISABLE  ((uint8_t)0x00)
#define CLK_ICKR_LSIEN_ENABLE   ((uint8_t)0x08)

#define CLK_ICKR_FHWU_MASK      ((uint8_t)0x04) /* Fast Wake-up from Active Halt/Halt mode */
#define CLK_ICKR_FHWU_DISABLE   ((uint8_t)0x00)
#define CLK_ICKR_FHWU_ENABLE    ((uint8_t)0x04)

#define CLK_ICKR_HSIRDY_MASK    ((uint8_t)0x02) /* High speed internal RC oscillator ready */
#define CLK_ICKR_HSIRDY_NOTREADY ((uint8_t)0x00)
#define CLK_ICKR_HSIRDY_READY   ((uint8_t)0x02)

#define CLK_ICKR_HSIEN_MASK     ((uint8_t)0x01) /* High speed internal RC oscillator enable */
#define CLK_ICKR_HSIEN_DISABLE  ((uint8_t)0x00)
#define CLK_ICKR_HSIEN_ENABLE   ((uint8_t)0x01)

#define CLK_ECKR_HSERDY_MASK    ((uint8_t)0x02) /* High speed external crystal oscillator ready */
#define CLK_ECKR_HSERDY_NOTREADY ((uint8_t)0x00)
#define CLK_ECKR_HSERDY_READY   ((uint8_t)0x02)

#define CLK_ECKR_HSEEN_MASK     ((uint8_t)0x01) /* High speed external crystal oscillator enable */
#define CLK_ECKR_HSEEN_DISABLE  ((uint8_t)0x01)
#define CLK_ECKR_HSEEN_ENABLE   ((uint8_t)0x01)

#define CLK_CMSR_CKM_MASK       ((uint8_t)0xFF) /* Clock master status bits */
#define CLK_CMSR_CKM_HSI        ((uint8_t)0xE1) /* Clock Source HSI. */
#define CLK_CMSR_CKM_LSI        ((uint8_t)0xD2) /* Clock Source LSI. */
#define CLK_CMSR_CKM_HSE        ((uint8_t)0xB4) /* Clock Source HSE. */

#define CLK_SWR_SWI_MASK        ((uint8_t)0xFF) /* Clock master selection bits */
#define CLK_SWR_SWI_HSI         ((uint8_t)0xE1) /* Clock Source HSI. */
#define CLK_SWR_SWI_LSI         ((uint8_t)0xD2) /* Clock Source LSI. */
#define CLK_SWR_SWI_HSE         ((uint8_t)0xB4) /* Clock Source HSE. */

#define CLK_SWCR_SWIF_MASK      ((uint8_t)0x08) /* Clock switch interrupt flag */
#define CLK_SWCR_SWIF_NOTREADY  ((uint8_t)0x00) // manual
#define CLK_SWCR_SWIF_READY     ((uint8_t)0x08)
#define CLK_SWCR_SWIF_NOOCCURANCE ((uint8_t)0x00) // auto
#define CLK_SWCR_SWIF_OCCURED   ((uint8_t)0x08)

#define CLK_SWCR_SWIEN_MASK     ((uint8_t)0x04) /* Clock switch interrupt enable */
#define CLK_SWCR_SWIEN_DISABLE  ((uint8_t)0x00)
#define CLK_SWCR_SWIEN_ENABLE   ((uint8_t)0x04)

#define CLK_SWCR_SWEN_MASK      ((uint8_t)0x02) /* Switch start/stop */
#define CLK_SWCR_SWEN_MANUAL    ((uint8_t)0x00)
#define CLK_SWCR_SWEN_AUTOMATIC ((uint8_t)0x02)

#define CLK_SWCR_SWBSY_MASK     ((uint8_t)0x01) /* Switch busy flag*/
#define CLK_SWCR_SWBSY_NOTBUSY  ((uint8_t)0x00)
#define CLK_SWCR_SWBSY_BUSY     ((uint8_t)0x01)

#define CLK_CKDIVR_HSIDIV_MASK  ((uint8_t)0x18) /* High speed internal clock prescaler */
#define CLK_CKDIVR_HSIDIV1      ((uint8_t)0x00) /* High speed internal clock prescaler: 1 */
#define CLK_CKDIVR_HSIDIV2      ((uint8_t)0x08) /* High speed internal clock prescaler: 2 */
#define CLK_CKDIVR_HSIDIV4      ((uint8_t)0x10) /* High speed internal clock prescaler: 4 */
#define CLK_CKDIVR_HSIDIV8      ((uint8_t)0x18) /* High speed internal clock prescaler: 8 */

#define CLK_CKDIVR_CPUDIV_MASK  ((uint8_t)0x07) /* CPU clock prescaler */
#define CLK_CKDIVR_CPUDIV1      ((uint8_t)0x00) /* CPU clock division factors 1 */
#define CLK_CKDIVR_CPUDIV2      ((uint8_t)0x01) /* CPU clock division factors 2 */
#define CLK_CKDIVR_CPUDIV4      ((uint8_t)0x02) /* CPU clock division factors 4 */
#define CLK_CKDIVR_CPUDIV8      ((uint8_t)0x03) /* CPU clock division factors 8 */
#define CLK_CKDIVR_CPUDIV16     ((uint8_t)0x04) /* CPU clock division factors 16 */
#define CLK_CKDIVR_CPUDIV32     ((uint8_t)0x05) /* CPU clock division factors 32 */
#define CLK_CKDIVR_CPUDIV64     ((uint8_t)0x06) /* CPU clock division factors 64 */
#define CLK_CKDIVR_CPUDIV128    ((uint8_t)0x07) /* CPU clock division factors 128 */

#define CLK_PCKENR1_MASK            ((uint8_t)0xFF)
#define CLK_PCKENR1_TIM1            ((uint8_t)0x80) /* Timer 1 clock enable */
#define CLK_PCKENR1_TIM3            ((uint8_t)0x40) /* Timer 3 clock enable */
#define CLK_PCKENR1_TIM2            ((uint8_t)0x20) /* Timer 2 clock enable */
#define CLK_PCKENR1_TIM5            ((uint8_t)0x20) /* Timer 5 clock enable */
#define CLK_PCKENR1_TIM4            ((uint8_t)0x10) /* Timer 4 clock enable */
#define CLK_PCKENR1_TIM6            ((uint8_t)0x10) /* Timer 6 clock enable */
#define CLK_PCKENR1_UART3           ((uint8_t)0x08) /* UART3 clock enable */
#define CLK_PCKENR1_UART2           ((uint8_t)0x08) /* UART2 clock enable */
#define CLK_PCKENR1_UART1           ((uint8_t)0x04) /* UART1 clock enable */
#define CLK_PCKENR1_SPI             ((uint8_t)0x02) /* SPI clock enable */
#define CLK_PCKENR1_I2C             ((uint8_t)0x01) /* I2C clock enable */

#define CLK_PCKENR2_MASK            ((uint8_t)0x8C)
#define CLK_PCKENR2_CAN             ((uint8_t)0x80) /* CAN clock enable */
#define CLK_PCKENR2_ADC             ((uint8_t)0x08) /* ADC clock enable */
#define CLK_PCKENR2_AWU             ((uint8_t)0x04) /* AWU clock enable */

#define CLK_CSSR_CSSD               ((uint8_t)0x08) /* Clock security system detection */
#define CLK_CSSR_CSSDIE             ((uint8_t)0x04) /* Clock security system detection interrupt enable */
#define CLK_CSSR_AUX                ((uint8_t)0x02) /* Auxiliary oscillator connected to master clock */
#define CLK_CSSR_CSSEN              ((uint8_t)0x01) /* Clock security system enable */

#define CLK_CCOR_CCOBSY             ((uint8_t)0x40) /* Configurable clock output busy */
#define CLK_CCOR_CCORDY             ((uint8_t)0x20) /* Configurable clock output ready */
#define CLK_CCOR_CCOSEL             ((uint8_t)0x1E) /* Configurable clock output selection */
#define CLK_CCOR_CCOEN              ((uint8_t)0x01) /* Configurable clock output enable */

#define CLK_HSITRIMR_HSITRIM_MASK   ((uint8_t)0x07) /* High speed internal oscillator trimmer */
#define CLK_HSITRIMR_HSITRIM_0      ((uint8_t)0x000 /* HSI Calibration Value 0 */
#define CLK_HSITRIMR_HSITRIM_1      ((uint8_t)0x01) /* HSI Calibration Value 1 */
#define CLK_HSITRIMR_HSITRIM_2      ((uint8_t)0x02) /* HSI Calibration Value 2 */
#define CLK_HSITRIMR_HSITRIM_3      ((uint8_t)0x03) /* HSI Calibration Value 3 */
#define CLK_HSITRIMR_HSITRIM_4      ((uint8_t)0x04) /* HSI Calibration Value 4 */
#define CLK_HSITRIMR_HSITRIM_5      ((uint8_t)0x05) /* HSI Calibration Value 5 */
#define CLK_HSITRIMR_HSITRIM_6      ((uint8_t)0x06) /* HSI Calibration Value 6 */
#define CLK_HSITRIMR_HSITRIM_7      ((uint8_t)0x07) /* HSI Calibration Value 7 */

#define CLK_SWIMCCR_SWIMDIV         ((uint8_t)0x01) /* SWIM Clock Dividing Factor */


//=============================================================================
// General Purpose Input/Output
//
typedef struct
{
    __IO uint8_t ODR;   // Output Data Register
    __IO uint8_t IDR;   // Input Data Register
    __IO uint8_t DDR;   // Data Direction Register
    __IO uint8_t CR1;   // Configuration Register 1
    __IO uint8_t CR2;   // Configuration Register 2
} stm8_gpio_t;

#define GPIOA_BaseAddress               0x5000
#define GPIOB_BaseAddress               0x5005
#define GPIOC_BaseAddress               0x500A
#define GPIOD_BaseAddress               0x500F
#define GPIOE_BaseAddress               0x5014
#define GPIOF_BaseAddress               0x5019
#define GPIOG_BaseAddress               0x501E
#define GPIOH_BaseAddress               0x5023
#define GPIOI_BaseAddress               0x5028
#define GPIOA                           ((stm8_gpio_t *)GPIOA_BaseAddress)
#define GPIOB                           ((stm8_gpio_t *)GPIOB_BaseAddress)
#define GPIOC                           ((stm8_gpio_t *)GPIOC_BaseAddress)
#define GPIOD                           ((stm8_gpio_t *)GPIOD_BaseAddress)
#define GPIOE                           ((stm8_gpio_t *)GPIOE_BaseAddress)
#define GPIOF                           ((stm8_gpio_t *)GPIOF_BaseAddress)
#define GPIOG                           ((stm8_gpio_t *)GPIOG_BaseAddress)
#define GPIOH                           ((stm8_gpio_t *)GPIOH_BaseAddress)
#define GPIOI                           ((stm8_gpio_t *)GPIOI_BaseAddress)

#define GPIO_PIN_0                      ((uint8_t)0x01)
#define GPIO_PIN_1                      ((uint8_t)0x02)
#define GPIO_PIN_2                      ((uint8_t)0x04)
#define GPIO_PIN_3                      ((uint8_t)0x08)
#define GPIO_PIN_4                      ((uint8_t)0x10)
#define GPIO_PIN_5                      ((uint8_t)0x20)
#define GPIO_PIN_6                      ((uint8_t)0x40)
#define GPIO_PIN_7                      ((uint8_t)0x80)

#define GPIO_ODR_0_MASK                 ((uint8_t)0x01)
#define GPIO_ODR_0_LOW                  ((uint8_t)0x00)
#define GPIO_ODR_0_HIGH                 ((uint8_t)0x01)
#define GPIO_ODR_1_MASK                 ((uint8_t)0x02)
#define GPIO_ODR_1_LOW                  ((uint8_t)0x00)
#define GPIO_ODR_1_HIGH                 ((uint8_t)0x02)
#define GPIO_ODR_2_MASK                 ((uint8_t)0x04)
#define GPIO_ODR_2_LOW                  ((uint8_t)0x00)
#define GPIO_ODR_2_HIGH                 ((uint8_t)0x04)
#define GPIO_ODR_3_MASK                 ((uint8_t)0x08)
#define GPIO_ODR_3_LOW                  ((uint8_t)0x00)
#define GPIO_ODR_3_HIGH                 ((uint8_t)0x08)
#define GPIO_ODR_4_MASK                 ((uint8_t)0x10)
#define GPIO_ODR_4_LOW                  ((uint8_t)0x00)
#define GPIO_ODR_4_HIGH                 ((uint8_t)0x10)
#define GPIO_ODR_5_MASK                 ((uint8_t)0x20)
#define GPIO_ODR_5_LOW                  ((uint8_t)0x00)
#define GPIO_ODR_5_HIGH                 ((uint8_t)0x20)
#define GPIO_ODR_6_MASK                 ((uint8_t)0x40)
#define GPIO_ODR_6_LOW                  ((uint8_t)0x00)
#define GPIO_ODR_6_HIGH                 ((uint8_t)0x40)
#define GPIO_ODR_7_MASK                 ((uint8_t)0x80)
#define GPIO_ODR_7_LOW                  ((uint8_t)0x00)
#define GPIO_ODR_7_HIGH                 ((uint8_t)0x80)

#define GPIO_IDR_0_MASK                 ((uint8_t)0x01)
#define GPIO_IDR_0_LOW                  ((uint8_t)0x00)
#define GPIO_IDR_0_HIGH                 ((uint8_t)0x01)
#define GPIO_IDR_1_MASK                 ((uint8_t)0x02)
#define GPIO_IDR_1_LOW                  ((uint8_t)0x00)
#define GPIO_IDR_1_HIGH                 ((uint8_t)0x02)
#define GPIO_IDR_2_MASK                 ((uint8_t)0x04)
#define GPIO_IDR_2_LOW                  ((uint8_t)0x00)
#define GPIO_IDR_2_HIGH                 ((uint8_t)0x04)
#define GPIO_IDR_3_MASK                 ((uint8_t)0x08)
#define GPIO_IDR_3_LOW                  ((uint8_t)0x00)
#define GPIO_IDR_3_HIGH                 ((uint8_t)0x08)
#define GPIO_IDR_4_MASK                 ((uint8_t)0x10)
#define GPIO_IDR_4_LOW                  ((uint8_t)0x00)
#define GPIO_IDR_4_HIGH                 ((uint8_t)0x10)
#define GPIO_IDR_5_MASK                 ((uint8_t)0x20)
#define GPIO_IDR_5_LOW                  ((uint8_t)0x00)
#define GPIO_IDR_5_HIGH                 ((uint8_t)0x20)
#define GPIO_IDR_6_MASK                 ((uint8_t)0x40)
#define GPIO_IDR_6_LOW                  ((uint8_t)0x00)
#define GPIO_IDR_6_HIGH                 ((uint8_t)0x40)
#define GPIO_IDR_7_MASK                 ((uint8_t)0x80)
#define GPIO_IDR_7_LOW                  ((uint8_t)0x00)
#define GPIO_IDR_7_HIGH                 ((uint8_t)0x80)

#define GPIO_DDR_0_MASK                 ((uint8_t)0x01)
#define GPIO_DDR_0_INPUT                ((uint8_t)0x00)
#define GPIO_DDR_0_OUTPUT               ((uint8_t)0x01)
#define GPIO_DDR_1_MASK                 ((uint8_t)0x02)
#define GPIO_DDR_1_INPUT                ((uint8_t)0x00)
#define GPIO_DDR_1_OUTPUT               ((uint8_t)0x02)
#define GPIO_DDR_2_MASK                 ((uint8_t)0x04)
#define GPIO_DDR_2_INPUT                ((uint8_t)0x00)
#define GPIO_DDR_2_OUTPUT               ((uint8_t)0x04)
#define GPIO_DDR_3_MASK                 ((uint8_t)0x08)
#define GPIO_DDR_3_INPUT                ((uint8_t)0x00)
#define GPIO_DDR_3_OUTPUT               ((uint8_t)0x08)
#define GPIO_DDR_4_MASK                 ((uint8_t)0x10)
#define GPIO_DDR_4_INPUT                ((uint8_t)0x00)
#define GPIO_DDR_4_OUTPUT               ((uint8_t)0x10)
#define GPIO_DDR_5_MASK                 ((uint8_t)0x20)
#define GPIO_DDR_5_INPUT                ((uint8_t)0x00)
#define GPIO_DDR_5_OUTPUT               ((uint8_t)0x20)
#define GPIO_DDR_6_MASK                 ((uint8_t)0x40)
#define GPIO_DDR_6_INPUT                ((uint8_t)0x00)
#define GPIO_DDR_6_OUTPUT               ((uint8_t)0x40)
#define GPIO_DDR_7_MASK                 ((uint8_t)0x80)
#define GPIO_DDR_7_INPUT                ((uint8_t)0x00)
#define GPIO_DDR_7_OUTPUT               ((uint8_t)0x80)

#define GPIO_CR1_0_INPUT_MASK           ((uint8_t)0x01)
#define GPIO_CR1_0_INPUT_FLOATING       ((uint8_t)0x00)
#define GPIO_CR1_0_INPUT_PULLUP         ((uint8_t)0x01)
#define GPIO_CR1_1_INPUT_MASK           ((uint8_t)0x02)
#define GPIO_CR1_1_INPUT_FLOATING       ((uint8_t)0x00)
#define GPIO_CR1_1_INPUT_PULLUP         ((uint8_t)0x02)
#define GPIO_CR1_2_INPUT_MASK           ((uint8_t)0x04)
#define GPIO_CR1_2_INPUT_FLOATING       ((uint8_t)0x00)
#define GPIO_CR1_2_INPUT_PULLUP         ((uint8_t)0x04)
#define GPIO_CR1_3_INPUT_MASK           ((uint8_t)0x08)
#define GPIO_CR1_3_INPUT_FLOATING       ((uint8_t)0x00)
#define GPIO_CR1_3_INPUT_PULLUP         ((uint8_t)0x08)
#define GPIO_CR1_4_INPUT_MASK           ((uint8_t)0x10)
#define GPIO_CR1_4_INPUT_FLOATING       ((uint8_t)0x00)
#define GPIO_CR1_4_INPUT_PULLUP         ((uint8_t)0x10)
#define GPIO_CR1_5_INPUT_MASK           ((uint8_t)0x20)
#define GPIO_CR1_5_INPUT_FLOATING       ((uint8_t)0x00)
#define GPIO_CR1_5_INPUT_PULLUP         ((uint8_t)0x20)
#define GPIO_CR1_6_INPUT_MASK           ((uint8_t)0x40)
#define GPIO_CR1_6_INPUT_FLOATING       ((uint8_t)0x00)
#define GPIO_CR1_6_INPUT_PULLUP         ((uint8_t)0x40)
#define GPIO_CR1_7_INPUT_MASK           ((uint8_t)0x80)
#define GPIO_CR1_7_INPUT_FLOATING       ((uint8_t)0x00)
#define GPIO_CR1_7_INPUT_PULLUP         ((uint8_t)0x80)

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

#define GPIO_CR2_0_OUTPUT_MASK          ((uint8_t)0x01)
#define GPIO_CR2_0_OUTPUT_2MHZ          ((uint8_t)0x00)
#define GPIO_CR2_0_OUTPUT_10MHZ         ((uint8_t)0x01)
#define GPIO_CR2_1_OUTPUT_MASK          ((uint8_t)0x02)
#define GPIO_CR2_1_OUTPUT_2MHZ          ((uint8_t)0x00)
#define GPIO_CR2_1_OUTPUT_10MHZ         ((uint8_t)0x02)
#define GPIO_CR2_2_OUTPUT_MASK          ((uint8_t)0x04)
#define GPIO_CR2_2_OUTPUT_2MHZ          ((uint8_t)0x00)
#define GPIO_CR2_2_OUTPUT_10MHZ         ((uint8_t)0x04)
#define GPIO_CR2_3_OUTPUT_MASK          ((uint8_t)0x08)
#define GPIO_CR2_3_OUTPUT_2MHZ          ((uint8_t)0x00)
#define GPIO_CR2_3_OUTPUT_10MHZ         ((uint8_t)0x08)
#define GPIO_CR2_4_OUTPUT_MASK          ((uint8_t)0x10)
#define GPIO_CR2_4_OUTPUT_2MHZ          ((uint8_t)0x00)
#define GPIO_CR2_4_OUTPUT_10MHZ         ((uint8_t)0x10)
#define GPIO_CR2_5_OUTPUT_MASK          ((uint8_t)0x20)
#define GPIO_CR2_5_OUTPUT_2MHZ          ((uint8_t)0x00)
#define GPIO_CR2_5_OUTPUT_10MHZ         ((uint8_t)0x20)
#define GPIO_CR2_6_OUTPUT_MASK          ((uint8_t)0x40)
#define GPIO_CR2_6_OUTPUT_2MHZ          ((uint8_t)0x00)
#define GPIO_CR2_6_OUTPUT_10MHZ         ((uint8_t)0x40)
#define GPIO_CR2_7_OUTPUT_MASK          ((uint8_t)0x80)
#define GPIO_CR2_7_OUTPUT_2MHZ          ((uint8_t)0x00)
#define GPIO_CR2_7_OUTPUT_10MHZ         ((uint8_t)0x80)


//=============================================================================
// Beeper
//

typedef struct
{
    __IO uint8_t CSR;   // Control/status register
} stm8_beep_t;

#define BEEP_BaseAddress        0x50F3
#define BEEP                    ((stm8_beep_t *)BEEP_BaseAddress)

#define BEEP_CSR_SEL_MASK       0xC0        // Selection
#define BEEP_CSR_SEL_1KHZ       0x00
#define BEEP_CSR_SEL_2KHZ       0x40
#define BEEP_CSR_SEL_4KHZ       0x80

#define BEEP_CSR_EN_MASK        0x20        // Enable
#define BEEP_CSR_EN_DISABLE     0x00
#define BEEP_CSR_EN_ENABLE      0x20

#define BEEP_CSR_DIV_MASK       0x1F        // Prescaler divider
#define BEEP_CSR_DIV_2          0x00
#define BEEP_CSR_DIV_3          0x01
#define BEEP_CSR_DIV_4          0x02
#define BEEP_CSR_DIV_5          0x03
#define BEEP_CSR_DIV_6          0x04
#define BEEP_CSR_DIV_7          0x05
#define BEEP_CSR_DIV_8          0x06
#define BEEP_CSR_DIV_9          0x07
#define BEEP_CSR_DIV_10         0x08
#define BEEP_CSR_DIV_11         0x09
#define BEEP_CSR_DIV_12         0x0A
#define BEEP_CSR_DIV_13         0x0B
#define BEEP_CSR_DIV_14         0x0C
#define BEEP_CSR_DIV_15         0x0D
#define BEEP_CSR_DIV_16         0x0E
#define BEEP_CSR_DIV_17         0x0F
#define BEEP_CSR_DIV_18         0x10
#define BEEP_CSR_DIV_19         0x11
#define BEEP_CSR_DIV_20         0x12
#define BEEP_CSR_DIV_21         0x13
#define BEEP_CSR_DIV_22         0x14
#define BEEP_CSR_DIV_23         0x15
#define BEEP_CSR_DIV_24         0x16
#define BEEP_CSR_DIV_25         0x17
#define BEEP_CSR_DIV_26         0x18
#define BEEP_CSR_DIV_27         0x19
#define BEEP_CSR_DIV_28         0x1A
#define BEEP_CSR_DIV_29         0x1B
#define BEEP_CSR_DIV_30         0x1C
#define BEEP_CSR_DIV_31         0x1D
#define BEEP_CSR_DIV_32         0x1E
#define BEEP_CSR_DIV_RESET      0x1F        // Reset value, do not set

//=============================================================================
// Auto Wakeup
//

typedef struct
{
    __IO uint8_t CSR;   // Control/status register
    __IO uint8_t APR;   // Asynchronous prescaler register
    __IO uint8_t TBR;   // Timebase selection register
} stm8_awu_t;

#define AWU_BaseAddress         0x50F3
#define AWU                     ((stm8_awu_t *)AWU_BaseAddress)

#define AWU_CSR_AWUF_MASK       0x20        // Flag (read clears)
#define AWU_CSR_AWUF_NONE       0x00
#define AWU_CSR_AWUF_OCCURRED   0x20

#define AWU_CSR_AWEN_MASK       0x10        // Enable
#define AWU_CSR_AWEN_DISABLE    0x00
#define AWU_CSR_AWEN_ENABLE     0x10

#define AWU_CSR_MSR_MASK        0x01        // Measurement enable
#define AWU_CSR_MSR_DISABLE     0x00
#define AWU_CSR_MSR_ENABLE      0x01

#define AWU_APR_DIV_MASK        0x3F        // Asynchronous prescaler divider
#define AWU_APR_DIV_2           0x00
#define AWU_APR_DIV_3           0x01
#define AWU_APR_DIV_4           0x02
#define AWU_APR_DIV_5           0x03
#define AWU_APR_DIV_6           0x04
#define AWU_APR_DIV_7           0x05
#define AWU_APR_DIV_8           0x06
#define AWU_APR_DIV_9           0x07
#define AWU_APR_DIV_10          0x08
#define AWU_APR_DIV_11          0x09
#define AWU_APR_DIV_12          0x0A
#define AWU_APR_DIV_13          0x0B
#define AWU_APR_DIV_14          0x0C
#define AWU_APR_DIV_15          0x0D
#define AWU_APR_DIV_16          0x0E
#define AWU_APR_DIV_17          0x0F
#define AWU_APR_DIV_18          0x10
#define AWU_APR_DIV_19          0x11
#define AWU_APR_DIV_20          0x12
#define AWU_APR_DIV_21          0x13
#define AWU_APR_DIV_22          0x14
#define AWU_APR_DIV_23          0x15
#define AWU_APR_DIV_24          0x16
#define AWU_APR_DIV_25          0x17
#define AWU_APR_DIV_26          0x18
#define AWU_APR_DIV_27          0x19
#define AWU_APR_DIV_28          0x1A
#define AWU_APR_DIV_29          0x1B
#define AWU_APR_DIV_30          0x1C
#define AWU_APR_DIV_31          0x1D
#define AWU_APR_DIV_32          0x1E
#define AWU_APR_DIV_33          0x1F
#define AWU_APR_DIV_34          0x20
#define AWU_APR_DIV_35          0x21
#define AWU_APR_DIV_36          0x22
#define AWU_APR_DIV_37          0x23
#define AWU_APR_DIV_38          0x24
#define AWU_APR_DIV_39          0x25
#define AWU_APR_DIV_40          0x26
#define AWU_APR_DIV_41          0x27
#define AWU_APR_DIV_42          0x28
#define AWU_APR_DIV_43          0x29
#define AWU_APR_DIV_44          0x2A
#define AWU_APR_DIV_45          0x2B
#define AWU_APR_DIV_46          0x2C
#define AWU_APR_DIV_47          0x2D
#define AWU_APR_DIV_48          0x2E
#define AWU_APR_DIV_49          0x2F
#define AWU_APR_DIV_50          0x30
#define AWU_APR_DIV_51          0x31
#define AWU_APR_DIV_52          0x32
#define AWU_APR_DIV_53          0x33
#define AWU_APR_DIV_54          0x34
#define AWU_APR_DIV_55          0x35
#define AWU_APR_DIV_56          0x36
#define AWU_APR_DIV_57          0x37
#define AWU_APR_DIV_58          0x38
#define AWU_APR_DIV_59          0x39
#define AWU_APR_DIV_60          0x3A
#define AWU_APR_DIV_61          0x3B
#define AWU_APR_DIV_62          0x3C
#define AWU_APR_DIV_63          0x3D
#define AWU_APR_DIV_64          0x3E

#define AWU_TBR_MASK            0x0F        // Timebase selection ((factor x APR)/lsi)
#define AWU_TBR_NONE            0x00
#define AWU_TBR_1               0x01        // 0.015625 ms - 0.5 ms
#define AWU_TBR_2               0x20        // 0.5 ms - 1.0 ms
#define AWU_TBR_4               0x03        // 1 ms - 2 ms
#define AWU_TBR_8               0x04        // 2 ms - 4 ms
#define AWU_TBR_16              0x05        // 4 ms - 8 ms
#define AWU_TBR_32              0x06        // 8 ms - 16 ms
#define AWU_TBR_64              0x07        // 16 ms - 32 ms
#define AWU_TBR_128             0x08        // 32 ms - 64 ms
#define AWU_TBR_256             0x09        // 64 ms - 128 ms
#define AWU_TBR_512             0x0A        // 128 ms - 256 ms
#define AWU_TBR_1024            0x0B        // 256 ms - 512 ms
#define AWU_TBR_2048            0x0C        // 512 ms - 1.024 s
#define AWU_TBR_4096            0x0D        // 1.024 s - 2.048 s
#define AWU_TBR_5x2048          0x0E        // 2.080 s - 5.120 s
#define AWU_TBR_30x2048         0x0F        // 5.280 s - 30.720 s


//=============================================================================
// Timers
//

//-----------------------------------------------------------------------------
// Timer 1
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

#define TIM1_BaseAddress        0x5250
#define TIM2_BaseAddress        0x5300
#define TIM3_BaseAddress        0x5320
#define TIM5_BaseAddress        0x5300
#define TIM6_BaseAddress        0x5340
#define TIM1                    ((stm8_tim1_t *)TIM1_BaseAddress)

#define TIM1_CR1_ARPE_MASK      ((uint8_t)0x80) /* Auto-Reload Preload Enable mask. */
#define TIM1_CR1_ARPE_DISABLE   ((uint8_t)0x00)
#define TIM1_CR1_ARPE_ENABLE    ((uint8_t)0x80)

#define TIM1_CR1_CMS_MASK       ((uint8_t)0x60) /* Center-aligned Mode Selection mask. */
#define TIM1_CR1_CMS_EDGE       ((uint8_t)0x00)
#define TIM1_CR1_CMS_CENTER1    ((uint8_t)0x20)
#define TIM1_CR1_CMS_CENTER2    ((uint8_t)0x40)
#define TIM1_CR1_CMS_CENTER3    ((uint8_t)0x60)

#define TIM1_CR1_DIR_MASK       ((uint8_t)0x10) /* Direction mask. */
#define TIM1_CR1_DIR_UP         ((uint8_t)0x00)
#define TIM1_CR1_DIR_DOWN       ((uint8_t)0x10)

#define TIM1_CR1_OPM_MASK       ((uint8_t)0x08) /* One Pulse Mode mask. */
#define TIM1_CR1_OPM_DISABLE    ((uint8_t)0x00)
#define TIM1_CR1_OPM_ENABLE     ((uint8_t)0x08)

#define TIM1_CR1_URS_MASK       ((uint8_t)0x04) /* Update Request Source mask. */
#define TIM1_CR1_URS_ALL        ((uint8_t)0x00)
#define TIM1_CR1_URS_UPDATE     ((uint8_t)0x04)

#define TIM1_CR1_UDIS_MASK      ((uint8_t)0x02) /* Update DIsable mask. */
#define TIM1_CR1_UDIS_DISABLE   ((uint8_t)0x00)
#define TIM1_CR1_UDIS_ENABLE    ((uint8_t)0x02)

#define TIM1_CR1_CEN_MASK       ((uint8_t)0x01) /* Counter Enable mask. */
#define TIM1_CR1_CEN_DISABLE    ((uint8_t)0x00)
#define TIM1_CR1_CEN_ENABLE     ((uint8_t)0x01)

#define TIM1_CR2_MMS_MASK       ((uint8_t)0x70) /* MMS Selection mask. */
#define TIM1_CR2_MMS_RESET      ((uint8_t)0x00)
#define TIM1_CR2_MMS_ENABLE     ((uint8_t)0x10)
#define TIM1_CR2_MMS_UPDATE     ((uint8_t)0x20)
#define TIM1_CR2_MMS_OC1        ((uint8_t)0x30)
#define TIM1_CR2_MMS_OC1REF     ((uint8_t)0x40)
#define TIM1_CR2_MMS_OC2REF     ((uint8_t)0x50)
#define TIM1_CR2_MMS_OC3REF     ((uint8_t)0x60)
#define TIM1_CR2_MMS_OC4REF     ((uint8_t)0x70)

#define TIM1_CR2_COMS_MASK      ((uint8_t)0x04) /* Capture/Compare Control Update Selection mask. */
#define TIM1_CR2_COMS_COMG      ((uint8_t)0x00)
#define TIM1_CR2_COMS_ALL       ((uint8_t)0x04)

#define TIM1_CR2_CCPC_MASK      ((uint8_t)0x01) /* Capture/Compare Preloaded Control mask. */
#define TIM1_CR2_CCPC_NOTPRELOAD ((uint8_t)0x00)
#define TIM1_CR2_CCPC_PRELOADED ((uint8_t)0x01)

#define TIM1_SMCR_MSM_MASK      ((uint8_t)0x80) /* Master/Slave Mode mask. */
#define TIM1_SMCR_MSM_DISABLE   ((uint8_t)0x00)
#define TIM1_SMCR_MSM_ENABLE    ((uint8_t)0x80)

#define TIM1_SMCR_TS_MASK       ((uint8_t)0x70) /* Trigger Selection mask. */
#define TIM1_SMCR_TS_TIM6       ((uint8_t)0x00)  /* TRIG Input source =  TIM6 TRIG Output  */
#define TIM1_SMCR_TS_TIM5       ((uint8_t)0x30)  /* TRIG Input source =  TIM5 TRIG Output  */
#define TIM1_SMCR_TS_TI1F_ED    ((uint8_t)0x40)
#define TIM1_SMCR_TS_TI1FP1     ((uint8_t)0x50)
#define TIM1_SMCR_TS_TI2FP2     ((uint8_t)0x60)
#define TIM1_SMCR_TS_ETRF       ((uint8_t)0x70)

#define TIM1_SMCR_SMS_MASK      ((uint8_t)0x07) /* Slave Mode Selection mask. */
#define TIM1_SMCR_SMS_DISABLE   ((uint8_t)0x00)
#define TIM1_SMCR_SMS_ENCODER1  ((uint8_t)0x01)
#define TIM1_SMCR_SMS_ENCODER2  ((uint8_t)0x02)
#define TIM1_SMCR_SMS_ENCODER3  ((uint8_t)0x03)
#define TIM1_SMCR_SMS_RESET     ((uint8_t)0x04)
#define TIM1_SMCR_SMS_GATED     ((uint8_t)0x05)
#define TIM1_SMCR_SMS_TRIGGER   ((uint8_t)0x06)
#define TIM1_SMCR_SMS_EXTERNAL1 ((uint8_t)0x07)

#define TIM1_ETR_ETP_MASK       ((uint8_t)0x80) /* External Trigger Polarity mask. */
#define TIM1_ETR_ETP_NOTINVERTED ((uint8_t)0x00)
#define TIM1_ETR_ETP_INVERTED   ((uint8_t)0x80)

#define TIM1_ETR_ECE_MASK       ((uint8_t)0x40) /* External Clock mask. */
#define TIM1_ETR_ECE_DISABLE    ((uint8_t)0x00)
#define TIM1_ETR_ECE_ENABLE     ((uint8_t)0x40)

#define TIM1_ETR_ETPS_MASK      ((uint8_t)0x30) /* External Trigger Prescaler mask. */
#define TIM1_ETR_ETPS_OFF       ((uint8_t)0x00)
#define TIM1_ETR_ETPS_DIV2      ((uint8_t)0x10)
#define TIM1_ETR_ETPS_DIV4      ((uint8_t)0x20)
#define TIM1_ETR_ETPS_DIV8      ((uint8_t)0x30)

#define TIM1_ETR_ETF_MASK       ((uint8_t)0x0F) /* External Trigger Filter mask. */
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

#define TIM1_IER_BIE_MASK       ((uint8_t)0x80) /* Break Interrupt Enable mask. */
#define TIM1_IER_BIE_DISABLE    ((uint8_t)0x00)
#define TIM1_IER_BIE_ENABLE     ((uint8_t)0x80)

#define TIM1_IER_TIE_MASK       ((uint8_t)0x40) /* Trigger Interrupt Enable mask. */
#define TIM1_IER_TIE_DISABLE    ((uint8_t)0x00)
#define TIM1_IER_TIE_ENABLE     ((uint8_t)0x40)

#define TIM1_IER_COMIE_MASK     ((uint8_t)0x20) /*  Commutation Interrupt Enable mask.*/
#define TIM1_IER_COMIE_DISABLE  ((uint8_t)0x00)
#define TIM1_IER_COMIE_ENABLE   ((uint8_t)0x20)

#define TIM1_IER_CC4IE_MASK     ((uint8_t)0x10) /* Capture/Compare 4 Interrupt Enable mask. */
#define TIM1_IER_CC4IE_DISABLE  ((uint8_t)0x00)
#define TIM1_IER_CC4IE_ENABLE   ((uint8_t)0x10)

#define TIM1_IER_CC3IE_MASK     ((uint8_t)0x08) /* Capture/Compare 3 Interrupt Enable mask. */
#define TIM1_IER_CC3IE_DISABLE  ((uint8_t)0x00)
#define TIM1_IER_CC3IE_ENABLE   ((uint8_t)0x08)

#define TIM1_IER_CC2IE_MASK     ((uint8_t)0x04) /* Capture/Compare 2 Interrupt Enable mask. */
#define TIM1_IER_CC2IE_DISABLE  ((uint8_t)0x00)
#define TIM1_IER_CC2IE_ENABLE   ((uint8_t)0x04)

#define TIM1_IER_CC1IE_MASK     ((uint8_t)0x02) /* Capture/Compare 1 Interrupt Enable mask. */
#define TIM1_IER_CC1IE_DISABLE  ((uint8_t)0x00)
#define TIM1_IER_CC1IE_ENABLE   ((uint8_t)0x02)

#define TIM1_IER_UIE_MASK       ((uint8_t)0x01) /* Update Interrupt Enable mask. */
#define TIM1_IER_UIE_DISABLE    ((uint8_t)0x00)
#define TIM1_IER_UIE_ENABLE     ((uint8_t)0x01)

#define TIM1_SR1_BIF_MASK       ((uint8_t)0x80) /* Break Interrupt Flag mask. */
#define TIM1_SR1_BIF_NONE       ((uint8_t)0x00)
#define TIM1_SR1_BIF_DETECTED   ((uint8_t)0x80)

#define TIM1_SR1_TIF_MASK       ((uint8_t)0x40) /* Trigger Interrupt Flag mask. */
#define TIM1_SR1_TIF_NONE       ((uint8_t)0x00)
#define TIM1_SR1_TIF_PENDING    ((uint8_t)0x40)

#define TIM1_SR1_COMIF_MASK     ((uint8_t)0x20) /* Commutation Interrupt Flag mask. */
#define TIM1_SR1_COMIF_NONE     ((uint8_t)0x00)
#define TIM1_SR1_COMIF_PENDING  ((uint8_t)0x20)

#define TIM1_SR1_CC4IF_MASK     ((uint8_t)0x10) /* Capture/Compare 4 Interrupt Flag mask. */
#define TIM1_SR1_CC4IF_NONE     ((uint8_t)0x00)
#define TIM1_SR1_CC4IF_MATCH    ((uint8_t)0x10) // Input
#define TIM1_SR1_CC4IF_CAPTURE  ((uint8_t)0x10) // Output

#define TIM1_SR1_CC3IF_MASK     ((uint8_t)0x08) /* Capture/Compare 3 Interrupt Flag mask. */
#define TIM1_SR1_CC3IF_NONE     ((uint8_t)0x00)
#define TIM1_SR1_CC3IF_MATCH    ((uint8_t)0x08) // Input
#define TIM1_SR1_CC3IF_CAPTURE  ((uint8_t)0x08) // Output

#define TIM1_SR1_CC2IF_MASK     ((uint8_t)0x04) /* Capture/Compare 2 Interrupt Flag mask. */
#define TIM1_SR1_CC2IF_NONE     ((uint8_t)0x00)
#define TIM1_SR1_CC2IF_MATCH    ((uint8_t)0x04) // Input
#define TIM1_SR1_CC2IF_CAPTURE  ((uint8_t)0x04) // Output

#define TIM1_SR1_CC1IF_MASK     ((uint8_t)0x02) /* Capture/Compare 1 Interrupt Flag mask. */
#define TIM1_SR1_CC1IF_NONE     ((uint8_t)0x00)
#define TIM1_SR1_CC1IF_MATCH    ((uint8_t)0x02) // Input
#define TIM1_SR1_CC1IF_CAPTURE  ((uint8_t)0x02) // Output

#define TIM1_SR1_UIF_MASK       ((uint8_t)0x01) /* Update Interrupt Flag mask. */
#define TIM1_SR1_UIF_NONE       ((uint8_t)0x00)
#define TIM1_SR1_UIF_PENDING    ((uint8_t)0x01)

#define TIM1_SR2_CC4OF_MASK     ((uint8_t)0x10) /* Capture/Compare 4 Overcapture Flag mask. */
#define TIM1_SR2_CC4OF_NONE     ((uint8_t)0x00)
#define TIM1_SR2_CC4OF_DETECTED ((uint8_t)0x10)

#define TIM1_SR2_CC3OF_MASK     ((uint8_t)0x08) /* Capture/Compare 3 Overcapture Flag mask. */
#define TIM1_SR2_CC3OF_NONE     ((uint8_t)0x00)
#define TIM1_SR2_CC3OF_DETECTED ((uint8_t)0x08)

#define TIM1_SR2_CC2OF_MASK     ((uint8_t)0x04) /* Capture/Compare 2 Overcapture Flag mask. */
#define TIM1_SR2_CC2OF_NONE     ((uint8_t)0x00)
#define TIM1_SR2_CC2OF_DETECTED ((uint8_t)0x04)

#define TIM1_SR2_CC1OF_MASK     ((uint8_t)0x02) /* Capture/Compare 1 Overcapture Flag mask. */
#define TIM1_SR2_CC1OF_NONE     ((uint8_t)0x00)
#define TIM1_SR2_CC1OF_DETECTED ((uint8_t)0x02)

#define TIM1_EGR_BG_MASK        ((uint8_t)0x80) /* Break Generation mask. */
#define TIM1_EGR_BG_DISABLE     ((uint8_t)0x00)
#define TIM1_EGR_BG_ENABLE      ((uint8_t)0x80)

#define TIM1_EGR_TG_MASK        ((uint8_t)0x40) /* Trigger Generation mask. */
#define TIM1_EGR_TG_DISABLE     ((uint8_t)0x00)
#define TIM1_EGR_TG_ENABLE      ((uint8_t)0x40)

#define TIM1_EGR_COMG_MASK      ((uint8_t)0x20) /* Capture/Compare Control Update Generation mask. */
#define TIM1_EGR_COMG_DISABLE   ((uint8_t)0x00)
#define TIM1_EGR_COMG_ENABLE    ((uint8_t)0x20)

#define TIM1_EGR_CC4G_MASK      ((uint8_t)0x10) /* Capture/Compare 4 Generation mask. */
#define TIM1_EGR_CC4G_DISABLE   ((uint8_t)0x00)
#define TIM1_EGR_CC4G_ENABLE    ((uint8_t)0x10)

#define TIM1_EGR_CC3G_MASK      ((uint8_t)0x08) /* Capture/Compare 3 Generation mask. */
#define TIM1_EGR_CC3G_DISABLE   ((uint8_t)0x00)
#define TIM1_EGR_CC3G_ENABLE    ((uint8_t)0x08)

#define TIM1_EGR_CC2G_MASK      ((uint8_t)0x04) /* Capture/Compare 2 Generation mask. */
#define TIM1_EGR_CC2G_DISABLE   ((uint8_t)0x00)
#define TIM1_EGR_CC2G_ENABLE    ((uint8_t)0x04)

#define TIM1_EGR_CC1G_MASK      ((uint8_t)0x02) /* Capture/Compare 1 Generation mask. */
#define TIM1_EGR_CC1G_DISABLE   ((uint8_t)0x00)
#define TIM1_EGR_CC1G_ENABLE    ((uint8_t)0x02)

#define TIM1_EGR_UG_MASK        ((uint8_t)0x01) /* Update Generation mask. */
#define TIM1_EGR_UG_DISABLE     ((uint8_t)0x00)
#define TIM1_EGR_UG_ENABLE      ((uint8_t)0x01)

#define TIM1_CCMR_ICxPSC_MASK   ((uint8_t)0x0C) /* Input Capture x Prescaler mask. */
#define TIM1_CCMR_ICxPSC_NONE   ((uint8_t)0x00)
#define TIM1_CCMR_ICxPSC_DIV2   ((uint8_t)0x04)
#define TIM1_CCMR_ICxPSC_DIV4   ((uint8_t)0x08)
#define TIM1_CCMR_ICxPSC_DIV8   ((uint8_t)0x0C)

#define TIM1_CCMR_ICxF_MASK     ((uint8_t)0xF0) /* Input Capture x Filter mask. */
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

#define TIM1_CCMR_OCxCE_MASK    ((uint8_t)0x80) /* Output Compare x Clear Enable mask. */
#define TIM1_CCMR_OCxCE_DISABLE ((uint8_t)0x00)
#define TIM1_CCMR_OCxCE_ENABLE  ((uint8_t)0x80)

#define TIM1_CCMR_OCxM_MASK     ((uint8_t)0x70) /* Output Compare x Mode mask. */
#define TIM1_CCMR_OCxM_FROZEN   ((uint8_t)0x00)
#define TIM1_CCMR_OCxM_ACTIVE   ((uint8_t)0x10)
#define TIM1_CCMR_OCxM_INACTIVE ((uint8_t)0x20)
#define TIM1_CCMR_OCxM_TOGGLE   ((uint8_t)0x30)
#define TIM1_CCMR_OCxM_FORCELOW ((uint8_t)0x40)
#define TIM1_CCMR_OCxM_FORCEHIGH ((uint8_t)0x50)
#define TIM1_CCMR_OCxM_PWM1     ((uint8_t)0x60)
#define TIM1_CCMR_OCxM_PWM2     ((uint8_t)0x70)

#define TIM1_CCMR_OCxPE_MASK    ((uint8_t)0x08) /* Output Compare x Preload Enable mask. */
#define TIM1_CCMR_OCxPE_DISABLE ((uint8_t)0x00)
#define TIM1_CCMR_OCxPE_ENABLE  ((uint8_t)0x08)

#define TIM1_CCMR_OCxFE_MASK    ((uint8_t)0x04) /* Output Compare x Fast Enable mask. */
#define TIM1_CCMR_OCxFE_DISABLE ((uint8_t)0x00)
#define TIM1_CCMR_OCxFE_ENABLE  ((uint8_t)0x04)

#define TIM1_CCMR_CCxS_MASK     ((uint8_t)0x03) /* Capture/Compare x Selection mask. */
#define TIM1_CCMR_CCxS_OUTPUT   ((uint8_t)0x00)
#define TIM1_CCMR_CCxS_IN_TI1FP2 ((uint8_t)0x01)
#define TIM1_CCMR_CCxS_IN_TI2FP1 ((uint8_t)0x02)
#define TIM1_CCMR_CCxS_IN_TRC   ((uint8_t)0x03)

#define TIM1_CCER1_CC2NP_MASK   ((uint8_t)0x80) /* Capture/Compare 2 Complementary output Polarity mask. */
#define TIM1_CCER1_CC2NP_DISABLE ((uint8_t)0x00)
#define TIM1_CCER1_CC2NP_ENABLE ((uint8_t)0x80)

#define TIM1_CCER1_CC2NE_MASK   ((uint8_t)0x40) /* Capture/Compare 2 Complementary output enable mask. */
#define TIM1_CCER1_CC2NE_DISABLE ((uint8_t)0x00)
#define TIM1_CCER1_CC2NE_ENABLE ((uint8_t)0x40)

#define TIM1_CCER1_CC2P_MASK    ((uint8_t)0x20) /* Capture/Compare 2 output Polarity mask. */
#define TIM1_CCER1_CC2P_DISABLE ((uint8_t)0x00)
#define TIM1_CCER1_CC2P_ENABLE  ((uint8_t)0x20)

#define TIM1_CCER1_CC2E_MASK    ((uint8_t)0x10) /* Capture/Compare 2 output enable mask. */
#define TIM1_CCER1_CC2E_DIABLE  ((uint8_t)0x00)
#define TIM1_CCER1_CC2E_ENABLE  ((uint8_t)0x10)

#define TIM1_CCER1_CC1NP_MASK   ((uint8_t)0x08) /* Capture/Compare 1 Complementary output Polarity mask. */
#define TIM1_CCER1_CC1NP_DISABLE ((uint8_t)0x00)
#define TIM1_CCER1_CC1NP_ENABLE ((uint8_t)0x08)

#define TIM1_CCER1_CC1NE_MASK   ((uint8_t)0x04) /* Capture/Compare 1 Complementary output enable mask. */
#define TIM1_CCER1_CC1NE_DISABLE ((uint8_t)0x00)
#define TIM1_CCER1_CC1NE_ENABLE ((uint8_t)0x04)

#define TIM1_CCER1_CC1P_MASK    ((uint8_t)0x02) /* Capture/Compare 1 output Polarity mask. */
#define TIM1_CCER1_CC1P_DISABLE ((uint8_t)0x00)
#define TIM1_CCER1_CC1P_ENABLE  ((uint8_t)0x02)

#define TIM1_CCER1_CC1E_MASK    ((uint8_t)0x01) /* Capture/Compare 1 output enable mask. */
#define TIM1_CCER1_CC1E_DISABLE ((uint8_t)0x00)
#define TIM1_CCER1_CC1E_ENABLE  ((uint8_t)0x01)

#define TIM1_CCER2_CC4P_MASK    ((uint8_t)0x20) /* Capture/Compare 4 output Polarity mask. */
#define TIM1_CCER2_CC4P_DISABLE  ((uint8_t)0x00)
#define TIM1_CCER2_CC4P_ENABLE  ((uint8_t)0x20)

#define TIM1_CCER2_CC4E_MASK    ((uint8_t)0x10) /* Capture/Compare 4 output enable mask. */
#define TIM1_CCER2_CC4E_DISABLE ((uint8_t)0x00)
#define TIM1_CCER2_CC4E_ENABLE  ((uint8_t)0x10)

#define TIM1_CCER2_CC3NP_MASK   ((uint8_t)0x08) /* Capture/Compare 3 Complementary output Polarity mask. */
#define TIM1_CCER2_CC3NP_DISABLE ((uint8_t)0x00)
#define TIM1_CCER2_CC3NP_ENABLE ((uint8_t)0x08)

#define TIM1_CCER2_CC3NE_MASK   ((uint8_t)0x04) /* Capture/Compare 3 Complementary output enable mask. */
#define TIM1_CCER2_CC3NE_DISABLE ((uint8_t)0x00)
#define TIM1_CCER2_CC3NE_ENABLE ((uint8_t)0x04)

#define TIM1_CCER2_CC3P_MASK    ((uint8_t)0x02) /* Capture/Compare 3 output Polarity mask. */
#define TIM1_CCER2_CC3P_DISABLE ((uint8_t)0x00)
#define TIM1_CCER2_CC3P_ENABLE  ((uint8_t)0x02)

#define TIM1_CCER2_CC3E_MASK    ((uint8_t)0x01) /* Capture/Compare 3 output enable mask. */
#define TIM1_CCER2_CC3E_DISABLE ((uint8_t)0x00)
#define TIM1_CCER2_CC3E_ENABLE  ((uint8_t)0x01)

#define TIM1_CNTRH_CNT_MASK     ((uint8_t)0xFF) /* Counter Value (MSB) mask. */
#define TIM1_CNTRL_CNT_MASK     ((uint8_t)0xFF) /* Counter Value (LSB) mask. */

#define TIM1_PSCRH_PSC_MASK     ((uint8_t)0xFF) /* Prescaler Value (MSB) mask. */
#define TIM1_PSCRL_PSC_MASK     ((uint8_t)0xFF) /* Prescaler Value (LSB) mask. */

#define TIM1_ARRH_ARR_MASK      ((uint8_t)0xFF) /* Autoreload Value (MSB) mask. */
#define TIM1_ARRL_ARR_MASK      ((uint8_t)0xFF) /* Autoreload Value (LSB) mask. */

#define TIM1_RCR_REP_MASK       ((uint8_t)0xFF) /* Repetition Counter Value mask. */

#define TIM1_CCR1H_CCR1_MASK    ((uint8_t)0xFF) /* Capture/Compare 1 Value (MSB) mask. */
#define TIM1_CCR1L_CCR1_MASK    ((uint8_t)0xFF) /* Capture/Compare 1 Value (LSB) mask. */

#define TIM1_CCR2H_CCR2_MASK    ((uint8_t)0xFF) /* Capture/Compare 2 Value (MSB) mask. */
#define TIM1_CCR2L_CCR2_MASK    ((uint8_t)0xFF) /* Capture/Compare 2 Value (LSB) mask. */

#define TIM1_CCR3H_CCR3_MASK    ((uint8_t)0xFF) /* Capture/Compare 3 Value (MSB) mask. */
#define TIM1_CCR3L_CCR3_MASK    ((uint8_t)0xFF) /* Capture/Compare 3 Value (LSB) mask. */

#define TIM1_CCR4H_CCR4_MASK    ((uint8_t)0xFF) /* Capture/Compare 4 Value (MSB) mask. */
#define TIM1_CCR4L_CCR4_MASK    ((uint8_t)0xFF) /* Capture/Compare 4 Value (LSB) mask. */

#define TIM1_BKR_MOE_MASK       ((uint8_t)0x80) /* Main Output Enable mask. */
#define TIM1_BKR_MOE_DISABLE    ((uint8_t)0x00)
#define TIM1_BKR_MOE_ENABLE     ((uint8_t)0x80)

#define TIM1_BKR_AOE_MASK       ((uint8_t)0x40) /* Automatic Output Enable mask. */
#define TIM1_BKR_AOE_DISABLE    ((uint8_t)0x00)
#define TIM1_BKR_AOE_ENABLE     ((uint8_t)0x40)

#define TIM1_BKR_BKP_MASK       ((uint8_t)0x20) /* Break Polarity mask. */
#define TIM1_BKR_BKP_LOW        ((uint8_t)0x00)
#define TIM1_BKR_BKP_HIGH       ((uint8_t)0x20)

#define TIM1_BKR_BKE_MASK       ((uint8_t)0x10) /* Break Enable mask. */
#define TIM1_BKR_BKE_DISABLE    ((uint8_t)0x00)
#define TIM1_BKR_BKE_ENABLE     ((uint8_t)0x10)

#define TIM1_BKR_OSSR_MASK      ((uint8_t)0x08) /* Off-State Selection for Run mode mask. */
#define TIM1_BKR_OSSR_DISABLE   ((uint8_t)0x00)
#define TIM1_BKR_OSSR_ENABLE    ((uint8_t)0x08)

#define TIM1_BKR_OSSI_MASK      ((uint8_t)0x04) /* Off-State Selection for Idle mode mask. */
#define TIM1_BKR_OSSI_DISABLE   ((uint8_t)0x00)
#define TIM1_BKR_OSSI_ENABLE    ((uint8_t)0x04)

#define TIM1_BKR_LOCK_MASK      ((uint8_t)0x03) /* Lock Configuration mask. */
#define TIM1_BKR_LOCK_OFF       ((uint8_t)0x00)
#define TIM1_BKR_LOCK_LEVEL1    ((uint8_t)0x01)
#define TIM1_BKR_LOCK_LEVEL2    ((uint8_t)0x02)
#define TIM1_BKR_LOCK_LEVEL3    ((uint8_t)0x03)

#define TIM1_DTR_DTG_MASK       ((uint8_t)0xFF) /* Dead-Time Generator set-up mask. */

#define TIM1_OISR_OIS4_MASK     ((uint8_t)0x40) /* Output Idle state 4 (OC4 output) mask. */
#define TIM1_OISR_OIS4_DISABLE  ((uint8_t)0x00)
#define TIM1_OISR_OIS4_ENABLE   ((uint8_t)0x40)

#define TIM1_OISR_OIS3N_MASK    ((uint8_t)0x20) /* Output Idle state 3 (OC3N output) mask. */
#define TIM1_OISR_OIS3N_DISABLE ((uint8_t)0x00)
#define TIM1_OISR_OIS3N_ENABLE  ((uint8_t)0x20)

#define TIM1_OISR_OIS3_MASK     ((uint8_t)0x10) /* Output Idle state 3 (OC3 output) mask. */
#define TIM1_OISR_OIS3_DISABLE  ((uint8_t)0x00)
#define TIM1_OISR_OIS3_ENABLE   ((uint8_t)0x10)

#define TIM1_OISR_OIS2N_MASK    ((uint8_t)0x08) /* Output Idle state 2 (OC2N output) mask. */
#define TIM1_OISR_OIS2N_DISABLE ((uint8_t)0x00)
#define TIM1_OISR_OIS2N_ENABLE  ((uint8_t)0x08)

#define TIM1_OISR_OIS2_MASK     ((uint8_t)0x04) /* Output Idle state 2 (OC2 output) mask. */
#define TIM1_OISR_OIS2_DISABLE  ((uint8_t)0x00)
#define TIM1_OISR_OIS2_ENABLE   ((uint8_t)0x04)

#define TIM1_OISR_OIS1N_MASK    ((uint8_t)0x02) /* Output Idle state 1 (OC1N output) mask. */
#define TIM1_OISR_OIS1N_DISABLE ((uint8_t)0x00)
#define TIM1_OISR_OIS1N_ENABLE  ((uint8_t)0x02)

#define TIM1_OISR_OIS1_MASK     ((uint8_t)0x01) /* Output Idle state 1 (OC1 output) mask. */
#define TIM1_OISR_OIS1_DISABLE  ((uint8_t)0x00)
#define TIM1_OISR_OIS1_ENABLE   ((uint8_t)0x01)

//-----------------------------------------------------------------------------
// Timer 4
//
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

#define TIM4_BaseAddress        0x5340
#define TIM4                    ((stm8_tim4_t *)TIM4_BaseAddress)

#define TIM4_CR1_ARPE_MASK      ((uint8_t)0x80) /* Auto-Reload Preload Enable mask. */
#define TIM4_CR1_ARPE_DISABLE   ((uint8_t)0x00)
#define TIM4_CR1_ARPE_ENABLE    ((uint8_t)0x80)

#define TIM4_CR1_OPM_MASK       ((uint8_t)0x08) /* One Pulse Mode mask. */
#define TIM4_CR1_OPM_DISABLE    ((uint8_t)0x00)
#define TIM4_CR1_OPM_ENABLE     ((uint8_t)0x08)

#define TIM4_CR1_URS_MASK       ((uint8_t)0x04) /* Update Request Source mask. */
#define TIM4_CR1_URS_DISABLE    ((uint8_t)0x00)
#define TIM4_CR1_URS_ENABLE     ((uint8_t)0x04)

#define TIM4_CR1_UDIS_MASK      ((uint8_t)0x02) /* Update Disable mask. */
#define TIM4_CR1_UDIS_DISABLE   ((uint8_t)0x00)
#define TIM4_CR1_UDIS_ENABLE    ((uint8_t)0x02)

#define TIM4_CR1_CEN_MASK       ((uint8_t)0x01) /* Counter Enable mask. */
#define TIM4_CR1_CEN_DISABLE    ((uint8_t)0x00)
#define TIM4_CR1_CEN_ENABLE     ((uint8_t)0x01)

#define TIM4_IER_UIE_MASK       ((uint8_t)0x01) /* Update Interrupt Enable mask. */
#define TIM4_IER_UIE_DISABLE    ((uint8_t)0x00)
#define TIM4_IER_UIE_ENABLE     ((uint8_t)0x01)

#define TIM4_SR1_UIF_MASK       ((uint8_t)0x01) /* Update Interrupt Flag mask. */
#define TIM4_SR1_UIF_CLEAR      ((uint8_t)0x00)
#define TIM4_SR1_UIF_NONE       ((uint8_t)0x00)
#define TIM4_SR1_UIF_PENDING    ((uint8_t)0x01)

#define TIM4_EGR_UG_MASK        ((uint8_t)0x01) /* Update Generation mask. */
#define TIM4_EGR_UG_DISABLE     ((uint8_t)0x00)
#define TIM4_EGR_UG_ENABLE      ((uint8_t)0x01)

#define TIM4_CNTR_CNT_MASK      ((uint8_t)0xFF) /* Counter Value (LSB) mask. */

#define TIM4_PSCR_PSC_MASK      ((uint8_t)0x07) /* Prescaler Value  mask. */
#define TIM4_PSCR_DIV1          ((uint8_t)0x00)
#define TIM4_PSCR_DIV2          ((uint8_t)0x01)
#define TIM4_PSCR_DIV4          ((uint8_t)0x02)
#define TIM4_PSCR_DIV8          ((uint8_t)0x03)
#define TIM4_PSCR_DIV16         ((uint8_t)0x04)
#define TIM4_PSCR_DIV32         ((uint8_t)0x05)
#define TIM4_PSCR_DIV64         ((uint8_t)0x06)
#define TIM4_PSCR_DIV128        ((uint8_t)0x07)

#define TIM4_ARR_ARR_MASK       ((uint8_t)0xFF) /* Autoreload Value mask. */

//=============================================================================
// Universal Asynchronous Receiver/Transmitter
//

//-----------------------------------------------------------------------------
// Uart 1
//
typedef struct
{
    __IO uint8_t SR;    // Status register
    __IO uint8_t DR;    // Data register
    __IO uint8_t BRR1;  // Baud rate register 1
    __IO uint8_t BRR2;  // Baud rate register 2
    __IO uint8_t CR1;   // Control register 1
    __IO uint8_t CR2;   // Control register 2
    __IO uint8_t CR3;   // Control register 3
    __IO uint8_t CR4;   // Control register 4
    __IO uint8_t CR5;   // Control register 5
    __IO uint8_t GTR;   // Guard time register
    __IO uint8_t PSCR;  // Prescaler register
} stm8_uart1_t;

typedef struct
{
    __IO uint8_t SR;    // Status register
    __IO uint8_t DR;    // Data register
    __IO uint8_t BRR1;  // Baud rate register 1
    __IO uint8_t BRR2;  // Baud rate register 2
    __IO uint8_t CR1;   // Control register 1
    __IO uint8_t CR2;   // Control register 2
    __IO uint8_t CR3;   // Control register 3
    __IO uint8_t CR4;   // Control register 4
    __IO uint8_t CR5;   // Control register 5
    __IO uint8_t CR6;   // Control register 6
    __IO uint8_t GTR;   // Guard time register
    __IO uint8_t PSCR;  // Prescaler register
} stm8_uart2_t;

typedef struct
{
    __IO uint8_t SR;    // Status register
    __IO uint8_t DR;    // Data register
    __IO uint8_t BRR1;  // Baud rate register 1
    __IO uint8_t BRR2;  // Baud rate register 2
    __IO uint8_t CR1;   // Control register 1
    __IO uint8_t CR2;   // Control register 2
    __IO uint8_t CR3;   // Control register 3
    __IO uint8_t CR4;   // Control register 4
    uint8_t RESERVED;
    __IO uint8_t CR6;   // Control register 6
} stm8_uart3_t;

typedef struct
{
    __IO uint8_t SR;    // Status register
    __IO uint8_t DR;    // Data register
    __IO uint8_t BRR1;  // Baud rate register 1
    __IO uint8_t BRR2;  // Baud rate register 2
    __IO uint8_t CR1;   // Control register 1
    __IO uint8_t CR2;   // Control register 2
    __IO uint8_t CR3;   // Control register 3
    __IO uint8_t CR4;   // Control register 4
    __IO uint8_t CR5;   // Control register 5
    __IO uint8_t CR6;   // Control register 6
    __IO uint8_t GTR;   // Guard time register
    __IO uint8_t PSCR;  // Prescaler register
} stm8_uart4_t;

typedef struct
{
    __IO uint8_t SR;    // Status register
    __IO uint8_t DR;    // Data register
    __IO uint8_t BRR1;  // Baud rate register 1
    __IO uint8_t BRR2;  // Baud rate register 2
    __IO uint8_t CR1;   // Control register 1
    __IO uint8_t CR2;   // Control register 2
    __IO uint8_t CR3;   // Control register 3
    __IO uint8_t CR4;   // Control register 4
    union
    {
        struct
        {
            __IO uint8_t CR5;   // Control register 5
            __IO uint8_t GTR;   // Guard time register
            __IO uint8_t PSCR;  // Prescaler register
        } uart1;
        struct
        {
            __IO uint8_t CR5;   // Control register 5
            __IO uint8_t CR6;   // Control register 6
            __IO uint8_t GTR;   // Guard time register
            __IO uint8_t PSCR;  // Prescaler register
        } uart2;
        struct
        {
            uint8_t RESERVED;
            __IO uint8_t CR6;   // Control register 6
        } uart3;
        struct
        {
            __IO uint8_t CR5;   // Control register 5
            __IO uint8_t CR6;   // Control register 6
            __IO uint8_t GTR;   // Guard time register
            __IO uint8_t PSCR;  // Prescaler register
        } uart4;
    };
} stm8_uart_t;

#define UART1_BaseAddress           0x5340
#define UART2_BaseAddress           0x5240
#define UART3_BaseAddress           0x5240
#define UART4_BaseAddress           0x5240
#define UART1                       ((stm8_uart1_t *)UART1_BaseAddress)
#define UART2                       ((stm8_uart2_t *)UART2_BaseAddress)
#define UART3                       ((stm8_uart3_t *)UART3_BaseAddress)
#define UART4                       ((stm8_uart4_t *)UART4_BaseAddress)

#define UARTx_SR_TXE_MASK           ((uint8_t)0x80)     // Transmit data register empty
#define UARTx_SR_TXE_NOTREADY       ((uint8_t)0x00)
#define UARTx_SR_TXE_READY          ((uint8_t)0x80)

#define UARTx_SR_TC_MASK            ((uint8_t)0x40)     // Transmission complete
#define UARTx_SR_TC_NOTCOMPLETE     ((uint8_t)0x00)
#define UARTx_SR_TC_COMPLETE        ((uint8_t)0x40)

#define UARTx_SR_RXNE_MASK          ((uint8_t)0x20)     // Read data register not empty
#define UARTx_SR_RXNE_NOTREADY      ((uint8_t)0x00)
#define UARTx_SR_RXNE_READY         ((uint8_t)0x20)

#define UARTx_SR_IDLE_MASK          ((uint8_t)0x10)     // IDLE line detected
#define UARTx_SR_IDLE_NOTDETECTED   ((uint8_t)0x00)
#define UARTx_SR_IDLE_DETECTED      ((uint8_t)0x10)

#define UARTx_SR_OR_MASK            ((uint8_t)0x08)     // Overrun error
#define UARTx_SR_OR_NONE            ((uint8_t)0x00)
#define UARTx_SR_OR_ERROR           ((uint8_t)0x08)

#define UARTx_SR_NF_MASK            ((uint8_t)0x04)     // Noise flag
#define UARTx_SR_NF_NOTDECTED       ((uint8_t)0x00)
#define UARTx_SR_NF_DETECTED        ((uint8_t)0x04)

#define UARTx_SR_FE_MASK            ((uint8_t)0x02)     // Framing error
#define UARTx_SR_FE_NOTDECTED       ((uint8_t)0x00)
#define UARTx_SR_FE_DETECTED        ((uint8_t)0x02)

#define UARTx_SR_PE_MASK            ((uint8_t)0x01)     // Parity error
#define UARTx_SR_PE_NOTDECTED       ((uint8_t)0x00)
#define UARTx_SR_PE_DETECTED        ((uint8_t)0x01)

#define UARTx_DR_MASK               ((uint8_t)0xFF)     // Data value

#define UARTx_BRR1_DIV11_4_MASK     ((uint8_t)0xFF)     // Bits 11:4
#define UARTx_BRR1_DIV11_4_SHIFT    0
#define UARTx_BRR2_DIV15_12_MASK    ((uint8_t)0xF0)     // Bits 15:12
#define UARTx_BRR2_DIV15_12_SHIFT   4
#define UARTx_BRR2_DIV3_0_MASK      ((uint8_t)0x0F)     // Bits 3:0
#define UARTx_BRR2_DIV3_0_SHIFT     0

#define UARTx_CR1_R8_MASK           ((uint8_t)0x80)     // Receive data bit 8
#define UARTx_CR1_R8_LOW            ((uint8_t)0x00)
#define UARTx_CR1_R8_HIGH           ((uint8_t)0x80)

#define UARTx_CR1_T8_MASK           ((uint8_t)0x40)     // Transmit data bit 8
#define UARTx_CR1_T8_LOW            ((uint8_t)0x00)
#define UARTx_CR1_T8_HIGH           ((uint8_t)0x40)

#define UARTx_CR1_UARTD_MASK        ((uint8_t)0x20)     // Uart disable
#define UARTx_CR1_UARTD_ENABLE      ((uint8_t)0x00)
#define UARTx_CR1_UARTD_DISABLE     ((uint8_t)0x20)

#define UARTx_CR1_M_MASK            ((uint8_t)0x10)     // Word length
#define UARTx_CR1_M_8BIT            ((uint8_t)0x00)
#define UARTx_CR1_M_9BIT            ((uint8_t)0x10)

#define UARTx_CR1_WAKE_MASK         ((uint8_t)0x08)     // Wakeup method
#define UARTx_CR1_WAKE_IDLE         ((uint8_t)0x00)
#define UARTx_CR1_WAKE_ADDRESS      ((uint8_t)0x08)

#define UARTx_CR1_PCEN_MASK         ((uint8_t)0x04)     // Parity control enable
#define UARTx_CR1_PCEN_DISABLE      ((uint8_t)0x00)
#define UARTx_CR1_PCEN_ENABLE       ((uint8_t)0x04)

#define UARTx_CR1_PS_MASK           ((uint8_t)0x02)     // Parity selection
#define UARTx_CR1_PS_EVEN           ((uint8_t)0x00)
#define UARTx_CR1_PS_ODD            ((uint8_t)0x02)

#define UARTx_CR1_PIEN_MASK         ((uint8_t)0x01)     // Parity interrupt enable
#define UARTx_CR1_PIEN_DISABLE      ((uint8_t)0x00)
#define UARTx_CR1_PIEN_ENABLE       ((uint8_t)0x01)

#define UARTx_CR2_TIEN_MASK         ((uint8_t)0x80)     // Transmitter interrupt enable
#define UARTx_CR2_TIEN_DISABLE      ((uint8_t)0x00)
#define UARTx_CR2_TIEN_ENABLE       ((uint8_t)0x80)

#define UARTx_CR2_TCIEN_MASK        ((uint8_t)0x40)     // Transmission complete interrupt enable
#define UARTx_CR2_TCIEN_DISABLE     ((uint8_t)0x00)
#define UARTx_CR2_TCIEN_ENABLE      ((uint8_t)0x40)

#define UARTx_CR2_RIEN_MASK         ((uint8_t)0x20)     // Receiver interrupt enable
#define UARTx_CR2_RIEN_DISABLE      ((uint8_t)0x00)
#define UARTx_CR2_RIEN_ENABLE       ((uint8_t)0x20)

#define UARTx_CR2_ILIEN_MASK        ((uint8_t)0x10)     // IDLE line interrupt enable
#define UARTx_CR2_ILIEN_DISABLE     ((uint8_t)0x00)
#define UARTx_CR2_ILIEN_ENABLE      ((uint8_t)0x10)

#define UARTx_CR2_TEN_MASK          ((uint8_t)0x08)     // Transmitter enable
#define UARTx_CR2_TEN_DISABLE       ((uint8_t)0x00)
#define UARTx_CR2_TEN_ENABLE        ((uint8_t)0x08)

#define UARTx_CR2_REN_MASK          ((uint8_t)0x04)     // Receiver enable
#define UARTx_CR2_REN_DISABLE       ((uint8_t)0x00)
#define UARTx_CR2_REN_ENABLE        ((uint8_t)0x04)

#define UARTx_CR2_RWU_MASK          ((uint8_t)0x02)     // Receiver wakeup
#define UARTx_CR2_RWU_ACTIVE        ((uint8_t)0x00)
#define UARTx_CR2_RWU_MUTE          ((uint8_t)0x02)

#define UARTx_CR2_SBK_MASK          ((uint8_t)0x01)     // Send break
#define UARTx_CR2_SBK_NONE          ((uint8_t)0x00)
#define UARTx_CR2_SBK_TRANSMIT      ((uint8_t)0x01)

#define UARTx_CR3_LINEN_MASK        ((uint8_t)0x40)     // LIN mode enable
#define UARTx_CR3_LINEN_DISABLE     ((uint8_t)0x00)
#define UARTx_CR3_LINEN_ENABLE      ((uint8_t)0x40)

#define UARTx_CR3_STOP_MASK         ((uint8_t)0x30)     // STOP bits
#define UARTx_CR3_STOP_1BIT         ((uint8_t)0x00)
#define UARTx_CR3_STOP_2BIT         ((uint8_t)0x20)
#define UARTx_CR3_STOP_1_5BIT       ((uint8_t)0x30)

#define UARTx_CR3_CLKEN_MASK        ((uint8_t)0x08)     // Clock enable (not UART3)
#define UARTx_CR3_CLKEN_DISABLE     ((uint8_t)0x00)
#define UARTx_CR3_CLKEN_ENABLE      ((uint8_t)0x08)

#define UARTx_CR3_CPOL_MASK         ((uint8_t)0x04)     // Clock polarity (not UART3)
#define UARTx_CR3_CPOL_LOW          ((uint8_t)0x00)
#define UARTx_CR3_CPOL_HIGH         ((uint8_t)0x04)

#define UARTx_CR3_CPHA_MASK         ((uint8_t)0x02)     // Clock phase (not UART3)
#define UARTx_CR3_CPHA_FIRST        ((uint8_t)0x00)
#define UARTx_CR3_CPHA_SECOND       ((uint8_t)0x02)

#define UARTx_CR3_LBCL_MASK         ((uint8_t)0x01)     // Last bit clock pulse (not UART3)
#define UARTx_CR3_LBCL_NOTOUTPUT    ((uint8_t)0x00)
#define UARTx_CR3_LBCL_OUTPUT       ((uint8_t)0x01)

#define UARTx_CR4_LBDIEN_MASK       ((uint8_t)0x40)     // LIN break detection interrupt enable
#define UARTx_CR4_LBDIEN_DISABLE    ((uint8_t)0x00)
#define UARTx_CR4_LBDIEN_ENABLE     ((uint8_t)0x40)

#define UARTx_CR4_LBDL_MASK         ((uint8_t)0x20)     // LIN break detection length
#define UARTx_CR4_LBDL_10BIT        ((uint8_t)0x00)
#define UARTx_CR4_LBDL_11BIT        ((uint8_t)0x20)

#define UARTx_CR4_LBDF_MASK         ((uint8_t)0x10)     // LIN break detection flag
#define UARTx_CR4_LBDF_NOTDETECTED  ((uint8_t)0x00)
#define UARTx_CR4_LBDF_DETECTED     ((uint8_t)0x10)

#define UARTx_CR4_ADD_MASK          ((uint8_t)0x0F)     // Address of the uart node

#define UARTx_CR5_SCEN_MASK         ((uint8_t)0x20)     // Smartcard mode enable (not UART3)
#define UARTx_CR5_SCEN_DISABLE      ((uint8_t)0x00)
#define UARTx_CR5_SCEN_ENABLE       ((uint8_t)0x20)

#define UARTx_CR5_NACK_MASK         ((uint8_t)0x10)     // Smartcard NACK enable (not UART3)
#define UARTx_CR5_NACK_DISABLE      ((uint8_t)0x00)
#define UARTx_CR5_NACK_ENABLE       ((uint8_t)0x10)

#define UARTx_CR5_HDSEL_MASK        ((uint8_t)0x08)     // Half-duplex selection (not UART2 or UART3)
#define UARTx_CR5_HDSEL_DISABLE     ((uint8_t)0x00)
#define UARTx_CR5_HDSEL_ENABLE      ((uint8_t)0x08)

#define UARTx_CR5_IRLP_MASK         ((uint8_t)0x04)     // IrDA low power (not UART3)
#define UARTx_CR5_IRLP_NORMAL       ((uint8_t)0x00)
#define UARTx_CR5_IRLP_LOWPOWER     ((uint8_t)0x04)

#define UARTx_CR5_IREN_MASK         ((uint8_t)0x02)     // IrDA mode enable (not UART3)
#define UARTx_CR5_IREN_DISABLE      ((uint8_t)0x00)
#define UARTx_CR5_IREN_ENABLE       ((uint8_t)0x02)

#define UARTx_CR6_LDUM_MASK         ((uint8_t)0x80)     // LIN divider update method (not UART1)
#define UARTx_CR6_LDUM_IMMEDIATE    ((uint8_t)0x00)
#define UARTx_CR6_LDUM_NEXTBYTE     ((uint8_t)0x80)

#define UARTx_CR6_LSLV_MASK         ((uint8_t)0x20)     // LIN slave enable (not UART1)
#define UARTx_CR6_LSLV_MASTER       ((uint8_t)0x00)
#define UARTx_CR6_LSLV_SLAVE        ((uint8_t)0x20)

#define UARTx_CR6_LASE_MASK         ((uint8_t)0x10)     // LIN automatic resynchronisation enable (not UART1)
#define UARTx_CR6_LASE_DISABLE      ((uint8_t)0x00)
#define UARTx_CR6_LASE_ENABLE       ((uint8_t)0x10)

#define UARTx_CR6_LHDIEN_MASK       ((uint8_t)0x04)     // LIN header detection interrupt enable (not UART1)
#define UARTx_CR6_LHDIEN_DISABLE    ((uint8_t)0x00)
#define UARTx_CR6_LHDIEN_ENABLE     ((uint8_t)0x04)

#define UARTx_CR6_LHDF_MASK         ((uint8_t)0x02)     // LIN header detection flag (not UART1)
#define UARTx_CR6_LHDF_CLEAR        ((uint8_t)0x00)
#define UARTx_CR6_LHDF_NOTDETECTED  ((uint8_t)0x00)
#define UARTx_CR6_LHDF_DETECTED     ((uint8_t)0x02)

#define UARTx_CR6_LSF_MASK          ((uint8_t)0x01)     // LIN sync field (not UART1)
#define UARTx_CR6_LSF_CLEAR         ((uint8_t)0x00)
#define UARTx_CR6_LSF_NOTSYNC       ((uint8_t)0x00)
#define UARTx_CR6_LSF_SYNC          ((uint8_t)0x01)

#define UARTx_GTR_MASK              ((uint8_t)0xFF)     // Guard time value (not UART3)

#define UARTx_PSCR_MASK             ((uint8_t)0xFF)     // Prescaler value (not UART3)

//=============================================================================
// System clock functions
//
// Only one of the following function should be used. The other can either be
// conditionally compiled out or the compiler might optimise them out if they
// aren't used.
//

uint32_t sysclock;

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

    sysclock = 128000;
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

    sysclock = 16000000;
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

    sysclock = 8000000;
}

uint32_t SysClock_GetClockFreq(void)
{
    return sysclock;
}

//=============================================================================
// Uart functions
//
// Note: Some of the function assume that the serial ports have the certain
// registers at the same offset. For the registers upto CR4, this is true for
// all four uarts.
//

uint8_t tx_buffer[32];
circular_buffer_t tx_cirbuf = { 0, 0, 31, tx_buffer };

typedef enum
{
    PARITY_NONE,
    PARITY_ODD,
    PARITY_EVEN
} uart_parity_t;

typedef enum
{
    STOPBITS_1,
    STOPBITS_1_5,
    STOPBITS_2
} uart_stopbits_t;

typedef enum
{
    WORDLENGTH_8,
    WORDLENGTH_9
} uart_wordlength_t;

//-----------------------------------------------------------------------------
// Calculate the values for the BRR registers
//
// Calculates the divider as fMASTER/BAUD. Eg. 16Mhz/9600=1666.666
// Then places nibbles of the resultant value in the BBR1 and BBR2 registers
// as follows:
//  Divider bit    15 14 13 12 11 10  9  8  7  6  5  4  3  2  1  0
//  BBR register    2  2  2  2  1  1  1  1  1  1  1  1  2  2  2  2
//  BBR bit         7  6  5  4  7  6  5  4  3  2  1  0  3  2  1  0
//
void Uart_CalcBRR(uint32_t baud, uint8_t *brr1, uint8_t *brr2)
{
    uint16_t d = SysClock_GetClockFreq() / baud;
    *brr2 = ((d >> 0) & UARTx_BRR2_DIV3_0_MASK) | ((d >> 8) & UARTx_BRR2_DIV15_12_MASK);
    *brr1 = (d >> 4) & UARTx_BRR1_DIV11_4_MASK;
}

//-----------------------------------------------------------------------------
// Configure uart2 with most common protocol settings
//
// Rx and TX at 9600 baud, 8 databits, no parity, 1 stop bit
//
void Uart2_Config9600_8N1(void)
{
    UART2->CR2 = (UART2->CR2 & ~(UARTx_CR2_TEN_MASK | UARTx_CR2_REN_MASK)) | UARTx_CR2_TEN_DISABLE | UARTx_CR2_REN_DISABLE;
    UART2->CR1 = (UART2->CR1 & ~UARTx_CR1_M_MASK) | UARTx_CR1_M_8BIT;
    UART2->CR1 = (UART2->CR1 & ~UARTx_CR1_PCEN_MASK) | UARTx_CR1_PCEN_DISABLE;
    UART2->CR3 = (UART2->CR3 & ~UARTx_CR3_STOP_MASK) | UARTx_CR3_STOP_1BIT;
    Uart_CalcBRR(9600, &UART2->BRR1, &UART2->BRR2);
    UART2->CR3 = (UART2->CR3 & ~UARTx_CR3_CLKEN_MASK) | UARTx_CR3_CLKEN_DISABLE;
    UART2->CR2 = (UART2->CR2 & ~(UARTx_CR2_TEN_MASK | UARTx_CR2_REN_MASK)) | UARTx_CR2_TEN_ENABLE | UARTx_CR2_REN_ENABLE;
}

//-----------------------------------------------------------------------------
// Disable transmit interrupts
//
inline void Uart2_DisableTxInterrupts(void)
{
    UART2->CR2 = (UART2->CR2 & ~UARTx_CR2_TIEN_MASK) | UARTx_CR2_TIEN_DISABLE;
}

//-----------------------------------------------------------------------------
// Enable transmit interrupts
//
inline void Uart2_EnableTxInterrupts(void)
{
    UART2->CR2 = (UART2->CR2 & ~UARTx_CR2_TIEN_MASK) | UARTx_CR2_TIEN_ENABLE;
}

//-----------------------------------------------------------------------------
// Disable receiver interrupts
//
inline void Uart2_DisableRxInterrupts(void)
{
    UART2->CR2 = (UART2->CR2 & ~UARTx_CR2_RIEN_MASK) | UARTx_CR2_RIEN_DISABLE;
}

//-----------------------------------------------------------------------------
// Enable receiver interrupts
//
inline void Uart2_EnableRxInterrupts(void)
{
    UART2->CR2 = (UART2->CR2 & ~UARTx_CR2_RIEN_MASK) | UARTx_CR2_RIEN_ENABLE;
}

//-----------------------------------------------------------------------------
// Send a byte, waiting till it can be sent
//
void Uart2_BlockingSendByte(uint8_t byte)
{
    while ((UART2->SR & UARTx_SR_TXE_MASK) == UARTx_SR_TXE_NOTREADY)
    {
    }
    UART2->DR = byte;
}

//-----------------------------------------------------------------------------
// Send a byte and return immediately (so long as circular buffer is not full)
//
// If this is the first byte in a packet, then it will be placed in the shift
// register. If it's the second byte, ie. one is still being sent but interrupts
// haven't been enabled then the byte is placed in the circular buffer and
// interrupts enabled so that when the first byte has been sent, the interrupt
// will use the byte in the buffer. In all other cases, the byte is placed in
// the circular buffer so long as it's not full. If it's full, then the function
// returns immediately.
//
bool Uart2_SendByte(uint8_t byte) __critical
{
    if ((UART2->CR2 & UARTx_CR2_TIEN_MASK) == UARTx_CR2_TIEN_ENABLE)
    {
        // Interrupts enabled, so buffer being used
        if (CircBuf_IsFull(&tx_cirbuf))
        {
            // However, buffer is full so can't send
            return false;
        }
        CircBuf_Put(&tx_cirbuf, byte);
    }
    else
    {
        if ((UART2->SR & UARTx_SR_TXE_MASK) == UARTx_SR_TXE_READY)
        {
            // No interrupts, shift register is OK to write, just send it
            UART2->DR = byte;
        }
        else
        {
            // No interrupts, but shift register still has something in it, use buffer
            CircBuf_Put(&tx_cirbuf, byte);
            Uart2_EnableTxInterrupts();
        }
    }
    return true;
}

//-----------------------------------------------------------------------------
// Send a string
//
void Uart2_SendString(const char *str)
{
    while (*str)
    {
        while (!Uart2_SendByte(*str))
        {
        }
        ++str;
    }
}

//-----------------------------------------------------------------------------
// Interrupt handler for the transmission
//
void UART2_TX_IRQHandler(void) __interrupt(20)
{
    //if (!CircBuf_IsEmpty(&tx_cirbuf))
    {
        UART2->DR = CircBuf_Get(&tx_cirbuf);  // Clears TXE flag
    }
    if (CircBuf_IsEmpty(&tx_cirbuf))
    {
        Uart2_DisableTxInterrupts();
    }
}

//-----------------------------------------------------------------------------
// Interrupt handler for the receiver
//
void UART2_RX_IRQHandler(void) __interrupt(21)
{
    (void)UART2->DR;      // Clears RXNE flag
}

//=============================================================================
// Timer functions
//
// Initialise timer 1 with a frequency of 50Khz
// (16000000 / 1) / (319 + 1) = 0.00002s = 50Khz
//

#define TIM1_PERIOD         320
#define TIM1_CH3_DUTY       ((uint16_t)((TIM1_PERIOD * 0) / 100))

//-----------------------------------------------------------------------------
// Set the auto-reload value
//
// Note that High register must be set first.
//
void Tim1_SetAutoReload(uint16_t period)
{
    TIM1->ARRH = ((period - 1) >> 8) & 0xFF;
    TIM1->ARRL = ((period - 1) >> 0) & 0xFF;
}

//-----------------------------------------------------------------------------
// Set the prescaler value
//
// Note that High register must be set first.
//
void Tim1_SetPrescaler(uint16_t division)
{
    TIM1->PSCRH = ((division - 1) >> 8) & 0xFF;
    TIM1->PSCRL = ((division - 1) >> 0) & 0xFF;
}

//-----------------------------------------------------------------------------
// Set the counter value
//
// Note that High register must be set first.
//
void Tim1_SetCounter(uint16_t counter)
{
    TIM1->CCR3H = (counter >> 8) & 0xFF;
    TIM1->CCR3L = (counter >> 0) & 0xFF;
}

//-----------------------------------------------------------------------------
// Configure the TIM1 timer
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

//-----------------------------------------------------------------------------
// Configure the TIM4 timer
//
void Tim4_Config(uint8_t prescaler, uint8_t reload)
{
    TIM4->CR1 = (TIM4->CR1 & ~TIM4_CR1_CEN_MASK) | TIM4_CR1_CEN_DISABLE;
    TIM4->PSCR = prescaler;
    TIM4->ARR = reload;
    TIM4->SR1 = (TIM4->SR1 & ~TIM4_SR1_UIF_MASK) | TIM4_SR1_UIF_CLEAR;
    TIM4->IER = (TIM4->IER & ~TIM4_IER_UIE_MASK) | TIM4_IER_UIE_ENABLE;
    TIM4->CNTR = 0;
    TIM4->CR1 = (TIM4->CR1 & ~(TIM4_CR1_ARPE_MASK | TIM4_CR1_CEN_MASK)) | TIM4_CR1_ARPE_ENABLE | TIM4_CR1_CEN_ENABLE;
}

//=============================================================================
// Beeper functions
//

typedef enum
{
    BEEP_1KHZ = 0x00,
    BEEP_2KHZ = 0x40,
    BEEP_4KHZ = 0x80
} beep_freq_t;

//=============================================================================
// Set frequency of beep
//
void Beep_SetFrequency(beep_freq_t freq)
{
    if ((BEEP->CSR & BEEP_CSR_DIV_MASK) == BEEP_CSR_DIV_RESET)
    {
        // Set default prescaler value if not already set
        BEEP->CSR = (BEEP->CSR & BEEP_CSR_DIV_MASK) | BEEP_CSR_DIV_13;
    }
    BEEP->CSR = (BEEP->CSR & BEEP_CSR_SEL_MASK) | freq;
}

//=============================================================================
// Set prescaler divider based on LSI frequency
//
// Note on the calculation:
//   lsi_freq is the frequency of the LSI RC clock as measured by a timer
//
// A is the integer part of LSIFreqkHz/4 and x the decimal part.
// x <= A/(1+2A) is equivalent to A >= x(1+2A) and also to 4A >= 4x(1+2A) [F1]
// but we know that A + x = LSIFreqkHz/4 ==> 4x = LSIFreqkHz-4A
// so [F1] can be written :
// 4A >= (LSIFreqkHz-4A)(1+2A)
//
void Beep_Calibrate(uint32_t lsi_freq)
{
    uint16_t khz;
    uint16_t div8;
    uint8_t div;

    khz = (uint16_t)(lsi_freq / 1000);

    // Calculation of BEEPER calibration value
    BEEP->CSR &= (uint8_t)(~BEEP_CSR_DIV_MASK); /* Clear bits */

    div8 = khz / 8;
    if ((8U * div8) >= ((khz - (8U * div8)) * (1U + (2U * div8))))
    {
        div = (uint8_t)(div8 - 2U);
    }
    else
    {
        div = (uint8_t)(div8 - 1U);
    }
    BEEP->CSR = (BEEP->CSR & BEEP_CSR_DIV_MASK) | div;
}

//=============================================================================
// Turn on beeper
//
void Beep_On(void)
{
    BEEP->CSR = (BEEP->CSR & ~BEEP_CSR_EN_MASK) | BEEP_CSR_EN_ENABLE;
}

//=============================================================================
// Turn off beeper
//
void Beep_Off(void)
{
    BEEP->CSR = (BEEP->CSR & ~BEEP_CSR_EN_MASK) | BEEP_CSR_EN_DISABLE;
}

//=============================================================================
// Toggle state of beeper
//
void Beep_Toggle(void)
{
    BEEP->CSR = (BEEP->CSR & ~BEEP_CSR_EN_MASK) | (~BEEP->CSR & BEEP_CSR_EN_MASK);
}

//=============================================================================
// Auto wakeup functions
//

// High nibble is APR value, low nibble is TBR value
typedef enum
{
    AWU_PERIOD_250us    = 0x0000,
    AWU_PERIOD_500us    = 0x1E01,
    AWU_PERIOD_1ms      = 0x1E02,
    AWU_PERIOD_2ms      = 0x1E03,
    AWU_PERIOD_4ms      = 0x1E04,
    AWU_PERIOD_8ms      = 0x1E05,
    AWU_PERIOD_16ms     = 0x1E06,
    AWU_PERIOD_32ms     = 0x1E07,
    AWU_PERIOD_64ms     = 0x1E08,
    AWU_PERIOD_128ms    = 0x1E09,
    AWU_PERIOD_256ms    = 0x1E0A,
    AWU_PERIOD_512ms    = 0x1E0B,
    AWU_PERIOD_1s       = 0x3D0C,
    AWU_PERIOD_2s       = 0x170C,
    AWU_PERIOD_12s      = 0x170F,
    AWU_PERIOD_30s      = 0x3E0F
} awu_period_t;

void Awu_SetPeriod(awu_period_t period)
{
    AWU->CSR = (AWU->CSR & ~AWU_CSR_AWEN_MASK) | AWU_CSR_AWEN_ENABLE;
    AWU->TBR = (AWU->TBR & ~AWU_TBR_MASK) | ((period >> 0) & 0xFF);
    AWU->APR = (AWU->APR & ~AWU_APR_DIV_MASK) | ((period >> 8) & 0xFF);
}

void Awu_SetIdleMode(void)
{
    AWU->CSR = (AWU->CSR & ~AWU_CSR_AWEN_MASK) | AWU_CSR_AWEN_DISABLE;
    AWU->TBR = (AWU->TBR & ~AWU_TBR_MASK) | AWU_TBR_NONE;
}

void Awu_Disable(void)
{
    AWU->CSR = (AWU->CSR & ~AWU_CSR_AWEN_MASK) | AWU_CSR_AWEN_DISABLE;
}

void Awu_Enable(void)
{
    AWU->CSR = (AWU->CSR & ~ AWU_CSR_AWEN_MASK) | AWU_CSR_AWEN_ENABLE;
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
// Setup 8-bit basic timer as 1ms system tick using interrupts to update the tick counter
//
// With 16Mhz HSI prescaler is 128 and reload value is 124. freq = 16000000/128 is 125Khz, therefore reload value is 125 - 1 or 124. Period is 125/125000 which is 0.001s or 1ms
// With 8Mhz HSE prescaler is 64 and reload value is 124. freq is 8000000/64 is 125Khz.
// With 128Khz LSI prescaler is 16 and reload value is 128. freq is 128000/16 is 8Khz.
//
void Systick_Init(void)
{
    //Tim4_Config(TIM4_PSCR_DIV1, 128);
    //Tim4_Config(TIM4_PSCR_DIV64, 124);
    Tim4_Config(TIM4_PSCR_DIV128, 124);
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
    TIM4->SR1 = (TIM4->SR1 & ~TIM4_SR1_UIF_MASK) | TIM4_SR1_UIF_CLEAR;
}

//=============================================================================
// GPIO functions
//

//-----------------------------------------------------------------------------
// Configure the GPIOs
//
void Gpio_Config(void)
{
    GPIOD->DDR |= GPIO_DDR_7_OUTPUT;
    GPIOD->CR1 |= GPIO_CR1_7_OUTPUT_PUSHPULL;
    GPIOD->CR2 |= GPIO_CR2_7_OUTPUT_2MHZ;
}

//-----------------------------------------------------------------------------
// Set the output high
//
void Gpio_TurnOnLED(void)
{
    GPIOD->ODR = (GPIOD->ODR & ~GPIO_ODR_7_MASK) | GPIO_ODR_7_HIGH;
}

//-----------------------------------------------------------------------------
// Set the output low
//
void Gpio_TurnOffLED(void)
{
    GPIOD->ODR = (GPIOD->ODR & ~GPIO_ODR_7_MASK) | GPIO_ODR_7_LOW;
}

//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
// Main super loop
//
void main(void)
{
    uint16_t value = 0;
    uint16_t timer = 0;
    uint16_t fader = 0;
    uint8_t flash = 0;
    uint8_t up = 0;
    uint16_t transmitter = 0;
    uint8_t serdata = 32;

    SysClock_HSI();
    Systick_Init();
    Tim1_Config();
    Gpio_Config();
    Uart2_Config9600_8N1();

    enableInterrupts;

    //CircBuf_Init(&tx_cirbuf, tx_buffer, 32);

    for (;;)
    {
        // Flash the LED if PD7 is connected
#ifdef FLASHER
        if (flash)
        {
            if (Systick_Timeout(&timer, 100))
            {
                Gpio_TurnOnLED();
                flash = 0;
            }
        }
        else
        {
            if (Systick_Timeout(&timer, 400))
            {
                Gpio_TurnOffLED();
                flash = 1;
            }
        }
#endif

        // Fade the LED in and out if PC3 is connected
#ifdef FADER
        if (Systick_Timeout(&fader, 10))
        {
            Tim1_SetCounter(value);
            if (up)
            {
                value += 10;
                if (value > TIM1_PERIOD)
                {
                    up = 0;
                }
            }
            else
            {
                if (value > 0)
                {
                    value -= 10;
                }
                else
                {
                    up = 1;
                }
            }
        }
#endif
        // Output a message using interrupts and a circular buffer
#ifdef SERIALIZER
        if (Systick_Timeout(&transmitter, 500))
        {
            Uart2_SendString("The quick brown fox jumps over the lazy dog Pack my box with five dozen liquor jugs 0123456789 ABCDEFGHIJKLMNOPQRSTUVWXYZ abcdefghijklmnopqrstuvwxyz\r\n");
        }

        Tim1_SetCounter((TIM1_PERIOD * 100) / (10000 / ((CircBuf_Used(&tx_cirbuf) * 100) / 32)));
#endif
    }
}
