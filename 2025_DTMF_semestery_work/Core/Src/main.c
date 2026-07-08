/* USER CODE BEGIN Header */
/**
 ******************************************************************************
 * @file           : main.c
 * @brief          : DTMF Detektor a Generátor (STM32 SAI + Goertzel)
 *
 * Popis funkčnosti:
 * 1. Přijímá audio (8kHz, 24-bit) přes I2S/SAI pomocí DMA (Circular buffer).
 * 2. Počítá Goertzelův algoritmus pro detekci DTMF tónů.
 * 3. Zobrazuje výsledky na TM1637 displeji (asynchronně).
 * 4. Komunikuje přes UART s PC.
 * 5. Umí generovat DTMF tóny zpět přes I2S/SAI do výstupu.
 * @author         : Karel Najman
 * @date           : 2025-12-20
 ******************************************************************************
 */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define MAX_24BIT 8388607.0f // Maximální hodnota pro 24-bit signál
#define DTMF_THRESHOLD 20.0f // Experimentálně nastavená prahová hodnota pro detekci tónu
#define DTMF_BLOCK_SIZE 2048 // při vzorkovací frekvenci 8kHz to dává cca 0.256s dat na jeden blok
#define FS 8000              // vzorkovací frekvence

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */
/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
UART_HandleTypeDef hlpuart1;
UART_HandleTypeDef huart3;
DMA_HandleTypeDef hdma_lpuart1_rx;
DMA_HandleTypeDef hdma_lpuart1_tx;

SAI_HandleTypeDef hsai_BlockA1;
SAI_HandleTypeDef hsai_BlockB1;
DMA_HandleTypeDef hdma_sai1_a;
DMA_HandleTypeDef hdma_sai1_b;

TIM_HandleTypeDef htim5;
TIM_HandleTypeDef htim6;
TIM_HandleTypeDef htim7;

PCD_HandleTypeDef hpcd_USB_OTG_FS;

DMA_HandleTypeDef hdma_memtomem_dma1_channel1;
/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
void PeriphCommonClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_DMA_Init(void);
static void MX_LPUART1_UART_Init(void);
static void MX_USART3_UART_Init(void);
static void MX_USB_OTG_FS_PCD_Init(void);
static void MX_TIM6_Init(void);
static void MX_TIM5_Init(void);
static void MX_SAI1_Init(void);
static void MX_TIM7_Init(void);
/* USER CODE BEGIN PFP */
void HAL_SAI_RxHalfCpltCallback(SAI_HandleTypeDef *hsai);
void HAL_SAI_RxCpltCallback(SAI_HandleTypeDef *hsai);
void HAL_SAI_TxCpltCallback(SAI_HandleTypeDef *hsai);
void Process_DTMF(int32_t *samples);
char DTMF_detect(int32_t *samples);
float Goertzel_algorithm(int target_freq, int32_t *samples);
void App_Logic(char detected_char, uint8_t mode);
void Prepare_DTMF_Tx(char key);
float Generate_Tone(float freq1, float freq2, float t);
void Test_Display(void);
void Display_Data(char data);
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
int32_t rx_buffer[DTMF_BLOCK_SIZE]; // read buffer circular DMA
int32_t tx_buffer[DTMF_BLOCK_SIZE]; // transmit buffer

volatile uint8_t data_ready = 0;
volatile uint8_t is_transmitting = 0; // Blokuje detekci, když sami vysíláme (anti-loop)
int32_t *process_ptr = NULL;

/* Definice DTMF frekvencí (Standard ITU-T) */
const int low_freq[4] = {697, 770, 852, 941};
const int high_freq[4] = {1209, 1336, 1477, 1633};
uint8_t display_buffer[4] = {0, 0, 0, 0};

const char key_map[4][4] = {
    {'1', '2', '3', 'A'},
    {'4', '5', '6', 'B'},
    {'7', '8', '9', 'C'},
    {'*', '0', '#', 'D'}};

/* USER CODE END 0 */

/**
 * @brief  The application entry point.
 * @retval int
 */
