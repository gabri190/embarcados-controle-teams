/************************************************************************
* 5 semestre - Eng. da Computao - Insper
*
* 2021 - Exemplo com HC05 com RTOS
*
*/

#include <asf.h>
#include "conf_board.h"
#include <string.h>
#include <math.h>

/************************************************************************/
/* defines                                                              */
/************************************************************************/


//input analogico
#define AFEC_POT AFEC0
#define AFEC_POT_ID ID_AFEC0
#define AFEC_POT_CHANNEL 0 // Canal do pino PD30

// LED e Buzzer
#define LED_PIO      PIOC
#define LED_PIO_ID   ID_PIOC
#define LED_IDX      8
#define LED_IDX_MASK (1 << LED_IDX)

#define BUZZER_PIO      PIOA
#define BUZZER_PIO_ID   ID_PIOA
#define BUZZER_IDX      21
#define BUZZER_IDX_MASK (1 << BUZZER_IDX)


// Define Botões
#define BUT_1_PIO      PIOA
#define BUT_1_PIO_ID   ID_PIOA
#define BUT_1_IDX      2
#define BUT_1_IDX_MASK (1 << BUT_1_IDX)

#define BUT_2_PIO      PIOD
#define BUT_2_PIO_ID   ID_PIOD

#define BUT_2_IDX      28
#define BUT_2_IDX_MASK (1 << BUT_2_IDX)

#define BUT_3_PIO      PIOD
#define BUT_3_PIO_ID   ID_PIOD
#define BUT_3_IDX      22
#define BUT_3_IDX_MASK (1 << BUT_3_IDX)

#define BUT_4_PIO      PIOD
#define BUT_4_PIO_ID   ID_PIOD
#define BUT_4_IDX      25
#define BUT_4_IDX_MASK (1 << BUT_4_IDX)

#define BUT_5_PIO      PIOD
#define BUT_5_PIO_ID   ID_PIOD
#define BUT_5_IDX      26
#define BUT_5_IDX_MASK (1 << BUT_5_IDX)
// usart (bluetooth ou serial)
// Descomente para enviar dados
// pela serial debug

#define DEBUG_SERIAL

#ifdef DEBUG_SERIAL
#define USART_COM USART1
#define USART_COM_ID ID_USART1
#else
#define USART_COM USART0
#define USART_COM_ID ID_USART0
#endif

/************************************************************************/
/* RTOS                                                                 */
/************************************************************************/

#define TASK_BLUETOOTH_STACK_SIZE            (4096/sizeof(portSTACK_TYPE))
#define TASK_BLUETOOTH_STACK_PRIORITY        (tskIDLE_PRIORITY)


#define TASK_LUZ_STACK_SIZE            (4096/sizeof(portSTACK_TYPE))
#define TASK_LUZ_STACK_PRIORITY        (tskIDLE_PRIORITY)


#define TASK_BUZ_STACK_SIZE            (4096/sizeof(portSTACK_TYPE))
#define TASK_BUZ_STACK_PRIORITY        (tskIDLE_PRIORITY)


#define TASK_PROC_STACK_SIZE            (4096/sizeof(portSTACK_TYPE))
#define TASK_PROC_STACK_PRIORITY        (tskIDLE_PRIORITY)




/************************************************************************/
/* prototypes                                                           */
/************************************************************************/

extern void vApplicationStackOverflowHook(xTaskHandle *pxTask,
signed char *pcTaskName);
extern void vApplicationIdleHook(void);
extern void vApplicationTickHook(void);
extern void vApplicationMallocFailedHook(void);
extern void xPortSysTickHandler(void);

/************************************************************************/
/* constants                                                            */
/************************************************************************/

TimerHandle_t xTimer;

/** Queue for msg log send data */
QueueHandle_t xQueueLed;
QueueHandle_t xQueueBuz;
QueueHandle_t xQueueLedBluet;
QueueHandle_t xQueueBuzBluet;
QueueHandle_t xQueueAdc;
QueueHandle_t xQueueProc;

typedef struct {
	uint value;
} adcData;


/************************************************************************/
/* variaveis globais                                                    */
/************************************************************************/
static void USART1_init(void);

