/**
******************************************************************************
* @file           : rotary.c
* @brief          : 通用旋转编码器驱动实现（平台无关）
******************************************************************************
* @attention
*
* 算法原理说明:
*
* 机械旋转编码器输出两相格雷码(Gray Code)，每转一格经历4个状态:
*
*   位置      A相    B相
*   ----------------------
*   起始      0      0
*   1/4转     1      0
*   1/2转     1      1
*   3/4转     0      1
*   下一格     0      0
*
* 顺时针旋转: 00 -> 10 -> 11 -> 01 -> 00
* 逆时针旋转: 00 -> 01 -> 11 -> 10 -> 00
*
* 本驱动使用有限状态机(FSM)解析这些状态变化:
* - 状态转移表存储了所有合法的状态转换路径
* - 只有完成完整的格雷码周期才会触发方向事件
* - 非法状态(如抖动、干扰)会自动复位，不会误触发
*
* 优势:
* 1. 内置消抖: 机械抖动只会在子状态间跳转，不影响最终结果
* 2. 抗干扰: EMI等导致的非法状态会被自动过滤
* 3. 高速响应: 无需延时消抖，可准确测量快速旋转
* 4. 代码简洁: 仅状态表+查表逻辑，易于移植和维护
*
******************************************************************************
*/

/* Includes ------------------------------------------------------------------*/
#include "rotary.h"

/* 私有宏定义
 * -----------------------------------------------------------------*/

// 状态机起始状态
#define R_START 0x0

#ifdef ROTARY_HALF_STEP
// ========== 半步模式状态定义 ==========
// 半步模式在00和11位置都触发事件，分辨率翻倍

#define R_CCW_BEGIN 0x1   // 开始逆时针旋转(从00进入)
#define R_CW_BEGIN 0x2    // 开始顺时针旋转(从00进入)
#define R_START_M 0x3     // 中间状态(11位置)
#define R_CW_BEGIN_M 0x4  // 开始顺时针旋转(从11进入)
#define R_CCW_BEGIN_M 0x5 // 开始逆时针旋转(从11进入)

/**
 * @brief 半步模式状态转移表
 * @note  行索引: 当前状态(0-5)
 *        列索引: 引脚状态 00,01,10,11 (B相<<1 | A相)
 *        表值: 新状态 (低4位=状态编号, 高4位=方向事件)
 */
static const uint8_t state_table[6][4] = {
    // 当前状态          引脚状态: 00          01          10          11
    /* R_START      */ {R_START_M, R_CW_BEGIN, R_CCW_BEGIN, R_START},
    /* R_CCW_BEGIN  */ {R_START_M | ROTARY_DIR_CCW, R_START, R_CCW_BEGIN, R_START},
    /* R_CW_BEGIN   */ {R_START_M | ROTARY_DIR_CW, R_CW_BEGIN, R_START, R_START},
    /* R_START_M    */ {R_START_M, R_CCW_BEGIN_M, R_CW_BEGIN_M, R_START},
    /* R_CW_BEGIN_M */ {R_START_M, R_START_M, R_CW_BEGIN_M, R_START | ROTARY_DIR_CW},
    /* R_CCW_BEGIN_M*/ {R_START_M, R_CCW_BEGIN_M, R_START_M, R_START | ROTARY_DIR_CCW},
};

#else
// ========== 全步模式状态定义 ==========
// 全步模式只在00位置触发事件(对应机械咔嗒声)

#define R_CW_FINAL 0x1  // 顺时针即将完成
#define R_CW_BEGIN 0x2  // 顺时针开始
#define R_CW_NEXT 0x3   // 顺时针进行中
#define R_CCW_BEGIN 0x4 // 逆时针开始
#define R_CCW_FINAL 0x5 // 逆时针即将完成
#define R_CCW_NEXT 0x6  // 逆时针进行中

/**
 * @brief 全步模式状态转移表
 * @note  行索引: 当前状态(0-6)
 *        列索引: 引脚状态 00,01,10,11 (B相<<1 | A相)
 *        表值: 新状态 (低4位=状态编号, 高4位=方向事件)
 *
 * 状态转移路径示例(顺时针):
 *   R_START(00) -> R_CW_BEGIN(10) -> R_CW_NEXT(11) -> R_CW_FINAL(01) ->
 * R_START|DIR_CW(00)
 */
