/*
 * main.c
 *
 * Simple blinking LED program for the STM8S105K4 development board from Hobbytronics.
 * Well a little bit more involved that a simple blinky as it includes proper timing
 * and other stuff to fully experience the development board and the capabilities of
 * the STM8 microcontroller.
 *
 * It's simple in that it doesn't have any reliance on the Standard Peripheral Library
 * or anything complicated and buggy like that.
 *
 * The command to compile it is:
 *   sdcc -mstm8 --std-sdcc99 --fverbose-asm --opt-code-size main.c
 *
 * MIT License
 *
 * Copyright (c) 2018 Jon Axtell
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
 * of the Software, and to permit persons to whom the Software is furnished to do
 * so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,
 * INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
 * PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
 * OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#if defined __SDCC__
#pragma disable_warning 126     // Disable unreachable code due to optimisation
#endif

#include <stdarg.h>             // For va_list macros in printf style output function

// Some preprocessor macros for conditional compilation of various features
//#define FLASHER
#define FADER
//#define SERIALIZER
//#define BEEPER
#define SQUARER

// Some basic macros to make life easy when using different compilers
#if defined __IAR_SYSTEMS_ICC__
// IAR
#define __PACKED                __attribute((packed))
#define __IO                    volatile
#define CRITICAL
#define INTERRUPT(f, x)         __interrupt void f(void)
#define disableInterrupts()     __asm("sim")
#define enableInterrupts()      __asm("rim")
#else
// SDCC
#define __PACKED
#define __IO                    volatile
#define CRITICAL                __critical
#define INTERRUPT(f, x)         void f(void) __interrupt(x)
#define disableInterrupts()     __asm sim __endasm
#define enableInterrupts()      __asm rim __endasm
#endif

// Some basic types
#define NULL    ((void *)0)
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
typedef const char * ptr_t;

// Useful macros
// Swap two bytes using the XOR method (requires the two bytes to be different otherwise zero is the result)
#define swap_bytes(b1, b2)      \
    if ((b1) != (b2))           \
    {                           \
        b1 = (b1) ^ (b2);       \
        b2 = (b2) ^ (b1);       \
        b1 = (b1) ^ (b2);       \
    }

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

//=============================================================================
// Returns how much of buffer is used as a percentage
//
uint16_t CircBuf_PercentUsed(circular_buffer_t *buf)
{
    return (10000 / ((CircBuf_Used(buf) * 100) / (buf->size + 1)));
}

//#############################################################################
// The peripherals in the STM8
//

//=============================================================================
// Unique ID
//
typedef struct
{
    const uint8_t X[2];
    const uint8_t Y[2];
    const uint8_t WAFER;
    const uint8_t LOT[7];
} stm8_uid_t;

#define UID_BaseAddress         0x48CD
#define UID                     ((stm_uid_t *)UID_BaseAddress)


//=============================================================================
// System clock
//
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

#define CLK_BaseAddress             0x50C0
#define CLK                         ((stm8_clk_t *)CLK_BaseAddress)

#define CLK_ICKR_SWUAH_MASK         ((uint8_t)0x20) /* Slow Wake-up from Active Halt/Halt modes */
#define CLK_ICKR_SWUAH_DISABLE      ((uint8_t)0x00)
#define CLK_ICKR_SWUAH_ENABLE       ((uint8_t)0x20)

#define CLK_ICKR_LSIRDY_MASK        ((uint8_t)0x10) /* Low speed internal oscillator ready */
#define CLK_ICKR_LSIRDY_NOTREADY    ((uint8_t)0x00)
#define CLK_ICKR_LSIRDY_READY       ((uint8_t)0x10)

#define CLK_ICKR_LSIEN_MASK         ((uint8_t)0x08) /* Low speed internal RC oscillator enable */
#define CLK_ICKR_LSIEN_DISABLE      ((uint8_t)0x00)
#define CLK_ICKR_LSIEN_ENABLE       ((uint8_t)0x08)

#define CLK_ICKR_FHWU_MASK          ((uint8_t)0x04) /* Fast Wake-up from Active Halt/Halt mode */
#define CLK_ICKR_FHWU_DISABLE       ((uint8_t)0x00)
#define CLK_ICKR_FHWU_ENABLE        ((uint8_t)0x04)

#define CLK_ICKR_HSIRDY_MASK        ((uint8_t)0x02) /* High speed internal RC oscillator ready */
#define CLK_ICKR_HSIRDY_NOTREADY    ((uint8_t)0x00)
#define CLK_ICKR_HSIRDY_READY       ((uint8_t)0x02)

#define CLK_ICKR_HSIEN_MASK         ((uint8_t)0x01) /* High speed internal RC oscillator enable */
#define CLK_ICKR_HSIEN_DISABLE      ((uint8_t)0x00)
#define CLK_ICKR_HSIEN_ENABLE       ((uint8_t)0x01)

#define CLK_ECKR_HSERDY_MASK        ((uint8_t)0x02) /* High speed external crystal oscillator ready */
#define CLK_ECKR_HSERDY_NOTREADY    ((uint8_t)0x00)
#define CLK_ECKR_HSERDY_READY       ((uint8_t)0x02)

#define CLK_ECKR_HSEEN_MASK         ((uint8_t)0x01) /* High speed external crystal oscillator enable */
#define CLK_ECKR_HSEEN_DISABLE      ((uint8_t)0x01)
#define CLK_ECKR_HSEEN_ENABLE       ((uint8_t)0x01)

#define CLK_CMSR_CKM_MASK           ((uint8_t)0xFF) /* Clock master status bits */
#define CLK_CMSR_CKM_HSI            ((uint8_t)0xE1) /* Clock Source HSI. */
#define CLK_CMSR_CKM_LSI            ((uint8_t)0xD2) /* Clock Source LSI. */
#define CLK_CMSR_CKM_HSE            ((uint8_t)0xB4) /* Clock Source HSE. */

#define CLK_SWR_SWI_MASK            ((uint8_t)0xFF) /* Clock master selection bits */
#define CLK_SWR_SWI_HSI             ((uint8_t)0xE1) /* Clock Source HSI. */
#define CLK_SWR_SWI_LSI             ((uint8_t)0xD2) /* Clock Source LSI. */
#define CLK_SWR_SWI_HSE             ((uint8_t)0xB4) /* Clock Source HSE. */

#define CLK_SWCR_SWIF_MASK          ((uint8_t)0x08) /* Clock switch interrupt flag */
#define CLK_SWCR_SWIF_NOTREADY      ((uint8_t)0x00) // manual
#define CLK_SWCR_SWIF_READY         ((uint8_t)0x08)
#define CLK_SWCR_SWIF_NOOCCURANCE   ((uint8_t)0x00) // auto
#define CLK_SWCR_SWIF_OCCURED       ((uint8_t)0x08)

#define CLK_SWCR_SWIEN_MASK         ((uint8_t)0x04) /* Clock switch interrupt enable */
#define CLK_SWCR_SWIEN_DISABLE      ((uint8_t)0x00)
#define CLK_SWCR_SWIEN_ENABLE       ((uint8_t)0x04)

#define CLK_SWCR_SWEN_MASK          ((uint8_t)0x02) /* Switch start/stop */
#define CLK_SWCR_SWEN_MANUAL        ((uint8_t)0x00)
#define CLK_SWCR_SWEN_AUTOMATIC     ((uint8_t)0x02)

#define CLK_SWCR_SWBSY_MASK         ((uint8_t)0x01) /* Switch busy flag*/
#define CLK_SWCR_SWBSY_NOTBUSY      ((uint8_t)0x00)
#define CLK_SWCR_SWBSY_BUSY         ((uint8_t)0x01)

#define CLK_CKDIVR_HSIDIV_MASK      ((uint8_t)0x18) /* High speed internal clock prescaler */
#define CLK_CKDIVR_HSIDIV1          ((uint8_t)0x00) /* High speed internal clock prescaler: 1 */
#define CLK_CKDIVR_HSIDIV2          ((uint8_t)0x08) /* High speed internal clock prescaler: 2 */
#define CLK_CKDIVR_HSIDIV4          ((uint8_t)0x10) /* High speed internal clock prescaler: 4 */
#define CLK_CKDIVR_HSIDIV8          ((uint8_t)0x18) /* High speed internal clock prescaler: 8 */

#define CLK_CKDIVR_CPUDIV_MASK      ((uint8_t)0x07) /* CPU clock prescaler */
#define CLK_CKDIVR_CPUDIV1          ((uint8_t)0x00) /* CPU clock division factors 1 */
#define CLK_CKDIVR_CPUDIV2          ((uint8_t)0x01) /* CPU clock division factors 2 */
#define CLK_CKDIVR_CPUDIV4          ((uint8_t)0x02) /* CPU clock division factors 4 */
#define CLK_CKDIVR_CPUDIV8          ((uint8_t)0x03) /* CPU clock division factors 8 */
#define CLK_CKDIVR_CPUDIV16         ((uint8_t)0x04) /* CPU clock division factors 16 */
#define CLK_CKDIVR_CPUDIV32         ((uint8_t)0x05) /* CPU clock division factors 32 */
#define CLK_CKDIVR_CPUDIV64         ((uint8_t)0x06) /* CPU clock division factors 64 */
#define CLK_CKDIVR_CPUDIV128        ((uint8_t)0x07) /* CPU clock division factors 128 */

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
#define BEEP_CSR_SEL_8KHZ       0x00
#define BEEP_CSR_SEL_16KHZ      0x40
#define BEEP_CSR_SEL_32KHZ      0x80

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

#define AWU_BaseAddress         0x50F0
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
// Independent Watchdog
//

typedef struct
{
    __IO uint8_t KR;    // Key register
    __IO uint8_t PR;    // Prescaler register
    __IO uint8_t RLR;   // Reload register
} stm8_iwdg_t;

#define IWDG_BaseAddress        0x50E0
#define IWDG                    ((stm8_iwdg_t *)IWDG_BaseAddress)

#define IWDG_KR_KEY_MASK        0xFF        // Key value
#define IWDG_KR_KEY_ENABLE      0xCC
#define IWDG_KR_KEY_ACCESS      0x55
#define IWDG_KR_KEY_REFRESH     0xAA

#define IWDG_PR_PR_MASK         0x07        // Prescaler divider
#define IWDG_PR_PR_DIV4         0x00        // 15.9ms with RL of 0xFF
#define IWDG_PR_PR_DIV8         0x01        // 31.9ms with RL of 0xFF
#define IWDG_PR_PR_DIV16        0x02        // 63.7ms with RL of 0xFF
#define IWDG_PR_PR_DIV32        0x03        // 127ms with RL of 0xFF
#define IWDG_PR_PR_DIV64        0x04        // 255ms with RL of 0xFF
#define IWDG_PR_PR_DIV128       0x05        // 510ms with RL of 0xFF
#define IWDG_PR_PR_DIV256       0x06        // 1.02s with RL of 0xFF

#define IWDG_RLR_RL_MASK        0xFF        // Watchdog counter reload value


//=============================================================================
// Window Watchdog
//

typedef struct
{
    __IO uint8_t CR;    // Control register
    __IO uint8_t WR;    // Window register
} stm8_wwdg_t;

#define WWDG_BaseAddress        0x50D1
#define WWDG                    ((stm8_wwdg_t *)WWDG_BaseAddress)

#define WWDG_CR_WDGA_MASK       0x80        // Activation bit
#define WWDG_CR_WDGA_DISABLE    0x00
#define WWDG_CR_WDGA_ENABLE     0x80

#define WWDG_CR_T_MASK          0x7F        // 7-bit counter
#define WWDG_CR_T_MAX           0x7F        // Bit 6 must be set to prevent a reset
#define WWDG_CR_T_MIN           0x3F        // Bit 6 clear so reset will be generated

#define WWDG_WR_W_MASK          0x7F        // 7-bit window value


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

#define TIM1_BaseAddress            0x5250
#define TIM2_BaseAddress            0x5300
#define TIM3_BaseAddress            0x5320
#define TIM1                        ((stm8_tim1_t *)TIM1_BaseAddress)

#define TIM1_CR1_ARPE_MASK          ((uint8_t)0x80) /* Auto-Reload Preload Enable mask. */
#define TIM1_CR1_ARPE_DISABLE       ((uint8_t)0x00)
#define TIM1_CR1_ARPE_ENABLE        ((uint8_t)0x80)

#define TIM1_CR1_CMS_MASK           ((uint8_t)0x60) /* Center-aligned Mode Selection mask. */
#define TIM1_CR1_CMS_EDGE           ((uint8_t)0x00)
#define TIM1_CR1_CMS_CENTER1        ((uint8_t)0x20)
#define TIM1_CR1_CMS_CENTER2        ((uint8_t)0x40)
#define TIM1_CR1_CMS_CENTER3        ((uint8_t)0x60)

#define TIM1_CR1_DIR_MASK           ((uint8_t)0x10) /* Direction mask. */
#define TIM1_CR1_DIR_UP             ((uint8_t)0x00)
#define TIM1_CR1_DIR_DOWN           ((uint8_t)0x10)

#define TIM1_CR1_OPM_MASK           ((uint8_t)0x08) /* One Pulse Mode mask. */
#define TIM1_CR1_OPM_DISABLE        ((uint8_t)0x00)
#define TIM1_CR1_OPM_ENABLE         ((uint8_t)0x08)

#define TIM1_CR1_URS_MASK           ((uint8_t)0x04) /* Update Request Source mask. */
#define TIM1_CR1_URS_ALL            ((uint8_t)0x00)
#define TIM1_CR1_URS_UPDATE         ((uint8_t)0x04)

#define TIM1_CR1_UDIS_MASK          ((uint8_t)0x02) /* Update DIsable mask. */
#define TIM1_CR1_UDIS_DISABLE       ((uint8_t)0x00)
#define TIM1_CR1_UDIS_ENABLE        ((uint8_t)0x02)

#define TIM1_CR1_CEN_MASK           ((uint8_t)0x01) /* Counter Enable mask. */
#define TIM1_CR1_CEN_DISABLE        ((uint8_t)0x00)
#define TIM1_CR1_CEN_ENABLE         ((uint8_t)0x01)

#define TIM1_CR2_MMS_MASK           ((uint8_t)0x70) /* MMS Selection mask. */
#define TIM1_CR2_MMS_RESET          ((uint8_t)0x00)
#define TIM1_CR2_MMS_ENABLE         ((uint8_t)0x10)
#define TIM1_CR2_MMS_UPDATE         ((uint8_t)0x20)
#define TIM1_CR2_MMS_OC1            ((uint8_t)0x30)
#define TIM1_CR2_MMS_OC1REF         ((uint8_t)0x40)
#define TIM1_CR2_MMS_OC2REF         ((uint8_t)0x50)
#define TIM1_CR2_MMS_OC3REF         ((uint8_t)0x60)
#define TIM1_CR2_MMS_OC4REF         ((uint8_t)0x70)

#define TIM1_CR2_COMS_MASK          ((uint8_t)0x04) /* Capture/Compare Control Update Selection mask. */
#define TIM1_CR2_COMS_COMG          ((uint8_t)0x00)
#define TIM1_CR2_COMS_ALL           ((uint8_t)0x04)

#define TIM1_CR2_CCPC_MASK          ((uint8_t)0x01) /* Capture/Compare Preloaded Control mask. */
#define TIM1_CR2_CCPC_NOTPRELOAD    ((uint8_t)0x00)
#define TIM1_CR2_CCPC_PRELOADED     ((uint8_t)0x01)

#define TIM1_SMCR_MSM_MASK          ((uint8_t)0x80) /* Master/Slave Mode mask. */
#define TIM1_SMCR_MSM_DISABLE       ((uint8_t)0x00)
#define TIM1_SMCR_MSM_ENABLE        ((uint8_t)0x80)

#define TIM1_SMCR_TS_MASK           ((uint8_t)0x70) /* Trigger Selection mask. */
#define TIM1_SMCR_TS_TIM6           ((uint8_t)0x00)  /* TRIG Input source =  TIM6 TRIG Output  */
#define TIM1_SMCR_TS_TIM5           ((uint8_t)0x30)  /* TRIG Input source =  TIM5 TRIG Output  */
#define TIM1_SMCR_TS_TI1F_ED        ((uint8_t)0x40)
#define TIM1_SMCR_TS_TI1FP1         ((uint8_t)0x50)
#define TIM1_SMCR_TS_TI2FP2         ((uint8_t)0x60)
#define TIM1_SMCR_TS_ETRF           ((uint8_t)0x70)

