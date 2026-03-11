#pragma once

#include <cstddef>
#include <fcntl.h>
#include <stdexcept>
#include <string>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

// RAII wrapper around a read-only memory-mapped file.
class MappedFile
{
    char *pointer = nullptr;
    size_t length = 0;
    int file_descriptor = -1;

public:
    explicit MappedFile(const std::string &path)
    {
        file_descriptor = open(path.c_str(), O_RDONLY);
        if (file_descriptor < 0)
            throw std::runtime_error("cannot open " + path);

        struct stat st;
        if (fstat(file_descriptor, &st) < 0)
        {
            close(file_descriptor);
            throw std::runtime_error("fstat failed for " + path);
        }

        length = static_cast<size_t>(st.st_size);
        void *p = mmap(nullptr, length, PROT_READ, MAP_PRIVATE, file_descriptor, 0);
        if (p == MAP_FAILED)
        {
            close(file_descriptor);
            throw std::runtime_error("mmap failed for " + path);
        }
        pointer = static_cast<char *>(p);
    }

    ~MappedFile()
    {
        if (pointer)
            munmap(pointer, length);
        if (file_descriptor >= 0)
            close(file_descriptor);
    }

    MappedFile(const MappedFile &) = delete;
    MappedFile &operator=(const MappedFile &) = delete;

    const char *data() const { return pointer; }
    size_t size() const { return length; }
};
