/**
******************************************************************************
* @file           : rotary.h
* @brief          : 通用旋转编码器驱动头文件（平台无关）
* @author         : 改编自Arduino Rotary库 (Ben Buxton)
* @date           : 2025
******************************************************************************
* @attention
*
* 本驱动适用于增量式旋转编码器(如EC11/EC12)
* 使用状态机算法，自带消抖功能，支持高速旋转
* 完全平台无关，可移植到任意嵌入式系统、PC、RTOS等
*
* 移植说明：
*   1. 实现两个GPIO读取函数
*   2. 在初始化时将函数指针传入
*   3. 周期性调用Rotary_Process()
*
******************************************************************************
*/

#ifndef __ROTARY_H__
#define __ROTARY_H__

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include <stdbool.h>
#include <stdint.h>

/* 配置选项
 * -------------------------------------------------------------------*/

/**
 * @brief 启用半步模式
 * @note  半步模式会在每个格雷码的00和11位置都触发事件，分辨率翻倍
 *        如果发现编码器"转两格才响应一次"，请取消注释此行
 */
#define ROTARY_HALF_STEP

/* 返回值定义
 * -----------------------------------------------------------------*/

/**
 * @brief 无完整步进事件
 * @note  表示编码器正在转动中，但还未完成一个完整的格雷码周期
 */
#define ROTARY_DIR_NONE 0x00

/**
 * @brief 顺时针旋转事件
 * @note  编码器顺时针转动一格(咔嗒一声)完成
 */
#define ROTARY_DIR_CW 0x10

/**
 * @brief 逆时针旋转事件
 * @note  编码器逆时针转动一格(咔嗒一声)完成
 */
#define ROTARY_DIR_CCW 0x20

/* 类型定义
 * -------------------------------------------------------------------*/

/**
 * @brief GPIO读取函数指针类型
 * @param user_data: 用户自定义数据（可传入引脚编号、端口地址等）
 * @retval true=高电平, false=低电平
 */
typedef bool (*Rotary_ReadPinFunc)(void *user_data);

/**
 * @brief 旋转编码器句柄结构体
 * @note  每个编码器需要一个独立的句柄实例
 */
typedef struct {
  Rotary_ReadPinFunc read_pin_a; /**< A相读取函数指针 */
  void *user_data_a;             /**< A相用户数据（传给read_pin_a） */

  Rotary_ReadPinFunc read_pin_b; /**< B相读取函数指针 */
  void *user_data_b;             /**< B相用户数据（传给read_pin_b） */

  uint8_t state; /**< 当前状态机状态（内部使用） */
} Rotary_HandleTypeDef;

/* 函数声明
 * -------------------------------------------------------------------*/

/**
 * @brief  初始化旋转编码器
 * @param  hrotary: 编码器句柄指针
 * @param  read_pin_a: A相读取函数
 * @param  user_data_a: 传递给read_pin_a的用户数据
 * @param  read_pin_b: B相读取函数
 * @param  user_data_b: 传递给read_pin_b的用户数据
 * @retval None
 *
 * @note   调用此函数前，请确保已配置好GPIO（方向、上拉等）
 *
 * @example STM32 HAL使用示例:
 *   bool read_rotary_a(void *data) {
 *       return HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_0) == GPIO_PIN_SET;
 *   }
 *   bool read_rotary_b(void *data) {
 *       return HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_1) == GPIO_PIN_SET;
 *   }
 *
 *   Rotary_HandleTypeDef rotary;
 *   Rotary_Init(&rotary, read_rotary_a, NULL, read_rotary_b, NULL);
 *
 * @example Arduino使用示例:
 *   bool read_pin(void *data) {
 *       int pin = (int)(intptr_t)data;
 *       return digitalRead(pin);
 *   }
 *
 *   Rotary_HandleTypeDef rotary;
 *   Rotary_Init(&rotary, read_pin, (void*)2, read_pin, (void*)3);
 *
 * @example ESP32使用示例:
 *   bool esp32_read_gpio(void *data) {
 *       gpio_num_t pin = (gpio_num_t)(intptr_t)data;
 *       return gpio_get_level(pin) == 1;
 *   }
 *
 *   Rotary_HandleTypeDef rotary;
 *   Rotary_Init(&rotary, esp32_read_gpio, (void*)GPIO_NUM_18, esp32_read_gpio,
 * (void*)GPIO_NUM_19);
 */
void Rotary_Init(Rotary_HandleTypeDef *hrotary, Rotary_ReadPinFunc read_pin_a,
                 void *user_data_a, Rotary_ReadPinFunc read_pin_b,
                 void *user_data_b);

/**
 * @brief  处理编码器状态(核心函数)
 * @param  hrotary: 编码器句柄指针
 * @retval 返回旋转方向
 *         @arg ROTARY_DIR_NONE: 无事件
 *         @arg ROTARY_DIR_CW:   顺时针旋转一格
 *         @arg ROTARY_DIR_CCW:  逆时针旋转一格
 *
 * @note   此函数应该周期性调用（定时器中断、GPIO中断或主循环）
 * @note   推荐调用频率: 1-5ms一次
 * @note   状态机自带消抖功能，无需额外延时
 *
 * @example 在裸机主循环中使用:
 *   while(1) {
 *       uint8_t result = Rotary_Process(&rotary);
 *       if(result == ROTARY_DIR_CW) {
 *           counter++;
 *       } else if(result == ROTARY_DIR_CCW) {
 *           counter--;
 *       }
 *       delay_ms(2);  // 2ms轮询一次
 *   }
 *
 * @example 在FreeRTOS任务中使用:
 *   void rotary_task(void *param) {
 *       while(1) {
 *           uint8_t result = Rotary_Process(&rotary);
 *           if(result != ROTARY_DIR_NONE) {
 *               // 处理旋转事件
 *               xQueueSend(event_queue, &result, 0);
 *           }
 *           vTaskDelay(pdMS_TO_TICKS(2));
 *       }
 *   }
 *
 * @example 在定时器中断中使用:
 *   void TIM3_IRQHandler(void) {
 *       if(__HAL_TIM_GET_FLAG(&htim3, TIM_FLAG_UPDATE)) {
 *           __HAL_TIM_CLEAR_FLAG(&htim3, TIM_FLAG_UPDATE);
 *
 *           uint8_t result = Rotary_Process(&rotary1);
 *           if(result == Rotary_DIR_CW) {
 *               counter++;  // 顺时针
 *           } else if(result == Rotary_DIR_CCW) {
 *               counter--;  // 逆时针
 *           }
 *       }
 *   }
 *
 * @example 在GPIO中断中使用:
 *   void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin) {
 *       if(GPIO_Pin == GPIO_PIN_0 || GPIO_Pin == GPIO_PIN_1) {
 *           uint8_t result = Rotary_Process(&rotary1);
 *           // 处理result...
 *       }
 *   }
 */
uint8_t Rotary_Process(Rotary_HandleTypeDef *hrotary);

/**
 * @brief  获取编码器当前引脚状态(调试用)
 * @param  hrotary: 编码器句柄指针
 * @retval 引脚状态(bit1=B相, bit0=A相)
 *         @arg 0b00: A=0, B=0
 *         @arg 0b01: A=1, B=0
 *         @arg 0b10: A=0, B=1
 *         @arg 0b11: A=1, B=1
 *
 * @note   此函数主要用于调试，检查引脚连接是否正确
 */
uint8_t Rotary_ReadPins(Rotary_HandleTypeDef *hrotary);

#ifdef __cplusplus
}
#endif

#endif /* __ROTARY_H__ */