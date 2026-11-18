/*
  ******************************************************************************
  HIER BITTE NAMEN + MATRIKELNUMMER EINTRAGEN
	
	In diesem Projekt gilt:
	*=============================================================================
  *        SYSCLK(Hz)                             | 64000000
  *-----------------------------------------------------------------------------
  *        AHB Prescaler                          | 1
  *-----------------------------------------------------------------------------
  *        APB2 Prescaler                         | 2
  *-----------------------------------------------------------------------------
  *        APB1 Prescaler                         | 2
  *=============================================================================
  ******************************************************************************
*/

/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include <stdio.h>
#include <string.h>



/** @addtogroup STM32F3xx_LL_Examples
  * @{
  */

/** @addtogroup Templates_LL
  * @{
  */

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
uint8_t txData[12] = {0};
uint8_t rxData[12] = {0};

volatile uint8_t txIndex = 0;
volatile uint8_t rxIndex = 0;
uint16_t irX = 0;
uint16_t irY = 0;
uint8_t posX = 0xFF;
uint8_t posY = 0xFF;

volatile int16_t servoX = 0;        //stellwert für die Servos
volatile int16_t servoY = 0;
volatile uint8_t servoPending = 0;      //merkt sich das neue Servodaten anstehen
volatile uint8_t spiSelect = 0;      //ob SPI gerade mit Servo oder LED/IR Kommuniziert (0 LEDs 1 Servo)
uint16_t servoData[2];      //enthält die zwei 16 Bit Wörter für Servo

volatile uint16_t adcWerte[4] = {0};        //Speicher für die ADC Werte
volatile uint16_t filter_adcWerte[4] = {0};
volatile uint8_t adcIndex = 0;
volatile uint32_t filterMedianOverAllChannels = 0;

volatile uint8_t gewicht;
volatile uint8_t altes_gewicht;


typedef struct
{
    uint8_t byte;
    uint8_t maske;
} IR_Sensor;

// X-Achse: von links nach rechts
const IR_Sensor mapX[16] = {
    {10, 0x80}, // X = 0
    { 4, 0x01}, // X = 1
    {10, 0x40}, // X = 2
    { 4, 0x02}, // X = 3
    {10, 0x20}, // X = 4
    { 4, 0x04}, // X = 5
    {10, 0x10}, // X = 6
    { 4, 0x08}, // X = 7
    {10, 0x08}, // X = 8
    { 4, 0x10}, // X = 9
    {10, 0x04}, // X = 10
    { 4, 0x20}, // X = 11
    {10, 0x02}, // X = 12
    { 4, 0x40}, // X = 13
    {10, 0x01}, // X = 14
    { 4, 0x80}  // X = 15
};

// Y-Achse: von unten nach oben
const IR_Sensor mapY[16] = {
    {7, 0x80}, // Y = 0
    {1, 0x01}, // Y = 1
    {7, 0x40}, // Y = 2
    {1, 0x02}, // Y = 3
    {7, 0x20}, // Y = 4
    {1, 0x04}, // Y = 5
    {7, 0x10}, // Y = 6
    {1, 0x08}, // Y = 7
    {7, 0x08}, // Y = 8
    {1, 0x10}, // Y = 9
    {7, 0x04}, // Y = 10
    {1, 0x20}, // Y = 11
    {7, 0x02}, // Y = 12
    {1, 0x40}, // Y = 13
    {7, 0x01}, // Y = 14
    {1, 0x80}  // Y = 15
};

volatile uint8_t matrix[16][16] = {0};




/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);

void GPIO_Init();
void SPI_Init();
void TIM2_Init();
void SPI_Start(uint8_t servor);
void USART2_Init(void);
void IR_Auswerten(void);
void LED_Fadenkreuz();
void LED_Set(uint8_t seite, uint8_t position);
uint8_t IR_Position(const IR_Sensor map[16]);
void Matrix_Update(void);
void ADC_Init(void);


/* Private functions ---------------------------------------------------------*/

/**
  * @brief  Main program
  * @param  None
  * @retval None
  */
int main(void)
{
  /* Configure the system clock to 64 MHz */
  SystemClock_Config();
  
  /* Add your application code here */
    GPIO_Init();
    SPI_Init();
		USART2_Init();
		SPI_Start(0);
		ADC_Init();
		LL_ADC_REG_StartConversion(ADC2);
	
	
	
  
  
  /* Infinite loop */
  while (1)
  {
  }
}