#define TIM1_SMCR_SMS_MASK          ((uint8_t)0x07) /* Slave Mode Selection mask. */
#define TIM1_SMCR_SMS_DISABLE       ((uint8_t)0x00)
#define TIM1_SMCR_SMS_ENCODER1      ((uint8_t)0x01)
#define TIM1_SMCR_SMS_ENCODER2      ((uint8_t)0x02)
#define TIM1_SMCR_SMS_ENCODER3      ((uint8_t)0x03)
#define TIM1_SMCR_SMS_RESET         ((uint8_t)0x04)
#define TIM1_SMCR_SMS_GATED         ((uint8_t)0x05)
#define TIM1_SMCR_SMS_TRIGGER       ((uint8_t)0x06)
#define TIM1_SMCR_SMS_EXTERNAL1     ((uint8_t)0x07)

#define TIM1_ETR_ETP_MASK           ((uint8_t)0x80) /* External Trigger Polarity mask. */
#define TIM1_ETR_ETP_NOTINVERTED    ((uint8_t)0x00)
#define TIM1_ETR_ETP_INVERTED       ((uint8_t)0x80)

#define TIM1_ETR_ECE_MASK           ((uint8_t)0x40) /* External Clock mask. */
#define TIM1_ETR_ECE_DISABLE        ((uint8_t)0x00)
#define TIM1_ETR_ECE_ENABLE         ((uint8_t)0x40)

#define TIM1_ETR_ETPS_MASK          ((uint8_t)0x30) /* External Trigger Prescaler mask. */
#define TIM1_ETR_ETPS_OFF           ((uint8_t)0x00)
#define TIM1_ETR_ETPS_DIV2          ((uint8_t)0x10)
#define TIM1_ETR_ETPS_DIV4          ((uint8_t)0x20)
#define TIM1_ETR_ETPS_DIV8          ((uint8_t)0x30)

#define TIM1_ETR_ETF_MASK           ((uint8_t)0x0F) /* External Trigger Filter mask. */
#define TIM1_ETR_ETF_DISABLE        ((uint8_t)0x00)
#define TIM1_ETR_ETF_DIV1_N2        ((uint8_t)0x01)
#define TIM1_ETR_ETF_DIV1_N4        ((uint8_t)0x02)
#define TIM1_ETR_ETF_DIV1_N8        ((uint8_t)0x03)
#define TIM1_ETR_ETF_DIV2_N6        ((uint8_t)0x04)
#define TIM1_ETR_ETF_DIV2_N8        ((uint8_t)0x05)
#define TIM1_ETR_ETF_DIV4_N6        ((uint8_t)0x06)
#define TIM1_ETR_ETF_DIV4_N8        ((uint8_t)0x07)
#define TIM1_ETR_ETF_DIV8_N6        ((uint8_t)0x08)
#define TIM1_ETR_ETF_DIV8_N8        ((uint8_t)0x09)
#define TIM1_ETR_ETF_DIV16_N5       ((uint8_t)0x0A)
#define TIM1_ETR_ETF_DIV16_N6       ((uint8_t)0x0B)
#define TIM1_ETR_ETF_DIV16_N8       ((uint8_t)0x0C)
#define TIM1_ETR_ETF_DIV32_N5       ((uint8_t)0x0D)
#define TIM1_ETR_ETF_DIV32_N6       ((uint8_t)0x0E)
#define TIM1_ETR_ETF_DIV32_N8       ((uint8_t)0x0F)

#define TIM1_IER_BIE_MASK           ((uint8_t)0x80) /* Break Interrupt Enable mask. */
#define TIM1_IER_BIE_DISABLE        ((uint8_t)0x00)
#define TIM1_IER_BIE_ENABLE         ((uint8_t)0x80)

#define TIM1_IER_TIE_MASK           ((uint8_t)0x40) /* Trigger Interrupt Enable mask. */
#define TIM1_IER_TIE_DISABLE        ((uint8_t)0x00)
#define TIM1_IER_TIE_ENABLE         ((uint8_t)0x40)

#define TIM1_IER_COMIE_MASK         ((uint8_t)0x20) /*  Commutation Interrupt Enable mask.*/
#define TIM1_IER_COMIE_DISABLE      ((uint8_t)0x00)
#define TIM1_IER_COMIE_ENABLE       ((uint8_t)0x20)

#define TIM1_IER_CC4IE_MASK         ((uint8_t)0x10) /* Capture/Compare 4 Interrupt Enable mask. */
#define TIM1_IER_CC4IE_DISABLE      ((uint8_t)0x00)
#define TIM1_IER_CC4IE_ENABLE       ((uint8_t)0x10)

#define TIM1_IER_CC3IE_MASK         ((uint8_t)0x08) /* Capture/Compare 3 Interrupt Enable mask. */
#define TIM1_IER_CC3IE_DISABLE      ((uint8_t)0x00)
#define TIM1_IER_CC3IE_ENABLE       ((uint8_t)0x08)

#define TIM1_IER_CC2IE_MASK         ((uint8_t)0x04) /* Capture/Compare 2 Interrupt Enable mask. */
#define TIM1_IER_CC2IE_DISABLE      ((uint8_t)0x00)
#define TIM1_IER_CC2IE_ENABLE       ((uint8_t)0x04)

#define TIM1_IER_CC1IE_MASK         ((uint8_t)0x02) /* Capture/Compare 1 Interrupt Enable mask. */
#define TIM1_IER_CC1IE_DISABLE      ((uint8_t)0x00)
#define TIM1_IER_CC1IE_ENABLE       ((uint8_t)0x02)

#define TIM1_IER_UIE_MASK           ((uint8_t)0x01) /* Update Interrupt Enable mask. */
#define TIM1_IER_UIE_DISABLE        ((uint8_t)0x00)
#define TIM1_IER_UIE_ENABLE         ((uint8_t)0x01)

#define TIM1_SR1_BIF_MASK           ((uint8_t)0x80) /* Break Interrupt Flag mask. */
#define TIM1_SR1_BIF_CLEAR          ((uint8_t)0x00)
#define TIM1_SR1_BIF_NONE           ((uint8_t)0x00)
#define TIM1_SR1_BIF_DETECTED       ((uint8_t)0x80)

#define TIM1_SR1_TIF_MASK           ((uint8_t)0x40) /* Trigger Interrupt Flag mask. */
#define TIM1_SR1_TIF_CLEAR          ((uint8_t)0x00)
#define TIM1_SR1_TIF_NONE           ((uint8_t)0x00)
#define TIM1_SR1_TIF_PENDING        ((uint8_t)0x40)

#define TIM1_SR1_COMIF_MASK         ((uint8_t)0x20) /* Commutation Interrupt Flag mask. */
#define TIM1_SR1_COMIF_CLEAR        ((uint8_t)0x00)
#define TIM1_SR1_COMIF_NONE         ((uint8_t)0x00)
#define TIM1_SR1_COMIF_PENDING      ((uint8_t)0x20)

#define TIM1_SR1_CC4IF_MASK         ((uint8_t)0x10) /* Capture/Compare 4 Interrupt Flag mask. */
#define TIM1_SR1_CC4IF_CLEAR        ((uint8_t)0x00)
#define TIM1_SR1_CC4IF_NONE         ((uint8_t)0x00)
#define TIM1_SR1_CC4IF_MATCH        ((uint8_t)0x10) // Input
#define TIM1_SR1_CC4IF_CAPTURE      ((uint8_t)0x10) // Output

#define TIM1_SR1_CC3IF_MASK         ((uint8_t)0x08) /* Capture/Compare 3 Interrupt Flag mask. */
#define TIM1_SR1_CC3IF_CLEAR        ((uint8_t)0x00)
#define TIM1_SR1_CC3IF_NONE         ((uint8_t)0x00)
#define TIM1_SR1_CC3IF_MATCH        ((uint8_t)0x08) // Input
#define TIM1_SR1_CC3IF_CAPTURE      ((uint8_t)0x08) // Output

#define TIM1_SR1_CC2IF_MASK         ((uint8_t)0x04) /* Capture/Compare 2 Interrupt Flag mask. */
#define TIM1_SR1_CC2IF_CLEAR        ((uint8_t)0x00)
#define TIM1_SR1_CC2IF_NONE         ((uint8_t)0x00)
#define TIM1_SR1_CC2IF_MATCH        ((uint8_t)0x04) // Input
#define TIM1_SR1_CC2IF_CAPTURE      ((uint8_t)0x04) // Output

#define TIM1_SR1_CC1IF_MASK         ((uint8_t)0x02) /* Capture/Compare 1 Interrupt Flag mask. */
#define TIM1_SR1_CC1IF_CLEAR        ((uint8_t)0x00)
#define TIM1_SR1_CC1IF_NONE         ((uint8_t)0x00)
#define TIM1_SR1_CC1IF_MATCH        ((uint8_t)0x02) // Input
#define TIM1_SR1_CC1IF_CAPTURE      ((uint8_t)0x02) // Output

#define TIM1_SR1_UIF_MASK           ((uint8_t)0x01) /* Update Interrupt Flag mask. */
#define TIM1_SR1_UIF_CLEAR          ((uint8_t)0x00)
#define TIM1_SR1_UIF_NONE           ((uint8_t)0x00)
#define TIM1_SR1_UIF_PENDING        ((uint8_t)0x01)

#define TIM1_SR2_CC4OF_MASK         ((uint8_t)0x10) /* Capture/Compare 4 Overcapture Flag mask. */
#define TIM1_SR2_CC4OF_NONE         ((uint8_t)0x00)
#define TIM1_SR2_CC4OF_DETECTED     ((uint8_t)0x10)

#define TIM1_SR2_CC3OF_MASK         ((uint8_t)0x08) /* Capture/Compare 3 Overcapture Flag mask. */
#define TIM1_SR2_CC3OF_NONE         ((uint8_t)0x00)
#define TIM1_SR2_CC3OF_DETECTED     ((uint8_t)0x08)

#define TIM1_SR2_CC2OF_MASK         ((uint8_t)0x04) /* Capture/Compare 2 Overcapture Flag mask. */
#define TIM1_SR2_CC2OF_NONE         ((uint8_t)0x00)
#define TIM1_SR2_CC2OF_DETECTED     ((uint8_t)0x04)

#define TIM1_SR2_CC1OF_MASK         ((uint8_t)0x02) /* Capture/Compare 1 Overcapture Flag mask. */
#define TIM1_SR2_CC1OF_NONE         ((uint8_t)0x00)
#define TIM1_SR2_CC1OF_DETECTED     ((uint8_t)0x02)

#define TIM1_EGR_BG_MASK            ((uint8_t)0x80) /* Break Generation mask. */
#define TIM1_EGR_BG_DISABLE         ((uint8_t)0x00)
#define TIM1_EGR_BG_ENABLE          ((uint8_t)0x80)

#define TIM1_EGR_TG_MASK            ((uint8_t)0x40) /* Trigger Generation mask. */
#define TIM1_EGR_TG_DISABLE         ((uint8_t)0x00)
#define TIM1_EGR_TG_ENABLE          ((uint8_t)0x40)

#define TIM1_EGR_COMG_MASK          ((uint8_t)0x20) /* Capture/Compare Control Update Generation mask. */
#define TIM1_EGR_COMG_DISABLE       ((uint8_t)0x00)
#define TIM1_EGR_COMG_ENABLE        ((uint8_t)0x20)

#define TIM1_EGR_CC4G_MASK          ((uint8_t)0x10) /* Capture/Compare 4 Generation mask. */
#define TIM1_EGR_CC4G_DISABLE       ((uint8_t)0x00)
#define TIM1_EGR_CC4G_ENABLE        ((uint8_t)0x10)

#define TIM1_EGR_CC3G_MASK          ((uint8_t)0x08) /* Capture/Compare 3 Generation mask. */
#define TIM1_EGR_CC3G_DISABLE       ((uint8_t)0x00)
#define TIM1_EGR_CC3G_ENABLE        ((uint8_t)0x08)

#define TIM1_EGR_CC2G_MASK          ((uint8_t)0x04) /* Capture/Compare 2 Generation mask. */
#define TIM1_EGR_CC2G_DISABLE       ((uint8_t)0x00)
#define TIM1_EGR_CC2G_ENABLE        ((uint8_t)0x04)

#define TIM1_EGR_CC1G_MASK          ((uint8_t)0x02) /* Capture/Compare 1 Generation mask. */
#define TIM1_EGR_CC1G_DISABLE       ((uint8_t)0x00)
#define TIM1_EGR_CC1G_ENABLE        ((uint8_t)0x02)

#define TIM1_EGR_UG_MASK            ((uint8_t)0x01) /* Update Generation mask. */
#define TIM1_EGR_UG_DISABLE         ((uint8_t)0x00)
#define TIM1_EGR_UG_ENABLE          ((uint8_t)0x01)

#define TIM1_CCMR_ICxPSC_MASK       ((uint8_t)0x0C) /* Input Capture x Prescaler mask. */
#define TIM1_CCMR_ICxPSC_DIV1       ((uint8_t)0x00)
#define TIM1_CCMR_ICxPSC_DIV2       ((uint8_t)0x04)
#define TIM1_CCMR_ICxPSC_DIV4       ((uint8_t)0x08)
#define TIM1_CCMR_ICxPSC_DIV8       ((uint8_t)0x0C)

#define TIM1_CCMR_ICxF_MASK         ((uint8_t)0xF0) /* Input Capture x Filter mask. */
#define TIM1_CCMR_ICxF_NONE         ((uint8_t)0x00)
#define TIM1_CCMR_ICxF_DIV1_N2      ((uint8_t)0x10)
#define TIM1_CCMR_ICxF_DIV1_N4      ((uint8_t)0x20)
#define TIM1_CCMR_ICxF_DIV1_N8      ((uint8_t)0x30)
#define TIM1_CCMR_ICxF_DIV2_N6      ((uint8_t)0x40)
#define TIM1_CCMR_ICxF_DIV2_N8      ((uint8_t)0x50)
#define TIM1_CCMR_ICxF_DIV4_N6      ((uint8_t)0x60)
#define TIM1_CCMR_ICxF_DIV4_N8      ((uint8_t)0x70)
#define TIM1_CCMR_ICxF_DIV8_N6      ((uint8_t)0x80)
#define TIM1_CCMR_ICxF_DIV8_N8      ((uint8_t)0x90)
#define TIM1_CCMR_ICxF_DIV16_N5     ((uint8_t)0xA0)
#define TIM1_CCMR_ICxF_DIV16_N6     ((uint8_t)0xB0)
#define TIM1_CCMR_ICxF_DIV16_N8     ((uint8_t)0xC0)
#define TIM1_CCMR_ICxF_DIV32_N5     ((uint8_t)0xD0)
#define TIM1_CCMR_ICxF_DIV32_N6     ((uint8_t)0xE0)
#define TIM1_CCMR_ICxF_DIV32_N8     ((uint8_t)0xF0)

#define TIM1_CCMR_OCxCE_MASK        ((uint8_t)0x80) /* Output Compare x Clear Enable mask. */
#define TIM1_CCMR_OCxCE_DISABLE     ((uint8_t)0x00)
#define TIM1_CCMR_OCxCE_ENABLE      ((uint8_t)0x80)

#define TIM1_CCMR_OCxM_MASK         ((uint8_t)0x70) /* Output Compare x Mode mask. */
#define TIM1_CCMR_OCxM_FROZEN       ((uint8_t)0x00)
#define TIM1_CCMR_OCxM_ACTIVE       ((uint8_t)0x10)
#define TIM1_CCMR_OCxM_INACTIVE     ((uint8_t)0x20)
#define TIM1_CCMR_OCxM_TOGGLE       ((uint8_t)0x30)
#define TIM1_CCMR_OCxM_FORCELOW     ((uint8_t)0x40)
#define TIM1_CCMR_OCxM_FORCEHIGH    ((uint8_t)0x50)
#define TIM1_CCMR_OCxM_PWM1         ((uint8_t)0x60)
#define TIM1_CCMR_OCxM_PWM2         ((uint8_t)0x70)

#define TIM1_CCMR_OCxPE_MASK        ((uint8_t)0x08) /* Output Compare x Preload Enable mask. */
#define TIM1_CCMR_OCxPE_DISABLE     ((uint8_t)0x00)
#define TIM1_CCMR_OCxPE_ENABLE      ((uint8_t)0x08)

#define TIM1_CCMR_OCxFE_MASK        ((uint8_t)0x04) /* Output Compare x Fast Enable mask. */
#define TIM1_CCMR_OCxFE_DISABLE     ((uint8_t)0x00)
#define TIM1_CCMR_OCxFE_ENABLE      ((uint8_t)0x04)