int main(void)
{

  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* Configure the peripherals common clocks */
  PeriphCommonClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_DMA_Init();
  MX_LPUART1_UART_Init();
  MX_USART3_UART_Init();
  MX_USB_OTG_FS_PCD_Init();
  MX_TIM6_Init();
  MX_TIM5_Init();
  MX_SAI1_Init();
  MX_TIM7_Init();
  /* USER CODE BEGIN 2 */

  // reset RX bufferu
  memset(rx_buffer, 0, sizeof(rx_buffer));
  data_ready = 0;

  // Spuštění příjmu audia přes DMA
  // HAL_SAI_Receive_DMA(&hsai_BlockB1, (uint8_t *)rx_buffer, sizeof(rx_buffer) / sizeof(int32_t));

  HAL_SAI_Receive_DMA(&hsai_BlockB1, (uint8_t *)rx_buffer, DTMF_BLOCK_SIZE);
  // Inicializace TM1637 displeje a vyčištění
  TM1637_Init(&htim7, SCLK_GPIO_Port, SCLK_Pin, SDO_GPIO_Port, SDO_Pin);
  TM1637_ClearDisplay();

  // Výpis startovací zprávy přes UART
  char test_msg[] = "Nucleo START\r\n";
  HAL_UART_Transmit(&hlpuart1, (uint8_t *)test_msg, strlen(test_msg), 100);

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */

  while (1)
  {
    // Test_Display(); // Testovací animační funkce pro displej

    // --- ZPRACOVÁNÍ DAT ---
    // Čekáme, až nám DMA Callback nastaví vlajku 'data_ready'.
    if (data_ready)
    {
      data_ready = 0; // Smazat vlajku
      if (process_ptr != NULL)
      {
        // Spustit výpočet nad aktuálním blokem dat
        Process_DTMF(process_ptr);
      }
    }
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
  }
  /* USER CODE END 3 */
}

/**
 * @brief System Clock Configuration
 * @retval None
 */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Configure the main internal regulator output voltage
   */
  if (HAL_PWREx_ControlVoltageScaling(PWR_REGULATOR_VOLTAGE_SCALE1_BOOST) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the RCC Oscillators according to the specified parameters
   * in the RCC_OscInitTypeDef structure.
   */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI48 | RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSI48State = RCC_HSI48_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
  RCC_OscInitStruct.PLL.PLLM = 2;
  RCC_OscInitStruct.PLL.PLLN = 30;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = RCC_PLLQ_DIV2;
  RCC_OscInitStruct.PLL.PLLR = RCC_PLLR_DIV2;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
   */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_5) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
 * @brief Peripherals Common Clock Configuration
 * @retval None
 */
