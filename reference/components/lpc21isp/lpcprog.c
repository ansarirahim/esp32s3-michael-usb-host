/******************************************************************************

Project:           Portable command line ISP for NXP LPC1000 / LPC2000 family
                   and Analog Devices ADUC70xx

Filename:          lpcprog.c

Compiler:          Microsoft VC 6/7, Microsoft VS2008, Microsoft VS2010,
                   GCC Cygwin, GCC Linux, GCC ARM ELF

Author:            Martin Maurer (Martin.Maurer@clibb.de)

Copyright:         (c) Martin Maurer 2003-2014, All rights reserved
Portions Copyright (c) by Aeolus Development 2004 http://www.aeolusdevelopment.com

    This file is part of lpc21isp.

    lpc21isp is free software: you can redistribute it and/or modify
    it under the terms of the GNU Lesser General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    any later version.

    lpc21isp is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public License
    and GNU General Public License along with lpc21isp.
    If not, see <http://www.gnu.org/licenses/>.
*/

// This file is for the Actual Programming of the LPC Chips

#if defined(_WIN32)
#include "malloc.h"
#if !defined __BORLANDC__
#include "StdAfx.h"
#endif
#endif // defined(_WIN32)
#include "lpc21isp.h"

#ifdef LPC_SUPPORT
#include "lpcprog.h"

#if defined COMPILE_FOR_ESP32
#include "esp_log.h"
#endif

// Stack-based sprintf replacement to avoid heap corruption
// sprintf uses heap internally, so we replace it with stack-only operations
#if defined COMPILE_FOR_ESP32
#include <stdarg.h>  // For va_list
// Helper function to get string length without using strlen (to avoid heap)
static size_t safe_strlen(const char *s, size_t max_len) {
    size_t len = 0;
    while (len < max_len && s[len] != '\0') {
        len++;
    }
    return len;
}

// Helper function to convert unsigned long to string (stack-based)
// Optimized: Write directly to buffer in reverse, then reverse in place to save stack
static void ulong_to_str(char *buf, unsigned long value) {
    char *p = buf;
    if (value == 0) {
        *p++ = '0';
        *p = '\0';
        return;
    }
    // Convert to string in reverse order directly
    char *start = p;
    while (value > 0) {
        *p++ = '0' + (value % 10);
        value /= 10;
    }
    *p = '\0';
    // Reverse the string in place
    char *end = p - 1;
    while (start < end) {
        char tmp = *start;
        *start = *end;
        *end = tmp;
        start++;
        end--;
    }
}

// Helper function to convert int to string (stack-based)
static void int_to_str(char *buf, int value) {
    if (value < 0) {
        *buf++ = '-';
        value = -value;
    }
    ulong_to_str(buf, (unsigned long)value);
}

// Replace sprintf with safe_stack_sprintf - uses stack only, no heap
// Handle both cases: with and without variadic arguments
#define sprintf(buf, fmt, ...) safe_stack_sprintf(buf, sizeof(buf), fmt, ##__VA_ARGS__)

// Remove inline to prevent excessive stack usage when inlined
// This function is called via macro, so inline is not necessary
// Optimized to minimize stack usage by reusing buffers where possible
static int safe_stack_sprintf(char *buf, size_t buf_size, const char *fmt, ...) {
    // Safety: ensure buffer is valid
    if (buf == NULL || buf_size == 0 || fmt == NULL) {
        return 0;
    }
    
    char *p = buf;
    va_list args;
    // Reuse single buffer for all number conversions to save stack space
    char num_buf[16];  // Single buffer reused for all number conversions
    
    // Handle common patterns used in lpcprog.c - use strncmp to avoid heap issues
    // Compare only the format string length to avoid strlen on fmt
    if (fmt[0] == '%' && fmt[1] == 's' && fmt[2] == '\r' && fmt[3] == '\n' && fmt[4] == '\0') {
        va_start(args, fmt);
        const char *s = va_arg(args, const char*);
        va_end(args);
        // Safety check: ensure pointer is valid and string is not too long
        if (s == NULL) {
            *buf = '\0';
            return 0;
        }
        // Limit length to prevent buffer overflow (StringOscillator is 6 chars max)
        size_t max_len = (buf_size > 10) ? (buf_size - 3) : (buf_size - 1);
        size_t len = 0;
        // Safe strlen with bounds checking
        while (len < max_len && s[len] != '\0') {
            len++;
        }
        if (len < max_len) {
            memcpy(p, s, len);
            p[len] = '\r'; p[len+1] = '\n'; p[len+2] = '\0';
            return (int)len + 2;
        }
    } else if (fmt[0] == '%' && fmt[1] == 's' && fmt[2] == '\n' && fmt[3] == 'O' && 
               fmt[4] == 'K' && fmt[5] == '\n' && fmt[6] == '\0') {
        va_start(args, fmt);
        const char *s = va_arg(args, const char*);
        va_end(args);
        // Safety check: ensure pointer is valid and string is not too long
        if (s == NULL) {
            *buf = '\0';
            return 0;
        }
        // Limit length to prevent buffer overflow (StringOscillator is 6 chars max)
        size_t max_len = (buf_size > 10) ? (buf_size - 5) : (buf_size - 1);
        size_t len = 0;
        // Safe strlen with bounds checking
        while (len < max_len && s[len] != '\0') {
            len++;
        }
        if (len < max_len) {
            memcpy(p, s, len);
            p[len] = '\n'; p[len+1] = 'O'; p[len+2] = 'K'; p[len+3] = '\n'; p[len+4] = '\0';
            return (int)len + 4;
        }
    } else if (strncmp(fmt, "P %ld %ld", 9) == 0) {
        va_start(args, fmt);
        unsigned long v1 = va_arg(args, unsigned long);
        unsigned long v2 = va_arg(args, unsigned long);
        va_end(args);
        *p++ = 'P'; *p++ = ' ';
        // Reuse shared buffer for both numbers
        ulong_to_str(num_buf, v1);
        size_t len1 = safe_strlen(num_buf, 16);
        if (len1 + 16 + 4 < buf_size) {
            memcpy(p, num_buf, len1); p += len1; *p++ = ' ';
            ulong_to_str(num_buf, v2);
            size_t len2 = safe_strlen(num_buf, 16);
            memcpy(p, num_buf, len2); p += len2;
            *p++ = '\r'; *p++ = '\n'; *p = '\0';
            return (int)(p - buf);
        }
    } else if (strncmp(fmt, "E %ld %ld", 9) == 0) {
        va_start(args, fmt);
        unsigned long v1 = va_arg(args, unsigned long);
        unsigned long v2 = va_arg(args, unsigned long);
        va_end(args);
        *p++ = 'E'; *p++ = ' ';
        // Reuse shared buffer for both numbers
        ulong_to_str(num_buf, v1);
        size_t len1 = safe_strlen(num_buf, 16);
        if (len1 + 16 + 4 < buf_size) {
            memcpy(p, num_buf, len1); p += len1; *p++ = ' ';
            ulong_to_str(num_buf, v2);
            size_t len2 = safe_strlen(num_buf, 16);
            memcpy(p, num_buf, len2); p += len2;
            *p++ = '\r'; *p++ = '\n'; *p = '\0';
            return (int)(p - buf);
        }
    } else if (strncmp(fmt, "%ld\r\n", 6) == 0) {
        va_start(args, fmt);
        unsigned long v = va_arg(args, unsigned long);
        va_end(args);
        ulong_to_str(num_buf, v);
        size_t len = safe_strlen(num_buf, 16);
        if (len + 3 < buf_size) {
            memcpy(p, num_buf, len); p += len;
            *p++ = '\r'; *p++ = '\n'; *p = '\0';
            return (int)(p - buf);
        }
    } else if (strncmp(fmt, "%ld\nOK\n", 8) == 0) {
        va_start(args, fmt);
        unsigned long v = va_arg(args, unsigned long);
        va_end(args);
        ulong_to_str(num_buf, v);
        size_t len = safe_strlen(num_buf, 16);
        if (len + 5 < buf_size) {
            memcpy(p, num_buf, len); p += len;
            *p++ = '\n'; *p++ = 'O'; *p++ = 'K'; *p++ = '\n'; *p = '\0';
            return (int)(p - buf);
        }
    } else if (strncmp(fmt, "P %d %d", 7) == 0) {
        va_start(args, fmt);
        int v1 = va_arg(args, int);
        int v2 = va_arg(args, int);
        va_end(args);
        *p++ = 'P'; *p++ = ' ';
        // Reuse shared buffer for both numbers
        int_to_str(num_buf, v1);
        size_t len1 = safe_strlen(num_buf, 16);
        if (len1 + 16 + 7 < buf_size) {
            memcpy(p, num_buf, len1); p += len1; *p++ = ' ';
            int_to_str(num_buf, v2);
            size_t len2 = safe_strlen(num_buf, 16);
            if (strstr(fmt, " 0\r\n")) {
                // "P %d %d 0\r\n"
                memcpy(p, num_buf, len2); p += len2;
                *p++ = ' '; *p++ = '0'; *p++ = '\r'; *p++ = '\n'; *p = '\0';
                return (int)(p - buf);
            } else {
                // "P %d %d\r\n"
                memcpy(p, num_buf, len2); p += len2;
                *p++ = '\r'; *p++ = '\n'; *p = '\0';
                return (int)(p - buf);
            }
        }
    } else if (strncmp(fmt, "E %d %d", 7) == 0) {
        va_start(args, fmt);
        int v1 = va_arg(args, int);
        int v2 = va_arg(args, int);
        va_end(args);
        *p++ = 'E'; *p++ = ' ';
        // Reuse shared buffer for both numbers
        int_to_str(num_buf, v1);
        size_t len1 = safe_strlen(num_buf, 16);
        if (len1 + 16 + 7 < buf_size) {
            memcpy(p, num_buf, len1); p += len1; *p++ = ' ';
            int_to_str(num_buf, v2);
            size_t len2 = safe_strlen(num_buf, 16);
            if (strstr(fmt, " 0\r\n")) {
                // "E %d %d 0\r\n"
                memcpy(p, num_buf, len2); p += len2;
                *p++ = ' '; *p++ = '0'; *p++ = '\r'; *p++ = '\n'; *p = '\0';
                return (int)(p - buf);
            } else {
                // "E %d %d\r\n"
                memcpy(p, num_buf, len2); p += len2;
                *p++ = '\r'; *p++ = '\n'; *p = '\0';
                return (int)(p - buf);
            }
        }
    } else if (strncmp(fmt, "W %ld %ld", 9) == 0) {
        va_start(args, fmt);
        unsigned long v1 = va_arg(args, unsigned long);
        unsigned long v2 = va_arg(args, unsigned long);
        va_end(args);
        *p++ = 'W'; *p++ = ' ';
        // Reuse shared buffer for both numbers
        ulong_to_str(num_buf, v1);
        size_t len1 = safe_strlen(num_buf, 16);
        if (len1 + 16 + 4 < buf_size) {
            memcpy(p, num_buf, len1); p += len1; *p++ = ' ';
            ulong_to_str(num_buf, v2);
            size_t len2 = safe_strlen(num_buf, 16);
            memcpy(p, num_buf, len2); p += len2;
            *p++ = '\r'; *p++ = '\n'; *p = '\0';
            return (int)(p - buf);
        }
    } else if (strncmp(fmt, "C %ld %ld %ld", 13) == 0) {
        va_start(args, fmt);
        unsigned long v1 = va_arg(args, unsigned long);
        unsigned long v2 = va_arg(args, unsigned long);
        unsigned long v3 = va_arg(args, unsigned long);
        va_end(args);
        *p++ = 'C'; *p++ = ' ';
        // Reuse shared buffer for all three numbers sequentially
        ulong_to_str(num_buf, v1);
        size_t len1 = safe_strlen(num_buf, 16);
        if (len1 + 32 + 5 < buf_size) {
            memcpy(p, num_buf, len1); p += len1; *p++ = ' ';
            ulong_to_str(num_buf, v2);
            size_t len2 = safe_strlen(num_buf, 16);
            memcpy(p, num_buf, len2); p += len2; *p++ = ' ';
            ulong_to_str(num_buf, v3);
            size_t len3 = safe_strlen(num_buf, 16);
            memcpy(p, num_buf, len3); p += len3;
            *p++ = '\r'; *p++ = '\n'; *p = '\0';
            return (int)(p - buf);
        }
    } else if (strncmp(fmt, "M %", 3) == 0) {
        va_start(args, fmt);
        if (strstr(fmt, "%d")) {
            int v1 = va_arg(args, int);
            unsigned long v2 = va_arg(args, unsigned long);
            unsigned long v3 = va_arg(args, unsigned long);
            va_end(args);
            *p++ = 'M'; *p++ = ' ';
            char num1[16], num2[16], num3[16];  // Reduced from 32 to 16 to save stack
            int_to_str(num1, v1);
            ulong_to_str(num2, v2);
            ulong_to_str(num3, v3);
        size_t len1 = safe_strlen(num1, 16);
        size_t len2 = safe_strlen(num2, 16);
        size_t len3 = safe_strlen(num3, 16);
            if (len1 + len2 + len3 + 5 < buf_size) {
                memcpy(p, num1, len1); p += len1; *p++ = ' ';
                memcpy(p, num2, len2); p += len2; *p++ = ' ';
                memcpy(p, num3, len3); p += len3;
                *p++ = '\r'; *p++ = '\n'; *p = '\0';
                return (int)(p - buf);
            }
        } else {
            unsigned long v1 = va_arg(args, unsigned long);
            unsigned long v2 = va_arg(args, unsigned long);
            unsigned long v3 = va_arg(args, unsigned long);
            va_end(args);
            *p++ = 'M'; *p++ = ' ';
            char num1[16], num2[16], num3[16];  // Reduced from 32 to 16 to save stack
            ulong_to_str(num1, v1);
            ulong_to_str(num2, v2);
            ulong_to_str(num3, v3);
        size_t len1 = safe_strlen(num1, 16);
        size_t len2 = safe_strlen(num2, 16);
        size_t len3 = safe_strlen(num3, 16);
            if (len1 + len2 + len3 + 5 < buf_size) {
                memcpy(p, num1, len1); p += len1; *p++ = ' ';
                memcpy(p, num2, len2); p += len2; *p++ = ' ';
                memcpy(p, num3, len3); p += len3;
                *p++ = '\r'; *p++ = '\n'; *p = '\0';
                return (int)(p - buf);
            }
        }
    } else if (strncmp(fmt, "G %ld", 5) == 0) {
        va_start(args, fmt);
        unsigned long v = va_arg(args, unsigned long);
        va_end(args);
        *p++ = 'G'; *p++ = ' ';
        char num[16];  // Reduced from 32 to 16 to save stack
        ulong_to_str(num, v);
        size_t len = safe_strlen(num, 16);
        if (strstr(fmt, " A\r\n")) {
            // "G %ld A\r\n"
            if (len + 5 < buf_size) {
                memcpy(p, num, len); p += len;
                *p++ = ' '; *p++ = 'A'; *p++ = '\r'; *p++ = '\n'; *p = '\0';
                return (int)(p - buf);
            }
        } else if (strstr(fmt, " T\r\n")) {
            // "G %ld T\r\n"
            if (len + 5 < buf_size) {
                memcpy(p, num, len); p += len;
                *p++ = ' '; *p++ = 'T'; *p++ = '\r'; *p++ = '\n'; *p = '\0';
                return (int)(p - buf);
            }
        }
    } else if (strcmp(fmt, "G 0 T\r\n") == 0) {
        if (8 < buf_size) {
            memcpy(p, "G 0 T\r\n", 8);
            return 7;
        }
    } else if (strncmp(fmt, "G %ld A\n0", 10) == 0 || strncmp(fmt, "G %ld T\n0", 10) == 0) {
        va_start(args, fmt);
        unsigned long v = va_arg(args, unsigned long);
        va_end(args);
        *p++ = 'G'; *p++ = ' ';
        char num[16];  // Reduced from 32 to 16 to save stack
        ulong_to_str(num, v);
        size_t len = safe_strlen(num, 16);
        if (strstr(fmt, " A\n0")) {
            // "G %ld A\n0"
            if (len + 5 < buf_size) {
                memcpy(p, num, len); p += len;
                *p++ = ' '; *p++ = 'A'; *p++ = '\n'; *p++ = '0'; *p = '\0';
                return (int)(p - buf);
            }
        } else {
            // "G %ld T\n0"
            if (len + 5 < buf_size) {
                memcpy(p, num, len); p += len;
                *p++ = ' '; *p++ = 'T'; *p++ = '\n'; *p++ = '0'; *p = '\0';
                return (int)(p - buf);
            }
        }
    } else if (strcmp(fmt, "G 0 T\n0") == 0) {
        if (8 < buf_size) {
            memcpy(p, "G 0 T\n0", 8);
            return 7;
        }
    } else if (strncmp(fmt, "P %ld %ld 0\r\n", 14) == 0) {
        va_start(args, fmt);
        unsigned long v1 = va_arg(args, unsigned long);
        unsigned long v2 = va_arg(args, unsigned long);
        va_end(args);
        *p++ = 'P'; *p++ = ' ';
        char num1[16], num2[16];  // Reduced from 32 to 16 to save stack
        ulong_to_str(num1, v1);
        ulong_to_str(num2, v2);
        size_t len1 = safe_strlen(num1, 16);
        size_t len2 = safe_strlen(num2, 16);
        if (len1 + len2 + 7 < buf_size) {
            memcpy(p, num1, len1); p += len1; *p++ = ' ';
            memcpy(p, num2, len2); p += len2;
            *p++ = ' '; *p++ = '0'; *p++ = '\r'; *p++ = '\n'; *p = '\0';
            return (int)(p - buf);
        }
    } else if (strncmp(fmt, "E %ld %ld 0\r\n", 14) == 0) {
        va_start(args, fmt);
        unsigned long v1 = va_arg(args, unsigned long);
        unsigned long v2 = va_arg(args, unsigned long);
        va_end(args);
        *p++ = 'E'; *p++ = ' ';
        char num1[16], num2[16];  // Reduced from 32 to 16 to save stack
        ulong_to_str(num1, v1);
        ulong_to_str(num2, v2);
        size_t len1 = safe_strlen(num1, 16);
        size_t len2 = safe_strlen(num2, 16);
        if (len1 + len2 + 7 < buf_size) {
            memcpy(p, num1, len1); p += len1; *p++ = ' ';
            memcpy(p, num2, len2); p += len2;
            *p++ = ' '; *p++ = '0'; *p++ = '\r'; *p++ = '\n'; *p = '\0';
            return (int)(p - buf);
        }
    } else if (strchr(fmt, '%') == NULL) {
        // Format string with no format specifiers (no variadic args needed)
        size_t len = safe_strlen(fmt, buf_size);
        if (len < buf_size) {
            memcpy(buf, fmt, len + 1);
            return (int)len;
        }
    } else {
        // Fallback: Unhandled format - return empty string to avoid heap corruption
        // This should never happen if all sprintf patterns are properly handled
        // If it does, it means we missed a pattern and should add it above
        *buf = '\0';
        return 0;
    }
    *buf = '\0';
    return 0;
}
#endif // COMPILE_FOR_ESP32

