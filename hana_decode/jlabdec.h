#ifndef __JLABDEC__
#define __JLABDEC__
#include <stdint.h>

typedef struct
{
  uint32_t undef:27;
  uint32_t data_type_tag:4;
  uint32_t data_type_defining:1;
} generic_data_word;

typedef union
{
  uint32_t raw;
  generic_data_word bf;
} generic_data_word_t;

/* 0: BLOCK HEADER */
typedef struct
{
  uint32_t number_of_events_in_block:8;
  uint32_t event_block_number:10;
  uint32_t module_ID:4;
  uint32_t slot_number:5;
  uint32_t data_type_tag:4;
  uint32_t data_type_defining:1;
} block_header;

typedef union
{
  uint32_t raw;
  block_header bf;
} block_header_t;

/* 1: BLOCK TRAILER */
typedef struct
{
  uint32_t words_in_block:22;
  uint32_t slot_number:5;
  uint32_t data_type_tag:4;
  uint32_t data_type_defining:1;
} block_trailer;

typedef union
{
  uint32_t raw;
  block_trailer bf;
} block_trailer_t;

/* 2: EVENT HEADER */
typedef struct
{
  uint32_t event_number:22;
  uint32_t slot_number:5;
  uint32_t data_type_tag:4;
  uint32_t data_type_defining:1;
} event_header;

typedef union
{
  uint32_t raw;
  event_header bf;
} event_header_t;

/* 13: EVENT TRAILER */
typedef struct
{
  uint32_t undef:22;
  uint32_t slot_number:5;
  uint32_t data_type_tag:4;
  uint32_t data_type_defining:1;
} event_trailer;

typedef union
{
  uint32_t raw;
  event_trailer bf;
} event_trailer_t;

/* 14: DATA NOT VALID */
typedef struct
{
  uint32_t undef:22;
  uint32_t slot_number:5;
  uint32_t data_type_tag:4;
  uint32_t data_type_defining:1;
} data_not_valid;

typedef union
{
  uint32_t raw;
  data_not_valid bf;
} data_not_valid_t;


/* 15: FILLER */
typedef struct
{
  uint32_t undef:22;
  uint32_t slot_number:5;
  uint32_t data_type_tag:4;
  uint32_t data_type_defining:1;
} filler_word;

typedef union
{
  uint32_t raw;
  filler_word bf;
} filler_word_t;

#endif /* __JLABDEC__ */
