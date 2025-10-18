/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2025 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "LCD1602.h"
#include "libraries.h"
#include "matrix_keypad.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */
// ======== STATE MACHINE STATES ========
// Defines all possible states for the door lock system
typedef enum {
  ST_IDLE = 0,       // Waiting for '*' to start password entry
  ST_COLLECT,        // Collecting digits from the keypad
  ST_VERIFY,         // Checking if the entered code matches
  ST_SUCCESS,        // Correct password entered
  ST_FAILURE         // Incorrect password entered
} LockState;
/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
// ======== KEYPAD CONNECTIONS ========

// Row pins configured as outputs
#define R1_PORT GPIOA
#define R1_PIN  GPIO_PIN_8
#define R2_PORT GPIOB
#define R2_PIN  GPIO_PIN_10
#define R3_PORT GPIOB
#define R3_PIN  GPIO_PIN_4
#define R4_PORT GPIOB
#define R4_PIN  GPIO_PIN_5

// Column pins configured as inputs
#define C1_PORT GPIOC
#define C1_PIN  GPIO_PIN_7
#define C2_PORT GPIOC
#define C2_PIN  GPIO_PIN_6
#define C3_PORT GPIOC
#define C3_PIN  GPIO_PIN_9
#define C4_PORT GPIOA
#define C4_PIN  GPIO_PIN_7

// ======== LED CONNECTIONS ========
// Green LED → password correct
#define LED_OK_PORT   GPIOC
#define LED_OK_PIN    led_o2_Pin
// Red LED → password incorrect
#define LED_ERR_PORT  GPIOB
#define LED_ERR_PIN   led_o1_Pin
/* USER CODE END PD */

/* Private variables ---------------------------------------------------------*/
COM_InitTypeDef BspCOMInit;
TIM_HandleTypeDef htim1;
/* USER CODE BEGIN PV */
// (no persistent variables needed beyond FSM globals)
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_TIM1_Init(void);
/* USER CODE BEGIN PFP */
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

// ===================== 7-SEGMENT DISPLAY HANDLING =====================
// Lookup table for each digit (bits = A B C D E F G DP)
static const uint8_t segmentMap[18] = {
    0b11111100, // 0
    0b01100000, // 1
    0b11011010, // 2
    0b11110010, // 3
    0b01100110, // 4
    0b10110110, // 5
    0b10111110, // 6
    0b11100000, // 7
    0b11111110, // 8
    0b11110110, // 9
    0b11101110, // A
    0b00111110, // b
    0b10011100, // C
    0b01111010, // d
    0b10011110, // E
    0b10001110, // F
    0b00000010, // '-' (used as placeholder)
    0b00000000  // blank
};