static void config_AFEC_pot(Afec *afec, uint32_t afec_id, uint32_t afec_channel, afec_callback_t callback);
static void configure_console(void);/************************************************************************/
/* RTOS application HOOK                                                */
/************************************************************************/

/* Called if stack overflow during execution */
extern void vApplicationStackOverflowHook(xTaskHandle *pxTask,
signed char *pcTaskName) {
	printf("stack overflow %x %s\r\n", pxTask, (portCHAR *)pcTaskName);
	/* If the parameters have been corrupted then inspect pxCurrentTCB to
	* identify which task has overflowed its stack.
	*/
	for (;;) {
	}
}

/* This function is called by FreeRTOS idle task */
extern void vApplicationIdleHook(void) {
	pmc_sleep(SAM_PM_SMODE_SLEEP_WFI);
}

/* This function is called by FreeRTOS each tick */
extern void vApplicationTickHook(void) { }

extern void vApplicationMallocFailedHook(void) {
	/* Called if a call to pvPortMalloc() fails because there is insufficient
	free memory available in the FreeRTOS heap.  pvPortMalloc() is called
	internally by FreeRTOS API functions that create tasks, queues, software
	timers, and semaphores.  The size of the FreeRTOS heap is set by the
	configTOTAL_HEAP_SIZE configuration constant in FreeRTOSConfig.h. */

	/* Force an assert. */
	configASSERT( ( volatile void * ) NULL );
}

/************************************************************************/
/* handlers / callbacks                                                 */
/************************************************************************/
void but1_callback(void) {
	int but1_flag = 1;
	
	BaseType_t xHigherPriorityTaskWoken = pdFALSE;
	//xSemaphoreGiveFromISR(xSemaphoreBut, &xHigherPriorityTaskWoken);
	xQueueSendFromISR(xQueueLed, (void *)&but1_flag, &xHigherPriorityTaskWoken);
}
void but2_callback(void) {
	int but2_flag = 2;

	BaseType_t xHigherPriorityTaskWoken = pdFALSE;
	//xSemaphoreGiveFromISR(xSemaphoreBut, &xHigherPriorityTaskWoken);
	xQueueSendFromISR(xQueueLed, (void *)&but2_flag, &xHigherPriorityTaskWoken);
}

void but3_callback(void) {
	int but3_flag = 3;

	BaseType_t xHigherPriorityTaskWoken = pdFALSE;
	//xSemaphoreGiveFromISR(xSemaphoreBut, &xHigherPriorityTaskWoken);
	xQueueSendFromISR(xQueueBuz, (void *)&but3_flag, &xHigherPriorityTaskWoken);
}

void but4_callback(void) {
	int but4_flag = 4;

	BaseType_t xHigherPriorityTaskWoken = pdFALSE;
	//xSemaphoreGiveFromISR(xSemaphoreBut, &xHigherPriorityTaskWoken);
	xQueueSendFromISR(xQueueBuz, (void *)&but4_flag, &xHigherPriorityTaskWoken);
}
void but5_callback(void) {
	int but5_flag=5;
	BaseType_t xHigherPriorityTaskWoken = pdFALSE;
	//xSemaphoreGiveFromISR(xSemaphoreBut, &xHigherPriorityTaskWoken);
	xQueueSendFromISR(xQueueLed, (void *)&but5_flag, &xHigherPriorityTaskWoken);
}

static void AFEC_pot_Callback(void) {
	adcData adc;
	adc.value = afec_channel_get_value(AFEC_POT, AFEC_POT_CHANNEL);
	BaseType_t xHigherPriorityTaskWoken = pdTRUE;
	xQueueSendFromISR(xQueueAdc, &adc, &xHigherPriorityTaskWoken);
}

/************************************************************************/
/* funcoes                                                              */
/************************************************************************/
int v[10];
int cont=0;
int media_movel(uint v[]) {
	int sum = 0;
	for (int i = 0; i < 10; i++) {
		sum += v[i];
	}
	return sum / 10;
}

void pisca_led(){
	pio_clear(LED_PIO,LED_IDX_MASK);
	delay_ms(10);	
}

