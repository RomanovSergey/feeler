/**
  ******************************************************************************
  * Компилятор:
  * https://launchpad.net/gcc-arm-embedded
  * примерчик вроде отсюда:
  * http://www.hertaville.com/stm32f0discovery-part-1-linux.html
  ******************************************************************************
*/

#include "stm32f0xx.h"
#include "uart.h"
#include "adc.h"
#include "main.h"
#include "../inc/buttons.h"


GLOBAL g;

void init(void);

int main(void) {

	init();

	while (1) {
		adc();
		buttons();
		uart();
		while(!(SysTick->CTRL & SysTick_CTRL_COUNTFLAG_Msk));//wait until systick timer (1ms)
	}
}

void init(void) {
// At this stage the microcontroller clock setting is already configured,
// this is done through SystemInit() function which is called from startup
// file (startup_stm32f0xx.s) before to branch to application main.
// To reconfigure the default setting of SystemInit() function, refer to
// system_stm32f0xx.c file


	NVIC_InitTypeDef NVIC_InitStruct;

	g.ADC_calib = 0;
	g.ADC_done  = 0;
	g.ADC_value = 0;

	//SysTick_CLKSourceConfig(SysTick_CLKSource_HCLK_Div8);
	SysTick_Config((uint32_t)48000);//запускаем системный таймер 1мс

	// GPIOC Periph clock enable =======================================
	RCC_AHBPeriphClockCmd(RCC_AHBPeriph_GPIOC, ENABLE);
	GPIO_InitTypeDef  GPIO_InitStructure;
	// Configure PC8 and PC9 in output pushpull mode
	GPIO_DeInit(GPIOC);
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_8 | GPIO_Pin_9;//лампочки на discovery плате
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
	GPIO_Init(GPIOC, &GPIO_InitStructure);

	//================================================================
	//uart2 on PA2 (tx) and PA3 (rx) pins ============================
	RCC_AHBPeriphClockCmd(RCC_AHBPeriph_GPIOA, ENABLE);
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART2, ENABLE);
	//
	GPIO_DeInit(GPIOA);
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_2;//uart TX
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;
	GPIO_Init(GPIOA, &GPIO_InitStructure);
	//
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_3;//uart RX
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;
	GPIO_Init(GPIOA, &GPIO_InitStructure);
	//
	GPIO_PinAFConfig(GPIOA, GPIO_PinSource2, GPIO_AF_1);
	GPIO_PinAFConfig(GPIOA, GPIO_PinSource3, GPIO_AF_1);
	//UART2 initialization
	USART_DeInit(USART2);
	USART_InitTypeDef USART_InitStruct;
	USART_InitStruct.USART_BaudRate = 115200;
	USART_InitStruct.USART_WordLength = USART_WordLength_8b;
	USART_InitStruct.USART_StopBits = USART_StopBits_1;
	USART_InitStruct.USART_Parity = USART_Parity_No;
	USART_InitStruct.USART_Mode = USART_Mode_Tx | USART_Mode_Rx;
	USART_InitStruct.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
	USART_Init(USART2, &USART_InitStruct);
	//
	//USART_ITConfig(USART2, USART_IT_TXE, ENABLE);
	USART_ITConfig(USART2, USART_IT_RXNE, ENABLE);
	USART_Cmd(USART2, ENABLE);
	//
	NVIC_InitStruct.NVIC_IRQChannel = USART2_IRQn;
	NVIC_InitStruct.NVIC_IRQChannelPriority = 3;
	NVIC_InitStruct.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&NVIC_InitStruct);

	//==================================================================
	//ADC pin config ===================================================
	RCC_AHBPeriphClockCmd(RCC_AHBPeriph_GPIOB, ENABLE);
	GPIO_DeInit(GPIOB);
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AN;
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
	GPIO_Init(GPIOB, &GPIO_InitStructure);
	//
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_ADC1, ENABLE);
	ADC_DeInit(ADC1);
	ADC_InitTypeDef ADC_InitStruct;
	ADC_InitStruct.ADC_Resolution = ADC_Resolution_12b;
	ADC_InitStruct.ADC_ContinuousConvMode = ENABLE;
	ADC_InitStruct.ADC_ExternalTrigConvEdge = ADC_ExternalTrigConvEdge_None;
	ADC_InitStruct.ADC_ExternalTrigConv = ADC_ExternalTrigConv_T2_TRGO;
	ADC_InitStruct.ADC_DataAlign = ADC_DataAlign_Right;
	ADC_InitStruct.ADC_ScanDirection = ADC_ScanDirection_Upward;
	ADC_Init(ADC1, &ADC_InitStruct);
	//
	ADC_JitterCmd(ADC1, ADC_JitterOff_PCLKDiv4, ENABLE);
	//
	g.ADC_calib = ADC_GetCalibrationFactor(ADC1);
	//
	ADC_Cmd(ADC1, ENABLE);
	while ( RESET == ADC_GetFlagStatus(ADC1, ADC_FLAG_ADRDY) );
	//
	ADC_ChannelConfig(ADC1, ADC_Channel_8, ADC_SampleTime_1_5Cycles);
	ADC_ClearFlag(ADC1, ADC_FLAG_EOC);
	ADC_ClearFlag(ADC1, ADC_FLAG_EOSMP);

	//======================================================================
	//drive pin PA8 for magnetic switch ====================================
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_8;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
	GPIO_Init(GPIOA, &GPIO_InitStructure);
	GPIO_WriteBit(GPIOA, GPIO_Pin_8, Bit_RESET);

	//======================================================================
	//input pin PA0 B1 Button ==============================================
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN;
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
	GPIO_Init(GPIOA, &GPIO_InitStructure);

	//======================================================================
	//timer for limit magnetic shwitch on ==================================
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2, ENABLE);
	TIM_DeInit(TIM2);
	//
	TIM_TimeBaseInitTypeDef TIM_TimeBaseInitStruct;
	TIM_TimeBaseInitStruct.TIM_Prescaler = 1;
	TIM_TimeBaseInitStruct.TIM_CounterMode = TIM_CounterMode_Up;
	TIM_TimeBaseInitStruct.TIM_Period = 1000;//not used
	TIM_TimeBaseInitStruct.TIM_ClockDivision = TIM_CKD_DIV1;
	TIM_TimeBaseInitStruct.TIM_RepetitionCounter = 0;//don't care
	TIM_TimeBaseInit(TIM2, &TIM_TimeBaseInitStruct);
	//
	TIM_ClearFlag(TIM2, TIM_FLAG_Update);
	TIM_ITConfig(TIM2, TIM_IT_Update, ENABLE);
	//
	TIM_SetCounter(TIM2, 0);
	TIM_ClearFlag(TIM2, TIM_FLAG_CC1);
	TIM_ITConfig(TIM2, TIM_IT_CC1, ENABLE);
	TIM_ClearFlag(TIM2, TIM_FLAG_CC2);
	TIM_ClearFlag(TIM2, TIM_FLAG_CC3);
	//
	TIM_Cmd(TIM2, DISABLE);
	//
	NVIC_InitStruct.NVIC_IRQChannel = TIM2_IRQn;
	NVIC_InitStruct.NVIC_IRQChannelPriority = 0;
	NVIC_InitStruct.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&NVIC_InitStruct);
	//======================================================================
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
		if (tim == 200) {
			tim = 0;
			if (stat == 0) {
				stat = 1;
				GPIO_SetBits( GPIOC, GPIO_Pin_8 );
			} else {
				stat = 0;
				GPIO_ResetBits( GPIOC, GPIO_Pin_8 );
			}
		}
		while(!(SysTick->CTRL & SysTick_CTRL_COUNTFLAG_Msk));//wait until systick timer (1ms)
	}
}
#endif

