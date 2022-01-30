/*
 * gps.c
 *
 *  Created on: Jan 30, 2022
 *      Author: rafee
 */

#include "gps.h"
#include "main.h"
#include <stdio.h>
#include <string.h>

//Declarations
UART_HandleTypeDef huart1;
UART_HandleTypeDef huart2;
DMA_HandleTypeDef hdma_usart1_rx;


//Variables
char gps_data[200];//this will be returned to the user
char coordinates[50];
char time[50];
char speed[50];
char altitude_data[50];




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

char* get_gps_data(char* choice) {
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
		HAL_UART_Transmit(&huart1, (uint8_t*) command, strlen(command), 200); //Send the command to the gps to display certain sentences.









		while (1) {
				if (flag == 1) { // interrupt signals that the buffer buff[300] is full
					//println("flag == 1");

					memset(gps_data, 0, sizeof(gps_data));


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
					//println("RAW DATA");
					//HAL_UART_Transmit(&huart2, buff, 255, 70);

					// splitting the buffStr by the "\n" delimiter with the strsep() C function
					// see http://www.manpagez.com/man/3/strsep/
					char *token, *string;

					char buffStr_copy[255];
					strcpy(buffStr_copy, buffStr);
					string = buffStr_copy;

					// actually splitting the string by "\n" delimiter
					while ((token = strsep(&string, "\n")) != NULL) {
						//println("AFTER newline splitting");

						memset(nmeaSnt, 0, 80);

						sprintf(nmeaSnt, "%s", token);

						// selecting only $GNGLL sentences, combined GPS and GLONASS
						// on my GPS sensor this good NMEA sentence is always 50 characters
						if ((strstr(nmeaSnt, "$GNGLL") != 0) && strlen(nmeaSnt) >= 49 && strstr(nmeaSnt, "*") != 0) {
							rawSum = strstr(nmeaSnt, "*");

							memcpy(smNmbr, &rawSum[1], 2);

							smNmbr[2] = '\0';

							uint8_t intSum = nmea0183_checksum(nmeaSnt);

							char hex[2];

							// "%X" unsigned hexadecimal integer (capital letters)
							sprintf(hex, "%X", intSum);

							// checksum data verification, if OK, then we can really trust
							// the data in the the NMEA sentence
							if (strstr(smNmbr, hex) != NULL) {

								//if we want display good $GNGLL NMEA sentences
								//HAL_UART_Transmit(&huart2, (uint8_t*)nmeaSnt, 50, 70);
								//HAL_UART_Transmit(&huart2, (uint8_t*) "\n\r", 2, 200);

								cnt = 0;

								// splitting the good NMEA sentence into the tokens by the comma delimiter
								for (char *pV = strtok(nmeaSnt, ","); pV != NULL; pV = strtok(NULL, ",")) {

									switch (cnt) {
									case 1:
										strcpy(latRawbuff, pV);
										latRaw = latRawbuff;
										break;
									case 2:
										strcpy(hemNSbuff, pV);
										hemNS = hemNSbuff;
										break;
									case 3:
										strcpy(lonRawbuff, pV);
										lonRaw = lonRawbuff;
										break;
									case 4:
										strcpy(hemEWbuff, pV);
										hemEW = hemEWbuff;
										break;
									case 5:
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
								sprintf(strLonMS, "%s", lonMS);

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

								//Coordinates/Timestamp:
//								newline();
//								newline();
//								print("Coordinates/Timestamp:");
//								newline();
//
//								print(hemNS);
//								print(" ");
//								print(latDg);
//								print("°");
//								print(latMS);
//								print("\', ");
//								print(hemEW);
//								print(" ");
//								print(lonDg);
//								print("°");
//								//print(strLonMS);
//								print(lonMS);
//
//								print("\', UTC: ");
//								println(strUTC);
//								newline();
								strcat(gps_data, hemNS);
								strcat(gps_data, " ");
								strcat(gps_data, latDg);
								strcat(gps_data, "°");
								strcat(gps_data, latMS);
								strcat(gps_data, "\', ");
								strcat(gps_data, hemEW);
								strcat(gps_data, " ");
								strcat(gps_data, lonDg);
								strcat(gps_data, "°");
								strcat(gps_data, lonMS);

								if (strcmp(choice, "coordinates") == 0) {
									return gps_data; //return just the coordinates.
								}

								strcat(gps_data, "\n\r");
								strcat(gps_data, strUTC);
								strcat(gps_data, "\n\r");

								if (strcmp(choice, "time") == 0) {
									strcpy(time, strUTC);
									return time; //return just the coordinates.
								}

							} // end of of the checksum data verification

						} // end of $GNGLL sentences selection


						//VTG
						// selecting only $GNVTG sentences, combined GPS and GLONASS
						// on my GPS sensor this good NMEA sentence is always 33 characters
						if ((strstr(nmeaSnt, "$GNVTG") != 0) && strlen(nmeaSnt) >= 32 && strstr(nmeaSnt, "*") != 0) {

							rawSum = strstr(nmeaSnt, "*");

							memcpy(smNmbr, &rawSum[1], 2);

							smNmbr[2] = '\0';

							uint8_t intSum = nmea0183_checksum(nmeaSnt);

							char hex[2];

							// "%X" unsigned hexadecimal integer (capital letters)
							sprintf(hex, "%X", intSum);

							// checksum data verification, if OK, then we can really trust
							// the data in the the NMEA sentence
							if (strstr(smNmbr, hex) != NULL) {

								//if we want display good $GNVTG NMEA sentences
								//HAL_UART_Transmit(&huart2, (uint8_t*)nmeaSnt, 33, 70);
								//HAL_UART_Transmit(&huart2, (uint8_t*) "\n\r", 2, 200);

								cnt = 0;

								// splitting the good NMEA sentence into the tokens by the comma delimiter
								for (char *pV = strtok(nmeaSnt, ","); pV != NULL; pV = strtok(NULL, ",")) {
									switch(cnt) {
										case 6:
											strcpy(ground_speed_buff, pV);
											ground_speed_raw = ground_speed_buff;
											break;
									}

									cnt++;

								}  // end for()
								memcpy(ground_speed_buff, &ground_speed_raw[0], 7);
								ground_speed_buff[7] = '\0';

								//Speed
//								newline();
//								newline();
//								println("Speed (kph):");
//								println(ground_speed_buff);
//								newline();

								if (strcmp(choice, "speed") == 0) {
									strcpy(speed, ground_speed_buff);
									return speed;
								}

								strcat(gps_data, ground_speed_buff);
								strcat(gps_data, "\n\r");

							} // end of of the checksum data verification

						} // end of $GNVTG sentences selection

						//GGA
						// selecting only $GNGGA sentences, combined GPS and GLONASS
						// on my GPS sensor this good NMEA sentence is always 72 characters
						if ((strstr(nmeaSnt, "$GNGGA") != 0) && strlen(nmeaSnt) >= 71 && strstr(nmeaSnt, "*") != 0) {

							rawSum = strstr(nmeaSnt, "*");

							memcpy(smNmbr, &rawSum[1], 2);

							smNmbr[2] = '\0';

							uint8_t intSum = nmea0183_checksum(nmeaSnt);

							char hex[2];

							// "%X" unsigned hexadecimal integer (capital letters)
							sprintf(hex, "%X", intSum);

							// checksum data verification, if OK, then we can really trust
							// the data in the the NMEA sentence
							if (strstr(smNmbr, hex) != NULL) {

								//if we want display good $GNGGA NMEA sentences
								//HAL_UART_Transmit(&huart2, (uint8_t*)nmeaSnt, 72, 70);
								//HAL_UART_Transmit(&huart2, (uint8_t*) "\n\r", 2, 200);

								cnt = 0;

								// splitting the good NMEA sentence into the tokens by the comma delimiter
								for (char *pV = strtok(nmeaSnt, ","); pV != NULL; pV = strtok(NULL, ",")) {

									switch(cnt) {
										case 9:
											strcpy(altitude, pV);
											break;
									}

									cnt++;

								}  // end for()

								//Altitude
//								newline();
//								newline();
//								println("Altitude (above mean sea level):");
//								println(altitude);
//								newline();

								if (strcmp(choice, "altitude") == 0) {
									println("test");
									strcpy(altitude_data, altitude);
									return altitude_data;
								}

								strcat(gps_data, altitude);

							} // end of of the checksum data verification

						} // end of $GNGAA sentences selection

					} // end of splitting the buffStr by the "\n" delimiter with the strsep() C function

					flag = 0; // we are ready to get new data from the sensor

//					println(gps_data);
//					newline();
//					newline();

					if (strcmp(choice, "all") == 0) {
						return gps_data;
					}



				} // end of one interrupt/full-buffer cycle

				HAL_Delay(200);

				/* USER CODE END WHILE */

				/* USER CODE BEGIN 3 */
			} //end while

}