void apaga_led(){
	pio_set(LED_PIO,LED_IDX_MASK);
	delay_ms(10);
}

//funcoes de toque do buzzer
void set_buzzer(){
	pio_set(BUZZER_PIO, BUZZER_IDX_MASK);
	delay_ms(10);
}
void clear_buzzer(){
	pio_clear(BUZZER_PIO, BUZZER_IDX_MASK);
	delay_ms(10);
}

//tocar o buzzer

static void config_AFEC_pot(Afec *afec, uint32_t afec_id, uint32_t afec_channel,
afec_callback_t callback) {
	/*************************************
	* Ativa e configura AFEC
	*************************************/
	/* Ativa AFEC - 0 */
	afec_enable(afec);

	/* struct de configuracao do AFEC */
	struct afec_config afec_cfg;

	/* Carrega parametros padrao */
	afec_get_config_defaults(&afec_cfg);

	/* Configura AFEC */
	afec_init(afec, &afec_cfg);

	/* Configura trigger por software */
	afec_set_trigger(afec, AFEC_TRIG_SW);

	/*** Configuracao específica do canal AFEC ***/
	struct afec_ch_config afec_ch_cfg;
	afec_ch_get_config_defaults(&afec_ch_cfg);
	afec_ch_cfg.gain = AFEC_GAINVALUE_0;
	afec_ch_set_config(afec, afec_channel, &afec_ch_cfg);

	/*
	* Calibracao:
	* Because the internal ADC offset is 0x200, it should cancel it and shift
	down to 0.
	*/
	afec_channel_set_analog_offset(afec, afec_channel, 0x200);

	/***  Configura sensor de temperatura ***/
	struct afec_temp_sensor_config afec_temp_sensor_cfg;

	afec_temp_sensor_get_config_defaults(&afec_temp_sensor_cfg);
	afec_temp_sensor_set_config(afec, &afec_temp_sensor_cfg);

	/* configura IRQ */
	afec_set_callback(afec, afec_channel, callback, 1);
	NVIC_SetPriority(afec_id, 4);
	NVIC_EnableIRQ(afec_id);
}


