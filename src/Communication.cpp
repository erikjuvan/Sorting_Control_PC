#include "Communication.hpp"
#include <iostream>

Communication::Communication() : m_serial("", 256000) {
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
	
	if (ret)
		m_is_connected = true;

	return ret;
}

void Communication::Disconnect() {
	while (m_transfer_active); {
		m_is_connected = false;
		m_serial.close();
	}		
}

int Communication::GetRxBufferLen() {
	if (IsConnected())
		return m_serial.available();
	else
		return 0;
}

int Communication::Write(const void* data, int size) {
	if (IsConnected() && !m_transfer_active) {
		m_transfer_active = true;
		int ret = m_serial.write((uint8_t*)data, size);
		m_transfer_active = false;
		return ret;
	} else
		return 0;
}

int Communication::Read(void* data, int size) {
	if (IsConnected() && !m_transfer_active) {
		m_transfer_active = true;
		int ret = m_serial.read((uint8_t*)data, size);
		m_transfer_active = false;
		return ret;
	}
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