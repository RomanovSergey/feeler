/**
  ******************************************************************************
  * Compieler:
  * https://launchpad.net/gcc-arm-embedded
  * example may be from here (do not remember):
  * http://www.hertaville.com/stm32f0discovery-part-1-linux.html
  *
  * for erasing (stlink-master)
  *    st-flash erase
  * for programming
  *    st-flash write feeler.bin 0x8000000
  ******************************************************************************
*/

#include "stm32f0xx.h"
#include "uart.h"
#include "main.h"
#include "buttons.h"
#include "magnetic.h"
#include "micro.h"
#include "displayDrv.h"
#include "sound.h"

void power(void);

GLOBAL_T g;

void init(void);

int main(void) {
	init();

	while (1) {
//		if ( g.alarm != 0 ) {
//			g.alarm--;
//			if ( g.alarm == 0 ) {
//				put_event( Ealarm );
//			}
//		}
		//magnetic();
		sound();
		buttons();
		display();
		//uart();
		power();

		while(!(SysTick->CTRL & SysTick_CTRL_COUNTFLAG_Msk));//wait until systick timer (1ms)
	}
}

void init(void) {
// At this stage the microcontroller clock setting is already configured,
// this is done through SystemInit() function which is called from startup
// file (startup_stm32f0xx.s) before to branch to application main.
// To reconfigure the default setting of SystemInit() function, refer to
// system_stm32f0xx.c file

	GPIO_InitTypeDef         GPIO_InitStructure;
	TIM_TimeBaseInitTypeDef  TIM_TimeBaseInitStruct;
	TIM_OCInitTypeDef        TIM_OCInitStruct;
	USART_InitTypeDef        USART_InitStruct;
	NVIC_InitTypeDef         NVIC_InitStruct;
	////ADC_InitTypeDef          ADC_InitStruct;

	g.alarm = 0;
	initCalib();

	//SysTick_CLKSourceConfig(SysTick_CLKSource_HCLK_Div8);
	SysTick_Config((uint32_t)48000);//запускаем системный таймер 1мс

	//================================================================
	RCC_AHBPeriphClockCmd(RCC_AHBPeriph_GPIOA, ENABLE);
	GPIO_DeInit(GPIOA);
	RCC_AHBPeriphClockCmd(RCC_AHBPeriph_GPIOB, ENABLE);
	GPIO_DeInit(GPIOB);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_TIM17, ENABLE);
	TIM_DeInit(TIM17);

	initDisplay();

	//================================================================
	// светодиодная подсветка: PA11 S1, S2; PA12 S3, S4.
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_11 | GPIO_Pin_12;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
	GPIO_Init(GPIOA, &GPIO_InitStructure);

	//======================================================================
	//Buttons input: PA8-B1, PA5-B2, PA7-B3 ================================
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_5 | GPIO_Pin_7 | GPIO_Pin_8;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN;
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
	GPIO_Init(GPIOA, &GPIO_InitStructure);

	//================================================================
	// управление питанием: PB0
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
	GPIO_Init(GPIOB, &GPIO_InitStructure);
	PWR_OFF;

	//================================================================
	// Sound buzzer: PB7
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_7;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
	GPIO_Init(GPIOB, &GPIO_InitStructure);
	GPIO_PinAFConfig(GPIOB, GPIO_PinSource7, GPIO_AF_2);
	//======================================================================
	//timer17 for PWM Sound player =========================================
	const uint16_t period = 500;
	//
	TIM_TimeBaseInitStruct.TIM_Prescaler = 48;
	TIM_TimeBaseInitStruct.TIM_CounterMode = TIM_CounterMode_Up;
	TIM_TimeBaseInitStruct.TIM_Period = period;// 2 kHz
	TIM_TimeBaseInitStruct.TIM_ClockDivision = TIM_CKD_DIV1;
	//TIM_TimeBaseInitStruct.TIM_RepetitionCounter = 0;
	TIM_TimeBaseInit(TIM17, &TIM_TimeBaseInitStruct);
	//
	TIM_SetCounter(TIM17, 0);
	//
	TIM_OCStructInit( &TIM_OCInitStruct );
	TIM_OCInitStruct.TIM_OCMode = TIM_OCMode_PWM1;
	TIM_OCInitStruct.TIM_OutputState = TIM_OutputState_Disable;
	TIM_OCInitStruct.TIM_OutputNState = TIM_OutputNState_Enable;//not used
	TIM_OCInitStruct.TIM_Pulse = period / 2;
	TIM_OCInitStruct.TIM_OCPolarity = TIM_OCPolarity_Low;
	TIM_OCInitStruct.TIM_OCNPolarity = TIM_OCNPolarity_Low;//not used
	TIM_OCInitStruct.TIM_OCIdleState = TIM_OCIdleState_Reset;//not used
	TIM_OCInitStruct.TIM_OCNIdleState = TIM_OCNIdleState_Reset;//not used
	TIM_OC1Init( TIM17, &TIM_OCInitStruct );
	//
	TIM_CtrlPWMOutputs(TIM17, DISABLE);
	TIM_Cmd(TIM17, DISABLE);

	//================================================================
	//uart1 on PA9 (tx) and PA10 (rx) pins ============================
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART1, ENABLE);
	//
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_9;//uart TX
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;
	GPIO_Init(GPIOA, &GPIO_InitStructure);
	//
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10;//uart RX
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;
	GPIO_Init(GPIOA, &GPIO_InitStructure);
	//
	GPIO_PinAFConfig(GPIOA, GPIO_PinSource9, GPIO_AF_1);
	GPIO_PinAFConfig(GPIOA, GPIO_PinSource10, GPIO_AF_1);
	//UART2 initialization
	USART_DeInit(USART1);
	USART_InitStruct.USART_BaudRate = 115200;
	USART_InitStruct.USART_WordLength = USART_WordLength_8b;
	USART_InitStruct.USART_StopBits = USART_StopBits_1;
	USART_InitStruct.USART_Parity = USART_Parity_No;
	USART_InitStruct.USART_Mode = USART_Mode_Tx | USART_Mode_Rx;
	USART_InitStruct.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
	USART_Init(USART1, &USART_InitStruct);
	//
	USART_Cmd(USART1, ENABLE);
	//
	NVIC_InitStruct.NVIC_IRQChannel = USART1_IRQn;
	NVIC_InitStruct.NVIC_IRQChannelPriority = 3;
	NVIC_InitStruct.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&NVIC_InitStruct);


	//15====================================================================
	//COMP1 ================================================================
	//COMP_DeInit();
	/*	RCC_APB2PeriphClockCmd(RCC_APB2Periph_SYSCFG, ENABLE);
	COMP_InitTypeDef COMP_InitStruct;
	COMP_InitStruct.COMP_InvertingInput = COMP_InvertingInput_IO;
	COMP_InitStruct.COMP_Output = COMP_Output_TIM3IC1;
	COMP_InitStruct.COMP_OutputPol = COMP_OutputPol_NonInverted;
	COMP_InitStruct.COMP_Hysteresis = COMP_Hysteresis_No;
	COMP_InitStruct.COMP_Mode = COMP_Mode_HighSpeed;
	COMP_Init(COMP_Selection_COMP1, &COMP_InitStruct);
	//
	COMP_Cmd(COMP_Selection_COMP1, ENABLE);

	//======================================================================
	//PA6 COMP1_OUT ========================================================
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_6;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
	GPIO_Init(GPIOA, &GPIO_InitStructure);
	//
	GPIO_PinAFConfig(GPIOA, GPIO_PinSource6, GPIO_AF_7);

	//======================================================================
	//PA0 COMP1_INM ========================================================
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AN;
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
	GPIO_Init(GPIOA, &GPIO_InitStructure);

	//======================================================================
	//PA1 COMP1_INP ========================================================
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_1;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AN;
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
	GPIO_Init(GPIOA, &GPIO_InitStructure);

	//17====================================================================
	//timer3 is 16 bit for count pulse magnetic (PA6 pin) ==================
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM3, ENABLE);
	TIM_DeInit(TIM3);
	//
	TIM_TimeBaseInitStruct.TIM_Prescaler = 1;
	TIM_TimeBaseInitStruct.TIM_CounterMode = TIM_CounterMode_Up;
	TIM_TimeBaseInitStruct.TIM_Period = 0xFFFF;//not used may be
	TIM_TimeBaseInitStruct.TIM_ClockDivision = TIM_CKD_DIV1;
	TIM_TimeBaseInitStruct.TIM_RepetitionCounter = 0;
	TIM_TimeBaseInit(TIM3, &TIM_TimeBaseInitStruct);
	//
	TIM_TIxExternalClockConfig(TIM3, TIM_TIxExternalCLK1Source_TI1, TIM_ICPolarity_Rising, 0x0);
	TIM_SelectInputTrigger(TIM3, TIM_TS_TI1FP1);
	//
	TIM_SetCounter(TIM3, 0);
	TIM_Cmd(TIM3, ENABLE);

	//======================================================================
	//timer2 is 32 bit for count time while t3 counts pulse ================
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2, ENABLE);
	TIM_DeInit(TIM2);
	//
	TIM_TimeBaseInitStruct.TIM_Prescaler = 0;
	TIM_TimeBaseInitStruct.TIM_CounterMode = TIM_CounterMode_Up;
	TIM_TimeBaseInitStruct.TIM_Period = 24000000L;
	TIM_TimeBaseInitStruct.TIM_ClockDivision = TIM_CKD_DIV1;
	TIM_TimeBaseInitStruct.TIM_RepetitionCounter = 0;
	TIM_TimeBaseInit(TIM2, &TIM_TimeBaseInitStruct);
	//
	TIM_ITConfig(TIM2, TIM_IT_Update, ENABLE);
	//
	TIM_SetCounter(TIM2, 0);
	TIM_Cmd(TIM2, ENABLE);
	//
	NVIC_InitStruct.NVIC_IRQChannel = TIM2_IRQn;
	NVIC_InitStruct.NVIC_IRQChannelPriority = 0;//main priority
	NVIC_InitStruct.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&NVIC_InitStruct);*/
}

void power(void)
{
	static const uint16_t ctim = 500;
	static int startTime = 0;
	if ( startTime < ctim ) {
		startTime++;
	} else if ( startTime == ctim ) {
		startTime++;
		PWR_ON;
		BL1_ON;
		dispPutEv( DIS_PAINT );
	} else if ( startTime > ctim ) {
		static int state = 0;
		static int timer = 0;
		//static int timoff = 0;
		timer++;
		if ( timer == 1000 ) {
			timer = 0;
			state ^= 1;
			if (state)
				BL1_OFF;
			else
				BL1_ON;
			//timoff++;
			//if ( timoff == 5 ) {
			//	PWR_OFF;
			//}
		}
	}
}

#ifdef  USE_FULL_ASSERT

/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t* file, uint32_t line)
{
	/* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
	static uint16_t tim = 0;
	static uint8_t stat = 0;
	/* Infinite loop */
	while (1) {
		tim++;
		if (tim == 100) {
			tim = 0;
			stat ^= 1;
			if (stat == 0) {
				BL1_ON;
			} else {
				BL1_OFF;
			}
		}
		while(!(SysTick->CTRL & SysTick_CTRL_COUNTFLAG_Msk));//wait until systick timer (1ms)
	}
}
#endif
