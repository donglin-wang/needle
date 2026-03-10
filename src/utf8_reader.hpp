#pragma once

#include <cstring>
#include <fstream>
#include <string>
#include <vector>

class UTF8Reader
{
    std::vector<uint32_t> data;
    const std::string path;
    size_t buffer_size = 1000;

public:
    explicit UTF8Reader(const std::string &path, size_t buffer_size = 1000)
        : path(path), buffer_size(buffer_size) {}

    void read()
    {
        std::vector<uint8_t> buffer(this->buffer_size);
        std::ifstream file(path, std::ios::binary);
        size_t num_carried = 0;

        while (file.read(reinterpret_cast<char *>(buffer.data() + num_carried),
                         buffer.size() - num_carried) ||
               file.gcount() > 0)
        {
            size_t total = num_carried + file.gcount();
            num_carried = this->parse_buffer(buffer, total);
            std::memmove(buffer.data(), buffer.data() + (total - num_carried),
                         num_carried);
        }
    }

    const std::vector<uint32_t> &get_data() const { return data; }
    std::vector<uint32_t> &get_data() { return data; }

    size_t parse_buffer(const std::vector<uint8_t> &buffer, size_t total)
    {
        for (size_t i = 0; i < total;)
        {
            uint8_t byte = buffer[i];
            size_t len;
            uint32_t code_point;

            if (byte < 0x80)
            {
                code_point = byte;
                len = 1;
            }
            else if ((byte & 0xE0) == 0xC0)
            {
                code_point = byte & 0x1F;
                len = 2;
            }
            else if ((byte & 0xF0) == 0xE0)
            {
                code_point = byte & 0x0F;
                len = 3;
            }
            else if ((byte & 0xF8) == 0xF0)
            {
                code_point = byte & 0x07;
                len = 4;
            }
            else
            {
                data.push_back(0xFFFD);
                i++;
                continue;
            }

            if (i + len > total)
                return total - i;

            for (size_t j = 1; j < len; j++)
            {
                if ((buffer[i + j] & 0xC0) != 0x80)
                {
                    code_point = 0xFFFD;
                    len = j;
                    break;
                }
                code_point = (code_point << 6) | (buffer[i + j] & 0x3F);
            }

            if (code_point > 0x10FFFF || (code_point >= 0xD800 && code_point <= 0xDFFF))
                code_point = 0xFFFD;

            data.push_back(code_point);
            i += len;
        }

        return 0;
    }
};
