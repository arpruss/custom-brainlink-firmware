// Helper functions for setting outputs and reading inputs on the auxiliary digital I/O. See ext_dio.c for more.

void set_output(char port, char value);
static char read_input(char port);