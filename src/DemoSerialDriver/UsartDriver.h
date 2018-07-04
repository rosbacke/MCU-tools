/*
 * UsartDriver.h
 *
 *  Created on: Jul 1, 2018
 *      Author: mikaelr
 */

#ifndef SRC_DEMOSERIALDRIVER_USARTDRIVER_H_
#define SRC_DEMOSERIALDRIVER_USARTDRIVER_H_

#include "stm32f4xx.h"

namespace hw
{
namespace stm32f429
{
enum DevType
{

    rcc,
    gpio,
    usart,
    spi,
    // etc.
};

template <DevType dt>
struct DevType_s
{
    // Protocol : using for DrvStruct to STM driver structure.
    // using DrvStruct = ...
};

template <>
struct DevType_s<DevType::rcc>
{
    using DrvStruct = RCC_TypeDef;
};
template <>
struct DevType_s<DevType::usart>
{
    using DrvStruct = USART_TypeDef;
};
template <>
struct DevType_s<DevType::gpio>
{
    using DrvStruct = GPIO_TypeDef;
};

template <DevType dt, int devNo>
struct Dev_s
{
    // Protocol. Proper pointer base.
    // DevType_s<dt>* dev;
};

template <>
struct Dev_s<DevType::rcc, 1>
{
    DevType_s<DevType::usart>* base = RCC;
};
template <>
struct Dev_s<DevType::usart, 1>
{
    DevType_s<DevType::usart>* base = USART1;
};
template <>
struct Dev_s<DevType::gpio, 1>
{
    DevType_s<DevType::usart>* base = GPIOA;
};
}

Got devices usart1 - 7, spi etc.Got pins, gpioa, b,
    c 1 - 31... Got mux setting 1 -
        16.

        Ange pin.Talar om GPIO enhet +
        vilken mux att programmera.Ange Usart +
        num.Anger device att programmera.

        I tabeller : Pin +
        device->mux setting.device->RCC clk setting
            .

        Cover _typical_ use case.

        class UsartHW
{
};

class UsartDriver
{
  public:
    UsartDriver();
    virtual ~UsartDriver();
};

#endif /* SRC_DEMOSERIALDRIVER_USARTDRIVER_H_ */
