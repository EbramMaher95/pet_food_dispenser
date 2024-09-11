/* USER CODE BEGIN Header */
/**
 ******************************************************************************
 * @file           : main.c
 * @brief          : Main program body
 ******************************************************************************
 * @attention
 *
 * Copyright (c) 2024 STMicroelectronics.
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

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

//the keypad structure
Matrix_Keypad_t kp = { .Rows = 4, .Columns = 4, .IntputPort = GPIOB,
		.OutputPort = GPIOB, .InputStartingPin = 6, .OutputStartingPin = 12 };
//the LCD structure:
Alcd_t lcd =
		{ .RS_GPIO = GPIOA, .RS_GPIO_Pin = GPIO_PIN_4, .EN_GPIO = GPIOA,
				.EN_GPIO_Pin = GPIO_PIN_5, .Data_GPIO = GPIOA,
				.Data_GPIO_Start_Pin = 0 };
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
I2C_HandleTypeDef hi2c2;

TIM_HandleTypeDef htim1;

/* USER CODE BEGIN PV */
ds1307_t CLK;

eeprom24c32_t memory;
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_TIM1_Init(void);
static void MX_I2C2_Init(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

/**
 * @brief  The application entry point.
 * @retval int
 */
int main(void) {

	/* USER CODE BEGIN 1 */

	/* USER CODE END 1 */

	/* MCU Configuration--------------------------------------------------------*/

	/* Reset of all peripherals, Initializes the Flash interface and the Systick. */
	HAL_Init();

	/* USER CODE BEGIN Init */

	/* USER CODE END Init */

	/* Configure the system clock */
	SystemClock_Config();

	/* USER CODE BEGIN SysInit */

	/* USER CODE END SysInit */

	/* Initialize all configured peripherals */
	MX_GPIO_Init();
	MX_TIM1_Init();
	MX_I2C2_Init();
	/* USER CODE BEGIN 2 */

	//initialize device drivers
	//first we initialize the lcd to display the updates
	//the LCD will be connected to: --> can be found in line 32 in the main.c file
	//A0,1,2,3 --> Data pins
	//A4:RS, A5:EN
	Alcd_Init(&lcd, 2, 16);

	//clear display
	Alcd_Clear(&lcd);

	Keypad_Init(&kp);

	//initialize the RTC
	Ds1307_init(&CLK, &hi2c2);

	//initialize the eeprom
	eeprom24c32_init(&memory, &hi2c2);

	//to initiate the base (counter)
	HAL_TIM_Base_Start(&htim1);

	//enable the OC pin (PWM pin)
	HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_1);

	//to change the duty cycle -> CCR
	//range is from 999 to 1999 (according to calculations)

	//the servo is set at 0 degree upon starting
	TIM1->CCR1 = 999;

	//a flag if button is pressed
	uint8_t password_comp_flag, error_code, delay_flag;
	int8_t status;
	password_comp_flag = 0;
	status = 0;
	error_code = 0;

	uint8_t dose_h, dose_m, dose_s, dose_num;

	//finite state machine section
	uint32_t current_tick;

	uint32_t dosing_tick = 0;

	uint32_t general_delay;

	// a string to save the RTC time
	char timeString[50];

	/*password section*/
	char menu_pass[] = "1234";
	char entered_password[5] = ""; // User input buffer (4 digits + null terminator)
	char time_date_buffer[5];
	uint16_t buffer;
	uint8_t input_index = 0;

	general_delay = HAL_GetTick() + 250;

	/**
	 * testing section
	 */

	/*
	 //to change the duty cycle -> CCR
	 //range is from 999 to 1999 (according to calculations)
	 TIM1->CCR1 = 1999;
	 GPIO_InitTypeDef c = { .Mode = GPIO_MODE_OUTPUT_PP, .Pin = GPIO_PIN_13,
	 .Speed = GPIO_SPEED_LOW };

	 HAL_GPIO_Init(GPIOC, &c);

	 HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, SET);

	 char str[16];
	 int16_t message;
	 uint8_t value;
	 eeprom24c32_read(&memory, &value, &memory.i2c_buffer[2]);

	 Alcd_Clear(&lcd);
	 // Display ADC value on the LCD
	 message = sprintf(str, "mem = %d", value);
	 Alcd_PutAt_n(&lcd, 0, 0, str, message);

	 HAL_Delay(2000);

	 value = 255;

	 while (value == 255) {

	 Keypad_Refresh(&kp);
	 if (Keypad_Get_Key(&kp, 12)) {
	 value = 12;
	 HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, RESET);
	 } else if (Keypad_Get_Key(&kp, 10)) {
	 value = 10;
	 HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, RESET);
	 } else if (Keypad_Get_Key(&kp, 1)) {
	 value = 1;
	 HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, RESET);
	 }

	 }

	 Alcd_Clear(&lcd);
	 Alcd_PutAt(&lcd, 0, 0, "exit while");

	 HAL_Delay(2000);

	 eeprom24c32_write(&memory, &value, &memory.i2c_buffer[2]);

	 Alcd_Clear(&lcd);
	 // Display ADC value on the LCD
	 message = sprintf(str, "value = %d", value);
	 Alcd_PutAt_n(&lcd, 0, 0, str, message);
	 */

	/**
	 * end of testing section
	 */
	/* USER CODE END 2 */

	/* Infinite loop */
	/* USER CODE BEGIN WHILE */
	while (1) {
		/* USER CODE END WHILE */

		/* USER CODE BEGIN 3 */

		//get the current tick number
		current_tick = HAL_GetTick();

		//in idle mode -> button flag is 0 and status is zero
		while ((status == 0) && (error_code == 0)
				&& (current_tick >= general_delay)) {

			//get the current tick number
			current_tick = HAL_GetTick();

			//displaying the time and date
			//the lcd will display the time
			// Read time from DS1307
			if (Ds1307_read(&CLK) == DS1307_OK) //in case reading is ok
					{

				//check if the time format is 24h or 12h
				//in case of 12h
				if (CLK.format == 1) {

					// display the time
					Alcd_Clear(&lcd);
					Alcd_Display_Control(&lcd, 1, 0, 0);
					snprintf(timeString, sizeof(timeString), "%02d:%02d:%02d",
							CLK.hour, CLK.min, CLK.sec);
					Alcd_PutAt_n(&lcd, 1, 0, timeString, strlen(timeString));

					//check for PM or AM
					//in case: AM
					if (CLK.AM_PM == 0) {
						Alcd_PutAt(&lcd, 1, 9, "AM");

					} else {
						Alcd_PutAt(&lcd, 1, 9, "PM");

					}

					snprintf(timeString, sizeof(timeString), "%02d-%02d-%04d",
							CLK.date, CLK.month, CLK.year);
					Alcd_PutAt_n(&lcd, 0, 0, timeString, strlen(timeString));

					snprintf(timeString, sizeof(timeString), "%02d", status);
					Alcd_PutAt_n(&lcd, 0, 14, timeString, strlen(timeString));
				}

				//in case of 24 format
				else {

					snprintf(timeString, sizeof(timeString), "%02d:%02d:%02d",
							CLK.hour, CLK.min, CLK.sec);

					// display the time
					Alcd_Clear(&lcd);
					Alcd_Display_Control(&lcd, 1, 0, 0);
					Alcd_PutAt_n(&lcd, 1, 0, timeString, strlen(timeString));

					Alcd_PutAt(&lcd, 0, 0, "Time");

					snprintf(timeString, sizeof(timeString), "%02d", status);
					Alcd_PutAt_n(&lcd, 0, 14, timeString, strlen(timeString));

				}

			} else {

				 Alcd_Clear(&lcd);
				 Alcd_PutAt(&lcd, 0, 0, "RTC failure");
				 error_code = 1;

			}
			snprintf(timeString, sizeof(timeString), "%02d", status);
			Alcd_PutAt_n(&lcd, 0, 14, timeString, strlen(timeString));

			//check if any button is pressed
			//check for any keypad input
			Keypad_Refresh(&kp);

			//in case menu is pressed -> move to state10
			if (Keypad_Get_Key(&kp, kp_button_save_menu)
					&& (current_tick >= general_delay)) {
				status = 10;

			}

			general_delay = HAL_GetTick() + 250;

		}

		//status 10 -> main menu
		while ((status == 10) && (current_tick >= general_delay)) {

			//get the current tick number
			current_tick = HAL_GetTick();

			Alcd_Clear(&lcd);
			Alcd_PutAt(&lcd, 0, 0, "1: view");
			Alcd_PutAt(&lcd, 1, 0, "2: edit");
			snprintf(timeString, sizeof(timeString), "%02d", status);
			Alcd_PutAt_n(&lcd, 0, 14, timeString, strlen(timeString));

			//check if any button is pressed
			//check for any keypad input
			Keypad_Refresh(&kp);

			//in case 1 is selected -> view menu (status 11)
			if (Keypad_Get_Key(&kp, kp_button_1)
					&& (current_tick >= general_delay)) {

				status = 11;

			}

			//2 is selected -> edit menu (status 20)
			else if (Keypad_Get_Key(&kp, kp_button_2)
					&& (current_tick >= general_delay)) {

				//ask the user for password
				Alcd_Clear(&lcd);
				status = 20;

			}

			//back is selected
			else if (Keypad_Get_Key(&kp, kp_button_no_back)
					&& (current_tick >= general_delay)) {

				status = 0;

			}

			general_delay = HAL_GetTick() + 250;
		}

		//status 11 -> view menu
		while ((status == 11) && (current_tick >= general_delay)) {

			//get the current tick number
			current_tick = HAL_GetTick();

			Alcd_Clear(&lcd);
			Alcd_PutAt(&lcd, 0, 0, "1: date");
			Alcd_PutAt(&lcd, 1, 0, "2: parameters");
			snprintf(timeString, sizeof(timeString), "%02d", status);
			Alcd_PutAt_n(&lcd, 0, 14, timeString, strlen(timeString));

			//check if any button is pressed
			//check for any keypad input
			Keypad_Refresh(&kp);

			//in case 1 is selected -> view date (status 13)
			if (Keypad_Get_Key(&kp, kp_button_1)
					&& (current_tick >= general_delay)) {

				status = 13;

			}

			//2 is selected -> view parameters (status 14)
			else if (Keypad_Get_Key(&kp, kp_button_2)
					&& (current_tick >= general_delay)) {

				status = 14;

			}

			//back is selected
			else if (Keypad_Get_Key(&kp, kp_button_no_back)
					&& (current_tick >= general_delay)) {

				//back to previous menu
				status = 10;

			}

			general_delay = HAL_GetTick() + 250;
		}

		//view the time and date -> status 13
		while ((status == 13) && (current_tick >= general_delay)) {

			//get the current tick number
			current_tick = HAL_GetTick();

			//displaying the time and date
			//the lcd will display the time
			// Read time from DS1307
			if (Ds1307_read(&CLK) == DS1307_OK) //in case reading is ok
					{

				//check if the time format is 24h or 12h
				//in case of 12h
				if (CLK.format == 1) {

					// display the time
					Alcd_Clear(&lcd);
					Alcd_Display_Control(&lcd, 1, 0, 0);
					snprintf(timeString, sizeof(timeString), "%02d:%02d:%02d",
							CLK.hour, CLK.min, CLK.sec);
					Alcd_PutAt_n(&lcd, 1, 0, timeString, strlen(timeString));

					//check for PM or AM
					//in case: AM
					if (CLK.AM_PM == 0) {
						Alcd_PutAt(&lcd, 1, 9, "AM");

					} else {
						Alcd_PutAt(&lcd, 1, 9, "PM");

					}

					snprintf(timeString, sizeof(timeString), "%02d-%02d-%04d",
							CLK.date, CLK.month, CLK.year);
					Alcd_PutAt_n(&lcd, 0, 0, timeString, strlen(timeString));

					snprintf(timeString, sizeof(timeString), "%02d", status);
					Alcd_PutAt_n(&lcd, 0, 14, timeString, strlen(timeString));
				}

				//in case of 24 format
				else {

					snprintf(timeString, sizeof(timeString), "%02d:%02d:%02d",
							CLK.hour, CLK.min, CLK.sec);

					// display the time
					Alcd_Clear(&lcd);
					Alcd_Display_Control(&lcd, 1, 0, 0);
					Alcd_PutAt_n(&lcd, 1, 0, timeString, strlen(timeString));

					snprintf(timeString, sizeof(timeString), "%02d-%02d-%04d",
							CLK.date, CLK.month, CLK.year);
					Alcd_PutAt_n(&lcd, 0, 0, timeString, strlen(timeString));

					snprintf(timeString, sizeof(timeString), "%02d", status);
					Alcd_PutAt_n(&lcd, 0, 14, timeString, strlen(timeString));

				}

			} else {
				Alcd_Clear(&lcd);
				Alcd_PutAt(&lcd, 0, 0, "RTC failure");
				error_code = 1;
			}
			snprintf(timeString, sizeof(timeString), "%02d", status);
			Alcd_PutAt_n(&lcd, 0, 14, timeString, strlen(timeString));

			//check if back is selected
			Keypad_Refresh(&kp);
			if (Keypad_Get_Key(&kp, kp_button_no_back)
					&& (current_tick >= general_delay)) {

				//back to previous menu
				status = 11;

			}

			general_delay = HAL_GetTick() + 250;
		}

		//view the time parameters -> status 14
		while ((status == 14) && (current_tick >= general_delay)) {

			//get the current tick number
			current_tick = HAL_GetTick();
			//clear the lcd
			Alcd_Clear(&lcd);
			Alcd_PutAt(&lcd, 0, 0, "Dose @");

			//reading dosing hours
			eeprom24c32_read(&memory, &dose_h, dosing_time_hours);

			//reading dosing minutes
			eeprom24c32_read(&memory, &dose_m, dosing_time_minutes);

			//reading dosing seconds
			eeprom24c32_read(&memory, &dose_s, dosing_time_seconds);

			snprintf(timeString, sizeof(timeString), "%02d:%02d:%02d", dose_h,
					dose_m, dose_s);
			Alcd_PutAt_n(&lcd, 1, 0, timeString, strlen(timeString));

			//check if back or next is selected
			Keypad_Refresh(&kp);
			//in case back is selected
			if (Keypad_Get_Key(&kp, kp_button_no_back)
					&& (current_tick >= general_delay)) {

				//back to previous menu
				status = 11;

			}

			//in case next is selected
			else if (Keypad_Get_Key(&kp, kp_button_next)
					&& (current_tick >= general_delay)) {

				//back to display no of doses -> status 15
				status = 15;

			}

			general_delay = HAL_GetTick() + 250;
		}

		//view the no. of doses -> status 15
		while ((status == 15) && (current_tick >= general_delay)) {

			//get the current tick number
			current_tick = HAL_GetTick();
			//clear the lcd
			Alcd_Clear(&lcd);
			Alcd_PutAt(&lcd, 0, 0, "doses no.");

			//reading dosing hours
			eeprom24c32_read(&memory, &dose_num, doses_number);

			snprintf(timeString, sizeof(timeString), "%02d", dose_num);
			Alcd_PutAt_n(&lcd, 1, 0, timeString, strlen(timeString));

			//check if back or next is selected
			Keypad_Refresh(&kp);
			//in case back is selected
			if (Keypad_Get_Key(&kp, kp_button_no_back)
					&& (current_tick >= general_delay)) {

				//back to previous menu
				status = 11;

			}

			//in case previous button is selected
			if (Keypad_Get_Key(&kp, kp_button_previous)
					&& (current_tick >= general_delay)) {

				//back to previous menu
				status = 14;

			}

			general_delay = HAL_GetTick() + 250;
		}

		//edit menu -> status 20
		while ((status == 20) && (current_tick >= general_delay)) {

			//get the current tick number
			current_tick = HAL_GetTick();

			Alcd_PutAt(&lcd, 0, 0, "Password:");
			snprintf(timeString, sizeof(timeString), "%02d", status);
			Alcd_PutAt_n(&lcd, 0, 14, timeString, strlen(timeString));

			Alcd_CursorAt(&lcd, 1, input_index);
			Alcd_Display_Control(&lcd, 1, 1, 1);

			Keypad_Refresh(&kp);

			//in case back is selected
			if (Keypad_Get_Key(&kp, kp_button_no_back)
					&& (current_tick >= general_delay)) {

				input_index = 0;  // Reset input index

				//back to previous menu
				status = 11;

			}

			//if 0 is pressed
			else if (Keypad_Get_Key(&kp, kp_button_0)
					&& (current_tick >= general_delay)) {

				//turn off the blinking
				Alcd_Display_Control(&lcd, 1, 0, 0);

				// store '0' in the entered password
				if (input_index < 4) {
					entered_password[input_index] = '0';
					entered_password[input_index + 1] = '\0'; // Null-terminate the string
					Alcd_PutAt(&lcd, 1, input_index, "*");
					input_index++;
					Alcd_CursorAt(&lcd, 1, input_index); // Move cursor to next position
				}

				//turn on the blinking again
				Alcd_Display_Control(&lcd, 1, input_index, 1);

				// Update keypad_delay after handling the key press
				general_delay = HAL_GetTick() + 250;
			}

			//if 1 is pressed
			else if (Keypad_Get_Key(&kp, kp_button_1)
					&& (current_tick >= general_delay)) {

				//turn off the blinking
				Alcd_Display_Control(&lcd, 1, 0, 0);

				// store '0' in the entered password
				if (input_index < 4) {
					entered_password[input_index] = '1';
					entered_password[input_index + 1] = '\0'; // Null-terminate the string
					Alcd_PutAt(&lcd, 1, input_index, "*");
					input_index++;
					Alcd_CursorAt(&lcd, 1, input_index); // Move cursor to next position
				}

				//turn on the blinking again
				Alcd_Display_Control(&lcd, 1, input_index, 1);

				// Update keypad_delay after handling the key press
				general_delay = HAL_GetTick() + 250;
			}

			//if 2 is pressed
			else if (Keypad_Get_Key(&kp, kp_button_2)
					&& (current_tick >= general_delay)) {

				//turn off the blinking
				Alcd_Display_Control(&lcd, 1, 0, 0);

				// store '0' in the entered password
				if (input_index < 4) {
					entered_password[input_index] = '2';
					entered_password[input_index + 1] = '\0'; // Null-terminate the string
					Alcd_PutAt(&lcd, 1, input_index, "*");
					input_index++;
					Alcd_CursorAt(&lcd, 1, input_index); // Move cursor to next position
				}

				//turn on the blinking again
				Alcd_Display_Control(&lcd, 1, input_index, 1);

				// Update keypad_delay after handling the key press
				general_delay = HAL_GetTick() + 250;
			}

			//if 3 is pressed
			else if (Keypad_Get_Key(&kp, kp_button_3)
					&& (current_tick >= general_delay)) {

				//turn off the blinking
				Alcd_Display_Control(&lcd, 1, 0, 0);

				// store '0' in the entered password
				if (input_index < 4) {
					entered_password[input_index] = '3';
					entered_password[input_index + 1] = '\0'; // Null-terminate the string
					Alcd_PutAt(&lcd, 1, input_index, "*");
					input_index++;
					Alcd_CursorAt(&lcd, 1, input_index); // Move cursor to next position
				}

				//turn on the blinking again
				Alcd_Display_Control(&lcd, 1, input_index, 1);

				// Update keypad_delay after handling the key press
				general_delay = HAL_GetTick() + 250;
			}
			//if 4 is pressed
			else if (Keypad_Get_Key(&kp, kp_button_4)
					&& (current_tick >= general_delay)) {

				//turn off the blinking
				Alcd_Display_Control(&lcd, 1, 0, 0);

				// store '0' in the entered password
				if (input_index < 4) {
					entered_password[input_index] = '4';
					entered_password[input_index + 1] = '\0'; // Null-terminate the string
					Alcd_PutAt(&lcd, 1, input_index, "*");
					input_index++;
					Alcd_CursorAt(&lcd, 1, input_index); // Move cursor to next position
				}

				//turn on the blinking again
				Alcd_Display_Control(&lcd, 1, input_index, 1);

				// Update keypad_delay after handling the key press
				general_delay = HAL_GetTick() + 250;
			}

			//if 5 is pressed
			else if (Keypad_Get_Key(&kp, kp_button_5)
					&& (current_tick >= general_delay)) {

				//turn off the blinking
				Alcd_Display_Control(&lcd, 1, 0, 0);

				// store '0' in the entered password
				if (input_index < 4) {
					entered_password[input_index] = '5';
					entered_password[input_index + 1] = '\0'; // Null-terminate the string
					Alcd_PutAt(&lcd, 1, input_index, "*");
					input_index++;
					Alcd_CursorAt(&lcd, 1, input_index); // Move cursor to next position
				}

				//turn on the blinking again
				Alcd_Display_Control(&lcd, 1, input_index, 1);

				// Update keypad_delay after handling the key press
				general_delay = HAL_GetTick() + 250;
			}

			//if 6 is pressed
			else if (Keypad_Get_Key(&kp, kp_button_6)
					&& (current_tick >= general_delay)) {

				//turn off the blinking
				Alcd_Display_Control(&lcd, 1, 0, 0);

				// store '0' in the entered password
				if (input_index < 4) {
					entered_password[input_index] = '6';
					entered_password[input_index + 1] = '\0'; // Null-terminate the string
					Alcd_PutAt(&lcd, 1, input_index, "*");
					input_index++;
					Alcd_CursorAt(&lcd, 1, input_index); // Move cursor to next position
				}

				//turn on the blinking again
				Alcd_Display_Control(&lcd, 1, input_index, 1);

				// Update keypad_delay after handling the key press
				general_delay = HAL_GetTick() + 250;
			}
			//if 7 is pressed
			else if (Keypad_Get_Key(&kp, kp_button_7)
					&& (current_tick >= general_delay)) {

				//turn off the blinking
				Alcd_Display_Control(&lcd, 1, 0, 0);

				// store '0' in the entered password
				if (input_index < 4) {
					entered_password[input_index] = '7';
					entered_password[input_index + 1] = '\0'; // Null-terminate the string
					Alcd_PutAt(&lcd, 1, input_index, "*");
					input_index++;
					Alcd_CursorAt(&lcd, 1, input_index); // Move cursor to next position
				}

				//turn on the blinking again
				Alcd_Display_Control(&lcd, 1, input_index, 1);

				// Update keypad_delay after handling the key press
				general_delay = HAL_GetTick() + 250;
			}
			//if 8 is pressed
			else if (Keypad_Get_Key(&kp, kp_button_8)
					&& (current_tick >= general_delay)) {

				//turn off the blinking
				Alcd_Display_Control(&lcd, 1, 0, 0);

				// store '0' in the entered password
				if (input_index < 4) {
					entered_password[input_index] = '8';
					entered_password[input_index + 1] = '\0'; // Null-terminate the string
					Alcd_PutAt(&lcd, 1, input_index, "*");
					input_index++;
					Alcd_CursorAt(&lcd, 1, input_index); // Move cursor to next position
				}

				//turn on the blinking again
				Alcd_Display_Control(&lcd, 1, input_index, 1);

				// Update keypad_delay after handling the key press
				general_delay = HAL_GetTick() + 250;
			}
			//if 9 is pressed
			else if (Keypad_Get_Key(&kp, kp_button_9)
					&& (current_tick >= general_delay)) {

				//turn off the blinking
				Alcd_Display_Control(&lcd, 1, 0, 0);

				// store '0' in the entered password
				if (input_index < 4) {
					entered_password[input_index] = '9';
					entered_password[input_index + 1] = '\0'; // Null-terminate the string
					Alcd_PutAt(&lcd, 1, input_index, "*");
					input_index++;
					Alcd_CursorAt(&lcd, 1, input_index); // Move cursor to next position
				}

				//turn on the blinking again
				Alcd_Display_Control(&lcd, 1, input_index, 1);

				// Update keypad_delay after handling the key press
				general_delay = HAL_GetTick() + 250;
			}

			//if yes is pressed
			else if (Keypad_Get_Key(&kp, kp_button_yes)
					&& (current_tick >= general_delay)) {
				// Compare entered password with the saved password (menu_pass)
				if (strcmp(entered_password, menu_pass) == 0) {

					// Password is correct, proceed to the next menu or operation
					Alcd_Clear(&lcd);
					Alcd_PutAt(&lcd, 0, 0, "Access Granted");
					//HAL_Delay(2000);

					//raise the password compare flag
					password_comp_flag = 1;

					Alcd_Display_Control(&lcd, 1, 0, 0);

					status = 21;

					input_index = 0; //reset the input index
					general_delay = HAL_GetTick() + 1500;
				}

				else {
					// Password is incorrect, display error message
					Alcd_Clear(&lcd);
					Alcd_PutAt(&lcd, 0, 0, "Wrong Password");

					//raise the password compare flag
					password_comp_flag = 1;

					input_index = 0; //reset the input index
					Alcd_Display_Control(&lcd, 1, 0, 0);

					status = 10;

					general_delay = HAL_GetTick() + 1500;

				}

				//create a delay
				if ((password_comp_flag == 1)
						&& (current_tick >= general_delay)) {

					password_comp_flag = 0;
				}
			}

		}

		//view the edit menu -> status 21
		while ((status == 21) && (current_tick >= general_delay)) {

			//get the current tick number
			current_tick = HAL_GetTick();

			Alcd_Clear(&lcd);
			Alcd_PutAt(&lcd, 0, 0, "1: parameters");
			Alcd_PutAt(&lcd, 1, 0, "2: calibration");
			snprintf(timeString, sizeof(timeString), "%02d", status);
			Alcd_PutAt_n(&lcd, 0, 14, timeString, strlen(timeString));

			//check if any button is pressed
			//check for any keypad input
			Keypad_Refresh(&kp);

			//in case 1 is selected -> edit parameters (status 23)
			if (Keypad_Get_Key(&kp, kp_button_1)
					&& (current_tick >= general_delay)) {

				status = 23;

			}

			//2 is selected -> calibrate dosing time (status 24)
			else if (Keypad_Get_Key(&kp, kp_button_2)
					&& (current_tick >= general_delay)) {

				status = 24;

			}

			//back is selected
			else if (Keypad_Get_Key(&kp, kp_button_no_back)
					&& (current_tick >= general_delay)) {

				//back to previous menu
				status = 10;

			}
			//next is selected
			else if (Keypad_Get_Key(&kp, kp_button_next)
					&& (current_tick >= general_delay)) {

				//go to next state
				status = 22;

			}

			general_delay = HAL_GetTick() + 250;
		}

		//view the extended edit menu -> status 22
		while ((status == 22) && (current_tick >= general_delay)) {

			//get the current tick number
			current_tick = HAL_GetTick();

			Alcd_Clear(&lcd);
			Alcd_PutAt(&lcd, 0, 0, "3: time");
			Alcd_PutAt(&lcd, 1, 0, "4: date");
			snprintf(timeString, sizeof(timeString), "%02d", status);
			Alcd_PutAt_n(&lcd, 0, 14, timeString, strlen(timeString));

			//check if any button is pressed
			//check for any keypad input
			Keypad_Refresh(&kp);

			//in case 3 is selected -> edit time (status 25)
			if (Keypad_Get_Key(&kp, kp_button_3)
					&& (current_tick >= general_delay)) {
				Alcd_Clear(&lcd);
				status = 25;

			}

			//4 is selected -> edit date (status 26)
			else if (Keypad_Get_Key(&kp, kp_button_4)
					&& (current_tick >= general_delay)) {

				Alcd_Clear(&lcd);
				status = 26;

			}

			//back is selected
			else if (Keypad_Get_Key(&kp, kp_button_no_back)
					&& (current_tick >= general_delay)) {

				//back to previous menu
				status = 10;

			}
			//previous is selected
			else if (Keypad_Get_Key(&kp, kp_button_previous)
					&& (current_tick >= general_delay)) {

				//go to previous state
				status = 21;

			}

			//next is selected
			else if (Keypad_Get_Key(&kp, kp_button_next)
					&& (current_tick >= general_delay)) {

				//go to next state
				status = 27;

			}

			general_delay = HAL_GetTick() + 250;
		}

		//edit the time
		while ((status == 25) && (current_tick >= general_delay)) {

			//get the current tick number
			current_tick = HAL_GetTick();

			Alcd_Clear(&lcd);
			Alcd_PutAt(&lcd, 0, 0, "format");
			Alcd_PutAt(&lcd, 1, 0, "1: 12h    2: 24h");
			snprintf(timeString, sizeof(timeString), "%02d", status);
			Alcd_PutAt_n(&lcd, 0, 14, timeString, strlen(timeString));

			//check if any button is pressed
			//check for any keypad input
			Keypad_Refresh(&kp);

			//in case 1 is selected -> 12h format (status 28)
			if (Keypad_Get_Key(&kp, kp_button_1)
					&& (current_tick >= general_delay)) {

				//set 12h format
				CLK.format = 1;
				status = 28;

			}

			//2 is selected -> 24h format (status 29)
			else if (Keypad_Get_Key(&kp, kp_button_2)
					&& (current_tick >= general_delay)) {

				//set 24h format
				CLK.format = 0;

				status = 17;

			}

			//back is selected
			else if (Keypad_Get_Key(&kp, kp_button_no_back)
					&& (current_tick >= general_delay)) {

				//back to previous menu
				status = 22;

			}

			general_delay = HAL_GetTick() + 250;
		}

		//12h time format
		while ((status == 28) && (current_tick >= general_delay)) {

			//get the current tick number
			current_tick = HAL_GetTick();

			Alcd_Clear(&lcd);
			Alcd_PutAt(&lcd, 0, 0, "format");
			Alcd_PutAt(&lcd, 1, 0, "1: AM    2: 2PM");
			snprintf(timeString, sizeof(timeString), "%02d", status);
			Alcd_PutAt_n(&lcd, 0, 14, timeString, strlen(timeString));

			//check if any button is pressed
			//check for any keypad input
			Keypad_Refresh(&kp);

			//in case 1 is selected -> AM (status 29)
			if (Keypad_Get_Key(&kp, kp_button_1)
					&& (current_tick >= general_delay)) {

				//set to AM
				CLK.AM_PM = 0;
				Alcd_Clear(&lcd);

				status = 17;

			}

			//2 is selected -> PM (status 29)
			else if (Keypad_Get_Key(&kp, kp_button_2)
					&& (current_tick >= general_delay)) {

				//set to pm
				CLK.AM_PM = 1;
				Alcd_Clear(&lcd);

				status = 17;

			}

			//back is selected
			else if (Keypad_Get_Key(&kp, kp_button_no_back)
					&& (current_tick >= general_delay)) {

				//back to previous menu
				status = 25;

			}

			general_delay = HAL_GetTick() + 250;
		}

		//transition state to clear the display
		while ((status == 17) && (current_tick >= general_delay)) {

			//get the current tick number
			current_tick = HAL_GetTick();

			Alcd_Clear(&lcd);
			status = 29;
			general_delay = HAL_GetTick() + 50;
		}

		//entering the hours state (29)
		while ((status == 29) && (current_tick >= general_delay)) {

			//get the current tick number
			current_tick = HAL_GetTick();

			//Alcd_Clear(&lcd);
			Alcd_PutAt(&lcd, 0, 0, "Enter hours");

			snprintf(timeString, sizeof(timeString), "%02d", status);
			Alcd_PutAt_n(&lcd, 0, 14, timeString, strlen(timeString));

			Alcd_CursorAt(&lcd, 1, input_index);
			Alcd_Display_Control(&lcd, 1, 1, 1);

			//check if any button is pressed
			//check for any keypad input
			Keypad_Refresh(&kp);

			//if back is entered
			if (Keypad_Get_Key(&kp, kp_button_no_back)
					&& (current_tick >= general_delay)) {

				//back to previous menu
				status = 25;

			}

			//0 is entered
			else if (Keypad_Get_Key(&kp, kp_button_0)
					&& (current_tick >= general_delay)) {

				//turn off the blinking
				Alcd_Display_Control(&lcd, 1, 0, 0);

				// store '0' in the entered time date buffer
				if (input_index < 2) {
					time_date_buffer[input_index] = '0';
					time_date_buffer[input_index + 1] = '\0'; // Null-terminate the string
					Alcd_PutAt(&lcd, 1, input_index, "0");
					input_index++;
					Alcd_CursorAt(&lcd, 1, input_index); // Move cursor to next position
				}

				//turn on the blinking again
				Alcd_Display_Control(&lcd, 1, input_index, 1);

				// Update keypad_delay after handling the key press
				general_delay = HAL_GetTick() + 250;

			}
			//1 is entered
			else if (Keypad_Get_Key(&kp, kp_button_1)
					&& (current_tick >= general_delay)) {

				//turn off the blinking
				Alcd_Display_Control(&lcd, 1, 0, 0);

				// store '0' in the entered time date buffer
				if (input_index < 2) {
					time_date_buffer[input_index] = '1';
					time_date_buffer[input_index + 1] = '\0'; // Null-terminate the string
					Alcd_PutAt(&lcd, 1, input_index, "1");
					input_index++;
					Alcd_CursorAt(&lcd, 1, input_index); // Move cursor to next position
				}

				//turn on the blinking again
				Alcd_Display_Control(&lcd, 1, input_index, 1);

				// Update keypad_delay after handling the key press
				general_delay = HAL_GetTick() + 250;

			}
			//2 is entered
			else if (Keypad_Get_Key(&kp, kp_button_2)
					&& (current_tick >= general_delay)) {

				//turn off the blinking
				Alcd_Display_Control(&lcd, 1, 0, 0);

				// store '0' in the entered time date buffer
				if (input_index < 2) {
					time_date_buffer[input_index] = '2';
					time_date_buffer[input_index + 1] = '\0'; // Null-terminate the string
					Alcd_PutAt(&lcd, 1, input_index, "2");
					input_index++;
					Alcd_CursorAt(&lcd, 1, input_index); // Move cursor to next position
				}

				//turn on the blinking again
				Alcd_Display_Control(&lcd, 1, input_index, 1);

				// Update keypad_delay after handling the key press
				general_delay = HAL_GetTick() + 250;

			}
			//3 is entered
			else if (Keypad_Get_Key(&kp, kp_button_3)
					&& (current_tick >= general_delay)) {

				//turn off the blinking
				Alcd_Display_Control(&lcd, 1, 0, 0);

				// store '0' in the entered time date buffer
				if (input_index < 2) {
					time_date_buffer[input_index] = '3';
					time_date_buffer[input_index + 1] = '\0'; // Null-terminate the string
					Alcd_PutAt(&lcd, 1, input_index, "3");
					input_index++;
					Alcd_CursorAt(&lcd, 1, input_index); // Move cursor to next position
				}

				//turn on the blinking again
				Alcd_Display_Control(&lcd, 1, input_index, 1);

				// Update keypad_delay after handling the key press
				general_delay = HAL_GetTick() + 250;

			}

			//4 is entered
			else if (Keypad_Get_Key(&kp, kp_button_4)
					&& (current_tick >= general_delay)) {

				//turn off the blinking
				Alcd_Display_Control(&lcd, 1, 0, 0);

				// store '0' in the entered time date buffer
				if (input_index < 2) {
					time_date_buffer[input_index] = '4';
					time_date_buffer[input_index + 1] = '\0'; // Null-terminate the string
					Alcd_PutAt(&lcd, 1, input_index, "4");
					input_index++;
					Alcd_CursorAt(&lcd, 1, input_index); // Move cursor to next position
				}

				//turn on the blinking again
				Alcd_Display_Control(&lcd, 1, input_index, 1);

				// Update keypad_delay after handling the key press
				general_delay = HAL_GetTick() + 250;

			}
			//5 is entered
			else if (Keypad_Get_Key(&kp, kp_button_5)
					&& (current_tick >= general_delay)) {

				//turn off the blinking
				Alcd_Display_Control(&lcd, 1, 0, 0);

				// store '0' in the entered time date buffer
				if (input_index < 2) {
					time_date_buffer[input_index] = '5';
					time_date_buffer[input_index + 1] = '\0'; // Null-terminate the string
					Alcd_PutAt(&lcd, 1, input_index, "5");
					input_index++;
					Alcd_CursorAt(&lcd, 1, input_index); // Move cursor to next position
				}

				//turn on the blinking again
				Alcd_Display_Control(&lcd, 1, input_index, 1);

				// Update keypad_delay after handling the key press
				general_delay = HAL_GetTick() + 250;

			}

			//6 is entered
			else if (Keypad_Get_Key(&kp, kp_button_6)
					&& (current_tick >= general_delay)) {

				//turn off the blinking
				Alcd_Display_Control(&lcd, 1, 0, 0);

				// store '0' in the entered time date buffer
				if (input_index < 2) {
					time_date_buffer[input_index] = '6';
					time_date_buffer[input_index + 1] = '\0'; // Null-terminate the string
					Alcd_PutAt(&lcd, 1, input_index, "6");
					input_index++;
					Alcd_CursorAt(&lcd, 1, input_index); // Move cursor to next position
				}

				//turn on the blinking again
				Alcd_Display_Control(&lcd, 1, input_index, 1);

				// Update keypad_delay after handling the key press
				general_delay = HAL_GetTick() + 250;

			}

			//7 is entered
			else if (Keypad_Get_Key(&kp, kp_button_7)
					&& (current_tick >= general_delay)) {

				//turn off the blinking
				Alcd_Display_Control(&lcd, 1, 0, 0);

				// store '0' in the entered time date buffer
				if (input_index < 2) {
					time_date_buffer[input_index] = '7';
					time_date_buffer[input_index + 1] = '\0'; // Null-terminate the string
					Alcd_PutAt(&lcd, 1, input_index, "7");
					input_index++;
					Alcd_CursorAt(&lcd, 1, input_index); // Move cursor to next position
				}

				//turn on the blinking again
				Alcd_Display_Control(&lcd, 1, input_index, 1);

				// Update keypad_delay after handling the key press
				general_delay = HAL_GetTick() + 250;

			}

			//8 is entered
			else if (Keypad_Get_Key(&kp, kp_button_8)
					&& (current_tick >= general_delay)) {

				//turn off the blinking
				Alcd_Display_Control(&lcd, 1, 0, 0);

				// store '0' in the entered time date buffer
				if (input_index < 2) {
					time_date_buffer[input_index] = '8';
					time_date_buffer[input_index + 1] = '\0'; // Null-terminate the string
					Alcd_PutAt(&lcd, 1, input_index, "8");
					input_index++;
					Alcd_CursorAt(&lcd, 1, input_index); // Move cursor to next position
				}

				//turn on the blinking again
				Alcd_Display_Control(&lcd, 1, input_index, 1);

				// Update keypad_delay after handling the key press
				general_delay = HAL_GetTick() + 250;

			}

			//9 is entered
			else if (Keypad_Get_Key(&kp, kp_button_9)
					&& (current_tick >= general_delay)) {

				//turn off the blinking
				Alcd_Display_Control(&lcd, 1, 0, 0);

				// store '0' in the entered time date buffer
				if (input_index < 2) {
					time_date_buffer[input_index] = '9';
					time_date_buffer[input_index + 1] = '\0'; // Null-terminate the string
					Alcd_PutAt(&lcd, 1, input_index, "9");
					input_index++;
					Alcd_CursorAt(&lcd, 1, input_index); // Move cursor to next position
				}

				//turn on the blinking again
				Alcd_Display_Control(&lcd, 1, input_index, 1);

				// Update keypad_delay after handling the key press
				general_delay = HAL_GetTick() + 250;

			}

			//when yes is entered -> check for hour validity (state 30)
			if (Keypad_Get_Key(&kp, kp_button_yes)
					&& (current_tick >= general_delay)) {

				Alcd_Clear(&lcd);

				snprintf(timeString, sizeof(timeString), "%02d", buffer);
				Alcd_PutAt_n(&lcd, 1, 0, timeString, strlen(timeString));

				status = 30;

			}

			//
			general_delay = HAL_GetTick() + 250;
		}

		//hours validation phase
		while ((status == 30) && (current_tick >= general_delay)) {

			//get the current tick number
			current_tick = HAL_GetTick();

			Alcd_Clear(&lcd);

			snprintf(timeString, sizeof(timeString), "%02d", status);
			Alcd_PutAt_n(&lcd, 0, 14, timeString, strlen(timeString));

			buffer = atoi(time_date_buffer);

			//in case of 12h format
			if (CLK.format == 1) {

				//check if time is within rang 1 to 12
				if (buffer > 0 && buffer < 13) {

					//move to the minutes state
					status = 31;

					//set the hours to the value
					CLK.hour = buffer;

					input_index = 0;
					//turn off the blinking
					Alcd_Display_Control(&lcd, 1, 0, 0);
					//turn on the blinking again
					Alcd_Display_Control(&lcd, 1, input_index, 1);

				} else {
					Alcd_PutAt(&lcd, 0, 0, "invalid");
					delay_flag = 1;
					input_index = 0;

					//turn off the blinking
					Alcd_Display_Control(&lcd, 1, 0, 0);
					//turn on the blinking again
					Alcd_Display_Control(&lcd, 1, input_index, 1);

					//return to entering hours
					status = 29;

					general_delay = HAL_GetTick() + 1000;
				}
			}

			//in case of 24h format
			if (CLK.format == 0) {

				//check if time is within rang 0 to 24
				if (buffer >= 0 && buffer < 25) {

					//move to the minutes state
					status = 31;

					//set the hours to the value
					CLK.hour = buffer - 20;

					input_index = 0;
					//turn off the blinking
					Alcd_Display_Control(&lcd, 1, 0, 0);
					//turn on the blinking again
					Alcd_Display_Control(&lcd, 1, input_index, 1);
				} else {
					Alcd_PutAt(&lcd, 0, 0, "invalid");
					delay_flag = 1;

					input_index = 0;
					//turn off the blinking
					Alcd_Display_Control(&lcd, 1, 0, 0);
					//turn on the blinking again
					Alcd_Display_Control(&lcd, 1, input_index, 1);
					//return to entering hours
					status = 29;

					general_delay = HAL_GetTick() + 1000;
				}
			}

			//create a delay
			if ((delay_flag == 1) && (current_tick >= general_delay)) {

				delay_flag = 0;
			}

		}

		//entering the minutes state (31)
		while ((status == 31) && (current_tick >= general_delay)) {

			//get the current tick number
			current_tick = HAL_GetTick();

			//Alcd_Clear(&lcd);
			Alcd_PutAt(&lcd, 0, 0, "Enter minutes");

			snprintf(timeString, sizeof(timeString), "%02d", status);
			Alcd_PutAt_n(&lcd, 0, 14, timeString, strlen(timeString));

			Alcd_CursorAt(&lcd, 1, input_index);
			Alcd_Display_Control(&lcd, 1, 1, 1);

			//check if any button is pressed
			//check for any keypad input
			Keypad_Refresh(&kp);

			//if back is entered
			if (Keypad_Get_Key(&kp, kp_button_no_back)
					&& (current_tick >= general_delay)) {

				//back to previous menu
				status = 29;

			}

			//0 is entered
			else if (Keypad_Get_Key(&kp, kp_button_0)
					&& (current_tick >= general_delay)) {

				//turn off the blinking
				Alcd_Display_Control(&lcd, 1, 0, 0);

				// store '0' in the entered time date buffer
				if (input_index < 2) {
					time_date_buffer[input_index] = '0';
					time_date_buffer[input_index + 1] = '\0'; // Null-terminate the string
					Alcd_PutAt(&lcd, 1, input_index, "0");
					input_index++;
					Alcd_CursorAt(&lcd, 1, input_index); // Move cursor to next position
				}

				//turn on the blinking again
				Alcd_Display_Control(&lcd, 1, input_index, 1);

				// Update keypad_delay after handling the key press
				general_delay = HAL_GetTick() + 250;

			}
			//1 is entered
			else if (Keypad_Get_Key(&kp, kp_button_1)
					&& (current_tick >= general_delay)) {

				//turn off the blinking
				Alcd_Display_Control(&lcd, 1, 0, 0);

				// store '0' in the entered time date buffer
				if (input_index < 2) {
					time_date_buffer[input_index] = '1';
					time_date_buffer[input_index + 1] = '\0'; // Null-terminate the string
					Alcd_PutAt(&lcd, 1, input_index, "1");
					input_index++;
					Alcd_CursorAt(&lcd, 1, input_index); // Move cursor to next position
				}

				//turn on the blinking again
				Alcd_Display_Control(&lcd, 1, input_index, 1);

				// Update keypad_delay after handling the key press
				general_delay = HAL_GetTick() + 250;

			}
			//2 is entered
			else if (Keypad_Get_Key(&kp, kp_button_2)
					&& (current_tick >= general_delay)) {

				//turn off the blinking
				Alcd_Display_Control(&lcd, 1, 0, 0);

				// store '0' in the entered time date buffer
				if (input_index < 2) {
					time_date_buffer[input_index] = '2';
					time_date_buffer[input_index + 1] = '\0'; // Null-terminate the string
					Alcd_PutAt(&lcd, 1, input_index, "2");
					input_index++;
					Alcd_CursorAt(&lcd, 1, input_index); // Move cursor to next position
				}

				//turn on the blinking again
				Alcd_Display_Control(&lcd, 1, input_index, 1);

				// Update keypad_delay after handling the key press
				general_delay = HAL_GetTick() + 250;

			}
			//3 is entered
			else if (Keypad_Get_Key(&kp, kp_button_3)
					&& (current_tick >= general_delay)) {

				//turn off the blinking
				Alcd_Display_Control(&lcd, 1, 0, 0);

				// store '0' in the entered time date buffer
				if (input_index < 2) {
					time_date_buffer[input_index] = '3';
					time_date_buffer[input_index + 1] = '\0'; // Null-terminate the string
					Alcd_PutAt(&lcd, 1, input_index, "3");
					input_index++;
					Alcd_CursorAt(&lcd, 1, input_index); // Move cursor to next position
				}

				//turn on the blinking again
				Alcd_Display_Control(&lcd, 1, input_index, 1);

				// Update keypad_delay after handling the key press
				general_delay = HAL_GetTick() + 250;

			}

			//4 is entered
			else if (Keypad_Get_Key(&kp, kp_button_4)
					&& (current_tick >= general_delay)) {

				//turn off the blinking
				Alcd_Display_Control(&lcd, 1, 0, 0);

				// store '0' in the entered time date buffer
				if (input_index < 2) {
					time_date_buffer[input_index] = '4';
					time_date_buffer[input_index + 1] = '\0'; // Null-terminate the string
					Alcd_PutAt(&lcd, 1, input_index, "4");
					input_index++;
					Alcd_CursorAt(&lcd, 1, input_index); // Move cursor to next position
				}

				//turn on the blinking again
				Alcd_Display_Control(&lcd, 1, input_index, 1);

				// Update keypad_delay after handling the key press
				general_delay = HAL_GetTick() + 250;

			}
			//5 is entered
			else if (Keypad_Get_Key(&kp, kp_button_5)
					&& (current_tick >= general_delay)) {

				//turn off the blinking
				Alcd_Display_Control(&lcd, 1, 0, 0);

				// store '0' in the entered time date buffer
				if (input_index < 2) {
					time_date_buffer[input_index] = '5';
					time_date_buffer[input_index + 1] = '\0'; // Null-terminate the string
					Alcd_PutAt(&lcd, 1, input_index, "5");
					input_index++;
					Alcd_CursorAt(&lcd, 1, input_index); // Move cursor to next position
				}

				//turn on the blinking again
				Alcd_Display_Control(&lcd, 1, input_index, 1);

				// Update keypad_delay after handling the key press
				general_delay = HAL_GetTick() + 250;

			}

			//6 is entered
			else if (Keypad_Get_Key(&kp, kp_button_6)
					&& (current_tick >= general_delay)) {

				//turn off the blinking
				Alcd_Display_Control(&lcd, 1, 0, 0);

				// store '0' in the entered time date buffer
				if (input_index < 2) {
					time_date_buffer[input_index] = '6';
					time_date_buffer[input_index + 1] = '\0'; // Null-terminate the string
					Alcd_PutAt(&lcd, 1, input_index, "6");
					input_index++;
					Alcd_CursorAt(&lcd, 1, input_index); // Move cursor to next position
				}

				//turn on the blinking again
				Alcd_Display_Control(&lcd, 1, input_index, 1);

				// Update keypad_delay after handling the key press
				general_delay = HAL_GetTick() + 250;

			}

			//7 is entered
			else if (Keypad_Get_Key(&kp, kp_button_7)
					&& (current_tick >= general_delay)) {

				//turn off the blinking
				Alcd_Display_Control(&lcd, 1, 0, 0);

				// store '0' in the entered time date buffer
				if (input_index < 2) {
					time_date_buffer[input_index] = '7';
					time_date_buffer[input_index + 1] = '\0'; // Null-terminate the string
					Alcd_PutAt(&lcd, 1, input_index, "7");
					input_index++;
					Alcd_CursorAt(&lcd, 1, input_index); // Move cursor to next position
				}

				//turn on the blinking again
				Alcd_Display_Control(&lcd, 1, input_index, 1);

				// Update keypad_delay after handling the key press
				general_delay = HAL_GetTick() + 250;

			}

			//8 is entered
			else if (Keypad_Get_Key(&kp, kp_button_8)
					&& (current_tick >= general_delay)) {

				//turn off the blinking
				Alcd_Display_Control(&lcd, 1, 0, 0);

				// store '0' in the entered time date buffer
				if (input_index < 2) {
					time_date_buffer[input_index] = '8';
					time_date_buffer[input_index + 1] = '\0'; // Null-terminate the string
					Alcd_PutAt(&lcd, 1, input_index, "8");
					input_index++;
					Alcd_CursorAt(&lcd, 1, input_index); // Move cursor to next position
				}

				//turn on the blinking again
				Alcd_Display_Control(&lcd, 1, input_index, 1);

				// Update keypad_delay after handling the key press
				general_delay = HAL_GetTick() + 250;

			}

			//9 is entered
			else if (Keypad_Get_Key(&kp, kp_button_9)
					&& (current_tick >= general_delay)) {

				//turn off the blinking
				Alcd_Display_Control(&lcd, 1, 0, 0);

				// store '0' in the entered time date buffer
				if (input_index < 2) {
					time_date_buffer[input_index] = '9';
					time_date_buffer[input_index + 1] = '\0'; // Null-terminate the string
					Alcd_PutAt(&lcd, 1, input_index, "9");
					input_index++;
					Alcd_CursorAt(&lcd, 1, input_index); // Move cursor to next position
				}

				//turn on the blinking again
				Alcd_Display_Control(&lcd, 1, input_index, 1);

				// Update keypad_delay after handling the key press
				general_delay = HAL_GetTick() + 250;

			}

			//when yes is entered -> check for miinutes validity (state 32)
			if (Keypad_Get_Key(&kp, kp_button_yes)
					&& (current_tick >= general_delay)) {

				Alcd_Clear(&lcd);

				snprintf(timeString, sizeof(timeString), "%02d", buffer);
				Alcd_PutAt_n(&lcd, 1, 0, timeString, strlen(timeString));

				status = 32;

			}

			//
			general_delay = HAL_GetTick() + 250;
		}

		//minutes validation phase
		while ((status == 32) && (current_tick >= general_delay)) {

			//get the current tick number
			current_tick = HAL_GetTick();

			Alcd_Clear(&lcd);

			snprintf(timeString, sizeof(timeString), "%02d", status);
			Alcd_PutAt_n(&lcd, 0, 14, timeString, strlen(timeString));

			buffer = atoi(time_date_buffer);

			//check if minutes is within the range 0 to 60
			if (buffer >= 0 && buffer < 61) {

				//move to the seconds state
				status = 33;

				//set the minutes to the value
				CLK.min = buffer;

				input_index = 0;
				//turn off the blinking
				Alcd_Display_Control(&lcd, 1, 0, 0);
				//turn on the blinking again
				Alcd_Display_Control(&lcd, 1, input_index, 1);
			} else {
				Alcd_PutAt(&lcd, 0, 0, "invalid");
				delay_flag = 1;

				input_index = 0;
				//turn off the blinking
				Alcd_Display_Control(&lcd, 1, 0, 0);
				//turn on the blinking again
				Alcd_Display_Control(&lcd, 1, input_index, 1);
				//return to entering hours
				status = 31;

				general_delay = HAL_GetTick() + 1000;
			}

			//create a delay
			if ((delay_flag == 1) && (current_tick >= general_delay)) {

				delay_flag = 0;
			}

		}

		//entering the seconds state (31)
		while ((status == 33) && (current_tick >= general_delay)) {

			//get the current tick number
			current_tick = HAL_GetTick();

			//Alcd_Clear(&lcd);
			Alcd_PutAt(&lcd, 0, 0, "Enter seconds");

			snprintf(timeString, sizeof(timeString), "%02d", status);
			Alcd_PutAt_n(&lcd, 0, 14, timeString, strlen(timeString));

			Alcd_CursorAt(&lcd, 1, input_index);
			Alcd_Display_Control(&lcd, 1, 1, 1);

			//check if any button is pressed
			//check for any keypad input
			Keypad_Refresh(&kp);

			//if back is entered
			if (Keypad_Get_Key(&kp, kp_button_no_back)
					&& (current_tick >= general_delay)) {

				//back to previous menu
				status = 31;

			}

			//0 is entered
			else if (Keypad_Get_Key(&kp, kp_button_0)
					&& (current_tick >= general_delay)) {

				//turn off the blinking
				Alcd_Display_Control(&lcd, 1, 0, 0);

				// store '0' in the entered time date buffer
				if (input_index < 2) {
					time_date_buffer[input_index] = '0';
					time_date_buffer[input_index + 1] = '\0'; // Null-terminate the string
					Alcd_PutAt(&lcd, 1, input_index, "0");
					input_index++;
					Alcd_CursorAt(&lcd, 1, input_index); // Move cursor to next position
				}

				//turn on the blinking again
				Alcd_Display_Control(&lcd, 1, input_index, 1);

				// Update keypad_delay after handling the key press
				general_delay = HAL_GetTick() + 250;

			}
			//1 is entered
			else if (Keypad_Get_Key(&kp, kp_button_1)
					&& (current_tick >= general_delay)) {

				//turn off the blinking
				Alcd_Display_Control(&lcd, 1, 0, 0);

				// store '0' in the entered time date buffer
				if (input_index < 2) {
					time_date_buffer[input_index] = '1';
					time_date_buffer[input_index + 1] = '\0'; // Null-terminate the string
					Alcd_PutAt(&lcd, 1, input_index, "1");
					input_index++;
					Alcd_CursorAt(&lcd, 1, input_index); // Move cursor to next position
				}

				//turn on the blinking again
				Alcd_Display_Control(&lcd, 1, input_index, 1);

				// Update keypad_delay after handling the key press
				general_delay = HAL_GetTick() + 250;

			}
			//2 is entered
			else if (Keypad_Get_Key(&kp, kp_button_2)
					&& (current_tick >= general_delay)) {

				//turn off the blinking
				Alcd_Display_Control(&lcd, 1, 0, 0);

				// store '0' in the entered time date buffer
				if (input_index < 2) {
					time_date_buffer[input_index] = '2';
					time_date_buffer[input_index + 1] = '\0'; // Null-terminate the string
					Alcd_PutAt(&lcd, 1, input_index, "2");
					input_index++;
					Alcd_CursorAt(&lcd, 1, input_index); // Move cursor to next position
				}

				//turn on the blinking again
				Alcd_Display_Control(&lcd, 1, input_index, 1);

				// Update keypad_delay after handling the key press
				general_delay = HAL_GetTick() + 250;

			}
			//3 is entered
			else if (Keypad_Get_Key(&kp, kp_button_3)
					&& (current_tick >= general_delay)) {

				//turn off the blinking
				Alcd_Display_Control(&lcd, 1, 0, 0);

				// store '0' in the entered time date buffer
				if (input_index < 2) {
					time_date_buffer[input_index] = '3';
					time_date_buffer[input_index + 1] = '\0'; // Null-terminate the string
					Alcd_PutAt(&lcd, 1, input_index, "3");
					input_index++;
					Alcd_CursorAt(&lcd, 1, input_index); // Move cursor to next position
				}

				//turn on the blinking again
				Alcd_Display_Control(&lcd, 1, input_index, 1);

				// Update keypad_delay after handling the key press
				general_delay = HAL_GetTick() + 250;

			}

			//4 is entered
			else if (Keypad_Get_Key(&kp, kp_button_4)
					&& (current_tick >= general_delay)) {

				//turn off the blinking
				Alcd_Display_Control(&lcd, 1, 0, 0);

				// store '0' in the entered time date buffer
				if (input_index < 2) {
					time_date_buffer[input_index] = '4';
					time_date_buffer[input_index + 1] = '\0'; // Null-terminate the string
					Alcd_PutAt(&lcd, 1, input_index, "4");
					input_index++;
					Alcd_CursorAt(&lcd, 1, input_index); // Move cursor to next position
				}

				//turn on the blinking again
				Alcd_Display_Control(&lcd, 1, input_index, 1);

				// Update keypad_delay after handling the key press
				general_delay = HAL_GetTick() + 250;

			}
			//5 is entered
			else if (Keypad_Get_Key(&kp, kp_button_5)
					&& (current_tick >= general_delay)) {

				//turn off the blinking
				Alcd_Display_Control(&lcd, 1, 0, 0);

				// store '0' in the entered time date buffer
				if (input_index < 2) {
					time_date_buffer[input_index] = '5';
					time_date_buffer[input_index + 1] = '\0'; // Null-terminate the string
					Alcd_PutAt(&lcd, 1, input_index, "5");
					input_index++;
					Alcd_CursorAt(&lcd, 1, input_index); // Move cursor to next position
				}

				//turn on the blinking again
				Alcd_Display_Control(&lcd, 1, input_index, 1);

				// Update keypad_delay after handling the key press
				general_delay = HAL_GetTick() + 250;

			}

			//6 is entered
			else if (Keypad_Get_Key(&kp, kp_button_6)
					&& (current_tick >= general_delay)) {

				//turn off the blinking
				Alcd_Display_Control(&lcd, 1, 0, 0);

				// store '0' in the entered time date buffer
				if (input_index < 2) {
					time_date_buffer[input_index] = '6';
					time_date_buffer[input_index + 1] = '\0'; // Null-terminate the string
					Alcd_PutAt(&lcd, 1, input_index, "6");
					input_index++;
					Alcd_CursorAt(&lcd, 1, input_index); // Move cursor to next position
				}

				//turn on the blinking again
				Alcd_Display_Control(&lcd, 1, input_index, 1);

				// Update keypad_delay after handling the key press
				general_delay = HAL_GetTick() + 250;

			}

			//7 is entered
			else if (Keypad_Get_Key(&kp, kp_button_7)
					&& (current_tick >= general_delay)) {

				//turn off the blinking
				Alcd_Display_Control(&lcd, 1, 0, 0);

				// store '0' in the entered time date buffer
				if (input_index < 2) {
					time_date_buffer[input_index] = '7';
					time_date_buffer[input_index + 1] = '\0'; // Null-terminate the string
					Alcd_PutAt(&lcd, 1, input_index, "7");
					input_index++;
					Alcd_CursorAt(&lcd, 1, input_index); // Move cursor to next position
				}

				//turn on the blinking again
				Alcd_Display_Control(&lcd, 1, input_index, 1);

				// Update keypad_delay after handling the key press
				general_delay = HAL_GetTick() + 250;

			}

			//8 is entered
			else if (Keypad_Get_Key(&kp, kp_button_8)
					&& (current_tick >= general_delay)) {

				//turn off the blinking
				Alcd_Display_Control(&lcd, 1, 0, 0);

				// store '0' in the entered time date buffer
				if (input_index < 2) {
					time_date_buffer[input_index] = '8';
					time_date_buffer[input_index + 1] = '\0'; // Null-terminate the string
					Alcd_PutAt(&lcd, 1, input_index, "8");
					input_index++;
					Alcd_CursorAt(&lcd, 1, input_index); // Move cursor to next position
				}

				//turn on the blinking again
				Alcd_Display_Control(&lcd, 1, input_index, 1);

				// Update keypad_delay after handling the key press
				general_delay = HAL_GetTick() + 250;

			}

			//9 is entered
			else if (Keypad_Get_Key(&kp, kp_button_9)
					&& (current_tick >= general_delay)) {

				//turn off the blinking
				Alcd_Display_Control(&lcd, 1, 0, 0);

				// store '0' in the entered time date buffer
				if (input_index < 2) {
					time_date_buffer[input_index] = '9';
					time_date_buffer[input_index + 1] = '\0'; // Null-terminate the string
					Alcd_PutAt(&lcd, 1, input_index, "9");
					input_index++;
					Alcd_CursorAt(&lcd, 1, input_index); // Move cursor to next position
				}

				//turn on the blinking again
				Alcd_Display_Control(&lcd, 1, input_index, 1);

				// Update keypad_delay after handling the key press
				general_delay = HAL_GetTick() + 250;

			}

			//when yes is entered -> check for seconds validity (state 34)
			if (Keypad_Get_Key(&kp, kp_button_yes)
					&& (current_tick >= general_delay)) {

				Alcd_Clear(&lcd);

				snprintf(timeString, sizeof(timeString), "%02d", buffer);
				Alcd_PutAt_n(&lcd, 1, 0, timeString, strlen(timeString));

				status = 34;

			}

			//
			general_delay = HAL_GetTick() + 250;
		}

		//seconds validation phase
		while ((status == 34) && (current_tick >= general_delay)) {

			//get the current tick number
			current_tick = HAL_GetTick();

			Alcd_Clear(&lcd);

			snprintf(timeString, sizeof(timeString), "%02d", status);
			Alcd_PutAt_n(&lcd, 0, 14, timeString, strlen(timeString));

			buffer = atoi(time_date_buffer);

			//check if minutes is within the range 0 to 60
			if (buffer >= 0 && buffer < 61) {

				//move to the confirmation state
				status = 35;

				//set the minutes to the value
				CLK.sec = buffer;

				input_index = 0;
				//turn off the blinking
				Alcd_Display_Control(&lcd, 1, 0, 0);
				//turn on the blinking again
				Alcd_Display_Control(&lcd, 1, input_index, 1);
			} else {
				Alcd_PutAt(&lcd, 0, 0, "invalid");
				delay_flag = 1;

				input_index = 0;
				//turn off the blinking
				Alcd_Display_Control(&lcd, 1, 0, 0);
				//turn on the blinking again
				Alcd_Display_Control(&lcd, 1, input_index, 1);
				//return to entering hours
				status = 33;

				general_delay = HAL_GetTick() + 1000;
			}

			//create a delay
			if ((delay_flag == 1) && (current_tick >= general_delay)) {

				delay_flag = 0;
			}

		}

		//time confirmation menu (state 35)
		while ((status == 35) && (current_tick >= general_delay)) {

			//get the current tick number
			current_tick = HAL_GetTick();

			Alcd_Clear(&lcd);
			Alcd_PutAt(&lcd, 0, 0, "confirm?");

			snprintf(timeString, sizeof(timeString), "%02d", status);
			Alcd_PutAt_n(&lcd, 0, 14, timeString, strlen(timeString));

			//check if any button is pressed
			//check for any keypad input
			Keypad_Refresh(&kp);

			//yes is selected
			if (Keypad_Get_Key(&kp, kp_button_yes)
					&& (current_tick >= general_delay)) {

				Ds1307_set(&CLK);

				//return to edit menu
				status = 21;

			}

			//back is selected -> back to seconds state
			else if (Keypad_Get_Key(&kp, kp_button_no_back)
					&& (current_tick >= general_delay)) {

				status = 33;

			}

			general_delay = HAL_GetTick() + 250;
		}

		//date edit menu -> enter day(state 26)
		while ((status == 26) && (current_tick >= general_delay)) {

			//get the current tick number
			current_tick = HAL_GetTick();

			//Alcd_Clear(&lcd);
			Alcd_PutAt(&lcd, 0, 0, "Enter day");

			snprintf(timeString, sizeof(timeString), "%02d", status);
			Alcd_PutAt_n(&lcd, 0, 14, timeString, strlen(timeString));

			Alcd_CursorAt(&lcd, 1, input_index);
			Alcd_Display_Control(&lcd, 1, 1, 1);

			//check if any button is pressed
			//check for any keypad input
			Keypad_Refresh(&kp);

			//if back is entered
			if (Keypad_Get_Key(&kp, kp_button_no_back)
					&& (current_tick >= general_delay)) {

				input_index = 0;

				//back to previous menu
				status = 22;

			}

			//0 is entered
			else if (Keypad_Get_Key(&kp, kp_button_0)
					&& (current_tick >= general_delay)) {

				//turn off the blinking
				Alcd_Display_Control(&lcd, 1, 0, 0);

				// store '0' in the entered time date buffer
				if (input_index < 2) {
					time_date_buffer[input_index] = '0';
					time_date_buffer[input_index + 1] = '\0'; // Null-terminate the string
					Alcd_PutAt(&lcd, 1, input_index, "0");
					input_index++;
					Alcd_CursorAt(&lcd, 1, input_index); // Move cursor to next position
				}

				//turn on the blinking again
				Alcd_Display_Control(&lcd, 1, input_index, 1);

				// Update keypad_delay after handling the key press
				general_delay = HAL_GetTick() + 250;

			}
			//1 is entered
			else if (Keypad_Get_Key(&kp, kp_button_1)
					&& (current_tick >= general_delay)) {

				//turn off the blinking
				Alcd_Display_Control(&lcd, 1, 0, 0);

				// store '0' in the entered time date buffer
				if (input_index < 2) {
					time_date_buffer[input_index] = '1';
					time_date_buffer[input_index + 1] = '\0'; // Null-terminate the string
					Alcd_PutAt(&lcd, 1, input_index, "1");
					input_index++;
					Alcd_CursorAt(&lcd, 1, input_index); // Move cursor to next position
				}

				//turn on the blinking again
				Alcd_Display_Control(&lcd, 1, input_index, 1);

				// Update keypad_delay after handling the key press
				general_delay = HAL_GetTick() + 250;

			}
			//2 is entered
			else if (Keypad_Get_Key(&kp, kp_button_2)
					&& (current_tick >= general_delay)) {

				//turn off the blinking
				Alcd_Display_Control(&lcd, 1, 0, 0);

				// store '0' in the entered time date buffer
				if (input_index < 2) {
					time_date_buffer[input_index] = '2';
					time_date_buffer[input_index + 1] = '\0'; // Null-terminate the string
					Alcd_PutAt(&lcd, 1, input_index, "2");
					input_index++;
					Alcd_CursorAt(&lcd, 1, input_index); // Move cursor to next position
				}

				//turn on the blinking again
				Alcd_Display_Control(&lcd, 1, input_index, 1);

				// Update keypad_delay after handling the key press
				general_delay = HAL_GetTick() + 250;

			}
			//3 is entered
			else if (Keypad_Get_Key(&kp, kp_button_3)
					&& (current_tick >= general_delay)) {

				//turn off the blinking
				Alcd_Display_Control(&lcd, 1, 0, 0);

				// store '0' in the entered time date buffer
				if (input_index < 2) {
					time_date_buffer[input_index] = '3';
					time_date_buffer[input_index + 1] = '\0'; // Null-terminate the string
					Alcd_PutAt(&lcd, 1, input_index, "3");
					input_index++;
					Alcd_CursorAt(&lcd, 1, input_index); // Move cursor to next position
				}

				//turn on the blinking again
				Alcd_Display_Control(&lcd, 1, input_index, 1);

				// Update keypad_delay after handling the key press
				general_delay = HAL_GetTick() + 250;

			}

			//4 is entered
			else if (Keypad_Get_Key(&kp, kp_button_4)
					&& (current_tick >= general_delay)) {

				//turn off the blinking
				Alcd_Display_Control(&lcd, 1, 0, 0);

				// store '0' in the entered time date buffer
				if (input_index < 2) {
					time_date_buffer[input_index] = '4';
					time_date_buffer[input_index + 1] = '\0'; // Null-terminate the string
					Alcd_PutAt(&lcd, 1, input_index, "4");
					input_index++;
					Alcd_CursorAt(&lcd, 1, input_index); // Move cursor to next position
				}

				//turn on the blinking again
				Alcd_Display_Control(&lcd, 1, input_index, 1);

				// Update keypad_delay after handling the key press
				general_delay = HAL_GetTick() + 250;

			}
			//5 is entered
			else if (Keypad_Get_Key(&kp, kp_button_5)
					&& (current_tick >= general_delay)) {

				//turn off the blinking
				Alcd_Display_Control(&lcd, 1, 0, 0);

				// store '0' in the entered time date buffer
				if (input_index < 2) {
					time_date_buffer[input_index] = '5';
					time_date_buffer[input_index + 1] = '\0'; // Null-terminate the string
					Alcd_PutAt(&lcd, 1, input_index, "5");
					input_index++;
					Alcd_CursorAt(&lcd, 1, input_index); // Move cursor to next position
				}

				//turn on the blinking again
				Alcd_Display_Control(&lcd, 1, input_index, 1);

				// Update keypad_delay after handling the key press
				general_delay = HAL_GetTick() + 250;

			}

			//6 is entered
			else if (Keypad_Get_Key(&kp, kp_button_6)
					&& (current_tick >= general_delay)) {

				//turn off the blinking
				Alcd_Display_Control(&lcd, 1, 0, 0);

				// store '0' in the entered time date buffer
				if (input_index < 2) {
					time_date_buffer[input_index] = '6';
					time_date_buffer[input_index + 1] = '\0'; // Null-terminate the string
					Alcd_PutAt(&lcd, 1, input_index, "6");
					input_index++;
					Alcd_CursorAt(&lcd, 1, input_index); // Move cursor to next position
				}

				//turn on the blinking again
				Alcd_Display_Control(&lcd, 1, input_index, 1);

				// Update keypad_delay after handling the key press
				general_delay = HAL_GetTick() + 250;

			}

			//7 is entered
			else if (Keypad_Get_Key(&kp, kp_button_7)
					&& (current_tick >= general_delay)) {

				//turn off the blinking
				Alcd_Display_Control(&lcd, 1, 0, 0);

				// store '0' in the entered time date buffer
				if (input_index < 2) {
					time_date_buffer[input_index] = '7';
					time_date_buffer[input_index + 1] = '\0'; // Null-terminate the string
					Alcd_PutAt(&lcd, 1, input_index, "7");
					input_index++;
					Alcd_CursorAt(&lcd, 1, input_index); // Move cursor to next position
				}

				//turn on the blinking again
				Alcd_Display_Control(&lcd, 1, input_index, 1);

				// Update keypad_delay after handling the key press
				general_delay = HAL_GetTick() + 250;

			}

			//8 is entered
			else if (Keypad_Get_Key(&kp, kp_button_8)
					&& (current_tick >= general_delay)) {

				//turn off the blinking
				Alcd_Display_Control(&lcd, 1, 0, 0);

				// store '0' in the entered time date buffer
				if (input_index < 2) {
					time_date_buffer[input_index] = '8';
					time_date_buffer[input_index + 1] = '\0'; // Null-terminate the string
					Alcd_PutAt(&lcd, 1, input_index, "8");
					input_index++;
					Alcd_CursorAt(&lcd, 1, input_index); // Move cursor to next position
				}

				//turn on the blinking again
				Alcd_Display_Control(&lcd, 1, input_index, 1);

				// Update keypad_delay after handling the key press
				general_delay = HAL_GetTick() + 250;

			}

			//9 is entered
			else if (Keypad_Get_Key(&kp, kp_button_9)
					&& (current_tick >= general_delay)) {

				//turn off the blinking
				Alcd_Display_Control(&lcd, 1, 0, 0);

				// store '0' in the entered time date buffer
				if (input_index < 2) {
					time_date_buffer[input_index] = '9';
					time_date_buffer[input_index + 1] = '\0'; // Null-terminate the string
					Alcd_PutAt(&lcd, 1, input_index, "9");
					input_index++;
					Alcd_CursorAt(&lcd, 1, input_index); // Move cursor to next position
				}

				//turn on the blinking again
				Alcd_Display_Control(&lcd, 1, input_index, 1);

				// Update keypad_delay after handling the key press
				general_delay = HAL_GetTick() + 250;

			}

			//when yes is entered -> check for day validation (state 36)
			if (Keypad_Get_Key(&kp, kp_button_yes)
					&& (current_tick >= general_delay)) {

				Alcd_Clear(&lcd);

				snprintf(timeString, sizeof(timeString), "%02d", buffer);
				Alcd_PutAt_n(&lcd, 1, 0, timeString, strlen(timeString));

				status = 36;

			}

			//
			general_delay = HAL_GetTick() + 250;
		}

//date validation phase
		while ((status == 36) && (current_tick >= general_delay)) {

			//get the current tick number
			current_tick = HAL_GetTick();

			Alcd_Clear(&lcd);

			snprintf(timeString, sizeof(timeString), "%02d", status);
			Alcd_PutAt_n(&lcd, 0, 14, timeString, strlen(timeString));

			buffer = atoi(time_date_buffer);

			//check if day is within the range 1 to 31
			if (buffer > 0 && buffer < 32) {

				//move to the month state
				status = 37;

				//set the day value
				CLK.date = buffer;

				input_index = 0;
				//turn off the blinking
				Alcd_Display_Control(&lcd, 1, 0, 0);
				//turn on the blinking again
				Alcd_Display_Control(&lcd, 1, input_index, 1);
			} else {
				Alcd_PutAt(&lcd, 0, 0, "invalid");
				delay_flag = 1;

				input_index = 0;
				//turn off the blinking
				Alcd_Display_Control(&lcd, 1, 0, 0);
				//turn on the blinking again
				Alcd_Display_Control(&lcd, 1, input_index, 1);
				//return to entering day
				status = 26;

				general_delay = HAL_GetTick() + 1000;
			}

			//create a delay
			if ((delay_flag == 1) && (current_tick >= general_delay)) {

				delay_flag = 0;
			}

		}

		//month edit menu -> enter month(state 37)
		while ((status == 37) && (current_tick >= general_delay)) {

			//get the current tick number
			current_tick = HAL_GetTick();

			//Alcd_Clear(&lcd);
			Alcd_PutAt(&lcd, 0, 0, "Enter month");

			snprintf(timeString, sizeof(timeString), "%02d", status);
			Alcd_PutAt_n(&lcd, 0, 14, timeString, strlen(timeString));

			Alcd_CursorAt(&lcd, 1, input_index);
			Alcd_Display_Control(&lcd, 1, 1, 1);

			//check if any button is pressed
			//check for any keypad input
			Keypad_Refresh(&kp);

			//if back is entered
			if (Keypad_Get_Key(&kp, kp_button_no_back)
					&& (current_tick >= general_delay)) {

				input_index = 0;

				//back to previous menu
				status = 26;

			}

			//0 is entered
			else if (Keypad_Get_Key(&kp, kp_button_0)
					&& (current_tick >= general_delay)) {

				//turn off the blinking
				Alcd_Display_Control(&lcd, 1, 0, 0);

				// store '0' in the entered time date buffer
				if (input_index < 2) {
					time_date_buffer[input_index] = '0';
					time_date_buffer[input_index + 1] = '\0'; // Null-terminate the string
					Alcd_PutAt(&lcd, 1, input_index, "0");
					input_index++;
					Alcd_CursorAt(&lcd, 1, input_index); // Move cursor to next position
				}

				//turn on the blinking again
				Alcd_Display_Control(&lcd, 1, input_index, 1);

				// Update keypad_delay after handling the key press
				general_delay = HAL_GetTick() + 250;

			}
			//1 is entered
			else if (Keypad_Get_Key(&kp, kp_button_1)
					&& (current_tick >= general_delay)) {

				//turn off the blinking
				Alcd_Display_Control(&lcd, 1, 0, 0);

				// store '0' in the entered time date buffer
				if (input_index < 2) {
					time_date_buffer[input_index] = '1';
					time_date_buffer[input_index + 1] = '\0'; // Null-terminate the string
					Alcd_PutAt(&lcd, 1, input_index, "1");
					input_index++;
					Alcd_CursorAt(&lcd, 1, input_index); // Move cursor to next position
				}

				//turn on the blinking again
				Alcd_Display_Control(&lcd, 1, input_index, 1);

				// Update keypad_delay after handling the key press
				general_delay = HAL_GetTick() + 250;

			}
			//2 is entered
			else if (Keypad_Get_Key(&kp, kp_button_2)
					&& (current_tick >= general_delay)) {

				//turn off the blinking
				Alcd_Display_Control(&lcd, 1, 0, 0);

				// store '0' in the entered time date buffer
				if (input_index < 2) {
					time_date_buffer[input_index] = '2';
					time_date_buffer[input_index + 1] = '\0'; // Null-terminate the string
					Alcd_PutAt(&lcd, 1, input_index, "2");
					input_index++;
					Alcd_CursorAt(&lcd, 1, input_index); // Move cursor to next position
				}

				//turn on the blinking again
				Alcd_Display_Control(&lcd, 1, input_index, 1);

				// Update keypad_delay after handling the key press
				general_delay = HAL_GetTick() + 250;

			}
			//3 is entered
			else if (Keypad_Get_Key(&kp, kp_button_3)
					&& (current_tick >= general_delay)) {

				//turn off the blinking
				Alcd_Display_Control(&lcd, 1, 0, 0);

				// store '0' in the entered time date buffer
				if (input_index < 2) {
					time_date_buffer[input_index] = '3';
					time_date_buffer[input_index + 1] = '\0'; // Null-terminate the string
					Alcd_PutAt(&lcd, 1, input_index, "3");
					input_index++;
					Alcd_CursorAt(&lcd, 1, input_index); // Move cursor to next position
				}

				//turn on the blinking again
				Alcd_Display_Control(&lcd, 1, input_index, 1);

				// Update keypad_delay after handling the key press
				general_delay = HAL_GetTick() + 250;

			}

			//4 is entered
			else if (Keypad_Get_Key(&kp, kp_button_4)
					&& (current_tick >= general_delay)) {

				//turn off the blinking
				Alcd_Display_Control(&lcd, 1, 0, 0);

				// store '0' in the entered time date buffer
				if (input_index < 2) {
					time_date_buffer[input_index] = '4';
					time_date_buffer[input_index + 1] = '\0'; // Null-terminate the string
					Alcd_PutAt(&lcd, 1, input_index, "4");
					input_index++;
					Alcd_CursorAt(&lcd, 1, input_index); // Move cursor to next position
				}

				//turn on the blinking again
				Alcd_Display_Control(&lcd, 1, input_index, 1);

				// Update keypad_delay after handling the key press
				general_delay = HAL_GetTick() + 250;

			}
			//5 is entered
			else if (Keypad_Get_Key(&kp, kp_button_5)
					&& (current_tick >= general_delay)) {

				//turn off the blinking
				Alcd_Display_Control(&lcd, 1, 0, 0);

				// store '0' in the entered time date buffer
				if (input_index < 2) {
					time_date_buffer[input_index] = '5';
					time_date_buffer[input_index + 1] = '\0'; // Null-terminate the string
					Alcd_PutAt(&lcd, 1, input_index, "5");
					input_index++;
					Alcd_CursorAt(&lcd, 1, input_index); // Move cursor to next position
				}

				//turn on the blinking again
				Alcd_Display_Control(&lcd, 1, input_index, 1);

				// Update keypad_delay after handling the key press
				general_delay = HAL_GetTick() + 250;

			}

			//6 is entered
			else if (Keypad_Get_Key(&kp, kp_button_6)
					&& (current_tick >= general_delay)) {

				//turn off the blinking
				Alcd_Display_Control(&lcd, 1, 0, 0);

				// store '0' in the entered time date buffer
				if (input_index < 2) {
					time_date_buffer[input_index] = '6';
					time_date_buffer[input_index + 1] = '\0'; // Null-terminate the string
					Alcd_PutAt(&lcd, 1, input_index, "6");
					input_index++;
					Alcd_CursorAt(&lcd, 1, input_index); // Move cursor to next position
				}

				//turn on the blinking again
				Alcd_Display_Control(&lcd, 1, input_index, 1);

				// Update keypad_delay after handling the key press
				general_delay = HAL_GetTick() + 250;

			}

			//7 is entered
			else if (Keypad_Get_Key(&kp, kp_button_7)
					&& (current_tick >= general_delay)) {

				//turn off the blinking
				Alcd_Display_Control(&lcd, 1, 0, 0);

				// store '0' in the entered time date buffer
				if (input_index < 2) {
					time_date_buffer[input_index] = '7';
					time_date_buffer[input_index + 1] = '\0'; // Null-terminate the string
					Alcd_PutAt(&lcd, 1, input_index, "7");
					input_index++;
					Alcd_CursorAt(&lcd, 1, input_index); // Move cursor to next position
				}

				//turn on the blinking again
				Alcd_Display_Control(&lcd, 1, input_index, 1);

				// Update keypad_delay after handling the key press
				general_delay = HAL_GetTick() + 250;

			}

			//8 is entered
			else if (Keypad_Get_Key(&kp, kp_button_8)
					&& (current_tick >= general_delay)) {

				//turn off the blinking
				Alcd_Display_Control(&lcd, 1, 0, 0);

				// store '0' in the entered time date buffer
				if (input_index < 2) {
					time_date_buffer[input_index] = '8';
					time_date_buffer[input_index + 1] = '\0'; // Null-terminate the string
					Alcd_PutAt(&lcd, 1, input_index, "8");
					input_index++;
					Alcd_CursorAt(&lcd, 1, input_index); // Move cursor to next position
				}

				//turn on the blinking again
				Alcd_Display_Control(&lcd, 1, input_index, 1);

				// Update keypad_delay after handling the key press
				general_delay = HAL_GetTick() + 250;

			}

			//9 is entered
			else if (Keypad_Get_Key(&kp, kp_button_9)
					&& (current_tick >= general_delay)) {

				//turn off the blinking
				Alcd_Display_Control(&lcd, 1, 0, 0);

				// store '0' in the entered time date buffer
				if (input_index < 2) {
					time_date_buffer[input_index] = '9';
					time_date_buffer[input_index + 1] = '\0'; // Null-terminate the string
					Alcd_PutAt(&lcd, 1, input_index, "9");
					input_index++;
					Alcd_CursorAt(&lcd, 1, input_index); // Move cursor to next position
				}

				//turn on the blinking again
				Alcd_Display_Control(&lcd, 1, input_index, 1);

				// Update keypad_delay after handling the key press
				general_delay = HAL_GetTick() + 250;

			}

			//when yes is entered -> check for month validation (state 38)
			if (Keypad_Get_Key(&kp, kp_button_yes)
					&& (current_tick >= general_delay)) {

				Alcd_Clear(&lcd);

				snprintf(timeString, sizeof(timeString), "%02d", buffer);
				Alcd_PutAt_n(&lcd, 1, 0, timeString, strlen(timeString));

				status = 38;

			}

			//
			general_delay = HAL_GetTick() + 250;
		}

//month validation phase
		while ((status == 38) && (current_tick >= general_delay)) {

			//get the current tick number
			current_tick = HAL_GetTick();

			Alcd_Clear(&lcd);

			snprintf(timeString, sizeof(timeString), "%02d", status);
			Alcd_PutAt_n(&lcd, 0, 14, timeString, strlen(timeString));

			buffer = atoi(time_date_buffer);

			//check if month is within the range 1 to 12
			if (buffer > 0 && buffer < 13) {

				//move to the month state
				status = 39;

				//set the month value
				CLK.month = buffer;

				input_index = 0;
				//turn off the blinking
				Alcd_Display_Control(&lcd, 1, 0, 0);
				//turn on the blinking again
				Alcd_Display_Control(&lcd, 1, input_index, 1);
			} else {
				Alcd_PutAt(&lcd, 0, 0, "invalid");
				delay_flag = 1;

				input_index = 0;
				//turn off the blinking
				Alcd_Display_Control(&lcd, 1, 0, 0);
				//turn on the blinking again
				Alcd_Display_Control(&lcd, 1, input_index, 1);
				//return to entering month
				status = 37;

				general_delay = HAL_GetTick() + 1000;
			}

			//create a delay
			if ((delay_flag == 1) && (current_tick >= general_delay)) {

				delay_flag = 0;
			}

		}

		//year edit menu -> enter month(state 39)
		while ((status == 39) && (current_tick >= general_delay)) {

			//get the current tick number
			current_tick = HAL_GetTick();

			//Alcd_Clear(&lcd);
			Alcd_PutAt(&lcd, 0, 0, "Enter year");

			snprintf(timeString, sizeof(timeString), "%02d", status);
			Alcd_PutAt_n(&lcd, 0, 14, timeString, strlen(timeString));

			Alcd_CursorAt(&lcd, 1, input_index);
			Alcd_Display_Control(&lcd, 1, 1, 1);

			//check if any button is pressed
			//check for any keypad input
			Keypad_Refresh(&kp);

			//if back is entered
			if (Keypad_Get_Key(&kp, kp_button_no_back)
					&& (current_tick >= general_delay)) {

				input_index = 0;

				//back to previous menu
				status = 37;

			}

			//0 is entered
			else if (Keypad_Get_Key(&kp, kp_button_0)
					&& (current_tick >= general_delay)) {

				//turn off the blinking
				Alcd_Display_Control(&lcd, 1, 0, 0);

				// store '0' in the entered time date buffer
				if (input_index < 4) {
					time_date_buffer[input_index] = '0';
					time_date_buffer[input_index + 1] = '\0'; // Null-terminate the string
					Alcd_PutAt(&lcd, 1, input_index, "0");
					input_index++;
					Alcd_CursorAt(&lcd, 1, input_index); // Move cursor to next position
				}

				//turn on the blinking again
				Alcd_Display_Control(&lcd, 1, input_index, 1);

				// Update keypad_delay after handling the key press
				general_delay = HAL_GetTick() + 250;

			}
			//1 is entered
			else if (Keypad_Get_Key(&kp, kp_button_1)
					&& (current_tick >= general_delay)) {

				//turn off the blinking
				Alcd_Display_Control(&lcd, 1, 0, 0);

				// store '0' in the entered time date buffer
				if (input_index < 4) {
					time_date_buffer[input_index] = '1';
					time_date_buffer[input_index + 1] = '\0'; // Null-terminate the string
					Alcd_PutAt(&lcd, 1, input_index, "1");
					input_index++;
					Alcd_CursorAt(&lcd, 1, input_index); // Move cursor to next position
				}

				//turn on the blinking again
				Alcd_Display_Control(&lcd, 1, input_index, 1);

				// Update keypad_delay after handling the key press
				general_delay = HAL_GetTick() + 250;

			}
			//2 is entered
			else if (Keypad_Get_Key(&kp, kp_button_2)
					&& (current_tick >= general_delay)) {

				//turn off the blinking
				Alcd_Display_Control(&lcd, 1, 0, 0);

				// store '0' in the entered time date buffer
				if (input_index < 4) {
					time_date_buffer[input_index] = '2';
					time_date_buffer[input_index + 1] = '\0'; // Null-terminate the string
					Alcd_PutAt(&lcd, 1, input_index, "2");
					input_index++;
					Alcd_CursorAt(&lcd, 1, input_index); // Move cursor to next position
				}

				//turn on the blinking again
				Alcd_Display_Control(&lcd, 1, input_index, 1);

				// Update keypad_delay after handling the key press
				general_delay = HAL_GetTick() + 250;

			}
			//3 is entered
			else if (Keypad_Get_Key(&kp, kp_button_3)
					&& (current_tick >= general_delay)) {

				//turn off the blinking
				Alcd_Display_Control(&lcd, 1, 0, 0);

				// store '0' in the entered time date buffer
				if (input_index < 4) {
					time_date_buffer[input_index] = '3';
					time_date_buffer[input_index + 1] = '\0'; // Null-terminate the string
					Alcd_PutAt(&lcd, 1, input_index, "3");
					input_index++;
					Alcd_CursorAt(&lcd, 1, input_index); // Move cursor to next position
				}

				//turn on the blinking again
				Alcd_Display_Control(&lcd, 1, input_index, 1);

				// Update keypad_delay after handling the key press
				general_delay = HAL_GetTick() + 250;

			}

			//4 is entered
			else if (Keypad_Get_Key(&kp, kp_button_4)
					&& (current_tick >= general_delay)) {

				//turn off the blinking
				Alcd_Display_Control(&lcd, 1, 0, 0);

				// store '0' in the entered time date buffer
				if (input_index < 4) {
					time_date_buffer[input_index] = '4';
					time_date_buffer[input_index + 1] = '\0'; // Null-terminate the string
					Alcd_PutAt(&lcd, 1, input_index, "4");
					input_index++;
					Alcd_CursorAt(&lcd, 1, input_index); // Move cursor to next position
				}

				//turn on the blinking again
				Alcd_Display_Control(&lcd, 1, input_index, 1);

				// Update keypad_delay after handling the key press
				general_delay = HAL_GetTick() + 250;

			}
			//5 is entered
			else if (Keypad_Get_Key(&kp, kp_button_5)
					&& (current_tick >= general_delay)) {

				//turn off the blinking
				Alcd_Display_Control(&lcd, 1, 0, 0);

				// store '0' in the entered time date buffer
				if (input_index < 4) {
					time_date_buffer[input_index] = '5';
					time_date_buffer[input_index + 1] = '\0'; // Null-terminate the string
					Alcd_PutAt(&lcd, 1, input_index, "5");
					input_index++;
					Alcd_CursorAt(&lcd, 1, input_index); // Move cursor to next position
				}

				//turn on the blinking again
				Alcd_Display_Control(&lcd, 1, input_index, 1);

				// Update keypad_delay after handling the key press
				general_delay = HAL_GetTick() + 250;

			}

			//6 is entered
			else if (Keypad_Get_Key(&kp, kp_button_6)
					&& (current_tick >= general_delay)) {

				//turn off the blinking
				Alcd_Display_Control(&lcd, 1, 0, 0);

				// store '0' in the entered time date buffer
				if (input_index < 4) {
					time_date_buffer[input_index] = '6';
					time_date_buffer[input_index + 1] = '\0'; // Null-terminate the string
					Alcd_PutAt(&lcd, 1, input_index, "6");
					input_index++;
					Alcd_CursorAt(&lcd, 1, input_index); // Move cursor to next position
				}

				//turn on the blinking again
				Alcd_Display_Control(&lcd, 1, input_index, 1);

				// Update keypad_delay after handling the key press
				general_delay = HAL_GetTick() + 250;

			}

			//7 is entered
			else if (Keypad_Get_Key(&kp, kp_button_7)
					&& (current_tick >= general_delay)) {

				//turn off the blinking
				Alcd_Display_Control(&lcd, 1, 0, 0);

				// store '0' in the entered time date buffer
				if (input_index < 4) {
					time_date_buffer[input_index] = '7';
					time_date_buffer[input_index + 1] = '\0'; // Null-terminate the string
					Alcd_PutAt(&lcd, 1, input_index, "7");
					input_index++;
					Alcd_CursorAt(&lcd, 1, input_index); // Move cursor to next position
				}

				//turn on the blinking again
				Alcd_Display_Control(&lcd, 1, input_index, 1);

				// Update keypad_delay after handling the key press
				general_delay = HAL_GetTick() + 250;

			}

			//8 is entered
			else if (Keypad_Get_Key(&kp, kp_button_8)
					&& (current_tick >= general_delay)) {

				//turn off the blinking
				Alcd_Display_Control(&lcd, 1, 0, 0);

				// store '0' in the entered time date buffer
				if (input_index < 4) {
					time_date_buffer[input_index] = '8';
					time_date_buffer[input_index + 1] = '\0'; // Null-terminate the string
					Alcd_PutAt(&lcd, 1, input_index, "8");
					input_index++;
					Alcd_CursorAt(&lcd, 1, input_index); // Move cursor to next position
				}

				//turn on the blinking again
				Alcd_Display_Control(&lcd, 1, input_index, 1);

				// Update keypad_delay after handling the key press
				general_delay = HAL_GetTick() + 250;

			}

			//9 is entered
			else if (Keypad_Get_Key(&kp, kp_button_9)
					&& (current_tick >= general_delay)) {

				//turn off the blinking
				Alcd_Display_Control(&lcd, 1, 0, 0);

				// store '0' in the entered time date buffer
				if (input_index < 4) {
					time_date_buffer[input_index] = '9';
					time_date_buffer[input_index + 1] = '\0'; // Null-terminate the string
					Alcd_PutAt(&lcd, 1, input_index, "9");
					input_index++;
					Alcd_CursorAt(&lcd, 1, input_index); // Move cursor to next position
				}

				//turn on the blinking again
				Alcd_Display_Control(&lcd, 1, input_index, 1);

				// Update keypad_delay after handling the key press
				general_delay = HAL_GetTick() + 250;

			}

			//when yes is entered -> check for year validation (state 40)
			if (Keypad_Get_Key(&kp, kp_button_yes)
					&& (current_tick >= general_delay)) {

				Alcd_Clear(&lcd);

				snprintf(timeString, sizeof(timeString), "%02d", buffer);
				Alcd_PutAt_n(&lcd, 1, 0, timeString, strlen(timeString));

				status = 40;

			}

			//
			general_delay = HAL_GetTick() + 250;
		}

//year validation phase
		while ((status == 40) && (current_tick >= general_delay)) {

			//get the current tick number
			current_tick = HAL_GetTick();

			Alcd_Clear(&lcd);

			snprintf(timeString, sizeof(timeString), "%02d", status);
			Alcd_PutAt_n(&lcd, 0, 14, timeString, strlen(timeString));

			buffer = atoi(time_date_buffer);

			//check if year is within the range 2024 to 2099
			if (buffer > 2023 && buffer < 2100) {

				//move to confirm save state
				status = 41;

				//set the year value
				CLK.year = buffer;

				input_index = 0;
				//turn off the blinking
				Alcd_Display_Control(&lcd, 1, 0, 0);
				//turn on the blinking again
				Alcd_Display_Control(&lcd, 1, input_index, 1);
			} else {
				Alcd_PutAt(&lcd, 0, 0, "invalid");
				delay_flag = 1;

				input_index = 0;
				//turn off the blinking
				Alcd_Display_Control(&lcd, 1, 0, 0);
				//turn on the blinking again
				Alcd_Display_Control(&lcd, 1, input_index, 1);
				//return to entering year
				status = 39;

				general_delay = HAL_GetTick() + 1000;
			}

			//create a delay
			if ((delay_flag == 1) && (current_tick >= general_delay)) {

				delay_flag = 0;
			}

		}

//date confirmation menu (state 41)
		while ((status == 41) && (current_tick >= general_delay)) {

			//get the current tick number
			current_tick = HAL_GetTick();

			Alcd_Clear(&lcd);
			Alcd_PutAt(&lcd, 0, 0, "confirm?");

			snprintf(timeString, sizeof(timeString), "%02d", status);
			Alcd_PutAt_n(&lcd, 0, 14, timeString, strlen(timeString));

			//check if any button is pressed
			//check for any keypad input
			Keypad_Refresh(&kp);

			//yes is selected
			if (Keypad_Get_Key(&kp, kp_button_yes)
					&& (current_tick >= general_delay)) {

				Ds1307_set(&CLK);

				//return to edit menu
				status = 21;

			}

			//back is selected -> back to enter year state
			else if (Keypad_Get_Key(&kp, kp_button_no_back)
					&& (current_tick >= general_delay)) {

				status = 39;

			}

			general_delay = HAL_GetTick() + 250;
		}

//status 24 -> calibrate dosing
		while ((status == 24) && (current_tick >= general_delay)) {

			//get the current tick number
			current_tick = HAL_GetTick();

			//moving the motor to the 0 position
			TIM1->CCR1 = 999;

			Alcd_Clear(&lcd);
			Alcd_PutAt(&lcd, 0, 0, "hold feeding");
			Alcd_PutAt(&lcd, 1, 0, "till finishing");
			snprintf(timeString, sizeof(timeString), "%02d", status);
			Alcd_PutAt_n(&lcd, 0, 14, timeString, strlen(timeString));

			//check if any button is pressed
			//check for any keypad input
			Keypad_Refresh(&kp);

			//in case feeding is selected -> start moving the motor
			if (Keypad_Get_Key(&kp, kp_button_force_feed)
					&& (current_tick >= general_delay)) {
				Alcd_Clear(&lcd);
				Alcd_PutAt(&lcd, 0, 0, "calibrating");

				dosing_tick = dosing_tick + HAL_GetTick();

				//moving the motor to the 180 position
				TIM1->CCR1 = 1999;

			}

			//yes is selected
			else if (Keypad_Get_Key(&kp, kp_button_yes)
					&& (current_tick >= general_delay)) {

				status = 42;

			}

			//back is selected
			else if (Keypad_Get_Key(&kp, kp_button_no_back)
					&& (current_tick >= general_delay)) {

				status = 21;

			}

			general_delay = HAL_GetTick() + 250;
		}

		//status 42 -> calibrate saving
		while ((status == 42) && (current_tick >= general_delay)) {

			// Update current_tick to the current time
			current_tick = HAL_GetTick();
			Alcd_Clear(&lcd);
			Alcd_PutAt(&lcd, 0, 0, "saved");

			// Check if the current tick has passed the general delay time
			if (current_tick >= general_delay) {
				Alcd_Clear(&lcd);
				Alcd_PutAt(&lcd, 0, 0, "saved");

				snprintf(timeString, sizeof(timeString), "ticks= %09d",
						dosing_tick);
				Alcd_PutAt_n(&lcd, 1, 0, timeString, strlen(timeString));

				// Writing the ticks to the EEPROM
				eeprom24c32_write(&memory, &dosing_tick, dosing_period);

				// Set general_delay to 1000ms after the current tick
				general_delay = current_tick + 1000;

				// Set the status
				status = 21;
			}
		}

		//edit parameters state 23
		while ((status == 23) && (current_tick >= general_delay)) {

			//get the current tick number
			current_tick = HAL_GetTick();

			Alcd_Clear(&lcd);
			Alcd_PutAt(&lcd, 0, 0, "1: doses no.");
			Alcd_PutAt(&lcd, 1, 0, "2: dosing hour");
			snprintf(timeString, sizeof(timeString), "%02d", status);
			Alcd_PutAt_n(&lcd, 0, 14, timeString, strlen(timeString));

			//check if any button is pressed
			//check for any keypad input
			Keypad_Refresh(&kp);

			//in case 1 is selected -> no. of doses (status 43)
			if (Keypad_Get_Key(&kp, kp_button_1)
					&& (current_tick >= general_delay)) {
				Alcd_Clear(&lcd);

				status = 43;

			}

			//2 is selected -> edit hour
			else if (Keypad_Get_Key(&kp, kp_button_2)
					&& (current_tick >= general_delay)) {

				Alcd_Clear(&lcd);
				status = 44;

			}

			//back is selected
			else if (Keypad_Get_Key(&kp, kp_button_no_back)
					&& (current_tick >= general_delay)) {

				Alcd_Clear(&lcd);
				//save? menu
				status = 48;

			}

			//next is selected
			else if (Keypad_Get_Key(&kp, kp_button_next)
					&& (current_tick >= general_delay)) {

				Alcd_Clear(&lcd);
				//go to extended parameters menu (state 49)
				status = 49;

			}

			general_delay = HAL_GetTick() + 250;
		}

		//enter no. of doses (state 43)
		while ((status == 43) && (current_tick >= general_delay)) {

			//get the current tick number
			current_tick = HAL_GetTick();

			Alcd_PutAt(&lcd, 0, 0, "doses no.?");
			snprintf(timeString, sizeof(timeString), "%02d", status);
			Alcd_PutAt_n(&lcd, 0, 14, timeString, strlen(timeString));

			Alcd_CursorAt(&lcd, 1, input_index);
			Alcd_Display_Control(&lcd, 1, 1, 1);

			Keypad_Refresh(&kp);

			//in case back is selected
			if (Keypad_Get_Key(&kp, kp_button_no_back)
					&& (current_tick >= general_delay)) {

				input_index = 0;  // Reset input index

				//back to previous menu
				status = 23;

			}

			//if 0 is pressed
			else if (Keypad_Get_Key(&kp, kp_button_0)
					&& (current_tick >= general_delay)) {

				//turn off the blinking
				Alcd_Display_Control(&lcd, 1, 0, 0);

				// store '0' in the entered password
				if (input_index < 2) {
					entered_password[input_index] = '0';
					entered_password[input_index + 1] = '\0'; // Null-terminate the string
					Alcd_PutAt(&lcd, 1, input_index, "0");
					input_index++;
					Alcd_CursorAt(&lcd, 1, input_index); // Move cursor to next position
				}

				//turn on the blinking again
				Alcd_Display_Control(&lcd, 1, input_index, 1);

				// Update keypad_delay after handling the key press
				general_delay = HAL_GetTick() + 250;
			}

			//if 1 is pressed
			else if (Keypad_Get_Key(&kp, kp_button_1)
					&& (current_tick >= general_delay)) {

				//turn off the blinking
				Alcd_Display_Control(&lcd, 1, 0, 0);

				// store '0' in the entered password
				if (input_index < 2) {
					entered_password[input_index] = '1';
					entered_password[input_index + 1] = '\0'; // Null-terminate the string
					Alcd_PutAt(&lcd, 1, input_index, "1");
					input_index++;
					Alcd_CursorAt(&lcd, 1, input_index); // Move cursor to next position
				}

				//turn on the blinking again
				Alcd_Display_Control(&lcd, 1, input_index, 1);

				// Update keypad_delay after handling the key press
				general_delay = HAL_GetTick() + 250;
			}

			//if 2 is pressed
			else if (Keypad_Get_Key(&kp, kp_button_2)
					&& (current_tick >= general_delay)) {

				//turn off the blinking
				Alcd_Display_Control(&lcd, 1, 0, 0);

				// store '0' in the entered password
				if (input_index < 2) {
					entered_password[input_index] = '2';
					entered_password[input_index + 1] = '\0'; // Null-terminate the string
					Alcd_PutAt(&lcd, 1, input_index, "2");
					input_index++;
					Alcd_CursorAt(&lcd, 1, input_index); // Move cursor to next position
				}

				//turn on the blinking again
				Alcd_Display_Control(&lcd, 1, input_index, 1);

				// Update keypad_delay after handling the key press
				general_delay = HAL_GetTick() + 250;
			}

			//if 3 is pressed
			else if (Keypad_Get_Key(&kp, kp_button_3)
					&& (current_tick >= general_delay)) {

				//turn off the blinking
				Alcd_Display_Control(&lcd, 1, 0, 0);

				// store '0' in the entered password
				if (input_index < 2) {
					entered_password[input_index] = '3';
					entered_password[input_index + 1] = '\0'; // Null-terminate the string
					Alcd_PutAt(&lcd, 1, input_index, "3");
					input_index++;
					Alcd_CursorAt(&lcd, 1, input_index); // Move cursor to next position
				}

				//turn on the blinking again
				Alcd_Display_Control(&lcd, 1, input_index, 1);

				// Update keypad_delay after handling the key press
				general_delay = HAL_GetTick() + 250;
			}
			//if 4 is pressed
			else if (Keypad_Get_Key(&kp, kp_button_4)
					&& (current_tick >= general_delay)) {

				//turn off the blinking
				Alcd_Display_Control(&lcd, 1, 0, 0);

				// store '0' in the entered password
				if (input_index < 2) {
					entered_password[input_index] = '4';
					entered_password[input_index + 1] = '\0'; // Null-terminate the string
					Alcd_PutAt(&lcd, 1, input_index, "4");
					input_index++;
					Alcd_CursorAt(&lcd, 1, input_index); // Move cursor to next position
				}

				//turn on the blinking again
				Alcd_Display_Control(&lcd, 1, input_index, 1);

				// Update keypad_delay after handling the key press
				general_delay = HAL_GetTick() + 250;
			}

			//if 5 is pressed
			else if (Keypad_Get_Key(&kp, kp_button_5)
					&& (current_tick >= general_delay)) {

				//turn off the blinking
				Alcd_Display_Control(&lcd, 1, 0, 0);

				// store '0' in the entered password
				if (input_index < 2) {
					entered_password[input_index] = '5';
					entered_password[input_index + 1] = '\0'; // Null-terminate the string
					Alcd_PutAt(&lcd, 1, input_index, "5");
					input_index++;
					Alcd_CursorAt(&lcd, 1, input_index); // Move cursor to next position
				}

				//turn on the blinking again
				Alcd_Display_Control(&lcd, 1, input_index, 1);

				// Update keypad_delay after handling the key press
				general_delay = HAL_GetTick() + 250;
			}

			//if 6 is pressed
			else if (Keypad_Get_Key(&kp, kp_button_6)
					&& (current_tick >= general_delay)) {

				//turn off the blinking
				Alcd_Display_Control(&lcd, 1, 0, 0);

				// store '0' in the entered password
				if (input_index < 2) {
					entered_password[input_index] = '6';
					entered_password[input_index + 1] = '\0'; // Null-terminate the string
					Alcd_PutAt(&lcd, 1, input_index, "6");
					input_index++;
					Alcd_CursorAt(&lcd, 1, input_index); // Move cursor to next position
				}

				//turn on the blinking again
				Alcd_Display_Control(&lcd, 1, input_index, 1);

				// Update keypad_delay after handling the key press
				general_delay = HAL_GetTick() + 250;
			}
			//if 7 is pressed
			else if (Keypad_Get_Key(&kp, kp_button_7)
					&& (current_tick >= general_delay)) {

				//turn off the blinking
				Alcd_Display_Control(&lcd, 1, 0, 0);

				// store '0' in the entered password
				if (input_index < 2) {
					entered_password[input_index] = '7';
					entered_password[input_index + 1] = '\0'; // Null-terminate the string
					Alcd_PutAt(&lcd, 1, input_index, "7");
					input_index++;
					Alcd_CursorAt(&lcd, 1, input_index); // Move cursor to next position
				}

				//turn on the blinking again
				Alcd_Display_Control(&lcd, 1, input_index, 1);

				// Update keypad_delay after handling the key press
				general_delay = HAL_GetTick() + 250;
			}
			//if 8 is pressed
			else if (Keypad_Get_Key(&kp, kp_button_8)
					&& (current_tick >= general_delay)) {

				//turn off the blinking
				Alcd_Display_Control(&lcd, 1, 0, 0);

				// store '0' in the entered password
				if (input_index < 2) {
					entered_password[input_index] = '8';
					entered_password[input_index + 1] = '\0'; // Null-terminate the string
					Alcd_PutAt(&lcd, 1, input_index, "8");
					input_index++;
					Alcd_CursorAt(&lcd, 1, input_index); // Move cursor to next position
				}

				//turn on the blinking again
				Alcd_Display_Control(&lcd, 1, input_index, 1);

				// Update keypad_delay after handling the key press
				general_delay = HAL_GetTick() + 250;
			}
			//if 9 is pressed
			else if (Keypad_Get_Key(&kp, kp_button_9)
					&& (current_tick >= general_delay)) {

				//turn off the blinking
				Alcd_Display_Control(&lcd, 1, 0, 0);

				// store '0' in the entered password
				if (input_index < 2) {
					entered_password[input_index] = '9';
					entered_password[input_index + 1] = '\0'; // Null-terminate the string
					Alcd_PutAt(&lcd, 1, input_index, "9");
					input_index++;
					Alcd_CursorAt(&lcd, 1, input_index); // Move cursor to next position
				}

				//turn on the blinking again
				Alcd_Display_Control(&lcd, 1, input_index, 1);

				// Update keypad_delay after handling the key press
				general_delay = HAL_GetTick() + 250;
			}

			//if yes is pressed
			else if (Keypad_Get_Key(&kp, kp_button_yes)
					&& (current_tick >= general_delay)) {

				//convert the string into integer
				buffer = atoi(entered_password);

				//write the value to eeprom
				eeprom24c32_write(&memory, &buffer, doses_number);

				input_index = 0;

				//return to the previous menu
				status = 23;
			}
		}

		//time entry state 44
		while ((status == 44) && (current_tick >= general_delay)) {

			//get the current tick number
			current_tick = HAL_GetTick();

			Alcd_Clear(&lcd);
			snprintf(timeString, sizeof(timeString), "%02d", status);
			Alcd_PutAt_n(&lcd, 0, 14, timeString, strlen(timeString));

			//check if the time format is 12h or 24h
			//in case 12h
			if (CLK.format == 1) {

				Alcd_PutAt(&lcd, 0, 0, "select time");
				Alcd_PutAt(&lcd, 1, 0, "1: AM   2: PM");

				//check for any keypad input
				Keypad_Refresh(&kp);

				//in case 1 is selected -> AM
				if (Keypad_Get_Key(&kp, kp_button_1)
						&& (current_tick >= general_delay)) {
					CLK.AM_PM = 0;

					Alcd_Clear(&lcd);

					status = 46;

				}

				//2 is selected -> PM
				else if (Keypad_Get_Key(&kp, kp_button_2)
						&& (current_tick >= general_delay)) {

					CLK.AM_PM = 1;
					Alcd_Clear(&lcd);
					status = 46;

				}

				//back is selected
				else if (Keypad_Get_Key(&kp, kp_button_no_back)
						&& (current_tick >= general_delay)) {

					Alcd_Clear(&lcd);
					//go to previous menu
					status = 23;

				}

			}

			else if (CLK.format == 0) {

				status = 46;

			}

			general_delay = HAL_GetTick() + 250;
		}

		//entering the hours state (46)
		while ((status == 46) && (current_tick >= general_delay)) {

			//get the current tick number
			current_tick = HAL_GetTick();

			//Alcd_Clear(&lcd);
			Alcd_PutAt(&lcd, 0, 0, "Enter hours");

			snprintf(timeString, sizeof(timeString), "%02d", status);
			Alcd_PutAt_n(&lcd, 0, 14, timeString, strlen(timeString));

			Alcd_CursorAt(&lcd, 1, input_index);
			Alcd_Display_Control(&lcd, 1, 1, 1);

			//check if any button is pressed
			//check for any keypad input
			Keypad_Refresh(&kp);

			//if back is entered
			if (Keypad_Get_Key(&kp, kp_button_no_back)
					&& (current_tick >= general_delay)) {

				//back to previous menu
				status = 23;

			}

			//0 is entered
			else if (Keypad_Get_Key(&kp, kp_button_0)
					&& (current_tick >= general_delay)) {

				//turn off the blinking
				Alcd_Display_Control(&lcd, 1, 0, 0);

				// store '0' in the entered time date buffer
				if (input_index < 2) {
					time_date_buffer[input_index] = '0';
					time_date_buffer[input_index + 1] = '\0'; // Null-terminate the string
					Alcd_PutAt(&lcd, 1, input_index, "0");
					input_index++;
					Alcd_CursorAt(&lcd, 1, input_index); // Move cursor to next position
				}

				//turn on the blinking again
				Alcd_Display_Control(&lcd, 1, input_index, 1);

				// Update keypad_delay after handling the key press
				general_delay = HAL_GetTick() + 250;

			}
			//1 is entered
			else if (Keypad_Get_Key(&kp, kp_button_1)
					&& (current_tick >= general_delay)) {

				//turn off the blinking
				Alcd_Display_Control(&lcd, 1, 0, 0);

				// store '0' in the entered time date buffer
				if (input_index < 2) {
					time_date_buffer[input_index] = '1';
					time_date_buffer[input_index + 1] = '\0'; // Null-terminate the string
					Alcd_PutAt(&lcd, 1, input_index, "1");
					input_index++;
					Alcd_CursorAt(&lcd, 1, input_index); // Move cursor to next position
				}

				//turn on the blinking again
				Alcd_Display_Control(&lcd, 1, input_index, 1);

				// Update keypad_delay after handling the key press
				general_delay = HAL_GetTick() + 250;

			}
			//2 is entered
			else if (Keypad_Get_Key(&kp, kp_button_2)
					&& (current_tick >= general_delay)) {

				//turn off the blinking
				Alcd_Display_Control(&lcd, 1, 0, 0);

				// store '0' in the entered time date buffer
				if (input_index < 2) {
					time_date_buffer[input_index] = '2';
					time_date_buffer[input_index + 1] = '\0'; // Null-terminate the string
					Alcd_PutAt(&lcd, 1, input_index, "2");
					input_index++;
					Alcd_CursorAt(&lcd, 1, input_index); // Move cursor to next position
				}

				//turn on the blinking again
				Alcd_Display_Control(&lcd, 1, input_index, 1);

				// Update keypad_delay after handling the key press
				general_delay = HAL_GetTick() + 250;

			}
			//3 is entered
			else if (Keypad_Get_Key(&kp, kp_button_3)
					&& (current_tick >= general_delay)) {

				//turn off the blinking
				Alcd_Display_Control(&lcd, 1, 0, 0);

				// store '0' in the entered time date buffer
				if (input_index < 2) {
					time_date_buffer[input_index] = '3';
					time_date_buffer[input_index + 1] = '\0'; // Null-terminate the string
					Alcd_PutAt(&lcd, 1, input_index, "3");
					input_index++;
					Alcd_CursorAt(&lcd, 1, input_index); // Move cursor to next position
				}

				//turn on the blinking again
				Alcd_Display_Control(&lcd, 1, input_index, 1);

				// Update keypad_delay after handling the key press
				general_delay = HAL_GetTick() + 250;

			}

			//4 is entered
			else if (Keypad_Get_Key(&kp, kp_button_4)
					&& (current_tick >= general_delay)) {

				//turn off the blinking
				Alcd_Display_Control(&lcd, 1, 0, 0);

				// store '0' in the entered time date buffer
				if (input_index < 2) {
					time_date_buffer[input_index] = '4';
					time_date_buffer[input_index + 1] = '\0'; // Null-terminate the string
					Alcd_PutAt(&lcd, 1, input_index, "4");
					input_index++;
					Alcd_CursorAt(&lcd, 1, input_index); // Move cursor to next position
				}

				//turn on the blinking again
				Alcd_Display_Control(&lcd, 1, input_index, 1);

				// Update keypad_delay after handling the key press
				general_delay = HAL_GetTick() + 250;

			}
			//5 is entered
			else if (Keypad_Get_Key(&kp, kp_button_5)
					&& (current_tick >= general_delay)) {

				//turn off the blinking
				Alcd_Display_Control(&lcd, 1, 0, 0);

				// store '0' in the entered time date buffer
				if (input_index < 2) {
					time_date_buffer[input_index] = '5';
					time_date_buffer[input_index + 1] = '\0'; // Null-terminate the string
					Alcd_PutAt(&lcd, 1, input_index, "5");
					input_index++;
					Alcd_CursorAt(&lcd, 1, input_index); // Move cursor to next position
				}

				//turn on the blinking again
				Alcd_Display_Control(&lcd, 1, input_index, 1);

				// Update keypad_delay after handling the key press
				general_delay = HAL_GetTick() + 250;

			}

			//6 is entered
			else if (Keypad_Get_Key(&kp, kp_button_6)
					&& (current_tick >= general_delay)) {

				//turn off the blinking
				Alcd_Display_Control(&lcd, 1, 0, 0);

				// store '0' in the entered time date buffer
				if (input_index < 2) {
					time_date_buffer[input_index] = '6';
					time_date_buffer[input_index + 1] = '\0'; // Null-terminate the string
					Alcd_PutAt(&lcd, 1, input_index, "6");
					input_index++;
					Alcd_CursorAt(&lcd, 1, input_index); // Move cursor to next position
				}

				//turn on the blinking again
				Alcd_Display_Control(&lcd, 1, input_index, 1);

				// Update keypad_delay after handling the key press
				general_delay = HAL_GetTick() + 250;

			}

			//7 is entered
			else if (Keypad_Get_Key(&kp, kp_button_7)
					&& (current_tick >= general_delay)) {

				//turn off the blinking
				Alcd_Display_Control(&lcd, 1, 0, 0);

				// store '0' in the entered time date buffer
				if (input_index < 2) {
					time_date_buffer[input_index] = '7';
					time_date_buffer[input_index + 1] = '\0'; // Null-terminate the string
					Alcd_PutAt(&lcd, 1, input_index, "7");
					input_index++;
					Alcd_CursorAt(&lcd, 1, input_index); // Move cursor to next position
				}

				//turn on the blinking again
				Alcd_Display_Control(&lcd, 1, input_index, 1);

				// Update keypad_delay after handling the key press
				general_delay = HAL_GetTick() + 250;

			}

			//8 is entered
			else if (Keypad_Get_Key(&kp, kp_button_8)
					&& (current_tick >= general_delay)) {

				//turn off the blinking
				Alcd_Display_Control(&lcd, 1, 0, 0);

				// store '0' in the entered time date buffer
				if (input_index < 2) {
					time_date_buffer[input_index] = '8';
					time_date_buffer[input_index + 1] = '\0'; // Null-terminate the string
					Alcd_PutAt(&lcd, 1, input_index, "8");
					input_index++;
					Alcd_CursorAt(&lcd, 1, input_index); // Move cursor to next position
				}

				//turn on the blinking again
				Alcd_Display_Control(&lcd, 1, input_index, 1);

				// Update keypad_delay after handling the key press
				general_delay = HAL_GetTick() + 250;

			}

			//9 is entered
			else if (Keypad_Get_Key(&kp, kp_button_9)
					&& (current_tick >= general_delay)) {

				//turn off the blinking
				Alcd_Display_Control(&lcd, 1, 0, 0);

				// store '0' in the entered time date buffer
				if (input_index < 2) {
					time_date_buffer[input_index] = '9';
					time_date_buffer[input_index + 1] = '\0'; // Null-terminate the string
					Alcd_PutAt(&lcd, 1, input_index, "9");
					input_index++;
					Alcd_CursorAt(&lcd, 1, input_index); // Move cursor to next position
				}

				//turn on the blinking again
				Alcd_Display_Control(&lcd, 1, input_index, 1);

				// Update keypad_delay after handling the key press
				general_delay = HAL_GetTick() + 250;

			}

			//when yes is entered -> check for hour validity (state 47)
			if (Keypad_Get_Key(&kp, kp_button_yes)
					&& (current_tick >= general_delay)) {

				Alcd_Clear(&lcd);

				snprintf(timeString, sizeof(timeString), "%02d", buffer);
				Alcd_PutAt_n(&lcd, 1, 0, timeString, strlen(timeString));

				status = 47;

			}

			//
			general_delay = HAL_GetTick() + 250;
		}

		//hours validation phase
		while ((status == 47) && (current_tick >= general_delay)) {

			//get the current tick number
			current_tick = HAL_GetTick();
			input_index = 0;

			Alcd_Clear(&lcd);

			snprintf(timeString, sizeof(timeString), "%02d", status);
			Alcd_PutAt_n(&lcd, 0, 14, timeString, strlen(timeString));

			buffer = atoi(time_date_buffer);

			//in case of 12h format
			if (CLK.format == 1) {

				//check if time is within rang 1 to 12
				if (buffer > 0 && buffer < 13) {

					//move to the previous menu
					status = 23;

					//set the hours to the value
					dose_h = buffer;

					input_index = 0;
					//turn off the blinking
					Alcd_Display_Control(&lcd, 1, 0, 0);
					//turn on the blinking again
					Alcd_Display_Control(&lcd, 1, input_index, 1);

				} else {
					Alcd_PutAt(&lcd, 0, 0, "invalid");
					delay_flag = 1;
					input_index = 0;

					//turn off the blinking
					Alcd_Display_Control(&lcd, 1, 0, 0);
					//turn on the blinking again
					Alcd_Display_Control(&lcd, 1, input_index, 1);

					//return to entering hours
					status = 46;

					general_delay = HAL_GetTick() + 1000;
				}
			}

			//in case of 24h format
			if (CLK.format == 0) {

				//check if time is within rang 0 to 24
				if (buffer >= 0 && buffer < 25) {

					//move to menu
					status = 23;

					//set the hours to the value
					dose_h = buffer;

					input_index = 0;
					//turn off the blinking
					Alcd_Display_Control(&lcd, 1, 0, 0);
					//turn on the blinking again
					Alcd_Display_Control(&lcd, 1, input_index, 1);
				} else {
					Alcd_PutAt(&lcd, 0, 0, "invalid");
					delay_flag = 1;

					input_index = 0;
					//turn off the blinking
					Alcd_Display_Control(&lcd, 1, 0, 0);
					//turn on the blinking again
					Alcd_Display_Control(&lcd, 1, input_index, 1);
					//return to entering hours
					status = 46;

					general_delay = HAL_GetTick() + 1000;
				}
			}

			//create a delay
			if ((delay_flag == 1) && (current_tick >= general_delay)) {

				delay_flag = 0;
			}

		}

		//save parameters? (state 48)
		while ((status == 48) && (current_tick >= general_delay)) {

			//get the current tick number
			current_tick = HAL_GetTick();

			Alcd_Clear(&lcd);
			Alcd_PutAt(&lcd, 0, 0, "save?");
			snprintf(timeString, sizeof(timeString), "%02d", status);
			Alcd_PutAt_n(&lcd, 0, 14, timeString, strlen(timeString));

			//check if any button is pressed
			//check for any keypad input
			Keypad_Refresh(&kp);

			//in case of yes
			if (Keypad_Get_Key(&kp, kp_button_yes)
					&& (current_tick >= general_delay)) {

				//save the parameters
				eeprom24c32_write(&memory, &dose_h, dosing_time_hours);
				eeprom24c32_write(&memory, &dose_m, dosing_time_minutes);
				eeprom24c32_write(&memory, &dose_s, dosing_time_seconds);
				status = 21;

			}

			//no is selected
			else if (Keypad_Get_Key(&kp, kp_button_no_back)
					&& (current_tick >= general_delay)) {

				status = 21;

			}

			general_delay = HAL_GetTick() + 250;
		}
		
		//extended edit parameters menu (state 49)
		while ((status == 49) && (current_tick >= general_delay)) {

			//get the current tick number
			current_tick = HAL_GetTick();

			Alcd_Clear(&lcd);
			Alcd_PutAt(&lcd, 0, 0, "3: minutes");
			Alcd_PutAt(&lcd, 1, 0, "4: seconds");
			snprintf(timeString, sizeof(timeString), "%02d", status);
			Alcd_PutAt_n(&lcd, 0, 14, timeString, strlen(timeString));

			//check if any button is pressed
			//check for any keypad input
			Keypad_Refresh(&kp);

			//in case 3 is selected -> edit minutes
			if (Keypad_Get_Key(&kp, kp_button_3)
					&& (current_tick >= general_delay)) {

				status = 50;

			}

			//4 is selected -> enter seconds
			else if (Keypad_Get_Key(&kp, kp_button_4)
					&& (current_tick >= general_delay)) {


				Alcd_Clear(&lcd);
				status = 51;

			}

			//back is selected
			else if (Keypad_Get_Key(&kp, kp_button_no_back)
					&& (current_tick >= general_delay)) {

				status = 23;

			}
			
			//previous is selected
			else if (Keypad_Get_Key(&kp, kp_button_previous)
					&& (current_tick >= general_delay)) {

				status = 23;

			}

			general_delay = HAL_GetTick() + 250;
		}
		
		//entering the minutes state (50)
		while ((status == 50) && (current_tick >= general_delay)) {

			//get the current tick number
			current_tick = HAL_GetTick();

			//Alcd_Clear(&lcd);
			Alcd_PutAt(&lcd, 0, 0, "Enter minutes");

			snprintf(timeString, sizeof(timeString), "%02d", status);
			Alcd_PutAt_n(&lcd, 0, 14, timeString, strlen(timeString));

			Alcd_CursorAt(&lcd, 1, input_index);
			Alcd_Display_Control(&lcd, 1, 1, 1);

			//check if any button is pressed
			//check for any keypad input
			Keypad_Refresh(&kp);

			//if back is entered
			if (Keypad_Get_Key(&kp, kp_button_no_back)
					&& (current_tick >= general_delay)) {

				//back to previous menu
				status = 49;

			}

			//0 is entered
			else if (Keypad_Get_Key(&kp, kp_button_0)
					&& (current_tick >= general_delay)) {

				//turn off the blinking
				Alcd_Display_Control(&lcd, 1, 0, 0);

				// store '0' in the entered time date buffer
				if (input_index < 2) {
					time_date_buffer[input_index] = '0';
					time_date_buffer[input_index + 1] = '\0'; // Null-terminate the string
					Alcd_PutAt(&lcd, 1, input_index, "0");
					input_index++;
					Alcd_CursorAt(&lcd, 1, input_index); // Move cursor to next position
				}

				//turn on the blinking again
				Alcd_Display_Control(&lcd, 1, input_index, 1);

				// Update keypad_delay after handling the key press
				general_delay = HAL_GetTick() + 250;

			}
			//1 is entered
			else if (Keypad_Get_Key(&kp, kp_button_1)
					&& (current_tick >= general_delay)) {

				//turn off the blinking
				Alcd_Display_Control(&lcd, 1, 0, 0);

				// store '0' in the entered time date buffer
				if (input_index < 2) {
					time_date_buffer[input_index] = '1';
					time_date_buffer[input_index + 1] = '\0'; // Null-terminate the string
					Alcd_PutAt(&lcd, 1, input_index, "1");
					input_index++;
					Alcd_CursorAt(&lcd, 1, input_index); // Move cursor to next position
				}

				//turn on the blinking again
				Alcd_Display_Control(&lcd, 1, input_index, 1);

				// Update keypad_delay after handling the key press
				general_delay = HAL_GetTick() + 250;

			}
			//2 is entered
			else if (Keypad_Get_Key(&kp, kp_button_2)
					&& (current_tick >= general_delay)) {

				//turn off the blinking
				Alcd_Display_Control(&lcd, 1, 0, 0);

				// store '0' in the entered time date buffer
				if (input_index < 2) {
					time_date_buffer[input_index] = '2';
					time_date_buffer[input_index + 1] = '\0'; // Null-terminate the string
					Alcd_PutAt(&lcd, 1, input_index, "2");
					input_index++;
					Alcd_CursorAt(&lcd, 1, input_index); // Move cursor to next position
				}

				//turn on the blinking again
				Alcd_Display_Control(&lcd, 1, input_index, 1);

				// Update keypad_delay after handling the key press
				general_delay = HAL_GetTick() + 250;

			}
			//3 is entered
			else if (Keypad_Get_Key(&kp, kp_button_3)
					&& (current_tick >= general_delay)) {

				//turn off the blinking
				Alcd_Display_Control(&lcd, 1, 0, 0);

				// store '0' in the entered time date buffer
				if (input_index < 2) {
					time_date_buffer[input_index] = '3';
					time_date_buffer[input_index + 1] = '\0'; // Null-terminate the string
					Alcd_PutAt(&lcd, 1, input_index, "3");
					input_index++;
					Alcd_CursorAt(&lcd, 1, input_index); // Move cursor to next position
				}

				//turn on the blinking again
				Alcd_Display_Control(&lcd, 1, input_index, 1);

				// Update keypad_delay after handling the key press
				general_delay = HAL_GetTick() + 250;

			}

			//4 is entered
			else if (Keypad_Get_Key(&kp, kp_button_4)
					&& (current_tick >= general_delay)) {

				//turn off the blinking
				Alcd_Display_Control(&lcd, 1, 0, 0);

				// store '0' in the entered time date buffer
				if (input_index < 2) {
					time_date_buffer[input_index] = '4';
					time_date_buffer[input_index + 1] = '\0'; // Null-terminate the string
					Alcd_PutAt(&lcd, 1, input_index, "4");
					input_index++;
					Alcd_CursorAt(&lcd, 1, input_index); // Move cursor to next position
				}

				//turn on the blinking again
				Alcd_Display_Control(&lcd, 1, input_index, 1);

				// Update keypad_delay after handling the key press
				general_delay = HAL_GetTick() + 250;

			}
			//5 is entered
			else if (Keypad_Get_Key(&kp, kp_button_5)
					&& (current_tick >= general_delay)) {

				//turn off the blinking
				Alcd_Display_Control(&lcd, 1, 0, 0);

				// store '0' in the entered time date buffer
				if (input_index < 2) {
					time_date_buffer[input_index] = '5';
					time_date_buffer[input_index + 1] = '\0'; // Null-terminate the string
					Alcd_PutAt(&lcd, 1, input_index, "5");
					input_index++;
					Alcd_CursorAt(&lcd, 1, input_index); // Move cursor to next position
				}

				//turn on the blinking again
				Alcd_Display_Control(&lcd, 1, input_index, 1);

				// Update keypad_delay after handling the key press
				general_delay = HAL_GetTick() + 250;

			}

			//6 is entered
			else if (Keypad_Get_Key(&kp, kp_button_6)
					&& (current_tick >= general_delay)) {

				//turn off the blinking
				Alcd_Display_Control(&lcd, 1, 0, 0);

				// store '0' in the entered time date buffer
				if (input_index < 2) {
					time_date_buffer[input_index] = '6';
					time_date_buffer[input_index + 1] = '\0'; // Null-terminate the string
					Alcd_PutAt(&lcd, 1, input_index, "6");
					input_index++;
					Alcd_CursorAt(&lcd, 1, input_index); // Move cursor to next position
				}

				//turn on the blinking again
				Alcd_Display_Control(&lcd, 1, input_index, 1);

				// Update keypad_delay after handling the key press
				general_delay = HAL_GetTick() + 250;

			}

			//7 is entered
			else if (Keypad_Get_Key(&kp, kp_button_7)
					&& (current_tick >= general_delay)) {

				//turn off the blinking
				Alcd_Display_Control(&lcd, 1, 0, 0);

				// store '0' in the entered time date buffer
				if (input_index < 2) {
					time_date_buffer[input_index] = '7';
					time_date_buffer[input_index + 1] = '\0'; // Null-terminate the string
					Alcd_PutAt(&lcd, 1, input_index, "7");
					input_index++;
					Alcd_CursorAt(&lcd, 1, input_index); // Move cursor to next position
				}

				//turn on the blinking again
				Alcd_Display_Control(&lcd, 1, input_index, 1);

				// Update keypad_delay after handling the key press
				general_delay = HAL_GetTick() + 250;

			}

			//8 is entered
			else if (Keypad_Get_Key(&kp, kp_button_8)
					&& (current_tick >= general_delay)) {

				//turn off the blinking
				Alcd_Display_Control(&lcd, 1, 0, 0);

				// store '0' in the entered time date buffer
				if (input_index < 2) {
					time_date_buffer[input_index] = '8';
					time_date_buffer[input_index + 1] = '\0'; // Null-terminate the string
					Alcd_PutAt(&lcd, 1, input_index, "8");
					input_index++;
					Alcd_CursorAt(&lcd, 1, input_index); // Move cursor to next position
				}

				//turn on the blinking again
				Alcd_Display_Control(&lcd, 1, input_index, 1);

				// Update keypad_delay after handling the key press
				general_delay = HAL_GetTick() + 250;

			}

			//9 is entered
			else if (Keypad_Get_Key(&kp, kp_button_9)
					&& (current_tick >= general_delay)) {

				//turn off the blinking
				Alcd_Display_Control(&lcd, 1, 0, 0);

				// store '0' in the entered time date buffer
				if (input_index < 2) {
					time_date_buffer[input_index] = '9';
					time_date_buffer[input_index + 1] = '\0'; // Null-terminate the string
					Alcd_PutAt(&lcd, 1, input_index, "9");
					input_index++;
					Alcd_CursorAt(&lcd, 1, input_index); // Move cursor to next position
				}

				//turn on the blinking again
				Alcd_Display_Control(&lcd, 1, input_index, 1);

				// Update keypad_delay after handling the key press
				general_delay = HAL_GetTick() + 250;

			}

			//when yes is entered -> check for miinutes validity (state 32)
			if (Keypad_Get_Key(&kp, kp_button_yes)
					&& (current_tick >= general_delay)) {

				Alcd_Clear(&lcd);

				snprintf(timeString, sizeof(timeString), "%02d", buffer);
				Alcd_PutAt_n(&lcd, 1, 0, timeString, strlen(timeString));

//go to validation phase
				status = 52;

			}

			//
			general_delay = HAL_GetTick() + 250;
		}

		//minutes validation phase
		while ((status == 52) && (current_tick >= general_delay)) {

			//get the current tick number
			current_tick = HAL_GetTick();
			input_index = 0;

			Alcd_Clear(&lcd);

			snprintf(timeString, sizeof(timeString), "%02d", status);
			Alcd_PutAt_n(&lcd, 0, 14, timeString, strlen(timeString));

			buffer = atoi(time_date_buffer);

			//check if minutes is within the range 0 to 60
			if (buffer >= 0 && buffer < 61) {


				status = 49;

				//set the minutes to the value
				dose_m = buffer;

				input_index = 0;
				//turn off the blinking
				Alcd_Display_Control(&lcd, 1, 0, 0);
				//turn on the blinking again
				Alcd_Display_Control(&lcd, 1, input_index, 1);
			} else {
				Alcd_PutAt(&lcd, 0, 0, "invalid");
				delay_flag = 1;

				input_index = 0;
				//turn off the blinking
				Alcd_Display_Control(&lcd, 1, 0, 0);
				//turn on the blinking again
				Alcd_Display_Control(&lcd, 1, input_index, 1);
				//return to entering hours
				status = 50;

				general_delay = HAL_GetTick() + 1000;
			}

			//create a delay
			if ((delay_flag == 1) && (current_tick >= general_delay)) {

				delay_flag = 0;
			}

		}

//entering the seconds state (51)
		while ((status == 51) && (current_tick >= general_delay)) {

			//get the current tick number
			current_tick = HAL_GetTick();

			//Alcd_Clear(&lcd);
			Alcd_PutAt(&lcd, 0, 0, "Enter seconds");

			snprintf(timeString, sizeof(timeString), "%02d", status);
			Alcd_PutAt_n(&lcd, 0, 14, timeString, strlen(timeString));

			Alcd_CursorAt(&lcd, 1, input_index);
			Alcd_Display_Control(&lcd, 1, 1, 1);

			//check if any button is pressed
			//check for any keypad input
			Keypad_Refresh(&kp);

			//if back is entered
			if (Keypad_Get_Key(&kp, kp_button_no_back)
					&& (current_tick >= general_delay)) {

				//back to previous menu
				status = 49;

			}

			//0 is entered
			else if (Keypad_Get_Key(&kp, kp_button_0)
					&& (current_tick >= general_delay)) {

				//turn off the blinking
				Alcd_Display_Control(&lcd, 1, 0, 0);

				// store '0' in the entered time date buffer
				if (input_index < 2) {
					time_date_buffer[input_index] = '0';
					time_date_buffer[input_index + 1] = '\0'; // Null-terminate the string
					Alcd_PutAt(&lcd, 1, input_index, "0");
					input_index++;
					Alcd_CursorAt(&lcd, 1, input_index); // Move cursor to next position
				}

				//turn on the blinking again
				Alcd_Display_Control(&lcd, 1, input_index, 1);

				// Update keypad_delay after handling the key press
				general_delay = HAL_GetTick() + 250;

			}
			//1 is entered
			else if (Keypad_Get_Key(&kp, kp_button_1)
					&& (current_tick >= general_delay)) {

				//turn off the blinking
				Alcd_Display_Control(&lcd, 1, 0, 0);

				// store '0' in the entered time date buffer
				if (input_index < 2) {
					time_date_buffer[input_index] = '1';
					time_date_buffer[input_index + 1] = '\0'; // Null-terminate the string
					Alcd_PutAt(&lcd, 1, input_index, "1");
					input_index++;
					Alcd_CursorAt(&lcd, 1, input_index); // Move cursor to next position
				}

				//turn on the blinking again
				Alcd_Display_Control(&lcd, 1, input_index, 1);

				// Update keypad_delay after handling the key press
				general_delay = HAL_GetTick() + 250;

			}
			//2 is entered
			else if (Keypad_Get_Key(&kp, kp_button_2)
					&& (current_tick >= general_delay)) {

				//turn off the blinking
				Alcd_Display_Control(&lcd, 1, 0, 0);

				// store '0' in the entered time date buffer
				if (input_index < 2) {
					time_date_buffer[input_index] = '2';
					time_date_buffer[input_index + 1] = '\0'; // Null-terminate the string
					Alcd_PutAt(&lcd, 1, input_index, "2");
					input_index++;
					Alcd_CursorAt(&lcd, 1, input_index); // Move cursor to next position
				}

				//turn on the blinking again
				Alcd_Display_Control(&lcd, 1, input_index, 1);

				// Update keypad_delay after handling the key press
				general_delay = HAL_GetTick() + 250;

			}
			//3 is entered
			else if (Keypad_Get_Key(&kp, kp_button_3)
					&& (current_tick >= general_delay)) {

				//turn off the blinking
				Alcd_Display_Control(&lcd, 1, 0, 0);

				// store '0' in the entered time date buffer
				if (input_index < 2) {
					time_date_buffer[input_index] = '3';
					time_date_buffer[input_index + 1] = '\0'; // Null-terminate the string
					Alcd_PutAt(&lcd, 1, input_index, "3");
					input_index++;
					Alcd_CursorAt(&lcd, 1, input_index); // Move cursor to next position
				}

				//turn on the blinking again
				Alcd_Display_Control(&lcd, 1, input_index, 1);

				// Update keypad_delay after handling the key press
				general_delay = HAL_GetTick() + 250;

			}

			//4 is entered
			else if (Keypad_Get_Key(&kp, kp_button_4)
					&& (current_tick >= general_delay)) {

				//turn off the blinking
				Alcd_Display_Control(&lcd, 1, 0, 0);

				// store '0' in the entered time date buffer
				if (input_index < 2) {
					time_date_buffer[input_index] = '4';
					time_date_buffer[input_index + 1] = '\0'; // Null-terminate the string
					Alcd_PutAt(&lcd, 1, input_index, "4");
					input_index++;
					Alcd_CursorAt(&lcd, 1, input_index); // Move cursor to next position
				}

				//turn on the blinking again
				Alcd_Display_Control(&lcd, 1, input_index, 1);

				// Update keypad_delay after handling the key press
				general_delay = HAL_GetTick() + 250;

			}
			//5 is entered
			else if (Keypad_Get_Key(&kp, kp_button_5)
					&& (current_tick >= general_delay)) {

				//turn off the blinking
				Alcd_Display_Control(&lcd, 1, 0, 0);

				// store '0' in the entered time date buffer
				if (input_index < 2) {
					time_date_buffer[input_index] = '5';
					time_date_buffer[input_index + 1] = '\0'; // Null-terminate the string
					Alcd_PutAt(&lcd, 1, input_index, "5");
					input_index++;
					Alcd_CursorAt(&lcd, 1, input_index); // Move cursor to next position
				}

				//turn on the blinking again
				Alcd_Display_Control(&lcd, 1, input_index, 1);

				// Update keypad_delay after handling the key press
				general_delay = HAL_GetTick() + 250;

			}

			//6 is entered
			else if (Keypad_Get_Key(&kp, kp_button_6)
					&& (current_tick >= general_delay)) {

				//turn off the blinking
				Alcd_Display_Control(&lcd, 1, 0, 0);

				// store '0' in the entered time date buffer
				if (input_index < 2) {
					time_date_buffer[input_index] = '6';
					time_date_buffer[input_index + 1] = '\0'; // Null-terminate the string
					Alcd_PutAt(&lcd, 1, input_index, "6");
					input_index++;
					Alcd_CursorAt(&lcd, 1, input_index); // Move cursor to next position
				}

				//turn on the blinking again
				Alcd_Display_Control(&lcd, 1, input_index, 1);

				// Update keypad_delay after handling the key press
				general_delay = HAL_GetTick() + 250;

			}

			//7 is entered
			else if (Keypad_Get_Key(&kp, kp_button_7)
					&& (current_tick >= general_delay)) {

				//turn off the blinking
				Alcd_Display_Control(&lcd, 1, 0, 0);

				// store '0' in the entered time date buffer
				if (input_index < 2) {
					time_date_buffer[input_index] = '7';
					time_date_buffer[input_index + 1] = '\0'; // Null-terminate the string
					Alcd_PutAt(&lcd, 1, input_index, "7");
					input_index++;
					Alcd_CursorAt(&lcd, 1, input_index); // Move cursor to next position
				}

				//turn on the blinking again
				Alcd_Display_Control(&lcd, 1, input_index, 1);

				// Update keypad_delay after handling the key press
				general_delay = HAL_GetTick() + 250;

			}

			//8 is entered
			else if (Keypad_Get_Key(&kp, kp_button_8)
					&& (current_tick >= general_delay)) {

				//turn off the blinking
				Alcd_Display_Control(&lcd, 1, 0, 0);

				// store '0' in the entered time date buffer
				if (input_index < 2) {
					time_date_buffer[input_index] = '8';
					time_date_buffer[input_index + 1] = '\0'; // Null-terminate the string
					Alcd_PutAt(&lcd, 1, input_index, "8");
					input_index++;
					Alcd_CursorAt(&lcd, 1, input_index); // Move cursor to next position
				}

				//turn on the blinking again
				Alcd_Display_Control(&lcd, 1, input_index, 1);

				// Update keypad_delay after handling the key press
				general_delay = HAL_GetTick() + 250;

			}

			//9 is entered
			else if (Keypad_Get_Key(&kp, kp_button_9)
					&& (current_tick >= general_delay)) {

				//turn off the blinking
				Alcd_Display_Control(&lcd, 1, 0, 0);

				// store '0' in the entered time date buffer
				if (input_index < 2) {
					time_date_buffer[input_index] = '9';
					time_date_buffer[input_index + 1] = '\0'; // Null-terminate the string
					Alcd_PutAt(&lcd, 1, input_index, "9");
					input_index++;
					Alcd_CursorAt(&lcd, 1, input_index); // Move cursor to next position
				}

				//turn on the blinking again
				Alcd_Display_Control(&lcd, 1, input_index, 1);

				// Update keypad_delay after handling the key press
				general_delay = HAL_GetTick() + 250;

			}

			//when yes is entered -> check for seconds validity (state 34)
			if (Keypad_Get_Key(&kp, kp_button_yes)
					&& (current_tick >= general_delay)) {

				Alcd_Clear(&lcd);

				snprintf(timeString, sizeof(timeString), "%02d", buffer);
				Alcd_PutAt_n(&lcd, 1, 0, timeString, strlen(timeString));

				status = 34;

			}

			//
			general_delay = HAL_GetTick() + 250;
		}

		//seconds validation phase
		while ((status == 53) && (current_tick >= general_delay)) {

			//get the current tick number
			current_tick = HAL_GetTick();

			input_index = 0;

			Alcd_Clear(&lcd);

			snprintf(timeString, sizeof(timeString), "%02d", status);
			Alcd_PutAt_n(&lcd, 0, 14, timeString, strlen(timeString));

			buffer = atoi(time_date_buffer);

			//check if seconds is within the range 0 to 60
			if (buffer >= 0 && buffer < 61) {


				status = 49;

				//set the minutes to the value
				dose_m = buffer;

				input_index = 0;
				//turn off the blinking
				Alcd_Display_Control(&lcd, 1, 0, 0);
				//turn on the blinking again
				Alcd_Display_Control(&lcd, 1, input_index, 1);
			} else {
				Alcd_PutAt(&lcd, 0, 0, "invalid");
				delay_flag = 1;

				input_index = 0;
				//turn off the blinking
				Alcd_Display_Control(&lcd, 1, 0, 0);
				//turn on the blinking again
				Alcd_Display_Control(&lcd, 1, input_index, 1);
				//return to entering hours
				status = 51;

				general_delay = HAL_GetTick() + 1000;
			}

			//create a delay
			if ((delay_flag == 1) && (current_tick >= general_delay)) {

				delay_flag = 0;
			}

		}

//
//
	}			//end of while 1

	/* USER CODE END 3 */
}

