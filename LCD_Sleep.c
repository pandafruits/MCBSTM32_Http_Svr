#include "stm32f4xx_hal.h"

/*********************** Hardware specific configuration **********************/

/*--------------- Graphic LCD interface hardware definitions -----------------*/

/* LCD /CS is NE4 - Bank 4 of NOR/SRAM Bank 1~4                               */
#define LCD_BASE   (0x60000000UL | 0x0C000000UL)          // LCD base address

#define LCD_REG16  (*((volatile uint16_t *)(LCD_BASE  ))) // LCD register address
#define LCD_DAT16  (*((volatile uint16_t *)(LCD_BASE+2))) // LCD data address

/************************ Local auxiliary functions ***************************/

#define delayms HAL_Delay

/**
  \fn          void wr_cmd (uint8_t cmd)
  \brief       Write a command to the LCD controller
  \param[in]   cmd  Command to write
*/
static __inline void wr_cmd (uint8_t cmd) {
  LCD_REG16 = cmd;
}

/**
  \fn          void wr_dat (uint16_t dat)
  \brief       Write data to the LCD controller
  \param[in]   dat  Data to write
*/
static __inline void wr_dat (uint16_t dat) {
  LCD_DAT16 = dat;
}


/**
  \fn          void wr_reg (uint8_t reg, uint16_t val)
  \brief       Write a value to the LCD register
  \param[in]   reg  Register to be written
  \param[in]   val  Value to write to the register
*/
static __inline void LCD_CtrlWrite_ILI9320 (uint8_t reg, uint16_t val) {
  wr_cmd(reg);
  wr_dat(val);
}

void LCD_EnterSleep(void)
{
	// Turn off backlight
	HAL_GPIO_WritePin(GPIOC, GPIO_PIN_7, GPIO_PIN_RESET);
	
	LCD_CtrlWrite_ILI9320(0x0007, 0x0000); // display OFF
	//************* Power OFF sequence **************//
	LCD_CtrlWrite_ILI9320(0x0010, 0x0000); // SAP, BT[3:0], APE, AP, DSTB, SLP
	LCD_CtrlWrite_ILI9320(0x0011, 0x0000); // DC1[2:0], DC0[2:0], VC[2:0]
	LCD_CtrlWrite_ILI9320(0x0012, 0x0000); // VREG1OUT voltage
	LCD_CtrlWrite_ILI9320(0x0013, 0x0000); // VDV[4:0] for VCOM amplitude
	delayms(200); // Dis-charge capacitor power voltage
	LCD_CtrlWrite_ILI9320(0x0010, 0x0002); // SAP, BT[3:0], APE, AP, DSTB, SLP
}

void LCD_ExitSleep(void)
{
	//*************Power On sequence ******************//
	LCD_CtrlWrite_ILI9320(0x0010, 0x0000); // SAP, BT[3:0], AP, DSTB, SLP
	LCD_CtrlWrite_ILI9320(0x0011, 0x0000); // DC1[2:0], DC0[2:0], VC[2:0]
	LCD_CtrlWrite_ILI9320(0x0012, 0x0000); // VREG1OUT voltage
	LCD_CtrlWrite_ILI9320(0x0013, 0x0000); // VDV[4:0] for VCOM amplitude
	delayms(200); // Dis-charge capacitor power voltage
	LCD_CtrlWrite_ILI9320(0x0010, 0x17B0); // SAP, BT[3:0], AP, DSTB, SLP, STB
	LCD_CtrlWrite_ILI9320(0x0011, 0x0037); // R11h=0x0031 at VCI=3.3V DC1[2:0], DC0[2:0], VC[2:0]
	delayms(50); // Delay 50ms
	LCD_CtrlWrite_ILI9320(0x0012, 0x013C); // R12h=0x0138 at VCI=3.3V VREG1OUT voltage
	delayms(50); // Delay 50ms
	LCD_CtrlWrite_ILI9320(0x0013, 0x1C00); // R13h=0x1800 at VCI=3.3V VDV[4:0] for VCOM amplitude
	LCD_CtrlWrite_ILI9320(0x0029, 0x000E); // R29h=0x0008 at VCI=3.3V VCM[4:0] for VCOMH
	delayms(50);
	LCD_CtrlWrite_ILI9320(0x0007, 0x0173); // 262K color and display ON
	
	delayms(200);
	
	// Turn on backlight
	HAL_GPIO_WritePin(GPIOC, GPIO_PIN_7, GPIO_PIN_SET);
}
