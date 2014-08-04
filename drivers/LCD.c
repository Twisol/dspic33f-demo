#include "LCD.h"

#include <stdbool.h>
#include <stdint.h>


// Local definitions not exposed to the user
typedef enum lcd_comand_t {
  CMD_DISPLAY_CLEAR = 0x01,
  CMD_DISPLAY_TOGGLE = 0x08,
  CMD_FUNCTION_SET = 0x20,
  CMD_ENTRY_MODE_SET = 0x04,
} lcd_comand_t;

typedef enum lcd_data_length_t {
  DL_8BIT = 0x10,
  DL_4BIT = 0x00,
} lcd_data_length_t;

typedef enum lcd_display_lines_t {
  N_DUAL_LINE = 0x08,
  N_SINGLE_LINE = 0x00,
} lcd_display_lines_t;

typedef enum lcd_font_width_t {
  F_5X10_DOTS = 0x04,
  F_5X8_DOTS = 0x00,
} lcd_font_width_t;


void LCD_Init() {
  RawComm_LCD_Init();
  
  RawComm_LCD_Send(false, false, 40,
        CMD_FUNCTION_SET
      | DL_8BIT
      | N_DUAL_LINE
      | F_5X8_DOTS
  );
}

void LCD_Display_Mode(lcd_display_mode_t mode) {
  RawComm_LCD_Send(false, false, 40,
      CMD_DISPLAY_TOGGLE
    | mode
  );
}

void LCD_Display_Clear() {
  RawComm_LCD_Send(false, false, 1640,
      CMD_DISPLAY_CLEAR
  );
}

void LCD_Draw_Mode(lcd_cursor_direction_t cursor_direction, lcd_shift_display_t shift_display) {
  RawComm_LCD_Send(false, false, 40,
      CMD_ENTRY_MODE_SET
    | cursor_direction
    | shift_display
  );
}

void LCD_PutChar(char const ch) {
  RawComm_LCD_Send(true, false, 40, ch);
}

void LCD_PutString(const char* const str) {
  const char* p = str;
  while (*p != '\0') {
    LCD_PutChar(*p);
    p += 1;
  }
}
