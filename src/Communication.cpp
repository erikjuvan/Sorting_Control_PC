#include "Communication.hpp"
#include <iostream>

Communication::Communication() : m_serial("", 256000) {
}

Communication::~Communication() {
	Disconnect();
}

bool Communication::Connect(const std::string& port) {
	bool ret = true;

	try {
		m_serial.setPort(port);
		m_serial.open();
	}
	catch (...) {
		ret = false;
	}
	
	if (ret) {
		m_is_connected = true;
		SetTimeout(500);
	}		

	return ret;
}

void Communication::Disconnect() {
	m_is_connected = false;
	m_serial.close();	
}

size_t Communication::GetRxBufferLen() {	
	if (IsConnected())
		return m_serial.available();
	else
		return 0;
}

size_t Communication::Write(const void* data, int size) {
	if (IsConnected())
		return m_serial.write((uint8_t*)data, size);
	else
		return 0;
}

size_t Communication::Write(const std::string& data) {
	if (IsConnected())
		return m_serial.write(data);
	else
		return 0;
}

size_t Communication::Read(void* data, int size) {
	if (IsConnected())
		return m_serial.read((uint8_t*)data, size);
	else
		return 0;	
}

std::string Communication::Readline() {
	if (IsConnected())
		return m_serial.readline();
	else
		return 0;
}

void Communication::Flush() {
	if (IsConnected())
		m_serial.flush();
}

void Communication::Purge() {
	if (IsConnected())
		m_serial.purge();
}

void Communication::SetTimeout(int ms) {	
	m_serial.setTimeout(serial::Timeout::simpleTimeout(ms));
}
