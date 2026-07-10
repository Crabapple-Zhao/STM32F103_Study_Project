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

/**
 * @brief 设置状态栏显示的标题文本 (ASCII, 最长 16 字符)。
 * @note 传入 nullptr 清空标题。字符串会被复制, 调用者无需保持生命周期。
 */
void astraSetStatusBarTitle(const char *title);

typedef struct {
  uint32_t ramBytes;
  uint32_t romBytes;
  uint32_t ramPercent;
  uint32_t romPercent;
} AstraMemoryUsage;

void astraGetMemoryUsage(AstraMemoryUsage *usage);

#ifdef __cplusplus
}
#endif

#endif //ASTRA_HAL_PORT_H_