/**
 * @brief System Clock Configuration
 * @retval None
 */
void SystemClock_Config(void) {
	RCC_OscInitTypeDef RCC_OscInitStruct = { 0 };
	RCC_ClkInitTypeDef RCC_ClkInitStruct = { 0 };

	/** Initializes the RCC Oscillators according to the specified parameters
	 * in the RCC_OscInitTypeDef structure.
	 */
	RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
	RCC_OscInitStruct.HSEState = RCC_HSE_ON;
	RCC_OscInitStruct.HSEPredivValue = RCC_HSE_PREDIV_DIV1;
	RCC_OscInitStruct.HSIState = RCC_HSI_ON;
	RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
	RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
	RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL9;
	if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK) {
		Error_Handler();
	}

	/** Initializes the CPU, AHB and APB buses clocks
	 */
	RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK
			| RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
	RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
	RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
	RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
	RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

	if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK) {
		Error_Handler();
	}
}

/**
 * @brief I2C2 Initialization Function
 * @param None
 * @retval None
 */
static void MX_I2C2_Init(void) {

	/* USER CODE BEGIN I2C2_Init 0 */

	/* USER CODE END I2C2_Init 0 */

	/* USER CODE BEGIN I2C2_Init 1 */

	/* USER CODE END I2C2_Init 1 */
	hi2c2.Instance = I2C2;
	hi2c2.Init.ClockSpeed = 100000;
	hi2c2.Init.DutyCycle = I2C_DUTYCYCLE_2;
	hi2c2.Init.OwnAddress1 = 0;
	hi2c2.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
	hi2c2.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
	hi2c2.Init.OwnAddress2 = 0;
	hi2c2.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
	hi2c2.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
	if (HAL_I2C_Init(&hi2c2) != HAL_OK) {
		Error_Handler();
	}
	/* USER CODE BEGIN I2C2_Init 2 */

	/* USER CODE END I2C2_Init 2 */

}