#define TIM1_CCMR_CCxS_MASK         ((uint8_t)0x03) /* Capture/Compare x Selection mask. */
#define TIM1_CCMR_CCxS_OUTPUT       ((uint8_t)0x00)
#define TIM1_CCMR_CCxS_DIRECT       ((uint8_t)0x01)
#define TIM1_CCMR_CCxS_INDIRECT     ((uint8_t)0x02)
#define TIM1_CCMR_CCxS_IN_TI1FP1    ((uint8_t)0x01)    // Channel 1
#define TIM1_CCMR_CCxS_IN_TI2FP1    ((uint8_t)0x02)    // Channel 1
#define TIM1_CCMR_CCxS_IN_TI2FP2    ((uint8_t)0x01)    // Channel 2
#define TIM1_CCMR_CCxS_IN_TI1FP2    ((uint8_t)0x02)    // Channel 2
#define TIM1_CCMR_CCxS_IN_TI3FP3    ((uint8_t)0x01)    // Channel 3
#define TIM1_CCMR_CCxS_IN_TI4FP3    ((uint8_t)0x02)    // Channel 3
#define TIM1_CCMR_CCxS_IN_TI4FP4    ((uint8_t)0x01)    // Channel 4
#define TIM1_CCMR_CCxS_IN_TI3FP4    ((uint8_t)0x02)    // Channel 5
#define TIM1_CCMR_CCxS_IN_TRC       ((uint8_t)0x03)

#define TIM1_CCER1_CC2NP_MASK       ((uint8_t)0x80) /* Capture/Compare 2 Complementary output Polarity mask. */
#define TIM1_CCER1_CC2NP_DISABLE    ((uint8_t)0x00)
#define TIM1_CCER1_CC2NP_ENABLE     ((uint8_t)0x80)

#define TIM1_CCER1_CC2NE_MASK       ((uint8_t)0x40) /* Capture/Compare 2 Complementary output enable mask. */
#define TIM1_CCER1_CC2NE_DISABLE    ((uint8_t)0x00)
#define TIM1_CCER1_CC2NE_ENABLE     ((uint8_t)0x40)

#define TIM1_CCER1_CC2P_MASK        ((uint8_t)0x20) /* Capture/Compare 2 output Polarity mask. */
#define TIM1_CCER1_CC2P_HIGH        ((uint8_t)0x00) // When output on OC2
#define TIM1_CCER1_CC2P_LOW         ((uint8_t)0x20)
#define TIM1_CCER1_CC2P_HIGH_RISING ((uint8_t)0x00) // When input with trigger on T1F
#define TIM1_CCER1_CC2P_LOW_FALLING ((uint8_t)0x20)
#define TIM1_CCER1_CC2P_RISING      ((uint8_t)0x00) // When input with capture on T1F or T2F
#define TIM1_CCER1_CC2P_FALLING     ((uint8_t)0x20)

#define TIM1_CCER1_CC2E_MASK        ((uint8_t)0x10) /* Capture/Compare 2 output enable mask. */
#define TIM1_CCER1_CC2E_DIABLE      ((uint8_t)0x00)
#define TIM1_CCER1_CC2E_ENABLE      ((uint8_t)0x10)

#define TIM1_CCER1_CC1NP_MASK       ((uint8_t)0x08) /* Capture/Compare 1 Complementary output Polarity mask. */
#define TIM1_CCER1_CC1NP_DISABLE    ((uint8_t)0x00)
#define TIM1_CCER1_CC1NP_ENABLE     ((uint8_t)0x08)

#define TIM1_CCER1_CC1NE_MASK       ((uint8_t)0x04) /* Capture/Compare 1 Complementary output enable mask. */
#define TIM1_CCER1_CC1NE_DISABLE    ((uint8_t)0x00)
#define TIM1_CCER1_CC1NE_ENABLE     ((uint8_t)0x04)

#define TIM1_CCER1_CC1P_MASK        ((uint8_t)0x02) /* Capture/Compare 1 output Polarity mask. */
#define TIM1_CCER1_CC1P_HIGH        ((uint8_t)0x00) // When output on OC1
#define TIM1_CCER1_CC1P_LOW         ((uint8_t)0x02)
#define TIM1_CCER1_CC1P_HIGH_RISING ((uint8_t)0x00) // When input with trigger on T1F
#define TIM1_CCER1_CC1P_LOW_FALLING ((uint8_t)0x02)
#define TIM1_CCER1_CC1P_RISING      ((uint8_t)0x00) // When input with capture on T1F or T2F
#define TIM1_CCER1_CC1P_FALLING     ((uint8_t)0x02)

#define TIM1_CCER1_CC1E_MASK        ((uint8_t)0x01) /* Capture/Compare 1 output enable mask. */
#define TIM1_CCER1_CC1E_DISABLE     ((uint8_t)0x00)
#define TIM1_CCER1_CC1E_ENABLE      ((uint8_t)0x01)

#define TIM1_CCER2_CC4P_MASK        ((uint8_t)0x20) /* Capture/Compare 4 output Polarity mask. */
#define TIM1_CCER2_CC4P_HIGH        ((uint8_t)0x00) // When output on OC4
#define TIM1_CCER2_CC4P_LOW         ((uint8_t)0x20)
#define TIM1_CCER2_CC4P_HIGH_RISING ((uint8_t)0x00) // When input with trigger on T1F
#define TIM1_CCER2_CC4P_LOW_FALLING ((uint8_t)0x20)
#define TIM1_CCER2_CC4P_RISING      ((uint8_t)0x00) // When input with capture on T1F or T2F
#define TIM1_CCER2_CC4P_FALLING     ((uint8_t)0x20)

#define TIM1_CCER2_CC4E_MASK        ((uint8_t)0x10) /* Capture/Compare 4 output enable mask. */
#define TIM1_CCER2_CC4E_DISABLE     ((uint8_t)0x00)
#define TIM1_CCER2_CC4E_ENABLE      ((uint8_t)0x10)

#define TIM1_CCER2_CC3NP_MASK       ((uint8_t)0x08) /* Capture/Compare 3 Complementary output Polarity mask. */
#define TIM1_CCER2_CC3NP_DISABLE    ((uint8_t)0x00)
#define TIM1_CCER2_CC3NP_ENABLE     ((uint8_t)0x08)

#define TIM1_CCER2_CC3NE_MASK       ((uint8_t)0x04) /* Capture/Compare 3 Complementary output enable mask. */
#define TIM1_CCER2_CC3NE_DISABLE    ((uint8_t)0x00)
#define TIM1_CCER2_CC3NE_ENABLE     ((uint8_t)0x04)

#define TIM1_CCER2_CC3P_MASK        ((uint8_t)0x02) /* Capture/Compare 3 output Polarity mask. */
#define TIM1_CCER2_CC3P_HIGH        ((uint8_t)0x00) // When output on OC3
#define TIM1_CCER2_CC3P_LOW         ((uint8_t)0x02)
#define TIM1_CCER2_CC3P_HIGH_RISING ((uint8_t)0x00) // When input with trigger on T1F
#define TIM1_CCER2_CC3P_LOW_FALLING ((uint8_t)0x02)
#define TIM1_CCER2_CC3P_RISING      ((uint8_t)0x00) // When input with capture on T1F or T2F
#define TIM1_CCER2_CC3P_FALLING     ((uint8_t)0x02)

#define TIM1_CCER2_CC3E_MASK        ((uint8_t)0x01) /* Capture/Compare 3 output enable mask. */
#define TIM1_CCER2_CC3E_DISABLE     ((uint8_t)0x00)
#define TIM1_CCER2_CC3E_ENABLE      ((uint8_t)0x01)

#define TIM1_CNTRH_CNT_MASK         ((uint8_t)0xFF) /* Counter Value (MSB) mask. */
#define TIM1_CNTRL_CNT_MASK         ((uint8_t)0xFF) /* Counter Value (LSB) mask. */

#define TIM1_PSCRH_PSC_MASK         ((uint8_t)0xFF) /* Prescaler Value (MSB) mask. */
#define TIM1_PSCRL_PSC_MASK         ((uint8_t)0xFF) /* Prescaler Value (LSB) mask. */

#define TIM1_ARRH_ARR_MASK          ((uint8_t)0xFF) /* Autoreload Value (MSB) mask. */
#define TIM1_ARRL_ARR_MASK          ((uint8_t)0xFF) /* Autoreload Value (LSB) mask. */

#define TIM1_RCR_REP_MASK           ((uint8_t)0xFF) /* Repetition Counter Value mask. */

#define TIM1_CCR1H_CCR1_MASK        ((uint8_t)0xFF) /* Capture/Compare 1 Value (MSB) mask. */
#define TIM1_CCR1L_CCR1_MASK        ((uint8_t)0xFF) /* Capture/Compare 1 Value (LSB) mask. */

#define TIM1_CCR2H_CCR2_MASK        ((uint8_t)0xFF) /* Capture/Compare 2 Value (MSB) mask. */
#define TIM1_CCR2L_CCR2_MASK        ((uint8_t)0xFF) /* Capture/Compare 2 Value (LSB) mask. */

#define TIM1_CCR3H_CCR3_MASK        ((uint8_t)0xFF) /* Capture/Compare 3 Value (MSB) mask. */
#define TIM1_CCR3L_CCR3_MASK        ((uint8_t)0xFF) /* Capture/Compare 3 Value (LSB) mask. */

#define TIM1_CCR4H_CCR4_MASK        ((uint8_t)0xFF) /* Capture/Compare 4 Value (MSB) mask. */
#define TIM1_CCR4L_CCR4_MASK        ((uint8_t)0xFF) /* Capture/Compare 4 Value (LSB) mask. */

#define TIM1_BKR_MOE_MASK           ((uint8_t)0x80) /* Main Output Enable mask. */
#define TIM1_BKR_MOE_DISABLE        ((uint8_t)0x00)
#define TIM1_BKR_MOE_ENABLE         ((uint8_t)0x80)

#define TIM1_BKR_AOE_MASK           ((uint8_t)0x40) /* Automatic Output Enable mask. */
#define TIM1_BKR_AOE_DISABLE        ((uint8_t)0x00)
#define TIM1_BKR_AOE_ENABLE         ((uint8_t)0x40)

#define TIM1_BKR_BKP_MASK           ((uint8_t)0x20) /* Break Polarity mask. */
#define TIM1_BKR_BKP_LOW            ((uint8_t)0x00)
#define TIM1_BKR_BKP_HIGH           ((uint8_t)0x20)

#define TIM1_BKR_BKE_MASK           ((uint8_t)0x10) /* Break Enable mask. */
#define TIM1_BKR_BKE_DISABLE        ((uint8_t)0x00)
#define TIM1_BKR_BKE_ENABLE         ((uint8_t)0x10)

#define TIM1_BKR_OSSR_MASK          ((uint8_t)0x08) /* Off-State Selection for Run mode mask. */
#define TIM1_BKR_OSSR_DISABLE       ((uint8_t)0x00)
#define TIM1_BKR_OSSR_ENABLE        ((uint8_t)0x08)

#define TIM1_BKR_OSSI_MASK          ((uint8_t)0x04) /* Off-State Selection for Idle mode mask. */
#define TIM1_BKR_OSSI_DISABLE       ((uint8_t)0x00)
#define TIM1_BKR_OSSI_ENABLE        ((uint8_t)0x04)

#define TIM1_BKR_LOCK_MASK          ((uint8_t)0x03) /* Lock Configuration mask. */
#define TIM1_BKR_LOCK_OFF           ((uint8_t)0x00)
#define TIM1_BKR_LOCK_LEVEL1        ((uint8_t)0x01)
#define TIM1_BKR_LOCK_LEVEL2        ((uint8_t)0x02)
#define TIM1_BKR_LOCK_LEVEL3        ((uint8_t)0x03)

#define TIM1_DTR_DTG_MASK           ((uint8_t)0xFF) /* Dead-Time Generator set-up mask. */

#define TIM1_OISR_OIS4_MASK         ((uint8_t)0x40) /* Output Idle state 4 (OC4 output) mask. */
#define TIM1_OISR_OIS4_DISABLE      ((uint8_t)0x00)
#define TIM1_OISR_OIS4_ENABLE       ((uint8_t)0x40)

#define TIM1_OISR_OIS3N_MASK        ((uint8_t)0x20) /* Output Idle state 3 (OC3N output) mask. */
#define TIM1_OISR_OIS3N_DISABLE     ((uint8_t)0x00)
#define TIM1_OISR_OIS3N_ENABLE      ((uint8_t)0x20)

#define TIM1_OISR_OIS3_MASK         ((uint8_t)0x10) /* Output Idle state 3 (OC3 output) mask. */
#define TIM1_OISR_OIS3_DISABLE      ((uint8_t)0x00)
#define TIM1_OISR_OIS3_ENABLE       ((uint8_t)0x10)

#define TIM1_OISR_OIS2N_MASK        ((uint8_t)0x08) /* Output Idle state 2 (OC2N output) mask. */
#define TIM1_OISR_OIS2N_DISABLE     ((uint8_t)0x00)
#define TIM1_OISR_OIS2N_ENABLE      ((uint8_t)0x08)

#define TIM1_OISR_OIS2_MASK         ((uint8_t)0x04) /* Output Idle state 2 (OC2 output) mask. */
#define TIM1_OISR_OIS2_DISABLE      ((uint8_t)0x00)
#define TIM1_OISR_OIS2_ENABLE       ((uint8_t)0x04)

#define TIM1_OISR_OIS1N_MASK        ((uint8_t)0x02) /* Output Idle state 1 (OC1N output) mask. */
#define TIM1_OISR_OIS1N_DISABLE     ((uint8_t)0x00)
#define TIM1_OISR_OIS1N_ENABLE      ((uint8_t)0x02)

#define TIM1_OISR_OIS1_MASK         ((uint8_t)0x01) /* Output Idle state 1 (OC1 output) mask. */
#define TIM1_OISR_OIS1_DISABLE      ((uint8_t)0x00)
#define TIM1_OISR_OIS1_ENABLE       ((uint8_t)0x01)

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

#define TIM4_BaseAddress            0x5340
#define TIM4                        ((stm8_tim4_t *)TIM4_BaseAddress)

#define TIM4_CR1_ARPE_MASK          ((uint8_t)0x80) /* Auto-Reload Preload Enable mask. */
#define TIM4_CR1_ARPE_DISABLE       ((uint8_t)0x00)
#define TIM4_CR1_ARPE_ENABLE        ((uint8_t)0x80)

#define TIM4_CR1_OPM_MASK           ((uint8_t)0x08) /* One Pulse Mode mask. */
#define TIM4_CR1_OPM_DISABLE        ((uint8_t)0x00)
#define TIM4_CR1_OPM_ENABLE         ((uint8_t)0x08)

#define TIM4_CR1_URS_MASK           ((uint8_t)0x04) /* Update Request Source mask. */
#define TIM4_CR1_URS_DISABLE        ((uint8_t)0x00)
#define TIM4_CR1_URS_ENABLE         ((uint8_t)0x04)

#define TIM4_CR1_UDIS_MASK          ((uint8_t)0x02) /* Update Disable mask. */
#define TIM4_CR1_UDIS_DISABLE       ((uint8_t)0x00)
#define TIM4_CR1_UDIS_ENABLE        ((uint8_t)0x02)

#define TIM4_CR1_CEN_MASK           ((uint8_t)0x01) /* Counter Enable mask. */
#define TIM4_CR1_CEN_DISABLE        ((uint8_t)0x00)
#define TIM4_CR1_CEN_ENABLE         ((uint8_t)0x01)

#define TIM4_IER_UIE_MASK           ((uint8_t)0x01) /* Update Interrupt Enable mask. */
#define TIM4_IER_UIE_DISABLE        ((uint8_t)0x00)
#define TIM4_IER_UIE_ENABLE         ((uint8_t)0x01)

#define TIM4_SR1_UIF_MASK           ((uint8_t)0x01) /* Update Interrupt Flag mask. */
#define TIM4_SR1_UIF_CLEAR          ((uint8_t)0x00)
#define TIM4_SR1_UIF_NONE           ((uint8_t)0x00)
#define TIM4_SR1_UIF_PENDING        ((uint8_t)0x01)

#define TIM4_EGR_UG_MASK            ((uint8_t)0x01) /* Update Generation mask. */
#define TIM4_EGR_UG_DISABLE         ((uint8_t)0x00)
#define TIM4_EGR_UG_ENABLE          ((uint8_t)0x01)

#define TIM4_CNTR_CNT_MASK          ((uint8_t)0xFF) /* Counter Value (LSB) mask. */

#define TIM4_PSCR_PSC_MASK          ((uint8_t)0x07) /* Prescaler Value  mask. */
#define TIM4_PSCR_DIV1              ((uint8_t)0x00)
#define TIM4_PSCR_DIV2              ((uint8_t)0x01)
#define TIM4_PSCR_DIV4              ((uint8_t)0x02)
#define TIM4_PSCR_DIV8              ((uint8_t)0x03)
#define TIM4_PSCR_DIV16             ((uint8_t)0x04)
#define TIM4_PSCR_DIV32             ((uint8_t)0x05)
#define TIM4_PSCR_DIV64             ((uint8_t)0x06)
#define TIM4_PSCR_DIV128            ((uint8_t)0x07)

