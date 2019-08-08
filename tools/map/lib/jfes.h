/**
    \file       jfes.h
    \author     Eremin Dmitry (http://github.com/NeonMercury)
    \date       October, 2015
    \brief      Json For Embedded Systems library headers.
*/

#ifndef JFES_H_INCLUDE_GUARD
#define JFES_H_INCLUDE_GUARD

/** Strict JSON mode. **/
//#define JFES_STRICT

/** Maximal tokens count */
#define JFES_MAX_TOKENS_COUNT   8192

/** NULL define for the jfes library. */
#ifndef JFES_NULL
#define JFES_NULL               ((void*)0)
#endif

/** size_t type for the jfes library. */
typedef unsigned int jfes_size_t;

/** JFES return statuses. */
typedef enum jfes_status {
    jfes_unknown            = 0x00,             /**< Unknown status */

    jfes_success            = 0x01,             /**< Last operation finished sucessfully. */

    jfes_invalid_arguments  = 0x02,             /**< Invalid arguments were passed to the function. */
    jfes_no_memory          = 0x03,             /**< Not enough tokens were provided. */
    jfes_invalid_input      = 0x04,             /**< Invalid character in JSON string. */
    jfes_error_part         = 0x05,             /**< The string is not a full JSON packet. More bytes expected. */
    jfes_unknown_type       = 0x06,             /**< Unknown token type. */
    jfes_not_found          = 0x07,             /**< Something was not found. */
} jfes_status_t;

/** Memory allocator function type. */
typedef void *(*jfes_malloc_t)(jfes_size_t);

/** Memory deallocator function type. */
typedef void (*jfes_free_t)(void*);

/** JFES string type. */
typedef struct jfes_string {
    char            *data;                      /**< String bytes. */
    jfes_size_t     size;                       /**< Allocated bytes count. */
} jfes_string_t;

/** JFES token types */
typedef enum jfes_token_type {
    jfes_type_undefined          = 0x00,        /**< Undefined token type. */
    
    jfes_type_null               = 0x01,        /**< Null token type. */

    jfes_type_boolean            = 0x02,        /**< Boolean token type. */
    jfes_type_integer            = 0x03,        /**< Integer token type. */
    jfes_type_double             = 0x04,        /**< Double token type. */
    jfes_type_string             = 0x05,        /**< String token type. */

    jfes_type_array              = 0x06,        /**< Array token type. */
    jfes_type_object             = 0x07,        /**< Object token type. */
} jfes_token_type_t;

/** Json value type is the same as token type. */
typedef jfes_token_type_t jfes_value_type_t;

/** JFES token structure. */
typedef struct jfes_token {
    jfes_token_type_t       type;               /**< Token type. */

    int                     start;              /**< Token start position. */
    int                     end;                /**< Token end position. */
    jfes_size_t             size;               /**< Token children count. */
} jfes_token_t;

/** JFES config structure. */
typedef struct jfes_config {
    jfes_malloc_t           jfes_malloc;        /**< Memory allocation function. */
    jfes_free_t             jfes_free;          /**< Memory deallocation function. */
} jfes_config_t;

/** JFES tokens data structure. */
typedef struct jfes_tokens_data {
    const jfes_config_t     *config;            /**< JFES configuration. */

    const char              *json_data;         /**< JSON string. */
    jfes_size_t             json_data_length;   /**< JSON string length. */

    jfes_token_t            *tokens;            /**< String parsing result in tokens. */
    jfes_size_t             tokens_count;       /**< Tokens count. */
    jfes_size_t             current_token;      /**< Index of current token. */
} jfes_tokens_data_t;

/** JFES parser structure. */
typedef struct jfes_parser {
    jfes_size_t             pos;                /**< Current offset in json string. */ 
    jfes_size_t             next_token;         /**< Next token to allocate. */
    int                     superior_token;     /**< Superior token node. */

    const jfes_config_t     *config;            /**< Pointer to jfes config. */
} jfes_parser_t;


/** JSON value structure. */
typedef struct jfes_value jfes_value_t;

/** JFES `key -> value` mapping structure. */
typedef struct jfes_object_map {
    jfes_string_t           key;                /**< Object key. */
    jfes_value_t            *value;             /**< Oject value. */
} jfes_object_map_t;

