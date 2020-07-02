#include "Communication.hpp"
#include "Helpers.hpp"
#include <iostream>
#include <thread>

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
        return std::string("");
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

void Communication::StopTransmissionAndSuperPurge()
{
    using namespace std::chrono_literals;

    // Stop transmission
    Write("VRBS,0\n");

    // Make sure there is some data in rx buffer
    int cnt = 0;
    while (GetRxBufferLen() <= 0) {
        // Safety, to prevent while (true)
        std::this_thread::sleep_for(10ms);
        if (++cnt > 10) {
            std::cerr << "ERROR: StopTransmissionAndSuperPurge: no data in buffer" << std::endl;
            break;
        }
    }

    // Clear data while it's there
    for (auto data_in_buf = GetRxBufferLen(); data_in_buf > 0; data_in_buf = GetRxBufferLen()) {
        Purge();
        Flush();
        std::this_thread::sleep_for(10ms);
    }

    // Make sure we are really stopped by sending the command and checking for confirmation
    Write("VRBS,0\n");
    ConfirmTransmission("VRBS,0\n");
}

void Communication::ConfirmTransmission(std::string const& str)
{
    auto ret_str    = Readline();
    auto tokens     = Help::TokenizeString(str, ", \n");
    auto tokens_ret = Help::TokenizeString(ret_str, ", \n");
    bool fault      = false;
    auto tmp_str    = str;
    if (tmp_str.back() == '\n')
        tmp_str.pop_back();
    std::string error_msg = "Transmission failed when sending: \"" + tmp_str + "\": ";

    // Command name
    if (tokens_ret[0] != tokens[0]) {
        error_msg += "Command name mismatch: expected: '" + tokens[0] + "' received: '" + tokens_ret[0] + "'\n";
        throw std::runtime_error(error_msg);
    }

    // Check for number of tokens mismatch
    if (tokens.size() != tokens_ret.size()) {
        error_msg += "Argument number mismatch: expected: " + std::to_string(tokens.size() - 1) + " received: " + std::to_string(tokens_ret.size() - 1) + "\n";
        throw std::runtime_error(error_msg);
    }

    // Check arguments
    if (tokens.size() > 1) {
        for (int i = 1; i < tokens.size(); ++i) {
            // Arguments are always numeric
            auto f     = std::stof(tokens[i]);
            auto f_ret = std::stof(tokens_ret[i]);
            if (std::abs(f - f_ret) > 0.1) {
                error_msg += "Argument number " + std::to_string(i) + " mismatch: expected: " + tokens[i] + " received: " + tokens_ret[i] + "\n";
                throw std::runtime_error(error_msg);
            }
        }
    }
}

std::vector<std::string> Communication::WriteAndTokenizeiResult(std::string const& str)
{
    Write(str);
    auto ret_data = Readline();
    if (ret_data.back() == '\n')
        ret_data.pop_back();

    auto str_vec = Help::TokenizeString(ret_data, ",\n");

    // Remove first string (command name) and only keep results
    str_vec.erase(str_vec.begin());
    return str_vec;
}