#include "main.h"
#include "matrix_keypad.h"

char keys[4][4] = {
    {'1', '2', '3', 'A'},
    {'4', '5', '6', 'B'},
    {'7', '8', '9', 'C'},
    {'*', '0', '#', 'D'}
};

// Define row and column GPIO ports and pins
GPIO_TypeDef* rowPorts[4] = { GPIOC, GPIOA, GPIOA, GPIOB };
uint16_t rowPins[4]       = { GPIO_PIN_5, GPIO_PIN_12, GPIO_PIN_11, GPIO_PIN_12 };

GPIO_TypeDef* colPorts[4] = { GPIOB, GPIOB, GPIOB, GPIOB };
uint16_t colPins[4]       = { GPIO_PIN_1, GPIO_PIN_15, GPIO_PIN_14, GPIO_PIN_13 };

void USER_MATRIX_KEYPAD_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    // Enable GPIO clocks if not already enabled
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();
    __HAL_RCC_GPIOC_CLK_ENABLE();

    // Initialize rows as output
    for (int i = 0; i < 4; i++) {
        GPIO_InitStruct.Pin = rowPins[i];
        GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
        GPIO_InitStruct.Pull = GPIO_NOPULL;
        GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
        HAL_GPIO_Init(rowPorts[i], &GPIO_InitStruct);
        HAL_GPIO_WritePin(rowPorts[i], rowPins[i], GPIO_PIN_SET); // Set high
    }

    // Initialize columns as input with pull-up
    for (int i = 0; i < 4; i++) {
        GPIO_InitStruct.Pin = colPins[i];
        GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
        GPIO_InitStruct.Pull = GPIO_PULLUP;
        HAL_GPIO_Init(colPorts[i], &GPIO_InitStruct);
    }
}

char USER_MATRIX_KEYPAD_Read(void)
{
    for (int row = 0; row < 4; row++) {
        // Set current row low
        HAL_GPIO_WritePin(rowPorts[row], rowPins[row], GPIO_PIN_RESET);

        // Check each column
        for (int col = 0; col < 4; col++) {
            if (HAL_GPIO_ReadPin(colPorts[col], colPins[col]) == GPIO_PIN_RESET) {
                HAL_Delay(10); // basic debounce
                // Wait for key release
                while (HAL_GPIO_ReadPin(colPorts[col], colPins[col]) == GPIO_PIN_RESET);
                HAL_GPIO_WritePin(rowPorts[row], rowPins[row], GPIO_PIN_SET);
                return keys[row][col];
            }
        }

        // Reset current row to high
        HAL_GPIO_WritePin(rowPorts[row], rowPins[row], GPIO_PIN_SET);
    }

    return 0; // no key pressed
}