/* Custom functions -----------------------------------------------*/



void ADC_Init(void)
{
    LL_GPIO_InitTypeDef gpio;

    LL_AHB1_GRP1_EnableClock(LL_AHB1_GRP1_PERIPH_GPIOC |
                             LL_AHB1_GRP1_PERIPH_ADC12);

    LL_GPIO_StructInit(&gpio);                          //GPIO für analog input konfigurieren
    gpio.Pin = LL_GPIO_PIN_2 | LL_GPIO_PIN_3 |
               LL_GPIO_PIN_4 | LL_GPIO_PIN_5;
    gpio.Mode = LL_GPIO_MODE_ANALOG;
    gpio.Pull = LL_GPIO_PULL_NO;
    LL_GPIO_Init(GPIOC, &gpio);

    LL_ADC_SetCommonClock(__LL_ADC_COMMON_INSTANCE(ADC2),
                          LL_ADC_CLOCK_SYNC_PCLK_DIV2);     //Takt für ADC wird aus Peripherietakt genommen -> 32Mhz/2 = 16Mhz

    LL_ADC_REG_SetSequencerLength(ADC2, LL_ADC_REG_SEQ_SCAN_ENABLE_4RANKS);     //festlegen das eine Messreihe aus 4 Wandlungen bestehet 
    LL_ADC_REG_SetSequencerRanks(ADC2, LL_ADC_REG_RANK_1, LL_ADC_CHANNEL_9);    //hier bestimmen welcher Kanal welchen Rank bekommt dann wird sequentiell abgearbeitet
    LL_ADC_REG_SetSequencerRanks(ADC2, LL_ADC_REG_RANK_2, LL_ADC_CHANNEL_8);
    LL_ADC_REG_SetSequencerRanks(ADC2, LL_ADC_REG_RANK_3, LL_ADC_CHANNEL_11);
    LL_ADC_REG_SetSequencerRanks(ADC2, LL_ADC_REG_RANK_4, LL_ADC_CHANNEL_5);
	
		LL_ADC_REG_SetContinuousMode(ADC2, LL_ADC_REG_CONV_CONTINUOUS);

    LL_ADC_SetChannelSamplingTime(ADC2, LL_ADC_CHANNEL_9, LL_ADC_SAMPLINGTIME_181CYCLES_5);  //Abtastzeit festlegen hier 61,5 Taktzyklen des ADC für einen Messwert ; 61,5/16Mhz=3,84µs
    LL_ADC_SetChannelSamplingTime(ADC2, LL_ADC_CHANNEL_8, LL_ADC_SAMPLINGTIME_181CYCLES_5);  //61,5 Zyklen weil dadurch der Kondensator sicher aufgeladen ist
    LL_ADC_SetChannelSamplingTime(ADC2, LL_ADC_CHANNEL_11, LL_ADC_SAMPLINGTIME_181CYCLES_5);
    LL_ADC_SetChannelSamplingTime(ADC2, LL_ADC_CHANNEL_5, LL_ADC_SAMPLINGTIME_181CYCLES_5);

    LL_ADC_EnableInternalRegulator(ADC2);       //internen Spannungsregulator für eine stabile Versorgungspannung 
    LL_mDelay(1);

    LL_ADC_StartCalibration(ADC2, LL_ADC_SINGLE_ENDED);     //interne Spannungsunebenheiten kalibrieren / Misst Spannung gegenüber Masse
    while (LL_ADC_IsCalibrationOnGoing(ADC2));              //braucht dafür zeit ist auch so im Reference Manual vorgegeben

    LL_ADC_Enable(ADC2);                                    //ADC einschalten
    while (!LL_ADC_IsActiveFlag_ADRDY(ADC2));               //warten bis eingeschaltet

    LL_ADC_EnableIT_EOC(ADC2);                              //Nach einer Wandlung Interrupt aktivieren
    NVIC_EnableIRQ(ADC1_2_IRQn);
}



