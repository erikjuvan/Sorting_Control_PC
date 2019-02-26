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
    size_t      Write(const void* m_data, int size);
    size_t      Write(const std::string& m_data);
    size_t      Read(void* m_data, int size);
    std::string Readline();
    void        Flush();
    void        Purge();
    void        SetTimeout(int ms);

private:
    std::mutex     m_mtx;
    serial::Serial m_serial;
    bool           m_is_connected{false};
};