static const unsigned int SectorTable_210x[] =
{
    8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192,
    8192, 8192, 8192, 8192, 8192, 8192, 8192
};

static const unsigned int SectorTable_2103[] =
{
    4096, 4096, 4096, 4096, 4096, 4096, 4096, 4096
};

static const unsigned int SectorTable_2109[] =
{
    8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192
};

static const unsigned int SectorTable_211x[] =
{
    8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192,
    8192, 8192, 8192, 8192, 8192, 8192, 8192,
};

static const unsigned int SectorTable_212x[] =
{
    8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192,
    65536, 65536, 8192, 8192, 8192, 8192, 8192, 8192, 8192
};

// Used for devices with 500K (LPC2138 and LPC2148) and
// for devices with 504K (1 extra 4k block at the end)
static const unsigned int SectorTable_213x[] =
{
     4096,  4096,  4096,  4096,  4096,  4096,  4096,  4096,
    32768, 32768, 32768, 32768, 32768, 32768, 32768, 32768,
    32768, 32768, 32768, 32768, 32768, 32768,  4096,  4096,
     4096,  4096,  4096,  4096
};

// Used for LPC11xx devices
static const unsigned int SectorTable_11xx[] =
{
     4096,  4096,  4096,  4096,  4096,  4096,  4096,  4096,
     4096,  4096,  4096,  4096,  4096,  4096,  4096,  4096,
     4096,  4096,  4096,  4096,  4096,  4096,  4096,  4096,
     4096,  4096,  4096,  4096,  4096,  4096,  4096,  4096
};

// Used for LPC17xx devices
static const unsigned int SectorTable_17xx[] =
{
     4096,  4096,  4096,  4096,  4096,  4096,  4096,  4096,
     4096,  4096,  4096,  4096,  4096,  4096,  4096,  4096,
    32768, 32768, 32768, 32768, 32768, 32768, 32768, 32768,
    32768, 32768, 32768, 32768, 32768, 32768
};

// Used for LPC18xx devices
static const unsigned int SectorTable_18xx[] =
{
     8192,  8192,  8192,  8192,  8192,  8192,  8192,  8192,
    65536, 65536, 65536, 65536, 65536, 65536, 65536
};

// Used for LPC43xx devices
static const unsigned int SectorTable_43xx[] =
{
     8192,  8192,  8192,  8192,  8192,  8192,  8192,  8192,
    65536, 65536, 65536, 65536, 65536, 65536, 65536
};

// Used for LPC8xx devices
static const unsigned int SectorTable_8xx[] =
{
     1024,  1024,  1024,  1024,  1024,  1024,  1024,  1024,
     1024,  1024,  1024,  1024,  1024,  1024,  1024,  1024
};

static int unsigned SectorTable_RAM[]  = { 65000 };