void GPIO_Init(void)
{
    LL_GPIO_InitTypeDef GPIO_InitStructure;

    LL_AHB1_GRP1_EnableClock(LL_AHB1_GRP1_PERIPH_GPIOA | LL_AHB1_GRP1_PERIPH_GPIOB);

    LL_GPIO_StructInit(&GPIO_InitStructure);
    
    //Konfig der Pins PB3 (SCLK), PB4 (MISO) und PB5 (MOSI) fuer SPI1
    GPIO_InitStructure.Pin = LL_GPIO_PIN_3 | LL_GPIO_PIN_4 | LL_GPIO_PIN_5; 
    GPIO_InitStructure.Mode = LL_GPIO_MODE_ALTERNATE;
    GPIO_InitStructure.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
    GPIO_InitStructure.Pull = LL_GPIO_PULL_NO;
    GPIO_InitStructure.Speed = LL_GPIO_SPEED_FREQ_HIGH;
    GPIO_InitStructure.Alternate = LL_GPIO_AF_5;

    LL_GPIO_Init(GPIOB, &GPIO_InitStructure);


    /*
    Konfig der Pins PA4, PA12 und PA15 als Ausgaenge
        PA4 bei 
            '0' mit Servohost für die Datenübertragung vorbereiten
            '1' bei der steigenden Flanke übernimmt der Servohost die übertragenen Daten
        PA12 bei
            '0' werden die aktuellen Zustände der IR-Empfangsdioden in die Schieberegister geladen
            '1' können die Daten seriell durch die Register geschoben werden
        PA15 bei
            '0' SPI mit LEDS und IR Sensoren verbunden
            '1' SPI mit Servohost verbunden
    */

    LL_GPIO_StructInit(&GPIO_InitStructure);

    GPIO_InitStructure.Pin = LL_GPIO_PIN_4 | LL_GPIO_PIN_12 | LL_GPIO_PIN_15;
    GPIO_InitStructure.Mode = LL_GPIO_MODE_OUTPUT;
    GPIO_InitStructure.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
    GPIO_InitStructure.Pull = LL_GPIO_PULL_NO;
    GPIO_InitStructure.Speed = LL_GPIO_SPEED_FREQ_HIGH;

    LL_GPIO_ResetOutputPin(GPIOA, LL_GPIO_PIN_4);       //Servohost für die datenübertragung vorbereiten
    LL_GPIO_ResetOutputPin(GPIOA, LL_GPIO_PIN_15);      //SPI ist mit Leds verbunden
    LL_GPIO_SetOutputPin(GPIOA, LL_GPIO_PIN_12);        //Daten können durch das schieberegister geschoben werden
    
    LL_GPIO_Init(GPIOA, &GPIO_InitStructure);
		
		//PA3 RX    PA2 TX
    LL_GPIO_StructInit(&GPIO_InitStructure);

    GPIO_InitStructure.Pin = LL_GPIO_PIN_2 | LL_GPIO_PIN_3;
    GPIO_InitStructure.Mode = LL_GPIO_MODE_ALTERNATE;
    GPIO_InitStructure.Alternate = LL_GPIO_AF_7;

    LL_GPIO_Init(GPIOA, &GPIO_InitStructure);
}

void USART2_Init(void)
{
    //UART Standardmäßig im Vollduplex
    LL_USART_InitTypeDef usart;

    LL_APB1_GRP1_EnableClock(LL_APB1_GRP1_PERIPH_USART2);

    LL_USART_StructInit(&usart);
    usart.BaudRate = 9600;
	  usart.StopBits = 1;
		usart.Parity = LL_USART_PARITY_NONE;
    usart.TransferDirection = LL_USART_DIRECTION_TX_RX;     //Empfangen und Senden einstellen

    LL_USART_Init(USART2, &usart);
    LL_USART_Enable(USART2);
    LL_USART_EnableIT_RXNE(USART2);             //RXNE Interrupt aktivieren
		//LL_USART_EnableIT_TXE(USART2);

    NVIC_SetPriority(USART2_IRQn, 2);
    NVIC_EnableIRQ(USART2_IRQn);
}

