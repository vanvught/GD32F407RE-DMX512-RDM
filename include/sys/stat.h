/**
 * @file stat.h
 *
 */
/* Copyright (C) 2026 by Arjan van Vught mailto:info@gd32-dmx.org
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:

 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.

 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#ifndef SYS_STAT_H_
#define SYS_STAT_H_

#include <time.h>
#include <sys/types.h>

struct stat { // NOLINT
    mode_t st_mode;
    off_t st_size;
    //    time_t  st_atime;
    time_t st_mtime;
    //    time_t  st_ctime;
};

#define S_IFMT 0170000   ///< These bits determine file type.
#define S_IFDIR 0040000  ///< Directory.
#define S_IFCHR 0020000  ///< Character device.
#define S_IFBLK 0060000  ///< Block device
#define S_IFREG 0100000  ///< Regular file.
#define S_IFIFO 0010000  ///< FIFO.  */
#define S_IFLNK 0120000  ///< Symbolic link.
#define S_IFSOCK 0140000 ///< Socket.
#define S_IREAD 0400     ///< Read by owner.
#define S_IWRITE 0200    ///< Write by owner.
#define S_IEXEC 0100     ///< Execute by owner.

#define S_IRUSR S_IREAD                        ///< Read by owner.
#define S_IWUSR S_IWRITE                       ///< Write by owner.
#define S_IXUSR S_IEXEC                        ///< Execute by owner.
#define S_IRWXU (S_IREAD | S_IWRITE | S_IEXEC) ///< Read,Write,Execute by owner

#define S_IRGRP (S_IRUSR >> 3) ///< Read by group.
#define S_IWGRP (S_IWUSR >> 3) ///< Write by group.
#define S_IXGRP (S_IXUSR >> 3) ///< Execute by group.
#define S_IRWXG (S_IRWXU >> 3) ///< Read,Write,Execute by user

#define S_IROTH (S_IRGRP >> 3) ///< Read by others.
#define S_IWOTH (S_IWGRP >> 3) ///< Write by others.
#define S_IXOTH (S_IXGRP >> 3) ///< Execute by others.
#define S_IRWXO (S_IRWXG >> 3) ///< Read,Write,Execute by other

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wshadow"

int stat(const char* path, struct stat* buf); // NOLINT

#pragma GCC diagnostic pop

#endif // SYS_STAT_H_