static const uint8_t state_table[7][4] = {
    // 当前状态          引脚状态: 00              01              10              11
    /* R_START     */ {R_START, R_CW_BEGIN, R_CCW_BEGIN, R_START},
    /* R_CW_FINAL  */ {R_CW_NEXT, R_START, R_CW_FINAL, R_START | ROTARY_DIR_CW},
    /* R_CW_BEGIN  */ {R_CW_NEXT, R_CW_BEGIN, R_START, R_START},
    /* R_CW_NEXT   */ {R_CW_NEXT, R_CW_BEGIN, R_CW_FINAL, R_START},
    /* R_CCW_BEGIN */ {R_CCW_NEXT, R_START, R_CCW_BEGIN, R_START},
    /* R_CCW_FINAL */ {R_CCW_NEXT, R_CCW_FINAL, R_START, R_START | ROTARY_DIR_CCW},
    /* R_CCW_NEXT  */ {R_CCW_NEXT, R_CCW_FINAL, R_CCW_BEGIN, R_START},
};

#endif

/* 函数实现
 * -------------------------------------------------------------------*/

/**
 * @brief  初始化旋转编码器
 * @note   调用此函数前，请确保已配置好GPIO
 * @note   使用定时器与中断或主循环：
 *         GPIO mode：Input mode(输入)
 *         Pull-Up/Pull-Down：根据实际情况来决定是否需要内部上拉电阻
 * @note   使用外部中断：
 *         GPIO Mode：GPIO_MODE_IT_RISING_FALLING(上升沿和下降沿触发)
 *         Pull-Up/Pull-Down：根据实际情况来决定是否需要内部上拉电阻
 */
void Rotary_Init(Rotary_HandleTypeDef *hrotary, Rotary_ReadPinFunc read_pin_a,
                 void *user_data_a, Rotary_ReadPinFunc read_pin_b,
                 void *user_data_b) {
  // 1. 保存引脚配置和回调函数到句柄
  hrotary->read_pin_a = read_pin_a;
  hrotary->user_data_a = user_data_a;
  hrotary->read_pin_b = read_pin_b;
  hrotary->user_data_b = user_data_b;

  // 2. 初始化状态机为起始状态
  hrotary->state = R_START;
}

/**
 * @brief  处理编码器状态(核心算法)
 * @note   使用状态机查表法解析格雷码序列
 * @retval 返回方向事件或无事件
 */
uint8_t Rotary_Process(Rotary_HandleTypeDef *hrotary) {
  // 步骤1: 通过回调函数读取引脚的电平状态
  uint8_t pin_a_state = hrotary->read_pin_a(hrotary->user_data_a) ? 1 : 0;
  uint8_t pin_b_state = hrotary->read_pin_b(hrotary->user_data_b) ? 1 : 0;

  // 步骤2: 将两个引脚状态组合成2位数(pinstate)
  // bit1 = B相, bit0 = A相
  // 可能的值: 0b00, 0b01, 0b10, 0b11
  uint8_t pinstate = (pin_b_state << 1) | pin_a_state;

  // 步骤3: 查状态转移表，获取新状态
  // - 当前状态(hrotary->state)作为行索引
  // - 引脚状态(pinstate)作为列索引
  // - 使用& 0x0F提取低4位的状态编号(过滤掉高4位的方向事件)
  hrotary->state = state_table[hrotary->state & 0x0F][pinstate];

  // 步骤4: 返回方向事件(提取高4位)
  // 0x30 = 0b00110000，用于屏蔽低4位，只保留高4位的事件标志
  // 返回值可能是:
  //   - ROTARY_DIR_NONE (0x00): 未完成一个完整周期
  //   - ROTARY_DIR_CW   (0x10): 顺时针完成一格
  //   - ROTARY_DIR_CCW  (0x20): 逆时针完成一格
  return hrotary->state & 0x30;
}

/**
 * @brief  读取编码器当前引脚状态(调试用)
 * @note   用于检查硬件连接是否正确
 * @retval bit1=B相状态, bit0=A相状态
 */
uint8_t Rotary_ReadPins(Rotary_HandleTypeDef *hrotary) {
  uint8_t pin_a = hrotary->read_pin_a(hrotary->user_data_a) ? 1 : 0;
  uint8_t pin_b = hrotary->read_pin_b(hrotary->user_data_b) ? 1 : 0;

  return (pin_b << 1) | pin_a;
}