void SPI_Start(uint8_t select)   //0 LED/IR auswählen    1 Servo auswählen
{
    LL_SPI_DisableIT_TXE(SPI1);     //SPI TXE/RXNE Interrupt zuerst deaktivieren
    LL_SPI_DisableIT_RXNE(SPI1);
    LL_SPI_Disable(SPI1);

    if (select) // servo
    {
        LL_SPI_SetDataWidth(SPI1, LL_SPI_DATAWIDTH_16BIT);          //16 Bit für Servo werte 16 bit weil von -200 bis 200
        LL_SPI_SetRxFIFOThreshold(SPI1, LL_SPI_RX_FIFO_TH_HALF);    //RXNE wird gesetzt sobald 2Byte empfangen wurden

        servoData[0] = (uint16_t)servoX;
        servoData[1] = (uint16_t)servoY;
        servoPending = 0;                                           //keine anstehenden Daten mehr

        LL_GPIO_ResetOutputPin(GPIOA, LL_GPIO_PIN_4);               //mit 0 wird der Servohost auf die ÜBertragung vorbereitet
        LL_GPIO_SetOutputPin(GPIOA, LL_GPIO_PIN_15);                //SPI mit Servo host verbinden
    }
    else if(!select) // LED/IR
    {
        LL_SPI_SetDataWidth(SPI1, LL_SPI_DATAWIDTH_8BIT);           //für IR/LED
        LL_SPI_SetRxFIFOThreshold(SPI1, LL_SPI_RX_FIFO_TH_QUARTER); //RXNE wird bei empfang von 1Byte gesetzt

        LL_GPIO_ResetOutputPin(GPIOA, LL_GPIO_PIN_15);              //SPI mit LED/IR verbinden
        LL_GPIO_ResetOutputPin(GPIOA, LL_GPIO_PIN_12);              //momentanaufnahme der IR Werte, werden ins schieberegister geladen
        LL_GPIO_SetOutputPin(GPIOA, LL_GPIO_PIN_12);                //nun können die IR Werte über SPI ausgelesen werden und die zuvor übertragenen LED Werte ins Ausgangsregister übernommen
    }

    spiSelect = select;                                               //ob servo aktiv ist oder nicht
    txIndex = 0;
    rxIndex = 0;

    LL_SPI_Enable(SPI1);                    //Nach der Umstellung wieder aktivieren
    LL_SPI_EnableIT_RXNE(SPI1);
    LL_SPI_EnableIT_TXE(SPI1);
}

void SPI_Init(void)
{
    LL_SPI_InitTypeDef SPI_InitStructure;

    LL_APB2_GRP1_EnableClock(LL_APB2_GRP1_PERIPH_SPI1);

    LL_SPI_StructInit(&SPI_InitStructure);

    SPI_InitStructure.Mode = LL_SPI_MODE_MASTER;
    SPI_InitStructure.BaudRate = LL_SPI_BAUDRATEPRESCALER_DIV256;    //8Mhz/64 = 125kBit/s berechnung
    SPI_InitStructure.NSS = LL_SPI_NSS_SOFT;                        //Negative Slave Select für 0 slave ist ausgewählt, NSS wird per Software bzw. GPIO gesteuert.

    LL_SPI_Init(SPI1, &SPI_InitStructure);

    // LL_SPI_SetInternalSSLevel(SPI1, LL_SPI_SS_LEVEL_HIGH);
    LL_SPI_SetRxFIFOThreshold(SPI1, LL_SPI_RX_FIFO_TH_QUARTER);				//RXNE wird schon bei 8Bit gesetzt normalerweise erst bei 16bit

    LL_SPI_Enable(SPI1);

    NVIC_SetPriority(SPI1_IRQn, 0);                                 //höchste priorität für SPI mit 0
    NVIC_EnableIRQ(SPI1_IRQn);
}

void LED_Set(uint8_t seite, uint8_t position)           //Seite 0 bis 3; Position 0 bis 15
{
    if (seite >= 4 || position >= 16)
        return;

    uint8_t byteIndex = seite * 3 + (position / 8) * 2;
    uint8_t bitIndex = position % 8;

    txData[byteIndex] |= (1U << bitIndex);
}

void IR_Auswerten(void)
{
    posX = IR_Position(mapX);
    posY = IR_Position(mapY);

    Matrix_Update();
}

void Matrix_Update(void)
{
    static uint8_t altX = 0xFF;
    static uint8_t altY = 0xFF;

    if (altX < 16 && altY < 16)
        matrix[altY][altX] = 0;

    if (posX < 16 && posY < 16)
        matrix[posY][posX] = 1;

    altX = posX;
    altY = posY;
}

uint8_t IR_Position(const IR_Sensor map[16])
{
    uint8_t erste = 0xFF;
    uint8_t letzte = 0;

    for (uint8_t i = 0; i < 16; i++)
    {
        if (rxData[map[i].byte] & map[i].maske)
        {
            if (erste == 0xFF)
                erste = i;

            letzte = i;
        }
    }

    if (erste == 0xFF)
        return 0xFF;

    return (erste + letzte) / 2;
}
   
