//
// HAL 移植接口。供 main.c 调用。
//
#pragma once
#ifndef ASTRA_HAL_PORT_H_
#define ASTRA_HAL_PORT_H_

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief 创建并注入 HAL 移植实例。
 * @return 0 成功，非 0 失败。
 * @note 必须在 astraCoreInit 之前调用。内部会 new AstraHALPort 并 HAL::inject。
 */
int astraHalInit(void);

#ifdef __cplusplus
}
#endif

#endif //ASTRA_HAL_PORT_H_
