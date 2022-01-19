/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

#include "string.h"
#include "stdlib.h"
#include "stdio.h"

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

uint8_t flag = 0;

// this interrupts changes flag to 1 as soon as the uint8_t buff[300] is full
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart) {
	flag = 1;
}

// function to calculate checksum of the NMEA sentence
// -4, but not -3 because the NMEA sentences are delimited with \r\n, and there also is the invisible \r in the end
int nmea0183_checksum(char *msg) {

	int checksum = 0;
	int j = 0;

	// the first $ sign and the last two bytes of original CRC + the * sign
	for (j = 1; j < strlen(msg) - 4; j++) {
		checksum = checksum ^ (unsigned) msg[j];
	}

	return checksum;
}

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
UART_HandleTypeDef huart1;
UART_HandleTypeDef huart2;
DMA_HandleTypeDef hdma_usart1_rx;

/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_DMA_Init(void);
static void MX_USART1_UART_Init(void);
static void MX_USART2_UART_Init(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
void print(char *message) {
	HAL_UART_Transmit(&huart2, (uint8_t*)message, strlen(message), 200);
}

void println(char *message) {
	HAL_UART_Transmit(&huart2, (uint8_t*)message, strlen(message), 200);
	HAL_UART_Transmit(&huart2, (uint8_t*)"\n\r", 2, 200);
}

void newline() {
	HAL_UART_Transmit(&huart2, (uint8_t*)"\n\r", 2, 200);
}

/* USER CODE END 0 */

/**
 * @brief  The application entry point.
 * @retval int
 */
int main(void) {
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

	/* USER CODE END SysInit */

	/* Initialize all configured peripherals */
	MX_GPIO_Init();
	MX_DMA_Init();
	MX_USART1_UART_Init();
	MX_USART2_UART_Init();
	/* USER CODE BEGIN 2 */

	//Store the raw data in various formats
	uint8_t buff[255];
	char buffStr[255];

	//Stores each indivudual sentence
	char nmeaSnt[80];

	char *rawSum;
	char smNmbr[3];

	// The Equator has a latitude of 0°,
	//the North Pole has a latitude of 90° North (written 90° N or +90°),
	//and the South Pole has a latitude of 90° South (written 90° S or −90°)

	char *latRaw;
	char latRawbuff[10];
	char latDg[2];
	char latMS[7];
	char *hemNS;
	char hemNSbuff[10];

	// longitude in degrees (0° at the Prime Meridian to +180° eastward and −180° westward)
	// that is why 3
	char *lonRaw;
	char lonRawbuff[10];
	char lonDg[3];
	char lonMS[7];
	char *hemEW;
	char hemEWbuff[10];
	char strLonMS[7];

	char *utcRaw; // raw UTC time from the NMEA sentence in the hhmmss format
	char utcRawbuff[10];
	char strUTC[8]; // UTC time in the readable hh:mm:ss format

	char hH[2]; // hours
	char mM[2]; // minutes
	char sS[2]; // seconds

	char *ground_speed_raw;
	char ground_speed_buff[7];

	char altitude[4];

	uint8_t cnt = 0;

	HAL_UART_Receive_DMA(&huart1, buff, 255);//Start receiving raw sentences from the GPS

	//char command[100] = "$PMTK314,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0*29\r\n"; // GNGLL sentence only. Sentence length: 50;
	//char command[100] = "$PMTK314,0,0,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0*29\r\n"; // GNGGA sentence only. Sentence length: 72;
	//char command[100] = "$PMTK314,0,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0*29\r\n"; // GNVTG sentence only. Sentence length: 33;
	//char command[100] = "$PMTK314,1,1,1,1,0,0,0,0,0,0,0,0,0,0,0,0,1,0,0*29\r\n"; //all sentences
	char command[100] = "$PMTK314,1,0,1,1,0,0,0,0,0,0,0,0,0,0,0,0,1,0,0*28\r\n"; //GLL, VTG, and GGA sentences only
	HAL_UART_Transmit(&huart1, (uint8_t*) command, strlen(command), 200);//this command is sent to the gps�.


	/* USER CODE END 2 */

	/* Infinite loop */
	/* USER CODE BEGIN WHILE */
	while (1) {
		newline();
		print("while loop");
		if (flag == 1) { // interrupt signals that the buffer buff[300] is full
			println("flag == 1");


			/*

			 $ - Start delimiter
			 * - Checksum delimiter
			 , - Field delimiter

			 1. $GNGLL log header
			 2. Latitude (Ddmm.mm) [The Equator has a latitude of 0°, the North Pole has a latitude of 90° North (written 90° N or +90°)]
			 3. Latitude direction (N = North, S = South)
			 4. Longitude (DDDmm.mm) [0° at the Prime Meridian to +180° eastward and −180° westward]
			 5. Longitude direction (E = East, W = West)
			 6. UTC time status of position (hours/minutes/seconds/decimal seconds) hhmmss
			 7. Data status: A = Data valid, V = Data invalid
			 8. Positioning system mode indicator
			 9. *xx Checksum
			 10. [CR][LF] Sentence terminator. In C \r\n (two characters).
			  or \r Carriage return
			  or \n Line feed, end delimiter

			 */

			memset(buffStr, 0, 255);

			sprintf(buffStr, "%s", buff);

			// if we want to display the incoming raw data
			println("RAW DATA");
			HAL_UART_Transmit(&huart2, buff, 255, 70);

			// splitting the buffStr by the "\n" delimiter with the strsep() C function
			// see http://www.manpagez.com/man/3/strsep/
			char *token, *string;

			//EITHER USE STRDUP, FREE AT THE END
			//string = strdup(buffStr);

			//OR USE STRCPY, NO FREEING AT THE END
			char buffStr_copy[255];
			strcpy(buffStr_copy, buffStr);
			string = buffStr_copy;

			newline();
			newline();
			newline();
			newline();
			println("********");
			println("********");
			println("********");
			println("********");
			println("********BUFF START:");
			println(buffStr);
			println("********BUFF END");
			println("********");
			println("********");
			println("********");
			println("********");
			newline();
			newline();
			newline();
			newline();

			// actually splitting the string by "\n" delimiter
			println("BEFORE newline splitting");
			newline();
			newline();
			newline();
			newline();
			println("********");
			println("********");
			println("********");
			println("********");
			println("********STRING START:");
			println(string);
			println("********STRING END");
			println("********");
			println("********");
			println("********");
			println("********");
			newline();
			newline();
			newline();
			newline();
			while ((token = strsep(&string, "\n")) != NULL) {
				println("AFTER newline splitting");

				memset(nmeaSnt, 0, 80);

				sprintf(nmeaSnt, "%s", token);

				// selecting only $GNGLL sentences, combined GPS and GLONASS
				// on my GPS sensor this good NMEA sentence is always 50 characters
				//println("BEFORE gll good");
				if ((strstr(nmeaSnt, "$GNGLL") != 0) && strlen(nmeaSnt) >= 49 && strstr(nmeaSnt, "*") != 0) {
					//println("AFTER gll good");

					rawSum = strstr(nmeaSnt, "*");

					memcpy(smNmbr, &rawSum[1], 2);

					smNmbr[2] = '\0';

					uint8_t intSum = nmea0183_checksum(nmeaSnt);

					char hex[2];

					// "%X" unsigned hexadecimal integer (capital letters)
					sprintf(hex, "%X", intSum);

					// checksum data verification, if OK, then we can really trust
					// the data in the the NMEA sentence
					//println("BEFORE gll checksum");
					if (strstr(smNmbr, hex) != NULL) {
						//println("AFTER gll checksum");

						//if we want display good $GNGLL NMEA sentences
						HAL_UART_Transmit(&huart2, (uint8_t*)nmeaSnt, 50, 70);
						HAL_UART_Transmit(&huart2, (uint8_t*) "\n\r", 2, 200);

						cnt = 0;

						// splitting the good NMEA sentence into the tokens by the comma delimiter
						//println("BEFORE gll splitting");
						for (char *pV = strtok(nmeaSnt, ","); pV != NULL; pV = strtok(NULL, ",")) {
							//println("AFTER gll splitting");

							switch (cnt) {
							case 1:
								//latRaw = strdup(pV);
								strcpy(latRawbuff, pV);
								latRaw = latRawbuff;
								break;
							case 2:
								//hemNS = strdup(pV);
								strcpy(hemNSbuff, pV);
								hemNS = hemNSbuff;
								break;
							case 3:
								//lonRaw = strdup(pV);
								strcpy(lonRawbuff, pV);
								lonRaw = lonRawbuff;
								break;
							case 4:
								//hemEW = strdup(pV);
								strcpy(hemEWbuff, pV);
								hemEW = hemEWbuff;
								break;
							case 5:
								//utcRaw = strdup(pV);
								strcpy(utcRawbuff, pV);
								utcRaw = utcRawbuff;
								break;
							}

							cnt++;

						}  // end for()

						memcpy(latDg, &latRaw[0], 2);
						latDg[2] = '\0';

						memcpy(latMS, &latRaw[2], 7);
						latMS[7] = '\0';

						memcpy(lonDg, &lonRaw[0], 3);
						lonDg[3] = '\0';

						memcpy(lonMS, &lonRaw[3], 7);
						lonMS[7] = '\0';
						//char strLonMS[7];
						sprintf(strLonMS, "%s", lonMS);

						//strcpy(strLonMS, lonMS);
//						newline();
//						print("*****INSIDE LONMS: ");
//						println(lonMS);
//						newline();
//						print("*****INSIDE STRLONMS: ");
//						println(strLonMS);
						newline();

						//converting the UTC time in the hh:mm:ss format
						memcpy(hH, &utcRaw[0], 2);
						hH[2] = '\0';

						memcpy(mM, &utcRaw[2], 2);
						mM[2] = '\0';

						memcpy(sS, &utcRaw[4], 2);
						sS[2] = '\0';

						strcpy(strUTC, hH);
						strcat(strUTC, ":");
						strcat(strUTC, mM);
						strcat(strUTC, ":");
						strcat(strUTC, sS);
						strUTC[8] = '\0';


						// HAL_UART_Transmit(&huart2, (uint8_t*) "Coordinates/Timestamp: ", 23, 200);
						// HAL_UART_Transmit(&huart2, (uint8_t*) "\r\n", 2, 200);

						// HAL_UART_Transmit(&huart2, (uint8_t*) hemNS, 1, 200);
						// HAL_UART_Transmit(&huart2, (uint8_t*) " ", 1, 200);
						// HAL_UART_Transmit(&huart2, (uint8_t*) latDg, 2, 200);
						// HAL_UART_Transmit(&huart2, (uint8_t*) "°", 2, 200);
						// HAL_UART_Transmit(&huart2, (uint8_t*) latMS, 7, 200);
						// HAL_UART_Transmit(&huart2, (uint8_t*) "\', ", 3, 200);

						// HAL_UART_Transmit(&huart2, (uint8_t*) hemEW, 1, 200);
						// HAL_UART_Transmit(&huart2, (uint8_t*) " ", 1, 200);
						// HAL_UART_Transmit(&huart2, (uint8_t*) lonDg, 3, 200);
						// HAL_UART_Transmit(&huart2, (uint8_t*) "°", 2, 200);
						// HAL_UART_Transmit(&huart2, (uint8_t*) strLonMS, strlen(strLonMS), 200);
						// HAL_UART_Transmit(&huart2, (uint8_t*) "\', UTC: ", 8, 200);

						// HAL_UART_Transmit(&huart2, (uint8_t*) strUTC, 8, 200);
						// HAL_UART_Transmit(&huart2, (uint8_t*) "\r\n", 2, 200);
						// HAL_UART_Transmit(&huart2, (uint8_t*) "\r\n", 2, 200);

						//Coordinates/Timestamp:
						newline();
						newline();
						print("Coordinates/Timestamp:");
						newline();

						print(hemNS);
						print(" ");
						print(latDg);
						print("°");
						print(latMS);
						print("\', ");
						print(hemEW);
						print(" ");
						print(lonDg);
						print("°");
						//print(strLonMS);
						print(lonMS);

						print("\', UTC: ");
						println(strUTC);
						newline();

					} // end of of the checksum data verification

				} // end of $GNGLL sentences selection


				//VTG
				// selecting only $GNVTG sentences, combined GPS and GLONASS
				// on my GPS sensor this good NMEA sentence is always 33 characters
				//println("BEFORE vtg good");
				if ((strstr(nmeaSnt, "$GNVTG") != 0) && strlen(nmeaSnt) >= 32 && strstr(nmeaSnt, "*") != 0) {
					//println("AFTER vtg good");

					rawSum = strstr(nmeaSnt, "*");

					memcpy(smNmbr, &rawSum[1], 2);

					smNmbr[2] = '\0';

					uint8_t intSum = nmea0183_checksum(nmeaSnt);

					char hex[2];

					// "%X" unsigned hexadecimal integer (capital letters)
					sprintf(hex, "%X", intSum);

					// checksum data verification, if OK, then we can really trust
					// the data in the the NMEA sentence
					//println("BEFORE vtg checksum");
					if (strstr(smNmbr, hex) != NULL) {
						//println("AFTER vtg checksum");

						//if we want display good $GNVTG NMEA sentences
						HAL_UART_Transmit(&huart2, (uint8_t*)nmeaSnt, 33, 70);
						HAL_UART_Transmit(&huart2, (uint8_t*) "\n\r", 2, 200);

						cnt = 0;

						// splitting the good NMEA sentence into the tokens by the comma delimiter
						//println("BEFORE vtg splitting");
						for (char *pV = strtok(nmeaSnt, ","); pV != NULL; pV = strtok(NULL, ",")) {
							//println("AFTER vtg splitting");
							switch(cnt) {
								case 6:
									//strcpy(ground_speed, pV);
									//ground_speed_raw = strdup(pV);
									strcpy(ground_speed_buff, pV);
									ground_speed_raw = ground_speed_buff;
									break;
							}

							cnt++;

						}  // end for()
						memcpy(ground_speed_buff, &ground_speed_raw[0], 7);
						ground_speed_buff[7] = '\0';

						// HAL_UART_Transmit(&huart2, (uint8_t*) "Speed (kph): ", 13, 200);
						// HAL_UART_Transmit(&huart2, (uint8_t*) "\r\n", 2, 200);
						// HAL_UART_Transmit(&huart2, (uint8_t*) ground_speed, 5, 200);
						// HAL_UART_Transmit(&huart2, (uint8_t*) "\r\n", 2, 200);
						// HAL_UART_Transmit(&huart2, (uint8_t*) "\r\n", 2, 200);

						//Speed
						newline();
						newline();
						println("Speed (kph):");
						println(ground_speed_buff);
						newline();

					} // end of of the checksum data verification

				} // end of $GNVTG sentences selection

				//GGA
				// selecting only $GNGGA sentences, combined GPS and GLONASS
				// on my GPS sensor this good NMEA sentence is always 72 characters
				//println("BEFORE gga good");
				if ((strstr(nmeaSnt, "$GNGGA") != 0) && strlen(nmeaSnt) >= 71 && strstr(nmeaSnt, "*") != 0) {
					//println("AFTER gga good");

					rawSum = strstr(nmeaSnt, "*");

					memcpy(smNmbr, &rawSum[1], 2);

					smNmbr[2] = '\0';

					uint8_t intSum = nmea0183_checksum(nmeaSnt);

					char hex[2];

					// "%X" unsigned hexadecimal integer (capital letters)
					sprintf(hex, "%X", intSum);

					// checksum data verification, if OK, then we can really trust
					// the data in the the NMEA sentence
					//println("BEFORE gga checksum");
					if (strstr(smNmbr, hex) != NULL) {
						//println("AFTER gga checksum");

						//if we want display good $GNGGA NMEA sentences
						HAL_UART_Transmit(&huart2, (uint8_t*)nmeaSnt, 72, 70);
						HAL_UART_Transmit(&huart2, (uint8_t*) "\n\r", 2, 200);

						cnt = 0;

						// splitting the good NMEA sentence into the tokens by the comma delimiter
						//println("BEFORE gga splitting");
						for (char *pV = strtok(nmeaSnt, ","); pV != NULL; pV = strtok(NULL, ",")) {
							//println("AFTER gga splitting");

							switch(cnt) {
								case 9:
									strcpy(altitude, pV);
									break;
							}

							cnt++;

						}  // end for()

						// HAL_UART_Transmit(&huart2, (uint8_t*) "Altitude (above mean sea level): ", 33, 200);
						// HAL_UART_Transmit(&huart2, (uint8_t*) "\r\n", 2, 200);
						// HAL_UART_Transmit(&huart2, (uint8_t*) altitude, 4, 200);
						// HAL_UART_Transmit(&huart2, (uint8_t*) "\r\n", 2, 200);
						// HAL_UART_Transmit(&huart2, (uint8_t*) "\r\n", 2, 200);

						//Altitude
						newline();
						newline();
						println("Altitude (above mean sea level):");
						println(altitude);
						newline();

					} // end of of the checksum data verification

				} // end of $GNGAA sentences selection

			} // end of splitting the buffStr by the "\n" delimiter with the strsep() C function
//
//			free(string);
//			free(latRaw);
//			free(hemNS);
//			free(lonRaw);
//			free(hemEW);
//			free(utcRaw);
//			free(ground_speed_raw);



			// HAL_UART_Transmit(&huart2, (uint8_t*) "Coordinates/Timestamp: ", 23, 200);
			// HAL_UART_Transmit(&huart2, (uint8_t*) "\r\n", 2, 200);

			// HAL_UART_Transmit(&huart2, (uint8_t*) hemNS, 1, 200);
			// HAL_UART_Transmit(&huart2, (uint8_t*) " ", 1, 200);
			// HAL_UART_Transmit(&huart2, (uint8_t*) latDg, 2, 200);
			// HAL_UART_Transmit(&huart2, (uint8_t*) "°", 2, 200);
			// HAL_UART_Transmit(&huart2, (uint8_t*) latMS, 7, 200);
			// HAL_UART_Transmit(&huart2, (uint8_t*) "\', ", 3, 200);

			// HAL_UART_Transmit(&huart2, (uint8_t*) hemEW, 1, 200);
			// HAL_UART_Transmit(&huart2, (uint8_t*) " ", 1, 200);
			// HAL_UART_Transmit(&huart2, (uint8_t*) lonDg, 3, 200);
			// HAL_UART_Transmit(&huart2, (uint8_t*) "°", 2, 200);
			// HAL_UART_Transmit(&huart2, (uint8_t*) strLonMS, strlen(strLonMS), 200);
			// HAL_UART_Transmit(&huart2, (uint8_t*) "\', UTC: ", 8, 200);

			// HAL_UART_Transmit(&huart2, (uint8_t*) strUTC, 8, 200);
			// HAL_UART_Transmit(&huart2, (uint8_t*) "\r\n", 2, 200);
			// HAL_UART_Transmit(&huart2, (uint8_t*) "\r\n", 2, 200);

			//Coordinates/Timestamp
//			newline();
//			print("*****OUTSIDE LONMS: ");
//			println(lonMS);
//			newline();
//			print("*****OUTSIDE STRLONMS: ");
//			println(strLonMS);
//			newline();



//			//Coordinates/Timestamp:
//			newline();
//			print("Coordinates/Timestamp:");
//			newline();
//
//			print(hemNS);
//			print(" ");
//			print(latDg);
//			print("°");
//			print(latMS);
//			print("\', ");
//			print(hemEW);
//			print(" ");
//			print(lonDg);
//			print("°");
//			//print(strLonMS);
//			print(lonMS);
//
//			print("\', UTC: ");
//			println(strUTC);
//			newline();

//			//Speed
//			println("Speed (kph):");
//			println(ground_speed);
//			newline();


//			//Altitude
//			println("Altitude (above mean sea level):");
//			println(altitude);
//			newline();

			//ground_speed[7] = "\0";//clear buff
			//memset(ground_speed, 0, 7);

			flag = 0; // we are ready to get new data from the sensor

		} // end of one interrupt/full-buffer cycle

		HAL_Delay(200);

		/* USER CODE END WHILE */

		/* USER CODE BEGIN 3 */
	} //end while
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
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
  RCC_OscInitStruct.PLL.PLLM = 1;
  RCC_OscInitStruct.PLL.PLLN = 10;
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
  * @brief USART1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART1_UART_Init(void)
{

  /* USER CODE BEGIN USART1_Init 0 */

  /* USER CODE END USART1_Init 0 */

  /* USER CODE BEGIN USART1_Init 1 */

  /* USER CODE END USART1_Init 1 */
  huart1.Instance = USART1;
  huart1.Init.BaudRate = 9600;
  huart1.Init.WordLength = UART_WORDLENGTH_8B;
  huart1.Init.StopBits = UART_STOPBITS_1;
  huart1.Init.Parity = UART_PARITY_NONE;
  huart1.Init.Mode = UART_MODE_TX_RX;
  huart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart1.Init.OverSampling = UART_OVERSAMPLING_16;
  huart1.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
  huart1.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;
  if (HAL_UART_Init(&huart1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART1_Init 2 */

  /* USER CODE END USART1_Init 2 */

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
  huart2.Init.BaudRate = 115200;
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
  * Enable DMA controller clock
  */
static void MX_DMA_Init(void)
{

  /* DMA controller clock enable */
  __HAL_RCC_DMA1_CLK_ENABLE();

  /* DMA interrupt init */
  /* DMA1_Channel5_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(DMA1_Channel5_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(DMA1_Channel5_IRQn);

}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOH_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(LD2_GPIO_Port, LD2_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin : B1_Pin */
  GPIO_InitStruct.Pin = B1_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_FALLING;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(B1_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : LD2_Pin */
  GPIO_InitStruct.Pin = LD2_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(LD2_GPIO_Port, &GPIO_InitStruct);

}

/* USER CODE BEGIN 4 */

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