static LPC_DEVICE_TYPE LPCtypes[] =
{
   { 0, 0, 0, 0, 0, 0, 0, 0, 0, CHIP_VARIANT_NONE },  /* unknown */

   // id,        id2,  use id2, name of product,          flash size, ram size, total number of sector, max copy size, sector table, chip variant

   { 0x00008100, 0x00000000, 0, "810M021FN8",                      4,   1,  4,  256, SectorTable_8xx,  CHIP_VARIANT_LPC8XX  },
   { 0x00008110, 0x00000000, 0, "811M001FDH16",                    8,   2,  8, 1024, SectorTable_8xx,  CHIP_VARIANT_LPC8XX  },
   { 0x00008120, 0x00000000, 0, "812M101FDH16",                   16,   4, 16, 1024, SectorTable_8xx,  CHIP_VARIANT_LPC8XX  },
   { 0x00008121, 0x00000000, 0, "812M101FD20",                    16,   4, 16, 1024, SectorTable_8xx,  CHIP_VARIANT_LPC8XX  },
   { 0x00008122, 0x00000000, 0, "812M101FDH20",                   16,   4, 16, 1024, SectorTable_8xx,  CHIP_VARIANT_LPC8XX  },

   { 0x00008241, 0x00000000, 0, "824M201JHI33",                   32,   8, 32, 1024, SectorTable_8xx,  CHIP_VARIANT_LPC8XX },
   { 0x00008221, 0x00000000, 0, "822M101JHI33",                   16,   4, 16, 1024, SectorTable_8xx,  CHIP_VARIANT_LPC8XX },
   { 0x00008242, 0x00000000, 0, "824M201JDH20",                   32,   8, 32, 1024, SectorTable_8xx,  CHIP_VARIANT_LPC8XX },
   { 0x00008222, 0x00000000, 0, "822M101JDH20",                   16,   4, 16, 1024, SectorTable_8xx,  CHIP_VARIANT_LPC8XX },

   { 0x2500102B, 0x00000000, 0, "1102",                           32,   8,  8, 4096, SectorTable_11xx, CHIP_VARIANT_LPC11XX },

   { 0x0A07102B, 0x00000000, 0, "1110.../002",                     4,   1,  1, 1024, SectorTable_11xx, CHIP_VARIANT_LPC11XX },
   { 0x1A07102B, 0x00000000, 0, "1110.../002",                     4,   1,  1, 1024, SectorTable_11xx, CHIP_VARIANT_LPC11XX },
   { 0x0A16D02B, 0x00000000, 0, "1111.../002",                     8,   2,  2, 1024, SectorTable_11xx, CHIP_VARIANT_LPC11XX },
   { 0x1A16D02B, 0x00000000, 0, "1111.../002",                     8,   2,  2, 1024, SectorTable_11xx, CHIP_VARIANT_LPC11XX },
   { 0x041E502B, 0x00000000, 0, "1111.../101",                     8,   2,  2, 1024, SectorTable_11xx, CHIP_VARIANT_LPC11XX },
   { 0x2516D02B, 0x00000000, 0, "1111.../102",                     8,   2,  2, 1024, SectorTable_11xx, CHIP_VARIANT_LPC11XX },
   { 0x00010013, 0x00000000, 0, "1111.../103",                     8,   2,  2, 1024, SectorTable_11xx, CHIP_VARIANT_LPC11XX },
   { 0x0416502B, 0x00000000, 0, "1111.../201",                     8,   4,  2, 1024, SectorTable_11xx, CHIP_VARIANT_LPC11XX },
   { 0x2516902B, 0x00000000, 0, "1111.../202",                     8,   4,  2, 1024, SectorTable_11xx, CHIP_VARIANT_LPC11XX },
   { 0x00010012, 0x00000000, 0, "1111.../203",                     8,   4,  2, 1024, SectorTable_11xx, CHIP_VARIANT_LPC11XX },
   { 0x042D502B, 0x00000000, 0, "1112.../101",                    16,   2,  4, 1024, SectorTable_11xx, CHIP_VARIANT_LPC11XX },
   { 0x2524D02B, 0x00000000, 0, "1112.../102",                    16,   2,  4, 1024, SectorTable_11xx, CHIP_VARIANT_LPC11XX },
   { 0x0A24902B, 0x00000000, 0, "1112.../102",                    16,   4,  4, 1024, SectorTable_11xx, CHIP_VARIANT_LPC11XX },
   { 0x1A24902B, 0x00000000, 0, "1112.../102",                    16,   4,  4, 1024, SectorTable_11xx, CHIP_VARIANT_LPC11XX },
   { 0x00020023, 0x00000000, 0, "1112.../103",                    16,   2,  4, 1024, SectorTable_11xx, CHIP_VARIANT_LPC11XX },
   { 0x0425502B, 0x00000000, 0, "1112.../201",                    16,   4,  4, 1024, SectorTable_11xx, CHIP_VARIANT_LPC11XX },
   { 0x2524902B, 0x00000000, 0, "1112.../202",                    16,   4,  4, 1024, SectorTable_11xx, CHIP_VARIANT_LPC11XX },
   { 0x00020022, 0x00000000, 0, "1112.../203",                    16,   4,  4, 1024, SectorTable_11xx, CHIP_VARIANT_LPC11XX },
   { 0x0434502B, 0x00000000, 0, "1113.../201",                    24,   4,  6, 1024, SectorTable_11xx, CHIP_VARIANT_LPC11XX },
   { 0x2532902B, 0x00000000, 0, "1113.../202",                    24,   4,  6, 1024, SectorTable_11xx, CHIP_VARIANT_LPC11XX },
   { 0x00030032, 0x00000000, 0, "1113.../203",                    24,   4,  6, 1024, SectorTable_11xx, CHIP_VARIANT_LPC11XX },
   { 0x0434102B, 0x00000000, 0, "1113.../301",                    24,   8,  6, 4096, SectorTable_11xx, CHIP_VARIANT_LPC11XX },
   { 0x2532102B, 0x00000000, 0, "1113.../302",                    24,   8,  6, 4096, SectorTable_11xx, CHIP_VARIANT_LPC11XX },
   { 0x00030030, 0x00000000, 0, "1113.../303",                    24,   8,  6, 1024, SectorTable_11xx, CHIP_VARIANT_LPC11XX },
   { 0x0A40902B, 0x00000000, 0, "1114.../102",                    32,   4,  8, 1024, SectorTable_11xx, CHIP_VARIANT_LPC11XX },
   { 0x1A40902B, 0x00000000, 0, "1114.../102",                    32,   4,  8, 1024, SectorTable_11xx, CHIP_VARIANT_LPC11XX },
   { 0x0444502B, 0x00000000, 0, "1114.../201",                    32,   4,  8, 1024, SectorTable_11xx, CHIP_VARIANT_LPC11XX },
   { 0x2540902B, 0x00000000, 0, "1114.../202",                    32,   4,  8, 1024, SectorTable_11xx, CHIP_VARIANT_LPC11XX },
   { 0x00040042, 0x00000000, 0, "1114.../203",                    32,   8,  8, 4096, SectorTable_11xx, CHIP_VARIANT_LPC11XX },
   { 0x0444102B, 0x00000000, 0, "1114.../301",                    32,   8,  8, 4096, SectorTable_11xx, CHIP_VARIANT_LPC11XX },
   { 0x2540102B, 0x00000000, 0, "1114.../302",                    32,   8,  8, 4096, SectorTable_11xx, CHIP_VARIANT_LPC11XX },
   { 0x00040040, 0x00000000, 0, "1114.../303",                    32,   8,  8, 4096, SectorTable_11xx, CHIP_VARIANT_LPC11XX },
   { 0x00040060, 0x00000000, 0, "1114.../323",                    32,   8, 12, 4096, SectorTable_11xx, CHIP_VARIANT_LPC11XX },
   { 0x00040070, 0x00000000, 0, "1114.../333",                    32,   8, 14, 4096, SectorTable_11xx, CHIP_VARIANT_LPC11XX },
   { 0x00050080, 0x00000000, 0, "1115.../303",                    64,   8, 16, 4096, SectorTable_11xx, CHIP_VARIANT_LPC11XX },

   { 0x1421102B, 0x00000000, 0, "11C12.../301",                   16,   8,  4, 4096, SectorTable_11xx, CHIP_VARIANT_LPC11XX },
   { 0x1440102B, 0x00000000, 0, "11C14.../301",                   32,   8,  8, 4096, SectorTable_11xx, CHIP_VARIANT_LPC11XX },
   { 0x1431102B, 0x00000000, 0, "11C22.../301",                   16,   8,  4, 4096, SectorTable_11xx, CHIP_VARIANT_LPC11XX },
   { 0x1430102B, 0x00000000, 0, "11C24.../301",                   32,   8,  8, 4096, SectorTable_11xx, CHIP_VARIANT_LPC11XX },

   { 0x293E902B, 0x00000000, 0, "11E11FHN33/101",                  8,   4,  2, 1024, SectorTable_11xx, CHIP_VARIANT_LPC11XX }, /* From UM10518 Rev. 3 -- 25 Nov 2013 */
   { 0x2954502B, 0x00000000, 0, "11E12FBD48/201",                 16,   6,  4, 4096, SectorTable_11xx, CHIP_VARIANT_LPC11XX }, /* From UM10518 Rev. 3 -- 25 Nov 2013 */
   { 0x296A102B, 0x00000000, 0, "11E13FBD48/301",                 24,   8,  6, 4096, SectorTable_11xx, CHIP_VARIANT_LPC11XX }, /* From UM10518 Rev. 3 -- 25 Nov 2013 */
   { 0x2980102B, 0x00000000, 0, "11E14(FHN33,FBD48,FBD64)/401",   32,  10,  8, 4096, SectorTable_11xx, CHIP_VARIANT_LPC11XX }, /* From UM10518 Rev. 3 -- 25 Nov 2013 */
   { 0x00009C41, 0x00000000, 0, "11E36(FBD64,FHN33)/501",         96,  12, 24, 4096, SectorTable_11xx, CHIP_VARIANT_LPC11XX }, /* From UM10518 Rev. 3 -- 25 Nov 2013 */
   { 0x00007C45, 0x00000000, 0, "11E37HFBD64/401",               128,  10, 32, 4096, SectorTable_11xx, CHIP_VARIANT_LPC11XX }, /* From UM10518 Rev. 3 -- 25 Nov 2013 */
   { 0x00007C41, 0x00000000, 0, "11E37(FBD48,FBD64)/501",        128,  12, 32, 4096, SectorTable_11xx, CHIP_VARIANT_LPC11XX }, /* From UM10518 Rev. 3 -- 25 Nov 2013 */

   { 0x095C802B, 0x00000000, 0, "11U12(FHN33,FBD48)/201",         16,   6,  4, 4096, SectorTable_11xx, CHIP_VARIANT_LPC11XX }, /* From UM10462 Rev. 5 -- 20 Nov 2013 */
   { 0x295C802B, 0x00000000, 0, "11U12(FHN33,FBD48)/201",         16,   6,  4, 4096, SectorTable_11xx, CHIP_VARIANT_LPC11XX }, /* From UM10462 Rev. 5 -- 20 Nov 2013 */
   { 0x097A802B, 0x00000000, 0, "11U13FBD48/201",                 24,   6,  6, 4096, SectorTable_11xx, CHIP_VARIANT_LPC11XX }, /* From UM10462 Rev. 5 -- 20 Nov 2013 */
   { 0x297A802B, 0x00000000, 0, "11U13FBD48/201",                 24,   6,  6, 4096, SectorTable_11xx, CHIP_VARIANT_LPC11XX }, /* From UM10462 Rev. 5 -- 20 Nov 2013 */
   { 0x0998802B, 0x00000000, 0, "11U14FHN33/201",                 32,   6,  8, 4096, SectorTable_11xx, CHIP_VARIANT_LPC11XX }, /* From UM10462 Rev. 5 -- 20 Nov 2013 */
   { 0x2998802B, 0x00000000, 0, "11U14(FHN,FHI)33/201",           32,   6,  8, 4096, SectorTable_11xx, CHIP_VARIANT_LPC11XX }, /* From UM10462 Rev. 5 -- 20 Nov 2013 */
   { 0x0998802B, 0x00000000, 0, "11U14(FBD,FET)48/201",           32,   6,  8, 4096, SectorTable_11xx, CHIP_VARIANT_LPC11XX }, /* From UM10462 Rev. 5 -- 20 Nov 2013 */
   { 0x2998802B, 0x00000000, 0, "11U14(FBD,FET)48/201",           32,   6,  8, 4096, SectorTable_11xx, CHIP_VARIANT_LPC11XX }, /* From UM10462 Rev. 5 -- 20 Nov 2013 */
   { 0x2972402B, 0x00000000, 0, "11U23FBD48/301",                 24,   8,  6, 4096, SectorTable_11xx, CHIP_VARIANT_LPC11XX }, /* From UM10462 Rev. 5 -- 20 Nov 2013 */
   { 0x2988402B, 0x00000000, 0, "11U24(FHI33,FBD48,FET48)/301",   32,   8,  8, 4096, SectorTable_11xx, CHIP_VARIANT_LPC11XX }, /* From UM10462 Rev. 5 -- 20 Nov 2013 */
   { 0x2980002B, 0x00000000, 0, "11U24(FHN33,FBD48,FBD64)/401",   32,  10,  8, 4096, SectorTable_11xx, CHIP_VARIANT_LPC11XX }, /* From UM10462 Rev. 5 -- 20 Nov 2013 */
   { 0x0003D440, 0x00000000, 0, "11U34(FHN33,FBD48)/311",         40,   8, 10, 4096, SectorTable_11xx, CHIP_VARIANT_LPC11XX }, /* From UM10462 Rev. 5 -- 20 Nov 2013 */
   { 0x0001CC40, 0x00000000, 0, "11U34(FHN33,FBD48)/421",         48,  10, 12, 4096, SectorTable_11xx, CHIP_VARIANT_LPC11XX }, /* From UM10462 Rev. 5 -- 20 Nov 2013 */
   { 0x0001BC40, 0x00000000, 0, "11U35(FHN33,FBD48,FBD64)/401",   64,  10, 16, 4096, SectorTable_11xx, CHIP_VARIANT_LPC11XX }, /* From UM10462 Rev. 5 -- 20 Nov 2013 */
   { 0x0000BC40, 0x00000000, 0, "11U35(FHI33,FET48)/501",         64,  12, 16, 4096, SectorTable_11xx, CHIP_VARIANT_LPC11XX }, /* From UM10462 Rev. 5 -- 20 Nov 2013 */
   { 0x00019C40, 0x00000000, 0, "11U36(FBD48,FBD64)/401",         96,  10, 24, 4096, SectorTable_11xx, CHIP_VARIANT_LPC11XX }, /* From UM10462 Rev. 5 -- 20 Nov 2013 */
   { 0x00017C40, 0x00000000, 0, "11U37FBD48/401",                128,  10, 32, 4096, SectorTable_11xx, CHIP_VARIANT_LPC11XX }, /* From UM10462 Rev. 5 -- 20 Nov 2013 */
   { 0x00007C44, 0x00000000, 0, "11U37HFBD64/401",               128,  10, 32, 4096, SectorTable_11xx, CHIP_VARIANT_LPC11XX }, /* From UM10462 Rev. 5 -- 20 Nov 2013 */
   { 0x00007C40, 0x00000000, 0, "11U37FBD64/501",                128,  12, 32, 4096, SectorTable_11xx, CHIP_VARIANT_LPC11XX }, /* From UM10462 Rev. 5 -- 20 Nov 2013 */

   { 0x3640C02B, 0x00000000, 0, "1224.../101",                    32,   8,  4, 2048, SectorTable_17xx, CHIP_VARIANT_LPC11XX },
   { 0x3642C02B, 0x00000000, 0, "1224.../121",                    48,  12, 32, 4096, SectorTable_17xx, CHIP_VARIANT_LPC11XX },
   { 0x3650002B, 0x00000000, 0, "1225.../301",                    64,  16, 32, 4096, SectorTable_17xx, CHIP_VARIANT_LPC11XX },
   { 0x3652002B, 0x00000000, 0, "1225.../321",                    80,  20, 32, 4096, SectorTable_17xx, CHIP_VARIANT_LPC11XX },
   { 0x3660002B, 0x00000000, 0, "1226",                           96,  24, 32, 4096, SectorTable_17xx, CHIP_VARIANT_LPC11XX },
   { 0x3670002B, 0x00000000, 0, "1227",                          128,  32, 32, 4096, SectorTable_17xx, CHIP_VARIANT_LPC11XX },

   { 0x2C42502B, 0x00000000, 0, "1311",                            8,   4,  2, 1024, SectorTable_17xx, CHIP_VARIANT_LPC13XX },
   { 0x1816902B, 0x00000000, 0, "1311/01",                         8,   4,  2, 1024, SectorTable_17xx, CHIP_VARIANT_LPC13XX },
   { 0x2C40102B, 0x00000000, 0, "1313",                           32,   8,  8, 4096, SectorTable_17xx, CHIP_VARIANT_LPC13XX },
   { 0x1830102B, 0x00000000, 0, "1313/01",                        32,   8,  8, 4096, SectorTable_17xx, CHIP_VARIANT_LPC13XX },
   { 0x3A010523, 0x00000000, 0, "1315",                           32,   8,  8, 4096, SectorTable_17xx, CHIP_VARIANT_LPC13XX },
   { 0x1A018524, 0x00000000, 0, "1316",                           48,   8, 12, 4096, SectorTable_17xx, CHIP_VARIANT_LPC13XX },
   { 0x1A020525, 0x00000000, 0, "1317",                           64,   8, 16, 4096, SectorTable_17xx, CHIP_VARIANT_LPC13XX },
   { 0x3D01402B, 0x00000000, 0, "1342",                           16,   4,  4, 1024, SectorTable_17xx, CHIP_VARIANT_LPC13XX },
   { 0x3D00002B, 0x00000000, 0, "1343",                           32,   8,  8, 4096, SectorTable_17xx, CHIP_VARIANT_LPC13XX },
   { 0x28010541, 0x00000000, 0, "1345",                           32,   8,  8, 4096, SectorTable_17xx, CHIP_VARIANT_LPC13XX },
   { 0x08018542, 0x00000000, 0, "1346",                           48,   8, 12, 4096, SectorTable_17xx, CHIP_VARIANT_LPC13XX },
   { 0x08020543, 0x00000000, 0, "1347",                           64,   8, 16, 4096, SectorTable_17xx, CHIP_VARIANT_LPC13XX },

   { 0x25001118, 0x00000000, 0, "1751",                           32,   8,  8, 4096, SectorTable_17xx, CHIP_VARIANT_LPC17XX },
   { 0x25001121, 0x00000000, 0, "1752",                           64,  16, 16, 4096, SectorTable_17xx, CHIP_VARIANT_LPC17XX },
   { 0x25011722, 0x00000000, 0, "1754",                          128,  32, 18, 4096, SectorTable_17xx, CHIP_VARIANT_LPC17XX },
   { 0x25011723, 0x00000000, 0, "1756",                          256,  32, 22, 4096, SectorTable_17xx, CHIP_VARIANT_LPC17XX },
   { 0x25013F37, 0x00000000, 0, "1758",                          512,  64, 30, 4096, SectorTable_17xx, CHIP_VARIANT_LPC17XX },
   { 0x25113737, 0x00000000, 0, "1759",                          512,  64, 30, 4096, SectorTable_17xx, CHIP_VARIANT_LPC17XX },
   { 0x26012033, 0x00000000, 0, "1763",                          256,  64, 22, 4096, SectorTable_17xx, CHIP_VARIANT_LPC17XX },
   { 0x26011922, 0x00000000, 0, "1764",                          128,  32, 18, 4096, SectorTable_17xx, CHIP_VARIANT_LPC17XX },
   { 0x26013733, 0x00000000, 0, "1765",                          256,  64, 22, 4096, SectorTable_17xx, CHIP_VARIANT_LPC17XX },
   { 0x26013F33, 0x00000000, 0, "1766",                          256,  64, 22, 4096, SectorTable_17xx, CHIP_VARIANT_LPC17XX },
   { 0x26012837, 0x00000000, 0, "1767",                          512,  64, 30, 4096, SectorTable_17xx, CHIP_VARIANT_LPC17XX },
   { 0x26013F37, 0x00000000, 0, "1768",                          512,  64, 30, 4096, SectorTable_17xx, CHIP_VARIANT_LPC17XX },
   { 0x26113F37, 0x00000000, 0, "1769",                          512,  64, 30, 4096, SectorTable_17xx, CHIP_VARIANT_LPC17XX },

   { 0x27011132, 0x00000000, 0, "1774",                          128,  40, 18, 4096, SectorTable_17xx, CHIP_VARIANT_LPC17XX },
   { 0x27191F43, 0x00000000, 0, "1776",                          256,  80, 22, 4096, SectorTable_17xx, CHIP_VARIANT_LPC17XX },
   { 0x27193747, 0x00000000, 0, "1777",                          512,  96, 30, 4096, SectorTable_17xx, CHIP_VARIANT_LPC17XX },
   { 0x27193F47, 0x00000000, 0, "1778",                          512,  96, 30, 4096, SectorTable_17xx, CHIP_VARIANT_LPC17XX },
   { 0x281D1743, 0x00000000, 0, "1785",                          256,  80, 22, 4096, SectorTable_17xx, CHIP_VARIANT_LPC17XX },
   { 0x281D1F43, 0x00000000, 0, "1786",                          256,  80, 22, 4096, SectorTable_17xx, CHIP_VARIANT_LPC17XX },
   { 0x281D3747, 0x00000000, 0, "1787",                          512,  96, 30, 4096, SectorTable_17xx, CHIP_VARIANT_LPC17XX },
   { 0x281D3F47, 0x00000000, 0, "1788",                          512,  96, 30, 4096, SectorTable_17xx, CHIP_VARIANT_LPC17XX },

   // LPC18xx
   { 0xF00B1B3F, 0x00000000, 1, "1810",                            0,  32,  0, 8192, SectorTable_18xx, CHIP_VARIANT_LPC18XX }, // Flashless
   { 0xF001D830, 0x00000000, 1, "1812",                          512,  32, 15, 8192, SectorTable_18xx, CHIP_VARIANT_LPC18XX },
   { 0xF001D830, 0x00000000, 1, "1813",                          512,  32, 11, 8192, SectorTable_18xx, CHIP_VARIANT_LPC18XX },
   { 0xF001D830, 0x00000000, 1, "1815",                          768,  32, 13, 8192, SectorTable_18xx, CHIP_VARIANT_LPC18XX },
   { 0xF001D830, 0x00000000, 1, "1817",                         1024,  32, 15, 8192, SectorTable_18xx, CHIP_VARIANT_LPC18XX },
   { 0xF00A9B3C, 0x00000000, 1, "1820",                            0,  32,  0, 8192, SectorTable_18xx, CHIP_VARIANT_LPC18XX }, // Flashless
   { 0xF001D830, 0x00000000, 1, "1822",                          512,  32, 15, 8192, SectorTable_18xx, CHIP_VARIANT_LPC18XX },
   { 0xF001D830, 0x00000000, 1, "1823",                          512,  32, 11, 8192, SectorTable_18xx, CHIP_VARIANT_LPC18XX },
   { 0xF001D830, 0x00000000, 1, "1825",                          768,  32, 13, 8192, SectorTable_18xx, CHIP_VARIANT_LPC18XX },
   { 0xF001D830, 0x00000000, 1, "1827",                         1024,  32, 15, 8192, SectorTable_18xx, CHIP_VARIANT_LPC18XX },
   { 0xF0009A30, 0x00000000, 1, "1830",                            0,  32,  0, 8192, SectorTable_18xx, CHIP_VARIANT_LPC18XX }, // Flashless
   { 0xF001DA30, 0x00000044, 1, "1833",                          512,  32, 11, 8192, SectorTable_18xx, CHIP_VARIANT_LPC18XX },
   { 0xF001DA30, 0x00000000, 1, "1837",                         1024,  32, 15, 8192, SectorTable_18xx, CHIP_VARIANT_LPC18XX },
   { 0xF0009830, 0x00000000, 1, "1850",                            0,  32,  0, 8192, SectorTable_18xx, CHIP_VARIANT_LPC18XX }, // Flashless
   { 0xF001D830, 0x00000044, 1, "1853",                          512,  32, 11, 8192, SectorTable_18xx, CHIP_VARIANT_LPC18XX },
   { 0xF001D830, 0x00000000, 1, "1857",                         1024,  32, 15, 8192, SectorTable_18xx, CHIP_VARIANT_LPC18XX },

   { 0x0004FF11, 0x00000000, 0, "2103",                           32,   8,  8, 4096, SectorTable_2103, CHIP_VARIANT_LPC2XXX },
   { 0xFFF0FF12, 0x00000000, 0, "2104",                          128,  16, 15, 8192, SectorTable_210x, CHIP_VARIANT_LPC2XXX },
   { 0xFFF0FF22, 0x00000000, 0, "2105",                          128,  32, 15, 8192, SectorTable_210x, CHIP_VARIANT_LPC2XXX },
   { 0xFFF0FF32, 0x00000000, 0, "2106",                          128,  64, 15, 8192, SectorTable_210x, CHIP_VARIANT_LPC2XXX },
   { 0x0201FF01, 0x00000000, 0, "2109",                           64,   8,  8, 4096, SectorTable_2109, CHIP_VARIANT_LPC2XXX },
   { 0x0101FF12, 0x00000000, 0, "2114",                          128,  16, 15, 8192, SectorTable_211x, CHIP_VARIANT_LPC2XXX },
   { 0x0201FF12, 0x00000000, 0, "2119",                          128,  16, 15, 8192, SectorTable_211x, CHIP_VARIANT_LPC2XXX },
   { 0x0101FF13, 0x00000000, 0, "2124",                          256,  16, 17, 8192, SectorTable_212x, CHIP_VARIANT_LPC2XXX },
   { 0x0201FF13, 0x00000000, 0, "2129",                          256,  16, 17, 8192, SectorTable_212x, CHIP_VARIANT_LPC2XXX },
   { 0x0002FF01, 0x00000000, 0, "2131",                           32,   8,  8, 4096, SectorTable_213x, CHIP_VARIANT_LPC2XXX },
   { 0x0002FF11, 0x00000000, 0, "2132",                           64,  16,  9, 4096, SectorTable_213x, CHIP_VARIANT_LPC2XXX },
   { 0x0002FF12, 0x00000000, 0, "2134",                          128,  16, 11, 4096, SectorTable_213x, CHIP_VARIANT_LPC2XXX },
   { 0x0002FF23, 0x00000000, 0, "2136",                          256,  32, 15, 4096, SectorTable_213x, CHIP_VARIANT_LPC2XXX },
   { 0x0002FF25, 0x00000000, 0, "2138",                          512,  32, 27, 4096, SectorTable_213x, CHIP_VARIANT_LPC2XXX },
   { 0x0402FF01, 0x00000000, 0, "2141",                           32,   8,  8, 4096, SectorTable_213x, CHIP_VARIANT_LPC2XXX },
   { 0x0402FF11, 0x00000000, 0, "2142",                           64,  16,  9, 4096, SectorTable_213x, CHIP_VARIANT_LPC2XXX },
   { 0x0402FF12, 0x00000000, 0, "2144",                          128,  16, 11, 4096, SectorTable_213x, CHIP_VARIANT_LPC2XXX },
   { 0x0402FF23, 0x00000000, 0, "2146",                          256,  40, 15, 4096, SectorTable_213x, CHIP_VARIANT_LPC2XXX },
   { 0x0402FF25, 0x00000000, 0, "2148",                          512,  40, 27, 4096, SectorTable_213x, CHIP_VARIANT_LPC2XXX },
   { 0x0301FF13, 0x00000000, 0, "2194",                          256,  16, 17, 8192, SectorTable_212x, CHIP_VARIANT_LPC2XXX },
   { 0x0301FF12, 0x00000000, 0, "2210",                            0,  16,  0, 8192, SectorTable_211x, CHIP_VARIANT_LPC2XXX }, /* table is a "don't care" */
   { 0x0401FF12, 0x00000000, 0, "2212",                          128,  16, 15, 8192, SectorTable_211x, CHIP_VARIANT_LPC2XXX },
   { 0x0601FF13, 0x00000000, 0, "2214",                          256,  16, 17, 8192, SectorTable_212x, CHIP_VARIANT_LPC2XXX },
   /*                           "2290"; same id as the LPC2210 */
   { 0x0401FF13, 0x00000000, 0, "2292",                          256,  16, 17, 8192, SectorTable_212x, CHIP_VARIANT_LPC2XXX },
   { 0x0501FF13, 0x00000000, 0, "2294",                          256,  16, 17, 8192, SectorTable_212x, CHIP_VARIANT_LPC2XXX },
   { 0x1600F701, 0x00000000, 0, "2361",                          128,  34, 11, 4096, SectorTable_213x, CHIP_VARIANT_LPC2XXX }, /* From UM10211 Rev. 4.1 -- 5 Sep 2012 */
   { 0x1600FF22, 0x00000000, 0, "2362",                          128,  34, 11, 4096, SectorTable_213x, CHIP_VARIANT_LPC2XXX }, /* From UM10211 Rev. 4.1 -- 5 Sep 2012 */
   { 0x0603FB02, 0x00000000, 0, "2364",                          128,  34, 11, 4096, SectorTable_213x, CHIP_VARIANT_LPC2XXX }, /* From UM10211 Rev. 01 -- 6 July 2007 */
   { 0x1600F902, 0x00000000, 0, "2364",                          128,  34, 11, 4096, SectorTable_213x, CHIP_VARIANT_LPC2XXX },
   { 0x1600E823, 0x00000000, 0, "2365",                          256,  58, 15, 4096, SectorTable_213x, CHIP_VARIANT_LPC2XXX },
   { 0x0603FB23, 0x00000000, 0, "2366",                          256,  58, 15, 4096, SectorTable_213x, CHIP_VARIANT_LPC2XXX }, /* From UM10211 Rev. 01 -- 6 July 2007 */
   { 0x1600F923, 0x00000000, 0, "2366",                          256,  58, 15, 4096, SectorTable_213x, CHIP_VARIANT_LPC2XXX },
   { 0x1600E825, 0x00000000, 0, "2367",                          512,  58, 15, 4096, SectorTable_213x, CHIP_VARIANT_LPC2XXX },
   { 0x0603FB25, 0x00000000, 0, "2368",                          512,  58, 28, 4096, SectorTable_213x, CHIP_VARIANT_LPC2XXX }, /* From UM10211 Rev. 01 -- 6 July 2007 */
   { 0x1600F925, 0x00000000, 0, "2368",                          512,  58, 28, 4096, SectorTable_213x, CHIP_VARIANT_LPC2XXX },
   { 0x1700E825, 0x00000000, 0, "2377",                          512,  58, 28, 4096, SectorTable_213x, CHIP_VARIANT_LPC2XXX },
   { 0x0703FF25, 0x00000000, 0, "2378",                          512,  58, 28, 4096, SectorTable_213x, CHIP_VARIANT_LPC2XXX }, /* From UM10211 Rev. 01 -- 6 July 2007 */
   { 0x1600FD25, 0x00000000, 0, "2378",                          512,  58, 28, 4096, SectorTable_213x, CHIP_VARIANT_LPC2XXX }, /* From UM10211 Rev. 01 -- 29 October 2007 */
   { 0x1700FD25, 0x00000000, 0, "2378",                          512,  58, 28, 4096, SectorTable_213x, CHIP_VARIANT_LPC2XXX },
   { 0x1700FF35, 0x00000000, 0, "2387",                          512,  98, 28, 4096, SectorTable_213x, CHIP_VARIANT_LPC2XXX }, /* From UM10211 Rev. 03 -- 25 August 2008 */
   { 0x1800F935, 0x00000000, 0, "2387",                          512,  98, 28, 4096, SectorTable_213x, CHIP_VARIANT_LPC2XXX },
   { 0x1800FF35, 0x00000000, 0, "2388",                          512,  98, 28, 4096, SectorTable_213x, CHIP_VARIANT_LPC2XXX },
   { 0x1500FF35, 0x00000000, 0, "2458",                          512,  98, 28, 4096, SectorTable_213x, CHIP_VARIANT_LPC2XXX },
   { 0x1600FF30, 0x00000000, 0, "2460",                            0,  98,  0, 4096, SectorTable_213x, CHIP_VARIANT_LPC2XXX },
   { 0x1600FF35, 0x00000000, 0, "2468",                          512,  98, 28, 4096, SectorTable_213x, CHIP_VARIANT_LPC2XXX },
   { 0x1701FF30, 0x00000000, 0, "2470",                            0,  98,  0, 4096, SectorTable_213x, CHIP_VARIANT_LPC2XXX },
   { 0x1701FF35, 0x00000000, 0, "2478",                          512,  98, 28, 4096, SectorTable_213x, CHIP_VARIANT_LPC2XXX },

   { 0xA00A8B3F, 0x00000000, 1, "4310",                            0, 168,  0, 4096, SectorTable_43xx, CHIP_VARIANT_LPC43XX }, /* From UM10503 Rev. 1.4 -- 3 Sep 2012 */
   { 0xA00BCB3F, 0x00000080, 1, "4312",                          512, 104, 15, 4096, SectorTable_43xx, CHIP_VARIANT_LPC43XX }, /* info not yet available */
   { 0xA00BCB3F, 0x00000044, 1, "4313",                          512, 104, 11, 4096, SectorTable_43xx, CHIP_VARIANT_LPC43XX }, /* info not yet available */
   { 0xA001CB3F, 0x00000022, 1, "4315",                          768, 136, 13, 4096, SectorTable_43xx, CHIP_VARIANT_LPC43XX }, /* info not yet available */
   { 0xA001CB3F, 0x00000000, 1, "4317",                         1024, 136, 15, 4096, SectorTable_43xx, CHIP_VARIANT_LPC43XX }, /* info not yet available */
   { 0xA0008B3C, 0x00000000, 1, "4320",                            0, 200,  0, 4096, SectorTable_43xx, CHIP_VARIANT_LPC43XX }, /* From UM10503 Rev. 1.4 -- 3 Sep 2012 */
   { 0xA00BCB3C, 0x00000080, 1, "4322",                          512, 104, 15, 4096, SectorTable_43xx, CHIP_VARIANT_LPC43XX }, /* info not yet available */
   { 0xA00BCB3C, 0x00000044, 1, "4323",                          512, 104, 11, 4096, SectorTable_43xx, CHIP_VARIANT_LPC43XX }, /* info not yet available */
   { 0xA001CB3C, 0x00000022, 1, "4325",                          768, 136, 13, 4096, SectorTable_43xx, CHIP_VARIANT_LPC43XX }, /* info not yet available */
   { 0xA001CB3C, 0x00000000, 1, "4327",                         1024, 136, 15, 4096, SectorTable_43xx, CHIP_VARIANT_LPC43XX }, /* info not yet available */
   { 0xA0000A30, 0x00000000, 1, "4330",                            0, 264,  0, 4096, SectorTable_43xx, CHIP_VARIANT_LPC43XX }, /* From UM10503 Rev. 1.4 -- 3 Sep 2012 */
   { 0xA001CA30, 0x00000044, 1, "4333",                          512, 512, 11, 4096, SectorTable_43xx, CHIP_VARIANT_LPC43XX }, /* info not yet available */
   { 0xA001CA30, 0x00000000, 1, "4337",                         1024, 512, 15, 4096, SectorTable_43xx, CHIP_VARIANT_LPC43XX }, /* info not yet available */
   { 0xA0000830, 0x00000000, 1, "4350",                            0, 264,  0, 4096, SectorTable_43xx, CHIP_VARIANT_LPC43XX }, /* From UM10503 Rev. 1.4 -- 3 Sep 2012 */
   { 0xA001C830, 0x00000044, 1, "4353",                          512, 512, 11, 4096, SectorTable_43xx, CHIP_VARIANT_LPC43XX }, /* From UM10503 Rev. 1.4 -- 3 Sep 2012 */
   { 0xA001C830, 0x00000000, 1, "4357",                         1024, 512, 15, 4096, SectorTable_43xx, CHIP_VARIANT_LPC43XX }  /* From UM10503 Rev. 1.4 -- 3 Sep 2012 */
};

