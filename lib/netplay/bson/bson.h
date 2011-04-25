/* bson.h */

/*    Copyright 2009, 2010 10gen Inc.
 *
 *    Licensed under the Apache License, Version 2.0 (the "License");
 *    you may not use this file except in compliance with the License.
 *    You may obtain a copy of the License at
 *
 *    http://www.apache.org/licenses/LICENSE-2.0
 *
 *    Unless required by applicable law or agreed to in writing, software
 *    distributed under the License is distributed on an "AS IS" BASIS,
 *    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *    See the License for the specific language governing permissions and
 *    limitations under the License.
 */

#ifndef _BSON_H_
#define _BSON_H_

#include "platform_hacks.h"
#include <time.h>

MONGO_EXTERN_C_START

typedef enum {
    bson_eoo=0 ,
    bson_double=1,
    bson_string=2,
    bson_object=3,
    bson_array=4,
    bson_bindata=5,
    bson_undefined=6,
    bson_oid=7,
    bson_bool=8,
    bson_date=9,
    bson_null=10,
    bson_regex=11,
    bson_dbref=12, /* deprecated */
    bson_code=13,
    bson_symbol=14,
    bson_codewscope=15,
    bson_int = 16,
    bson_timestamp = 17,
    bson_long = 18
} bson_type;

typedef int bson_bool_t;

typedef struct {
    char * data;
    bson_bool_t owned;
} bson;

typedef struct {
    const char * cur;
    bson_bool_t first;
} bson_iterator;

typedef struct {
    char * buf;
    char * cur;
    int bufSize;
    bson_bool_t finished;
    int stack[32];
    int stackPos;
} bson_buffer;

#pragma pack(1)
typedef union{
    char bytes[12];
    int ints[3];
} bson_oid_t;
#pragma pack()

typedef int64_t bson_date_t; /* milliseconds since epoch UTC */

typedef struct {
  int i; /* increment */
  int t; /* time in seconds */
} bson_timestamp_t;

/* ----------------------------
   READING
   ------------------------------ */

bson * bson_empty(bson * obj); /* returns pointer to static empty bson object */
void bson_copy(bson* out, const bson* in); /* puts data in new buffer. NOOP if out==NULL */
bson * bson_from_buffer(bson * b, bson_buffer * buf);
bson * bson_init( bson * b , char * data , bson_bool_t mine );
int bson_size(const bson * b );
void bson_destroy( bson * b );

void bson_print( bson * b );
void bson_print_raw( const char * bson , int depth );

/* advances iterator to named field */
/* returns bson_eoo (which is false) if field not found */
bson_type bson_find(bson_iterator* it, const bson* obj, const char* name);

void bson_iterator_init( bson_iterator * i , const char * bson );

/* more returns true for eoo. best to loop with bson_iterator_next(&it) */
bson_bool_t bson_iterator_more( const bson_iterator * i );
bson_type bson_iterator_next( bson_iterator * i );

bson_type bson_iterator_type( const bson_iterator * i );
const char * bson_iterator_key( const bson_iterator * i );
const char * bson_iterator_value( const bson_iterator * i );

/* these convert to the right type (return 0 if non-numeric) */
double bson_iterator_double( const bson_iterator * i );
int bson_iterator_int( const bson_iterator * i );
int64_t bson_iterator_long( const bson_iterator * i );

/* return the bson timestamp as a whole or in parts */
bson_timestamp_t bson_iterator_timestamp( const bson_iterator * i );

/* false: boolean false, 0 in any type, or null */
/* true: anything else (even empty strings and objects) */
bson_bool_t bson_iterator_bool( const bson_iterator * i );

/* these assume you are using the right type */
double bson_iterator_double_raw( const bson_iterator * i );
int bson_iterator_int_raw( const bson_iterator * i );
int64_t bson_iterator_long_raw( const bson_iterator * i );
bson_bool_t bson_iterator_bool_raw( const bson_iterator * i );
bson_oid_t* bson_iterator_oid( const bson_iterator * i );

/* these can also be used with bson_code and bson_symbol*/
const char * bson_iterator_string( const bson_iterator * i );
int bson_iterator_string_len( const bson_iterator * i );

/* works with bson_code, bson_codewscope, and bson_string */
/* returns NULL for everything else */
const char * bson_iterator_code(const bson_iterator * i);

/* calls bson_empty on scope if not a bson_codewscope */
void bson_iterator_code_scope(const bson_iterator * i, bson * scope);

/* both of these only work with bson_date */
bson_date_t bson_iterator_date(const bson_iterator * i);
time_t bson_iterator_time_t(const bson_iterator * i);

