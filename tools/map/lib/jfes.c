/**
    \file       jfes.c
    \author     Eremin Dmitry (http://github.com/NeonMercury)
    \date       October, 2015
    \brief      Json For Embedded Systems library source code.
*/

#include "jfes.h"

/** Needed for the buffer in jfes_(int/double)_to_string(_r). */
#define JFES_MAX_DIGITS                 64

/** Needed for the default precision in jfes_double_to_string(_r). */
#define JFES_DOUBLE_PRECISION           0.000000001

/** Needed for the boolean-related functions */
#define JFES_TRUE_VALUE                 "true"
#define JFES_FALSE_VALUE                "false"

/** Needed for the jfes_is_null function */
#define JFES_NULL_VALUE                 "null"

/** Integer types */
typedef enum jfes_integer_type {
    jfes_not_integer                = 0x00,     /**< String can't be interpreted as integer. */
    jfes_octal_integer              = 0x08,     /**< Octal integer (starts from zero: `023` = `19`) */
    jfes_decimal_integer            = 0x0A,     /**< Decimal integer */
    jfes_hexadecimal_integer        = 0x10,     /**< Hexadecimal integer (mask: `0x...` or `0X...`) */
    
} jfes_integer_type_t;

/** Stream helper. */
typedef struct jfes_stringstream {
    char                    *data;              /**< String data. */
    jfes_size_t             max_size;           /**< Maximal data size. */
    jfes_size_t             current_index;      /**< Current index to the end of the data. */
} jfes_stringstream_t;

/**
    Memory comparing function.

    \param[in]      p1                  Pointer to the first buffer.
    \param[in]      p2                  Pointer to the second buffer.
    \param[in]      count               Number of bytes to compare.

    \return         Zero if all bytes are equal.
*/
static int jfes_memcmp(const void *p1, const void *p2, jfes_size_t count) {
    int delta = 0;

    unsigned char *ptr1 = (unsigned char *)p1;
    unsigned char *ptr2 = (unsigned char *)p2;

    while (count-- > 0 && delta == 0) {
        delta = *(ptr1++) - *(ptr2++);
    }

    return delta;
}

/**
    Copies memory. Doesn't support overlapping.

    \param[out]     dst                 Output memory block.
    \param[in]      src                 Input memory block.
    \param[in]      count               Bytes count to copy.

    \return         Pointer to the destination memory.
*/
static void *jfes_memcpy(void *dst, const void *src, jfes_size_t count) {
    unsigned char *destination  = (unsigned char *)dst;
    unsigned char *source       = (unsigned char *)src;
    while (count-- > 0) {
        *(destination++) = *(source++);
    }

    return dst;
}

/**
    Checks configuration object and its members.

    \param[in]      config              Object to be checked.

    \return         Zero if something wrong. Anything otherwise.
*/
static int jfes_check_configuration(const jfes_config_t *config) {
    return config && config->jfes_malloc && config->jfes_free;
}

/** 
    Allocates jfes_string.

    \param[in]      config              JFES configuration.
    \param[out]     str                 String to be allocated.
    \param[in]      size                Size to allocate.

    \return         jfes_success if everything is OK.
*/
static jfes_status_t jfes_allocate_string(const jfes_config_t *config, jfes_string_t *str, jfes_size_t size) {
    if (!jfes_check_configuration(config) || !str || size == 0) {
        return jfes_invalid_arguments;
    }

    jfes_status_t status = jfes_success;

    str->data = (char*)config->jfes_malloc(size);
    if (!str->data) {
        status = jfes_no_memory;
        size = 0;
    }

    str->size = size;

    return status;
}

/**
    Deallocates jfes_string.

    \param[in]      config              JFES configuration.
    \param[out]     str                 String to be deallocated.

    \return         jfes_success if everything is OK.
*/
static jfes_status_t jfes_free_string(const jfes_config_t *config, jfes_string_t *str) {
    if (!jfes_check_configuration(config) || !str) {
        return jfes_invalid_arguments;
    }

    if (str->size > 0) {
        str->size = 0;
        config->jfes_free(str->data);
        str->data = JFES_NULL;
    }

    return jfes_success;
}

/**
    Creates string object.

    \param[in]      config              JFES configuration.
    \param[out]     str                 String to be created.
    \param[in]      string              Initial string value.
    \param[in]      size                Initial string length.

    \return         jfes_success if everything is OK.
*/
static jfes_status_t jfes_create_string(const jfes_config_t *config, jfes_string_t *str, const char *string, jfes_size_t size) {
    if (!jfes_check_configuration(config) || !str || !string || size == 0) {
        return jfes_invalid_arguments;
    }

    const jfes_status_t status = jfes_allocate_string(config, str, size + 1);
    if (jfes_status_is_good(status)) {
        jfes_memcpy(str->data, string, size);
        str->data[size] = '\0';
    }

    return status;
}

/**
    Finds length of the null-terminated string.

    \param[in]      data                Null-terminated string.

    \return         Length if the string is null-terminated.
*/
static jfes_size_t jfes_strlen(const char *data) {
    if (!data) {
        return 0;
    }

    const char *p = data;
    while (*p++);

    return (jfes_size_t)(p - data) - 1;
}

/**
    Analyzes input string on the subject of whether it's equal to "null".

    \param[in]      data                Input string.
    \param[in]      length              Optional. String length. You can pass zero
                                        if `data` is a null-terminated string.

    \return         Zero if input string is not equal to "null". Otherwise anything.
*/
static int jfes_is_null(const char *data, jfes_size_t length) {
    if (!data) {
        return 0;
    }

    if (length == 0) {
        length = jfes_strlen(data);
    }

    return length == jfes_strlen(JFES_NULL_VALUE) && jfes_memcmp(data, JFES_NULL_VALUE, length) == 0;
}

/**
    Analyzes input string on the subject of whether it is boolean.

    \param[in]      data                Input string.
    \param[in]      length              Optional. String length. You can pass zero
                                        if `data` is a null-terminated string.

    \return         Zero if input string is not boolean. Otherwise anything.
*/
static int jfes_is_boolean(const char *data, jfes_size_t length) {
    if (!data) {
        return 0;
    }

    if (length == 0) {
        length = jfes_strlen(data);
    }

    return length == jfes_strlen(JFES_TRUE_VALUE) && jfes_memcmp(data, JFES_TRUE_VALUE, length) == 0 ||
           length == jfes_strlen(JFES_FALSE_VALUE) && jfes_memcmp(data, JFES_FALSE_VALUE, length) == 0;
}