void PeriphCommonClock_Config(void)
{
  RCC_PeriphCLKInitTypeDef PeriphClkInit = {0};

  /** Initializes the peripherals clock
   */
  PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_SAI1;
  PeriphClkInit.Sai1ClockSelection = RCC_SAI1CLKSOURCE_PLLSAI1;
  PeriphClkInit.PLLSAI1.PLLSAI1Source = RCC_PLLSOURCE_HSI;
  PeriphClkInit.PLLSAI1.PLLSAI1M = 5;
  PeriphClkInit.PLLSAI1.PLLSAI1N = 96;
  PeriphClkInit.PLLSAI1.PLLSAI1P = RCC_PLLP_DIV25;
  PeriphClkInit.PLLSAI1.PLLSAI1Q = RCC_PLLQ_DIV2;
  PeriphClkInit.PLLSAI1.PLLSAI1R = RCC_PLLR_DIV2;
  PeriphClkInit.PLLSAI1.PLLSAI1ClockOut = RCC_PLLSAI1_SAI1CLK;
  if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
 * @brief LPUART1 Initialization Function
 * @param None
 * @retval None
 */
static void MX_LPUART1_UART_Init(void)
{

  /* USER CODE BEGIN LPUART1_Init 0 */

  /* USER CODE END LPUART1_Init 0 */

  /* USER CODE BEGIN LPUART1_Init 1 */

  /* USER CODE END LPUART1_Init 1 */
  hlpuart1.Instance = LPUART1;
  hlpuart1.Init.BaudRate = 2 * 115200;
  hlpuart1.Init.WordLength = UART_WORDLENGTH_8B;
  hlpuart1.Init.StopBits = UART_STOPBITS_1;
  hlpuart1.Init.Parity = UART_PARITY_NONE;
  hlpuart1.Init.Mode = UART_MODE_TX_RX;
  hlpuart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  hlpuart1.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
  hlpuart1.Init.ClockPrescaler = UART_PRESCALER_DIV1;
  hlpuart1.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;
  hlpuart1.FifoMode = UART_FIFOMODE_DISABLE;
  if (HAL_UART_Init(&hlpuart1) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_UARTEx_SetTxFifoThreshold(&hlpuart1, UART_TXFIFO_THRESHOLD_1_8) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_UARTEx_SetRxFifoThreshold(&hlpuart1, UART_RXFIFO_THRESHOLD_1_8) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_UARTEx_DisableFifoMode(&hlpuart1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN LPUART1_Init 2 */

  /* USER CODE END LPUART1_Init 2 */
}

/**
 * @brief USART3 Initialization Function
 * @param None
 * @retval None
 */
static void MX_USART3_UART_Init(void)
{

  /* USER CODE BEGIN USART3_Init 0 */

  /* USER CODE END USART3_Init 0 */

  /* USER CODE BEGIN USART3_Init 1 */

  /* USER CODE END USART3_Init 1 */
  huart3.Instance = USART3;
  huart3.Init.BaudRate = 115200;
  huart3.Init.WordLength = UART_WORDLENGTH_8B;
  huart3.Init.StopBits = UART_STOPBITS_1;
  huart3.Init.Parity = UART_PARITY_NONE;
  huart3.Init.Mode = UART_MODE_TX_RX;
  huart3.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart3.Init.OverSampling = UART_OVERSAMPLING_16;
  huart3.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
  huart3.Init.ClockPrescaler = UART_PRESCALER_DIV1;
  huart3.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;
  if (HAL_UART_Init(&huart3) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_UARTEx_SetTxFifoThreshold(&huart3, UART_TXFIFO_THRESHOLD_1_8) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_UARTEx_SetRxFifoThreshold(&huart3, UART_RXFIFO_THRESHOLD_1_8) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_UARTEx_DisableFifoMode(&huart3) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART3_Init 2 */

  /* USER CODE END USART3_Init 2 */
}

/**
 * @brief SAI1 Initialization Function
 * @param None
 * @retval None
 */
static void MX_SAI1_Init(void)
{

  /* USER CODE BEGIN SAI1_Init 0 */

  /* USER CODE END SAI1_Init 0 */

  /* USER CODE BEGIN SAI1_Init 1 */

  /* USER CODE END SAI1_Init 1 */
  hsai_BlockA1.Instance = SAI1_Block_A;
  hsai_BlockA1.Init.AudioMode = SAI_MODEMASTER_TX;
  hsai_BlockA1.Init.Synchro = SAI_ASYNCHRONOUS;
  hsai_BlockA1.Init.OutputDrive = SAI_OUTPUTDRIVE_DISABLE;
  hsai_BlockA1.Init.NoDivider = SAI_MASTERDIVIDER_ENABLE;
  hsai_BlockA1.Init.FIFOThreshold = SAI_FIFOTHRESHOLD_EMPTY;
  hsai_BlockA1.Init.AudioFrequency = SAI_AUDIO_FREQUENCY_8K;
  hsai_BlockA1.Init.SynchroExt = SAI_SYNCEXT_DISABLE;
  hsai_BlockA1.Init.MonoStereoMode = SAI_MONOMODE;
  hsai_BlockA1.Init.CompandingMode = SAI_NOCOMPANDING;
  hsai_BlockA1.Init.TriState = SAI_OUTPUT_NOTRELEASED;
  if (HAL_SAI_InitProtocol(&hsai_BlockA1, SAI_I2S_STANDARD, SAI_PROTOCOL_DATASIZE_24BIT, 2) != HAL_OK)
  {
    Error_Handler();
  }
  hsai_BlockB1.Instance = SAI1_Block_B;
  hsai_BlockB1.Init.AudioMode = SAI_MODEMASTER_RX;
  hsai_BlockB1.Init.Synchro = SAI_ASYNCHRONOUS;
  hsai_BlockB1.Init.OutputDrive = SAI_OUTPUTDRIVE_DISABLE;
  hsai_BlockB1.Init.NoDivider = SAI_MASTERDIVIDER_ENABLE;
  hsai_BlockB1.Init.FIFOThreshold = SAI_FIFOTHRESHOLD_EMPTY;
  hsai_BlockB1.Init.AudioFrequency = SAI_AUDIO_FREQUENCY_8K;
  hsai_BlockB1.Init.SynchroExt = SAI_SYNCEXT_DISABLE;
  hsai_BlockB1.Init.MonoStereoMode = SAI_MONOMODE;
  hsai_BlockB1.Init.CompandingMode = SAI_NOCOMPANDING;
  if (HAL_SAI_InitProtocol(&hsai_BlockB1, SAI_I2S_STANDARD, SAI_PROTOCOL_DATASIZE_24BIT, 2) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN SAI1_Init 2 */

  /* USER CODE END SAI1_Init 2 */
}

/**
 * @brief TIM5 Initialization Function
 * @param None
 * @retval None
 */
static void MX_TIM5_Init(void)
{

  /* USER CODE BEGIN TIM5_Init 0 */

  /* USER CODE END TIM5_Init 0 */

  TIM_ClockConfigTypeDef sClockSourceConfig = {0};
  TIM_MasterConfigTypeDef sMasterConfig = {0};

  /* USER CODE BEGIN TIM5_Init 1 */

  /* USER CODE END TIM5_Init 1 */
  htim5.Instance = TIM5;
  htim5.Init.Prescaler = 0;
  htim5.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim5.Init.Period = 4294967295;
  htim5.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim5.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_Base_Init(&htim5) != HAL_OK)
  {
    Error_Handler();
  }
  sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
  if (HAL_TIM_ConfigClockSource(&htim5, &sClockSourceConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim5, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM5_Init 2 */

  /* USER CODE END TIM5_Init 2 */
}

/**
 * @brief TIM6 Initialization Function
 * @param None
 * @retval None
 */
static void MX_TIM6_Init(void)
{

  /* USER CODE BEGIN TIM6_Init 0 */

  /* USER CODE END TIM6_Init 0 */

  TIM_MasterConfigTypeDef sMasterConfig = {0};

  /* USER CODE BEGIN TIM6_Init 1 */

  /* USER CODE END TIM6_Init 1 */
  htim6.Instance = TIM6;
  htim6.Init.Prescaler = 1000;
  htim6.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim6.Init.Period = 12000;
  htim6.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_Base_Init(&htim6) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim6, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM6_Init 2 */

  /* USER CODE END TIM6_Init 2 */
}

/**
 * @brief TIM7 Initialization Function
 * @param None
 * @retval None
 */
static void MX_TIM7_Init(void)
{

  /* USER CODE BEGIN TIM7_Init 0 */
  __HAL_RCC_TIM7_CLK_ENABLE();
  /* USER CODE END TIM7_Init 0 */

  TIM_MasterConfigTypeDef sMasterConfig = {0};

  /* USER CODE BEGIN TIM7_Init 1 */

  /* USER CODE END TIM7_Init 1 */
  htim7.Instance = TIM7;
  htim7.Init.Prescaler = 120 - 1;
  htim7.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim7.Init.Period = 10;
  htim7.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_Base_Init(&htim7) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim7, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  // /* USER CODE BEGIN TIM7_Init 2 */
  HAL_NVIC_SetPriority(TIM7_IRQn, 2, 0);
  // Povolí přerušení globálně
  HAL_NVIC_EnableIRQ(TIM7_IRQn);
  // /* USER CODE END TIM7_Init 2 */
}

/**
 * @brief USB_OTG_FS Initialization Function
 * @param None
 * @retval None
 */
static void MX_USB_OTG_FS_PCD_Init(void)
{

  /* USER CODE BEGIN USB_OTG_FS_Init 0 */

  /* USER CODE END USB_OTG_FS_Init 0 */

  /* USER CODE BEGIN USB_OTG_FS_Init 1 */

  /* USER CODE END USB_OTG_FS_Init 1 */
  hpcd_USB_OTG_FS.Instance = USB_OTG_FS;
  hpcd_USB_OTG_FS.Init.dev_endpoints = 6;
  hpcd_USB_OTG_FS.Init.speed = PCD_SPEED_FULL;
  hpcd_USB_OTG_FS.Init.phy_itface = PCD_PHY_EMBEDDED;
  hpcd_USB_OTG_FS.Init.Sof_enable = ENABLE;
  hpcd_USB_OTG_FS.Init.low_power_enable = DISABLE;
  hpcd_USB_OTG_FS.Init.lpm_enable = DISABLE;
  hpcd_USB_OTG_FS.Init.battery_charging_enable = ENABLE;
  hpcd_USB_OTG_FS.Init.use_dedicated_ep1 = DISABLE;
  hpcd_USB_OTG_FS.Init.vbus_sensing_enable = ENABLE;
  if (HAL_PCD_Init(&hpcd_USB_OTG_FS) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USB_OTG_FS_Init 2 */

  /* USER CODE END USB_OTG_FS_Init 2 */
}

/**
 * Enable DMA controller clock
 * Configure DMA for memory to memory transfers
 *   hdma_memtomem_dma1_channel1
 */
static void MX_DMA_Init(void)
{

  /* DMA controller clock enable */
  __HAL_RCC_DMAMUX1_CLK_ENABLE();
  __HAL_RCC_DMA2_CLK_ENABLE();
  __HAL_RCC_DMA1_CLK_ENABLE();

  /* Configure DMA request hdma_memtomem_dma1_channel1 on DMA1_Channel1 */
  hdma_memtomem_dma1_channel1.Instance = DMA1_Channel1;
  hdma_memtomem_dma1_channel1.Init.Request = DMA_REQUEST_MEM2MEM;
  hdma_memtomem_dma1_channel1.Init.Direction = DMA_MEMORY_TO_MEMORY;
  hdma_memtomem_dma1_channel1.Init.PeriphInc = DMA_PINC_ENABLE;
  hdma_memtomem_dma1_channel1.Init.MemInc = DMA_MINC_ENABLE;
  hdma_memtomem_dma1_channel1.Init.PeriphDataAlignment = DMA_PDATAALIGN_WORD;
  hdma_memtomem_dma1_channel1.Init.MemDataAlignment = DMA_MDATAALIGN_WORD;
  hdma_memtomem_dma1_channel1.Init.Mode = DMA_NORMAL;
  hdma_memtomem_dma1_channel1.Init.Priority = DMA_PRIORITY_LOW;
  if (HAL_DMA_Init(&hdma_memtomem_dma1_channel1) != HAL_OK)
  {
    Error_Handler();
  }

  /* DMA interrupt init */
  /* DMA1_Channel1_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(DMA1_Channel1_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(DMA1_Channel1_IRQn);
  /* DMA1_Channel3_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(DMA1_Channel3_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(DMA1_Channel3_IRQn);
  /* DMA1_Channel4_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(DMA1_Channel4_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(DMA1_Channel4_IRQn);
  /* DMA2_Channel1_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(DMA2_Channel1_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(DMA2_Channel1_IRQn);
  /* DMA2_Channel2_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(DMA2_Channel2_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(DMA2_Channel2_IRQn);
}

/**
 * @brief GPIO Initialization Function
 * @param None
 * @retval None
 */
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};
  /* USER CODE BEGIN MX_GPIO_Init_1 */

  /* USER CODE END MX_GPIO_Init_1 */

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOE_CLK_ENABLE();
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOF_CLK_ENABLE();
  __HAL_RCC_GPIOH_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();
  __HAL_RCC_GPIOD_CLK_ENABLE();
  __HAL_RCC_GPIOG_CLK_ENABLE();
  HAL_PWREx_EnableVddIO2();
  __HAL_RCC_GPIOA_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOB, LD3_Pin | LD2_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(USB_PowerSwitchOn_GPIO_Port, USB_PowerSwitchOn_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOC, LD1_Pin | PIN1_Pin | PIN2_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOD, SDO_Pin | SCLK_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin : B1_Pin */
  GPIO_InitStruct.Pin = B1_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING_FALLING;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(B1_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pins : LD3_Pin LD2_Pin */
  GPIO_InitStruct.Pin = LD3_Pin | LD2_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /*Configure GPIO pin : USB_OverCurrent_Pin */
  GPIO_InitStruct.Pin = USB_OverCurrent_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(USB_OverCurrent_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : USB_PowerSwitchOn_Pin */
  GPIO_InitStruct.Pin = USB_PowerSwitchOn_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(USB_PowerSwitchOn_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pins : LD1_Pin PIN1_Pin PIN2_Pin */
  GPIO_InitStruct.Pin = LD1_Pin | PIN1_Pin | PIN2_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

  /*Configure GPIO pins : SDO_Pin SCLK_Pin */
  GPIO_InitStruct.Pin = SDO_Pin | SCLK_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_OD;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
  HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);

  /* EXTI interrupt init*/
  HAL_NVIC_SetPriority(EXTI15_10_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(EXTI15_10_IRQn);

  /* USER CODE BEGIN MX_GPIO_Init_2 */
  /* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */

/* --- DMA CALLBACKY --- */

// Zavolá se, když je plná PRVNÍ POLOVINA bufferu
void HAL_SAI_RxHalfCpltCallback(SAI_HandleTypeDef *hsai)
{
  process_ptr = &rx_buffer[0];
  data_ready = 1;
}

// Zavolá se, když je plná DRUHÁ POLOVINA bufferu
void HAL_SAI_RxCpltCallback(SAI_HandleTypeDef *hsai)
{
  process_ptr = &rx_buffer[DTMF_BLOCK_SIZE / 2]; // ukazatel na druhou polovinu bufferu
  data_ready = 1;
}

// Zavolá se, když skončí odesílání tónu (TX)
void HAL_SAI_TxCpltCallback(SAI_HandleTypeDef *hsai)
{
  is_transmitting = 0; // vysílání skončilo --> je možné znovu poslouchat
}

// Přerušení časovače pro obsluhu TM1637 a bliknutí LED2
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
  if (htim->Instance == TIM7)
  {
    // Obsluha TM1637 v přerušení časovače
    TM1637_Timer_Callback(htim);

    // blikání/svícení LED2 jako indikace běhu hlavní smyčky
    HAL_GPIO_TogglePin(LD2_GPIO_Port, LD2_Pin);
  }
}

// button click přerušení, vyčistění displeje
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
  if (GPIO_Pin == B1_Pin)
  {
    TM1637_ClearDisplay();
    // vyprázdnění bufferu
    for (int i = 0; i < 4; i++)
    {
      display_buffer[i] = 0;
    }
  }
}

/**
 * @brief Hlavní funkce pro zpracování bloku dat
 * @param samples Ukazatel na vzorky signálu
 */
void Process_DTMF(int32_t *samples)
{
  if (is_transmitting == 1) // pokud právě vysíláme, neděláme detekci
    return;

  static uint32_t last_success_time = 0;
  static char last_char = 0;

  char current_char = DTMF_detect(samples); // detekce DTMF znaku

  if (current_char != '\0') // pokud byl detekován platný znak
  {
    uint32_t now = HAL_GetTick();
    // Ochrana proti opakované detekci téhož znaku během krátké doby
    if (current_char == last_char && (now - last_success_time < 300))
    {
      return;
    }
    last_char = current_char;
    last_success_time = now;

    App_Logic(current_char, 1);
  }
  else
  {
    last_char = '\0'; // reset posledního znaku při tichu
  }
}

/**
 * @brief Goertzelův algoritmus
 * Vypočítá energii (magnitudu) pro jednu konkrétní frekvenci.
 * @param target_freq Cílová frekvence pro detekci
 * @param samples Ukazatel na vzorky signálu
 * @return Magnituda (energie) na cílové frekvenci
 */
float Goertzel_algorithm(int target_freq, int32_t *samples)
{
  float coeff, Q0, Q1, Q2;
  float magnitude;

  Q1 = 0.0f;
  Q2 = 0.0f;

  coeff = 2.0f * arm_cos_f32(2.0f * M_PI * target_freq / (float)FS);

  for (int j = 0; j < DTMF_BLOCK_SIZE / 2; j++) // 2048 / 2 = 1024 vzorků
  {
    float sample_norm = 0;
    // Normalizace 24bit signálu na float
    if (0x00800000 & samples[j]) // záporné číslo
    {
      sample_norm = (0xFF000000 | samples[j]); // sign extend
    }
    else
    {
      sample_norm = (0x00FFFFFF & samples[j]); // kladné číslo
    }


    Q0 = (coeff * Q1) - Q2 + sample_norm;
    Q2 = Q1;
    Q1 = Q0;
  }

  magnitude = (Q1 * Q1) + (Q2 * Q2) - (coeff * Q1 * Q2);

  return magnitude;
}

/**
 * @brief Detekce DTMF znaku v bloku vzorků
 * @param samples Ukazatel na vzorky signálu
 * @return Detekovaný DTMF znak nebo '\0' pokud žádný znak nebyl detekován
 */
char DTMF_detect(int32_t *samples)
{
  char detected_char = '\0'; // Výchozí hodnota - "žádný" znak
  float magnitude;

  // --- 1. Hledání nejsilnější Nízké frekvence ---
  float max_mag_low = 0.0f;
  int max_ind_low = -1;

  for (int i = 0; i < 4; i++)
  {
    magnitude = Goertzel_algorithm(low_freq[i], samples);

    if (magnitude > max_mag_low)
    {
      max_mag_low = magnitude;
      max_ind_low = i;
    }
  }

  // --- 2. Hledání nejsilnější Vysoké frekvence ---
  float max_mag_high = 0.0f;
  int max_ind_high = -1;

  for (int i = 0; i < 4; i++)
  {
    magnitude = Goertzel_algorithm(high_freq[i], samples);

    if (magnitude > max_mag_high)
    {
      max_mag_high = magnitude;
      max_ind_high = i;
    }
  }

  // --- 3. Filtrace Šumu ---
  // Pokud ani nejsilnější tóny nepřekročí práh, je to šum.
  if (max_mag_low < DTMF_THRESHOLD || max_mag_high < DTMF_THRESHOLD)
  {
    return '\0'; // Príliš slabé = Ticho
  }

  // --- 4. Twist Check (Kontrola poměru) ---
  // Ověřuje, zda jsou obě frekvence (L a H) zhruba stejně silné.
  // Brání falešné detekci při pískání nebo jednofrekvenčním rušení.
  float ratio;
  if (max_mag_high > 0)
    ratio = max_mag_low / max_mag_high;
  else
    ratio = 999.0f; // ochrana proti dělení nulou

  // tolerance 4:1
  if (ratio > 4.0f || ratio < 0.25f)
    return '\0';

  // --- 5. Úspěšná detekce ---
  if (max_ind_low != -1 && max_ind_high != -1)
    detected_char = key_map[max_ind_low][max_ind_high];

  return detected_char;
}
/**
 * @brief Aplikační logika
 * @param detected_char Detekovaný DTMF znak
 * @param mode Režim operace (1 = zobrazit na displeji, a vysílá, 2 = vysílat DTMF tón, default = nic)
 * Co se stane, když je detekován znak.
 */
void App_Logic(char detected_char, uint8_t mode)
{
  char char_to_send = detected_char;

  // výpis detekovaného znaku přes UART
  char msg[32];
  sprintf(msg, "Detekovany znak: %c\r\n", char_to_send);
  HAL_UART_Transmit(&hlpuart1, (uint8_t *)msg, strlen(msg), 10);

  switch (mode)
  {
  case 1:
    /* zobrazí na displaji */
    Display_Data(detected_char);
  case 2:
    /* echo DTMF tón */
    Prepare_DTMF_Tx(char_to_send);
    is_transmitting = 1; // vysílání

    // Spustit přenos přes DMA
    HAL_SAI_Transmit_DMA(&hsai_BlockA1, (uint8_t *)tx_buffer, sizeof(tx_buffer) / sizeof(int32_t));
    break;

  default:
    break;
  }
}

/**
 * @brief Připraví DTMF tón pro vysílání
 * @param key DTMF znak k vysílání
 */
void Prepare_DTMF_Tx(char key)
{
  float key_low_f = 0.0f;
  float key_high_f = 0.0f;
  uint8_t found = 0;

  for (int x = 0; x < 4; x++)
  {
    for (int y = 0; y < 4; y++)
    {
      if (key == key_map[x][y])
      {
        key_low_f = (float)low_freq[x];
        key_high_f = (float)high_freq[y];
        found = 1;
        break; // pro rychlejší ukončení vnořené smyčky
      }
    }
    if (found)
      break; // pro rychlejší ukončení vnější smyčky
  }

  for (int n = 0; n < DTMF_BLOCK_SIZE; n++)
  {
    float f_t = (float)n / (float)FS; // časová osa

    // Pokud frekvence nenajde, key_low_f a key_high_f zůstanou 0.0f (ticho)
    float sample = Generate_Tone(key_low_f, key_high_f, f_t);

    int32_t pcm_sample = 0;
    pcm_sample = (int32_t)(sample * MAX_24BIT * 0.5f); // škálování na 24-bit PCM
    tx_buffer[n] = pcm_sample;
  }
}

/**
 * @brief Generuje DTMF tón kombinací dvou frekvencí
 * @param freq_1 První frekvence
 * @param freq_2 Druhá frekvence
 * @param t Čas
 * @return Hodnota vzorku tónu
 */
float Generate_Tone(float freq_1, float freq_2, float t)
{
  return arm_sin_f32(2.0f * PI * freq_1 * t) + arm_sin_f32(2.0f * PI * freq_2 * t);
}

/**
 * @brief Testovací funkce pro displej
 * Zobrazuje střídavě horní, střední a spodní segmenty na všech 4 segmentech.
 */
void Test_Display(void)
{
  static uint32_t last_time = 0;
  if (HAL_GetTick() - last_time > 1000) // každou 1 sekundu
  {
    last_time = HAL_GetTick();
    static uint8_t toggle = 0;

    uint8_t segment_data[4];
    switch (toggle)
    {
    case 0:
      /* vrchni segemnty */
      segment_data[0] = 0x01;
      segment_data[1] = 0x01;
      segment_data[2] = 0x01;
      segment_data[3] = 0x01;
      toggle = 1;
      break;
    case 1:
      /* střední segmenty */
      segment_data[0] = 0x40;
      segment_data[1] = 0x40;
      segment_data[2] = 0x40;
      segment_data[3] = 0x40;
      toggle = 2;
      break;

    default:
      /*spodní segmenty*/
      segment_data[0] = 0x08;
      segment_data[1] = 0x08;
      segment_data[2] = 0x08;
      segment_data[3] = 0x08;
      toggle = 0;
      break;
    }
    TM1637_Update_Async(segment_data, 4);
  }
}

/**
 * @brief Zobrazí znak na displeji
 * Posune stávající znaky doleva a přidá nový znak na konec.
 * @param data Znak k zobrazení
 */
void Display_Data(char data)
{
  // Shift aktuálních znaků doleva
  display_buffer[0] = display_buffer[1];
  display_buffer[1] = display_buffer[2];
  display_buffer[2] = display_buffer[3];
  // Příchozí znak nakonec
  display_buffer[3] = TM1637_CharToSegment(data);
  TM1637_Update_Async(display_buffer, 4);
}
/* USER CODE END 4 */

/**
 * @brief  This function is executed in case of error occurrence.
 * @retval None
 */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}
#ifdef USE_FULL_ASSERT
/**
 * @brief  Reports the name of the source file and the source line number
 *         where the assert_param error has occurred.
 * @param  file: pointer to the source file name
 * @param  line: assert_param error line source number
 * @retval None
 */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
