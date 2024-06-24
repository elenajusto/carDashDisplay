/*
 * LCD12864B.c
 *
 *  Created on: Jun 13, 2024
 *      Author: elena
 */
#include "ST7920LCD.h"
#include "stm32g0xx.h"

#define SCLK_PIN GPIO_PIN_4
#define SCLK_PORT GPIOA
#define CS_PIN GPIO_PIN_0
#define CS_PORT GPIOA
#define SID_PIN GPIO_PIN_1
#define SID_PORT GPIOA
#define RST_PIN GPIO_PIN_1
#define RST_PORT GPIOB

uint8_t startRow, startCol, endRow, endCol; 			// coordinates of the dirty rectangle
uint8_t numRows = 64;
uint8_t numCols = 128;
uint8_t Graphic_Check = 0;

/* LCD CONTROL FUNCTION DEFINITIONS */

/* A replacement for SPI_TRANSMIT */
void SendByteSPI(uint8_t byte)
{
	for (int i=0;i<8;i++){
		if ( (byte<<i)&0x80 ){
				HAL_GPIO_WritePin(SID_PORT, SID_PIN, GPIO_PIN_SET);		// SID=1  OR MOSI
			} else {
				HAL_GPIO_WritePin(SID_PORT, SID_PIN, GPIO_PIN_RESET);	// SID=0
			}
		HAL_GPIO_WritePin(SCLK_PORT, SCLK_PIN, GPIO_PIN_RESET);			// SCLK =0  OR SCK
		HAL_GPIO_WritePin(SCLK_PORT, SCLK_PIN, GPIO_PIN_SET);			// SCLK=1
	}
}

void ST7920_SendCmd (uint8_t cmd)
{
	HAL_GPIO_WritePin(CS_PORT, CS_PIN, GPIO_PIN_SET);  	  // PUll the CS high

	SendByteSPI(0xf8+(0<<1));  						   	  // send the SYNC + RS(0)
	SendByteSPI(cmd&0xf0);  						   	  // send the higher nibble first
	SendByteSPI((cmd<<4)&0xf0);  					      // send the lower nibble
	delay_us(50);

	HAL_GPIO_WritePin(CS_PORT, CS_PIN, GPIO_PIN_RESET);   // PUll the CS LOW
}

void ST7920_SendData (uint8_t data)
{
	HAL_GPIO_WritePin(CS_PORT, CS_PIN, GPIO_PIN_SET);  	// PUll the CS high

	SendByteSPI(0xf8+(1<<1));  							// send the SYNC + RS(1)
	SendByteSPI(data&0xf0);  							// send the higher nibble first
	SendByteSPI((data<<4)&0xf0);  						// send the lower nibble
	delay_us(50);
	HAL_GPIO_WritePin(CS_PORT, CS_PIN, GPIO_PIN_RESET); // PUll the CS LOW
}

void ST7920_SendString(int row, int col, char* string)
{
    switch (row)
    {
        case 0:
            col |= 0x80;
            break;
        case 1:
            col |= 0x90;
            break;
        case 2:
            col |= 0x88;
            break;
        case 3:
            col |= 0x98;
            break;
        default:
            col |= 0x80;
            break;
    }

    ST7920_SendCmd(col);

    while (*string)
    	{
    		ST7920_SendData(*string++);
    	}
}

/* Switch to graphic mode or normal mode::: enable = 1 -> graphic mode enable = 0 -> normal mode */
void ST7920_GraphicMode (int enable)   // 1-enable, 0-disable
{
	if (enable == 1)
	{
		ST7920_SendCmd(0x30);  	// 8 bit mode
		HAL_Delay (1);
		ST7920_SendCmd(0x34);  	// switch to Extended instructions
		HAL_Delay (1);
		ST7920_SendCmd(0x36);  	// enable graphics
		HAL_Delay (1);
		Graphic_Check = 1;  	// update the variable
	}

	else if (enable == 0)
	{
		ST7920_SendCmd(0x30);  	// 8 bit mode
		HAL_Delay (1);
		Graphic_Check = 0;  	// update the variable
	}
}