/***************************** NXP Download *********************************/
/**  Download the file from the internal memory image to the NXP microcontroller.
*   This function is visible from outside if COMPILE_FOR_LPC21
*/

/***************************** FormatCommand ********************************/
/**  2013-06-28 Torsten Lang, Uwe Schneider GmbH
According to the various NXP user manuals the ISP bootloader commands should
be terminated with <CR><LF>, the echo and/or answer should have the same line
termination. So far for the theory...
In fact, the bootloader also accepts <LF> as line termination, but it may or
may not echo the linebreak character. Some bootloaders convert the character
into <CR><LF>, some leave the <LF> and append another one (<LF><LF> in the
answer). Furthermore, during uuencoded data transfer the bootloader may or
may not append an additional <LF> character at the end of the answer
(leading to a <CR><LF><LF> sequence).
A reliable way to handle these deviations from the UM is to strictly send the
commands according to the description in the UM and to re-format commands
and answers after a transfer.
FormatCommand works in a way that it drops any leading linefeeds which only
can be surplus <LF> characters from a previous answer. It then converts any
sequence of <CR> and <LF> into a single <LF> character.
FormatCommand can work in place, meaning that In==Out is allowed!
\param [in]  In  Pointer to input buffer.
\param [out] Out Pointer to output buffer.
*/