// Displays a single digit on one of the four 7-segment positions
static void DisplayDigit(int pos, int value)
{
    // Disable all digits before updating (prevents ghosting)
    HAL_GPIO_WritePin(GPIOC, seg_12_Pin | seg_9_Pin | seg_8_Pin, GPIO_PIN_SET);
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_10, GPIO_PIN_SET);

    // Select the segment pattern for the given value
    uint8_t segs = segmentMap[(value >= 0 && value < 18) ? value : 17];

    // Write segment states (A–G + DP)
    HAL_GPIO_WritePin(seg_11_GPIO_Port, seg_11_Pin, (segs & (1 << 7)) ? GPIO_PIN_SET : GPIO_PIN_RESET);
    HAL_GPIO_WritePin(seg_7_GPIO_Port,  seg_7_Pin,  (segs & (1 << 6)) ? GPIO_PIN_SET : GPIO_PIN_RESET);
    HAL_GPIO_WritePin(seg_4_GPIO_Port,  seg_4_Pin,  (segs & (1 << 5)) ? GPIO_PIN_SET : GPIO_PIN_RESET);
    HAL_GPIO_WritePin(seg_2_GPIO_Port,  seg_2_Pin,  (segs & (1 << 4)) ? GPIO_PIN_SET : GPIO_PIN_RESET);
    HAL_GPIO_WritePin(seg_1_GPIO_Port,  seg_1_Pin,  (segs & (1 << 3)) ? GPIO_PIN_SET : GPIO_PIN_RESET);
    HAL_GPIO_WritePin(seg_10_GPIO_Port, seg_10_Pin, (segs & (1 << 2)) ? GPIO_PIN_SET : GPIO_PIN_RESET);
    HAL_GPIO_WritePin(seg_5_GPIO_Port,  seg_5_Pin,  (segs & (1 << 1)) ? GPIO_PIN_SET : GPIO_PIN_RESET);
    HAL_GPIO_WritePin(seg_3_GPIO_Port,  seg_3_Pin,  (segs & (1 << 0)) ? GPIO_PIN_SET : GPIO_PIN_RESET);

    // Enable only the selected digit (active low)
    if (pos == 0) HAL_GPIO_WritePin(seg_12_GPIO_Port, seg_12_Pin, GPIO_PIN_RESET);
    if (pos == 1) HAL_GPIO_WritePin(seg_9_GPIO_Port,  seg_9_Pin,  GPIO_PIN_RESET);
    if (pos == 2) HAL_GPIO_WritePin(seg_8_GPIO_Port,  seg_8_Pin,  GPIO_PIN_RESET);
    if (pos == 3) HAL_GPIO_WritePin(GPIOA,            GPIO_PIN_10,GPIO_PIN_RESET);

    // Ensure last digit line (PA10) stays off when not selected
    if (pos != 3)
        HAL_GPIO_WritePin(GPIOA, GPIO_PIN_10, GPIO_PIN_SET);
}

// ===================== MATRIX KEYPAD SCAN =====================
// Scans the 4x4 keypad and returns the pressed key (char)
static char read_keypad(void)
{
    // Set all rows HIGH before scanning
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_8, GPIO_PIN_SET);
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_10, GPIO_PIN_SET);
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_4, GPIO_PIN_SET);
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_5, GPIO_PIN_SET);

    // Macro to check columns efficiently
    #define CHECK_COL(col_port, col_pin, value) \
        if (!HAL_GPIO_ReadPin(col_port, col_pin)) { while(!HAL_GPIO_ReadPin(col_port, col_pin)); return value; }

    // Scan each row one by one
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_8, GPIO_PIN_RESET); HAL_Delay(2);
    CHECK_COL(GPIOC, GPIO_PIN_7, '1');
    CHECK_COL(GPIOC, GPIO_PIN_6, '2');
    CHECK_COL(GPIOC, GPIO_PIN_9, '3');
    CHECK_COL(GPIOA, GPIO_PIN_7, 'A');
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_8, GPIO_PIN_SET);

    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_10, GPIO_PIN_RESET); HAL_Delay(2);
    CHECK_COL(GPIOC, GPIO_PIN_7, '4');
    CHECK_COL(GPIOC, GPIO_PIN_6, '5');
    CHECK_COL(GPIOC, GPIO_PIN_9, '6');
    CHECK_COL(GPIOA, GPIO_PIN_7, 'B');
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_10, GPIO_PIN_SET);

    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_4, GPIO_PIN_RESET); HAL_Delay(2);
    CHECK_COL(GPIOC, GPIO_PIN_7, '7');
    CHECK_COL(GPIOC, GPIO_PIN_6, '8');
    CHECK_COL(GPIOC, GPIO_PIN_9, '9');
    CHECK_COL(GPIOA, GPIO_PIN_7, 'C');
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_4, GPIO_PIN_SET);

    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_5, GPIO_PIN_RESET); HAL_Delay(2);
    CHECK_COL(GPIOC, GPIO_PIN_7, '*');
    CHECK_COL(GPIOC, GPIO_PIN_6, '0');
    CHECK_COL(GPIOC, GPIO_PIN_9, '#');
    CHECK_COL(GPIOA, GPIO_PIN_7, 'D');
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_5, GPIO_PIN_SET);

    return 0; // No key pressed
}

// ===================== DOOR LOCK STATE MACHINE =====================
// Password configuration
static const int PASS_LEN = 4;
static const int passcode[4] = {1, 2, 3, 4};

