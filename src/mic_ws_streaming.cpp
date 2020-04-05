// ----------------------------------------
// Includes
// ----------------------------------------
#include "mic_ws_streaming.h"
#include <WebSocketsServer.h>
#include <freertos/queue.h>
#include <math.h>

// ----------------------------------------
// Defines
// ----------------------------------------
#define BUFF_SIZE 4000
#define DATA_RATE 8000

// typedef struct{
//     int idBuff
// }

// ----------------------------------------
// global variables
// ----------------------------------------
static WebSocketsServer webSocketAudio = WebSocketsServer(82);
static float audioBuffer1[BUFF_SIZE];
static float audioBuffer2[BUFF_SIZE];

static BaseType_t xReturnedWSHandler;
static BaseType_t xReturnedAudioStream;
QueueHandle_t xQueueAudioBuffId = NULL;


// ----------------------------------------
// Functions
// ----------------------------------------
void onAudioWebSocketEvent(uint8_t client_num, WStype_t type, uint8_t * payload, size_t length);
void vTaskAudioWSHandler( void * pvParameters );
void vTaskAudioStream( void * pvParameters );

// Init function
void micStreamingInit(){
    webSocketAudio.begin();
    webSocketAudio.onEvent(onAudioWebSocketEvent);
    xQueueAudioBuffId = xQueueCreate( 10, sizeof( unsigned )  );

    /* Create the task, storing the handle. */
    xReturnedWSHandler = xTaskCreate(vTaskAudioWSHandler, "vTaskAudioWSHandler",  4096,  ( void * ) NULL,  2, NULL);
    xReturnedAudioStream = xTaskCreate(vTaskAudioStream, "vTaskAudioWSHandler",  4096,  ( void * ) NULL,  2, NULL);
}


// Callback: receiving any WebSocket message
void onAudioWebSocketEvent(uint8_t client_num,
                      WStype_t type,
                      uint8_t * payload,
                      size_t length) {
  //String for ws reply message
  String out_string;

  // Figure out the type of WebSocket event
  switch(type) {
 
    // Client has disconnected
    case WStype_DISCONNECTED:
      Serial.printf("[%u] Disconnected!\n", client_num);
      break;
 
    // New client has connected
    case WStype_CONNECTED:
      {
        IPAddress ip = webSocketAudio.remoteIP(client_num);
        Serial.printf("[%u] Connection from ", client_num);
        Serial.println(ip.toString());
      }
      break;
 
    // Handle text messages from client
    case WStype_TEXT:
      
      // Print out raw message
      Serial.printf("[%u] Received text: %s\n", client_num, payload);
 
     //Prepare return string
      out_string += "echo: \"";
      out_string += String((char *)payload);
      out_string += "\"\n\nThanks for the message";

      webSocketAudio.sendTXT(client_num, out_string.c_str());
      break;

    // For everything else: do nothing
    case WStype_BIN:
    case WStype_ERROR:
    case WStype_FRAGMENT_TEXT_START:
    case WStype_FRAGMENT_BIN_START:
    case WStype_FRAGMENT:
    case WStype_FRAGMENT_FIN:
    default:
      break;
  }
}

//taks that handles websocket comunication 
void vTaskAudioWSHandler( void * pvParameters )
{
    //configASSERT( 1 );
    TickType_t xLastWakeTime;
    const TickType_t xFrequency = pdMS_TO_TICKS(100); // every 100 ms handle the websocket and verify if new audio buffer is available for broadcasting
    int buff_id = 0; //id of the buffer containing the audio stream

    for( ;; )
    {
        // Wait for the next cycle.
        vTaskDelayUntil( &xLastWakeTime, xFrequency );
        webSocketAudio.loop();
        if( xQueueReceive( xQueueAudioBuffId, &( buff_id ), ( TickType_t ) 0 ) == pdPASS )
        {
            if(buff_id == 0)
            {
                webSocketAudio.broadcastBIN((uint8_t *)audioBuffer1, BUFF_SIZE * sizeof(audioBuffer1[0]));
                Serial.printf("Spedito buffer %d, [0]: %f, [10]: %f [20]: %f\n",1,audioBuffer1[0],audioBuffer1[10],audioBuffer1[20]);
            }
            else
            {
                webSocketAudio.broadcastBIN((uint8_t *)audioBuffer2, BUFF_SIZE * sizeof(audioBuffer2[0]));
                Serial.printf("Spedito buffer %d, [0]: %f, [10]: %f [20]: %f\n",2,audioBuffer2[0],audioBuffer2[10],audioBuffer2[20]);
            }
        }
    }
}