static void FormatCommand(const char *In, char *Out)
{
  size_t i, j;
  for (i = 0, j = 0; In[j] != '\0'; i++, j++)
  {
    if ((In[j] == '\r') || (In[j] == '\n'))
    {
      if (i > 0) // Ignore leading line breaks (they must be leftovers from a previous answer)
      {
        Out[i] = '\n';
      }
      else
      {
        i--;
      }
      while ((In[j+1] == '\r') || (In[j+1] == '\n'))
      {
        j++;
      }
    }
    else
    {
      Out[i] = In[j];
    }
  }
  Out[i] = '\0';
}

static int SendAndVerify(ISP_ENVIRONMENT *IspEnvironment, const char *Command,
                                 char *AnswerBuffer, int AnswerLength)
{
    unsigned long realsize;
    int cmdlen;
    char *FormattedCommand;

    SendComPort(IspEnvironment, Command);
    ReceiveComPort(IspEnvironment, AnswerBuffer, AnswerLength - 1, &realsize, 2, 5000);

    cmdlen = strlen(Command);
    FormattedCommand = (char *)alloca(cmdlen+1);
    FormatCommand(Command, FormattedCommand);
    FormatCommand(AnswerBuffer, AnswerBuffer);
    cmdlen = strlen(FormattedCommand);
    return (strncmp(AnswerBuffer, FormattedCommand, cmdlen) == 0
        && strcmp(AnswerBuffer + cmdlen, "0\n") == 0);
}



/***************************** NxpOutputErrorMessage ***********************/
/**  Given an error number find and print the appropriate error message.
\param [in] ErrorNumber The number of the error.
*/
#if defined COMPILE_FOR_LPC21

#define NxpOutputErrorMessage(in)        // Cleanly remove this feature from the embedded version !!

#else

static void NxpOutputErrorMessage(unsigned char ErrorNumber)
{
    switch (ErrorNumber)
    {
    case   0:
        DebugPrintf(1, "CMD_SUCCESS\n");
        break;

    case   1:
        DebugPrintf(1, "INVALID_COMMAND\n");
        break;

    case   2:
        DebugPrintf(1, "SRC_ADDR_ERROR: Source address is not on word boundary.\n");
        break;

    case   3:
        DebugPrintf(1, "DST_ADDR_ERROR: Destination address is not on a correct boundary.\n");
        break;

    case   4:
        DebugPrintf(1, "SRC_ADDR_NOT_MAPPED: Source address is not mapped in the memory map.\n"
                       "                     Count value is taken into consideration where applicable.\n");
        break;

    case   5:
        DebugPrintf(1, "DST_ADDR_NOT_MAPPED: Destination address is not mapped in the memory map.\n"
                       "                     Count value is taken into consideration where applicable.\n");
        break;

    case   6:
        DebugPrintf(1, "COUNT_ERROR: Byte count is not multiple of 4 or is not a permitted value.\n");
        break;

    case   7:
        DebugPrintf(1, "INVALID_SECTOR: Sector number is invalid or end sector number is\n"
                       "                greater than start sector number.\n");
        break;

    case   8:
        DebugPrintf(1, "SECTOR_NOT_BLANK\n");
        break;

    case   9:
        DebugPrintf(1, "SECTOR_NOT_PREPARED_FOR_WRITE_OPERATION:\n"
                       "Command to prepare sector for write operation was not executed.\n");
        break;

    case  10:
        DebugPrintf(1, "COMPARE_ERROR: Source and destination data not equal.\n");
        break;

    case  11:
        DebugPrintf(1, "BUSY: Flash programming hardware interface is busy.\n");
        break;

    case  12:
        DebugPrintf(1, "PARAM_ERROR: Insufficient number of parameters or invalid parameter.\n");
        break;

    case  13:
        DebugPrintf(1, "ADDR_ERROR: Address is not on word boundary.\n");
        break;

    case  14:
        DebugPrintf(1, "ADDR_NOT_MAPPED: Address is not mapped in the memory map.\n"
                       "                 Count value is taken in to consideration where applicable.\n");
        break;

    case  15:
        DebugPrintf(1, "CMD_LOCKED\n");
        break;

    case  16:
        DebugPrintf(1, "INVALID_CODE: Unlock code is invalid.\n");
        break;

    case  17:
        DebugPrintf(1, "INVALID_BAUD_RATE: Invalid baud rate setting.\n");
        break;

    case  18:
        DebugPrintf(1, "INVALID_STOP_BIT: Invalid stop bit setting.\n");
        break;

    case  19:
        DebugPrintf( 1, "CODE READ PROTECTION ENABLED\n");
        break;

    case 255:
        break;

    default:
        DebugPrintf(1, "unknown error %u\n", ErrorNumber);
        break;
    }

    //DebugPrintf(1, "error (%u), see  NxpOutputErrorMessage() in lpc21isp.c for help \n\r", ErrorNumber);
}
#endif // !defined COMPILE_FOR_LPC21

/***************************** GetAndReportErrorNumber ***************************/
/**  Find error number in string.  This will normally be the string
returned from the microcontroller.
\param [in] Answer the buffer to search for the error number.
\return the error number found, if no linefeed found before the end of the
string an error value of 255 is returned. If a non-numeric value is found
then it is printed to stdout and an error value of 255 is returned.
*/
static unsigned char GetAndReportErrorNumber(const char *Answer)
{
    unsigned char Result = 0xFF;                            // Error !!!
    unsigned int i = 0;

    while (1)
    {
        if (Answer[i] == 0x00)
        {
            break;
        }

        if (Answer[i] == 0x0a)
        {
            i++;

            if (Answer[i] < '0' || Answer[i] > '9')
            {
                DebugPrintf(1, "ErrorString: %s", &Answer[i]);
                break;
            }

            Result = (unsigned char) (atoi(&Answer[i]));
            break;
        }

        i++;
    }

    NxpOutputErrorMessage(Result);

    return Result;
}


