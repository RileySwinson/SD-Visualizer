#pragma once

#include <fstream>
#include <ios>
#include <string>
#include <type_traits>

// Binary blob reader for SDT dump files.
// Adapted from Muller's reference path-guider code.
// Reads size-prefixed values: each scalar is preceded by a uint16 declaring sizeof(T).
class BlobReader {
public:
    BlobReader(const std::string& fn) : file(fn, std::ios::in | std::ios::binary) {}

    // Length-prefixed string (the header format used by SDT dumps)
    std::string readString() {
        uint32_t len;
        file.read((char*)&len, 4);
        std::string r(len, '\0');
        file.read(&r[0], len);
        return r;
    }

    // Reads [uint16 size][T value], asserting size == sizeof(T)
    template <typename T>
    typename std::enable_if<std::is_standard_layout<T>::value, BlobReader&>::type
    operator>>(T& e) {
        uint16_t size;
        Read(&size, 1);
        if (size != sizeof(T)) { valid = false; return *this; }
        Read(&e, 1);
        return *this;
    }

    template <typename T>
    void Read(T* t, size_t c) {
        file.read(reinterpret_cast<char*>(t), c * sizeof(T));
    }

    bool isValid() const { return valid && bool(file); }

private:
    std::ifstream file;
    bool valid = true;
};
