#include "config.h"

#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "prelude.h"


static void cast_data(idmef_value_t *value)
{
        assert(_idmef_value_cast(value, IDMEF_VALUE_TYPE_INT8, -1) < 0);
        assert(_idmef_value_cast(value, IDMEF_VALUE_TYPE_UINT8, -1) < 0);
        assert(_idmef_value_cast(value, IDMEF_VALUE_TYPE_INT16, -1) < 0);
        assert(_idmef_value_cast(value, IDMEF_VALUE_TYPE_UINT16, -1) < 0);
        assert(_idmef_value_cast(value, IDMEF_VALUE_TYPE_INT32, -1) < 0);
        assert(_idmef_value_cast(value, IDMEF_VALUE_TYPE_UINT32, -1) < 0);
        assert(_idmef_value_cast(value, IDMEF_VALUE_TYPE_INT64, -1) < 0);
        assert(_idmef_value_cast(value, IDMEF_VALUE_TYPE_UINT64, -1) < 0);
        assert(_idmef_value_cast(value, IDMEF_VALUE_TYPE_FLOAT, -1) < 0);
        assert(_idmef_value_cast(value, IDMEF_VALUE_TYPE_DOUBLE, -1) < 0);
        assert(_idmef_value_cast(value, IDMEF_VALUE_TYPE_TIME, -1) < 0);
        assert(_idmef_value_cast(value, IDMEF_VALUE_TYPE_ENUM, -1) < 0);
        assert(_idmef_value_cast(value, IDMEF_VALUE_TYPE_LIST, -1) < 0);
        assert(_idmef_value_cast(value, IDMEF_VALUE_TYPE_CLASS, -1) < 0);

        assert(_idmef_value_cast(value, IDMEF_VALUE_TYPE_STRING, -1) == 0);
        assert(idmef_value_get_type(value) == IDMEF_VALUE_TYPE_STRING);
}

static void cast_int8(void)
{
        idmef_value_t *value;

        assert(idmef_value_new_int8(&value, INT8_MIN) == 0);
        assert(_idmef_value_cast(value, IDMEF_VALUE_TYPE_UINT8, -1) < 0);
        assert(_idmef_value_cast(value, IDMEF_VALUE_TYPE_UINT16, -1) < 0);
        assert(_idmef_value_cast(value, IDMEF_VALUE_TYPE_UINT32, -1) < 0);
        assert(_idmef_value_cast(value, IDMEF_VALUE_TYPE_UINT64, -1) < 0);

        assert(_idmef_value_cast(value, IDMEF_VALUE_TYPE_INT16, -1) == 0);
        assert(idmef_value_get_type(value) == IDMEF_VALUE_TYPE_INT16);
        assert(idmef_value_get_int16(value) == INT8_MIN);
        idmef_value_destroy(value);

        assert(idmef_value_new_int8(&value, INT8_MIN) == 0);
        assert(_idmef_value_cast(value, IDMEF_VALUE_TYPE_INT32, -1) == 0);
        assert(idmef_value_get_type(value) == IDMEF_VALUE_TYPE_INT32);
        assert(idmef_value_get_int32(value) == INT8_MIN);
        idmef_value_destroy(value);

        assert(idmef_value_new_int8(&value, INT8_MIN) == 0);
        assert(_idmef_value_cast(value, IDMEF_VALUE_TYPE_INT64, -1) == 0);
        assert(idmef_value_get_type(value) == IDMEF_VALUE_TYPE_INT64);
        assert(idmef_value_get_int64(value) == INT8_MIN);
        idmef_value_destroy(value);

        assert(idmef_value_new_int8(&value, INT8_MIN) == 0);
        assert(_idmef_value_cast(value, IDMEF_VALUE_TYPE_FLOAT, -1) == 0);
        assert(idmef_value_get_type(value) == IDMEF_VALUE_TYPE_FLOAT);
        assert(idmef_value_get_float(value) == INT8_MIN);
        idmef_value_destroy(value);

        assert(idmef_value_new_int8(&value, INT8_MIN) == 0);
        assert(_idmef_value_cast(value, IDMEF_VALUE_TYPE_DOUBLE, -1) == 0);
        assert(idmef_value_get_type(value) == IDMEF_VALUE_TYPE_DOUBLE);
        assert(idmef_value_get_double(value) == INT8_MIN);
        idmef_value_destroy(value);

        assert(idmef_value_new_int8(&value, INT8_MIN) == 0);
        assert(_idmef_value_cast(value, IDMEF_VALUE_TYPE_DATA, -1) == 0);
        assert(idmef_value_get_type(value) == IDMEF_VALUE_TYPE_DATA);
        assert((int) idmef_data_get_uint32(idmef_value_get_data(value)) == INT8_MIN);
        idmef_value_destroy(value);
}

