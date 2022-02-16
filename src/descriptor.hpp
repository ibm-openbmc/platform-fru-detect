/* SPDX-License-Identifier: Apache-2.0 */
#pragma once

#include <filesystem>
#include <ios>

// An RAII compliant object for holding a posix file descriptor
class FileDescriptor
{
  private:
    int fd;

  public:
    FileDescriptor() = delete;
    FileDescriptor(const FileDescriptor&) = delete;
    FileDescriptor& operator=(const FileDescriptor&) = delete;
    FileDescriptor(FileDescriptor&&) noexcept;
    FileDescriptor& operator=(FileDescriptor&&) noexcept;

    explicit FileDescriptor(const std::filesystem::path& name,
                        std::ios_base::openmode mode = std::ios_base::in |
                                                       std::ios_base::out);

    explicit FileDescriptor(int fd);

    ~FileDescriptor();
    int descriptor();
};
