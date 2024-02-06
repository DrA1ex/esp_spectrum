#pragma once

#define __DEBUG_LEVEL_VERBOSE 0

#ifndef DEBUG_LEVEL
#define DEBUG_LEVEL 1
#endif

#ifdef DEBUG
#define D_PRINT(x) Serial.println(x)
#define D_PRINTF(...) Serial.printf(__VA_ARGS__)
#define D_WRITE(x) Serial.print(x)

#if DEBUG_LEVEL == __DEBUG_LEVEL_VERBOSE
#define VERBOSE(ARG) (ARG)
#else
#define VERBOSE(ARG)
#endif

#else
#define D_PRINT(x)
#define D_PRINTF(...)
#define D_WRITE(x)

#define VERBOSE(ARG)
#endif