/** JSON array structure. */
typedef struct jfes_array {
    jfes_value_t            **items;            /**< JSON items in array. */    
    jfes_size_t             count;              /**< Items count in array. */
} jfes_array_t;

/** JSON object structure. */
typedef struct jfes_object {
    jfes_object_map_t       **items;            /**< JSON items in object. */
    jfes_size_t             count;              /**< Items count in object. */
} jfes_object_t;

/** JFES value data union. */
typedef union jfes_value_data {
    int                     bool_val;           /**< Boolean JSON value. */

    int                     int_val;            /**< Integer JSON value. */
    double                  double_val;         /**< Double JSON value. */
    jfes_string_t           string_val;         /**< String JSON value. */

    jfes_array_t            *array_val;         /**< Array JSON value. */
    jfes_object_t           *object_val;        /**< Object JSON value. */
} jfes_value_data_t;

/** JSON value structure. */
struct jfes_value {
    jfes_value_type_t       type;               /**< JSON value type. */
    jfes_value_data_t       data;               /**< Value data. */
};

/** 
    JFES status analizer function.

    \param[in]      status              Status variable.

    \return Zero status is not jfes_success. Otherwise anything else.
*/
int jfes_status_is_good(jfes_status_t status);

/**
    JFES status analizer function.

    \param[in]      status              Status variable.

    \return Zero if status is jfes_success. Otherwise anything else.
*/
int jfes_status_is_bad(jfes_status_t status);

/**
    JFES parser initialization.

    \param[out]     parser              Pointer to the jfes_parser_t object.
    \param[in]      config              JFES configuration.

    \return         jfes_success if everything is OK.
*/
jfes_status_t jfes_init_parser(jfes_parser_t *parser, const jfes_config_t *config);

/**
    Resets all parser fields, except memory allocation functions.

    \param[out]     parser              Pointer to the jfes_parser_t object.

    \return         jfes_success if everything is OK.
*/
jfes_status_t jfes_reset_parser(jfes_parser_t *parser);

/**
    Runs JSON parser. It parses a JSON data string into an 
    array of tokens, each describing a single JSON object.

    \param[in]      parser              Pointer to the jfes_parser_t object.
    \param[in]      json                JSON data string.
    \param[in]      length              JSON data length.
    \param[out]     tokens              Tokens array to fill.
    \param[in, out] max_tokens_count    Maximal count of tokens in tokens array.
                                        Will contain tokens count.

    \return         jfes_success if everything is OK.
*/
jfes_status_t jfes_parse_tokens(jfes_parser_t *parser, const char *json, 
    jfes_size_t length, jfes_token_t *tokens, jfes_size_t *max_tokens_count);

/**
    Runs JSON parser and fills jfes_value_t object.

    \param[in]      config              JFES configuration.
    \param[in]      json                JSON data string.
    \param[in]      length              JSON data length.
    \param[out]     value               Output value.

    \return         jfes_success if everything is OK.
*/
jfes_status_t jfes_parse_to_value(const jfes_config_t *config, const char *json,
    jfes_size_t length, jfes_value_t *value);

/**
    Frees all resources captured by the object.

    \param[in]      config              JFES configuration.
    \param[in,out]  value               Object to free.

    \return         jfes_success if everything is OK.
*/
jfes_status_t jfes_free_value(const jfes_config_t *config, jfes_value_t *value);

/**
    Allocates a new null value.

    \param[in]      config              JFES configuration.

    \return         Allocated JFES value or JFES_NULL if something went wrong.
*/
jfes_value_t *jfes_create_null_value(const jfes_config_t *config);

/**
    Allocates a new boolean value.

    \param[in]      config              JFES configuration.
    \param[in]      value               Value to pass it to the object.

    \return         Allocated JFES value or JFES_NULL if something went wrong.
*/
jfes_value_t *jfes_create_boolean_value(const jfes_config_t *config, int value);

/**
    Allocates a new integer value.

    \param[in]      config              JFES configuration.
    \param[in]      value               Value to pass it to the object.

    \return         Allocated JFES value or JFES_NULL if something went wrong.
*/
jfes_value_t *jfes_create_integer_value(const jfes_config_t *config, int value);

/**
    Allocates a new double value.

    \param[in]      config              JFES configuration.
    \param[in]      value               Value to pass it to the object.

    \return         Allocated JFES value or JFES_NULL if something went wrong.
*/
jfes_value_t *jfes_create_double_value(const jfes_config_t *config, double value);