void LED_Fadenkreuz(void)
{
    // Alte LED-Muster löschen
    for (uint8_t i = 0; i < 12; i++)
        txData[i] = 0;

    // Kugelposition in der 16x16-Matrix suchen
    for (uint8_t y = 0; y < 16; y++)
    {
        for (uint8_t x = 0; x < 16; x++)
        {
            if (matrix[y][x] == 1)
            {
                // X-Achse: unten und oben
                LED_Set(1, (x + 8) % 16);
                LED_Set(3, (23 - x) % 16);

                // Y-Achse: rechts und links
                LED_Set(0, (y + 8) % 16);
                LED_Set(2, (23 - y) % 16);

                return;
            }
        }
    }
}

		



void SPI1_IRQHandler(void)
{
    uint8_t anzahl = 12;        //anzahl der bytes bei LED/IR
	
    if (spiSelect)
    {
        anzahl = 2;             //anzahl der bytes bei Servoübertragung
    }
       

    // Daten empfangen
    if (LL_SPI_IsActiveFlag_RXNE(SPI1))     //abhängig von SPi_start wird dieser bei 1Byte oder 2Byte ausgelöst
    {
        if (spiSelect)
        {
            LL_SPI_ReceiveData16(SPI1);     //verwerfen der empfangenen Daten da wir sie nicht benötigen
            rxIndex++;
        }
        else
        {
            rxData[rxIndex] = LL_SPI_ReceiveData8(SPI1);
            rxIndex++;
        }
                                            //hier braucht man RXNE Interrupt nicht disablen weil ja schon TXE Interrupt disabled geworden ist, wenn keine neuen Daten mehr gesendet werden, werden auch keine empfangen
    }

    // Daten senden
    if (LL_SPI_IsActiveFlag_TXE(SPI1) && txIndex < anzahl)
    {
        if (spiSelect) // servo
        {
            LL_SPI_TransmitData16(SPI1, servoData[txIndex]);        //senden der Servodaten
            txIndex++;
        }
            
        else if(!spiSelect) // LED/IR
        {
            LL_SPI_TransmitData8(SPI1, txData[txIndex]);
            txIndex++;

        }
            
        if (txIndex == anzahl)
        {
            LL_SPI_DisableIT_TXE(SPI1);         //TXE Interrupt deaktiviern da wir sonst immer wieder den interrupt aufrufen würden
        }
    }

    // Wenn alle Daten übertragen wurden
    if (txIndex == anzahl && rxIndex == anzahl)
    {
        if (spiSelect)
        {
            LL_GPIO_SetOutputPin(GPIOA, LL_GPIO_PIN_4);
            SPI_Start(0);
        }
        else
        {
            IR_Auswerten();
            LED_Fadenkreuz();

            if (servoPending)
            {
                SPI_Start(1);
            }
            else
            {
                SPI_Start(0);
            }
        }
    }
}

void USART2_IRQHandler(void)
{
    static char eingabe[6] = {0};     //speichern der eingabe
    static uint8_t i = 0;       //cnt für eingabe

    if (LL_USART_IsActiveFlag_RXNE(USART2))
    {
        char c = LL_USART_ReceiveData8(USART2); //auslesen des zeichens

        if (c == '\r' || c == '\n')     // CR-LF -> \r , LF -> \n
        {
            eingabe[i] = '\0';          //das sscanf weiß wo die eingabe endet

            char achse;     //eingabe in achse und wert aufteilen
            int wert = 0;

            if (sscanf(eingabe, "%c=%d", &achse, &wert) == 2 &&
                wert >= -200 && wert <= 200)                            // zeichen und variable unterscheiden mit %c=%d, ==2 bedeutet zwei werte wurden erfolgreich eingelesen
            {
                if (achse == 'x') {
                    servoX = wert;
                    servoPending = 1;
                }
                else if (achse == 'y') {
                    servoY = wert;
                    servoPending = 1;
                }
            }
            i = 0;
						memset(eingabe, 0, sizeof(eingabe));
        }
        else if (i <6)
        {
            eingabe[i++] = c;   //eingabe speichert die zeichen
        }
    }
		
}