/**
    Analyzes input string on the subject of whether it is an integer.

    \param[in]      data                Input string.
    \param[in]      length              Optional. String length. You can pass zero
                                        if `data` is a null-terminated string.

    \return         Zero if input string is not an integer. Otherwise anything.
*/
static int jfes_is_integer(const char *data, jfes_size_t length) {
    if (!data) {
        return 0;
    }

    if (length == 0) {
        length = jfes_strlen(data);
    }

    jfes_size_t offset = 0;
    if (data[0] == '-') {
        offset = 1;
    }

    int hexadecimal_case = 0;
    if (length > offset + 2 && data[offset] == '0' && 
       (data[offset + 1] == 'x' || data[offset + 1] == 'X')) {
        hexadecimal_case = 1;
    }

    int is_integer = 1;

    for (jfes_size_t i = offset; i < length; i++) {
        if ((data[i] >= '0' && data[i] <= '9') || (hexadecimal_case &&
            ((data[i] >= 'A' && data[i] <= 'F') || (data[i] >= 'a' && data[i] <= 'f'))) || 
            (i == offset + 1 && ((data[i] == 'x') || (data[i] == 'X')) && length > offset + 2)) {

            if (hexadecimal_case && i == offset && data[i] != '0') {
                is_integer = 0;
                break;
            }
            continue;
        }

        is_integer = 0;
        break;
    }

    return is_integer;
}

/**
    Analyzes input string on the subject of whether it is a double.

    \param[in]      data                Input string.
    \param[in]      length              Optional. String length. You can pass zero
                                        if `data` is a null-terminated string.

    \return         Zero if input string is not a double. Otherwise anything.
*/
static int jfes_is_double(const char *data, jfes_size_t length) {
    if (!data) {
        return 0;
    }

    if (length == 0) {
        length = jfes_strlen(data);
    }

    jfes_size_t offset = 0;
    if (data[0] == '-') {
        offset = 1;
    }

    int dot_already_been = 0;
    int exp_already_been = 0;

    for (jfes_size_t i = offset; i < length; i++) {
        if (data[i] < '0' || data[i] > '9') {
            if (data[i] == '.' && !dot_already_been) {
                dot_already_been = 1;
                continue;
            }
            else if ((data[i] == 'e' || data[i] == 'E') && i + 2 < length &&
                (data[i + 1] == '+' || data[i + 1] == '-') && !exp_already_been) {
                exp_already_been = 1;
                i++;
                continue;
            }

            return 0;
        }
    }

    return 1;
}

/**
    Analyzes string and returns its boolean value.

    \param[in]      data                String for analysis.
    \param[in]      length              Optional. String length. You can pass zero
                                        if `data` is a null-terminated string;

    \return         1 if data == 'true'. Otherwise 0.
*/
static int jfes_string_to_boolean(const char *data, jfes_size_t length) {
    if (!data) {
        return 0;
    }

    if (length == 0) {
        length = jfes_strlen(data);
    }

    if (length == jfes_strlen(JFES_TRUE_VALUE) && jfes_memcmp(data, JFES_TRUE_VALUE, length) == 0) {
        return 1;
    }

    return 0;
}

/** 
    Analyzes string and returns the integer type.

    \param[in]      data                String for analysis.
    \param[in]      length              Optional. String length. You can pass zero
                                        if `data` is a null-terminated string.

    \return         Integer type.
*/
jfes_integer_type_t jfes_get_integer_type(const char *data, jfes_size_t length) {
    if (!data) {
        return jfes_not_integer;
    }

    if (length == 0) {
        length = jfes_strlen(data);
    }

    if (!jfes_is_integer(data, length)) {
        return jfes_not_integer;
    }

    jfes_size_t offset = 0;
    if (data[0] == '-') {
        offset = 1;
    }

    if (length > offset + 2) {
        if (data[offset] == '0') {
            if (data[offset + 1] == 'x' || data[offset + 1] == 'X') {
                return jfes_hexadecimal_integer;
            }

            return jfes_octal_integer;
        }
    }

    return jfes_decimal_integer;
}