void io_init(void) {

	// Configura LED
	pmc_enable_periph_clk(LED_PIO_ID);
	pio_configure(LED_PIO, PIO_OUTPUT_0, LED_IDX_MASK, PIO_DEFAULT);

	// Configura BUZZER
	pmc_enable_periph_clk(BUZZER_PIO_ID);
	pio_configure(BUZZER_PIO, PIO_OUTPUT_0, BUZZER_IDX_MASK, PIO_DEFAULT);

	// Configura botao 1 da placa OLED
	pmc_enable_periph_clk(BUT_1_PIO_ID);
	pio_configure(BUT_1_PIO, PIO_INPUT, BUT_1_IDX_MASK, PIO_PULLUP | PIO_DEBOUNCE);
	pio_set_debounce_filter(BUT_1_PIO, BUT_1_IDX_MASK, 60);

    // Configura botao 2 da placa OLED
    pmc_enable_periph_clk(BUT_2_PIO_ID);
    pio_configure(BUT_2_PIO, PIO_INPUT, BUT_2_IDX_MASK, PIO_PULLUP | PIO_DEBOUNCE);
	pio_set_debounce_filter(BUT_1_PIO, BUT_1_IDX_MASK, 60);

	
	// Configura botao 3 da placa OLED
	pmc_enable_periph_clk(BUT_3_PIO_ID);
	pio_configure(BUT_3_PIO, PIO_INPUT, BUT_3_IDX_MASK, PIO_PULLUP | PIO_DEBOUNCE);
	pio_set_debounce_filter(BUT_1_PIO, BUT_1_IDX_MASK, 60);

	
	// Configura botao 4 da placa OLED
	pmc_enable_periph_clk(BUT_4_PIO_ID);
	pio_configure(BUT_4_PIO, PIO_INPUT, BUT_4_IDX_MASK, PIO_PULLUP | PIO_DEBOUNCE);
	pio_set_debounce_filter(BUT_1_PIO, BUT_1_IDX_MASK, 60);

	// Configura botao 5 da placa OLED
	pmc_enable_periph_clk(BUT_5_PIO_ID);
	pio_configure(BUT_5_PIO, PIO_INPUT, BUT_5_IDX_MASK, PIO_PULLUP | PIO_DEBOUNCE);
	pio_set_debounce_filter(BUT_1_PIO, BUT_1_IDX_MASK, 60);

	pio_handler_set(BUT_1_PIO,
	BUT_1_PIO_ID,
	BUT_1_IDX_MASK,
	PIO_IT_RISE_EDGE,
	but1_callback
	);
    
	pio_handler_set(BUT_2_PIO,
	BUT_2_PIO_ID,
	BUT_2_IDX_MASK,
	PIO_IT_RISE_EDGE,
	but2_callback
	);
	
	pio_handler_set(BUT_3_PIO,
	BUT_3_PIO_ID,
	BUT_3_IDX_MASK,
	PIO_IT_RISE_EDGE,
	but3_callback
	);
	
	pio_handler_set(BUT_4_PIO,
	BUT_4_PIO_ID,
	BUT_4_IDX_MASK,
	PIO_IT_RISE_EDGE,
	but4_callback
	);
	
	pio_handler_set(BUT_5_PIO,
	BUT_5_PIO_ID,
	BUT_5_IDX_MASK,
	PIO_IT_RISE_EDGE,
	but5_callback
	);


	pio_enable_interrupt(BUT_1_PIO, BUT_1_IDX_MASK);
	pio_get_interrupt_status(BUT_1_PIO);

	pio_enable_interrupt(BUT_2_PIO, BUT_2_IDX_MASK);
	pio_get_interrupt_status(BUT_2_PIO);

	pio_enable_interrupt(BUT_3_PIO, BUT_3_IDX_MASK);
	pio_get_interrupt_status(BUT_3_PIO);
	
	pio_enable_interrupt(BUT_4_PIO, BUT_4_IDX_MASK);
	pio_get_interrupt_status(BUT_4_PIO);
	
	pio_enable_interrupt(BUT_5_PIO, BUT_5_IDX_MASK);
	pio_get_interrupt_status(BUT_5_PIO);


	NVIC_EnableIRQ(BUT_1_PIO_ID);
	NVIC_SetPriority(BUT_1_PIO_ID, 4);
	
	NVIC_EnableIRQ(BUT_2_PIO_ID);
	NVIC_SetPriority(BUT_2_PIO_ID, 4);
	
	NVIC_EnableIRQ(BUT_3_PIO_ID);
	NVIC_SetPriority(BUT_3_PIO_ID, 4);
	
	NVIC_EnableIRQ(BUT_4_PIO_ID);
	NVIC_SetPriority(BUT_4_PIO_ID, 4);
	
	NVIC_EnableIRQ(BUT_5_PIO_ID);
	NVIC_SetPriority(BUT_5_PIO_ID, 4);
}

static void configure_console(void) {
	const usart_serial_options_t uart_serial_options = {
		.baudrate = CONF_UART_BAUDRATE,
		#if (defined CONF_UART_CHAR_LENGTH)
		.charlength = CONF_UART_CHAR_LENGTH,
		#endif
		.paritytype = CONF_UART_PARITY,
		#if (defined CONF_UART_STOP_BITS)
		.stopbits = CONF_UART_STOP_BITS,
		#endif
	};

	/* Configure console UART. */
	stdio_serial_init(CONF_UART, &uart_serial_options);

	/* Specify that stdout should not be buffered. */
	#if defined(__GNUC__)
	setbuf(stdout, NULL);
	#else
	/* Already the case in IAR's Normal DLIB default configuration: printf()
	* emits one character at a time.
	*/
	#endif
}

uint32_t usart_puts(uint8_t *pstring) {
	uint32_t i ;

	while(*(pstring + i))
	if(uart_is_tx_empty(USART_COM))
	usart_serial_putchar(USART_COM, *(pstring+i++));
}

void usart_put_string(Usart *usart, char str[]) {
	usart_serial_write_packet(usart, str, strlen(str));
}

