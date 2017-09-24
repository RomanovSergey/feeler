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
#include "pwr.h"
#include "adc.h"

void init(void);

int main(void) {
	DISRESET_LOW;//reset display
	init();

	urtPrint("Start\n");
	micro_initCalib();
	mgPutEv( MG_OFF );
	while (1) {
		magnetic();
		//adc();
		//sound();
		display();
		buttons();
		uart();
		power();
//		PWR_EnterSleepMode(PWR_SLEEPEntry_WFE);
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
	ADC_InitTypeDef          ADC_InitStruct;

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
	BEEP_OFF;
	GPIO_PinAFConfig(GPIOB, GPIO_PinSource7, GPIO_AF_2);

	//======================================================================
	//timer17 for PWM Sound player =========================================
	const uint16_t sndperiod = 500;
	//
	TIM_TimeBaseInitStruct.TIM_Prescaler = 48;
	TIM_TimeBaseInitStruct.TIM_CounterMode = TIM_CounterMode_Up;
	TIM_TimeBaseInitStruct.TIM_Period = sndperiod;// 2 kHz
	TIM_TimeBaseInitStruct.TIM_ClockDivision = TIM_CKD_DIV1;
	//TIM_TimeBaseInitStruct.TIM_RepetitionCounter = 0;
	TIM_TimeBaseInit(TIM17, &TIM_TimeBaseInitStruct);
	//
	TIM_SetCounter(TIM17, 0);
	//
	TIM_OCStructInit( &TIM_OCInitStruct );
	TIM_OCInitStruct.TIM_OCMode = TIM_OCMode_PWM1;
	TIM_OCInitStruct.TIM_OutputState = TIM_OutputState_Disable;
	TIM_OCInitStruct.TIM_OutputNState = TIM_OutputNState_Enable;
	TIM_OCInitStruct.TIM_Pulse = sndperiod / 2;
	TIM_OCInitStruct.TIM_OCPolarity = TIM_OCPolarity_Low;
	TIM_OCInitStruct.TIM_OCNPolarity = TIM_OCNPolarity_Low;
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
	USART_InitStruct.USART_Mode = USART_Mode_Tx;// | USART_Mode_Rx;
	USART_InitStruct.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
	USART_Init(USART1, &USART_InitStruct);
	//
	USART_Cmd(USART1, ENABLE);
	//
	NVIC_InitStruct.NVIC_IRQChannel = USART1_IRQn;
	NVIC_InitStruct.NVIC_IRQChannelPriority = 3;
	NVIC_InitStruct.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&NVIC_InitStruct);

	//======================================================================
	//ADC for battary PA3, LC PA0 ==========================================
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_3 | GPIO_Pin_0;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AN;
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
	GPIO_Init(GPIOA, &GPIO_InitStructure);

	ADC_DeInit(ADC1);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_ADC1, ENABLE);
	ADC_GetCalibrationFactor( ADC1 );
	//adcSaveCalibData(  );
	ADC_VrefintCmd( ENABLE );
	//ADC_TempSensorCmd( ENABLE );
	ADC_Cmd(ADC1, ENABLE);
	while ( RESET == ADC_GetFlagStatus(ADC1, ADC_FLAG_ADRDY) );
	ADC_JitterCmd( ADC1, ADC_JitterOff_PCLKDiv4, ENABLE );

	ADC_InitStruct.ADC_Resolution = ADC_Resolution_12b;
	ADC_InitStruct.ADC_ContinuousConvMode = DISABLE;
	ADC_InitStruct.ADC_ExternalTrigConvEdge = ADC_ExternalTrigConvEdge_None;//ADC_ExternalTrigConvEdge_Falling;
	ADC_InitStruct.ADC_ExternalTrigConv = ADC_ExternalTrigConv_T3_TRGO;
	ADC_InitStruct.ADC_DataAlign = ADC_DataAlign_Right;
	ADC_InitStruct.ADC_ScanDirection = ADC_ScanDirection_Upward;
	ADC_Init( ADC1, &ADC_InitStruct );

	// Channel_3 - battary, Channel_16 - temperature (4 mks), Channel_17 - vrefint (4 mks)
	// Channel_0 - LC
	ADC1->CHSELR = 0;
	ADC1->CHSELR |= ADC_Channel_0; // ADC_Channel_3 | ADC_Channel_16 | ADC_Channel_17;
	ADC1->SMPR = ADC_SampleTime_55_5Cycles;//ADC_SampleTime_71_5Cycles; //  (4 mks)

	ADC_ClearFlag(ADC1, ADC_FLAG_EOC);
	ADC_ITConfig( ADC1, ADC_IT_EOC, ENABLE );

	NVIC_InitStruct.NVIC_IRQChannel = ADC1_COMP_IRQn;
	NVIC_InitStruct.NVIC_IRQChannelPriority = 0;
	NVIC_InitStruct.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&NVIC_InitStruct);


	//PA6 LC resonator ==============================================
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_6;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
	GPIO_Init(GPIOA, &GPIO_InitStructure);
	GPIO_ResetBits(GPIOA,GPIO_Pin_6);
	GPIO_PinAFConfig(GPIOA, GPIO_PinSource6, GPIO_AF_1); // Tim3

	//timer3 is 16 bit for LC frequency (PA6 pin) ==================
	const uint16_t lcperiod = 480; // 100 kHz
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM3, ENABLE);
	TIM_DeInit(TIM3);
	TIM_TimeBaseInitStruct.TIM_Prescaler = 0;
	TIM_TimeBaseInitStruct.TIM_CounterMode = TIM_CounterMode_Up;
	TIM_TimeBaseInitStruct.TIM_Period = lcperiod;
	TIM_TimeBaseInitStruct.TIM_ClockDivision = TIM_CKD_DIV1;
	TIM_TimeBaseInitStruct.TIM_RepetitionCounter = 0;
	TIM_TimeBaseInit(TIM3, &TIM_TimeBaseInitStruct);
	//
	TIM_SetCounter(TIM3, 0);
	//
	TIM_OCStructInit( &TIM_OCInitStruct ); // todo: delete
	TIM_OCInitStruct.TIM_OCMode = TIM_OCMode_PWM1;
	TIM_OCInitStruct.TIM_OutputState = TIM_OutputState_Enable;
	TIM_OCInitStruct.TIM_OutputNState = TIM_OutputNState_Disable;
	TIM_OCInitStruct.TIM_Pulse = lcperiod / 2;
	TIM_OCInitStruct.TIM_OCPolarity = TIM_OCPolarity_High;
	TIM_OCInitStruct.TIM_OCNPolarity = TIM_OCNPolarity_Low;
	TIM_OCInitStruct.TIM_OCIdleState = TIM_OCIdleState_Reset;//not used
	TIM_OCInitStruct.TIM_OCNIdleState = TIM_OCNIdleState_Reset;//not used
	TIM_OC1Init( TIM3, &TIM_OCInitStruct );
	//
	TIM_OCInitStruct.TIM_OCMode = TIM_OCMode_PWM1;
	TIM_OCInitStruct.TIM_OutputState = TIM_OutputState_Disable;
	TIM_OCInitStruct.TIM_OutputNState = TIM_OutputNState_Disable;
	TIM_OCInitStruct.TIM_Pulse = lcperiod / 4;
	TIM_OCInitStruct.TIM_OCPolarity = TIM_OCPolarity_Low;
	TIM_OCInitStruct.TIM_OCNPolarity = TIM_OCNPolarity_Low;
	TIM_OCInitStruct.TIM_OCIdleState = TIM_OCIdleState_Reset;//not used
	TIM_OCInitStruct.TIM_OCNIdleState = TIM_OCNIdleState_Reset;//not used
	TIM_OC2Init( TIM3, &TIM_OCInitStruct );
	//
	//TIM_SelectOutputTrigger( TIM3, TIM_TRGOSource_OC1Ref );
	//TIM_SelectMasterSlaveMode( TIM3, TIM_MasterSlaveMode_Enable );
	TIM_Cmd(TIM3, ENABLE);
	TIM_ITConfig( TIM3, TIM_IT_CC2, ENABLE );
	//
	NVIC_InitStruct.NVIC_IRQChannel = TIM3_IRQn;
	NVIC_InitStruct.NVIC_IRQChannelPriority = 0;
	NVIC_InitStruct.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&NVIC_InitStruct);
}

#ifdef  USE_FULL_ASSERT
// USE_FULL_ASSERT in stm32f0xx_conf.h
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