#define TIM4_ARR_ARR_MASK           ((uint8_t)0xFF) /* Autoreload Value mask. */

//=============================================================================
// Intra Integrated Circuit (I2C)
//

//-----------------------------------------------------------------------------
// I2C 1
//
typedef struct
{
    __IO uint8_t CR1;       // Control register #1
    __IO uint8_t CR2;       // Control register #2
    __IO uint8_t FREQR;     // Frequency register
    __IO uint8_t OARL;      // Own address register low
    __IO uint8_t OARH;      // Own address register high
    uint8_t RESERVED1;
    __IO uint8_t DR;        // Data register
    __IO uint8_t SR1;       // Status register #1
    __IO uint8_t SR2;       // Status register #2
    __IO uint8_t SR3;       // Status register #3
    __IO uint8_t ITR;       // Interrupt register
    __IO uint8_t CCRL;      // Clock control register low
    __IO uint8_t CCRH;      // Clock control register high
    __IO uint8_t TRISER;    // Rise time register
} stm8_i2c_t;

#define I2C_BaseAddress             0x5210
#define I2C                         ((stm8_i2c_t *)I2C_BaseAddress)

#define I2C_CR1_NOSTRETCH_MASK      ((uint8_t)0x80)     // Clock stretching disable
#define I2C_CR1_NOSTRETCH_ENABLE    ((uint8_t)0x00)
#define I2C_CR1_NOSTRETCH_DISABLE   ((uint8_t)0x80)

#define I2C_CR1_ENGC_MASK           ((uint8_t)0x40)     // General call enable
#define I2C_CR1_ENGC_DISABLE        ((uint8_t)0x00)
#define I2C_CR1_ENGC_ENABLE         ((uint8_t)0x40)

#define I2C_CR1_PE_MASK             ((uint8_t)0x01)     // Peripheral enable
#define I2C_CR1_PE_DISABLE          ((uint8_t)0x00)
#define I2C_CR1_PE_ENABLE           ((uint8_t)0x01)

#define I2C_CR2_SWRST_MASK          ((uint8_t)0x80)     // Software reset
#define I2C_CR2_SWRST_RUNNING       ((uint8_t)0x00)
#define I2C_CR2_SWRST_RESET         ((uint8_t)0x80)

#define I2C_CR2_POS_MASK            ((uint8_t)0x08)     // Acknowledge position
#define I2C_CR2_POS_CURRENT         ((uint8_t)0x00)
#define I2C_CR2_POS_NEXT            ((uint8_t)0x08)

#define I2C_CR2_ACK_MASK            ((uint8_t)0x04)     // Acknowledge enable
#define I2C_CR2_ACK_DISABLE         ((uint8_t)0x00)
#define I2C_CR2_ACK_ENABLE          ((uint8_t)0x04)

#define I2C_CR2_STOP_MASK           ((uint8_t)0x02)     // Stop generation
#define I2C_CR2_STOP_DISABLE        ((uint8_t)0x00)
#define I2C_CR2_STOP_ENABLE         ((uint8_t)0x02)

#define I2C_CR2_START_MASK          ((uint8_t)0x01)     // Start generation
#define I2C_CR2_START_DISABLE       ((uint8_t)0x00)
#define I2C_CR2_START_ENABLE        ((uint8_t)0x01)

#define I2C_FREQR_FREQ_MASK         ((uint8_t)0x3F)     // Peripheral clock frequency
#define I2C_FREQR_FREQ_1MHZ         ((uint8_t)0x01)     // Min for standard mode
#define I2C_FREQR_FREQ_2MHZ         ((uint8_t)0x02)
#define I2C_FREQR_FREQ_3MHZ         ((uint8_t)0x03)
#define I2C_FREQR_FREQ_4MHZ         ((uint8_t)0x04)     // Min for fast mode
#define I2C_FREQR_FREQ_5MHZ         ((uint8_t)0x05)
#define I2C_FREQR_FREQ_6MHZ         ((uint8_t)0x06)
#define I2C_FREQR_FREQ_7MHZ         ((uint8_t)0x07)
#define I2C_FREQR_FREQ_8MHZ         ((uint8_t)0x08)
#define I2C_FREQR_FREQ_9MHZ         ((uint8_t)0x09)
#define I2C_FREQR_FREQ_10MHZ        ((uint8_t)0x0A)
#define I2C_FREQR_FREQ_11MHZ        ((uint8_t)0x0B)
#define I2C_FREQR_FREQ_12MHZ        ((uint8_t)0x0C)
#define I2C_FREQR_FREQ_13MHZ        ((uint8_t)0x0D)
#define I2C_FREQR_FREQ_14MHZ        ((uint8_t)0x0E)
#define I2C_FREQR_FREQ_15MHZ        ((uint8_t)0x0F)
#define I2C_FREQR_FREQ_16MHZ        ((uint8_t)0x10)
#define I2C_FREQR_FREQ_17MHZ        ((uint8_t)0x11)
#define I2C_FREQR_FREQ_18MHZ        ((uint8_t)0x12)
#define I2C_FREQR_FREQ_19MHZ        ((uint8_t)0x13)
#define I2C_FREQR_FREQ_20MHZ        ((uint8_t)0x14)
#define I2C_FREQR_FREQ_21MHZ        ((uint8_t)0x15)
#define I2C_FREQR_FREQ_22MHZ        ((uint8_t)0x16)
#define I2C_FREQR_FREQ_23MHZ        ((uint8_t)0x17)
#define I2C_FREQR_FREQ_24MHZ        ((uint8_t)0x18)

#define I2C_OARL_ADD_MASK           ((uint8_t)0xFF)     // Address 7:0 (bit 0 don't care in 7 bit mode)

#define I2C_OARH_ADD_MASK           ((uint8_t)0x06)     // Address 9:8 (only for 10 bit mode)
#define I2C_OARH_ADD_SHIFT          1

#define I2C_OARH_ADDCONF_MASK       ((uint8_t)0x40)     // Address mode configuration
#define I2C_OARH_ADDCONF            ((uint8_t)0x40)     // Must always be written as 1

#define I2C_OARH_ADDMODE_MASK       ((uint8_t)0x80)     // Address mode (slave)
#define I2C_OARH_ADDMODE_7BIT       ((uint8_t)0x00)
#define I2C_OARH_ADDMODE_10BIT      ((uint8_t)0x80)

#define I2C_DR_DR_MASK              ((uint8_t)0xFF)     // Data register

#define I2C_SR1_TXE_MASK            ((uint8_t)0x80)     // Data register empty (transmitters)
#define I2C_SR1_TXE_NOT_EMPTY       ((uint8_t)0x00)
#define I2C_SR1_TXE_EMPTY           ((uint8_t)0x80)

#define I2C_SR1_RXNE_MASK           ((uint8_t)0x40)     // Data register not empty (receivers)
#define I2C_SR1_RXNE_EMPTY          ((uint8_t)0x00)
#define I2C_SR1_RXNE_NOT_EMPTY      ((uint8_t)0x40)

#define I2C_SR1_STOPF_MASK          ((uint8_t)0x10)     // Stop detection (slave)
#define I2C_SR1_STOPF_NOT_DETECTED  ((uint8_t)0x00)
#define I2C_SR1_STOPF_DETECTED      ((uint8_t)0x10)

#define I2C_SR1_ADD10_MASK          ((uint8_t)0x08)     // 10-bit header sent
#define I2C_SR1_ADD10_NOT_SENT      ((uint8_t)0x00)
#define I2C_SR1_ADD10_SENT          ((uint8_t)0x08)

#define I2C_SR1_BTF_MASK            ((uint8_t)0x04)     // Byte transfer finished
#define I2C_SR1_BTF_NOT_DONE        ((uint8_t)0x00)
#define I2C_SR1_BTF_DONE            ((uint8_t)0x04)

#define I2C_SR1_ADDR_MASK           ((uint8_t)0x02)     // Address sent/matched
#define I2C_SR1_ADDR_MISMATCH       ((uint8_t)0x00)     // Receiver
#define I2C_SR1_ADDR_MATCH          ((uint8_t)0x02)
#define I2C_SR1_ADDR_NOT_END_OF_TX  ((uint8_t)0x00)     // Transmitter
#define I2C_SR1_ADDR_END_OF_TX      ((uint8_t)0x02)

#define I2C_SR1_SB_MASK             ((uint8_t)0x01)     // Start bit
#define I2C_SR1_SB_NOT_DONE         ((uint8_t)0x00)
#define I2C_SR1_SB_DONE             ((uint8_t)0x01)

#define I2C_SR2_WUFH_MASK           ((uint8_t)0x20)     // Wakeup from halt
#define I2C_SR2_WUFH_CLEAR          ((uint8_t)0x00)
#define I2C_SR2_WUFH_NOT_DONE       ((uint8_t)0x00)
#define I2C_SR2_WUFH_DONE           ((uint8_t)0x20)

#define I2C_SR2_OVR_MASK            ((uint8_t)0x08)     // Overrun/Underrun
#define I2C_SR2_OVR_CLEAR           ((uint8_t)0x00)
#define I2C_SR2_OVR_NOT_DONE        ((uint8_t)0x00)
#define I2C_SR2_OVR_DONE            ((uint8_t)0x08)

#define I2C_SR2_AF_MASK             ((uint8_t)0x04)     // Acknowledge failure
#define I2C_SR2_AF_CLEAR            ((uint8_t)0x00)
#define I2C_SR2_AF_NO_FAILURE       ((uint8_t)0x00)
#define I2C_SR2_AF_FAILURE          ((uint8_t)0x04)

#define I2C_SR2_ARLO_MASK           ((uint8_t)0x02)     // Arbitration lost
#define I2C_SR2_ARLO_CLEAR          ((uint8_t)0x00)
#define I2C_SR2_ARLO_NOT_DECTECTED  ((uint8_t)0x00)
#define I2C_SR2_ARLO_DECTECTED      ((uint8_t)0x02)

#define I2C_SR2_BERR_MASK           ((uint8_t)0x01)     // Bus error
#define I2C_SR2_BERR_CLEAR          ((uint8_t)0x00)
#define I2C_SR2_BERR_NOT_DECTECTED  ((uint8_t)0x00)
#define I2C_SR2_BERR_DECTECTED      ((uint8_t)0x01)

#define I2C_SR3_DUALF_MASK          ((uint8_t)0x80)     // Dual flag
#define I2C_SR3_DUALF_MATCH_OAR1    ((uint8_t)0x00)
#define I2C_SR3_DUALF_MATCH_OAR2    ((uint8_t)0x80)

#define I2C_SR3_GENCALL_MASK        ((uint8_t)0x10)     // General call header
#define I2C_SR3_GENCALL_NOT_RECEIVED ((uint8_t)0x00)
#define I2C_SR3_GENCALL_RECEIVED    ((uint8_t)0x10)

#define I2C_SR3_TRA_MASK            ((uint8_t)0x04)     // Transmitter/Receiver
#define I2C_SR3_TRA_RECEIVED        ((uint8_t)0x00)
#define I2C_SR3_TRA_TRANSMITTED     ((uint8_t)0x04)

#define I2C_SR3_BUSY_MASK           ((uint8_t)0x02)     // Bus busy
#define I2C_SR3_BUSY_NO_COMMS       ((uint8_t)0x00)
#define I2C_SR3_BUSY_ONGOING        ((uint8_t)0x02)

#define I2C_SR3_MSL_MASK            ((uint8_t)0x01)     // Master/Slave
#define I2C_SR3_MSL_SLAVE           ((uint8_t)0x00)
#define I2C_SR3_MSL_MASTER          ((uint8_t)0x01)

#define I2C_ITR_BUFEN_MASK          ((uint8_t)0x04)     // Buffer interrupt enable
#define I2C_ITR_BUFEN_DISABLE       ((uint8_t)0x00)
#define I2C_ITR_BUFEN_ENABLE        ((uint8_t)0x04)

#define I2C_ITR_EVTEN_MASK          ((uint8_t)0x02)     // Event interrupt enable
#define I2C_ITR_EVTEN_DISABLE       ((uint8_t)0x00)
#define I2C_ITR_EVTEN_ENABLE        ((uint8_t)0x02)

#define I2C_ITR_ERREN_MASK          ((uint8_t)0x04)     // Error interrupt enable
#define I2C_ITR_ERREN_DISABLE       ((uint8_t)0x00)
#define I2C_ITR_ERREN_ENABLE        ((uint8_t)0x04)

#define I2C_CCRL_CCR_MASK           ((uint8_t)0xFF)     // Clock register low

#define I2C_CCRH_FS_MASK            ((uint8_t)0x80)     // I2C master mode selection
#define I2C_CCRH_FS_STANDARD        ((uint8_t)0x00)
#define I2C_CCRH_FS_FAST            ((uint8_t)0x80)

#define I2C_CCRH_DUTY_MASK          ((uint8_t)0x40)     // Fast mode duty cycle
#define I2C_CCRH_DUTY_2             ((uint8_t)0x00)
#define I2C_CCRH_DUTY_169           ((uint8_t)0x40)

#define I2C_CCRH_CCR_MASK           ((uint8_t)0x0F)     // Clock register high

#define I2C_TRISE_MASK              ((uint8_t)0x3F)     // Maximum rise time


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
    } uart;
} stm8_uart_t;

#define UART1_BaseAddress           0x5340
#define UART2_BaseAddress           0x5240
#define UART1                       ((stm8_uart1_t *)UART1_BaseAddress)
#define UART2                       ((stm8_uart2_t *)UART2_BaseAddress)

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

//-----------------------------------------------------------------------------
// Return the current clock frequency
//
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

circular_buffer_t *tx2_cirbuf;
circular_buffer_t *rx2_cirbuf;

//-----------------------------------------------------------------------------
// Set up the circular buffers for the UART functions to use
//
void Uart2_Init(circular_buffer_t *tx, circular_buffer_t *rx)
{
    tx2_cirbuf = tx;
    rx2_cirbuf = rx;
}

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
void Uart_CalcBRR(uint32_t baud, volatile uint8_t *brr1, volatile uint8_t *brr2)
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
bool Uart2_SendByte(uint8_t byte) CRITICAL
{
    if ((UART2->CR2 & UARTx_CR2_TIEN_MASK) == UARTx_CR2_TIEN_ENABLE)
    {
        // Interrupts enabled, so buffer being used
        if (CircBuf_IsFull(tx2_cirbuf))
        {
            // However, buffer is full so can't send
            return false;
        }
        CircBuf_Put(tx2_cirbuf, byte);
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
            CircBuf_Put(tx2_cirbuf, byte);
            Uart2_EnableTxInterrupts();
        }
    }
    return true;
}

bool Uart2_BufferIsFull(void)
{
    if (CircBuf_IsFull(tx2_cirbuf))
    {
        return true;
    }
    return false;
}

//-----------------------------------------------------------------------------
// Send a byte, waiting till it can be sent
//
void Uart2_BlockingSendByte(uint8_t byte)
{
    while (!Uart2_SendByte(byte))
    {
    }
}

//-----------------------------------------------------------------------------
// Send a byte, without using circular buffer, but blocking till it can be sent
//
void Uart2_DirectSendByte(uint8_t byte)
{
    while ((UART2->SR & UARTx_SR_TXE_MASK) == UARTx_SR_TXE_NOTREADY)
    {
    }
    UART2->DR = byte;
}

//-----------------------------------------------------------------------------
// Interrupt handler for the transmission
//
#if defined __IAR_SYSTEMS_ICC__
#pragma vector=20
#endif
INTERRUPT(UART2_TX_IRQHandler, 20)
{
    //if (!CircBuf_IsEmpty(&tx_cirbuf))
    {
        UART2->DR = CircBuf_Get(tx2_cirbuf);  // Clears TXE flag
    }
    if (CircBuf_IsEmpty(tx2_cirbuf))
    {
        Uart2_DisableTxInterrupts();
    }
}

//-----------------------------------------------------------------------------
// Interrupt handler for the receiver
//
#if defined __IAR_SYSTEMS_ICC__
#pragma vector=21
#endif
INTERRUPT(UART2_RX_IRQHandler, 21)
{
    (void)UART2->DR;      // Clears RXNE flag
}

//=============================================================================
// String functions using the serial port
//

void (*OutputByteFunc)(uint8_t ch) = Uart2_BlockingSendByte;

//-----------------------------------------------------------------------------
// Set up the output character function
//
void OutputInit(void (*func)(uint8_t ch))
{
    OutputByteFunc = func;
}

