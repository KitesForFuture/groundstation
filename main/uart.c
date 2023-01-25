#define RX_BUF_SIZE (1024)

typedef struct uart_data
{
	uart_port_t uart_num;
	float currentMessage[100];
	int current_index;
	int reading;
	int byteOrderReversed;
	
} uart_data;

static uart_data uart[2];
static int first_uart_already_initialized = false;

//tx_pin/rx_pin = GPIO_NUM_13 ...
//uart_num = UART_NUM_1 or UART_NUM_2
static void initUART(int number/*either 0 or 1*/, gpio_num_t tx_pin, gpio_num_t rx_pin, int byteOrderReversed)
{
	if(first_uart_already_initialized){
		uart[number].uart_num = UART_NUM_2;
	}else{
		uart[number].uart_num = UART_NUM_1;
		first_uart_already_initialized = true;
	}
	
	uart[number].current_index = 0;
	uart[number].reading = false;
	uart[number].byteOrderReversed = byteOrderReversed;
	
	uart_config_t uart_config = {
    	.baud_rate = 115200,
    	.data_bits = UART_DATA_8_BITS,
    	.parity = UART_PARITY_DISABLE,
    	.stop_bits = UART_STOP_BITS_1,
    	.flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
    	.source_clk = UART_SCLK_APB,
	};
	uart_driver_install(uart[number].uart_num, RX_BUF_SIZE * 2, 0, 0, NULL, 0);
    uart_param_config(uart[number].uart_num, &uart_config);
    uart_set_pin(uart[number].uart_num, tx_pin, rx_pin, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
    
}

static void reversedByteOrderCopy(float* destinationArray, float* sourceArray, int destinationIndex, int sourceIndex){
	for(int j = 0; j < 4; j++){
		memcpy(destinationArray+j + 4*destinationIndex, sourceArray+3-j + 4*sourceIndex, 1);
	}
}

void sendUARTArray100(float* array, int length, int number){
	float d[102];
	d[0] = INFINITY;
	for(int i = 0; i < length; i++){
		d[i+1] = array[i];
	}
	d[length+1] = -INFINITY;
	if(uart[number].byteOrderReversed){
		printf("!!!Not implemented byte order reversing yet in the function sendUARTArray100!!!\n");
	}
	uint8_t to_be_sent[102*4];
	memcpy(to_be_sent, d, 102*4);
	uart_write_bytes(uart[number].uart_num, &to_be_sent, 102*4);
}

void sendUART(float number1, float number2, int number){
	float d[4];
    d[0] = INFINITY;
    d[1] = number1;
    d[2] = number2;
    d[3] = -INFINITY;
    
    float r_d[4];
    
    for(int i = 0; i < 4; i++){
    	if(uart[number].byteOrderReversed){
    		reversedByteOrderCopy(r_d, d, i, i);
    	}else{
    		r_d[i] = d[i];
    	}
	}
    
    uint8_t to_be_sent[16];
    memcpy(to_be_sent, r_d, 16);
    
    uart_write_bytes(uart[number].uart_num, &to_be_sent, 16);
}

static uint8_t data[RX_BUF_SIZE];
static float float_data[RX_BUF_SIZE/4];


// returns 0 if reading is waiting for new incoming bytes
// else returns length of message
int processUART(int number, float* message){
	int uart_buffered_data_byte_length = 0;
	uart_get_buffered_data_len(uart[number].uart_num, (size_t*)&uart_buffered_data_byte_length);
	int uart_buffered_data_float_length = (uart_buffered_data_byte_length)/4;
	if(uart_buffered_data_byte_length > RX_BUF_SIZE) uart_buffered_data_byte_length = RX_BUF_SIZE;
	
	//READ uart BUFFER
	//const int number_of_read_bytes = 
	uart_read_bytes(uart[number].uart_num, data, uart_buffered_data_byte_length, 0);
	
	//CONVERT to FLOAT
	memcpy(float_data, data, uart_buffered_data_float_length*4);
	
	for(int i = 0; i < uart_buffered_data_float_length; i++){
		
		if(uart[number].reading && float_data[i] == -INFINITY){
			//DONE READING, message complete, saving message and RETURNING
			memcpy(message, uart[number].currentMessage, uart[number].current_index*4);
			uart[number].reading = false;
			return uart[number].current_index;
		}
		if(uart[number].reading){
			//APPEND to currentMessage
			if(uart[number].byteOrderReversed){
				reversedByteOrderCopy(uart[number].currentMessage, float_data, uart[number].current_index, i);
			}else{
				uart[number].currentMessage[uart[number].current_index] = float_data[i];
			}
			
			uart[number].current_index += (uart[number].current_index < 99) ? 1 : 0;
		}else if(float_data[i] == INFINITY){
			//START READING
			uart[number].reading = true;
			uart[number].current_index = 0;
		}
	}
	// message not yet complete
	return 0;
}

