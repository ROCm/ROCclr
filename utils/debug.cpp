/* Copyright (c) 2008 - 2021 Advanced Micro Devices, Inc.

 Permission is hereby granted, free of charge, to any person obtaining a copy
 of this software and associated documentation files (the "Software"), to deal
 in the Software without restriction, including without limitation the rights
 to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 copies of the Software, and to permit persons to whom the Software is
 furnished to do so, subject to the following conditions:

 The above copyright notice and this permission notice shall be included in
 all copies or substantial portions of the Software.

 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 THE SOFTWARE. */

#include "top.hpp"
#include "utils/debug.hpp"
#include "os/os.hpp"

#if !defined(AMD_LOG_LEVEL)
#include "utils/flags.hpp"
#endif

#include <cstdlib>
#include <cstdio>
#include <cstdarg>

#ifdef _WIN32
#include <windows.h>
#endif  // _WIN32

namespace amd {

FILE* outFile = stderr;

// ================================================================================================
//! \cond ignore
extern "C" void breakpoint(void) {
#ifdef _MSC_VER
  DebugBreak();
#endif  // _MSC_VER
}
//! \endcond

// ================================================================================================
void report_fatal(const char* file, int line, const char* message) {
  // FIXME_lmoriche: Obfuscate the message string
  #if (defined(DEBUG))
    fprintf(outFile, "%s:%d: %s\n", file, line, message);
  #else
    fprintf(outFile, "%s\n", message);
  #endif
  ::abort();
}

// ================================================================================================
void report_warning(const char* message) { fprintf(outFile, "Warning: %s\n", message); }

// ================================================================================================
void log_entry(LogLevel level, const char* file, int line, const char* message) {
  if (level == LOG_NONE) {
    return;
  }
  fprintf(outFile, ":%d:%s:%d: %s\n", level, file, line, message);
  fflush(outFile);
}

// ================================================================================================
void log_timestamped(LogLevel level, const char* file, int line, const char* message) {
  static bool gotstart = false;  // not thread-safe, but not scary if fails
  static uint64_t start;

  if (!gotstart) {
    start = Os::timeNanos();
    gotstart = true;
  }

  uint64_t time = Os::timeNanos() - start;
  if (level == LOG_NONE) {
    return;
  }
#if 0
    fprintf(outFile, ":%d:%s:%d: (%010lld) %s\n", level, file, line, time, message);
#else  // if you prefer fixed-width fields
  fprintf(outFile, ":% 2d:%15s:% 5d: (%010lld) us %s\n", level, file, line, time / 1000ULL,
          message);
#endif
  fflush(outFile);
}

// ================================================================================================
void log_printf(LogLevel level, const char* file, int line, const char* format, ...) {
  va_list ap;

  va_start(ap, format);
  char message[4096];
  vsnprintf(message, sizeof(message), format, ap);
  va_end(ap);
  uint64_t timeUs = Os::timeNanos() / 1000ULL;
  fprintf(outFile, ":%d:%-25s:%-4d: %010lld us: %s\n", level, file, line, timeUs/1ULL, message);
  fflush(outFile);
}

// ================================================================================================
void log_printf(LogLevel level, const char* file, int line, uint64_t* start,
                const char* format, ...) {
  va_list ap;

  va_start(ap, format);
  char message[4096];
  vsnprintf(message, sizeof(message), format, ap);
  va_end(ap);
  uint64_t timeUs = Os::timeNanos() / 1000ULL;
  if (start == 0 || *start == 0) {
     fprintf(outFile, ":%d:%-25s:%-4d: %010lld us: %s\n", level, file, line, timeUs/1ULL, message);
  } else {
     fprintf(outFile, ":%d:%-25s:%-4d: %010lld us: %s: duration: %lld us\n", level, file, line,
             timeUs/1ULL, message, (timeUs - *start)/1ULL);
  }
  fflush(outFile);
  if (*start == 0) {
     *start = timeUs;
  }
}

}  // namespace amd