static void cast_int16(void)
{
        idmef_value_t *value;

        assert(idmef_value_new_int16(&value, INT16_MIN) == 0);
        assert(_idmef_value_cast(value, IDMEF_VALUE_TYPE_INT8, -1) < 0);
        assert(_idmef_value_cast(value, IDMEF_VALUE_TYPE_UINT8, -1) < 0);
        assert(_idmef_value_cast(value, IDMEF_VALUE_TYPE_UINT16, -1) < 0);
        assert(_idmef_value_cast(value, IDMEF_VALUE_TYPE_UINT32, -1) < 0);
        assert(_idmef_value_cast(value, IDMEF_VALUE_TYPE_UINT64, -1) < 0);

        assert(_idmef_value_cast(value, IDMEF_VALUE_TYPE_INT32, -1) == 0);
        assert(idmef_value_get_type(value) == IDMEF_VALUE_TYPE_INT32);
        assert(idmef_value_get_int32(value) == INT16_MIN);
        idmef_value_destroy(value);

        assert(idmef_value_new_int16(&value, INT16_MIN) == 0);
        assert(_idmef_value_cast(value, IDMEF_VALUE_TYPE_INT64, -1) == 0);
        assert(idmef_value_get_type(value) == IDMEF_VALUE_TYPE_INT64);
        assert(idmef_value_get_int64(value) == INT16_MIN);
        idmef_value_destroy(value);

        assert(idmef_value_new_int16(&value, INT16_MIN) == 0);
        assert(_idmef_value_cast(value, IDMEF_VALUE_TYPE_FLOAT, -1) == 0);
        assert(idmef_value_get_type(value) == IDMEF_VALUE_TYPE_FLOAT);
        assert(idmef_value_get_float(value) == INT16_MIN);
        idmef_value_destroy(value);

        assert(idmef_value_new_int16(&value, INT16_MIN) == 0);
        assert(_idmef_value_cast(value, IDMEF_VALUE_TYPE_DOUBLE, -1) == 0);
        assert(idmef_value_get_type(value) == IDMEF_VALUE_TYPE_DOUBLE);
        assert(idmef_value_get_double(value) == INT16_MIN);
        idmef_value_destroy(value);

        assert(idmef_value_new_int16(&value, INT16_MIN) == 0);
        assert(_idmef_value_cast(value, IDMEF_VALUE_TYPE_DATA, -1) == 0);
        assert(idmef_value_get_type(value) == IDMEF_VALUE_TYPE_DATA);
        assert((int) idmef_data_get_uint32(idmef_value_get_data(value)) == INT16_MIN);
        idmef_value_destroy(value);
}