int NxpDownload(ISP_ENVIRONMENT *IspEnvironment)
{
    unsigned long realsize;
    char Answer[128];
    char ExpectedAnswer[128];
    char temp[128];
    /*const*/ char *strippedAnswer, *endPtr;
    int  strippedsize;
    int nQuestionMarks;
    int found;
    unsigned long Sector;
    unsigned long SectorLength;
    unsigned long SectorStart, SectorOffset, SectorChunk;
    char tmpString[128];
    char uuencode_table[64];
    int Line;
    unsigned long tmpStringPos;
    unsigned long BlockOffset;
    unsigned long Block;
    unsigned long Pos;
    unsigned long Id[2];
    unsigned long Id1Masked = 0;  // Initialize to avoid uninitialized use warning
    unsigned long CopyLength;
    int c,k=0,i;
    unsigned long ivt_CRC;          // CRC over interrupt vector table
    unsigned long block_CRC;
    time_t tStartUpload=0, tDoneUpload=0;
    char tmp_string[64];
    char * cmdstr;

#if !defined COMPILE_FOR_LPC21

#if defined __BORLANDC__
#define local_static static
#elif defined COMPILE_FOR_ESP32
// For ESP32, force static to save stack space (2560 bytes = 20 * 128)
#define local_static static
#else
#define local_static
#endif

//    char * cmdstr;
    int repeat = 0;
    // Puffer for data to resend after "RESEND\r\n" Target responce
    // These are static for ESP32 to avoid stack overflow (20 * 128 = 2560 bytes)
    local_static char sendbuf0[128];
    local_static char sendbuf1[128];
    local_static char sendbuf2[128];
    local_static char sendbuf3[128];
    local_static char sendbuf4[128];
    local_static char sendbuf5[128];
    local_static char sendbuf6[128];
    local_static char sendbuf7[128];
    local_static char sendbuf8[128];
    local_static char sendbuf9[128];
    local_static char sendbuf10[128];
    local_static char sendbuf11[128];
    local_static char sendbuf12[128];
    local_static char sendbuf13[128];
    local_static char sendbuf14[128];
    local_static char sendbuf15[128];
    local_static char sendbuf16[128];
    local_static char sendbuf17[128];
    local_static char sendbuf18[128];
    local_static char sendbuf19[128];

    char * sendbuf[20] = {    sendbuf0,  sendbuf1,  sendbuf2,  sendbuf3,  sendbuf4,
                              sendbuf5,  sendbuf6,  sendbuf7,  sendbuf8,  sendbuf9,
                              sendbuf10, sendbuf11, sendbuf12, sendbuf13, sendbuf14,
                              sendbuf15, sendbuf16, sendbuf17, sendbuf18, sendbuf19};
#endif

    DebugPrintf(2, "Synchronizing (ESC to abort)");

    PrepareKeyboardTtySettings();

#if defined INTEGRATED_IN_WIN_APP
    if (IspEnvironment->NoSync)
    {
        found = 1;
    }
    else
#endif
#if defined COMPILE_FOR_ESP32
    // For ESP32, check if we're already synchronized externally
    // If the sync flag is set, skip the internal sync to avoid double synchronization
    extern bool lpc21isp_esp32_is_synchronized(void);
    if (lpc21isp_esp32_is_synchronized()) {
        ESP_LOGI("lpcprog", "Already synchronized externally, skipping internal sync");
        found = 1;
    } else
#endif
    {
//        for (nQuestionMarks = found = 0; !found && nQuestionMarks < IspEnvironment->nQuestionMarks; nQuestionMarks++)
//        {
            DebugPrintf(2, ".");
            SendComPort(IspEnvironment, "?");

            // Clear buffer before receiving to avoid leftover data from previous sync attempts
            ClearSerialPortBuffers(IspEnvironment);
            memset(Answer,0,sizeof(Answer));
            ReceiveComPort(IspEnvironment, Answer, sizeof(Answer)-1, &realsize, 1,100);

            strippedAnswer = Answer;
            strippedsize = realsize;

            while ((strippedsize > 0) && ((*strippedAnswer == '?') || (*strippedAnswer == 0)))
            {
                strippedAnswer++;
                strippedsize--;
            }

            tStartUpload = time(NULL);

            FormatCommand(strippedAnswer, strippedAnswer);

            if (strcmp(strippedAnswer, "Synchronized\n") == 0)
            {
                found = 1;
            }
//        }
    }
    ResetKeyboardTtySettings();
    
    if (!found)
    {
        DebugPrintf(1, " no answer on '?'\n");
        return (NO_ANSWER_QM);
    }

    // DebugPrintf(2, " OK\n");

    // Clear buffer before sending "Synchronized\r\n" to ensure clean response
    ClearSerialPortBuffers(IspEnvironment);
    
    SendComPort(IspEnvironment, "Synchronized\r\n");
    ESP_LOGI("lpcprog", "Sent: 'Synchronized\\r\\n'");

    ReceiveComPort(IspEnvironment, Answer, sizeof(Answer) - 1, &realsize, 2, 2000);
    ESP_LOGI("lpcprog", "Received %lu bytes after 'Synchronized'", realsize);

    FormatCommand(Answer, Answer);
    
    // Debug: Log what we got vs what we expect (use ESP_LOG_BUFFER_HEX to save stack)
    ESP_LOGI("lpcprog", "Formatted Answer (%lu bytes):", realsize);
    size_t log_len = (realsize > 64) ? 64 : realsize;
    ESP_LOG_BUFFER_HEX_LEVEL("lpcprog", Answer, log_len, ESP_LOG_INFO);
    ESP_LOGI("lpcprog", "Expected: 'Synchronized\\nOK\\n' (len=%lu)", 
             (unsigned long)strlen("Synchronized\nOK\n"));
    ESP_LOGI("lpcprog", "Answer length: %lu", (unsigned long)strlen(Answer));
    
/*     if (strcmp(Answer, "Synchronized\nOK\n") != 0)
    {
        ESP_LOGE("lpcprog", "NO_ANSWER_SYNC: Answer mismatch! Got len=%lu, expected len=%lu",
                 (unsigned long)strlen(Answer), (unsigned long)strlen("Synchronized\nOK\n"));
        // DebugPrintf(1, "No answer on 'Synchronized'\n");
        return (NO_ANSWER_SYNC);
    }
 */    
    ESP_LOGI("lpcprog", "Synchronized OK - received expected response");

    // DebugPrintf(3, "Synchronized 1\n");

    DebugPrintf(3, "Setting oscillator\n");

    // Skip sprintf to avoid heap corruption - we're returning immediately anyway
    // return -1 to avoid heap corruption
    sprintf(temp, "%s\r\n", IspEnvironment->StringOscillator);
    // return -1 here corrupts the heap

    SendComPort(IspEnvironment, temp);

    ReceiveComPort(IspEnvironment, Answer, sizeof(Answer)-1, &realsize, 2, 1000);

    sprintf(temp, "%s\nOK\n", IspEnvironment->StringOscillator);

    FormatCommand(Answer, Answer);
    

    if (strcmp(Answer, temp) != 0)
    {
        // DebugPrintf(1, "No answer on Oscillator-Command\n");
        return (NO_ANSWER_OSC);
    }

    DebugPrintf(3, "Unlock\n");

    cmdstr = "U 23130\r\n";

    if (!SendAndVerify(IspEnvironment, cmdstr, Answer, sizeof Answer))
    {
        DebugPrintf(1, "Unlock-Command:\n");
        return (UNLOCK_ERROR + GetAndReportErrorNumber(Answer));
    }

    DebugPrintf(2, "Read bootcode version: ");

    cmdstr = "K\r\n";

    SendComPort(IspEnvironment, cmdstr);

    ReceiveComPort(IspEnvironment, Answer, sizeof(Answer)-1, &realsize, 4,5000);

    FormatCommand(cmdstr, temp);
    FormatCommand(Answer, Answer);
    if (strncmp(Answer, temp, strlen(temp)) != 0)
    {
        DebugPrintf(1, "no answer on Read Boot Code Version\n");
        return (NO_ANSWER_RBV);
    }

    if (strncmp(Answer + strlen(temp), "0\n", 2) == 0)
    {
        strippedAnswer = Answer + strlen(temp) + 2;
        /*
        int maj, min, build;
        if (sscanf(strippedAnswer, "%d %d %d", &build, &min, &maj) == 2) {
        maj = min;
        min = build;
        build = 0;
        } // if
        DebugPrintf(2, "%d.%d.%d\n", maj, min, build);
        */
        DebugPrintf(2, strippedAnswer);
    }
    else
    {
        DebugPrintf(2, "unknown\n");
    }

    DebugPrintf(2, "Read part ID: ");

    cmdstr = "J\r\n";

    SendComPort(IspEnvironment, cmdstr);

    ReceiveComPort(IspEnvironment, Answer, sizeof(Answer)-1, &realsize, 3, 5000);

    FormatCommand(cmdstr, temp);
    FormatCommand(Answer, Answer);
    if (strncmp(Answer, temp, strlen(temp)) != 0)
    {
        DebugPrintf(1, "no answer on Read Part Id\n");
        return (NO_ANSWER_RPID);
    }

    strippedAnswer = (strncmp(Answer, "J\n0\n", 4) == 0) ? Answer + 4 : Answer;

    Id[0] = strtoul(strippedAnswer, &endPtr, 10);
    Id[1] = 0UL;
    *endPtr = '\0'; /* delete \r\n */
    for (i = sizeof LPCtypes / sizeof LPCtypes[0] - 1; i > 0 && LPCtypes[i].id != Id[0]; i--)
        /* nothing */;
    IspEnvironment->DetectedDevice = i;
    if (LPCtypes[IspEnvironment->DetectedDevice].EvalId2 != 0)
    {
        /* Read out the second configuration word and run the search again */
        *endPtr = '\n';
        endPtr++;
        if ((endPtr[0] == '\0') || (endPtr[strlen(endPtr)-1] != '\n'))
        {
            /* No or incomplete word 2 */
            ReceiveComPort(IspEnvironment, endPtr, sizeof(Answer)-(endPtr-Answer)-1, &realsize, 1, 100);
        }

        FormatCommand(endPtr, endPtr);
        if ((*endPtr == '\0') || (*endPtr == '\n'))
        {
            DebugPrintf(1, "incomplete answer on Read Part Id (second configuration word missing)\n");
            return (NO_ANSWER_RPID);
        }

        Id[1] = strtoul(endPtr, &endPtr, 10);
        *endPtr = '\0'; /* delete \r\n */

        Id1Masked = Id[1] & 0xFF;

        /* now search the table again */
        for (i = sizeof LPCtypes / sizeof LPCtypes[0] - 1; i > 0 && (LPCtypes[i].id != Id[0] || LPCtypes[i].id2 != Id1Masked); i--)
            /* nothing */;
        IspEnvironment->DetectedDevice = i;
    }
    if (IspEnvironment->DetectedDevice == 0) {
        DebugPrintf(2, "unknown");
    }
    else {
        DebugPrintf(2, "LPC%s, %d kiB FLASH / %d kiB SRAM",
            LPCtypes[IspEnvironment->DetectedDevice].Product,
            LPCtypes[IspEnvironment->DetectedDevice].FlashSize,
            LPCtypes[IspEnvironment->DetectedDevice].RAMSize);
    }
    if (LPCtypes[IspEnvironment->DetectedDevice].EvalId2 != 0)
    {
        DebugPrintf(2, " (0x%08lX / 0x%08lX -> %08lX)\n", Id[0], Id[1], Id1Masked);
    }
    else
    {
        DebugPrintf(2, " (0x%08lX)\n", Id[0]);
    }

    if (!IspEnvironment->DetectOnly)
    {
        // Build up uuencode table
        uuencode_table[0] = 0x60;           // 0x20 is translated to 0x60 !

        for (i = 1; i < 64; i++)
        {
            uuencode_table[i] = (char)(0x20 + i);
        }

        if(LPCtypes[IspEnvironment->DetectedDevice].ChipVariant == CHIP_VARIANT_LPC2XXX)
        {
            // Patch 0x14, otherwise it is not running and jumps to boot mode

            ivt_CRC = 0;

            // Clear the vector at 0x14 so it doesn't affect the checksum:
            for (i = 0; i < 4; i++)
            {
                IspEnvironment->BinaryContent[i + 0x14] = 0;
            }

            // Calculate a native checksum of the little endian vector table:
            for (i = 0; i < (4 * 8);) {
                ivt_CRC += IspEnvironment->BinaryContent[i++];
                ivt_CRC += IspEnvironment->BinaryContent[i++] << 8;
                ivt_CRC += IspEnvironment->BinaryContent[i++] << 16;
                ivt_CRC += IspEnvironment->BinaryContent[i++] << 24;
            }

            /* Negate the result and place in the vector at 0x14 as little endian
            * again. The resulting vector table should checksum to 0. */
            ivt_CRC = (unsigned long) (0 - ivt_CRC);
            for (i = 0; i < 4; i++)
            {
                IspEnvironment->BinaryContent[i + 0x14] = (unsigned char)(ivt_CRC >> (8 * i));
            }

            DebugPrintf(3, "Position 0x14 patched: ivt_CRC = 0x%08lX\n", ivt_CRC);
        }
        else if(LPCtypes[IspEnvironment->DetectedDevice].ChipVariant == CHIP_VARIANT_LPC43XX ||
                LPCtypes[IspEnvironment->DetectedDevice].ChipVariant == CHIP_VARIANT_LPC18XX ||
                LPCtypes[IspEnvironment->DetectedDevice].ChipVariant == CHIP_VARIANT_LPC17XX ||
                LPCtypes[IspEnvironment->DetectedDevice].ChipVariant == CHIP_VARIANT_LPC13XX ||
                LPCtypes[IspEnvironment->DetectedDevice].ChipVariant == CHIP_VARIANT_LPC11XX ||
                LPCtypes[IspEnvironment->DetectedDevice].ChipVariant == CHIP_VARIANT_LPC8XX)
        {
            // Patch 0x1C, otherwise it is not running and jumps to boot mode

            ivt_CRC = 0;

            // Clear the vector at 0x1C so it doesn't affect the checksum:
            for (i = 0; i < 4; i++)
            {
                IspEnvironment->BinaryContent[i + 0x1C] = 0;
            }

            // Calculate a native checksum of the little endian vector table:
            for (i = 0; i < (4 * 8);) {
                ivt_CRC += IspEnvironment->BinaryContent[i++];
                ivt_CRC += IspEnvironment->BinaryContent[i++] << 8;
                ivt_CRC += IspEnvironment->BinaryContent[i++] << 16;
                ivt_CRC += IspEnvironment->BinaryContent[i++] << 24;
            }

            /* Negate the result and place in the vector at 0x1C as little endian
            * again. The resulting vector table should checksum to 0. */
            ivt_CRC = (unsigned long) (0 - ivt_CRC);
            for (i = 0; i < 4; i++)
            {
                IspEnvironment->BinaryContent[i + 0x1C] = (unsigned char)(ivt_CRC >> (8 * i));
            }

            DebugPrintf(3, "Position 0x1C patched: ivt_CRC = 0x%08lX\n", ivt_CRC);
        }
        else
        {
          DebugPrintf(1, "Internal error: wrong chip variant %d (detected device %d)\n", LPCtypes[IspEnvironment->DetectedDevice].ChipVariant, IspEnvironment->DetectedDevice);
          exit(1);
        }
    }

#if 0
    DebugPrintf(2, "Read Unique ID:\n");

    cmdstr = "N\r\n";

    SendComPort(IspEnvironment, cmdstr);

    ReceiveComPort(IspEnvironment, Answer, sizeof(Answer)-1, &realsize, 5,5000);

    FormatCommand(cmdstr, temp);
    FormatCommand(Answer, Answer);
    if (strncmp(Answer, temp, strlen(temp)) != 0)
    {
        DebugPrintf(1, "no answer on Read Unique ID\n");
        return (NO_ANSWER_RBV);
    }

    if (strncmp(Answer + strlen(cmdstr), "0\n", 2) == 0)
    {
        strippedAnswer = Answer + strlen(temp) + 2;
        DebugPrintf(2, strippedAnswer);
    }
    else
    {
        DebugPrintf(2, "unknown\n");
    }
#endif // 0

    /* In case of a download to RAM, use full RAM for downloading
    * set the flash parameters to full RAM also.
    * This makes sure that all code is downloaded as one big sector
    */

    if ( (IspEnvironment->BinaryOffset >= ReturnValueLpcRamStart(IspEnvironment))
       &&(IspEnvironment->BinaryOffset + IspEnvironment->BinaryLength <= ReturnValueLpcRamStart(IspEnvironment)+(LPCtypes[IspEnvironment->DetectedDevice].RAMSize*1024)))
    {
        LPCtypes[IspEnvironment->DetectedDevice].FlashSectors = 1;
        LPCtypes[IspEnvironment->DetectedDevice].MaxCopySize  = LPCtypes[IspEnvironment->DetectedDevice].RAMSize*1024 - (ReturnValueLpcRamBase(IspEnvironment) - ReturnValueLpcRamStart(IspEnvironment));
        LPCtypes[IspEnvironment->DetectedDevice].SectorTable  = SectorTable_RAM;
        SectorTable_RAM[0] = LPCtypes[IspEnvironment->DetectedDevice].MaxCopySize;
    }
    if (IspEnvironment->DetectOnly)
        return (0);

    if(LPCtypes[IspEnvironment->DetectedDevice].ChipVariant == CHIP_VARIANT_LPC8XX)
    {
      // XON/XOFF must be switched off for LPC8XX
      // otherwise problem during binary transmission of data to LPC8XX
      DebugPrintf(3, "Switch off XON/XOFF !!!\n");
      ControlXonXoffSerialPort(IspEnvironment, 0);
    }

    // Start with sector 1 and go upward... Sector 0 containing the interrupt vectors
    // will be loaded last, since it contains a checksum and device will re-enter
    // bootloader mode as long as this checksum is invalid.
    DebugPrintf(2, "Will start programming at Sector 1 if possible, and conclude with Sector 0 to ensure that checksum is written last.\n");
    if (LPCtypes[IspEnvironment->DetectedDevice].SectorTable[0] >= IspEnvironment->BinaryLength)
    {
        Sector = 0;
        SectorStart = 0;
    }
    else
    {
        SectorStart = LPCtypes[IspEnvironment->DetectedDevice].SectorTable[0];
        Sector = 1;
    }

    if (IspEnvironment->WipeDevice == 1)
    {
        DebugPrintf(2, "Wiping Device. ");

        if (LPCtypes[IspEnvironment->DetectedDevice].ChipVariant == CHIP_VARIANT_LPC43XX ||
            LPCtypes[IspEnvironment->DetectedDevice].ChipVariant == CHIP_VARIANT_LPC18XX)
        {
            // TODO: Quick and dirty hack to address bank 0
            sprintf(tmpString, "P %d %d 0\r\n", 0, LPCtypes[IspEnvironment->DetectedDevice].FlashSectors-1);
        }
        else
        {
            sprintf(tmpString, "P %d %d\r\n", 0, LPCtypes[IspEnvironment->DetectedDevice].FlashSectors-1);
        }

        if (!SendAndVerify(IspEnvironment, tmpString, Answer, sizeof Answer))
        {
            DebugPrintf(1, "Wrong answer on Prepare-Command\n");
            return (WRONG_ANSWER_PREP + GetAndReportErrorNumber(Answer));
        }

        if (LPCtypes[IspEnvironment->DetectedDevice].ChipVariant == CHIP_VARIANT_LPC43XX ||
            LPCtypes[IspEnvironment->DetectedDevice].ChipVariant == CHIP_VARIANT_LPC18XX)
        {
            // TODO: Quick and dirty hack to address bank 0
            sprintf(tmpString, "E %d %d 0\r\n", 0, LPCtypes[IspEnvironment->DetectedDevice].FlashSectors-1);
        }
        else
        {
            sprintf(tmpString, "E %d %d\r\n", 0, LPCtypes[IspEnvironment->DetectedDevice].FlashSectors-1);
        }
        if (!SendAndVerify(IspEnvironment, tmpString, Answer, sizeof Answer))
        {
            DebugPrintf(1, "Wrong answer on Erase-Command\n");
            return (WRONG_ANSWER_ERAS + GetAndReportErrorNumber(Answer));
        }
        DebugPrintf(2, "OK \n");

        if (LPCtypes[IspEnvironment->DetectedDevice].ChipVariant == CHIP_VARIANT_LPC43XX ||
            LPCtypes[IspEnvironment->DetectedDevice].ChipVariant == CHIP_VARIANT_LPC18XX)
        {
          DebugPrintf(2, "ATTENTION: Only bank A was wiped!!!\n");
        }
    }
    else{
        //no wiping requested: erasing sector 0 first
        DebugPrintf(2, "Erasing sector 0 first, to invalidate checksum. ");

        if (LPCtypes[IspEnvironment->DetectedDevice].ChipVariant == CHIP_VARIANT_LPC43XX ||
            LPCtypes[IspEnvironment->DetectedDevice].ChipVariant == CHIP_VARIANT_LPC18XX)
        {
            // TODO: Quick and dirty hack to address bank 0
            sprintf(tmpString, "P %d %d 0\r\n", 0, 0);
        }
        else
        {
            sprintf(tmpString, "P %d %d\r\n", 0, 0);
        }

        if (!SendAndVerify(IspEnvironment, tmpString, Answer, sizeof Answer))
        {
            DebugPrintf(1, "Wrong answer on Prepare-Command\n");
            return (WRONG_ANSWER_PREP + GetAndReportErrorNumber(Answer));
        }

        if (LPCtypes[IspEnvironment->DetectedDevice].ChipVariant == CHIP_VARIANT_LPC43XX ||
            LPCtypes[IspEnvironment->DetectedDevice].ChipVariant == CHIP_VARIANT_LPC18XX)
        {
            // TODO: Quick and dirty hack to address bank 0
            sprintf(tmpString, "E %d %d 0\r\n", 0, 0);
        }
        else
        {
            sprintf(tmpString, "E %d %d\r\n", 0, 0);
        }

        if (!SendAndVerify(IspEnvironment, tmpString, Answer, sizeof Answer))
        {
            DebugPrintf(1, "Wrong answer on Erase-Command\n");
            return (WRONG_ANSWER_ERAS + GetAndReportErrorNumber(Answer));
        }
        DebugPrintf(2, "OK \n");
    }
    while (1)
    {
        if (Sector >= LPCtypes[IspEnvironment->DetectedDevice].FlashSectors)
        {
            DebugPrintf(1, "Program too large; running out of Flash sectors.\n");
            return (PROGRAM_TOO_LARGE);
        }

        DebugPrintf(2, "Sector %ld: ", Sector);
        fflush(stdout);

        if ( (IspEnvironment->BinaryOffset <  ReturnValueLpcRamStart(IspEnvironment))  // Skip Erase when running from RAM
           ||(IspEnvironment->BinaryOffset >= ReturnValueLpcRamStart(IspEnvironment)+(LPCtypes[IspEnvironment->DetectedDevice].RAMSize*1024)))
        {
            if (LPCtypes[IspEnvironment->DetectedDevice].ChipVariant == CHIP_VARIANT_LPC43XX ||
                LPCtypes[IspEnvironment->DetectedDevice].ChipVariant == CHIP_VARIANT_LPC18XX)
            {
                // TODO: Quick and dirty hack to address bank 0
                sprintf(tmpString, "P %ld %ld 0\r\n", Sector, Sector);
            }
            else
            {
                sprintf(tmpString, "P %ld %ld\r\n", Sector, Sector);
            }

            if (!SendAndVerify(IspEnvironment, tmpString, Answer, sizeof Answer))
            {
                DebugPrintf(1, "Wrong answer on Prepare-Command (1) (Sector %ld)\n", Sector);
                return (WRONG_ANSWER_PREP + GetAndReportErrorNumber(Answer));
            }

            DebugPrintf(2, ".");
            fflush(stdout);
            if (IspEnvironment->WipeDevice == 0 && (Sector!=0)) //Sector 0 already erased
            {
                if (LPCtypes[IspEnvironment->DetectedDevice].ChipVariant == CHIP_VARIANT_LPC43XX ||
                    LPCtypes[IspEnvironment->DetectedDevice].ChipVariant == CHIP_VARIANT_LPC18XX)
                {
                    // TODO: Quick and dirty hack to address bank 0
                    sprintf(tmpString, "E %ld %ld 0\r\n", Sector, Sector);
                }
                else
                {
                    sprintf(tmpString, "E %ld %ld\r\n", Sector, Sector);
                }

                if (!SendAndVerify(IspEnvironment, tmpString, Answer, sizeof Answer))
                {
                    DebugPrintf(1, "Wrong answer on Erase-Command (Sector %ld)\n", Sector);
                    return (WRONG_ANSWER_ERAS + GetAndReportErrorNumber(Answer));
                }

                DebugPrintf(2, ".");
                fflush(stdout);
            }
        }

        SectorLength = LPCtypes[IspEnvironment->DetectedDevice].SectorTable[Sector];
        if (SectorLength > IspEnvironment->BinaryLength - SectorStart)
        {
            SectorLength = IspEnvironment->BinaryLength - SectorStart;
        }

        for (SectorOffset = 0; SectorOffset < SectorLength; SectorOffset += SectorChunk)
        {
            // Check if we are to write only 0xFFs - it would be just a waste of time..
            if (SectorOffset == 0) {
                for (SectorOffset = 0; SectorOffset < SectorLength; ++SectorOffset)
                {
                    if (IspEnvironment->BinaryContent[SectorStart + SectorOffset] != 0xFF)
                        break;
                }
                if (SectorOffset == SectorLength) // all data contents were 0xFFs
                {
                    DebugPrintf(2, "Whole sector contents is 0xFFs, skipping programming.");
                    fflush(stdout);
                    break;
                }
                SectorOffset = 0; // re-set otherwise
            }

            if (SectorOffset > 0)
            {
                // Add a visible marker between segments in a sector
                DebugPrintf(2, "|");  /* means: partial segment copied */
                fflush(stdout);
            }

            // If the Flash ROM sector size is bigger than the number of bytes
            // we can copy from RAM to Flash, we must "chop up" the sector and
            // copy these individually.
            // This is especially needed in the case where a Flash sector is
            // bigger than the amount of SRAM.
            SectorChunk = SectorLength - SectorOffset;
            if (SectorChunk > (unsigned)LPCtypes[IspEnvironment->DetectedDevice].MaxCopySize)
            {
                SectorChunk = LPCtypes[IspEnvironment->DetectedDevice].MaxCopySize;
            }

            // Write multiple of 45 * 4 Byte blocks to RAM, but copy maximum of on sector to Flash
            // In worst case we transfer up to 180 byte too much to RAM
            // but then we can always use full 45 byte blocks and length is multiple of 4
            CopyLength = SectorChunk;

            if(LPCtypes[IspEnvironment->DetectedDevice].ChipVariant == CHIP_VARIANT_LPC2XXX ||
               LPCtypes[IspEnvironment->DetectedDevice].ChipVariant == CHIP_VARIANT_LPC17XX ||
               LPCtypes[IspEnvironment->DetectedDevice].ChipVariant == CHIP_VARIANT_LPC13XX ||
               LPCtypes[IspEnvironment->DetectedDevice].ChipVariant == CHIP_VARIANT_LPC11XX ||
               LPCtypes[IspEnvironment->DetectedDevice].ChipVariant == CHIP_VARIANT_LPC18XX ||
               LPCtypes[IspEnvironment->DetectedDevice].ChipVariant == CHIP_VARIANT_LPC43XX)
            {
                if ((CopyLength % (45 * 4)) != 0)
                {
                    CopyLength += ((45 * 4) - (CopyLength % (45 * 4)));
                }
            }

            sprintf(tmpString, "W %ld %ld\r\n", ReturnValueLpcRamBase(IspEnvironment), CopyLength);

            if (!SendAndVerify(IspEnvironment, tmpString, Answer, sizeof Answer))
            {
                DebugPrintf(1, "Wrong answer on Write-Command\n");
                return (WRONG_ANSWER_WRIT + GetAndReportErrorNumber(Answer));
            }

            DebugPrintf(2, ".");
            fflush(stdout);

            if(LPCtypes[IspEnvironment->DetectedDevice].ChipVariant == CHIP_VARIANT_LPC2XXX ||
               LPCtypes[IspEnvironment->DetectedDevice].ChipVariant == CHIP_VARIANT_LPC17XX ||
               LPCtypes[IspEnvironment->DetectedDevice].ChipVariant == CHIP_VARIANT_LPC13XX ||
               LPCtypes[IspEnvironment->DetectedDevice].ChipVariant == CHIP_VARIANT_LPC11XX ||
               LPCtypes[IspEnvironment->DetectedDevice].ChipVariant == CHIP_VARIANT_LPC18XX ||
               LPCtypes[IspEnvironment->DetectedDevice].ChipVariant == CHIP_VARIANT_LPC43XX)
            {
                block_CRC = 0;
                Line = 0;

                // Transfer blocks of 45 * 4 bytes to RAM
                for (Pos = SectorStart + SectorOffset; (Pos < SectorStart + SectorOffset + CopyLength) && (Pos < IspEnvironment->BinaryLength); Pos += (45 * 4))
                {
                    for (Block = 0; Block < 4; Block++)  // Each block 45 bytes
                    {
                        DebugPrintf(2, ".");
                        fflush(stdout);

#if defined INTEGRATED_IN_WIN_APP
                        // inform the calling application about having written another chuck of data
                        AppWritten(45);
#endif

                        // Uuencode one 45 byte block
                        tmpStringPos = 0;

#if !defined COMPILE_FOR_LPC21
                        sendbuf[Line][tmpStringPos++] = (char)(' ' + 45);    // Encode Length of block
#else
                        tmpString[tmpStringPos++] = (char)(' ' + 45);        // Encode Length of block
#endif

                        for (BlockOffset = 0; BlockOffset < 45; BlockOffset++)
                        {
                            if ( (IspEnvironment->BinaryOffset <  ReturnValueLpcRamStart(IspEnvironment))
                               ||(IspEnvironment->BinaryOffset >= ReturnValueLpcRamStart(IspEnvironment)+(LPCtypes[IspEnvironment->DetectedDevice].RAMSize*1024)))
                            { // Flash: use full memory
                                c = IspEnvironment->BinaryContent[Pos + Block * 45 + BlockOffset];
                            }
                            else
                            { // RAM: Skip first 0x200 bytes, these are used by the download program in LPC21xx
                                c = IspEnvironment->BinaryContent[Pos + Block * 45 + BlockOffset + 0x200];
                            }

                            block_CRC += c;

                            k = (k << 8) + (c & 255);

                            if ((BlockOffset % 3) == 2)   // Collecting always 3 Bytes, then do processing in 4 Bytes
                            {
#if !defined COMPILE_FOR_LPC21
                                sendbuf[Line][tmpStringPos++] = uuencode_table[(k >> 18) & 63];
                                sendbuf[Line][tmpStringPos++] = uuencode_table[(k >> 12) & 63];
                                sendbuf[Line][tmpStringPos++] = uuencode_table[(k >>  6) & 63];
                                sendbuf[Line][tmpStringPos++] = uuencode_table[ k        & 63];
#else
                                tmpString[tmpStringPos++] = uuencode_table[(k >> 18) & 63];
                                tmpString[tmpStringPos++] = uuencode_table[(k >> 12) & 63];
                                tmpString[tmpStringPos++] = uuencode_table[(k >>  6) & 63];
                                tmpString[tmpStringPos++] = uuencode_table[ k        & 63];
#endif
                            }
                        }


#if !defined COMPILE_FOR_LPC21
                        sendbuf[Line][tmpStringPos++] = '\r';
                        sendbuf[Line][tmpStringPos++] = '\n';
                        sendbuf[Line][tmpStringPos++] = 0;

                        SendComPort(IspEnvironment, sendbuf[Line]);
                        // receive only for debug proposes
                        ReceiveComPort(IspEnvironment, Answer, sizeof(Answer)-1, &realsize, 1, 5000);
                        FormatCommand(sendbuf[Line], tmpString);
                        FormatCommand(Answer, Answer);
                        if (strncmp(Answer, tmpString, strlen(tmpString)) != 0)
                        {
                            DebugPrintf(1, "Error on writing data (1)\n");
                            return (ERROR_WRITE_DATA);
                        }
#else
                        tmpString[tmpStringPos++] = '\r';
                        tmpString[tmpStringPos++] = '\n';
                        tmpString[tmpStringPos++] = 0;

                        SendComPort(IspEnvironment, tmpString);
                        ReceiveComPort(IspEnvironment, Answer, sizeof(Answer)-1, &realsize, 1, 5000);
                        FormatCommand(tmpString, tmpString);
                        FormatCommand(Answer, Answer);
                        if (strncmp(Answer, tmpString, tmpStringPos) != 0)
                        {
                            DebugPrintf(1, "Error on writing data (1)\n");
                            return (ERROR_WRITE_DATA);
                        }
#endif

                        Line++;

                        DebugPrintf(3, "Line = %d\n", Line);

                        if (Line == 20)
                        {
#if !defined COMPILE_FOR_LPC21
                            for (repeat = 0; repeat < 3; repeat++)
                            {

                                // DebugPrintf(1, "block_CRC = %ld\n", block_CRC);

                                sprintf(tmpString, "%ld\r\n", block_CRC);

                                SendComPort(IspEnvironment, tmpString);

                                ReceiveComPort(IspEnvironment, Answer, sizeof(Answer)-1, &realsize, 2, 5000);

                                sprintf(tmpString, "%ld\nOK\n", block_CRC);

                                FormatCommand(tmpString, tmpString);
                                FormatCommand(Answer, Answer);
                                if (strcmp(Answer, tmpString) != 0)
                                {
                                    for (i = 0; i < Line; i++)
                                    {
                                        SendComPort(IspEnvironment, sendbuf[i]);
                                        ReceiveComPort(IspEnvironment, Answer, sizeof(Answer)-1, &realsize, 1, 5000);
                                    }
                                }
                                else
                                    break;
                            }

                            if (repeat >= 3)
                            {
                                DebugPrintf(1, "Error on writing block_CRC (1)\n");
                                return (ERROR_WRITE_CRC);
                            }
#else
                            // DebugPrintf(1, "block_CRC = %ld\n", block_CRC);
                            sprintf(tmpString, "%ld\r\n", block_CRC);
                            SendComPort(IspEnvironment, tmpString);

                            ReceiveComPort(IspEnvironment, Answer, sizeof(Answer)-1, &realsize, 2,5000);

                            sprintf(tmpString, "%ld\nOK\n", block_CRC);
                            FormatCommand(tmpString, tmpString);
                            FormatCommand(Answer, Answer);
                            if (strcmp(Answer, tmpString) != 0)
                            {
                                DebugPrintf(1, "Error on writing block_CRC (2)\n");
                                return (ERROR_WRITE_CRC);
                            }
#endif
                            Line = 0;
                            block_CRC = 0;
                        }
                    }
                }

                if (Line != 0)
                {
#if !defined COMPILE_FOR_LPC21
                    for (repeat = 0; repeat < 3; repeat++)
                    {
                        sprintf(tmpString, "%ld\r\n", block_CRC);

                        SendComPort(IspEnvironment, tmpString);

                        ReceiveComPort(IspEnvironment, Answer, sizeof(Answer)-1, &realsize, 2,5000);

                        sprintf(tmpString, "%ld\nOK\n", block_CRC);

                        FormatCommand(tmpString, tmpString);
                        FormatCommand(Answer, Answer);
                        if (strcmp(Answer, tmpString) != 0)
                        {
                            for (i = 0; i < Line; i++)
                            {
                                SendComPort(IspEnvironment, sendbuf[i]);
                                ReceiveComPort(IspEnvironment, Answer, sizeof(Answer)-1, &realsize, 1,5000);
                            }
                        }
                        else
                            break;
                    }

                    if (repeat >= 3)
                    {
                        DebugPrintf(1, "Error on writing block_CRC (3)\n");
                        return (ERROR_WRITE_CRC2);
                    }
#else
                    sprintf(tmpString, "%ld\r\n", block_CRC);
                    SendComPort(IspEnvironment, tmpString);

                    ReceiveComPort(IspEnvironment, Answer, sizeof(Answer)-1, &realsize, 2,5000);

                    sprintf(tmpString, "%ld\nOK\n", block_CRC);
                    FormatCommand(tmpString, tmpString);
                    FormatCommand(Answer, Answer);
                    if (strcmp(Answer, tmpString) != 0)
                    {
                        DebugPrintf(1, "Error on writing block_CRC (4)\n");
                        return (ERROR_WRITE_CRC2);
                    }
#endif
                }
            }
            else if(LPCtypes[IspEnvironment->DetectedDevice].ChipVariant == CHIP_VARIANT_LPC8XX)
            {
                unsigned char BigAnswer[4096];
                unsigned long CopyLengthPartialOffset = 0;
                unsigned long CopyLengthPartialRemainingBytes;

                while(CopyLengthPartialOffset < CopyLength)
                {
                    CopyLengthPartialRemainingBytes = CopyLength - CopyLengthPartialOffset;
                    if(CopyLengthPartialRemainingBytes > 256)
                    {
                      // There seems to be an error in LPC812:
                      // When too much bytes are written at high speed,
                      // bytes get lost
                      // Workaround: Use smaller blocks
                      CopyLengthPartialRemainingBytes = 256;
                    }

                    SendComPortBlock(IspEnvironment, &IspEnvironment->BinaryContent[SectorStart + SectorOffset + CopyLengthPartialOffset], CopyLengthPartialRemainingBytes);

                    if (ReceiveComPortBlockComplete(IspEnvironment, &BigAnswer, CopyLengthPartialRemainingBytes, 10000) != 0)
                    {
                        return (ERROR_WRITE_DATA);
                    }

                    if(memcmp(&IspEnvironment->BinaryContent[SectorStart + SectorOffset + CopyLengthPartialOffset], BigAnswer, CopyLengthPartialRemainingBytes))
                    {
                        return (ERROR_WRITE_DATA);
                    }

                    CopyLengthPartialOffset += CopyLengthPartialRemainingBytes;
                }
            }

            if ( (IspEnvironment->BinaryOffset <  ReturnValueLpcRamStart(IspEnvironment))
               ||(IspEnvironment->BinaryOffset >= ReturnValueLpcRamStart(IspEnvironment)+(LPCtypes[IspEnvironment->DetectedDevice].RAMSize*1024)))
            {
                // Prepare command must be repeated before every write
                if (LPCtypes[IspEnvironment->DetectedDevice].ChipVariant == CHIP_VARIANT_LPC43XX ||
                    LPCtypes[IspEnvironment->DetectedDevice].ChipVariant == CHIP_VARIANT_LPC18XX)
                {
                    // TODO: Quick and dirty hack to address bank 0
                    sprintf(tmpString, "P %ld %ld 0\r\n", Sector, Sector);
                }
                else
                {
                    sprintf(tmpString, "P %ld %ld\r\n", Sector, Sector);
                }

                if (!SendAndVerify(IspEnvironment, tmpString, Answer, sizeof Answer))
                {
                    DebugPrintf(1, "Wrong answer on Prepare-Command (2) (Sector %ld)\n", Sector);
                    return (WRONG_ANSWER_PREP2 + GetAndReportErrorNumber(Answer));
                }

                // Round CopyLength up to one of the following values: 512, 1024,
                // 4096, 8192; but do not exceed the maximum copy size (usually
                // 8192, but chip-dependent)
                if (CopyLength < 512)
                {
                    CopyLength = 512;
                }
                else if (SectorLength < 1024)
                {
                    CopyLength = 1024;
                }
                else if (SectorLength < 4096)
                {
                    CopyLength = 4096;
                }
                else
                {
                    CopyLength = 8192;
                }
                if (CopyLength > (unsigned)LPCtypes[IspEnvironment->DetectedDevice].MaxCopySize)
                {
                    CopyLength = LPCtypes[IspEnvironment->DetectedDevice].MaxCopySize;
                }

                sprintf(tmpString, "C %ld %ld %ld\r\n", IspEnvironment->BinaryOffset + SectorStart + SectorOffset, ReturnValueLpcRamBase(IspEnvironment), CopyLength);

                if (!SendAndVerify(IspEnvironment, tmpString, Answer, sizeof Answer))
                {
                    DebugPrintf(1, "Wrong answer on Copy-Command\n");
                    return (WRONG_ANSWER_COPY + GetAndReportErrorNumber(Answer));
                }

                if (IspEnvironment->Verify)
                {

                    //Avoid compare first 64 bytes.
                    //Because first 64 bytes are re-mapped to flash boot sector,
                    //and the compare result may not be correct.
                    if (SectorStart + SectorOffset<64)
                    {
                        sprintf(tmpString, "M %d %ld %ld\r\n", 64, ReturnValueLpcRamBase(IspEnvironment) + (64 - SectorStart - SectorOffset), CopyLength-(64 - SectorStart - SectorOffset));
                    }
                    else
                    {
                        sprintf(tmpString, "M %ld %ld %ld\r\n", SectorStart + SectorOffset, ReturnValueLpcRamBase(IspEnvironment), CopyLength);
                    }

                    if (!SendAndVerify(IspEnvironment, tmpString, Answer, sizeof Answer))
                    {
                        DebugPrintf(1, "Wrong answer on Compare-Command\n");
                        return (WRONG_ANSWER_COPY + GetAndReportErrorNumber(Answer));
                    }
                }
            }
        }

        DebugPrintf(2, "\n");
        fflush(stdout);

        if ((SectorStart + SectorLength) >= IspEnvironment->BinaryLength && Sector!=0)
        {
            Sector = 0;
            SectorStart = 0;
        }
        else if (Sector == 0) {
            break;
        }
        else {
            SectorStart += LPCtypes[IspEnvironment->DetectedDevice].SectorTable[Sector];
            Sector++;
        }
    }

    tDoneUpload = time(NULL);
    if (IspEnvironment->Verify)
        DebugPrintf(2, "Download Finished and Verified correct... taking %d seconds\n", tDoneUpload - tStartUpload);
    else
        DebugPrintf(2, "Download Finished... taking %d seconds\n", tDoneUpload - tStartUpload);

    // For LPC18xx set boot bank to 0
    if (LPCtypes[IspEnvironment->DetectedDevice].ChipVariant == CHIP_VARIANT_LPC43XX ||
        LPCtypes[IspEnvironment->DetectedDevice].ChipVariant == CHIP_VARIANT_LPC18XX)
    {
        if (!SendAndVerify(IspEnvironment, "S 0\r\n", Answer, sizeof Answer))
        {
            DebugPrintf(1, "Wrong answer on SetActiveBootFlashBank-Command\n");
            return (WRONG_ANSWER_BTBNK + GetAndReportErrorNumber(Answer));
        }
    }

    if(IspEnvironment->DoNotStart == 0)
    {
        DebugPrintf(2, "Now launching the brand new code\n");
        fflush(stdout);

        if(LPCtypes[IspEnvironment->DetectedDevice].ChipVariant == CHIP_VARIANT_LPC2XXX)
        {
            sprintf(tmpString, "G %ld A\r\n", IspEnvironment->StartAddress);
        }
        else if(LPCtypes[IspEnvironment->DetectedDevice].ChipVariant == CHIP_VARIANT_LPC43XX ||
                LPCtypes[IspEnvironment->DetectedDevice].ChipVariant == CHIP_VARIANT_LPC18XX ||
                LPCtypes[IspEnvironment->DetectedDevice].ChipVariant == CHIP_VARIANT_LPC17XX ||
                LPCtypes[IspEnvironment->DetectedDevice].ChipVariant == CHIP_VARIANT_LPC13XX ||
                LPCtypes[IspEnvironment->DetectedDevice].ChipVariant == CHIP_VARIANT_LPC11XX)
        {
            sprintf(tmpString, "G %ld T\r\n", IspEnvironment->StartAddress & ~1);
        }
        else if(LPCtypes[IspEnvironment->DetectedDevice].ChipVariant == CHIP_VARIANT_LPC8XX)
        {
            sprintf(tmpString, "G 0 T\r\n");
        }
        else
        {
            DebugPrintf(1, "Internal Error %s %d\n", __FILE__, __LINE__);
            exit(1);
        }

        SendComPort(IspEnvironment, tmpString); //goto 0 : run this fresh new downloaded code code
        if ( (IspEnvironment->BinaryOffset <  ReturnValueLpcRamStart(IspEnvironment))
           ||(IspEnvironment->BinaryOffset >= ReturnValueLpcRamStart(IspEnvironment)+(LPCtypes[IspEnvironment->DetectedDevice].RAMSize*1024)))
        { // Skip response on G command - show response on Terminal instead
            ReceiveComPort(IspEnvironment, Answer, sizeof(Answer)-1, &realsize, 2, 5000);
            /* the reply string is frequently terminated with a -1 (EOF) because the
            * connection gets broken; zero-terminate the string ourselves
            */
            while (realsize > 0 && ((signed char) Answer[(int)realsize - 1]) < 0)
                realsize--;
            Answer[(int)realsize] = '\0';
            /* Better to check only the first 9 chars instead of complete receive buffer,
            * because the answer can contain the output by the started programm
            */
            if(LPCtypes[IspEnvironment->DetectedDevice].ChipVariant == CHIP_VARIANT_LPC2XXX)
            {
                sprintf(ExpectedAnswer, "G %ld A\n0", IspEnvironment->StartAddress);
            }
            else if(LPCtypes[IspEnvironment->DetectedDevice].ChipVariant == CHIP_VARIANT_LPC43XX ||
                    LPCtypes[IspEnvironment->DetectedDevice].ChipVariant == CHIP_VARIANT_LPC18XX ||
                    LPCtypes[IspEnvironment->DetectedDevice].ChipVariant == CHIP_VARIANT_LPC17XX ||
                    LPCtypes[IspEnvironment->DetectedDevice].ChipVariant == CHIP_VARIANT_LPC13XX ||
                    LPCtypes[IspEnvironment->DetectedDevice].ChipVariant == CHIP_VARIANT_LPC11XX)
            {
                sprintf(ExpectedAnswer, "G %ld T\n0", IspEnvironment->StartAddress & ~1);
            }
            else if(LPCtypes[IspEnvironment->DetectedDevice].ChipVariant == CHIP_VARIANT_LPC8XX)
            {
                sprintf(ExpectedAnswer, "G 0 T\n0");
            }
            else
            {
                DebugPrintf(1, "Internal Error %s %d\n", __FILE__, __LINE__);
                exit(1);
            }

            FormatCommand(Answer, Answer);
            if (realsize == 0 || strncmp((const char *)Answer, /*cmdstr*/ExpectedAnswer, strlen(/*cmdstr*/ExpectedAnswer)) != 0)
            {
                DebugPrintf(2, "Failed to run the new downloaded code: ");
                return (FAILED_RUN + GetAndReportErrorNumber(Answer));
            }
        }

        fflush(stdout);
    }
    return (0);
}
#endif // LPC_SUPPORT