/**
    Analyses string and returns its integer value.

    \param[in]      data                String for analysis. Can be decimal, octal or hexadecimal.
    \param[in]      length              Optional. String length. You can pass zero
                                        if `data` is a null-terminated string.

    \return         Integer representation of the input data.

    \warning        This function doesn't check if the string is a correct integer, it's just ignoring
                    all incorrect characters. If you aren't sure of the correctness of the input arguments,
                    check them by `jfes_is_integer()` function.
*/
static int jfes_string_to_integer(const char *data, jfes_size_t length) {
    if (!data) {
        return 0;
    }

    if (length == 0) {
        length = jfes_strlen(data);
    }

    int result = 0;
    int sign = 1;

    jfes_integer_type_t base = jfes_get_integer_type(data, length);
    if (base == jfes_not_integer) {
        return 0;
    }

    jfes_size_t offset = 0;

    if (data[0] == '-') {
        sign = -1;
        offset = 1;
    }

    for (jfes_size_t i = offset; i < length; i++) {
        char c = data[i];
        if ((c >= '0' && c <= '9') || (base == jfes_hexadecimal_integer && 
            ((c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F')))) {
            int value = 0;
            if (c >= '0' && c <= '9') {
                value = c - '0';
            }
            else if (c >= 'A' && c <= 'F') {
                value = c - 'A' + 10;
            }
            else if (c >= 'a' && c <= 'f') {
                value = c - 'a' + 10;
            }
            result = result * (int)base + value;
        }
    }

    return result * sign;
}

/**
    Analyses string and returns its double value.

    \param[in]      data                String for analysis.
    \param[in]      length              Optional. String length. You can pass zero
                                        if `data` is a null-terminated string.

    \return         Double representation of the input data.

    \warning        This function doen't check that string is correct floating point value, it's just ignoring
                    all incorrect characters. If you aren't sure of the correctness of the input arguments,
                    check them by `jfes_is_double()` function.
*/
static double jfes_string_to_double(const char *data, jfes_size_t length) {
    if (!data) {
        return 0.0;
    }

    double result = 0.0;
    double sign = 1.0;

    int after_dot = 0;
    int exp = 0;

    int direction = 0;

    jfes_size_t index = 0;

    if (length == 0) {
        length = jfes_strlen(data);
    }

    if (data[0] == '-') {
        sign = -1.0;
        index = 1;
    }

    for (; index < length; index++) {
        char c = data[index];
        if (c >= '0' && c <= '9') {
            result = result * 10.0 + (c - '0');
            if (after_dot) {
                exp--;
            }
        } 
        else if (c == '.') {
            after_dot = 1;
            continue;
        }
        else if (index + 2 < length && (c == 'e' || c == 'E')) {
            index++;
            if (data[index] == '+') {
                direction = 1;
                index++;
            }
            else if (data[index] == '-') {
                direction = -1;
                index++;
            }
            break;
        }
    }

    if (index < length) {
        if (jfes_is_integer(data + index, length - index)) {
            int new_exp = jfes_string_to_integer(data + index, length - index);
            exp += direction * new_exp;
        }
    }

    while (exp < 0) {
        result /= 10.0;
        exp++;
    }
    while (exp > 0) {
        result *= 10.0;
        exp--;
    }

    return sign * result;
}

/**
    Returns boolean value as string.

    \param[in]      value               Value to stringify.

    \return         String representation of the given value.
*/
char *jfes_boolean_to_string(int value) {
    return value ? JFES_TRUE_VALUE : JFES_FALSE_VALUE;
}

/**
    Thread safe version of the `jfes_integer_to_string`.

    \param[in]      value               Value to stringify.
    \param[out]     output              Output buffer to store result.
    \param[in]      output_size         Maximal size of output buffer.

    \return         String representation of the given value.

    \warning        output_size must be greater than 1, because the output will be
    null-terminated (if `output_size == 1`, then this single byte will be '\0').
*/
char *jfes_integer_to_string_r(int value, char *output, jfes_size_t output_size) {
    if (!output || output_size <= 1) {
        return JFES_NULL;
    }

    char *p = &output[0] + output_size;
    *--p = '\0';

    jfes_size_t number_length = 0;

    int sign = 1;
    if (value < 0) {
        sign = -1;
    }

    do {
        *--p = '0' + (char)(sign * (value % 10));
        value /= 10;
        number_length++;
    } while (value != 0 && p > output);

    if (sign < 0 && p > output) {
        *--p = '-';
        number_length++;
    }

    if (p == output && value != 0) {
        return JFES_NULL;
    }

    if (number_length + 1 < output_size) {
        for (jfes_size_t i = 0; i < number_length + 1; i++) {
            output[i] = *(p + i);
        }
    }

    return output;
}

/**
    Returns integer value as string.

    \param[in]      value               Value to stringify.

    \return         String representation of the given value.
*/
char *jfes_integer_to_string(int value) {
    static char buf[JFES_MAX_DIGITS + 1];
    return jfes_integer_to_string_r(value, &buf[0], JFES_MAX_DIGITS);
}

/**
    Thread safe version of the `jfes_double_to_string`.

    \param[in]      value               Value to stringify.
    \param[out]     output              Output buffer to store result.
    \param[in]      output_size         Maximal size of output buffer.
    \param[in]      precision_eps       Double precision (like 0.00001).

    \return         String representation of the given value.

    \warning        output_size must be greater than 1, because the output will be
    null-terminated (if `output_size == 1`, then this single byte will be '\0').
*/
char *jfes_double_to_string_r(double value, char *output, jfes_size_t output_size, double precision_eps) {
    if (!output || output_size <= 1) {
        return JFES_NULL;
    }

    int int_value = (int)value;
    double fractional_value = value - int_value;
    if (fractional_value < 0) {
        fractional_value = -fractional_value;
    }

    double precision = precision_eps;
    while (precision < 1.0) {
        fractional_value *= 10;
        if (fractional_value - (int)fractional_value < precision_eps) {
            break;
        }

        precision *= 10.0;
    }

    int fractional_int = (int)fractional_value;

    char *int_value_s = jfes_integer_to_string_r(int_value, &output[0], output_size);
    if (!int_value_s) {
        return JFES_NULL;
    }
    jfes_size_t value_length = jfes_strlen(int_value_s);

    output[value_length] = '.';

    char *fractional_value_s = jfes_integer_to_string_r(fractional_int, 
        &output[value_length + 1], output_size - value_length - 1);
    if (!fractional_value_s) {
        return JFES_NULL;
    }

    return &output[0];
}

/**
    Returns double value as string.

    \param[in]      value               Value to stringify.

    \return         String representation of the given value.
*/
char *jfes_double_to_string(double value) {
    static char buf[JFES_MAX_DIGITS];

    static double precision_eps = JFES_DOUBLE_PRECISION;
    return jfes_double_to_string_r(value, &buf[0], JFES_MAX_DIGITS, precision_eps);
}

/**
    Initializes jfes_stringstream object.

    \param[out]     stream              Object to initialize.
    \param[in]      data                Pointer to the memory.
    \param[in]      max_size            Size of the allocated memory.

    \return         jfes_success if everything is OK.
*/
jfes_status_t jfes_initialize_stringstream(jfes_stringstream_t *stream, char *data, jfes_size_t max_size) {
    if (!stream || !data || max_size == 0) {
        return jfes_invalid_arguments;
    }

    stream->data = data;
    stream->max_size = max_size;
    stream->current_index = 0;

    return jfes_success;
}

/**
    Adds given data to the given stream.

    \param[out]     stream              Stringstream object.
    \param[in]      data                Data to add.
    \param[in]      data_length         Optional. Child key length. You can pass 0
                                        if the data string is zero-terminated.
    
    \return         jfes_success if everything is OK.
*/
jfes_status_t jfes_add_to_stringstream(jfes_stringstream_t *stream, char *data, jfes_size_t data_length) {
    if (!stream || !data) {
        return jfes_invalid_arguments;
    }

    if (stream->current_index >= stream->max_size) {
        return jfes_no_memory;
    }

    if (data_length == 0) {
        data_length = jfes_strlen(data);
    }
    jfes_status_t status = jfes_success;

    jfes_size_t size_to_add = data_length;
    
    if (stream->current_index + size_to_add > stream->max_size) {
        size_to_add = stream->max_size - stream->current_index;
        status = jfes_no_memory;
    }

    jfes_memcpy(stream->data + stream->current_index, data, size_to_add);

    stream->current_index += size_to_add;
    return status;
}

/**
    Allocates a fresh unused token from the token pool.

    \param[in, out] parser              Pointer to the jfes_parser_t object.
    \param[in, out] tokens              Tokens array.
    \param[in]      max_tokens          Maximal tokens count.

    \return         Pointer to the allocated token from the token pool.
*/
static jfes_token_t *jfes_allocate_token(jfes_parser_t *parser, jfes_token_t *tokens, jfes_size_t max_tokens) {
    if (!parser || !tokens || max_tokens == 0 || parser->next_token >= max_tokens) {
        return JFES_NULL;
    }
    
    jfes_token_t *token = &tokens[parser->next_token++];
    token->start = token->end = -1;
    token->size = 0;

    return token;
}

/** 
    Analyzes the source string and returns most likely type.

    \param[in]      data                Source string bytes.
    \param[in]      length              Source string length.

    \return         Most likely token type or `jfes_type_undefined`.
*/
static jfes_token_type_t jfes_get_token_type(const char *data, jfes_size_t length) {
    if (!data || length == 0) {
        return jfes_type_undefined;
    }

    if (jfes_is_null(data, length)) {
        return jfes_type_null;
    }
    else if (jfes_is_boolean(data, length)) {
        return jfes_type_boolean;
    }
    else if (jfes_is_integer(data, length)) {
        return jfes_type_integer;
    }
    else if (jfes_is_double(data, length)) {
        return jfes_type_double;
    }

    return jfes_type_undefined;
}

/**
    Fills token type and boundaries.

    \param[in, out] token               Pointer to the token to fill.
    \param[in]      type                Token type.
    \param[in]      start               Token boundary start.
    \param[in]      end                 Token boundary end.
*/
static void jfes_fill_token(jfes_token_t *token, jfes_token_type_t type, int start, int end) {
    if (token) {
        token->type = type;
        token->start = start;
        token->end = end;
        token->size = 0;
    }
}

/**
    Fills next available token with JSON primitive.

    \param[in, out] parser              Pointer to the jfes_parser_t object.
    \param[in]      json                JSON data string.
    \param[in]      length              JSON data length.
    \param[out]     tokens              Tokens array to fill.
    \param[in]      max_tokens_count    Maximal count of tokens in tokens array.

    \return         jfes_success if everything is OK.
*/
static jfes_status_t jfes_parse_primitive(jfes_parser_t *parser, const char *json, jfes_size_t length,
        jfes_token_t *tokens, jfes_size_t max_tokens_count) {
    if (!parser || !json || length == 0 || !tokens || max_tokens_count == 0) {
        return jfes_invalid_arguments;
    }

    int found = 0;

    jfes_size_t start = parser->pos;
    while (length && json[parser->pos] != '\0') {
        char c = json[parser->pos];
        if (c == '\t' || c == '\n' || c == '\r' || c == ' ' ||
            c == ',' || c == ']' || c == '}'
#ifndef JFES_STRICT
            || c == ':'
#endif
            ) {
            found = 1;
            break;
        }

        parser->pos++;
    }

#ifdef JFES_STRICT
    if (!found) {
        parser->pos = start;
        return jfes_error_part;
    }
#endif

    jfes_token_t *token = jfes_allocate_token(parser, tokens, max_tokens_count);
    if (!token) {
        parser->pos = start;
        return jfes_no_memory;
    }

    jfes_size_t token_length = parser->pos - start;
    jfes_token_type_t type = jfes_get_token_type(json + start, token_length);

    jfes_fill_token(token, type, start, parser->pos);
    parser->pos--;

    return jfes_success;
}

/**
    Fills next available token with JSON string.

    \param[in, out] parser              Pointer to the jfes_parser_t object.
    \param[in]      json                JSON data string.
    \param[in]      length              JSON data length.
    \param[out]     tokens              Tokens array to fill.
    \param[in]      max_tokens_count    Maximal count of tokens in tokens array.

    \return         jfes_success if everything is OK.
*/
static jfes_status_t jfes_parse_string(jfes_parser_t *parser, const char *json, jfes_size_t length,
        jfes_token_t *tokens, jfes_size_t max_tokens_count) {
    if (!parser || !json || length == 0 || !tokens || max_tokens_count == 0) {
        return jfes_invalid_arguments;
    }

    jfes_size_t start = parser->pos++;
    while (parser->pos < length && json[parser->pos] != '\0') {
        char c = json[parser->pos];
        if (c == '\"') {
            jfes_token_t *token = jfes_allocate_token(parser, tokens, max_tokens_count);
            if (!token) {
                parser->pos = start;
                return jfes_no_memory;
            }

            jfes_fill_token(token, jfes_type_string, start + 1, parser->pos);
            return jfes_success;
        }
        else if (c == '\\' && parser->pos + 1 < length) {
            parser->pos++;
            switch (json[parser->pos]) {
            case '\"': case '/': case '\\': case 'b': case 'f':
            case 'r': case 'n': case 't':
                break;

            case 'u':
                parser->pos++;
                for (jfes_size_t i = 0;  i < 4 && parser->pos < length && json[parser->pos] != '\0'; i++, parser->pos++) {
                    char symbol = json[parser->pos];
                    if ((symbol < (int)'0' || symbol > (int)'9') &&
                        (symbol < (int)'A' || symbol > (int)'F') &&
                        (symbol < (int)'a' || symbol > (int)'f')) {
                        parser->pos = start;
                        return jfes_invalid_input;
                    }
                }
                parser->pos--;
                break;
            default:
                parser->pos = start;
                return jfes_invalid_input;
            }
        }
        parser->pos++;
    }

    parser->pos = start;
    return jfes_error_part;
}

int jfes_status_is_good(jfes_status_t status) {
    return status == jfes_success;
}

int jfes_status_is_bad(jfes_status_t status) {
    return !jfes_status_is_good(status);
}

jfes_status_t jfes_init_parser(jfes_parser_t *parser, const jfes_config_t *config) {
    if (!parser || !config) {
        return jfes_invalid_arguments;
    }

    parser->config = config;

    return jfes_reset_parser(parser);
}

jfes_status_t jfes_reset_parser(jfes_parser_t *parser) {
    if (!parser) {
        return jfes_invalid_arguments;
    }

    parser->pos = 0;
    parser->next_token = 0;
    parser->superior_token = -1;

    return jfes_success;
}

jfes_status_t jfes_parse_tokens(jfes_parser_t *parser, const char *json,
        jfes_size_t length, jfes_token_t *tokens, jfes_size_t *max_tokens_count) {
    if (!parser || !json || length == 0 || !tokens || !max_tokens_count || *max_tokens_count == 0) {
        return jfes_invalid_arguments;
    }

    jfes_reset_parser(parser);

    jfes_token_t *token = JFES_NULL;

    jfes_size_t count = parser->next_token;
    while (parser->pos < length && json[parser->pos] != '\0') {
        char c = json[parser->pos];
        switch (c) {
        case '{': case '[':
            {
                count++;
                token = jfes_allocate_token(parser, tokens, *max_tokens_count);
                if (!token) {
                    return jfes_no_memory;
                }
                if (parser->superior_token != -1) {
                    tokens[parser->superior_token].size++;
                }

                token->type = (c == '{' ? jfes_type_object : jfes_type_array);
                token->start = parser->pos;
                parser->superior_token = parser->next_token - 1;
            }
            break;

        case '}': case ']':
            {
                int i = 0;
                for (i = (int)parser->next_token - 1; i >= 0; i--) {
                    token = &tokens[i];
                    if (token->start != -1 && token->end == -1) {
                        parser->superior_token = -1;
                        token->end = parser->pos + 1;
                        break;
                    }
                }

                if (i == -1) {
                    return jfes_invalid_input;
                }

                while (i >= 0) {
                    token = &tokens[i];
                    if (token->start != -1 && token->end == -1) {
                        parser->superior_token = i;
                        break;
                    }
                    i--;
                }
            }
            break;

        case '\"':
            {
                jfes_status_t status = jfes_parse_string(parser, json, length, tokens, *max_tokens_count);
                if (jfes_status_is_bad(status)) {
                    return status;
                }

                count++;

                if (parser->superior_token != -1 && tokens != JFES_NULL) {
                    tokens[parser->superior_token].size++;
                }
            }
            break;

        case '\t': case '\r': case '\n': case ' ':
            break;

        case ':':
            parser->superior_token = parser->next_token - 1;
            break;

        case ',':
            {
                if (parser->superior_token != -1 &&
                    tokens[parser->superior_token].type != jfes_type_array &&
                    tokens[parser->superior_token].type != jfes_type_object) {
                    for (int i = parser->next_token - 1; i >= 0; i--) {
                        if (tokens[i].type == jfes_type_array || tokens[i].type == jfes_type_object) {
                            if (tokens[i].start != -1 && tokens[i].end == -1) {
                                parser->superior_token = i;
                                break;
                            }
                        }
                    }
                }
            }
            break;

#ifdef JFES_STRICT
        case '-': case '0': case '1': case '2': case '3': case '4':
        case '5': case '6': case '7': case '8': case '9':
        case 't': case 'f': case 'n':
            if (parser->superior_token != -1) {
                jfes_token_t *token = &tokens[parser->superior_token];
                if (token->type == jfes_object || (token->type == jfes_string && token->size != 0)) {
                    return jfes_invalid_input;
                }
            }
#else
        default:
#endif
            {
                jfes_status_t status = jfes_parse_primitive(parser, json, length, tokens, *max_tokens_count);
                if (jfes_status_is_bad(status)) {
                    return status;
                }

                count++;
                if (parser->superior_token != -1) {
                    tokens[parser->superior_token].size++;
                }
            }
            break;
#ifdef JFES_STRICT
        default:
            return jfes_invalid_input;
#endif
        }
        parser->pos++;
    }

    for (int i = (int)parser->next_token - 1; i >= 0; i--) {
        if (tokens[i].start != -1 && tokens[i].end == -1) {
            return jfes_error_part;
        }
    }

    *max_tokens_count = count;
    return jfes_success;
}

/**
    Creates jfes value node from the tokens sequence.

    \param[in]      tokens_data         Pointer to the jfes_tokens_data_t object.
    \param[out]     value               Pointer to the value to create node.

    \return         jfes_success if everything is OK.
*/
jfes_status_t jfes_create_node(jfes_tokens_data_t *tokens_data, jfes_value_t *value) {
    if (!tokens_data || !value) {
        return jfes_invalid_arguments;
    }

    if (tokens_data->current_token >= tokens_data->tokens_count) {
        return jfes_invalid_arguments;
    }

    jfes_malloc_t jfes_malloc = tokens_data->config->jfes_malloc;
    jfes_free_t   jfes_free   = tokens_data->config->jfes_free;

    jfes_token_t *token = &tokens_data->tokens[tokens_data->current_token];
    tokens_data->current_token++;
    
    value->type = token->type;

    switch (token->type) {
    case jfes_type_null:
        break;

    case jfes_type_boolean:
        value->data.bool_val = jfes_string_to_boolean(tokens_data->json_data + token->start, 
            token->end - token->start);
        break;

    case jfes_type_integer:
        value->data.int_val = jfes_string_to_integer(tokens_data->json_data + token->start,
            token->end - token->start);
        break;

    case jfes_type_double:
        value->data.double_val = jfes_string_to_double(tokens_data->json_data + token->start,
            token->end - token->start);
        break;

    case jfes_type_string:
        jfes_create_string(tokens_data->config, &value->data.string_val, 
            tokens_data->json_data + token->start, token->end - token->start);
        break;

    case jfes_type_array:
        value->data.array_val = (jfes_array_t*)jfes_malloc(sizeof(jfes_array_t));
        if (!value->data.array_val) {
            return jfes_no_memory;
        }

        if (token->size > 0) {
            value->data.array_val->count = token->size;
            value->data.array_val->items = (jfes_value_t**)jfes_malloc(token->size * sizeof(jfes_value_t*));
            if (!value->data.array_val->items) {
                jfes_free(value->data.array_val);
                return jfes_no_memory;
            }

            for (jfes_size_t i = 0; i < token->size; i++) {
                jfes_value_t *item = (jfes_value_t*)jfes_malloc(sizeof(jfes_value_t));
                if (!item) {
                    jfes_free(value->data.array_val->items);
                    jfes_free(value->data.array_val);
                    return jfes_no_memory;
                }
                value->data.array_val->items[i] = item;

                jfes_status_t status = jfes_create_node(tokens_data, item);
                if (jfes_status_is_bad(status)) {
                    value->data.array_val->count = i + 1;

                    jfes_free_value(tokens_data->config, value);
                    return status;
                }
            }
        }
        break;

    case jfes_type_object:
        value->data.object_val = (jfes_object_t*)jfes_malloc(sizeof(jfes_object_t));
        if (!value->data.object_val) {
            return jfes_no_memory;
        }

        if (token->size > 0) {
            value->data.object_val->count = token->size;
            value->data.object_val->items = (jfes_object_map_t**)jfes_malloc(token->size * sizeof(jfes_object_map_t*));
            if (!value->data.object_val->items) {
                jfes_free(value->data.object_val);
                return jfes_no_memory;
            }

            for (jfes_size_t i = 0; i < token->size; i++) {
                jfes_object_map_t *item = (jfes_object_map_t*)jfes_malloc(sizeof(jfes_object_map_t));
                if (!item) {
                    jfes_free(value->data.object_val->items);
                    jfes_free(value->data.object_val);
                    return jfes_no_memory;
                }
                value->data.object_val->items[i] = item;

                jfes_token_t *key_token = &tokens_data->tokens[tokens_data->current_token++];
                
                jfes_size_t key_length = key_token->end - key_token->start;

                jfes_create_string(tokens_data->config, &item->key, 
                    tokens_data->json_data + key_token->start, key_length);

                item->value = (jfes_value_t*)jfes_malloc(sizeof(jfes_value_t));

                jfes_status_t status = jfes_create_node(tokens_data, item->value);
                if (jfes_status_is_bad(status)) {
                    value->data.object_val->count = i + 1;

                    jfes_free_value(tokens_data->config, value);
                    return status;
                }
            }
        }
        break;

    default:
        return jfes_unknown_type;
    }
    return jfes_success;
}

jfes_status_t jfes_parse_to_value(const jfes_config_t *config, const char *json,
        jfes_size_t length, jfes_value_t *value) {
    if (!jfes_check_configuration(config) || !json || length == 0 || !value) {
        return jfes_invalid_arguments;
    }

    jfes_parser_t parser;
    jfes_status_t status = jfes_init_parser(&parser, config);
    if (jfes_status_is_bad(status)) {
        return status;
    }

    jfes_size_t tokens_count = 1024;
    jfes_token_t *tokens = JFES_NULL;

    status = jfes_no_memory;
    while (status == jfes_no_memory && tokens_count <= JFES_MAX_TOKENS_COUNT) {
        jfes_reset_parser(&parser);

        tokens = (jfes_token_t*)parser.config->jfes_malloc(tokens_count * sizeof(jfes_token_t));
        if (!tokens) {
            return jfes_no_memory;
        }

        jfes_size_t current_tokens_count = tokens_count;
        status = jfes_parse_tokens(&parser, json, length, tokens, &current_tokens_count);
        if (jfes_status_is_good(status)) {
            tokens_count = current_tokens_count;
            break;
        }

        tokens_count *= 2;
        parser.config->jfes_free(tokens);
    }
    
    if (jfes_status_is_bad(status)) {
        return status;
    }

    jfes_tokens_data_t tokens_data = { 0 };
    tokens_data.config = config;

    tokens_data.json_data = json;
    tokens_data.json_data_length = length;

    tokens_data.tokens = tokens;
    tokens_data.tokens_count = tokens_count;
    tokens_data.current_token = 0;

    status = jfes_create_node(&tokens_data, value); /* what if no success here? */

    parser.config->jfes_free(tokens);
    return jfes_success;
}

jfes_status_t jfes_free_value(const jfes_config_t *config, jfes_value_t *value) {
    if (!jfes_check_configuration(config) || !value) {
        return jfes_invalid_arguments;
    }

    if (value->type == jfes_type_array) {
        if (value->data.array_val && value->data.array_val->count > 0) {
            for (jfes_size_t i = 0; i < value->data.array_val->count; i++) {
                jfes_value_t *item = value->data.array_val->items[i];
                jfes_free_value(config, item);
                config->jfes_free(item);
            }

            config->jfes_free(value->data.array_val->items);
        }

        config->jfes_free(value->data.array_val);
    }
    else if (value->type == jfes_type_object) {
        if (value->data.object_val && value->data.object_val->count > 0) {
            for (jfes_size_t i = 0; i < value->data.object_val->count; i++) {
                jfes_object_map_t *object_map = value->data.object_val->items[i];

                config->jfes_free(object_map->key.data);

                jfes_free_value(config, object_map->value);
                config->jfes_free(object_map->value);

                config->jfes_free(object_map);
            }

            config->jfes_free(value->data.object_val->items);
        }

        config->jfes_free(value->data.object_val);
    }
    else if (value->type == jfes_type_string) {
        if (value->data.string_val.size > 0) {
            config->jfes_free(value->data.string_val.data);
        }
    }

    return jfes_success;
}

jfes_value_t *jfes_create_null_value(const jfes_config_t *config) {
    if (!config) {
        return JFES_NULL;
    }

    jfes_value_t *result = (jfes_value_t*)config->jfes_malloc(sizeof(jfes_value_t));
    if (!result) {
        return JFES_NULL;
    }
    result->type = jfes_type_null;

    return result;
}

jfes_value_t *jfes_create_boolean_value(const jfes_config_t *config, int value) {
    if (!config) {
        return JFES_NULL;
    }

    jfes_value_t *result = (jfes_value_t*)config->jfes_malloc(sizeof(jfes_value_t));
    if (!result) {
        return JFES_NULL;
    }
    result->type = jfes_type_boolean;
    result->data.bool_val = value;

    return result;
}

jfes_value_t *jfes_create_integer_value(const jfes_config_t *config, int value) {
    if (!config) {
        return JFES_NULL;
    }

    jfes_value_t *result = (jfes_value_t*)config->jfes_malloc(sizeof(jfes_value_t));
    if (!result) {
        return JFES_NULL;
    }
    result->type = jfes_type_integer;
    result->data.int_val = value;

    return result;
}

jfes_value_t *jfes_create_double_value(const jfes_config_t *config, double value) {
    if (!config) {
        return JFES_NULL;
    }

    jfes_value_t *result = (jfes_value_t*)config->jfes_malloc(sizeof(jfes_value_t));
    if (!result) {
        return JFES_NULL;
    }
    result->type = jfes_type_double;
    result->data.double_val = value;

    return result;
}

jfes_value_t *jfes_create_string_value(const jfes_config_t *config, const char *value, jfes_size_t length) {
    if (!jfes_check_configuration(config) || !value) {
        return JFES_NULL;
    }

    if (length == 0) {
        length = jfes_strlen(value);
    }

    jfes_value_t *result = (jfes_value_t*)config->jfes_malloc(sizeof(jfes_value_t));
    if (!result) {
        return JFES_NULL;
    }
    result->type = jfes_type_string;
    
    jfes_status_t status = jfes_create_string(config, &result->data.string_val, value, length);
    if (jfes_status_is_bad(status)) {
        config->jfes_free(result);
        return JFES_NULL;
    }

    return result;
}

jfes_value_t *jfes_create_array_value(const jfes_config_t *config) {
    if (!config) {
        return JFES_NULL;
    }

    jfes_value_t *result = (jfes_value_t*)config->jfes_malloc(sizeof(jfes_value_t));
    if (!result) {
        return JFES_NULL;
    }

    result->type = jfes_type_array;

    result->data.array_val = (jfes_array_t*)config->jfes_malloc(sizeof(jfes_array_t));
    if (!result->data.array_val) {
        config->jfes_free(result);
        return JFES_NULL;
    }
    result->data.array_val->count = 0;
    result->data.array_val->items = JFES_NULL;

    return result;
}

jfes_value_t *jfes_create_object_value(const jfes_config_t *config) {
    if (!config) {
        return JFES_NULL;
    }

    jfes_value_t *result = (jfes_value_t*)config->jfes_malloc(sizeof(jfes_value_t));
    if (!result) {
        return JFES_NULL;
    }
    result->type = jfes_type_object;

    result->data.object_val = (jfes_object_t*)config->jfes_malloc(sizeof(jfes_object_t));
    if (!result->data.object_val) {
        config->jfes_free(result);
        return JFES_NULL;
    }
    result->data.object_val->count = 0;
    result->data.object_val->items = JFES_NULL;

    return result;
}

jfes_value_t *jfes_get_child(const jfes_value_t *value, const char *key, jfes_size_t key_length) {
    jfes_object_map_t *mapped_item = jfes_get_mapped_child(value, key, key_length);
    if (mapped_item) {
        return mapped_item->value;
    }

    return JFES_NULL;
}

jfes_object_map_t *jfes_get_mapped_child(const jfes_value_t *value, const char *key, jfes_size_t key_length) {
    if (!value || !key || value->type != jfes_type_object) {
        return JFES_NULL;
    }

    if (key_length == 0) {
        key_length = jfes_strlen(key);
    }

    for (jfes_size_t i = 0; i < value->data.object_val->count; i++) {
        jfes_object_map_t *item = value->data.object_val->items[i];
        if (item && (item->key.size - 1) == key_length &&
                jfes_memcmp(item->key.data, key, key_length) == 0) {
            return item;
        }
    }

    return JFES_NULL;
}

jfes_status_t jfes_place_to_array(const jfes_config_t *config, const jfes_value_t *value, jfes_value_t *item) {
    if (!jfes_check_configuration(config) || !value || !item || value->type != jfes_type_array) {
        return jfes_invalid_arguments;
    }

    return jfes_place_to_array_at(config, value, item, value->data.array_val->count);
}

jfes_status_t jfes_place_to_array_at(const jfes_config_t *config, const jfes_value_t *value, jfes_value_t *item, jfes_size_t place_at) {
    if (!jfes_check_configuration(config) || !value || !item || value->type != jfes_type_array) {
        return jfes_invalid_arguments;
    }

    if (place_at > value->data.array_val->count) {
        place_at = value->data.array_val->count;
    }

    jfes_value_t **items_array = (jfes_value_t**)config->jfes_malloc((value->data.array_val->count + 1) * sizeof(jfes_value_t*));
    if (!items_array) {
        return jfes_no_memory;
    }

    jfes_size_t offset = 0;
    for (jfes_size_t i = 0; i < value->data.array_val->count; i++) {
        if (i == place_at) {
            offset = 1;
        }
        items_array[i + offset] = value->data.array_val->items[i];
    }

    items_array[place_at] = item;

    value->data.array_val->count++;

    config->jfes_free(value->data.array_val->items);
    value->data.array_val->items = items_array;
    return jfes_success;
}

jfes_status_t jfes_remove_from_array(const jfes_config_t *config, const jfes_value_t *value, jfes_size_t index) {
    if (!jfes_check_configuration(config) || !value || value->type != jfes_type_array) {
        return jfes_invalid_arguments;
    }

    if (index >= value->data.array_val->count) {
        return jfes_not_found;
    }

    jfes_value_t *item = value->data.array_val->items[index];
    jfes_free_value(config, item);
    config->jfes_free(item);

    for (jfes_size_t i = index; i < value->data.array_val->count - 1; i++) {
        value->data.array_val->items[i] = value->data.array_val->items[i + 1];
    }

    value->data.array_val->count--;
    return jfes_success;
}

jfes_status_t jfes_set_object_property(const jfes_config_t *config, const jfes_value_t *value,
        jfes_value_t *item, const char *key, jfes_size_t key_length) {
    if (!jfes_check_configuration(config) || !value || !item || !key || value->type != jfes_type_object) {
        return jfes_invalid_arguments;
    }

    if (key_length == 0) {
        key_length = jfes_strlen(key);
    }

    jfes_object_map_t *object_map = jfes_get_mapped_child(value, key, key_length);
    if (object_map) {
        jfes_free_value(config, object_map->value);
        config->jfes_free(object_map->value);
    }
    else {
        jfes_object_map_t **items_map = (jfes_object_map_t**)config->jfes_malloc((value->data.object_val->count + 1) * sizeof(jfes_object_map_t*));
        if (!items_map) {
            return jfes_no_memory;
        }

        for (jfes_size_t i = 0; i < value->data.object_val->count; i++) {
            items_map[i] = value->data.object_val->items[i];
        }

        items_map[value->data.object_val->count] = (jfes_object_map_t*)config->jfes_malloc(sizeof(jfes_object_map_t));
        if (!items_map[value->data.object_val->count]) {
            config->jfes_free(items_map);
            return jfes_no_memory;
        }
        object_map = items_map[value->data.object_val->count];

        jfes_status_t status = jfes_create_string(config, &object_map->key, key, key_length);
        if (jfes_status_is_bad(status)) {
            config->jfes_free(items_map);
            return status;
        }

        config->jfes_free(value->data.object_val->items);

        value->data.object_val->items = items_map;
        value->data.object_val->count++;
    }

    object_map->value = item;
    return jfes_success;
}

jfes_status_t jfes_remove_object_property(const jfes_config_t *config, const jfes_value_t *value,
        const char *key, jfes_size_t key_length) {
    if (!jfes_check_configuration(config) || !value || value->type != jfes_type_object || !key) {
        return jfes_invalid_arguments;
    }

    if (key_length == 0) {
        key_length = jfes_strlen(key);
    }

    jfes_object_map_t *mapped_item = jfes_get_mapped_child(value, key, key_length);
    if (!mapped_item) {
        return jfes_not_found;
    }

    jfes_free_value(config, mapped_item->value);
    config->jfes_free(mapped_item->value);

    jfes_free_string(config, &mapped_item->key);

    jfes_size_t i = 0;
    for (i = 0; i < value->data.object_val->count; i++) {
        jfes_object_map_t *item = value->data.object_val->items[i];
        if (item == mapped_item) {
            config->jfes_free(item);
            mapped_item = JFES_NULL;
            break;
        }
    }

    /* We found item to remove and set it to JFES_NULL. Next we need to shift other items. */
    if (!mapped_item) {
        for (; i < value->data.object_val->count - 1; i++) {
            value->data.object_val->items[i] = value->data.object_val->items[i + 1];
        }
    }

    value->data.object_val->count--;

    return jfes_success;
}

/**
Dumps JFES value to memory.

\param[in]      value               JFES value to dump.
\param[out]     data                Allocated memory to store.
\param[in, out] max_size            Maximal size of data. Will store data length.
\param[in]      beautiful           Beautiful JSON.
\param[in]      indent              Indent. Works only when beautiful is true.
\param[in]      indent_string       String to indent JSON.

\return         jfes_success if everything is OK.
*/
static jfes_status_t jfes_value_to_stream_helper(const jfes_value_t *value, jfes_stringstream_t *stream,
    int beautiful, jfes_size_t indent, const char *indent_string);

/**
    Dumps JFES array value to memory.

    \param[in]      value               JFES value to dump.
    \param[out]     data                Allocated memory to store.
    \param[in, out] max_size            Maximal size of data. Will store data length.
    \param[in]      beautiful           Beautiful JSON.
    \param[in]      indent              Indent. Works only when beautiful is true.
    \param[in]      indent_string       String to indent JSON.

    \return         jfes_success if everything is OK.
*/
static jfes_status_t jfes_array_value_to_stream_helper(const jfes_value_t *value, jfes_stringstream_t *stream, 
        int beautiful, jfes_size_t indent, const char *indent_string) {
    if (!value || !stream || value->type != jfes_type_array) {
        return jfes_invalid_arguments;
    }

    int with_indent = 1;

    jfes_add_to_stringstream(stream, "[", 0);
    if (beautiful) {
        if (value->data.array_val->count > 0 &&
            (value->data.array_val->items[0]->type == jfes_type_object || value->data.array_val->items[0]->type == jfes_type_array)) {
            jfes_add_to_stringstream(stream, "\n", 0);
        }
        else {
            with_indent = 0;
            jfes_add_to_stringstream(stream, " ", 0);
        }
    }

    for (jfes_size_t i = 0; i < value->data.array_val->count; i++) {
        jfes_value_t *item = value->data.array_val->items[i];
        
        if (beautiful && with_indent) {
            for (jfes_size_t j = 0; j < indent + 1; j++) {
                jfes_add_to_stringstream(stream, (char*)indent_string, 0);
            }
        }

        jfes_status_t status = jfes_value_to_stream_helper(item, stream, beautiful, indent + 1, indent_string);
        if (jfes_status_is_bad(status)) {
            return status;
        }

        if (i < value->data.array_val->count - 1) {
            jfes_add_to_stringstream(stream, ",", 0);
        }

        if (beautiful) {
            if ((i < value->data.array_val->count - 1 && 
                (value->data.array_val->items[i + 1]->type == jfes_type_array ||
                value->data.array_val->items[i + 1]->type == jfes_type_object))
                ||
                (i == value->data.array_val->count - 1 && with_indent)
                ||
                (value->data.array_val->items[i]->type == jfes_type_array ||
                 value->data.array_val->items[i]->type == jfes_type_object)) {
                jfes_add_to_stringstream(stream, "\n", 0);
                with_indent = 1;
            }
            else {
                jfes_add_to_stringstream(stream, " ", 0);
                with_indent = 0;
            }
        }
    }

    if (beautiful && with_indent) {
        for (jfes_size_t i = 0; i < indent; i++) {
            jfes_add_to_stringstream(stream, (char*)indent_string, 0);
        }
    }
    return jfes_add_to_stringstream(stream, "]", 0);
}

/**
    Dumps JFES object value to memory.

    \param[in]      value               JFES value to dump.
    \param[out]     data                Allocated memory to store.
    \param[in, out] max_size            Maximal size of data. Will store data length.
    \param[in]      beautiful           Beautiful JSON.
    \param[in]      indent              Indent. Works only when beautiful is true.
    \param[in]      indent_string       String to indent JSON.

    \return         jfes_success if everything is OK.
*/
static jfes_status_t jfes_object_value_to_stream_helper(const jfes_value_t *value, jfes_stringstream_t *stream,
        int beautiful, jfes_size_t indent, const char *indent_string) {
    if (!value || !stream || value->type != jfes_type_object) {
        return jfes_invalid_arguments;
    }

    jfes_add_to_stringstream(stream, "{", 0);
    if (beautiful) {
        jfes_add_to_stringstream(stream, "\n", 0);
    }

    for (jfes_size_t i = 0; i < value->data.array_val->count; i++) {
        jfes_object_map_t *object_map = value->data.object_val->items[i];

        if (beautiful) {
            for (jfes_size_t j = 0; j < indent + 1; j++) {
                jfes_add_to_stringstream(stream, (char*)indent_string, 0);
            }
        }

        jfes_add_to_stringstream(stream, "\"", 0);
        jfes_add_to_stringstream(stream, object_map->key.data, object_map->key.size - 1);
        jfes_add_to_stringstream(stream, "\":", 0);
        if (beautiful) {
            jfes_add_to_stringstream(stream, " ", 0);
        }

        jfes_status_t status = jfes_value_to_stream_helper(object_map->value, stream, beautiful, indent + 1, indent_string);
        if (jfes_status_is_bad(status)) {
            return status;
        }

        if (i < value->data.array_val->count - 1) {
            jfes_add_to_stringstream(stream, ",", 0);
        }

        if (beautiful) {
            jfes_add_to_stringstream(stream, "\n", 0);
        }
    }

    if (beautiful) {
        for (jfes_size_t i = 0; i < indent; i++) {
            jfes_add_to_stringstream(stream, (char*)indent_string, 0);
        }
    }
    return jfes_add_to_stringstream(stream, "}", 0);
}

static jfes_status_t jfes_value_to_stream_helper(const jfes_value_t *value, jfes_stringstream_t *stream, 
        int beautiful, jfes_size_t indent, const char *indent_string) {
    if (!value || !stream) {
        return jfes_invalid_arguments;
    }

    switch (value->type) {
    case jfes_type_null:
        return jfes_add_to_stringstream(stream, "null", 0);
    case jfes_type_boolean:
        return jfes_add_to_stringstream(stream, jfes_boolean_to_string(value->data.bool_val), 0);

    case jfes_type_integer:
    {
        char buffer[JFES_MAX_DIGITS];
        return jfes_add_to_stringstream(stream, jfes_integer_to_string_r(value->data.int_val, &buffer[0], JFES_MAX_DIGITS), 0);
    }

    case jfes_type_double:
    {
        char buffer[JFES_MAX_DIGITS];
        return jfes_add_to_stringstream(stream, jfes_double_to_string_r(
            value->data.double_val, &buffer[0], JFES_MAX_DIGITS, JFES_DOUBLE_PRECISION), 0);
    }

    case jfes_type_string:
        jfes_add_to_stringstream(stream, "\"", 0);
        jfes_add_to_stringstream(stream, value->data.string_val.data, value->data.string_val.size - 1);
        return jfes_add_to_stringstream(stream, "\"", 0);

    case jfes_type_array:
        return jfes_array_value_to_stream_helper(value, stream, beautiful, indent, indent_string);

    case jfes_type_object:
        return jfes_object_value_to_stream_helper(value, stream, beautiful, indent, indent_string);

    default:
        break;
    }
    return jfes_success;
}

/**
    Dumps JFES value to memory stream.

    \param[in]      value               JFES value to dump.
    \param[out]     data                Allocated memory to store.
    \param[in, out] max_size            Maximal size of data. Will store data length.
    \param[in]      beautiful           Beautiful JSON.

    \return         jfes_success if everything is OK.
*/
static jfes_status_t jfes_value_to_stream(const jfes_value_t *value, jfes_stringstream_t *stream, int beautiful) {
    return jfes_value_to_stream_helper(value, stream, beautiful, 0, "    ");
}

jfes_status_t jfes_value_to_string(const jfes_value_t *value, char *data, jfes_size_t *max_size, int beautiful) {
    if (!data || !max_size || *max_size == 0) {
        return jfes_invalid_arguments;
    }

    jfes_stringstream_t stream;
    jfes_status_t status = jfes_initialize_stringstream(&stream, data, *max_size);
    if (jfes_status_is_bad(status)) {
        return status;
    }

    status = jfes_value_to_stream(value, &stream, beautiful);
    if (jfes_status_is_bad(status)) {
        return status;
    }

    *max_size = stream.current_index;
    return jfes_success;
}