/**
 * @brief TIM1 Initialization Function
 * @param None
 * @retval None
 */
static void MX_TIM1_Init(void) {

	/* USER CODE BEGIN TIM1_Init 0 */

	/* USER CODE END TIM1_Init 0 */

	TIM_MasterConfigTypeDef sMasterConfig = { 0 };
	TIM_OC_InitTypeDef sConfigOC = { 0 };
	TIM_BreakDeadTimeConfigTypeDef sBreakDeadTimeConfig = { 0 };

	/* USER CODE BEGIN TIM1_Init 1 */

	/* USER CODE END TIM1_Init 1 */
	htim1.Instance = TIM1;
	htim1.Init.Prescaler = 71;
	htim1.Init.CounterMode = TIM_COUNTERMODE_UP;
	htim1.Init.Period = 19999;
	htim1.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
	htim1.Init.RepetitionCounter = 0;
	htim1.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
	if (HAL_TIM_PWM_Init(&htim1) != HAL_OK) {
		Error_Handler();
	}
	sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
	sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
	if (HAL_TIMEx_MasterConfigSynchronization(&htim1, &sMasterConfig)
			!= HAL_OK) {
		Error_Handler();
	}
	sConfigOC.OCMode = TIM_OCMODE_PWM1;
	sConfigOC.Pulse = 1000;
	sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
	sConfigOC.OCNPolarity = TIM_OCNPOLARITY_HIGH;
	sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
	sConfigOC.OCIdleState = TIM_OCIDLESTATE_RESET;
	sConfigOC.OCNIdleState = TIM_OCNIDLESTATE_RESET;
	if (HAL_TIM_PWM_ConfigChannel(&htim1, &sConfigOC, TIM_CHANNEL_1)
			!= HAL_OK) {
		Error_Handler();
	}
	sBreakDeadTimeConfig.OffStateRunMode = TIM_OSSR_DISABLE;
	sBreakDeadTimeConfig.OffStateIDLEMode = TIM_OSSI_DISABLE;
	sBreakDeadTimeConfig.LockLevel = TIM_LOCKLEVEL_OFF;
	sBreakDeadTimeConfig.DeadTime = 0;
	sBreakDeadTimeConfig.BreakState = TIM_BREAK_DISABLE;
	sBreakDeadTimeConfig.BreakPolarity = TIM_BREAKPOLARITY_HIGH;
	sBreakDeadTimeConfig.AutomaticOutput = TIM_AUTOMATICOUTPUT_DISABLE;
	if (HAL_TIMEx_ConfigBreakDeadTime(&htim1, &sBreakDeadTimeConfig)
			!= HAL_OK) {
		Error_Handler();
	}
	/* USER CODE BEGIN TIM1_Init 2 */

	/* USER CODE END TIM1_Init 2 */
	HAL_TIM_MspPostInit(&htim1);

}

/**
 * @brief GPIO Initialization Function
 * @param None
 * @retval None
 */
static void MX_GPIO_Init(void) {
	/* USER CODE BEGIN MX_GPIO_Init_1 */
	/* USER CODE END MX_GPIO_Init_1 */

	/* GPIO Ports Clock Enable */
	__HAL_RCC_GPIOD_CLK_ENABLE();
	__HAL_RCC_GPIOB_CLK_ENABLE();
	__HAL_RCC_GPIOA_CLK_ENABLE();
	__HAL_RCC_GPIOC_CLK_ENABLE();

	/* USER CODE BEGIN MX_GPIO_Init_2 */
	/* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

/**
 * @brief  This function is executed in case of error occurrence.
 * @retval None
 */
void Error_Handler(void) {
	/* USER CODE BEGIN Error_Handler_Debug */
	/* User can add his own implementation to report the HAL error return state */
	__disable_irq();
	while (1) {
	}
	/* USER CODE END Error_Handler_Debug */
}

#ifdef  USE_FULL_ASSERT
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