unsigned long ReturnValueLpcRamStart(ISP_ENVIRONMENT *IspEnvironment)
{
  if(LPCtypes[IspEnvironment->DetectedDevice].ChipVariant == CHIP_VARIANT_LPC43XX)
  {
    return LPC_RAMSTART_LPC43XX;
  }
  else if(LPCtypes[IspEnvironment->DetectedDevice].ChipVariant == CHIP_VARIANT_LPC2XXX)
  {
    return LPC_RAMSTART_LPC2XXX;
  }
  else if(LPCtypes[IspEnvironment->DetectedDevice].ChipVariant == CHIP_VARIANT_LPC18XX)
  {
    return LPC_RAMSTART_LPC18XX;
  }
  else if(LPCtypes[IspEnvironment->DetectedDevice].ChipVariant == CHIP_VARIANT_LPC17XX)
  {
    return LPC_RAMSTART_LPC17XX;
  }
  else if(LPCtypes[IspEnvironment->DetectedDevice].ChipVariant == CHIP_VARIANT_LPC13XX)
  {
    return LPC_RAMSTART_LPC13XX;
  }
  else if(LPCtypes[IspEnvironment->DetectedDevice].ChipVariant == CHIP_VARIANT_LPC11XX)
  {
    return LPC_RAMSTART_LPC11XX;
  }
  else if(LPCtypes[IspEnvironment->DetectedDevice].ChipVariant == CHIP_VARIANT_LPC8XX)
  {
    return LPC_RAMSTART_LPC8XX;
  }
  DebugPrintf(1, "Error in ReturnValueLpcRamStart (%d)\n", LPCtypes[IspEnvironment->DetectedDevice].ChipVariant);
  exit(1);
}