void ST7920_DrawBitmap(const unsigned char* graphic)
{
	uint8_t x, y;
	for(y = 0; y < 64; y++)
	{
		if(y < 32)
		{
			for(x = 0; x < 8; x++)							// Draws top half of the screen.
			{												// In extended instruction mode, vertical and horizontal coordinates must be specified before sending data in.
				ST7920_SendCmd(0x80 | y);					// Vertical coordinate of the screen is specified first. (0-31)
				ST7920_SendCmd(0x80 | x);					// Then horizontal coordinate of the screen is specified. (0-8)
				ST7920_SendData(graphic[2*x + 16*y]);		// Data to the upper byte is sent to the coordinate.
				ST7920_SendData(graphic[2*x+1 + 16*y]);		// Data to the lower byte is sent to the coordinate.
			}
		}
		else
		{
			for(x = 0; x < 8; x++)							// Draws bottom half of the screen.
			{												// Actions performed as same as the upper half screen.
				ST7920_SendCmd(0x80 | (y-32));				// Vertical coordinate must be scaled back to 0-31 as it is dealing with another half of the screen.
				ST7920_SendCmd(0x88 | x);
				ST7920_SendData(graphic[2*x + 16*y]);
				ST7920_SendData(graphic[2*x+1 + 16*y]);
			}
		}

	}
}

/* Update the display with the selected graphics */
void ST7920_Update(void)
{
	ST7920_DrawBitmap(image);
}

void ST7920_Clear()
{
	if (Graphic_Check == 1)  // if the graphic mode is set
	{
		uint8_t x, y;
		for(y = 0; y < 64; y++)
		{
			if(y < 32)
			{
				ST7920_SendCmd(0x80 | y);
				ST7920_SendCmd(0x80);
			}
			else
			{
				ST7920_SendCmd(0x80 | (y-32));
				ST7920_SendCmd(0x88);
			}
			for(x = 0; x < 8; x++)
			{
				ST7920_SendData(0);
				ST7920_SendData(0);
			}
		}
	}

	else
	{
		ST7920_SendCmd(0x01);   // clear the display using command
		HAL_Delay(2); // delay >1.6 ms
	}
}

void ST7920_Init (void)
{
	HAL_GPIO_WritePin(RST_PORT, RST_PIN, GPIO_PIN_RESET);  // RESET=0
	HAL_Delay(10);   // wait for 10ms
	HAL_GPIO_WritePin(RST_PORT, RST_PIN, GPIO_PIN_SET);  // RESET=1

	HAL_Delay(50);   		//wait for >40 ms


	ST7920_SendCmd(0x30);  	// 8bit mode
	delay_us(110);  		//  >100us delay

	ST7920_SendCmd(0x30);  	// 8bit mode
	delay_us(40);  			// >37us delay

	ST7920_SendCmd(0x08);  	// D=0, C=0, B=0
	delay_us(110);  		// >100us delay

	ST7920_SendCmd(0x01);  	// clear screen
	HAL_Delay(12);  		// >10 ms delay


	ST7920_SendCmd(0x06);  	// cursor increment right no shift
	HAL_Delay(1);  			// 1ms delay

	ST7920_SendCmd(0x0C);  	// D=1, C=0, B=0
    HAL_Delay(1);  			// 1ms delay

	ST7920_SendCmd(0x02);  	// return to home
	HAL_Delay(1);  			// 1ms delay

}

/* DELAY FUNCTION DEFINITIONS */
extern TIM_HandleTypeDef htim1;

void delay_init ()
{
	HAL_TIM_Base_Start(&htim1);  					// Change according to setup
}

