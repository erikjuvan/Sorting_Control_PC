#include "Communication.hpp"
#include <iostream>

Communication::Communication() :
    m_serial("", 460800)
{
}

Communication::~Communication()
{
    Disconnect();
}

bool Communication::Connect(const std::string& port)
{
    std::scoped_lock<std::mutex> sl(m_mtx);

    bool ret = true;

    try {
        m_serial.setPort(port);
        m_serial.open();
    } catch (...) {
        ret = false;
    }

    if (ret) {
        m_is_connected = true;
    }

    return ret;
}

void Communication::Disconnect()
{
    std::scoped_lock<std::mutex> sl(m_mtx);
    m_is_connected = false;
    m_serial.close();
}

size_t Communication::GetRxBufferLen()
{
    std::scoped_lock<std::mutex> sl(m_mtx);
    if (IsConnected())
        return m_serial.available();
    else
        return 0;
}

size_t Communication::Write(const void* m_data, int size)
{
    std::scoped_lock<std::mutex> sl(m_mtx);
    if (IsConnected())
        return m_serial.write((uint8_t*)m_data, size);
    else
        return 0;
}

size_t Communication::Write(const std::string& m_data)
{
    std::scoped_lock<std::mutex> sl(m_mtx);
    if (IsConnected())
        return m_serial.write(m_data);
    else
        return 0;
}

size_t Communication::Read(void* m_data, int size)
{
    std::scoped_lock<std::mutex> sl(m_mtx);
    if (IsConnected())
        return m_serial.read((uint8_t*)m_data, size);
    else
        return 0;
}

std::string Communication::Readline()
{
    std::scoped_lock<std::mutex> sl(m_mtx);
    if (IsConnected())
        return m_serial.readline();
    else
        return 0;
}

void Communication::Flush()
{
    std::scoped_lock<std::mutex> sl(m_mtx);
    if (IsConnected())
        m_serial.flush();
}

void Communication::Purge()
{
    std::scoped_lock<std::mutex> sl(m_mtx);
    if (IsConnected())
        m_serial.purge();
}

void Communication::SetTimeout(int ms)
{
    std::scoped_lock<std::mutex> sl(m_mtx);

    auto to = serial::Timeout::simpleTimeout(ms);
    m_serial.setTimeout(to);
}