//task that generates a stream of fake audio buffer
void vTaskAudioStream( void * pvParameters )
{
    TickType_t xLastWakeTime;
    const TickType_t xFrequency = pdMS_TO_TICKS(1000);
    int buff_id = 0;

    // stub
    unsigned freq_1 = 500; //Hz
    unsigned freq_2 = 200; //Hz
    unsigned freq = 0;
    const float timeStep = 1.0f/DATA_RATE;

    for( ;; )
    {
        // Wait for the next cycle.
        vTaskDelayUntil( &xLastWakeTime, xFrequency );
        
        //generate fake wave
        float time = 0;
        if(buff_id == 0) 
        {
            freq = freq_1;
            for (unsigned ui = 0; ui < BUFF_SIZE; ui++) {
                audioBuffer1[ui] = sinf(2*PI*freq*time);
                time += timeStep;
            }
        }
        else 
        {
            freq = freq_2;
            for (unsigned ui = 0; ui < BUFF_SIZE; ui++) {
                audioBuffer2[ui] = sinf(2*PI*freq*time);
                time += timeStep;
            }
        }        

        /* Task code goes here. */
        if(xQueueSend(xQueueAudioBuffId, ( void * ) &buff_id, ( TickType_t ) 0 ) != pdPASS )
        {
            Serial.println("xQueueAudioBuffId queue is full...");
        }
        //Toggle_id
        buff_id++;
        if(buff_id > 1) buff_id = 0;
    }
}



// Example from: https://github.com/0015/IdeasNProjects/tree/master/ESP32_MICROPHONE
// video tutorial: https://www.youtube.com/watch?v=m8LwPNXqK9o

// #include <driver/i2s.h>
// #define I2S_WS 15
// #define I2S_SD 13
// #define I2S_SCK 2

// #define I2S_PORT I2S_NUM_0

// void setup() {
//   Serial.begin(115200);
//   Serial.println("Setup I2S ...");

//   delay(1000);
//   i2s_install();
//   i2s_setpin();
//   i2s_start(I2S_PORT);
//   delay(500);
// }

// void loop() {
//   int32_t sample = 0;
//   int bytes = i2s_pop_sample(I2S_PORT, (char*)&sample, portMAX_DELAY);
//   if(bytes > 0){
//     Serial.println(sample);
//   }
// }

// void i2s_install(){
//   const i2s_config_t i2s_config = {
//     .mode = i2s_mode_t(I2S_MODE_MASTER | I2S_MODE_RX),
//     .sample_rate = 44100,
//     .bits_per_sample = i2s_bits_per_sample_t(16),
//     .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT,
//     .communication_format = i2s_comm_format_t(I2S_COMM_FORMAT_I2S | I2S_COMM_FORMAT_I2S_MSB),
//     .intr_alloc_flags = 0, // default interrupt priority
//     .dma_buf_count = 8,
//     .dma_buf_len = 64,
//     .use_apll = false
//   };

//   i2s_driver_install(I2S_PORT, &i2s_config, 0, NULL);
// }

// void i2s_setpin(){
//   const i2s_pin_config_t pin_config = {
//     .bck_io_num = I2S_SCK,
//     .ws_io_num = I2S_WS,
//     .data_out_num = -1,
//     .data_in_num = I2S_SD
//   };

//   i2s_set_pin(I2S_PORT, &pin_config);
// }