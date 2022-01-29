#pragma once

#include <math.h>

#define LOG2DB 8.6858896380650365530225783783321f   // 20 / ln(10)
#define DB2LOG 0.11512925464970228420089957273422f // ln(10) / 20

#define NORM(v, min_v, max_v) \
    ((v) - (min_v)) / ((max_v) - (min_v))

#define PLAIN(v, min_v, max_v) \
    ((v) * ((max_v) - (min_v)) + (min_v))

#define HDR_GAIN_MIN               -12.0f
#define HDR_GAIN_MAX                12.0f
#define HDR_GAIN_DEFAULT_N          NORM(-5.0f, HDR_GAIN_MIN, HDR_GAIN_MAX)

static inline float db2norm(float db, float min, float max)
{
    return (db - min) / (max - min);
}

static inline float norm2db(float v, float min, float max)
{
    return (v * max) + (min * (1.0f - v));
}

static inline float normdb2factor(float v, float min, float max)
{
    return powf(10.0f, norm2db(v, min, max) / 20.0f);
}

enum {
    /** parameter ID */
    kBypassId = 0,      ///< Bypass value (we will handle the bypass process) (is automatable)
    kHDRGainId,
    kNrParams
};