//-----------------------------------------------------------------------------
// Output a character
//
void OutputChar(char ch)
{
    OutputByteFunc(ch);
}

//-----------------------------------------------------------------------------
// Output a string
//
void OutputString(const char *str)
{
    while (*str)
    {
        OutputByteFunc(*str);
        ++str;
    }
}

//-----------------------------------------------------------------------------
// Output an unsigned integer in decimal
//
void OutputUInt32(uint32_t i)
{
    uint32_t div = 1000000000;
    bool zero = true;

    while (div != 0)
    {
        // Get a digit
        int ch = i / div;
        i = i - (ch * div);
        div = div / 10;

        // Check for a non-zero digit to clear the leading zero flag
        if (ch != 0)
        {
            zero = false;
        }

        // Only output a digit if non-zero, or zero and not a leading zero
        if (!zero)
        {
            OutputByteFunc(ch + '0');
        }
    }

    // Zero will be true at this point if nothing has been output, so output "0"
    if (zero)
    {
        OutputByteFunc('0');
    }
}

//-----------------------------------------------------------------------------
// Output an signed integer in decoimal
//
void OutputInt32(int32_t i)
{
    if (i < 0)
    {
        OutputByteFunc('-');
        i = -i;
    }
    OutputUInt32(i);
}

//-----------------------------------------------------------------------------
// Output a hex value
//
void OutputHex(uint32_t h, int width)
{
    while (width != 0)
    {
        uint8_t ch = (h >> ((width - 1) * 4)) & 0x0F;
        OutputByteFunc("0123456789ABCDEF"[ch]);
        --width;
    }
}

//-----------------------------------------------------------------------------
// Convert integer to string
//
void UintToString(uint32_t value, char* string, unsigned char radix)
{
    uint8_t index = 0;
    uint8_t i = 0;

    // generate the number in reverse order
    do
    {
        string[index] = "0123456789ABCDEF"[value % radix];
        value /= radix;
        ++index;
    } while (value != 0);

    // null terminate the string
    string[index--] = '\0';

    // reverse the order of digits
    while (index > i)
    {
        swap_bytes(string[i], string[index]);
        ++i;
        --index;
    }
}

bool IsDigit(char c)
{
    return (c >= '0') && (c <= '9');
}

uint16_t StringLength(const char *str)
{
    register int i = 0;
    while (*str++)
    {
        ++i;
    }
    return i;
}

