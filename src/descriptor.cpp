/* SPDX-License-Identifier: Apache-2.0 */
#include <fcntl.h>
#include <unistd.h>

#include <descriptor.hpp>

#include <iostream>
#include <stdexcept>

FileDescriptor::FileDescriptor(const std::filesystem::path& name,
                               std::ios_base::openmode mode) :
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-vararg)
    fd(open(name.c_str(), mode))
{
    if (fd < 0)
    {
        throw std::out_of_range(name.string() + " failed to open");
    }
}

FileDescriptor::FileDescriptor(int fdIn) : fd(fdIn){};

FileDescriptor::FileDescriptor(FileDescriptor&& in) noexcept
{
    fd = in.fd;
    in.fd = -1;
}

FileDescriptor& FileDescriptor::operator=(FileDescriptor&& in) noexcept
{
    fd = in.fd;
    in.fd = -1;
    return *this;
}

FileDescriptor::~FileDescriptor()
{
    if (fd)
    {
        int r = close(fd);
        if (r < 0)
        {
            std::cerr << "Failed to close fd " << std::to_string(fd);
        }
    }
}

int FileDescriptor::descriptor()
{
    return fd;
}
