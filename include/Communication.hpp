#pragma once

#include <mutex>
#include <serial/serial.h>

class Communication
{
public:
    Communication();
    ~Communication();

    bool        Connect(const std::string& port);
    void        Disconnect();
    inline bool IsConnected() { return m_is_connected; }
    size_t      GetRxBufferLen();
    size_t      Write(const void* data, int size);
    size_t      Write(const std::string& data);
    size_t      Read(void* data, int size);
    std::string Readline();
    void        Flush();
    void        Purge();
    void        SetTimeout(int ms);

private:
    serial::Serial m_serial;
    volatile bool  m_is_connected{false};
};
