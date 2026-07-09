//
// Created by Fir on 2024/3/7 007.
// 此文件的作用是引导STM32进入astra UI 基于C++
// this file is used to guide STM32 into astra UI based on C++
// 改造：移除了对 hal_dreamCore 的依赖，HAL 实例由用户通过 HAL::inject 注入。
//

#ifndef ASTRA_CORE_SRC_ASTRA_ASTRA_ROCKET_H_
#define ASTRA_CORE_SRC_ASTRA_ASTRA_ROCKET_H_

#ifdef __cplusplus
extern "C" {
#endif

/*---- C 接口，供 main.c 直接调用 ----*/

//初始化 astra UI（菜单树 + 启动器）。调用前必须先 astraHalInit() 注入 HAL。
void astraCoreInit(void);

//进入 astra UI 主循环（内部死循环，不会返回）
void astraCoreStart(void);

//单次执行 astra UI 更新（非阻塞，供主循环手动调用）
void astraLoop(void);

//简单的画图自测，用于验证 HAL 是否实现正确
void astraCoreTest(void);

//销毁启动器并释放 HAL 实例
void astraCoreDestroy(void);

//显示开机画面 (居中 Dev-Beta / 版本 / 硬件信息, 持续 2s)
void astraShowBootScreen(void);

#ifdef __cplusplus
}
#endif

#endif //ASTRA_CORE_SRC_ASTRA_ASTRA_ROCKET_H_
