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
	//std::lock_guard<std::mutex> lock(m_mutex);

	m_is_connected = false;
	m_serial.close();	
}

int Communication::GetRxBufferLen() {
	//std::lock_guard<std::mutex> lock(m_mutex);

	if (IsConnected())
		return static_cast<int>(m_serial.available());
	else
		return 0;
}

int Communication::Write(const void* data, int size) {	
	//std::lock_guard<std::mutex> lock(m_mutex);

	if (IsConnected()) {
		int ret = static_cast<int>(m_serial.write((uint8_t*)data, size));
		return ret;		
	} else
		return 0;
}

int Communication::Write(const std::string& data) {	
	//std::lock_guard<std::mutex> lock(m_mutex);

	if (IsConnected()) {
		int ret = static_cast<int>(m_serial.write(data));
		return ret;
	}
	else
		return 0;		
}

int Communication::Read(void* data, int size) {
	//std::lock_guard<std::mutex> lock(m_mutex);

	if (IsConnected()) {
		int ret = static_cast<int>(m_serial.read((uint8_t*)data, size));
		return ret;
	}
	else
		return 0;
}

void Communication::Flush() {
	//std::lock_guard<std::mutex> lock(m_mutex);

	if (IsConnected())
		m_serial.flush();
}

void Communication::Purge() {
	//std::lock_guard<std::mutex> lock(m_mutex);

	if (IsConnected())
		m_serial.purge();
}