/*
 * This file is part of the Micro Python project, http://micropython.org/
 *
 * The MIT License (MIT)
 *
 * Copyright (c) 2013, 2014 Damien P. George
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include <stdio.h>
#include <string.h>

#include "py/nlr.h"
#include "py/smallint.h"
#include "py/obj.h"
#include "py/runtime.h"
#include "py/gc.h"

#include "timeutils.h"

#include "modpyb.h"
#include "ciaanxp_mphal.h"


/// \function time()
/// Returns the number of seconds, as an integer, since 1/1/2000.
STATIC mp_obj_t time_time(void) {
    
    uint32_t yr;
	uint32_t mon;
	uint32_t day;
	uint32_t wd;
	uint32_t hr;
	uint32_t min;
	uint32_t sec; 

	mp_hal_getTimeRTC(&hr, &min, &sec, &day, &mon, &yr, &wd);
		
    return mp_obj_new_int(timeutils_seconds_since_2000(yr, mon, day, hr, min, sec));
}
MP_DEFINE_CONST_FUN_OBJ_0(time_time_obj, time_time);


/// \function localtime([secs])
/// Convert a time expressed in seconds since Jan 1, 2000 into an 8-tuple which
/// contains: (year, month, mday, hour, minute, second, weekday, yearday)
/// If secs is not provided or None, then the current time from the RTC is used.
/// year includes the century (for example 2014)
/// month   is 1-12
/// mday    is 1-31
/// hour    is 0-23
/// minute  is 0-59
/// second  is 0-59
/// weekday is 0-6 for Mon-Sun.
/// yearday is 1-366
STATIC mp_obj_t time_localtime(mp_uint_t n_args, const mp_obj_t *args) {
    if (n_args == 0 || args[0] == mp_const_none) {
        // get current date and time
		uint32_t yr;
		uint32_t mon;
		uint32_t day;
		uint32_t wd;
		uint32_t hr;
		uint32_t min;
		uint32_t sec; 
		mp_hal_getTimeRTC(&hr, &min, &sec, &day, &mon, &yr, &wd);

        mp_obj_t tuple[8] = {
            mp_obj_new_int(yr),
            mp_obj_new_int(mon),
            mp_obj_new_int(day),
            mp_obj_new_int(hr),
            mp_obj_new_int(min),
            mp_obj_new_int(sec),
            mp_obj_new_int(wd),
            mp_obj_new_int(timeutils_year_day(yr, mon, day)),
        };
        return mp_obj_new_tuple(8, tuple);
    } else {
        mp_int_t seconds = mp_obj_get_int(args[0]);
        timeutils_struct_time_t tm;
        timeutils_seconds_since_2000_to_struct_time(seconds, &tm);
        mp_obj_t tuple[8] = {
            tuple[0] = mp_obj_new_int(tm.tm_year),
            tuple[1] = mp_obj_new_int(tm.tm_mon),
            tuple[2] = mp_obj_new_int(tm.tm_mday),
            tuple[3] = mp_obj_new_int(tm.tm_hour),
            tuple[4] = mp_obj_new_int(tm.tm_min),
            tuple[5] = mp_obj_new_int(tm.tm_sec),
            tuple[6] = mp_obj_new_int(tm.tm_wday),
            tuple[7] = mp_obj_new_int(tm.tm_yday),
        };
        return mp_obj_new_tuple(8, tuple);
    }
}
MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(time_localtime_obj, 0, 1, time_localtime);


/// \function mktime()
/// This is inverse function of localtime. It's argument is a full 8-tuple
/// which expresses a time as per localtime. It returns an integer which is
/// the number of seconds since Jan 1, 2000.
STATIC mp_obj_t time_mktime(mp_obj_t tuple) {

    mp_uint_t len;
    mp_obj_t *elem;

    mp_obj_get_array(tuple, &len, &elem);

    // localtime generates a tuple of len 8. CPython uses 9, so we accept both.
    if (len < 8 || len > 9) {
        nlr_raise(mp_obj_new_exception_msg_varg(&mp_type_TypeError, "mktime needs a tuple of length 8 or 9 (%d given)", len));
    }

    return mp_obj_new_int_from_uint(timeutils_mktime(mp_obj_get_int(elem[0]),
            mp_obj_get_int(elem[1]), mp_obj_get_int(elem[2]), mp_obj_get_int(elem[3]),
            mp_obj_get_int(elem[4]), mp_obj_get_int(elem[5])));
}
MP_DEFINE_CONST_FUN_OBJ_1(time_mktime_obj, time_mktime);


/// \function sleep(seconds)
/// Sleep for the given number of seconds.  Seconds can be a floating-point number to
/// sleep for a fractional number of seconds.
STATIC mp_obj_t time_sleep(mp_obj_t seconds_o) {
#if MICROPY_PY_BUILTINS_FLOAT
    if (MP_OBJ_IS_INT(seconds_o)) {
#endif
        mp_hal_milli_delay(1000 * mp_obj_get_int(seconds_o));
#if MICROPY_PY_BUILTINS_FLOAT
    } else {
        mp_hal_milli_delay((uint32_t)(1000 * mp_obj_get_float(seconds_o)));
    }
#endif
    return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_1(time_sleep_obj, time_sleep);


STATIC const mp_map_elem_t time_module_globals_table[] = {
    { MP_OBJ_NEW_QSTR(MP_QSTR___name__), MP_OBJ_NEW_QSTR(MP_QSTR_utime) },
    { MP_OBJ_NEW_QSTR(MP_QSTR_time), (mp_obj_t)&time_time_obj },
	
    { MP_OBJ_NEW_QSTR(MP_QSTR_localtime), (mp_obj_t)&time_localtime_obj },    
    { MP_OBJ_NEW_QSTR(MP_QSTR_mktime), (mp_obj_t)&time_mktime_obj },
    { MP_OBJ_NEW_QSTR(MP_QSTR_sleep), (mp_obj_t)&time_sleep_obj },

    /*
    { MP_OBJ_NEW_QSTR(MP_QSTR_sleep_ms), (mp_obj_t)&time_sleep_ms_obj },
    { MP_OBJ_NEW_QSTR(MP_QSTR_sleep_us), (mp_obj_t)&time_sleep_us_obj },
    { MP_OBJ_NEW_QSTR(MP_QSTR_ticks_ms), (mp_obj_t)&time_ticks_ms_obj },
    { MP_OBJ_NEW_QSTR(MP_QSTR_ticks_us), (mp_obj_t)&time_ticks_us_obj },
    { MP_OBJ_NEW_QSTR(MP_QSTR_ticks_cpu), (mp_obj_t)&time_ticks_cpu_obj },
    { MP_OBJ_NEW_QSTR(MP_QSTR_ticks_diff), (mp_obj_t)&time_ticks_diff_obj },
    */
};

STATIC MP_DEFINE_CONST_DICT(time_module_globals, time_module_globals_table);

const mp_obj_module_t mp_module_utime = {
    .base = { &mp_type_module },
    .name = MP_QSTR_utime,
    .globals = (mp_obj_dict_t*)&time_module_globals,
};
