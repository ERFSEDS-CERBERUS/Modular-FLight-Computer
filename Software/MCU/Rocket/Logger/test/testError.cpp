#include <iostream>
#include "Logger.hpp"
#include <iostream>
#include <string>


void flash_write_callback(const char * msg, const size_t size);
void transmit_callback(const char * msg, const size_t size);




//Should print [ERROR]: Test Error
int main() {
	Logger::setTransmitCallback(&transmit_callback);
	Logger::setFlashWriteCallback(&flash_write_callback);
	Logger::SetLogLevelUSB(ERROR);
	Logger::Error("Test Error", 11);
	return 0;
}


void flash_write_callback(const char * msg, const size_t size) {
	std::string message(msg, size);
	std::cout << "[Store]: " << message << std::endl;	
}
void transmit_callback(const char * msg, const size_t size) {
	std::string message(msg, size);
	std::cout << "[Transmit]: "  << message << std::endl;
}