void delay_us (uint16_t delay)
{
	__HAL_TIM_SET_COUNTER(&htim1, 0);  				// Reset the counter
	while ((__HAL_TIM_GET_COUNTER(&htim1))<delay);  // Wait for the delay to complete
}

void delay_ms(uint16_t delay)
{
	HAL_Delay (delay);
}

/* ANIMATION FUNCTION DEFINITIONS */
const uint8_t* FrontBitmapArray[] = {
    Front1degLeft, Front2degLeft, Front3degLeft, Front4degLeft,
    Front5degLeft, Front6degLeft, Front7degLeft, Front8degLeft,
    Front9degLeft, Front10degLeft, Front11degLeft, Front12degLeft,
    Front13degLeft, Front14degLeft, Front15degLeft, Front16degLeft,
    Front17degLeft, Front18degLeft, Front19degLeft, Front20degLeft,
    Front21degLeft, Front22degLeft, Front23degLeft, Front24degLeft,
    Front25degLeft, Front26degLeft, Front27degLeft, Front28degLeft,
    Front29degLeft, Front30degLeft, Front31degLeft, Front32degLeft,
    Front33degLeft, Front34degLeft, Front35degLeft
};

const uint8_t* rightAnimations[] = {
    frontRight1, frontRight2, frontRight3, frontRight4, frontRight5,
    frontRight6, frontRight7, frontRight8, frontRight9, frontRight10,
    frontRight11, frontRight12, frontRight13, frontRight14, frontRight15,
    frontRight16, frontRight17, frontRight18, frontRight19, frontRight20,
    frontRight21, frontRight22, frontRight23, frontRight24, frontRight25,
    frontRight26, frontRight27, frontRight28, frontRight29, frontRight30,
    frontRight31, frontRight32, frontRight33, frontRight34, frontRight35
};

void DrawLeftBitmapsInLoop(void) {
    int i;
    ST7920_GraphicMode(1);

    // Loop to count up from 1 to 35
    for (i = 0; i < 35; i++) {
        ST7920_DrawBitmap(FrontBitmapArray[i]);
    }

    // Loop to count down from 35 to 1
    for (i = 34; i >= 0; i--) {
        ST7920_DrawBitmap(FrontBitmapArray[i]);
    }
}

void DrawRightAnimationsInLoop(void) {
    int i;
    ST7920_GraphicMode(1);

    // Loop to count up from 1 to 35
    for (i = 0; i < 35; i++) {
        ST7920_DrawBitmap(rightAnimations[i]);
    }

    // Loop to count down from 35 to 0
    for (i = 34; i >= 0; i--) {
        ST7920_DrawBitmap(rightAnimations[i]);
    }
}

/* STATE DISPLAY FUNCTIONS */
void stateOne(){
	ST7920_Clear();
	ST7920_GraphicMode(0);

	ST7920_SendString(0,2, "State 1");
	ST7920_SendString(1,2, "Mode:");
	ST7920_SendString(2,2, "Manual");
	//ST7920_SendString(3,0, "Auto: ON/OFF");
}

void stateTwo(){
	ST7920_Clear();
	ST7920_GraphicMode(0);

	ST7920_SendString(0,2, "State 2");
	ST7920_SendString(1,2, "Mode:");
	ST7920_SendString(2,2, "Autonomous");
	//ST7920_SendString(3,0, "Auto: ON/OFF");
}


void stateThree(){
	ST7920_Clear();
	ST7920_GraphicMode(0);

	ST7920_SendString(0,2, "State 3");
	ST7920_SendString(1,2, "Mode");
	ST7920_SendString(2,2, "Inspection");
	//ST7920_SendString(3,0, "Auto: ON/OFF");
}


void stateFour(){
	ST7920_Clear();
	ST7920_GraphicMode(0);

	ST7920_SendString(0,2, "State 4");
	ST7920_SendString(1,2, "Mode:");
	ST7920_SendString(2,2, "Autocross");
	//ST7920_SendString(3,0, "Auto: ON/OFF");
}