/**
    Allocates a new string value.

    \param[in]      config              JFES configuration.
    \param[in]      value               Value to pass it to the object.
    \param[in]      length              Optional. String length. You can pass 0
                                        if the string is zero-terminated.

    \return         Allocated JFES value or JFES_NULL if something went wrong.
*/
jfes_value_t *jfes_create_string_value(const jfes_config_t *config, const char *value, jfes_size_t length);

/**
    Allocates a new array value.

    \param[in]      config              JFES configuration.

    \return         Allocated JFES value or JFES_NULL if something went wrong.
*/
jfes_value_t *jfes_create_array_value(const jfes_config_t *config);

/**
    Allocates a new object value.

    \param[in]      config              JFES configuration.

    \return         Allocated JFES value or JFES_NULL if something went wrong.
*/
jfes_value_t *jfes_create_object_value(const jfes_config_t *config);

/**
    Finds child value if given parent value is object.

    \param[in]      value               Parent object value.
    \param[in]      key                 Child key.
    \param[in]      key_length          Optional. Child key length. You can pass 0
                                        if the key string is zero-terminated.

    \return         Child value by the given key or JFES_NULL if nothing was found.
*/
jfes_value_t *jfes_get_child(const jfes_value_t *value, const char *key, jfes_size_t key_length);

/**
    Finds child value if given parent value is object.

    \param[in]      value               Parent object value.
    \param[in]      key                 Child key.
    \param[in]      key_length          Optional. Child key length. You can pass 0
                                        if the key string is zero-terminated.

    \return         Mapped child value with the given key or JFES_NULL if nothing was found.
*/
jfes_object_map_t *jfes_get_mapped_child(const jfes_value_t *value, const char *key, jfes_size_t key_length);

/**
    Adds a new item to the given array value.

    \param[in]      config              JFES configuration.
    \param[in]      value               Array value.
    \param[in]      item                Item to add. Must be allocated on heap.

    \return         jfes_success if everything is OK.
*/
jfes_status_t jfes_place_to_array(const jfes_config_t *config, const jfes_value_t *value, jfes_value_t *item);

/**
    Adds a new item to the given array value in the given place.

    \param[in]      config              JFES configuration.
    \param[in]      value               Array value.
    \param[in]      item                Item to add. Must be allocated on heap.
    \param[in]      place_at            Index to place.

    \return         jfes_success if everything is OK.
*/
jfes_status_t jfes_place_to_array_at(const jfes_config_t *config, const jfes_value_t *value, jfes_value_t *item, jfes_size_t place_at);

/**
    Removes an item with the given index from the array.

    \param[in]      config              JFES configuration.
    \param[in]      value               Array value.
    \param[in]      index               Index to remove.

    \return         jfes_success if everything is OK.
*/
jfes_status_t jfes_remove_from_array(const jfes_config_t *config, const jfes_value_t *value, jfes_size_t index);

/**
    Adds a new item to the given object.

    \param[in]      config              JFES configuration.
    \param[in]      value               Array value.
    \param[in]      item                Item to add. Must be allocated on heap.
    \param[in]      key                 Child key.
    \param[in]      key_length          Optional. Child key length. You can pass 0
                                        if the key string is zero-terminated.

    \return         jfes_success if everything is OK.
*/
jfes_status_t jfes_set_object_property(const jfes_config_t *config, const jfes_value_t *value, 
    jfes_value_t *item, const char *key, jfes_size_t key_length);

/**
    Removes a child object with the given key.

    \param[in]      config              JFES configuration.
    \param[in]      value               Array value.
    \param[in]      key                 Child key.
    \param[in]      key_length          Optional. Child key length. You can pass 0
                                        if the key string is zero-terminated.

    \return         jfes_success if everything is OK.
*/
jfes_status_t jfes_remove_object_property(const jfes_config_t *config, const jfes_value_t *value, 
    const char *key, jfes_size_t key_length);

/**
    Dumps JFES value to the memory.

    \param[in]      value               JFES value to dump.
    \param[out]     data                Allocated memory to store.
    \param[in, out] max_size            Maximal size of data. Will store data length.
    \param[in]      beautiful           Beautiful JSON.

    \return         jfes_success if everything is OK.
*/
jfes_status_t jfes_value_to_string(const jfes_value_t *value, char *data, jfes_size_t *max_size, int beautiful);

#endif