int usart_get_string(Usart *usart, char buffer[], int bufferlen, uint timeout_ms) {
	uint timecounter = timeout_ms;
	uint32_t rx;
	uint32_t counter = 0;

	while( (timecounter > 0) && (counter < bufferlen - 1)) {
		if(usart_read(usart, &rx) == 0) {
			buffer[counter++] = rx;
		}
		else{
			timecounter--;
			vTaskDelay(1);
		}
	}
	buffer[counter] = 0x00;
	return counter;
}

void usart_send_command(Usart *usart, char buffer_rx[], int bufferlen,
char buffer_tx[], int timeout) {
	usart_put_string(usart, buffer_tx);
	usart_get_string(usart, buffer_rx, bufferlen, timeout);
}

void config_usart0(void) {
	sysclk_enable_peripheral_clock(ID_USART0);
	usart_serial_options_t config;
	config.baudrate = 9600;
	config.charlength = US_MR_CHRL_8_BIT;
	config.paritytype = US_MR_PAR_NO;
	config.stopbits = false;
	usart_serial_init(USART0, &config);
	usart_enable_tx(USART0);
	usart_enable_rx(USART0);

	// RX - PB0  TX - PB1
	pio_configure(PIOB, PIO_PERIPH_C, (1 << 0), PIO_DEFAULT);
	pio_configure(PIOB, PIO_PERIPH_C, (1 << 1), PIO_DEFAULT);
}

int hc05_init(void) {
	char buffer_rx[128];
	usart_send_command(USART_COM, buffer_rx, 1000, "AT", 100);
	vTaskDelay( 500 / portTICK_PERIOD_MS);
	usart_send_command(USART_COM, buffer_rx, 1000, "AT", 100);
	vTaskDelay( 500 / portTICK_PERIOD_MS);
	usart_send_command(USART_COM, buffer_rx, 1000, "AT+NAMEagoravai", 100);
	vTaskDelay( 500 / portTICK_PERIOD_MS);
	usart_send_command(USART_COM, buffer_rx, 1000, "AT", 100);
	vTaskDelay( 500 / portTICK_PERIOD_MS);
	usart_send_command(USART_COM, buffer_rx, 1000, "AT+PIN0000", 100);
}

/************************************************************************/
/* TASKS                                                                */
/************************************************************************/
void vTimerCallback(TimerHandle_t xTimer) {
	/* Selecina canal e inicializa conversão */
	afec_channel_enable(AFEC_POT, AFEC_POT_CHANNEL);
	afec_start_software_conversion(AFEC_POT);
}


static void task_luz(void *pvParameters) {
	int luz_flag;
	while (1) {
		if(xQueueReceive(xQueueLed,&(luz_flag),(TickType_t) 0)){
			//printf("luz(1,2,5) printando:%d\n ",luz_flag);
			pisca_led();
			xQueueSend(xQueueLedBluet,(void *)&luz_flag,(TickType_t)10);
		}
		apaga_led();
	}
}


// int t;
// //tocar o buzzer
void buzzer_test(int freq){
	double t_ms=(1E6)/(double) freq;
 	pio_set(BUZZER_PIO, BUZZER_IDX_MASK);
 	delay_us(t_ms/2);
 	pio_clear(BUZZER_PIO, BUZZER_IDX_MASK);
 	delay_us(t_ms/2);
 }

static void task_buz(void *pvParameters) {
	
	int buzzer_flag ;
	while (1) {
		if(xQueueReceive(xQueueBuz,&(buzzer_flag),(TickType_t) 0)){
			//printf("buzzer(3,4) printando:%d\n ",buzzer_flag);
			buzzer_test(100);
			xQueueSend(xQueueBuzBluet,(void *)&buzzer_flag,(TickType_t)10);
		}
		
		
	}
}


