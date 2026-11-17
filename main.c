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
void SPI_Start();
void IR_Auswerten(void);
void LED_Fadenkreuz();
void LED_Set(uint8_t seite, uint8_t position);
uint8_t IR_Position(const IR_Sensor map[16]);
void Matrix_Update(void);


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
	
	
    LL_SPI_EnableIT_RXNE(SPI1);                          //sobald das Receive Register voll ist wird der SPI1 Interrupt aufgerufen
    LL_SPI_EnableIT_TXE(SPI1);                           //sobald das Transmit Register leer ist wird der SPI1 Interrupt aufgerufen
	
	
  
  
  /* Infinite loop */
  while (1)
  {
  }
}


/* Custom functions -----------------------------------------------*/


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
    if(LL_SPI_IsActiveFlag_RXNE(SPI1))
    {
        if (rxIndex < 12)
        {
            rxData[rxIndex] = LL_SPI_ReceiveData8(SPI1);
            rxIndex++;

            if (rxIndex == 12)
            {
                IR_Auswerten();     //nachdem alle werte Empfangen sind
                LED_Fadenkreuz();   //werden die LED Positionen für das Fadenkreuz gesetzt 
								// dauer von 0,2ms
							
							  txIndex = 0;
								rxIndex = 0;
							
								LL_GPIO_ResetOutputPin(GPIOA, LL_GPIO_PIN_12);       // IR-Sensorzustaende laden
								LL_GPIO_SetOutputPin(GPIOA, LL_GPIO_PIN_12);        
            }
        }
        else
        {
           
        }
    }

    if((LL_SPI_IsActiveFlag_TXE(SPI1)) && txIndex < 12)
    {
        LL_SPI_TransmitData8(SPI1, txData[txIndex]);
        txIndex++;
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