int bson_iterator_bin_len( const bson_iterator * i );
char bson_iterator_bin_type( const bson_iterator * i );
const char * bson_iterator_bin_data( const bson_iterator * i );

const char * bson_iterator_regex( const bson_iterator * i );
const char * bson_iterator_regex_opts( const bson_iterator * i );

/* these work with bson_object and bson_array */
void bson_iterator_subobject(const bson_iterator * i, bson * sub);
void bson_iterator_subiterator(const bson_iterator * i, bson_iterator * sub);

/* str must be at least 24 hex chars + null byte */
void bson_oid_from_string(bson_oid_t* oid, const char* str);
void bson_oid_to_string(const bson_oid_t* oid, char* str);
void bson_oid_gen(bson_oid_t* oid);

time_t bson_oid_generated_time(bson_oid_t* oid); /* Gives the time the OID was created */

/* ----------------------------
   BUILDING
   ------------------------------ */

bson_buffer * bson_buffer_init( bson_buffer * b );
bson_buffer * bson_ensure_space( bson_buffer * b , const int bytesNeeded );

/**
 * @return the raw data.  you either should free this OR call bson_destroy not both
 */
char * bson_buffer_finish( bson_buffer * b );
void bson_buffer_destroy( bson_buffer * b );

bson_buffer * bson_append_oid( bson_buffer * b , const char * name , const bson_oid_t* oid );
bson_buffer * bson_append_new_oid( bson_buffer * b , const char * name );
bson_buffer * bson_append_int( bson_buffer * b , const char * name , const int i );
bson_buffer * bson_append_long( bson_buffer * b , const char * name , const int64_t i );
bson_buffer * bson_append_double( bson_buffer * b , const char * name , const double d );
bson_buffer * bson_append_string( bson_buffer * b , const char * name , const char * str );
bson_buffer * bson_append_string_n( bson_buffer * b, const char * name, const char * str , int len);
bson_buffer * bson_append_symbol( bson_buffer * b , const char * name , const char * str );
bson_buffer * bson_append_symbol_n( bson_buffer * b , const char * name , const char * str , int len );
bson_buffer * bson_append_code( bson_buffer * b , const char * name , const char * str );
bson_buffer * bson_append_code_n( bson_buffer * b , const char * name , const char * str , int len );
bson_buffer * bson_append_code_w_scope( bson_buffer * b , const char * name , const char * code , const bson * scope);
bson_buffer * bson_append_code_w_scope_n( bson_buffer * b , const char * name , const char * code , int size , const bson * scope);
bson_buffer * bson_append_binary( bson_buffer * b, const char * name, char type, const char * str, int len );
bson_buffer * bson_append_bool( bson_buffer * b , const char * name , const bson_bool_t v );
bson_buffer * bson_append_null( bson_buffer * b , const char * name );
bson_buffer * bson_append_undefined( bson_buffer * b , const char * name );
bson_buffer * bson_append_regex( bson_buffer * b , const char * name , const char * pattern, const char * opts );
bson_buffer * bson_append_bson( bson_buffer * b , const char * name , const bson* bson);
bson_buffer * bson_append_element( bson_buffer * b, const char * name_or_null, const bson_iterator* elem);
bson_buffer * bson_append_timestamp( bson_buffer * b, const char * name, bson_timestamp_t * ts );

/* these both append a bson_date */
bson_buffer * bson_append_date(bson_buffer * b, const char * name, bson_date_t millis);
bson_buffer * bson_append_time_t(bson_buffer * b, const char * name, time_t secs);

bson_buffer * bson_append_start_object( bson_buffer * b , const char * name );
bson_buffer * bson_append_start_array( bson_buffer * b , const char * name );
bson_buffer * bson_append_finish_object( bson_buffer * b );

void bson_numstr(char* str, int i);
void bson_incnumstr(char* str);


/* ------------------------------
   ERROR HANDLING - also used in mongo code
   ------------------------------ */

void * bson_malloc(int size); /* checks return value */
void * bson_realloc(void * ptr, int size); /* checks return value */

/* bson_err_handlers shouldn't return!!! */
typedef void(*bson_err_handler)(const char* errmsg);

/* returns old handler or NULL */
/* default handler prints error then exits with failure*/
bson_err_handler set_bson_err_handler(bson_err_handler func);

/* does nothing is ok != 0 */
void bson_fatal( int ok );
void bson_fatal_msg( int ok, const char* msg );

MONGO_EXTERN_C_END
#endif