// Global FSM variables
static volatile LockState state = ST_IDLE;
static int buf[4];
static int idx = 0;
static char last_key_seen = 0;

// Clears current input buffer
static void clear_input(void)
{
    for (int i = 0; i < 4; ++i) buf[i] = -1;
    idx = 0;
}

// Turns off both LEDs
static void leds_off(void)
{
    HAL_GPIO_WritePin(LED_OK_PORT,  LED_OK_PIN,  GPIO_PIN_RESET);
    HAL_GPIO_WritePin(LED_ERR_PORT, LED_ERR_PIN, GPIO_PIN_RESET);
}

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{
  HAL_Init();
  SystemClock_Config();
  MX_GPIO_Init();
  MX_TIM1_Init();

  /* USER CODE BEGIN 2 */
  USER_MATRIX_KEYPAD_Init();
  HAL_TIM_Base_Start(&htim1);
  leds_off();
  clear_input();
  state = ST_IDLE;
  /* USER CODE END 2 */

  while (1)
  {
    // --- DISPLAY MULTIPLEXING ---
    // Refresh all digits continuously to keep display visible
    HAL_GPIO_WritePin(GPIOC, seg_12_Pin | seg_9_Pin | seg_8_Pin, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_10, GPIO_PIN_RESET);

    for (int repeat = 0; repeat < 3; repeat++) { // repeat for better brightness
        for (int pos = 0; pos < 4; pos++) {
            int val = (pos < idx) ? buf[pos] : 17; // blank unused digits
            DisplayDigit(pos, val);
            HAL_Delay(1);
        }
    }

    // --- READ KEYPAD INPUT ---
    char k = read_keypad();
    if (k == 0) { last_key_seen = 0; continue; }   // no key pressed
    if (k == last_key_seen) { continue; }          // ignore hold
    last_key_seen = k;                             // detect new press

    // --- FINITE STATE MACHINE LOGIC ---
    switch (state)
    {
        case ST_IDLE:
            leds_off();
            if (k == '*') {
                clear_input();     // start new input sequence
                state = ST_COLLECT;
            }
            break;

        case ST_COLLECT:
            if (k >= '0' && k <= '9') {
                if (idx < PASS_LEN) {
                    buf[idx++] = (k - '0');  // store pressed digit
                    HAL_Delay(5);            // short delay for display update
                }
            } else if (k == '*') {
                if (idx == PASS_LEN) state = ST_VERIFY;  // proceed if 4 digits entered
            } else if (k == '#') {
                clear_input();   // clear buffer if user presses '#'
            }
            break;

        case ST_VERIFY:
        {
            int match = 1;
            for (int i = 0; i < PASS_LEN; ++i) {
                if (buf[i] != passcode[i]) { match = 0; break; }
            }
            if (match) {
                HAL_GPIO_WritePin(LED_OK_PORT, LED_OK_PIN, GPIO_PIN_SET);
                HAL_GPIO_WritePin(LED_ERR_PORT, LED_ERR_PIN, GPIO_PIN_RESET);
                state = ST_SUCCESS;
            } else {
                HAL_GPIO_WritePin(LED_OK_PORT, LED_OK_PIN, GPIO_PIN_RESET);
                HAL_GPIO_WritePin(LED_ERR_PORT, LED_ERR_PIN, GPIO_PIN_SET);
                state = ST_FAILURE;
            }
        }
        break;

        case ST_SUCCESS:
            // Remains on success state until user restarts with '*'
            if (k == '*') {
                leds_off();
                clear_input();
                state = ST_COLLECT;
            }
            break;

        case ST_FAILURE:
            // Show red LED for 2 seconds, then return to idle
            HAL_Delay(2000);
            leds_off();
            clear_input();
            state = ST_IDLE;
            break;
    }
  }
}
/* USER CODE END 3 */



