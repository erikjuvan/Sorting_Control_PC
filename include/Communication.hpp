#pragma once

#include <serial\serial.h>
#include <mutex>

class Communication {	

public:
	Communication();

	bool Connect(const std::string& port);
	void Disconnect();
	bool IsConnected();
	int GetRxBufferLen();	
	int Write(const void* data, int size);
	int Write(const std::string& data);
	int Read(void* data, int size);
	void Flush();
	void Purge();

private:
	serial::Serial			m_serial;	
	volatile bool			m_is_connected{ false };
	
	std::mutex				m_mutex;	// mutex does not seem to be neccessary
};

inline bool Communication::IsConnected() {
	return m_is_connected; 
}