static void task_proc(void *pvParameters){
	
	// configura ADC e TC para controlar a leitura
	config_AFEC_pot(AFEC_POT, AFEC_POT_ID, AFEC_POT_CHANNEL, AFEC_pot_Callback);
	xTimer = xTimerCreate(/* Just a text name, not used by the RTOS
                        kernel. */
                        "Timer",
                        /* The timer period in ticks, must be
                        greater than 0. */
                        100,
                        /* The timers will auto-reload themselves
                        when they expire. */
                        pdTRUE,
                        /* The ID is used to store a count of the
                        number of times the timer has expired, which
                        is initialised to 0. */
                        (void *)0,
                        /* Timer callback */
                        vTimerCallback);
  xTimerStart(xTimer, 0);
  int media_antiga=0;
  // variável para receber dados da fila
  adcData adc;
  while (1){
	if (xQueueReceive(xQueueAdc, &(adc),(TickType_t) 0)) {
		//printf("ADC: %d \n", adc_msg);
		//printf("Proc: %d \n", adc);
		v[cont] = adc.value;
		cont+=1;
		if (cont > 9) {
			cont = 0;
		}
		int media = media_movel(v);
		//printf("A ADC value foi  %ld\n ",media);
		if(abs(media-media_antiga)>100){
			xQueueSend(xQueueProc, (void *)&media, 10);
			media_antiga=media;
			
		}
		/*else{
			printf("Não passa na queueProc\n");
			
		}*/
	} 
		//printf("Não chegou a msg");
	
  }
  	
}

void task_bluetooth(void) {
	printf("Task Bluetooth started \n");
	
	printf("Inicializando HC05 \n");
	config_usart0();
	hc05_init();
	// configura LEDs e Botões
	io_init();
	int led_flag,buz_flag;
	int media;
	char init='A';
	char eof = 'X';
	char vol= 'V';
	
	//handshake
	
		char rx;
		if (usart_read(USART_COM, &rx) == 0){
			if (rx == '1'){
				while(!usart_is_tx_ready(USART_COM)) {
					vTaskDelay(10 / portTICK_PERIOD_MS);
				}
				usart_write(USART_COM, '1');
				vTaskDelay(100 / portTICK_PERIOD_MS);
				for(int i=0;i<5;i++){
					pisca_led();
					apaga_led();
				}
				
				}else{
				while(!usart_is_tx_ready(USART_COM)) {
					vTaskDelay(10 / portTICK_PERIOD_MS);
				}
				usart_write(USART_COM, '0');
				vTaskDelay(100 / portTICK_PERIOD_MS);
			}
			
		}
		
		

		
		
	// Task não deve retornar.
	while(1) {
		if (xQueueReceive(xQueueProc,&media,(TickType_t) 0)){
			// envia inicio de pacote
			while(!usart_is_tx_ready(USART_COM)) {
				vTaskDelay(10 / portTICK_PERIOD_MS);
			}
			usart_write(USART_COM, init);
			
			while(!usart_is_tx_ready(USART_COM)) {
				vTaskDelay(10 / portTICK_PERIOD_MS);
			}
			usart_write(USART_COM, vol);
			//printf("volume é :%",vol);
			// envia status botão
			while(!usart_is_tx_ready(USART_COM)) {
				vTaskDelay(10 / portTICK_PERIOD_MS);
			}
			//printf("recebe a média: %d \n",media);
			usart_write(USART_COM, media);
			while(!usart_is_tx_ready(USART_COM)) {
				vTaskDelay(10 / portTICK_PERIOD_MS);
			}
			
			usart_write(USART_COM, media>>8);
			
			// envia fim de pacote
			while(!usart_is_tx_ready(USART_COM)) {
				vTaskDelay(10 / portTICK_PERIOD_MS);
			}
			usart_write(USART_COM, eof);
			//printf("volume é :%",vol);

			// dorme por 500 ms
			vTaskDelay(500 / portTICK_PERIOD_MS);
		}
		if(xQueueReceive(xQueueLedBluet,&(led_flag),(TickType_t) 0)){
			// envia status botão
			char value;
			if (led_flag == 1){
				value = '1';
			}
			if (led_flag == 2){
				value = '2';
			}
			if (led_flag == 5){
				value = '5';
			}
			
			// envia inicio de pacote
			while(!usart_is_tx_ready(USART_COM)) {
				vTaskDelay(10 / portTICK_PERIOD_MS);
			}
			usart_write(USART_COM, init);
			
			while(!usart_is_tx_ready(USART_COM)) {
				vTaskDelay(10 / portTICK_PERIOD_MS);
			}
			usart_write(USART_COM, value);
					
			// envia fim de pacote
			while(!usart_is_tx_ready(USART_COM)) {
				vTaskDelay(10 / portTICK_PERIOD_MS);
			}
			usart_write(USART_COM, eof);
			vTaskDelay(500 / portTICK_PERIOD_MS);
						
		}
		
		if(xQueueReceive(xQueueBuzBluet,&(buz_flag),(TickType_t) 0)){
			// envia status botão
			char value;
			if (buz_flag == 3){
				value = '3';
			}
			if (buz_flag == 4){
				value = '4';
			}
			
			while(!usart_is_tx_ready(USART_COM)) {
				vTaskDelay(10 / portTICK_PERIOD_MS);
			}
			usart_write(USART_COM, init);
			
			while(!usart_is_tx_ready(USART_COM)) {
				vTaskDelay(10 / portTICK_PERIOD_MS);
			}
			usart_write(USART_COM, value);
				
			// envia fim de pacote
			while(!usart_is_tx_ready(USART_COM)) {
				vTaskDelay(10 / portTICK_PERIOD_MS);
			}
			usart_write(USART_COM, eof);

			// dorme por 500 ms
			vTaskDelay(500 / portTICK_PERIOD_MS);
		}
		
	}
}

