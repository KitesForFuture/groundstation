


#define RX_BUF_SIZE (1024)

#define TXD_PIN (GPIO_NUM_13)//17)
#define RXD_PIN (GPIO_NUM_12)//16)

#define UART UART_NUM_2

void initUART()
{
	uart_config_t uart_config = {
    	.baud_rate = 115200,
    	.data_bits = UART_DATA_8_BITS,
    	.parity = UART_PARITY_DISABLE,
    	.stop_bits = UART_STOP_BITS_1,
    	.flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
    	.source_clk = UART_SCLK_APB,
	};
	uart_driver_install(UART, RX_BUF_SIZE * 2, 0, 0, NULL, 0);
    uart_param_config(UART, &uart_config);
    uart_set_pin(UART, TXD_PIN, RXD_PIN, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
    
}

void sendUART(float number1, float number2){
	float d[4];
    d[0] = 314;
    d[1] = number1;
    d[2] = number2;
    d[3] = 314;
    
    uint8_t d_b[16];
    memcpy(d_b, d, 16);
    uint8_t to_be_sent[16];
    for(int i = 0; i < 4; i++){
    	for(int j = 0; j < 4; j++){
			memcpy(to_be_sent+j + 4*i, d_b+3-j + 4*i, 1);
		}
	}
    
    uart_write_bytes(UART, &to_be_sent, 16);
}

static uint8_t data[RX_BUF_SIZE+1];

int receiveUART(float* number1, float* number2){
	int length = 0;
	uart_get_buffered_data_len(UART, (size_t*)&length);
	if(length > 16){
       	const int rxBytes = uart_read_bytes(UART, data, RX_BUF_SIZE+1, 0);
       	if(rxBytes != 0){
       		uint8_t e_b[16];
       		//memcpy(e_b, data, 16);
       		for(int i = 0; i < 4; i++){
       			for(int j = 0; j < 4; j++){
	       			memcpy(e_b+j + 4*i, data+3-j + 4*i, 1);
	       		}
    	   	}
    	   	float e[4];
       		memcpy(e, e_b, 16);
       		if(e[0] == 314 && e[3] == 314){
	   		    //printf("received(receive attempt): %d, %d, %d, %d.\n", e[0], e[1], e[2], e[3]);
	   		    *number1 = e[1];
	   		    *number2 = e[2];
	   		    return 1;
	   		}
       	}
    }
    return 0;
}
























