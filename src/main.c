/**
  ******************************************************************************
  * Compieler:
  * https://launchpad.net/gcc-arm-embedded
  * example may be from here (do not remember):
  * http://www.hertaville.com/stm32f0discovery-part-1-linux.html
  *
  * for erasing
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

GLOBAL_T g;

void init(void);

int main(void) {
	init();

	while (1) {
		if ( g.alarm != 0 ) {
			g.alarm--;
			if ( g.alarm == 0 ) {
				put_event( Ealarm );
			}
		}
		//magnetic();
		buttons();
		display();
		//uart();
		{
			static const uint16_t ctim = 200;
			static int count = 0;
			if ( count < ctim ) {
				count++;
			} else if ( count == ctim ) {
				count++;
				PWR_ON;
				BL1_ON;
			} else if ( count > ctim ) {
				static int state = 0;
				static int timer = 0;
				timer++;
				if ( timer == 1000 ) {
					timer = 0;
					state ^= 1;
					if (state)
						BL1_OFF;
					else
						BL1_ON;
				}
			}
		}
//		{
//			static int state = 0;
//			state ^= 1;
//			if (state)
//				BEEP_ON;
//			else
//				BEEP_OFF;
//		}

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
	//NVIC_InitTypeDef         NVIC_InitStruct;
	//USART_InitTypeDef        USART_InitStruct;
	TIM_TimeBaseInitTypeDef  TIM_TimeBaseInitStruct;
	////ADC_InitTypeDef          ADC_InitStruct;

	g.alarm = 0;
	initCalib();
	put_event( Erepaint );

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
	GPIO_PinAFConfig(GPIOB, GPIO_PinSource7, GPIO_AF_5);
	//======================================================================
	//timer17 for PWM Sound player =========================================
	const uint16_t period = 48000;
	//
	TIM_TimeBaseInitStruct.TIM_Prescaler = 0;
	TIM_TimeBaseInitStruct.TIM_CounterMode = TIM_CounterMode_Up;
	TIM_TimeBaseInitStruct.TIM_Period = period;// 1 kHz
	TIM_TimeBaseInitStruct.TIM_ClockDivision = TIM_CKD_DIV1;
	//TIM_TimeBaseInitStruct.TIM_RepetitionCounter = 0;
	TIM_TimeBaseInit(TIM17, &TIM_TimeBaseInitStruct);
	//
	TIM_SetCounter(TIM17, 0);
	//
	TIM_OCInitTypeDef TIM_OCInitStruct;
	//TIM_OCStructInit( &TIM_OCInitStruct );
	TIM_OCInitStruct.TIM_OCMode = TIM_OCMode_PWM1;
	TIM_OCInitStruct.TIM_OutputState = TIM_OutputState_Enable;
	TIM_OCInitStruct.TIM_OutputNState = TIM_OutputNState_Disable;
	TIM_OCInitStruct.TIM_Pulse = period / 2;
	TIM_OCInitStruct.TIM_OCPolarity = TIM_OCPolarity_Low;
	TIM_OCInitStruct.TIM_OCNPolarity = TIM_OCNPolarity_Low;//not used
	TIM_OCInitStruct.TIM_OCIdleState = TIM_OCIdleState_Reset;//not used
	TIM_OCInitStruct.TIM_OCNIdleState = TIM_OCNIdleState_Reset;//not used
	TIM_OC1Init( TIM17, &TIM_OCInitStruct );
	//
	TIM_CtrlPWMOutputs(TIM17, ENABLE);
	TIM_Cmd(TIM17, ENABLE);


	/*// GPIOC Periph clock enable =======================================
	RCC_AHBPeriphClockCmd(RCC_AHBPeriph_GPIOC, ENABLE);
	*/

	//================================================================
	//uart2 on PA2 (tx) and PA3 (rx) pins ============================
	/*
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
	USART_InitStruct.USART_BaudRate = 115200;
	USART_InitStruct.USART_WordLength = USART_WordLength_8b;
	USART_InitStruct.USART_StopBits = USART_StopBits_1;
	USART_InitStruct.USART_Parity = USART_Parity_No;
	USART_InitStruct.USART_Mode = USART_Mode_Tx | USART_Mode_Rx;
	USART_InitStruct.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
	USART_Init(USART2, &USART_InitStruct);
	//
	USART_Cmd(USART2, ENABLE);
	//
	NVIC_InitStruct.NVIC_IRQChannel = USART2_IRQn;
	NVIC_InitStruct.NVIC_IRQChannelPriority = 3;
	NVIC_InitStruct.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&NVIC_InitStruct);

	//15====================================================================
	//COMP1 ================================================================
	//COMP_DeInit();
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_SYSCFG, ENABLE);
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



//для кругового буфера событий
#define LEN_BITS   3
#define LEN_BUF    (1<<LEN_BITS) // 8 или 2^3 или (1<<3)
#define LEN_MASK   (LEN_BUF-1)   // bits: 0000 0111
static uint8_t bufEv[LEN_BUF] = {0};
static uint8_t tail = 0;
static uint8_t head = 0;

/*
 * возвращает 1 если в кольцевом буфере есть свободное место для элемента, иначе 0
 */
static int has_free(void) {
	if ( ((tail + 1) & LEN_MASK) == head ) {
		return 0;//свободного места нет
	}
	return 1;//есть свободное место
}

/*
 * помещает событие в круговой буфер
 * return 1 - успешно; 0 - нет места в буфере
 */
int put_event(uint8_t event) {
	if (event == 0) {
		return 1;//событие с нулевым кодом пусть не будет для удобства
	}
	if (has_free()) {
		bufEv[head] = event;
		head = (1 + head) & LEN_MASK;//инкремент кругового индекса
		return 1;
	} else {
		return 0;//нет места в буфере
	}
}

/*
 *  извлекает событие из кругового буфера
 *  если 0 - нет событий
 */
uint8_t get_event(void) {
	uint8_t event = 0;
	if (head != tail) {//если в буфере есть данные
		event = bufEv[tail];
		tail = (1 + tail) & LEN_MASK;//инкремент кругового индекса
	}
	return event;
}

