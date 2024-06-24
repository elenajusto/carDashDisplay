/*
 * LCD12864B.h
 *
 *  Created on: Jun 13, 2024
 *      Author: elena
 */

#ifndef INC_ST7920LCD_H_
#define INC_ST7920LCD_H_

#include "stdint.h"
#include "bitmap.h"

extern uint8_t image[(128 * 64)/8];

extern const uint8_t* FrontBitmapArray[];
extern const uint8_t* rightAnimations[];

/* LCD CONTROL FUNTCION PROTOTYPES */

void SendByteSPI(uint8_t byte);

void ST7920_SendCmd (uint8_t cmd);

void ST7920_SendData (uint8_t data);

/*
 * 'row' = starting ROW for the string (from 0 to 3)
 * 'col' = starting COL for the string (from 0 to 7)
 */
void ST7920_SendString(int row, int col, char* string);

void ST7920_GraphicMode (int enable);

void ST7920_Clear();

void ST7920_DrawBitmap(const unsigned char* graphic);

void ST7920_Update(void);

void ST7920_Init (void);


/* DELAY FUNCTION PROTOTYPES */
void delay_init();

void delay_us (uint16_t delay);

void delay_ms(uint16_t delay);


/* ANIMATION PROTOTYPES */
void DrawLeftBitmapsInLoop(void);

void DrawRightAnimationsInLoop(void);

/* STATE DISPLAY PROTOTYPES */
void stateOne();
void stateTwo();
void stateThree();
void stateFour();

#endif /* INC_ST7920LCD_H_ */
