#ifndef __VFTDCDEC__
#define __VFTDCDEC__
#include <stdint.h>

/* 3: TRIGGER TIME */
typedef struct
{
  uint32_t T_F:8;
  uint32_t T_E:8;
  uint32_t T_D:8;
  uint32_t undef:3;
  uint32_t data_type_tag:4;
  uint32_t data_type_defining:1;
} vfTDC_trigger_time_1;

typedef union
{
  uint32_t raw;
  vfTDC_trigger_time_1 bf;
} vfTDC_trigger_time_1_t;

typedef struct
{
  uint32_t T_C:8;
  uint32_t T_B:8;
  uint32_t T_A:8;
  uint32_t undef:3;
  uint32_t data_type_tag:4;
  uint32_t data_type_defining:1;
} vfTDC_trigger_time_2;

typedef union
{
  uint32_t raw;
  vfTDC_trigger_time_2 bf;
} vfTDC_trigger_time_2_t;

/* 7: TIME MEASUREMENT */
typedef struct
{
  uint32_t time_fine:7;
  uint32_t two_ns:1;
  uint32_t time_coarse:10;
  uint32_t edge_type:1;
  uint32_t chan:5;
  uint32_t group:3;
  uint32_t data_type_tag:4;
  uint32_t data_type_defining:1;
} vfTDC_time_measurement;

typedef union
{
  uint32_t raw;
  vfTDC_time_measurement bf;
} vfTDC_time_measurement_t;

void  vfTDCDataDecode(uint32_t data);


#endif /* __VFTDCDEC__ */
