/* @file  main.c
   @brief main source file for STM32L4 integration with UBLox-Neo 6M GPS
          (ST25 Board TFT Display support included)
   @author Shyam Jha (Avinashee Tech)
*/
/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <stdio.h>
#include "ringbuffer.h"
#include "gps.h"
#include "st25_discovery_lcd.h"
#include "avinasheetech_logo_jpg.h"
#include "gps_navigation_jpg.h"
#include "ili9341_cube.h"
#include "5x5_font.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

//TFT screen attributes
#define LCD_HEIGHT_Pixels 240
#define LCD_WIDTH_Pixels  320
#define DC_PORT								GPIOC
#define DC_PIN								GPIO_PIN_7
#define BURST_MAX_SIZE                      500

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
SPI_HandleTypeDef hspi2;

UART_HandleTypeDef huart2;

/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_SPI2_Init(void);
static void MX_USART2_UART_Init(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

char GGA_Dataset[100];      //buffer to save GGA sentence data
char GSV_Dataset[100];      //buffer to save GSV sentence data
char lcd_gps_buffer1[30];
char lcd_gps_buffer2[30];
char lcd_gps_buffer3[30];
char lcd_gps_buffer4[30];
char lcd_gps_buffer5[30];

gps_t ublox_gps;            //main GPS data structure
uint8_t gps_fix_valid = 0;
uint8_t tot_sats_valid = 0;

unsigned char c;
void Uart_isr (UART_HandleTypeDef *huart);

/**
 * @brief  draw image function
 * @param  image_array image pixel wise RGB code
 * @param  orientation image rotation
 * @retval None
 */
void DrawImage(const uint8_t *image_array,uint8_t orientation){
	uint64_t k = 0;
   //Display Logo image
   for(uint32_t i = 0; i < 240; i++)                 //height
   {
	   for(uint32_t j = 320; j > 0; j--)             //width
	   {
		   BSP_LCD_DrawPixel(i, j, (image_array[k]<<8)|(image_array[k+1]),orientation);
		   k+=2;
	   }
   }
}

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

  /* USER CODE BEGIN SysInit */

  /*Initialize LCD and clear with White Background*/
  BSP_LCD_Init( );
  BSP_LCD_SetFont( &Font22 );
  BSP_LCD_SetBackColor(LCD_COLOR_WHITE);
  BSP_LCD_SetTextColor(LCD_COLOR_RED);
  BSP_LCD_Clear(LCD_COLOR_WHITE);

  /*Text and Image Introduction*/
  BSP_LCD_DisplayPicture(0,0,namaste);   //namaste intro image
  HAL_Delay(2000);                       //some delay
  BSP_LCD_Clear(LCD_COLOR_WHITE);
  BSP_LCD_DisplayStringAtLine(4, (uint8_t*) "  Welcome back to  ");  //text display
  HAL_Delay(2000);                                                   //some delay
  BSP_LCD_DisplayPicture(0,0,avinashee_logo);  //channel logo image
  HAL_Delay(2000);                             //some delay
  BSP_LCD_Clear(LCD_COLOR_WHITE);
  BSP_LCD_DisplayStringAtLine(4, (uint8_t*) " Let's go watch our");
  BSP_LCD_DisplayStringAtLine(5, (uint8_t*) "      GNSS data     ");
  HAL_Delay(2000);
  BSP_LCD_DisplayPicture(0,0,gps_nav_frame1);  //gps satellite image frame 1
  HAL_Delay(4);                                //some delay
  BSP_LCD_DisplayPicture(0,0,gps_nav_frame2);  //gps satellite image frame 2
  HAL_Delay(1);                                //some delay
  BSP_LCD_DisplayPicture(0,0,gps_nav_frame3);  //gps satellite image frame 3
  HAL_Delay(1);                                //some delay
  BSP_LCD_Clear(LCD_COLOR_WHITE);

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_SPI2_Init();
  MX_USART2_UART_Init();
  /* USER CODE BEGIN 2 */

  /*initialize gps serial buffer*/
  buffer_init();

  /*enable the UART in Interrupt mode*/
  HAL_UART_Receive_IT(&huart2, &c, 1);
  HAL_Delay(1000);

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */

	  //GGA message search
	  if(find_dataset(GGA)==1){
		  memset(GGA_Dataset,0,sizeof(GGA_Dataset));
		  save_dataset(GGA_Dataset);
		  gps_fix_valid = decode_ggadataset(GGA_Dataset, &ublox_gps);
	  }

	  ublox_gps.gsv.current_dataset = 0;
	  ublox_gps.gsv.total_datasets = 0;

	  //GSV message search (multiple messages handled)
	  do{
		  if(find_dataset(GSV)==1){
		  		  memset(GSV_Dataset,0,sizeof(GSV_Dataset));
		  		  save_dataset(GSV_Dataset);
		  		  tot_sats_valid = decode_gsvdataset(GSV_Dataset, &ublox_gps);
		  }

	  }while(ublox_gps.gsv.current_dataset<ublox_gps.gsv.total_datasets);

	  //display details of GGA message if gps fix valid
	  if(gps_fix_valid==1){
		  /*prepare lcd data buffers*/
		  memset(lcd_gps_buffer1,0,sizeof(lcd_gps_buffer1));
		  sprintf(lcd_gps_buffer1,"Lat:%f %c",ublox_gps.gga.location.latitude,ublox_gps.gga.location.NS);
		  memset(lcd_gps_buffer2,0,sizeof(lcd_gps_buffer2));
		  sprintf(lcd_gps_buffer2,"Lon:%f %c",ublox_gps.gga.location.longitude,ublox_gps.gga.location.EW);
		  memset(lcd_gps_buffer3,0,sizeof(lcd_gps_buffer3));
		  sprintf(lcd_gps_buffer3,"GPS Quality: %d",ublox_gps.gga.gps_fix_valid);
		  memset(lcd_gps_buffer4,0,sizeof(lcd_gps_buffer4));
		  sprintf(lcd_gps_buffer4,"Sat Count: %d",ublox_gps.gga.satellite_count);
		  memset(lcd_gps_buffer5,0,sizeof(lcd_gps_buffer5));
		  sprintf(lcd_gps_buffer5,"Time (IST): %02d:%02d:%02d",ublox_gps.gga.IST_time.hour,ublox_gps.gga.IST_time.minute,ublox_gps.gga.IST_time.second);
		  gps_fix_valid = 0;

		  /*display lcd data buffers*/
		  BSP_LCD_Clear(LCD_COLOR_WHITE);
		  BSP_LCD_DisplayStringAtLine(Line1, (uint8_t*)lcd_gps_buffer1); //display latitude
		  BSP_LCD_DisplayStringAtLine(Line2, (uint8_t*)lcd_gps_buffer2); //display longitude
		  BSP_LCD_DisplayStringAtLine(Line3, (uint8_t*)lcd_gps_buffer3); //display gps fix status
		  BSP_LCD_DisplayStringAtLine(Line4, (uint8_t*)lcd_gps_buffer4); //display number of satellites used for position calculation
		  BSP_LCD_DisplayStringAtLine(Line5, (uint8_t*)lcd_gps_buffer5); //display current time in IST format
	  }

	  HAL_Delay(5000); //some delay for GGA details to be present on screen

	  //display details of GSV message if satellites available
	  if(tot_sats_valid==1){
		  /*prepare lcd data buffer*/
		  memset(lcd_gps_buffer1,0,sizeof(lcd_gps_buffer1));
		  sprintf(lcd_gps_buffer1,"Total Sats: %d",ublox_gps.gsv.total_num_sats); //display total satellites tracked

		  /*display lcd data buffer*/
		  BSP_LCD_Clear(LCD_COLOR_WHITE);
		  BSP_LCD_DisplayStringAtLine(Line1, (uint8_t*)lcd_gps_buffer1);

		  /*prepare lcd data buffer*/
		  memset(lcd_gps_buffer2,0,sizeof(lcd_gps_buffer2));
		  sprintf(lcd_gps_buffer2,"Sat-Ele-Azi-Snr");  //display format for satellite details to be presented

		  /*display lcd data buffer*/
		  BSP_LCD_DisplayStringAtLine(Line2, (uint8_t*)lcd_gps_buffer2);

		  uint8_t line_index = 3;
		  for(int sat_index=0;sat_index<(ublox_gps.gsv.total_num_sats);sat_index++){
			  if(line_index==8){
				  line_index = 0;
				  HAL_Delay(3000); //some delay for first screen of satellite info
				  //clear screen once first 5 satellite info displayed
				  BSP_LCD_Clear(LCD_COLOR_WHITE);
			  }

			  if(sat_index<6){
				  memset(lcd_gps_buffer3,0,sizeof(lcd_gps_buffer3));
				  sprintf(lcd_gps_buffer3,"%d - %d - %d - %d",ublox_gps.gsv.sat_id[sat_index],ublox_gps.gsv.sat_elev[sat_index],ublox_gps.gsv.sat_azi[sat_index],ublox_gps.gsv.sat_snr[sat_index]);
				  //display details for that particular satellite on first screen
				  BSP_LCD_DisplayStringAtLine(line_index, (uint8_t*)lcd_gps_buffer3);
				  line_index++;
			  }
			  else{
				  memset(lcd_gps_buffer3,0,sizeof(lcd_gps_buffer3));
				  sprintf(lcd_gps_buffer3,"%d - %d - %d - %d",ublox_gps.gsv.sat_id[sat_index],ublox_gps.gsv.sat_elev[sat_index],ublox_gps.gsv.sat_azi[sat_index],ublox_gps.gsv.sat_snr[sat_index]);
				  //display details for further satellite on next screen after clearing
				  BSP_LCD_DisplayStringAtLine(line_index, (uint8_t*)lcd_gps_buffer3);
				  line_index++;
			  }

		  }
		  tot_sats_valid = 0;

	  }

	  HAL_Delay(3000); //some delay for next screen of satellite info


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
  if (HAL_PWREx_ControlVoltageScaling(PWR_REGULATOR_VOLTAGE_SCALE1) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLM = 1;
  RCC_OscInitStruct.PLL.PLLN = 18;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV7;
  RCC_OscInitStruct.PLL.PLLQ = RCC_PLLQ_DIV2;
  RCC_OscInitStruct.PLL.PLLR = RCC_PLLR_DIV2;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_4) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief SPI2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_SPI2_Init(void)
{

  /* USER CODE BEGIN SPI2_Init 0 */

  /* USER CODE END SPI2_Init 0 */

  /* USER CODE BEGIN SPI2_Init 1 */

  /* USER CODE END SPI2_Init 1 */
  /* SPI2 parameter configuration*/
  hspi2.Instance = SPI2;
  hspi2.Init.Mode = SPI_MODE_MASTER;
  hspi2.Init.Direction = SPI_DIRECTION_2LINES;
  hspi2.Init.DataSize = SPI_DATASIZE_8BIT;
  hspi2.Init.CLKPolarity = SPI_POLARITY_LOW;
  hspi2.Init.CLKPhase = SPI_PHASE_1EDGE;
  hspi2.Init.NSS = SPI_NSS_SOFT;
  hspi2.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_8;
  hspi2.Init.FirstBit = SPI_FIRSTBIT_MSB;
  hspi2.Init.TIMode = SPI_TIMODE_DISABLE;
  hspi2.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
  hspi2.Init.CRCPolynomial = 7;
  hspi2.Init.CRCLength = SPI_CRC_LENGTH_DATASIZE;
  hspi2.Init.NSSPMode = SPI_NSS_PULSE_DISABLE;
  if (HAL_SPI_Init(&hspi2) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN SPI2_Init 2 */

  /* USER CODE END SPI2_Init 2 */

}

/**
  * @brief USART2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART2_UART_Init(void)
{

  /* USER CODE BEGIN USART2_Init 0 */

  /* USER CODE END USART2_Init 0 */

  /* USER CODE BEGIN USART2_Init 1 */

  /* USER CODE END USART2_Init 1 */
  huart2.Instance = USART2;
  huart2.Init.BaudRate = 9600;
  huart2.Init.WordLength = UART_WORDLENGTH_8B;
  huart2.Init.StopBits = UART_STOPBITS_1;
  huart2.Init.Parity = UART_PARITY_NONE;
  huart2.Init.Mode = UART_MODE_TX_RX;
  huart2.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart2.Init.OverSampling = UART_OVERSAMPLING_16;
  huart2.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
  huart2.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;
  if (HAL_UART_Init(&huart2) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART2_Init 2 */

  /* USER CODE END USART2_Init 2 */

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
  __HAL_RCC_GPIOH_CLK_ENABLE();
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();
  __HAL_RCC_GPIOD_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(SPI2_NSS_GPIO_Port, SPI2_NSS_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(LCD_RST_GPIO_Port, LCD_RST_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(LCD_DC_GPIO_Port, LCD_DC_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin : SPI2_NSS_Pin */
  GPIO_InitStruct.Pin = SPI2_NSS_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
  HAL_GPIO_Init(SPI2_NSS_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : LCD_RST_Pin */
  GPIO_InitStruct.Pin = LCD_RST_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
  HAL_GPIO_Init(LCD_RST_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : LCD_DC_Pin */
  GPIO_InitStruct.Pin = LCD_DC_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
  HAL_GPIO_Init(LCD_DC_GPIO_Port, &GPIO_InitStruct);

/* USER CODE BEGIN MX_GPIO_Init_2 */
/* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */

/**
 * @brief  UART ISR
 * @param  huart pointer to UART handle structure
 * @retval None
 * @note   insert character into the ring buffer on UART receive
 *         complete interrupt
 */
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
	buffer_insert(c);
	HAL_UART_Receive_IT(&huart2, &c, 1);
}


/**
 * @brief  customized implementation like strtok
 * @param  str pointer to character array
 * @param  delims delimiter seperating values in the character array
 * @retval seperated tokens
 * @note   customized strtok implementation to retain null values between delimiters.
 *         Example - "GGA,1,0,,,0" gives "GGA" "1" "0" "empty" "empty" "0"
 */
char * strtok_function (char * str, char const * delims)
{
  static char  * src = NULL;
  char  *  p,  * ret = 0;

  if (str != NULL)
    src = str;

  if (src == NULL)
    return NULL;

  if ((p = strpbrk (src, delims)) != NULL) {
    *p  = 0;    //replace delimiter with null character
    ret = src;  //get token
    src = ++p;  //increment past delimiter

  } else if (*src) {
    ret = src;  //get last token
    src = NULL;
  }

  return ret;
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
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
