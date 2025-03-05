#include <stdio.h>
#include <stdint.h>
#include "jlabdec.h"
#include "vfTDCdec.h"

void
vfTDCDataDecode(uint32_t data)
{
  static uint32_t type_last = 15;	/* initialize to type FILLER WORD */
  static uint32_t time_last = 0;
  static int new_type = 0;
  int type_current = 0;
  generic_data_word_t gword;

  gword.raw = data;

  if(gword.bf.data_type_defining) /* data type defining word */
    {
      new_type = 1;
      type_current = gword.bf.data_type_tag;
    }
  else
    {
      new_type = 0;
      type_current = type_last;
    }

  switch( type_current )
    {
    case 0:		/* BLOCK HEADER */
      {
	block_header_t d; d.raw = data;

	printf("%8X - BLOCK HEADER - slot = %d  modID = %d   n_evts = %d   n_blk = %d\n",
	       d.raw,
	       d.bf.slot_number,
	       d.bf.module_ID,
	       d.bf.number_of_events_in_block,
	       d.bf.event_block_number);

      }

    case 1:		/* BLOCK TRAILER */
      {
	block_trailer_t d; d.raw = data;

	printf("%8X - BLOCK TRAILER - slot = %d   n_words = %d\n",
		     d.raw,
		     d.bf.slot_number,
		     d.bf.words_in_block);
	break;
      }

    case 2:		/* EVENT HEADER */
      {
	event_header_t d; d.raw = data;

	printf("%8X - EVENT HEADER 1 - slot = %d   event num = %d\n",
	       d.raw,
	       d.bf.slot_number,
	       d.bf.event_number);
	break;
      }

    case 3:		/* TRIGGER TIME */
      {
	if( new_type )
	  {
	    vfTDC_trigger_time_1_t d; d.raw = data;

	    printf("%8X - TRIGGER TIME 1 - time = %08x\n",
		   d.raw,
		   (d.bf.T_D<<16) | (d.bf.T_E<<8) | (d.bf.T_F) );

	    time_last = 1;
	  }
	else
	  {
	    vfTDC_trigger_time_2_t d; d.raw = data;
	    if( time_last == 1 )
	      {
		printf("%8X - TRIGGER TIME 2 - time = %08x\n",
		       d.raw,
		       (d.bf.T_A<<16) | (d.bf.T_B<<8) | (d.bf.T_C) );
	      }
	    else
	      printf("%8X - TRIGGER TIME - (ERROR)\n", data);

	    time_last = 0;
	  }
	break;
      }

    case 7:             /* TIME MEASUREMENT */
      {
	vfTDC_time_measurement_t d; d.raw = data;

      printf("%8X - TDC - grp = %d  ch = %2d  edge = %d  coarse = %4d  2ns = %d  fine time = %3d\n",
	     data,
	     d.bf.group,
	     d.bf.chan,
	     d.bf.edge_type,
	     d.bf.time_coarse,
	     d.bf.two_ns,
	     d.bf.time_fine);

	break;
      }

    case 14:		/* DATA NOT VALID (no data available) */
      {
	data_not_valid_t d; d.raw = data;

	printf("%8X - DATA NOT VALID = %d\n",
	       d.raw,
	       d.bf.data_type_tag);
	break;
      }

    case 15:		/* FILLER WORD */
      {
	filler_word_t d; d.raw = data;

	printf("%8X - FILLER WORD - slot = %d  data = 0x%06x\n",
	       d.raw,
	       d.bf.slot_number,
	       d.bf.undef);
	break;
      }

    case 4:
    case 5:
    case 6:
    case 8:
    case 9:
    case 10:
    case 11:
    case 12:
    case 13:
    default:
      {
	printf("%8X - UNDEFINED TYPE = %d\n",
	       gword.raw,
	       gword.bf.data_type_tag);
	break;
      }

    }

  type_last = type_current;	/* save type of current data word */

}
