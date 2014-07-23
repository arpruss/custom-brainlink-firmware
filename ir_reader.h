/* Used for capturing IR signals. More detail in ir_reader.c. */


volatile uint8_t captured_signal[191]; // Temporary storage for the captured signal
volatile uint8_t signal_count = 0;      // Counts the number of signal edges seen (rises and falls)
volatile uint8_t ir_read_time_out = 0;  // Capturing times out if we fail to see anything after 3-4 seconds
volatile uint8_t ir_read_flag = 0;     

void init_ir_read(void);
void ir_fail(void);
void disable_ir_read(void);
void dump_data(void);
void write_data_to_eeprom(int position);
void read_data_from_eeprom(int position);
void print_data_from_eeprom(int position);
int read_data_from_serial(void);