/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Configure the main internal regulator output voltage
  */
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE3);

  while(!__HAL_PWR_GET_FLAG(PWR_FLAG_VOSRDY)) {}

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_CSI;
  RCC_OscInitStruct.CSIState = RCC_CSI_ON;
  RCC_OscInitStruct.CSICalibrationValue = RCC_CSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLL1_SOURCE_CSI;
  RCC_OscInitStruct.PLL.PLLM = 1;
  RCC_OscInitStruct.PLL.PLLN = 36;
  RCC_OscInitStruct.PLL.PLLP = 2;
  RCC_OscInitStruct.PLL.PLLQ = 2;
  RCC_OscInitStruct.PLL.PLLR = 2;
  RCC_OscInitStruct.PLL.PLLRGE = RCC_PLL1_VCIRANGE_2;
  RCC_OscInitStruct.PLL.PLLVCOSEL = RCC_PLL1_VCORANGE_WIDE;
  RCC_OscInitStruct.PLL.PLLFRACN = 0;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2
                              |RCC_CLOCKTYPE_PCLK3;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB3CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_3) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure the programming delay
  */
  __HAL_FLASH_SET_PROGRAM_DELAY(FLASH_PROGRAMMING_DELAY_1);
}

/**
  * @brief TIM1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM1_Init(void)
{

  /* USER CODE BEGIN TIM1_Init 0 */

  /* USER CODE END TIM1_Init 0 */

  TIM_ClockConfigTypeDef sClockSourceConfig = {0};
  TIM_MasterConfigTypeDef sMasterConfig = {0};

  /* USER CODE BEGIN TIM1_Init 1 */

  /* USER CODE END TIM1_Init 1 */
  htim1.Instance = TIM1;
  htim1.Init.Prescaler = 72-1;
  htim1.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim1.Init.Period = 65534;
  htim1.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim1.Init.RepetitionCounter = 0;
  htim1.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_Base_Init(&htim1) != HAL_OK)
  {
    Error_Handler();
  }
  sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
  if (HAL_TIM_ConfigClockSource(&htim1, &sClockSourceConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterOutputTrigger2 = TIM_TRGO2_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim1, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM1_Init 2 */

  /* USER CODE END TIM1_Init 2 */

}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};
  /* USER CODE BEGIN MX_GPIO_Init_1 */

  /* USER CODE END MX_GPIO_Init_1 */

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOH_CLK_ENABLE();
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOC, seg_10_Pin|seg_11_Pin|seg_12_Pin|led_o2_Pin
                          |seg_5_Pin|seg_7_Pin|seg_9_Pin|seg_8_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOA, buzzer_Pin|r1_Pin|seg_4_Pin|seg_6_Pin
                          |seg_3_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOB, seg_1_Pin|r2_Pin|seg_2_Pin|buzzerB13_Pin
                          |GPIO_PIN_15|r3_Pin|r4_Pin|led_o1_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pins : seg_10_Pin seg_11_Pin seg_12_Pin led_o2_Pin
                           seg_5_Pin seg_7_Pin seg_9_Pin seg_8_Pin */
  GPIO_InitStruct.Pin = seg_10_Pin|seg_11_Pin|seg_12_Pin|led_o2_Pin
                          |seg_5_Pin|seg_7_Pin|seg_9_Pin|seg_8_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

  /*Configure GPIO pins : buzzer_Pin r1_Pin seg_4_Pin seg_6_Pin
                           seg_3_Pin */
  GPIO_InitStruct.Pin = buzzer_Pin|r1_Pin|seg_4_Pin|seg_6_Pin
                          |seg_3_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /*Configure GPIO pin : c4_Pin */
  GPIO_InitStruct.Pin = c4_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(c4_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pins : seg_1_Pin r2_Pin seg_2_Pin buzzerB13_Pin
                           PB15 r3_Pin r4_Pin led_o1_Pin */
  GPIO_InitStruct.Pin = seg_1_Pin|r2_Pin|seg_2_Pin|buzzerB13_Pin
                          |GPIO_PIN_15|r3_Pin|r4_Pin|led_o1_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /*Configure GPIO pins : c2_Pin c1_Pin c3_Pin */
  GPIO_InitStruct.Pin = c2_Pin|c1_Pin|c3_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

  /* USER CODE BEGIN MX_GPIO_Init_2 */

  /* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}
#ifdef USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
