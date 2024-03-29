#include "num_config_vars.h"
#include "timer.c"

#define RC_MODE 0
#define DATA_MODE 1
#define LINE_TENSION_REQUEST_MODE 2
#define LINE_LENGTH_MODE 3
#define CONFIG_MODE 4

#define DATALENGTH 2


static uint8_t broadcast_mac[] = { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF };
const uint8_t WIFI_CHANNEL = 0;

float receivedData[DATALENGTH] = {0.0, 0.0};//TODO: needed?
float tension_request = 0;

static void (*receive_config_callback)(float*);
static void (*receive_debugging_data_callback)(float*);

typedef struct __attribute__((packed)) esp_now_msg_t
{
	uint32_t mode;
	float data[DATALENGTH];
	// Can put lots of things here
} esp_now_msg_t;

typedef struct __attribute__((packed)) esp_now_msg_t_large
{
	uint32_t mode;
	float data[NUM_CONFIG_FLOAT_VARS];
} esp_now_msg_t_large;

typedef struct __attribute__((packed)) esp_now_msg_t_medium
{
	uint32_t mode;
	float data[6];
} esp_now_msg_t_medium;

//TODO: I think this is not needed anymore, as the groundstation does not receive data from the kite
static void msg_recv_cb(const uint8_t *mac_addr, const uint8_t *data, int len)
{
	printf("received esp-now message of size %d\n", len);
	if (len == sizeof(esp_now_msg_t))
	{
		esp_now_msg_t msg;
		memcpy(&msg, data, len);
		if(msg.mode == LINE_TENSION_REQUEST_MODE){
			tension_request = msg.data[0];
		}
	}
	
	if (len == sizeof(esp_now_msg_t_large))
	{
		esp_now_msg_t_large msg;
		memcpy(&msg, data, len);
		if(msg.mode == CONFIG_MODE){
			(*receive_config_callback)(msg.data);
		}
	}
	
	if (len == sizeof(esp_now_msg_t_medium)){
		esp_now_msg_t_medium msg;
		memcpy(&msg, data, len);
		(*receive_debugging_data_callback)(msg.data);
		
	}
}


static void msg_send_cb(const uint8_t* mac, esp_now_send_status_t sendStatus)
{
	//printf("send_cb called");
	switch (sendStatus){
		case ESP_NOW_SEND_SUCCESS:
			//printf("Send success\n");
			break;
		
		case ESP_NOW_SEND_FAIL:
			//printf("Send Failure\n");
			break;
		
		default:
			break;
	}
}




// init wifi on the esp
// register callbacks
void network_setup(void (*received_config_callback_arg)(float*), void (*receive_debugging_data_callback_arg)(float*))
{
	receive_config_callback = received_config_callback_arg;
	receive_debugging_data_callback = receive_debugging_data_callback_arg;
	//TODO: check if neccessary when WIFI_STORAGE_RAM is used
	// Initialize FS NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK( nvs_flash_erase() );
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK( ret );
	
	
	//ESP_ERROR_CHECK(esp_netif_init());
	
	// Wifi
	wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
	esp_wifi_init(&cfg);
	
	
    esp_wifi_set_storage(WIFI_STORAGE_RAM);
	// Puts ESP in STATION MODE
	
	esp_wifi_set_mode(WIFI_MODE_STA); // MUST BE CALLED AFTER esp_wifi_init(&cfg) to have a non-volatile effect on the flash nvs!
	
	esp_wifi_config_80211_tx_rate(ESP_IF_WIFI_STA, WIFI_PHY_RATE_LORA_250K);// TODO: does this influence the range?
	
	esp_wifi_start();
	
	ESP_ERROR_CHECK( esp_wifi_set_protocol(ESP_IF_WIFI_STA, WIFI_PROTOCOL_LR) );
	
	
	
	// ESP NOW
	esp_now_init();
	
	esp_now_peer_info_t peer_info;
	peer_info.channel = WIFI_CHANNEL;
	memcpy(peer_info.peer_addr, broadcast_mac, 6);
	peer_info.ifidx = ESP_IF_WIFI_STA;
	peer_info.encrypt = false;
	esp_now_add_peer(&peer_info);
	
	// Register Send Callback
	esp_now_register_send_cb(msg_send_cb);
	
	// Register Receive Callback
	esp_now_register_recv_cb(msg_recv_cb); //TODO: I believe this is not needed anymore.
}

// used by the kite to send data to the data receiver
void sendDataArray(float data[DATALENGTH], uint32_t mode){

	esp_now_msg_t msg;
	
	msg.mode = mode;
	for(int i = 0; i < DATALENGTH; i++){
		msg.data[i] = data[i];
	}
	
	// Pack
	uint16_t packet_size = sizeof(esp_now_msg_t);
	uint8_t msg_data[packet_size]; // Byte array
	memcpy(&msg_data[0], &msg, sizeof(esp_now_msg_t));
	
	// Send
	esp_now_send(broadcast_mac, msg_data, packet_size);
}

void sendData(uint32_t mode, float data0, float data1){
	
	float to_be_sent[DATALENGTH];
	for(int i = 0; i < DATALENGTH; i++){
		to_be_sent[i] = 0;
	}
	to_be_sent[0] = data0;
	to_be_sent[1] = data1;
	sendDataArray(to_be_sent, mode);
}


void sendDataArrayLarge(uint32_t mode, float* data, int length){
	esp_now_msg_t_large msg;
	msg.mode = mode;
	for(int i = 0; i < length; i++){
		msg.data[i] = data[i];
	}
	// Pack
	uint16_t packet_size = sizeof(esp_now_msg_t_large);
	uint8_t msg_data[packet_size]; // Byte array
	memcpy(&msg_data[0], &msg, sizeof(esp_now_msg_t_large));
	
	// Send
	esp_now_send(broadcast_mac, msg_data, packet_size);
}


