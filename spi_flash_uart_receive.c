

/*****************************************************************************************
  Filename   :    spi_flash_uart_receive.c
  Description:    uart_receive
  
  Generated by Musa ÇUFALCI 2018
  Copyright (c) Musa ÇUFALCI All rights reserved.
/******************************************************************************************

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netdb.h>
#include <stdint.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"

#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event_loop.h"
#include "esp_err.h"
#include "esp_log.h"
#include "esp_ota_ops.h"
#include "esp_partition.h"

#include "nvs.h"
#include "nvs_flash.h"

#include "driver/gpio.h"
#include "driver/uart.h"
#include "driver/i2s.h"

#include "sdkconfig.h"
#include "esp_spi_flash.h"
#include "rom/spi_flash.h"
#include "rom/uart.h"

#include "cJSON.h"

//***********************************************************************************************

#define delay(ms) (vTaskDelay(ms/portTICK_RATE_MS));
#define UARTB_TXD (18)
#define UARTB_RXD (19)
#define UARTA_TXD (17)
#define UARTA_RXD (16)
#define LED1 25
#define LED2 26
#define DEFAULT_BAUD 921600

#define BUFFSIZE 256
#define UART_BUFFSIZE 256
#define PACKETSIZE 1024
#define HEXSTRINGSIZE 128

//***********************************************************************************************

int char2int(char input);

void backtofactory()
{
    esp_partition_iterator_t  pi ;                                  
    const esp_partition_t*    factory ;                             
    esp_err_t                 err ;

    pi = esp_partition_find ( ESP_PARTITION_TYPE_APP,             
                              ESP_PARTITION_SUBTYPE_APP_OTA_0,    
                              "ota_0" ) ;
    if ( pi == NULL )                                             
    {
    }
    else
    {
        factory = esp_partition_get ( pi ) ;                        
        esp_partition_iterator_release ( pi ) ;                    
        err = esp_ota_set_boot_partition ( factory ) ;              
        if ( err != ESP_OK )                                        
   {

   }
   else
   {
            esp_restart() ;                                         // Restart ESP
        }
    }
}

//***********************************************************************************************

void Write_flash_Task(void *pvParameters)
{
	uint8_t receive_buffer[PACKETSIZE];
	uint8_t json_buffer[PACKETSIZE];
	memset (receive_buffer,0,PACKETSIZE);
	memset (json_buffer,0,PACKETSIZE);

	unsigned char val[HEXSTRINGSIZE];
	memset (val, 0, HEXSTRINGSIZE);

	int say1=0;
	int b=0;
	int i=0;
	char bul[1];
	memset(bul, 0, 1);
	bul[0] = '{';
	bul[1] = '}';
	//		unsigned char temp[BUFFSIZE]={0};
	//		unsigned char temp2[1]={0};

	//		char * FD_JSON_read="";
	//uint32_t FD_JSON_read=0;
	//char * FC_JSON="";
	//		unsigned char temp[BUFFSIZE]= {0};

	int count_erase = 32;
	int packet_count=0;
	int c=0;
	//		char tmpByte[128];
	//		char backHex[257];


//***********************************************************************************************

	esp_partition_iterator_t  pi ;                                  
	const esp_partition_t*    factory ;                             
	esp_err_t                 err ;

	pi = esp_partition_find ( ESP_PARTITION_TYPE_APP,             
	ESP_PARTITION_SUBTYPE_APP_OTA_0,    "ota_0" ) ;

	factory = esp_partition_get ( pi ) ;                        
	
	
//***********************************************************************************************
	uint32_t base_addr = factory->address;
//***********************************************************************************************


	//UART1 configuration
	const int uart_num1 = UART_NUM_1;
	uart_config_t uart_config1 = {
		.baud_rate = DEFAULT_BAUD,
		.data_bits = UART_DATA_8_BITS,
		.parity    = UART_PARITY_DISABLE,
		.stop_bits = UART_STOP_BITS_1,
		.flow_ctrl = UART_HW_FLOWCTRL_DISABLE,    //UART_HW_FLOWCTRL_CTS_RTS,
		.rx_flow_ctrl_thresh = 122,
	};
	
	//----------------------------------------------------------------------------------------
	
	//Configure UART1 parameters
	uart_param_config(uart_num1, &uart_config1);
	uart_set_pin(uart_num1, UARTB_TXD, UARTA_RXD, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
	uart_driver_install(uart_num1, PACKETSIZE, 0, 0, NULL, 0);

	while(1)
	{
		//Read data from UARTA
		int len1 = uart_read_bytes(uart_num1, receive_buffer, PACKETSIZE, 0);
		if(len1 > 0) {
			i=0;
			for (i = 0; i < len1; i++) {
				if (receive_buffer[i] == bul[0])
				{
					say1++;
				}
				if(say1==1) {
					json_buffer[b] = receive_buffer[i];
					b++;
				}
				if (receive_buffer[i] == bul[1])
				{
					say1=0;


					b=0;

					cJSON* FR_packet = cJSON_Parse((char*)json_buffer);
					char* FD_JSON = cJSON_GetObjectItem(FR_packet, "FD")->valuestring;


					char raw_fdjson[128] = { 0 };
					char temp[2] = { 0 };
					for (c = 0; c < 128; c++)
					{
						temp[0] = char2int(FD_JSON[c * 2]);
						temp[1] = char2int(FD_JSON[c * 2 + 1]);
						raw_fdjson[c] = (temp[0] * 0x10) + temp[1];
					}


					int FC_JSON = cJSON_GetObjectItem(FR_packet,"FC")->valueint;
					printf("FC_JSON: %d\r\n", FC_JSON);

					packet_count = FC_JSON;

					int size_FDJSON= strlen(FD_JSON);


					if (count_erase == 32){
						count_erase = 0;
						spi_flash_erase_sector(base_addr / 4096);
						delay(10);
					}

//					int size_val = sizeof(val);
//					printf("\nsize_val:  %d\r", size_val);

					spi_flash_write(base_addr, (uint32_t*)raw_fdjson, HEXSTRINGSIZE);
					delay(1);
					base_addr += HEXSTRINGSIZE;

					printf("\npacket_count: %d\r\n", packet_count);
					count_erase++;

					if(packet_count==1130) {
						printf("\npacket_count: %d\r\n", packet_count);
						packet_count=0;
						printf("Write_flash test done\n");
						memset (json_buffer, 0, PACKETSIZE);
						cJSON_Delete(FR_packet);

						memset (receive_buffer, 0, PACKETSIZE);
						backtofactory();
					}

	//***********************************************************************************************
	
					memset (json_buffer, 0, PACKETSIZE);
					cJSON_Delete(FR_packet);
				}
			}
			memset (receive_buffer, 0, PACKETSIZE);
		}
		delay(10);
	}
	delay(10);
	vTaskDelete(NULL);
}
//***********************************************************************************************
void app_main()
{

xTaskCreatePinnedToCore(&Write_flash_Task, "Write_flash_Task", 8192, NULL, 12, NULL, 0);
delay(1);

while(1){
	delay(1000);
    ESP_LOGI("main","heap:%d",xPortGetFreeHeapSize());
    }
}

////***********************************************************************************************
//				cJSON_Delete(FR_packet);
////***********************************************************************************************


int char2int(char input)
{
  if(input >= '0' && input <= '9')
    return input - '0';
  if(input >= 'A' && input <= 'F')
    return input - 'A' + 10;
  if(input >= 'a' && input <= 'f')
    return input - 'a' + 10;
//  throw std::invalid_argument("Invalid input string");
  return 0;
}

//----------------------------------------------------------------------------------------