static void cast_int32(void)
{
        idmef_value_t *value;

        assert(idmef_value_new_int32(&value, INT32_MIN) == 0);
        assert(_idmef_value_cast(value, IDMEF_VALUE_TYPE_INT8, -1) < 0);
        assert(_idmef_value_cast(value, IDMEF_VALUE_TYPE_INT16, -1) < 0);
        assert(_idmef_value_cast(value, IDMEF_VALUE_TYPE_UINT8, -1) < 0);
        assert(_idmef_value_cast(value, IDMEF_VALUE_TYPE_UINT16, -1) < 0);
        assert(_idmef_value_cast(value, IDMEF_VALUE_TYPE_UINT32, -1) < 0);
        assert(_idmef_value_cast(value, IDMEF_VALUE_TYPE_UINT64, -1) < 0);

        assert(_idmef_value_cast(value, IDMEF_VALUE_TYPE_INT64, -1) == 0);
        assert(idmef_value_get_type(value) == IDMEF_VALUE_TYPE_INT64);
        assert(idmef_value_get_int64(value) == INT32_MIN);
        idmef_value_destroy(value);

        assert(idmef_value_new_int32(&value, INT32_MIN) == 0);
        assert(_idmef_value_cast(value, IDMEF_VALUE_TYPE_FLOAT, -1) == 0);
        assert(idmef_value_get_type(value) == IDMEF_VALUE_TYPE_FLOAT);
        assert(idmef_value_get_float(value) == INT32_MIN);
        idmef_value_destroy(value);

        assert(idmef_value_new_int32(&value, INT32_MIN) == 0);
        assert(_idmef_value_cast(value, IDMEF_VALUE_TYPE_DOUBLE, -1) == 0);
        assert(idmef_value_get_type(value) == IDMEF_VALUE_TYPE_DOUBLE);
        assert(idmef_value_get_double(value) == INT32_MIN);
        idmef_value_destroy(value);

        assert(idmef_value_new_int32(&value, INT32_MIN) == 0);
        assert(_idmef_value_cast(value, IDMEF_VALUE_TYPE_DATA, -1) == 0);
        assert(idmef_value_get_type(value) == IDMEF_VALUE_TYPE_DATA);
        assert((int) idmef_data_get_uint32(idmef_value_get_data(value)) == INT32_MIN);
        idmef_value_destroy(value);

        assert(idmef_value_new_int32(&value, INT32_MAX) == 0);
        assert(_idmef_value_cast(value, IDMEF_VALUE_TYPE_TIME, -1) == 0);
        assert(idmef_value_get_type(value) == IDMEF_VALUE_TYPE_TIME);
        assert(idmef_time_get_sec(idmef_value_get_time(value)) == INT32_MAX);
        idmef_value_destroy(value);
}


static void cast_string(void)
{
        idmef_data_t *data;
        idmef_value_t *value;
        prelude_string_t *str;

        assert(prelude_string_new_ref(&str, "abcdefgh") == 0);
        assert(idmef_value_new_string(&value, str) == 0);
        idmef_value_dont_have_own_data(value);

        assert(_idmef_value_cast(value, IDMEF_VALUE_TYPE_INT8, -1) < 0);
//        assert(_idmef_value_cast(value, IDMEF_VALUE_TYPE_UINT8, -1) < 0);
        assert(_idmef_value_cast(value, IDMEF_VALUE_TYPE_INT16, -1) < 0);
        assert(_idmef_value_cast(value, IDMEF_VALUE_TYPE_UINT16, -1) < 0);
        assert(_idmef_value_cast(value, IDMEF_VALUE_TYPE_INT32, -1) < 0);
        assert(_idmef_value_cast(value, IDMEF_VALUE_TYPE_UINT32, -1) < 0);
        assert(_idmef_value_cast(value, IDMEF_VALUE_TYPE_INT64, -1) < 0);
        assert(_idmef_value_cast(value, IDMEF_VALUE_TYPE_UINT64, -1) < 0);
        assert(_idmef_value_cast(value, IDMEF_VALUE_TYPE_FLOAT, -1) < 0);
        assert(_idmef_value_cast(value, IDMEF_VALUE_TYPE_DOUBLE, -1) < 0);
        assert(_idmef_value_cast(value, IDMEF_VALUE_TYPE_TIME, -1) < 0);
        assert(_idmef_value_cast(value, IDMEF_VALUE_TYPE_ENUM, -1) < 0);
        assert(_idmef_value_cast(value, IDMEF_VALUE_TYPE_LIST, -1) < 0);
        assert(_idmef_value_cast(value, IDMEF_VALUE_TYPE_CLASS, -1) < 0);

        assert(_idmef_value_cast(value, IDMEF_VALUE_TYPE_DATA, -1) == 0);
        assert(idmef_value_get_type(value) == IDMEF_VALUE_TYPE_DATA);
        assert(data = idmef_value_get_data(value));
        assert(idmef_data_get_len(data) == (prelude_string_get_len(str) + 1));
        assert(memcmp(prelude_string_get_string(str), idmef_data_get_data(data), idmef_data_get_len(data)) == 0);
        prelude_string_destroy(str);

        cast_data(value);
        idmef_value_destroy(value);

        assert(prelude_string_new_ref(&str, "2008-01-01 20:42:31") == 0);
        assert(idmef_value_new_string(&value, str) == 0);
        assert(_idmef_value_cast(value, IDMEF_VALUE_TYPE_TIME, -1) == 0);
        idmef_value_destroy(value);
}


int main(void)
{
        cast_int8();
        cast_int16();
        cast_int32();
        cast_string();
        exit(0);
}