uint16_t OutputText(const char *format, ... )
{
    va_list ap;
    union
    {
        uint8_t b[5];
        int32_t l;
        uint32_t ul;
        const char *p;
    } value;
    char c;
    bool justify_left = false;
    bool zero_padding = false;
    bool prefix_sign = false;
    bool prefix_space = false;
    bool signed_argument = false;
    bool long_argument = false;
    bool char_argument = false;
    uint16_t char_count = 0;
    uint8_t radix = 0;
    uint8_t width = 0;
    int8_t decimals = -1;
    uint8_t length = 0;

    va_start(ap, format);
    while (c = *format++)
    {
        if (c != '%')
        {
output_char:
            OutputChar(c);
            ++char_count;
            continue;
        }

        // Reset all flags, counters, etc for a new format specification
        justify_left = false;
        zero_padding = false;
        prefix_sign = false;
        prefix_space = false;
        signed_argument = false;
        long_argument = false;
        char_argument = false;
        radix = 0;
        width = 0;
        decimals = -1;

        // Process format specification
next_format_char:
        c = *format++;
        if (c == '%')
        {
            // Double percent is output as a single %
            goto output_char;
        }

        // Handle width and precision numbers
        if (IsDigit(c))
        {
            if (decimals == -1)
            {
                width = (10 * width) + (c - '0');
                if (width == 0)
                {
                    zero_padding = true;
                }
            }
            else
            {
                decimals = (10 * decimals) + (c - '0');
            }
            goto next_format_char;
        }

        if (c == '.')
        {
            decimals = 0;
            goto next_format_char;
        }

        switch(c)
        {
            case '-':
            {
                justify_left = true;
                goto next_format_char;
            }
            case '+':
            {
                prefix_sign = true;
                goto next_format_char;
            }
            case ' ':
            {
                prefix_space = true;
                goto next_format_char;
            }
            case 'l':
            {
                long_argument = true;
                goto next_format_char;
            }
            case 'd':
            case 'i':
            {
                signed_argument = true;
                radix = 10;
                break;
            }
            case 'p':
            case 'u':
            {
                radix = 10;
                break;
            }
            case 'x':
            {
                radix = 16;
                break;
            }
            case 'b':
            {
                char_argument = true;
                goto next_format_char;
            }
            case 'c':
            {
                if (char_argument)
                {
                    c = va_arg(ap, char);
                }
                else
                {
                    c = va_arg(ap, int);
                }
                OutputChar(c);
                ++char_count;
                break;
            }
            case 's':
            {
                value.p = va_arg(ap, ptr_t);
                length = StringLength(value.p);
                if (decimals == -1)
                {
                    decimals = length;
                }

                // Right justification
                if ((!justify_left) && (length < width))
                {
                    width -= length;
                    while (width != 0)
                    {
                        OutputChar(' ');
                        ++char_count;
                        --width;
                    }
                }

                // String itself
                while ((c = *value.p) && (decimals-- > 0))
                {
                    OutputChar(c);
                    ++char_count;
                    ++value.p;
                }

                // Left justification
                if (justify_left && (length < width))
                {
                    width -= length;
                    while (width != 0)
                    {
                        OutputChar(' ');
                        ++char_count;
                        --width;
                    }
                }
                break;
            }
            default:
            {
                goto output_char;
            }
        }

        if (radix != 0)
        {
            uint8_t store[6];
            uint8_t *pstore = &store[5];
            bool lsd = true;

            // l, p, u, i, d, or b have been used
            if (char_argument)
            {
                value.l = va_arg(ap, char);
                if (!signed_argument)
                {
                    value.l &= 0xFF;
                }
            }
            else if (long_argument)
            {
                value.l = va_arg(ap, long);
            }
            else
            {
                value.l = va_arg(ap, int);
                if (!signed_argument)
                {
                    value.l &= 0xFFFF;
                }
            }

            if (signed_argument)
            {
                if (value.l < 0)
                {
                    value.l = -value.l;
                }
                else
                {
                    signed_argument = false;
                }
            }

            length = 0;
            do
            {
                uint8_t *pb4 = &value.b[4];
                int i = 32;
                value.b[4] = 0;

                do
                {
                    *pb4 = (*pb4 << 1) | ((value.ul >> 31) & 0x01);
                    value.ul <<= 1;
                    if (radix <= *pb4)
                    {
                        *pb4 -= radix;
                        value.ul |= 1;
                    }
                } while (--i);

                if (!lsd)
                {
                    *pstore = (value.b[4] << 4) | (value.b[4] >> 4) | *pstore;
                    --pstore;
                }
                else
                {
                    *pstore = value.b[4];
                }
                ++length;
                lsd = !lsd;
            } while (value.ul != 0);

            if (width == 0)
            {
                width = 1;
            }

            if (!zero_padding && !justify_left)
            {
                while (width > (length + 1))
                {
                    OutputChar(' ');
                    ++char_count;
                    --width;
                }
            }

            if (signed_argument)
            {
                OutputChar('-');
                ++char_count;
                --width;
            }
            else if (length != 0)
            {
                if (prefix_sign)
                {
                    OutputChar('+');
                    ++char_count;
                    --width;
                }
                else if (prefix_space)
                {
                    OutputChar(' ');
                    ++char_count;
                    --width;
                }
            }

            if (!justify_left)
            {
                while (width > length)
                {
                    OutputChar(zero_padding ? '0' : ' ');
                    ++char_count;
                    --width;
                }
            }
            else
            {
                if (width > length)
                {
                    width -= length;
                }
                else
                {
                    width = 0;
                }
            }

            while (length)
            {
                uint8_t d;

                lsd = !lsd;
                if (!lsd)
                {
                    ++pstore;
                    value.b[4] = *pstore >>4;
                }
                else
                {
                    value.b[4] = *pstore & 0x0F;
                }

                d = value.b[4] + '0';
                if (d > '9')
                {
                    d += ('A' - '0' - 10);
                }
                OutputChar(d);
                ++char_count;
                --length;
            }

            if (justify_left)
            {
                while (width > 0)
                {
                    OutputChar(' ');
                    ++char_count;
                    --width;
                }
            }
        }
    }
    return char_count;
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
// Typedefs for API to timer
//
typedef enum
{
    TIM1_ICPSC_DIV1 = TIM1_CCMR_ICxPSC_DIV1,
    TIM1_ICPSC_DIV2 = TIM1_CCMR_ICxPSC_DIV2,
    TIM1_ICPSC_DIV4 = TIM1_CCMR_ICxPSC_DIV4,
    TIM1_ICPSC_DIV8 = TIM1_CCMR_ICxPSC_DIV8
} tim1_ic_prescaler_t;

typedef enum
{
    TIM1_ICPOL_RISING  = 0,
    TIM1_ICPOL_FALLING = 1
} tim1_ic_polarity_t;

typedef enum
{
    TIM1_ICFILT_NONE = TIM1_CCMR_ICxF_NONE,
    TIM1_ICFILT_DIV1_N2 = TIM1_CCMR_ICxF_DIV1_N2,
    TIM1_ICFILT_DIV1_N4 = TIM1_CCMR_ICxF_DIV1_N4,
    TIM1_ICFILT_DIV1_N8 = TIM1_CCMR_ICxF_DIV1_N8,
    TIM1_ICFILT_DIV2_N6 = TIM1_CCMR_ICxF_DIV2_N6,
    TIM1_ICFILT_DIV2_N8 = TIM1_CCMR_ICxF_DIV2_N8,
    TIM1_ICFILT_DIV4_N6 = TIM1_CCMR_ICxF_DIV4_N6,
    TIM1_ICFILT_DIV4_N8 = TIM1_CCMR_ICxF_DIV4_N8,
    TIM1_ICFILT_DIV8_N6 = TIM1_CCMR_ICxF_DIV8_N6,
    TIM1_ICFILT_DIV8_N8 = TIM1_CCMR_ICxF_DIV8_N8,
    TIM1_ICFILT_DIV16_N5 = TIM1_CCMR_ICxF_DIV16_N5,
    TIM1_ICFILT_DIV16_N6 = TIM1_CCMR_ICxF_DIV16_N6,
    TIM1_ICFILT_DIV16_N8 = TIM1_CCMR_ICxF_DIV16_N8,
    TIM1_ICFILT_DIV32_N5 = TIM1_CCMR_ICxF_DIV32_N5,
    TIM1_ICFILT_DIV32_N6 = TIM1_CCMR_ICxF_DIV32_N6,
    TIM1_ICFILT_DIV32_N8 = TIM1_CCMR_ICxF_DIV32_N8
} tim1_ic_filter_t;

//-----------------------------------------------------------------------------
// Set the auto-reload value
//
// Note that High register must be set first.
//
inline void Tim1_SetAutoReload(uint16_t period)
{
    TIM1->ARRH = ((period - 1) >> 8) & 0xFF;
    TIM1->ARRL = ((period - 1) >> 0) & 0xFF;
}

//-----------------------------------------------------------------------------
// Set the prescaler value
//
// Note that High register must be set first.
//
inline void Tim1_SetPrescaler(uint16_t division)
{
    TIM1->PSCRH = ((division - 1) >> 8) & 0xFF;
    TIM1->PSCRL = ((division - 1) >> 0) & 0xFF;
}

//-----------------------------------------------------------------------------
// Set the counter value
//
// Note that High register must be set first.
//
inline void Tim1_SetCounter(uint16_t counter)
{
    TIM1->CCR3H = (counter >> 8) & 0xFF;
    TIM1->CCR3L = (counter >> 0) & 0xFF;
}

//-----------------------------------------------------------------------------
// Disable the timer
//
inline void Tim1_Disable(void)
{
    TIM1->CR1 = (TIM1->CR1 & ~TIM1_CR1_CEN_MASK) | TIM1_CR1_CEN_DISABLE;
}

//-----------------------------------------------------------------------------
// Enable the timer
//
inline void Tim1_Enable(void)
{
    TIM1->CR1 = (TIM1->CR1 & ~TIM1_CR1_CEN_MASK) | TIM1_CR1_CEN_ENABLE;
}

//-----------------------------------------------------------------------------
// Set the prescaler value for capture on channel 1
//
inline void Tim1_SetCapturePrescaler(tim1_ic_prescaler_t prescaler)
{
    TIM1->CCMR1 = (TIM1->CCMR1 & ~TIM1_CCMR_ICxPSC_MASK) | prescaler;
}

//-----------------------------------------------------------------------------
// Wait for the capture flag on channel 1
//
inline void Tim1_WaitCapture1(void)
{
    while ((TIM1->SR1 & ~TIM1_SR1_CC1IF_MATCH) != TIM1_SR1_CC1IF_CAPTURE)
    {
    }
}

//-----------------------------------------------------------------------------
// Get the capture/compare value for channel 1
//
inline uint16_t Tim1_GetCapture1Time(void)
{
    return (TIM1->CCR1H << 8) | TIM1->CCR1L;
}

//-----------------------------------------------------------------------------
// Clear the capture flag for channel 1
//
inline void Tim1_ClearCapture1(void)
{
    TIM1->SR1 = (TIM1->SR1 & ~TIM1_SR1_CC1IF_MATCH) | TIM1_SR1_CC1IF_CLEAR;
}

//-----------------------------------------------------------------------------
// Disable the capture on channel 1
//
inline void Tim1_DisableCapture1(void)
{
    TIM1->CCER1 = (TIM1->CCER1 & ~TIM1_CCER1_CC1E_MASK) | TIM1_CCER1_CC1E_DISABLE;
}

//-----------------------------------------------------------------------------
// Enable the capture on channel 1
//
inline void Tim1_EnableCapture1(void)
{
    TIM1->CCER1 = (TIM1->CCER1 & ~TIM1_CCER1_CC1E_MASK) | TIM1_CCER1_CC1E_ENABLE;
}

//-----------------------------------------------------------------------------
// Set the polarity of capture on channel 1
//
inline void Tim1_SetPolarityCapture1(tim1_ic_polarity_t polarity)
{
    switch (polarity)
    {
        case TIM1_ICPOL_RISING:
        {
            TIM1->CCER1 = (TIM1->CCER1 & ~TIM1_CCER1_CC1P_MASK) | TIM1_CCER1_CC1P_RISING;
            break;
        }
        case TIM1_ICPOL_FALLING:
        {
            TIM1->CCER1 = (TIM1->CCER1 & ~TIM1_CCER1_CC1P_MASK) | TIM1_CCER1_CC1P_FALLING;
            break;
        }
    }
}

//-----------------------------------------------------------------------------
// Set the polarity of capture on channel 1
//
inline void Tim1_SetFilterCapture1(tim1_ic_filter_t filter)
{
    TIM1->CCMR1 = (TIM1->CCMR1 & ~TIM1_CCMR_ICxF_MASK) | filter;
}

//-----------------------------------------------------------------------------
// Configure channel 1 for input
//
inline void Tim1_ConfigCapture1(tim1_ic_polarity_t polarity, tim1_ic_filter_t filter)
{
    Tim1_DisableCapture1();

    /* Select the Input and set the filter */
    TIM1->CCMR1 = (TIM1->CCMR1 & ~TIM1_CCMR_CCxS_MASK) | TIM1_CCMR_CCxS_DIRECT;
    Tim1_SetFilterCapture1(filter);
    Tim1_SetPolarityCapture1(polarity);

    Tim1_EnableCapture1();
}

//-----------------------------------------------------------------------------
// Configure the TIM1 timer
//
void Tim1_Config(void)
{
    // Disable to setup
    Tim1_Disable();

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
                    (TIM1_CCER2_CC3P_HIGH) |
                    (TIM1_CCER2_CC3NE_ENABLE) |
                    (TIM1_CCER2_CC3NP_ENABLE);
    TIM1->CCMR3 = (TIM1->CCMR3 & ~(TIM1_CCMR_OCxM_MASK | TIM1_CCMR_OCxPE_MASK)) | TIM1_CCMR_OCxM_PWM2 | TIM1_CCMR_OCxPE_ENABLE;
    TIM1->OISR &= ~(TIM1_OISR_OIS3_MASK | TIM1_OISR_OIS3N_MASK);
    TIM1->OISR |= (TIM1_OISR_OIS3_ENABLE) | (TIM1_OISR_OIS3N_ENABLE);
    TIM1->CCR3H = (TIM1_CH3_DUTY >> 8) & 0xFF;
    TIM1->CCR3L = (TIM1_CH3_DUTY >> 0) & 0xFF;
    TIM1->BKR |= TIM1_BKR_MOE_ENABLE;

    Tim1_Enable();
}

//-----------------------------------------------------------------------------
// Configure the TIM1 timer for PWM use
//
void Tim1_ConfigPWM(void)
{
    // Disable to setup
    Tim1_Disable();

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
                    (TIM1_CCER2_CC3P_HIGH) |
                    (TIM1_CCER2_CC3NE_ENABLE) |
                    (TIM1_CCER2_CC3NP_ENABLE);
    TIM1->CCMR3 = (TIM1->CCMR3 & ~(TIM1_CCMR_OCxM_MASK | TIM1_CCMR_OCxPE_MASK)) | TIM1_CCMR_OCxM_PWM2 | TIM1_CCMR_OCxPE_ENABLE;
    TIM1->OISR &= ~(TIM1_OISR_OIS3_MASK | TIM1_OISR_OIS3N_MASK);
    TIM1->OISR |= (TIM1_OISR_OIS3_ENABLE) | (TIM1_OISR_OIS3N_ENABLE);
    TIM1->CCR3H = (TIM1_CH3_DUTY >> 8) & 0xFF;
    TIM1->CCR3L = (TIM1_CH3_DUTY >> 0) & 0xFF;
    TIM1->BKR |= TIM1_BKR_MOE_ENABLE;

    Tim1_Enable();
}

//-----------------------------------------------------------------------------
// Disable the TIM4 timer
//
inline void Tim4_Disable(void)
{
    TIM4->CR1 = (TIM4->CR1 & ~TIM4_CR1_CEN_MASK) | TIM4_CR1_CEN_DISABLE;
}

//-----------------------------------------------------------------------------
// Enable the TIM4 timer
//
inline void Tim4_Enable(void)
{
    TIM4->CR1 = (TIM4->CR1 & ~TIM4_CR1_CEN_MASK) | TIM4_CR1_CEN_ENABLE;
}

//-----------------------------------------------------------------------------
// Configure the TIM4 timer
//
void Tim4_Config(uint8_t prescaler, uint8_t reload)
{
    Tim4_Disable();
    TIM4->PSCR = prescaler;
    TIM4->ARR = reload;
    TIM4->SR1 = (TIM4->SR1 & ~TIM4_SR1_UIF_MASK) | TIM4_SR1_UIF_CLEAR;
    TIM4->IER = (TIM4->IER & ~TIM4_IER_UIE_MASK) | TIM4_IER_UIE_ENABLE;
    TIM4->CNTR = 0;
    TIM4->CR1 = (TIM4->CR1 & ~TIM4_CR1_ARPE_MASK) | TIM4_CR1_ARPE_ENABLE;
    Tim4_Enable();
}

//-----------------------------------------------------------------------------
// Configure the TIM4 timer for 1ms interrupts
//
// Setup 8-bit basic timer as 1ms system tick using interrupts to update the tick counter
//
// With 16Mhz HSI prescaler is 128 and reload value is 124. freq = 16000000/128 is 125Khz, therefore reload value is 125 - 1 or 124. Period is 125/125000 which is 0.001s or 1ms
// With 8Mhz HSE prescaler is 64 and reload value is 124. freq is 8000000/64 is 125Khz.
// With 128Khz LSI prescaler is 16 and reload value is 128. freq is 128000/16 is 8Khz.
//
void Tim4_Config1ms(void)
{
    if (SysClock_GetClockFreq() == 16000000)
    {
        Tim4_Config(TIM4_PSCR_DIV128, 124);
    }
    else if (SysClock_GetClockFreq() == 8000000)
    {
        Tim4_Config(TIM4_PSCR_DIV64, 124);
    }
    else if (SysClock_GetClockFreq() == 128000)
    {
        Tim4_Config(TIM4_PSCR_DIV1, 128);
    }
}

//=============================================================================
// Beeper functions
//

typedef enum
{
    BEEP_8KHZ  = 0,      // 128Khz/(8*prescale) -> 500Hz to 8Khz
    BEEP_16KHZ = 1,      // 128Khz/(4*prescale) -> 1Khz to 16Khz
    BEEP_32KHZ = 2       // 128Khz/(2*prescale) -> 2Khz to 32Khz
} beep_freq_t;

typedef enum
{
    BEEP_PRESCALE_2    = 0x00,
    BEEP_PRESCALE_3    = 0x01,
    BEEP_PRESCALE_4    = 0x02,
    BEEP_PRESCALE_5    = 0x03,
    BEEP_PRESCALE_6    = 0x04,
    BEEP_PRESCALE_7    = 0x05,
    BEEP_PRESCALE_8    = 0x06,
    BEEP_PRESCALE_9    = 0x07,
    BEEP_PRESCALE_10   = 0x08,
    BEEP_PRESCALE_11   = 0x09,
    BEEP_PRESCALE_12   = 0x0A,
    BEEP_PRESCALE_13   = 0x0B,
    BEEP_PRESCALE_14   = 0x0C,
    BEEP_PRESCALE_15   = 0x0D,
    BEEP_PRESCALE_16   = 0x0E,
    BEEP_PRESCALE_17   = 0x0F,
    BEEP_PRESCALE_18   = 0x10,
    BEEP_PRESCALE_19   = 0x11,
    BEEP_PRESCALE_20   = 0x12,
    BEEP_PRESCALE_21   = 0x13,
    BEEP_PRESCALE_22   = 0x14,
    BEEP_PRESCALE_23   = 0x15,
    BEEP_PRESCALE_24   = 0x16,
    BEEP_PRESCALE_25   = 0x17,
    BEEP_PRESCALE_26   = 0x18,
    BEEP_PRESCALE_27   = 0x19,
    BEEP_PRESCALE_28   = 0x1A,
    BEEP_PRESCALE_29   = 0x1B,
    BEEP_PRESCALE_30   = 0x1C,
    BEEP_PRESCALE_31   = 0x1D,
    BEEP_PRESCALE_32   = 0x1E
} beep_prescaler_t;

//-----------------------------------------------------------------------------
// Turn on beeper
//
inline void Beep_On(void)
{
    BEEP->CSR = (BEEP->CSR & ~BEEP_CSR_EN_MASK) | BEEP_CSR_EN_ENABLE;
}

//-----------------------------------------------------------------------------
// Turn off beeper
//
inline void Beep_Off(void)
{
    BEEP->CSR = (BEEP->CSR & ~BEEP_CSR_EN_MASK) | BEEP_CSR_EN_DISABLE;
}

//-----------------------------------------------------------------------------
// Toggle state of beeper
//
inline void Beep_Toggle(void)
{
    BEEP->CSR = (BEEP->CSR & ~BEEP_CSR_EN_MASK) | (~BEEP->CSR & BEEP_CSR_EN_MASK);
}

//-----------------------------------------------------------------------------
// Set prescaler for beep
//
inline void Beep_SetPrescaler(beep_prescaler_t prescaler)
{
    BEEP->CSR = (BEEP->CSR & ~BEEP_CSR_DIV_MASK) | prescaler;
}

//-----------------------------------------------------------------------------
// Set frequency of beep
//
void Beep_SetFrequency(beep_freq_t freq)
{
    switch (freq)
    {
        case BEEP_8KHZ:
        {
            BEEP->CSR = (BEEP->CSR & ~BEEP_CSR_SEL_MASK) | BEEP_CSR_SEL_8KHZ;
            break;
        }
        case BEEP_16KHZ:
        {
            BEEP->CSR = (BEEP->CSR & ~BEEP_CSR_SEL_MASK) | BEEP_CSR_SEL_16KHZ;
            break;
        }
        case BEEP_32KHZ:
        {
            BEEP->CSR = (BEEP->CSR & ~BEEP_CSR_SEL_MASK) | BEEP_CSR_SEL_32KHZ;
            break;
        }
    }
}

//-----------------------------------------------------------------------------
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

//-----------------------------------------------------------------------------
// Set the period for auto wakeup
//
void Awu_SetPeriod(uint8_t tbr, uint8_t apr)
{
    AWU->CSR = (AWU->CSR & ~AWU_CSR_AWEN_MASK) | AWU_CSR_AWEN_ENABLE;
    AWU->TBR = (AWU->TBR & ~AWU_TBR_MASK) | tbr;
    AWU->APR = (AWU->APR & ~AWU_APR_DIV_MASK) | apr;
}

//-----------------------------------------------------------------------------
// Set the period for auto wakeup
//
// Sample period have been predefined in the awu_period_t enum
//
void Awu_SetPresetPeriod(awu_period_t period)
{
    Awu_SetPeriod((period >> 0) & 0xFF, (period >> 8) & 0xFF);
}

//-----------------------------------------------------------------------------
// Set the idle mode
//
void Awu_SetIdleMode(void)
{
    AWU->CSR = (AWU->CSR & ~AWU_CSR_AWEN_MASK) | AWU_CSR_AWEN_DISABLE;
    AWU->TBR = (AWU->TBR & ~AWU_TBR_MASK) | AWU_TBR_NONE;
}

//-----------------------------------------------------------------------------
// Disable the auto wakeup functionality
//
void Awu_Disable(void)
{
    AWU->CSR = (AWU->CSR & ~AWU_CSR_AWEN_MASK) | AWU_CSR_AWEN_DISABLE;
}

//-----------------------------------------------------------------------------
// Enable the auto wakeup functionality
//
void Awu_Enable(void)
{
    AWU->CSR = (AWU->CSR & ~ AWU_CSR_AWEN_MASK) | AWU_CSR_AWEN_ENABLE;
}


//-----------------------------------------------------------------------------
//  Measure the LSI frequency using timer IC1 and update the calibration registers.
//
//   It is recommended to use a timer clock frequency of at least 10MHz in order
//   to obtain a better in the LSI frequency measurement.
//
// Two capture samples are taken from the timer and used to calculate the LSI's
// frequency.
//
uint32_t AWU_MeasureLSI(void)
{
    uint32_t lsi_freq_hz = 0;
    uint32_t fmaster;
    uint16_t ICValue1;
    uint16_t ICValue2;

    // Get master frequency
    fmaster = SysClock_GetClockFreq();

    // Enable the LSI measurement: LSI clock connected to timer Input Capture 1
    AWU->CSR = (AWU->CSR & ~ AWU_CSR_MSR_MASK) | AWU_CSR_MSR_ENABLE;

    // Measure the LSI frequency with TIMER Input Capture 1
    // Capture only every 8 events!!!
    // Enable capture of TI1
    Tim1_ConfigCapture1(TIM1_ICPOL_RISING, TIM1_ICFILT_NONE);
    Tim1_SetPrescaler(TIM1_ICPSC_DIV8);

    // TIM1_ICInit(TIM1_CHANNEL_1, TIM1_ICPOLARITY_RISING, TIM1_ICSELECTION_DIRECTTI, TIM1_ICPSC_DIV8, 0);

    Tim1_Enable();

    // Capture first sample
    Tim1_WaitCapture1();
    ICValue1 = Tim1_GetCapture1Time();
    Tim1_ClearCapture1();

    // Capture second sample
    Tim1_WaitCapture1();
    ICValue2 = Tim1_GetCapture1Time();
    Tim1_ClearCapture1();

    Tim1_DisableCapture1();
    Tim1_Disable();

    // Compute LSI clock frequency
    lsi_freq_hz = (8 * fmaster) / (ICValue2 - ICValue1);

    // Disable the LSI measurement: LSI clock disconnected from timer Input Capture 1
    AWU->CSR = (AWU->CSR & ~AWU_CSR_MSR_MASK) | AWU_CSR_MSR_DISABLE;

    return lsi_freq_hz;
}

//=============================================================================
// Watchdog functions
//
//
typedef enum
{
    IWDG_PERIOD_16MS    = 0x00,     // 15.9ms with RL of 0xFF
    IWDG_PERIOD_32MS    = 0x01,     // 31.9ms with RL of 0xFF
    IWDG_PERIOD_63MS    = 0x02,     // 63.7ms with RL of 0xFF
    IWDG_PERIOD_127MS   = 0x03,     // 127ms with RL of 0xFF
    IWDG_PERIOD_255MS   = 0x04,     // 255ms with RL of 0xFF
    IWDG_PERIOD_510MS   = 0x05,     // 510ms with RL of 0xFF
    IWDG_PERIOD_1S      = 0x06      // 1.02s with RL of 0xFF
} iwdg_period_t;

//-----------------------------------------------------------------------------
// Initialise the independent watchdog
//
void Iwdg_Init(iwdg_period_t period)
{
    IWDG->KR = IWDG_KR_KEY_ACCESS;
    IWDG->PR = (IWDG->PR & ~IWDG_PR_PR_MASK) | period;
    IWDG->RLR = 0xFF;
    IWDG->KR = IWDG_KR_KEY_ENABLE;
}

//-----------------------------------------------------------------------------
// Refresh the reload register (feed the dog)
//
void Iwdg_Refresh(void)
{
    IWDG->KR = IWDG_KR_KEY_REFRESH;
}

//-----------------------------------------------------------------------------
// Initialise the window watchdog
//
void Wwdg_Init(void)
{
    WWDG->CR = (WWDG->CR & ~(WWDG_CR_WDGA_MASK | WWDG_CR_T_MASK)) | WWDG_CR_WDGA_ENABLE | WWDG_CR_T_MAX;
}

//-----------------------------------------------------------------------------
// Refresh the downcounter register (feed the dog)
//
void Wwdg_Refresh(void)
{
    WWDG->CR = WWDG_CR_T_MAX;
}

//-----------------------------------------------------------------------------
// Return the period between refreshes before a reset occurs
//
uint16_t Wwdg_Period(void)
{
    uint16_t period = SysClock_GetClockFreq() * 12288;
    return (period * WWDG_CR_T_MAX) - (period * WWDG_CR_T_MIN);
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
void Systick_Init(void)
{
    Tim4_Config1ms();
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
#if defined __IAR_SYSTEMS_ICC__
#pragma vector=23
#endif
INTERRUPT(TIM4_UPD_OVF_IRQHandler, 23)
{
    ++systick;

    // Clear Interrupt Pending bit
    TIM4->SR1 = (TIM4->SR1 & ~TIM4_SR1_UIF_MASK) | TIM4_SR1_UIF_CLEAR;
}


//=============================================================================
// I2C functions
//
//

// Use when clock is 10Mhz
// MSbit is the duty cycle, rest is the CCR value
/*typedef enum
{
    I2C_SPEED_400KHZ    = 0x8001,
    I2C_SPEED_370KHZ    = 0x0009,
    I2C_SPEED_350KHZ    = 0x0009,
    I2C_SPEED_320KHZ    = 0x000A,
    I2C_SPEED_300KHZ    = 0x000B,
    I2C_SPEED_270KHZ    = 0x000C,
    I2C_SPEED_250KHZ    = 0x000D,
    I2C_SPEED_220KHZ    = 0x000F,
    I2C_SPEED_200KHZ    = 0x8002,
    I2C_SPEED_170KHZ    = 0x0013,
    I2C_SPEED_150KHZ    = 0x0016,
    I2C_SPEED_120KHZ    = 0x001B,
    I2C_SPEED_100KHZ    = 0x0032,
    I2C_SPEED_50KHZ     = 0x0064,
    I2C_SPEED_30KHZ     = 0x00A6,
    I2C_SPEED_20KHZ     = 0x00FA
} i2c_speed_t;
*/

typedef enum
{
    I2C_SPEED_STANDARD,
    I2C_SPEED_FULL
} i2c_speed_t;

// Use when clock is 16Mhz
// MSbit is the duty cycle, rest is the CCR value
typedef enum
{
    I2C_FREQUENCY_400KHZ    = 0x000D,
    I2C_FREQUENCY_370KHZ    = 0x000E,
    I2C_FREQUENCY_350KHZ    = 0x000F,
    I2C_FREQUENCY_320KHZ    = 0x8002,
    I2C_FREQUENCY_300KHZ    = 0x0011,
    I2C_FREQUENCY_270KHZ    = 0x0013,
    I2C_FREQUENCY_250KHZ    = 0x0015,
    I2C_FREQUENCY_220KHZ    = 0x0018,
    I2C_FREQUENCY_200KHZ    = 0x001A,
    I2C_FREQUENCY_170KHZ    = 0x001F,
    I2C_FREQUENCY_150KHZ    = 0x0023,
    I2C_FREQUENCY_120KHZ    = 0x002C,
    I2C_FREQUENCY_100KHZ    = 0x0050,
    I2C_FREQUENCY_50KHZ     = 0x00A0,
    I2C_FREQUENCY_30KHZ     = 0x010A,
    I2C_FREQUENCY_20KHZ     = 0x0190
} i2c_frequency_t;

typedef enum
{
    I2C_DIRECTION_WRITE = 0,
    I2C_DIRECTION_READ  = 1
} i2c_direction_t;

/**
  * @brief  I2C Flags
  * @brief Elements values convention: 0xXXYY
  *  X = SRx registers index
  *      X = 1 : SR1
  *      X = 2 : SR2
  *      X = 3 : SR3
  *  Y = Flag mask in the register
  */
typedef enum
{
  /* SR1 register flags */
  I2C_FLAG_TXEMPTY             = (uint16_t)0x0180,  /*!< Transmit Data Register Empty flag */
  I2C_FLAG_RXNOTEMPTY          = (uint16_t)0x0140,  /*!< Read Data Register Not Empty flag */
  I2C_FLAG_STOPDETECTION       = (uint16_t)0x0110,  /*!< Stop detected flag */
  I2C_FLAG_HEADERSENT          = (uint16_t)0x0108,  /*!< 10-bit Header sent flag */
  I2C_FLAG_TRANSFERFINISHED    = (uint16_t)0x0104,  /*!< Data Byte Transfer Finished flag */
  I2C_FLAG_ADDRESSSENTMATCHED  = (uint16_t)0x0102,  /*!< Address Sent/Matched (master/slave) flag */
  I2C_FLAG_STARTDETECTION      = (uint16_t)0x0101,  /*!< Start bit sent flag */

  /* SR2 register flags */
  I2C_FLAG_WAKEUPFROMHALT      = (uint16_t)0x0220,  /*!< Wake Up From Halt Flag */
  I2C_FLAG_OVERRUNUNDERRUN     = (uint16_t)0x0208,  /*!< Overrun/Underrun flag */
  I2C_FLAG_ACKNOWLEDGEFAILURE  = (uint16_t)0x0204,  /*!< Acknowledge Failure Flag */
  I2C_FLAG_ARBITRATIONLOSS     = (uint16_t)0x0202,  /*!< Arbitration Loss Flag */
  I2C_FLAG_BUSERROR            = (uint16_t)0x0201,  /*!< Misplaced Start or Stop condition */

  /* SR3 register flags */
  I2C_FLAG_GENERALCALL         = (uint16_t)0x0310,  /*!< General Call header received Flag */
  I2C_FLAG_TRANSMITTERRECEIVER = (uint16_t)0x0304,  /*!< Transmitter Receiver Flag */
  I2C_FLAG_BUSBUSY             = (uint16_t)0x0302,  /*!< Bus Busy Flag */
  I2C_FLAG_MASTERSLAVE         = (uint16_t)0x0301   /*!< Master Slave Flag */
} I2C_Flag_TypeDef;

/**
  * @brief I2C Pending bits
  * Elements values convention: 0xXYZZ
  *  X = SRx registers index
  *      X = 1 : SR1
  *      X = 2 : SR2
  *  Y = Position of the corresponding Interrupt
  *  ZZ = flag mask in the dedicated register(X register)
  */

typedef enum
{
    /* SR1 register flags */
    I2C_ITPENDINGBIT_TXEMPTY             = (uint16_t)0x1680,    /*!< Transmit Data Register Empty  */
    I2C_ITPENDINGBIT_RXNOTEMPTY          = (uint16_t)0x1640,    /*!< Read Data Register Not Empty  */
    I2C_ITPENDINGBIT_STOPDETECTION       = (uint16_t)0x1210,    /*!< Stop detected  */
    I2C_ITPENDINGBIT_HEADERSENT          = (uint16_t)0x1208,    /*!< 10-bit Header sent */
    I2C_ITPENDINGBIT_TRANSFERFINISHED    = (uint16_t)0x1204,    /*!< Data Byte Transfer Finished  */
    I2C_ITPENDINGBIT_ADDRESSSENTMATCHED  = (uint16_t)0x1202,    /*!< Address Sent/Matched (master/slave)  */
    I2C_ITPENDINGBIT_STARTDETECTION      = (uint16_t)0x1201,    /*!< Start bit sent  */

    /* SR2 register flags */
    I2C_ITPENDINGBIT_WAKEUPFROMHALT      = (uint16_t)0x2220,    /*!< Wake Up From Halt  */
    I2C_ITPENDINGBIT_OVERRUNUNDERRUN     = (uint16_t)0x2108,    /*!< Overrun/Underrun  */
    I2C_ITPENDINGBIT_ACKNOWLEDGEFAILURE  = (uint16_t)0x2104,    /*!< Acknowledge Failure  */
    I2C_ITPENDINGBIT_ARBITRATIONLOSS     = (uint16_t)0x2102,    /*!< Arbitration Loss  */
    I2C_ITPENDINGBIT_BUSERROR            = (uint16_t)0x2101     /*!< Misplaced Start or Stop condition */
} I2C_ITPendingBit_TypeDef;

/**
  * @brief I2C possible events
  * Values convention: 0xXXYY
  * XX = Event SR3 corresponding value
  * YY = Event SR1 corresponding value
  * @note if Event = EV3_2 the rule above does not apply
  * YY = Event SR2 corresponding value
  */

typedef enum
{
  /*========================================
                       I2C Master Events (Events grouped in order of communication)
                                                          ==========================================*/
  /**
    * @brief  Communication start
    *
    * After sending the START condition (I2C_GenerateSTART() function) the master
    * has to wait for this event. It means that the Start condition has been correctly
    * released on the I2C bus (the bus is free, no other devices is communicating).
    *
    */
  /* --EV5 */
  I2C_EVENT_MASTER_MODE_SELECT               = (uint16_t)0x0301,  /*!< BUSY, MSL and SB flag */

  /**
    * @brief  Address Acknowledge
    *
    * After checking on EV5 (start condition correctly released on the bus), the
    * master sends the address of the slave(s) with which it will communicate
    * (I2C_Send7bitAddress() function, it also determines the direction of the communication:
    * Master transmitter or Receiver).
    * Then the master has to wait that a slave acknowledges his address.
    * If an acknowledge is sent on the bus, one of the following events will
    * be set:
    *
    *  1) In case of Master Receiver (7-bit addressing):
    *  the I2C_EVENT_MASTER_RECEIVER_MODE_SELECTED event is set.
    *
    *  2) In case of Master Transmitter (7-bit addressing):
    *  the I2C_EVENT_MASTER_TRANSMITTER_MODE_SELECTED is set
    *
    *  3) In case of 10-Bit addressing mode, the master (just after generating the START
    *  and checking on EV5) has to send the header of 10-bit addressing mode (I2C_SendData()
    *  function).
    *  Then master should wait on EV9. It means that the 10-bit addressing
    *  header has been correctly sent on the bus.
    *  Then master should send the second part of the 10-bit address (LSB) using
    *  the function I2C_Send7bitAddress(). Then master should wait for event EV6.
    *
    */
  /* --EV6 */
  I2C_EVENT_MASTER_TRANSMITTER_MODE_SELECTED = (uint16_t)0x0782,  /*!< BUSY, MSL, ADDR, TXE and TRA flags */
  I2C_EVENT_MASTER_RECEIVER_MODE_SELECTED    = (uint16_t)0x0302,  /*!< BUSY, MSL and ADDR flags */
  /* --EV9 */
  I2C_EVENT_MASTER_MODE_ADDRESS10            = (uint16_t)0x0308,  /*!< BUSY, MSL and ADD10 flags */

  /**
    * @brief Communication events
    *
    * If a communication is established (START condition generated and slave address
    * acknowledged) then the master has to check on one of the following events for
    * communication procedures:
    *
    * 1) Master Receiver mode: The master has to wait on the event EV7 then to read
    *    the data received from the slave (I2C_ReceiveData() function).
    *
    * 2) Master Transmitter mode: The master has to send data (I2C_SendData()
    *    function) then to wait on event EV8 or EV8_2.
    *    These two events are similar:
    *     - EV8 means that the data has been written in the data register and is
    *       being shifted out.
    *     - EV8_2 means that the data has been physically shifted out and output
    *       on the bus.
    *     In most cases, using EV8 is sufficient for the application.
    *     Using EV8_2 leads to a slower communication but ensures more reliable test.
    *     EV8_2 is also more suitable than EV8 for testing on the last data transmission
    *     (before Stop condition generation).
    *
    *  @note In case the user software does not guarantee that this event EV7 is
    *  managed before the current byte end of transfer, then user may check on EV7
    *  and BTF flag at the same time (ie. (I2C_EVENT_MASTER_BYTE_RECEIVED | I2C_FLAG_BTF)).
    *  In this case the communication may be slower.
    *
    */
  /* Master RECEIVER mode -----------------------------*/
  /* --EV7 */
  I2C_EVENT_MASTER_BYTE_RECEIVED             = (uint16_t)0x0340,  /*!< BUSY, MSL and RXNE flags */

  /* Master TRANSMITTER mode --------------------------*/
  /* --EV8 */
  I2C_EVENT_MASTER_BYTE_TRANSMITTING         = (uint16_t)0x0780,  /*!< TRA, BUSY, MSL, TXE flags */
  /* --EV8_2 */

  I2C_EVENT_MASTER_BYTE_TRANSMITTED          = (uint16_t)0x0784,  /*!< EV8_2: TRA, BUSY, MSL, TXE and BTF flags */


  /*========================================
                       I2C Slave Events (Events grouped in order of communication)
                                                          ==========================================*/

  /**
    * @brief  Communication start events
    *
    * Wait on one of these events at the start of the communication. It means that
    * the I2C peripheral detected a Start condition on the bus (generated by master
    * device) followed by the peripheral address.
    * The peripheral generates an ACK condition on the bus (if the acknowledge
    * feature is enabled through function I2C_AcknowledgeConfig()) and the events
    * listed above are set :
    *
    * 1) In normal case (only one address managed by the slave), when the address
    *   sent by the master matches the own address of the peripheral (configured by
    *   I2C_OwnAddress1 field) the I2C_EVENT_SLAVE_XXX_ADDRESS_MATCHED event is set
    *   (where XXX could be TRANSMITTER or RECEIVER).
    *
    * 2) In case the address sent by the master is General Call (address 0x00) and
    *   if the General Call is enabled for the peripheral (using function I2C_GeneralCallCmd())
    *   the following event is set I2C_EVENT_SLAVE_GENERALCALLADDRESS_MATCHED.
    *
    */

  /* --EV1  (all the events below are variants of EV1) */
  /* 1) Case of One Single Address managed by the slave */
  I2C_EVENT_SLAVE_RECEIVER_ADDRESS_MATCHED    = (uint16_t)0x0202,  /*!< BUSY and ADDR flags */
  I2C_EVENT_SLAVE_TRANSMITTER_ADDRESS_MATCHED = (uint16_t)0x0682,  /*!< TRA, BUSY, TXE and ADDR flags */

  /* 2) Case of General Call enabled for the slave */
  I2C_EVENT_SLAVE_GENERALCALLADDRESS_MATCHED  = (uint16_t)0x1200,  /*!< EV2: GENCALL and BUSY flags */

  /**
    * @brief  Communication events
    *
    * Wait on one of these events when EV1 has already been checked :
    *
    * - Slave RECEIVER mode:
    *     - EV2: When the application is expecting a data byte to be received.
    *     - EV4: When the application is expecting the end of the communication:
    *       master sends a stop condition and data transmission is stopped.
    *
    * - Slave Transmitter mode:
    *    - EV3: When a byte has been transmitted by the slave and the application
    *      is expecting the end of the byte transmission.
    *      The two events I2C_EVENT_SLAVE_BYTE_TRANSMITTED and I2C_EVENT_SLAVE_BYTE_TRANSMITTING
    *      are similar. The second one can optionally be used when the user software
    *      doesn't guarantee the EV3 is managed before the current byte end of transfer.
    *    - EV3_2: When the master sends a NACK in order to tell slave that data transmission
    *      shall end (before sending the STOP condition).
    *      In this case slave has to stop sending data bytes and expect a Stop
    *      condition on the bus.
    *
    *  @note In case the  user software does not guarantee that the event EV2 is
    *  managed before the current byte end of transfer, then user may check on EV2
    *  and BTF flag at the same time (ie. (I2C_EVENT_SLAVE_BYTE_RECEIVED | I2C_FLAG_BTF)).
    *  In this case the communication may be slower.
    *
    */
  /* Slave RECEIVER mode --------------------------*/
  /* --EV2 */
  I2C_EVENT_SLAVE_BYTE_RECEIVED              = (uint16_t)0x0240,  /*!< BUSY and RXNE flags */
  /* --EV4  */
  I2C_EVENT_SLAVE_STOP_DETECTED              = (uint16_t)0x0010,  /*!< STOPF flag */

  /* Slave TRANSMITTER mode -----------------------*/
  /* --EV3 */
  I2C_EVENT_SLAVE_BYTE_TRANSMITTED           = (uint16_t)0x0684,  /*!< TRA, BUSY, TXE and BTF flags */
  I2C_EVENT_SLAVE_BYTE_TRANSMITTING          = (uint16_t)0x0680,  /*!< TRA, BUSY and TXE flags */
  /* --EV3_2 */
  I2C_EVENT_SLAVE_ACK_FAILURE                = (uint16_t)0x0004  /*!< AF flag */
} I2C_Event_TypeDef;

//=============================================================================
// Calculate CCR values
//
// This function will calculate the CCR and DUTY values for a given peripheral
// clock frequency and requested I2C data rate. The function works by iteratively
// going through all potential CCR/DUTY values and finding the one which is the
// closest to the requested speed.
//
// The system clock in Mhz is passed in freq and the requested speed is passed
// in speed. The CCR & DUTY values that produce an actual speed with the smallest
// margin of error are returned.

// Calculation to deduce CCR from a requested frequency is:
// 1/((CCR/freq)) * [2|3|25]
//
// Examples:
// 1/((1/10Mhz) * 25)
// 1/(0.0000001 * 25)
// 1/0.0000025 => 400000
//
// 1/((9/10Mhz) * 3)
// 1/(0.0000009 * 3)
// 1/0.0000027 => 370370.3703
//
// 1/((13/16Mhz) * 3)
// 1/(0.000000812 * 3)
// 1/0.000002438 => 410256.4102
// Error is 410256-400000 => 10256/400000 = 2.56%
//
// 1/((14/freq) * 3)
// 1/((14/16000000) * 3)
// 1/(0.000000875 * 3)
// 1/0.000002625 => 380952.3809
// Error is 380952-370000  => 10952/370000 = 2.96%
//
// Using integers only:
// 1000000000 / (((CCR * 1000) / Mhz) * DUTY)
//
// Example calculations:
// 1000000000 / ((1000/10) * 25) => 400000 => 400Khz
// 1000000000 / ((9000/10) * 30) => 370370 => 370Khz
// 1000000000 / ((14000/16) * 30) => 380952 => 370Khz
// 1000000000 / ((20000/16) * 25) => 320000 => 320Khz
// 1000000000 / ((250000/10) * 2) => 20000 => 20Khz
//

// Calculate actual speed for a given clock frequency and ccr/duty values
static uint32_t CalcSpeed(uint8_t freq, uint32_t ccr, uint8_t duty)
{
    return 1000000000UL / (((ccr * 1000UL) / freq) * (uint32_t)(duty));
}

// Calculate margin of error for actual speed against requested speed
// Returns value accurate to two decimal places but as integer so needs dividing by 100.
static uint32_t CalcError(uint32_t req, uint32_t act)
{
    if (req > act)
    {
        return ((req - act) * 10000) / req;
    }
    return ((act - req) * 10000) / req;
}

void I2C_CalculateCCR(uint8_t freq, uint32_t speed, uint16_t *ccr, uint8_t *duty, uint32_t *actual, uint32_t *error)
{
    uint32_t prev_speed_0 = 0;
    uint32_t calc_speed_0;
    uint32_t calc_speed_1;
    uint32_t i;

    if (speed > 100000)
    {
        // Fast mode speeds, duty used in this mode so two calculations need to be carried out
        for (i = 1; i < 4096; ++i)
        {
            calc_speed_0 = CalcSpeed(freq, i, 3);       // Duty 0 (2:1)
            calc_speed_1 = CalcSpeed(freq, i, 25);      // Duty 1 (16:9)
            if (calc_speed_0 == speed)
            {
                *duty = 0;
                *ccr = i;
                *actual = calc_speed_0;
                *error = 0;
                return;
            }
            if (calc_speed_1 == speed)
            {
                *duty = 1;
                *ccr = i;
                *actual = calc_speed_1;
                *error = 0;
                return;
            }
            if (calc_speed_0 < speed)
            {
                // Calculated speed lower than requested, find which is closer,
                // the current speed or the previously calculated speed.
                //
                // Duty is nearly always 0 in this case, so assume 0.
                // TODO: Find out if this is case with duty=0 at all clock frequencies.
                *duty = 0;
                if ((prev_speed_0 - speed) > (speed - calc_speed_0))
                {
                    // Currently calculated speed is closer
                    *error = CalcError(speed, calc_speed_0);
                    *actual = calc_speed_0;
                    *ccr = i;
                }
                else
                {
                    // Previously calculated speed is closer
                    *error = CalcError(speed, prev_speed_0);
                    *actual = prev_speed_0;
                    *ccr = i - 1;
                }
                return;
            }
            prev_speed_0 = calc_speed_0;
        }
    }
    else
    {
        // Standard mode speeds, duty is ignored in this mode
        for (i = 1; i < 4096; ++i)
        {
            calc_speed_0 = CalcSpeed(freq, i, 2);       // 1:1 duty cycle
            if (calc_speed_0 == speed)
            {
                *duty = 0;
                *ccr = i;
                *actual = calc_speed_0;
                *error = 0;
                return;
            }
            if (calc_speed_0 < speed)
            {
                *duty = 0;
                if ((prev_speed_0 - speed) > (speed - calc_speed_0))
                {
                    // Currently calculated speed is closer
                    *error = CalcError(speed, calc_speed_0);
                    *actual = calc_speed_0;
                    *ccr = i;
                }
                else
                {
                    // Previously calculated speed is closer
                    *error = CalcError(speed, prev_speed_0);
                    *actual = prev_speed_0;
                    *ccr = i - 1;
                }
                return;
            }
            prev_speed_0 = calc_speed_0;
        }
    }

    // No matching settings found, probably due to clock frequency being too low
    // for the requested speed.
    *ccr = 0;
    *duty = 0;
    *actual = 0;
    *error = 0;
}

//-----------------------------------------------------------------------------
// Disable the I2C peripheral
//
void I2C_Disable(void)
{
    I2C->CR1 = (I2C->CR1 & ~I2C_CR1_PE_MASK) | I2C_CR1_PE_DISABLE;
}

//-----------------------------------------------------------------------------
// Enable the I2C peripheral
//
void I2C_Enable(void)
{
    I2C->CR1 = (I2C->CR1 & ~I2C_CR1_PE_MASK) | I2C_CR1_PE_ENABLE;
}

//-----------------------------------------------------------------------------
// Disable start condition
//
void I2C_DisbleStart(void)
{
    I2C->CR2 = (I2C->CR2 & ~I2C_CR2_START_MASK) | I2C_CR2_START_DISABLE;
}

//-----------------------------------------------------------------------------
// Enable start condition
//
void I2C_EnableStart(void)
{
    I2C->CR2 = (I2C->CR2 & ~I2C_CR2_START_MASK) | I2C_CR2_START_ENABLE;
}

//-----------------------------------------------------------------------------
// Disable stop condition
//
void I2C_DisableStop(void)
{
    I2C->CR2 = (I2C->CR2 & ~I2C_CR2_STOP_MASK) | I2C_CR2_STOP_DISABLE;
}

//-----------------------------------------------------------------------------
// Enable stop condition
//
void I2C_EnableStop(void)
{
    I2C->CR2 = (I2C->CR2 & ~I2C_CR2_STOP_MASK) | I2C_CR2_STOP_ENABLE;
}

//-----------------------------------------------------------------------------
// Disable sending ACK
//
void I2C_DisableACK(void)
{
    I2C->CR2 = (I2C->CR2 & ~I2C_CR2_ACK_MASK) | I2C_CR2_ACK_DISABLE;
}

//-----------------------------------------------------------------------------
// Enable sending ACK condition
//
void I2C_EnableACK(void)
{
    I2C->CR2 = (I2C->CR2 & ~I2C_CR2_ACK_MASK) | I2C_CR2_ACK_ENABLE;
}

//-----------------------------------------------------------------------------
// Perform a software reset of the peripheral
//
// This is used if the peripheral gets stuck in the busy state due to
// a misbehaving slave device or problems on the line.
//
void I2C_SoftwareReset(void)
{
    I2C->CR2 = (I2C->CR2 & ~I2C_CR2_SWRST_MASK) | I2C_CR2_SWRST_RESET;
    // TODO: Any delay required?
    I2C->CR2 = (I2C->CR2 & ~I2C_CR2_SWRST_MASK) | I2C_CR2_SWRST_RUNNING;
}

//-----------------------------------------------------------------------------
// Disable clock stretching
//
void I2C_DisableClockStretch(void)
{
    I2C->CR1 = (I2C->CR1 & ~I2C_CR1_NOSTRETCH_MASK) | I2C_CR1_NOSTRETCH_DISABLE;
}

//-----------------------------------------------------------------------------
// Enable clock stretching
//
void I2C_EnableClockStretch(void)
{
    I2C->CR1 = (I2C->CR1 & ~I2C_CR1_NOSTRETCH_MASK) | I2C_CR1_NOSTRETCH_ENABLE;
}

//-----------------------------------------------------------------------------
// Check bus busy state
//
// Note that this reads SR3 and reading this register after SR1 has been read
// clears the ADDR flag in SR1. Therefore only call this function before any
// communications is started, do not use in the middle of any transaction.
//
bool I2C_BusBusy(void)
{
    return (I2C->SR3 & I2C_SR3_BUSY_MASK) == I2C_SR3_BUSY_ONGOING;
}

//-----------------------------------------------------------------------------
// Initialise the I2C peripheral
//
// Note that a system clock frequency multiple of 10Mhz is needed as this
// satisfies the requirement for fast mode at 400Khz.
//
void I2C_Init(i2c_speed_t speed, uint8_t freq, uint16_t ccr, uint8_t duty)
{
    I2C_Disable();
    I2C->FREQR = freq;
    I2C->CCRL = (ccr >> 0) & I2C_CCRL_CCR_MASK;
    I2C->CCRH = (ccr >> 8) & I2C_CCRH_CCR_MASK;
    I2C->CCRH = (I2C->CCRH & ~I2C_CCRH_FS_MASK) | speed;
    I2C->CCRH = (I2C->CCRH & ~I2C_CCRH_DUTY_MASK) | duty;
    I2C->OARH = (I2C->OARH & ~(I2C_OARH_ADDMODE_MASK | I2C_OARH_ADDCONF_MASK)) | I2C_OARH_ADDMODE_7BIT | I2C_OARH_ADDCONF;
    if (speed == I2C_SPEED_STANDARD)
    {
        I2C->TRISER = I2C->FREQR + 1;
    }
    else
    {
        I2C->TRISER = ((I2C->FREQR * 3) / 10) + 1;;
    }
    I2C_Enable();
}

//-----------------------------------------------------------------------------
// Configure for standard mode I2C comms master
//
// Assumes 7-bit addressing, default acknowledgement
//
void I2C_ConfigStdModeMaster(void)
{
    I2C_EnableACK();
    I2C_EnableStart();
}

//-----------------------------------------------------------------------------
// Read a byte from the data register
//
bool I2C_ReceiveData(uint8_t *data)
{
    uint16_t timeout;

    I2C->CR2 = (I2C->CR2 & ~ I2C_CR2_ACK_MASK) | I2C_CR2_ACK_ENABLE;
    while ((I2C->SR1 & I2C_SR1_RXNE_MASK) == I2C_SR1_RXNE_EMPTY)
    {
        if (Systick_Timeout(&timeout, 100))
        {
            return false;
        }
    }
    *data = I2C->DR;
    return true;
}

//-----------------------------------------------------------------------------
// Write a byte to the data register
//
bool I2C_SendData(uint8_t data)
{
    uint16_t timeout;

    I2C->DR = data;
    while ((I2C->SR1 & I2C_SR1_TXE_MASK) == I2C_SR1_TXE_NOT_EMPTY)
    {
        if (Systick_Timeout(&timeout, 100))
        {
            return false;
        }
    }
    return true;
}

//-----------------------------------------------------------------------------
// Send address to slave device
//
// This is for 7-bit addressing in the range 0-7F. The address is shifted
// internally to make space for the r/w flag.
//
bool I2C_SendAddress(uint8_t addr, i2c_direction_t dir)
{
    uint16_t timeout;

    I2C->DR = (addr << 1) | dir;
    while ((I2C->SR1 & I2C_SR1_ADDR_MASK) == I2C_SR1_ADDR_NOT_END_OF_TX)
    {
        if (Systick_Timeout(&timeout, 100))
        {
            return false;
        }
    }
    (void)I2C->SR3; // Clear EV6
    I2C->CR2 = (I2C->CR2 & ~I2C_CR2_ACK_MASK) | I2C_CR2_ACK_ENABLE;
    return true;
}

//-----------------------------------------------------------------------------
// Set the start condition
//
bool I2C_Start(void)
{
    uint16_t timeout;

    I2C->CR2 = (I2C->CR2 & ~I2C_CR2_START_MASK) | I2C_CR2_START_ENABLE;
    while ((I2C->SR1 & I2C_SR1_SB_MASK) == I2C_SR1_SB_NOT_DONE)
    {
        if (Systick_Timeout(&timeout, 100))
        {
            return false;
        }
    }
    return true;
}

//-----------------------------------------------------------------------------
// Set the stop condition
//
bool I2C_Stop(void)
{
    uint16_t timeout;

    I2C->CR2 = (I2C->CR2 & ~I2C_CR2_STOP_MASK) | I2C_CR2_STOP_ENABLE;
    while ((I2C->SR3 & I2C_SR3_MSL_MASK) == I2C_SR3_MSL_MASTER)
    {
        if (Systick_Timeout(&timeout, 100))
        {
            return false;
        }
    }
    return true;
}

//-----------------------------------------------------------------------------
// Transmit a block of data to the slave device
//
void I2C_Transmit(uint8_t address, const uint8_t *data, uint8_t len)
{

}

//-----------------------------------------------------------------------------
// Receive a block of data to the slave device
//
uint8_t I2C_Receive(uint8_t address, const uint8_t *data, uint8_t len)
{
	return 0;
}

//=============================================================================
// GPIO functions
//

//-----------------------------------------------------------------------------
// Configure the GPIOs
//
void Gpio_Config(void)
{
    // LED
    GPIOD->DDR |= GPIO_DDR_7_OUTPUT;
    GPIOD->CR1 |= GPIO_CR1_7_OUTPUT_PUSHPULL;
    GPIOD->CR2 |= GPIO_CR2_7_OUTPUT_2MHZ;

    // BEEPER
    GPIOD->DDR |= GPIO_DDR_4_OUTPUT;
    GPIOD->CR1 |= GPIO_CR1_4_OUTPUT_PUSHPULL;
    GPIOD->CR2 |= GPIO_CR2_4_OUTPUT_2MHZ;

    // I2C
    GPIOB->DDR |= GPIO_DDR_4_OUTPUT;
    GPIOB->CR1 |= GPIO_CR1_4_OUTPUT_OPENDRAIN;
    GPIOB->CR2 |= GPIO_CR2_4_OUTPUT_2MHZ;

    GPIOB->DDR |= GPIO_DDR_5_OUTPUT;
    GPIOB->CR1 |= GPIO_CR1_5_OUTPUT_OPENDRAIN;
    GPIOB->CR2 |= GPIO_CR2_5_OUTPUT_2MHZ;
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

void TestI2CSpeeds(uint8_t clock)
{
    static const uint32_t speed[] =
    {
        400000,
        370000,
        350000,
        320000,
        300000,
        270000,
        250000,
        220000,
        200000,
        170000,
        150000,
        120000,
        100000,
        50000,
        30000,
        20000
    };
    int i;
    uint16_t ccr;
    uint8_t duty;
    uint32_t actual;

    OutputText("Clock=%iMhz\r\n", clock);
    for (i = 0; i < sizeof(speed) / sizeof(uint32_t); ++i)
    {
        uint32_t error;
        I2C_CalculateCCR(clock, speed[i], &ccr, &duty, &actual, &error);
        OutputText("Req=%6lu, Actual=%6lu, Error=%4lu.%02lu, CCR=%4i, Duty=%1i\r\n", speed[i], actual, (error / 100), error - ((error / 100) * 100), ccr, duty);
    }
}

uint8_t txbuffer[32];
circular_buffer_t txbuf;

void main(void)
{
#ifdef FADER
    uint16_t fade = 0;
    uint16_t fader = 0;
    uint8_t up = 0;
#endif
#ifdef FLASHER
    uint16_t flasher = 0;
    uint8_t flash = 0;
#endif
#ifdef SERIALIZER
    uint16_t transmitter = 0;
#endif
#ifdef BEEPER
    beep_prescaler_t pre = BEEP_PRESCALE_2;
    beep_freq_t freq = BEEP_8KHZ;
    uint16_t beeper;
#endif
#ifdef SQUARER
    uint16_t squarer = 0;
#endif
    uint32_t lsi_freq = 0;
    uint16_t ccr;
    uint8_t duty;

    SysClock_HSI();
    Systick_Init();
    //lsi_freq = AWU_MeasureLSI();
    Tim1_ConfigPWM();
    Gpio_Config();
    CircBuf_Init(&txbuf, txbuffer, 32);
    Uart2_Init(&txbuf, NULL);
    Uart2_Config9600_8N1();
    OutputInit(&Uart2_BlockingSendByte);

    enableInterrupts();

#ifdef BEEPER
    Beep_SetPrescaler(BEEP_PRESCALE_2);
    Beep_SetFrequency(BEEP_8KHZ);
    Beep_On();
#endif

#ifdef SQUARER
    //TestI2CSpeeds(20);
    //TestI2CSpeeds(16);
    //TestI2CSpeeds(10);
    //TestI2CSpeeds(8);
    //TestI2CSpeeds(1);

    I2C_Init(I2C_SPEED_STANDARD, 16, 0x50, 0);
    I2C_ConfigStdModeMaster();
#endif

    for (;;)
    {
        // Do I2C stuff
#ifdef SQUARER
        if (Systick_Timeout(&squarer, 500))
        {
            static int counter;
            if (!I2C_BusBusy())
            {
                OutputText("I2C kicked %d\r\n", ++counter);
                if (!I2C_Start())
                {
                    OutputText("Start failed SR1=%x SR2=%x SR3=%x\r\n", I2C->SR1, I2C->SR2, I2C->SR3);
                }
                else
                {
                    I2C_SendAddress(0x40, I2C_DIRECTION_WRITE);
                    I2C_SendData(0xFF);
                    I2C_Stop();
                }
            }
        }
#endif // SQUARER

        // Flash the LED if PD7 is connected
#ifdef FLASHER
        if (flash)
        {
            if (Systick_Timeout(&flasher, 100))
            {
                Gpio_TurnOnLED();
                flash = 0;
            }
        }
        else
        {
            if (Systick_Timeout(&flasher, 400))
            {
                Gpio_TurnOffLED();
                flash = 1;
            }
        }
#endif // FLASHER

        // Fade the LED in and out if PC3 is connected
#ifdef FADER
        if (Systick_Timeout(&fader, 10))
        {
            Tim1_SetCounter(fade);
            if (up)
            {
                fade += 10;
                if (fade > TIM1_PERIOD)
                {
                    up = 0;
                }
                GPIOB->ODR = (GPIOB->ODR & ~GPIO_ODR_4_MASK) | GPIO_ODR_4_LOW;
            }
            else
            {
                if (fade > 0)
                {
                    fade -= 10;
                }
                else
                {
                    up = 1;
                }
                GPIOB->ODR = (GPIOB->ODR & ~GPIO_ODR_4_MASK) | GPIO_ODR_4_HIGH;
            }
        }
#endif // FADER

#ifdef BEEPER
        if (Systick_Timeout(&beeper, 100))
        {
            Beep_SetPrescaler(pre);
            if (++pre > BEEP_PRESCALE_32)
            {
                pre = BEEP_PRESCALE_2;
                if (++freq > BEEP_32KHZ)
                {
                    freq = BEEP_8KHZ;
                }
                Beep_SetFrequency(freq);
            }
        }
#endif // BEEPER

        // Output a message using interrupts and a circular buffer
#ifdef SERIALIZER
        if (Systick_Timeout(&transmitter, 1))
        {
            static uint32_t i = 0;

            OutputText("%08lx\r", i);
            ++i;
            //OutputString("The quick brown fox jumps over the lazy dog Pack my box with five dozen liquor jugs 0123456789 ABCDEFGHIJKLMNOPQRSTUVWXYZ abcdefghijklmnopqrstuvwxyz\r\n");
        }

        // Indicate via the brightness of the LED, the amount of buffer space in use
#ifndef FADER
        Tim1_SetCounter((TIM1_PERIOD * 100) /  CircBuf_PercentUsed(&tx_cirbuf));
#endif // FADER
#endif // SERIALIZER
    }
}
