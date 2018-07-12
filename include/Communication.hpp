#pragma once

#include <serial\serial.h>

class Communication {	

public:
	Communication();

	bool Connect(const std::string& port);
	void Disconnect();
	bool IsConnected();
	int GetRxBufferLen();	
	int Write(const void* data, int size);
	int Read(void* data, int size);
	void Flush();
	void Purge();

private:
	serial::Serial	m_serial;
	bool			m_transfer_active{ false };
	bool			m_is_connected{ false };
};

inline bool Communication::IsConnected() {
	return m_is_connected; 
}