void ADC1_2_IRQHandler(void)
{
		static int32_t cnt = 0; // bis 65000
	
		static int64_t channel1 = 0;
		static int64_t channel2 = 0;
		static int64_t channel3 = 0;
		static int64_t channel4 = 0;
	
		
	
		if(LL_ADC_IsActiveFlag_EOS(ADC2))
			{
			channel1 += adcWerte[0];	
			channel2 += adcWerte[1];
			channel3 += adcWerte[2];
			channel4 += adcWerte[3];
				
			
			if(cnt == 65000)
			{
				filter_adcWerte[0] = channel1 / 65000;
				filter_adcWerte[1] = channel2 / 65000;
				filter_adcWerte[2] = channel3 / 65000;
				filter_adcWerte[3] = channel4 / 65000;
				
				channel1 = channel2 = channel3 = channel4 = 0;
				cnt = 0;
				
				filterMedianOverAllChannels = (filter_adcWerte[0] + filter_adcWerte[0] + filter_adcWerte[0] + filter_adcWerte[0]) / 4; // this caclulation is for kg calculations
				
				
				if((filterMedianOverAllChannels - 944) < 0) {
					gewicht = 0;
				}
				else {
				gewicht = (filterMedianOverAllChannels - 944) * 0.506;
					if(gewicht != altes_gewicht) {
				LL_USART_TransmitData8(USART2, gewicht);
				altes_gewicht = gewicht;
			}
				}
					
			}
			
			cnt++;
		}
	
    if (LL_ADC_IsActiveFlag_EOC(ADC2))      //Wenn eine Wandlung zu Ende ist 
    {
        adcWerte[adcIndex] = LL_ADC_REG_ReadConversionData12(ADC2);     //12 Bit Ergebnis der Wandlung speichern
        adcIndex++;
				
				
				

        if (adcIndex == 4)
        {
            adcIndex = 0;
        }
    }
}





/* Custom functions END-----------------------------------------------*/















/* ==============   BOARD SPECIFIC CONFIGURATION CODE BEGIN    ============== */
/**
  * @brief  System Clock Configuration
  *         The system Clock is configured as follow : 
  *            System Clock source            = PLL (HSI)
  *            SYSCLK(Hz)                     = 64000000
  *            HCLK(Hz)                       = 64000000
  *            AHB Prescaler                  = 1
  *            APB1 Prescaler                 = 2
  *            APB2 Prescaler                 = 2
  *            PLLMUL                         = 16
  *            Flash Latency(WS)              = 2
  * @param  None
  * @retval None
  */
void SystemClock_Config(void)
{
  /* Set FLASH latency */ 
  LL_FLASH_SetLatency(LL_FLASH_LATENCY_2);

  /* Enable HSI if not already activated*/
  if (LL_RCC_HSI_IsReady() == 0)
  {
    /* Enable HSI and wait for activation*/
    LL_RCC_HSI_Enable(); 
    while(LL_RCC_HSI_IsReady() != 1)
    {
    };
  }

  /* Main PLL configuration and activation */
  LL_RCC_PLL_ConfigDomain_SYS(LL_RCC_PLLSOURCE_HSI_DIV_2, LL_RCC_PLL_MUL_16);
  
  LL_RCC_PLL_Enable();
  while(LL_RCC_PLL_IsReady() != 1) 
  {
  };
  
  /* Sysclk activation on the main PLL */
  LL_RCC_SetAHBPrescaler(LL_RCC_SYSCLK_DIV_1);
  LL_RCC_SetSysClkSource(LL_RCC_SYS_CLKSOURCE_PLL);
  while(LL_RCC_GetSysClkSource() != LL_RCC_SYS_CLKSOURCE_STATUS_PLL)
  {
  };
  
  /* Set APB1 & APB2 prescaler*/
  LL_RCC_SetAPB1Prescaler(LL_RCC_APB1_DIV_2);
  LL_RCC_SetAPB2Prescaler(LL_RCC_APB2_DIV_2);
  
  /* Set systick to 1ms in using frequency set to 64MHz */
  /* This frequency can be calculated through LL RCC macro */
  /* ex: __LL_RCC_CALC_PLLCLK_FREQ ((HSI_VALUE / 2), LL_RCC_PLL_MUL_16) */
  LL_Init1msTick(64000000);
  
  /* Update CMSIS variable (which can be updated also through SystemCoreClockUpdate function) */
  LL_SetSystemCoreClock(64000000);
}
/* ==============   BOARD SPECIFIC CONFIGURATION CODE END      ============== */

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
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d", file, line) */

  /* Infinite loop */
  while (1)
  {
  }
}
#endif

/**
  * @}
  */

/**
  * @}
  */
