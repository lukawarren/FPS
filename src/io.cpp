#include "io.h"

std::pair<std::ifstream, std::streamsize> io_read_file(
    const std::string& filename,
    const size_t max_size
)
{
    // Check isn't folder
    if (!std::filesystem::is_regular_file(filename))
        throw std::runtime_error(filename + " is not a file");

    // Open file
    std::ifstream file(filename, std::ios::binary);
    if (!file.is_open())
        throw std::runtime_error("Unable to open file " + filename);

    // Get file length
    file.seekg(0, std::ios::end);
    const auto file_length = file.tellg();
    file.seekg(0, std::ios::beg);

    if (max_size != 0 && (size_t)file_length > max_size)
        throw std::runtime_error(filename + " is too large");

    return { std::move(file), file_length };
}

std::pair<u8*, size_t> io_read_file(const std::string& filename)
{
    auto file = io_read_file(filename, 0);
    u8* buffer = new u8[file.second + 1];
    buffer[file.second] = '\0';
    file.first.read(reinterpret_cast<char*>(buffer), file.second);
    file.first.close();
    return std::pair<u8*, size_t> { buffer, (size_t)file.second };
}