/************************************************************************/
/* main                                                                 */
/************************************************************************/

int main(void) {
	/* Initialize the SAM system */
	sysclk_init();
	board_init();

	configure_console();
	
		io_init();
		
	xQueueAdc=xQueueCreate(100,sizeof(unsigned long));
	if(xQueueAdc==NULL)
	printf("falha em criar a queue xqueueAdc \n");
	
	xQueueProc=xQueueCreate(100,sizeof(unsigned long));
	if(xQueueProc==NULL)
	printf("falha em criar a queue xqueuePROc \n"); 
	
	xQueueLed = xQueueCreate(100, sizeof(unsigned long));
	if (xQueueLed == NULL)
	printf("falha em criar a queue xQueueLed \n");
	
	xQueueBuz = xQueueCreate(100, sizeof(unsigned long));
	if (xQueueBuz == NULL)
	printf("falha em criar a queue xQueueBuz \n");
	
	xQueueLedBluet = xQueueCreate(100, sizeof(unsigned long));
	if (xQueueLedBluet == NULL)
	printf("falha em criar a queue xQueueLedBluet \n");
	
	xQueueBuzBluet = xQueueCreate(100, sizeof(unsigned long));
	if (xQueueBuzBluet == NULL)
	printf("falha em criar a queue xQueueBuzBluet \n");
		
	/* Create task to make led blink */
	if(xTaskCreate(task_bluetooth, "BLT", TASK_BLUETOOTH_STACK_SIZE, NULL,	TASK_BLUETOOTH_STACK_PRIORITY, NULL)!=pdPASS){
		    printf("Failed to create BLT task\r\n");
		}
	/* Create task to make led blink */
	if(xTaskCreate(task_proc, "Analógico", TASK_BLUETOOTH_STACK_SIZE, NULL,	TASK_BLUETOOTH_STACK_PRIORITY, NULL)!=pdPASS){
		printf("Failed to create BLT task\r\n");
	}
	
	
	/* Create task to control oled */
	if (xTaskCreate(task_luz, "Liga Led", TASK_LUZ_STACK_SIZE, NULL, TASK_LUZ_STACK_PRIORITY, NULL) != pdPASS) {
		printf("Failed to create Liga Led task\r\n");
	}
	
	if(xTaskCreate(task_buz,"Liga Buzzer",TASK_BUZ_STACK_SIZE,NULL,TASK_BUZ_STACK_PRIORITY,NULL)!=pdPASS){
		printf("Failed to create Liga Buzzer task\r\n");
	}
	/* Start the scheduler. */
	vTaskStartScheduler();

	while(1){}

	/* Will only get here if there was insufficient memory to create the idle task. */
	return 0;
}