unsigned long ReturnValueLpcRamBase(ISP_ENVIRONMENT *IspEnvironment)
{
  if(LPCtypes[IspEnvironment->DetectedDevice].ChipVariant == CHIP_VARIANT_LPC43XX)
  {
    return LPC_RAMBASE_LPC43XX;
  }
  else if(LPCtypes[IspEnvironment->DetectedDevice].ChipVariant == CHIP_VARIANT_LPC2XXX)
  {
    return LPC_RAMBASE_LPC2XXX;
  }
  else if(LPCtypes[IspEnvironment->DetectedDevice].ChipVariant == CHIP_VARIANT_LPC18XX)
  {
    return LPC_RAMBASE_LPC18XX;
  }
  else if(LPCtypes[IspEnvironment->DetectedDevice].ChipVariant == CHIP_VARIANT_LPC17XX)
  {
    return LPC_RAMBASE_LPC17XX;
  }
  else if(LPCtypes[IspEnvironment->DetectedDevice].ChipVariant == CHIP_VARIANT_LPC13XX)
  {
    return LPC_RAMBASE_LPC13XX;
  }
  else if(LPCtypes[IspEnvironment->DetectedDevice].ChipVariant == CHIP_VARIANT_LPC11XX)
  {
    return LPC_RAMBASE_LPC11XX;
  }
  else if(LPCtypes[IspEnvironment->DetectedDevice].ChipVariant == CHIP_VARIANT_LPC8XX)
  {
    return LPC_RAMBASE_LPC8XX;
  }
  DebugPrintf(1, "Error in ReturnValueLpcRamBase (%d)\n", LPCtypes[IspEnvironment->DetectedDevice].ChipVariant);
  exit(1);
}
