/*!
 *************************************************************************************
 * \file biaridecod.c
 *
 * \brief
 *    binary arithmetic decoder routines
 * \date
 *    21. Oct 2000
 * \author
 *    Main contributors (see contributors.h for copyright, address and affiliation details)
 *    - Detlev Marpe                    <marpe@hhi.de>
 *    - Gabi Blaettermann               <blaetter@hhi.de>
 *************************************************************************************
 */

#include <stdlib.h>
#include <math.h>
#include "global.h"
#include "biaridecod.h"

extern int symbolCount;

/************************************************************************
 * M a c r o s
 ************************************************************************
 */

#define get_byte(){                                                     \
            Dbuffer = Dcodestrm[(*Dcodestrm_len)++];        \
                        Dbits_to_go = 7;                                \
          }


/************************************************************************
 ************************************************************************
                      init / exit decoder
 ************************************************************************
 ************************************************************************/


/*!
 ************************************************************************
 * \brief
 *    Allocates memory for the DecodingEnvironment struct
 * \return DecodingContextPtr
 *    allocated memory
 ************************************************************************
 */
DecodingEnvironmentPtr arideco_create_decoding_environment()
{
  DecodingEnvironmentPtr dep;

  if ((dep = calloc(1,sizeof(DecodingEnvironment))) == NULL)
    no_mem_exit("arideco_create_decoding_environment: dep");
  return dep;
}


/*!
 ***********************************************************************
 * \brief
 *    Frees memory of the DecodingEnvironment struct
 ***********************************************************************
 */
void arideco_delete_decoding_environment(DecodingEnvironmentPtr dep)
{
  if (dep == NULL)
  {
    snprintf(errortext, ET_SIZE, "Error freeing dep (NULL pointer)");
    error (errortext, 200);
  }
  else
    free(dep);
}


/*!
 ************************************************************************
 * \brief
 *    Initializes the DecodingEnvironment for the arithmetic coder
 ************************************************************************
 */
void arideco_start_decoding(DecodingEnvironmentPtr dep, unsigned char *cpixcode,
                            int firstbyte, int *cpixcode_len )
{
  int i;
  int bits;

  bits = CODE_VALUE_BITS;

  Dcodestrm = cpixcode;
  Dcodestrm_len = cpixcode_len;
  *Dcodestrm_len = firstbyte;

  Dbits_to_go = 0;
  dep->Dvalue = 0;

  for (i = 0; i < bits; i++)
  {
    if (--Dbits_to_go < 0)
      get_byte();
    dep->Dvalue += dep->Dvalue  + (Dbuffer & 1);
    Dbuffer >>= 1;
  }
  dep->Dlow = 0;
  dep->Dhigh = TOP_VALUE;
}


/*!
 ************************************************************************
 * \brief
 *    arideco_bits_read
 ************************************************************************
 */
int arideco_bits_read(DecodingEnvironmentPtr dep)
{
  return 8 * ((*Dcodestrm_len)-1) + (8 - Dbits_to_go) - CODE_VALUE_BITS;
}


/*!
 ************************************************************************
 * \brief
 *    arideco_done_decoding():
 ************************************************************************
 */
void arideco_done_decoding(DecodingEnvironmentPtr dep)
{
  (*Dcodestrm_len)++;
}



/*!
 ************************************************************************
 * \brief
 *    biari_decode_symbol():
 * \return
 *    the decoded symbol
 ************************************************************************
 */
unsigned int biari_decode_symbol(DecodingEnvironmentPtr dep, BiContextTypePtr bi_ct )
{
  unsigned int scaled_range;
  unsigned int symbol=0;
  int Dlow_m1 = dep->Dlow - 1;

#if  AAC_FRAC_TABLE
  if ( (scaled_range =  ( ( (dep->Dhigh - Dlow_m1) * ((bi_ct->cum_freq[1]*ARITH_CUM_FREQ_TABLE[bi_ct->cum_freq[0]])>>16))>>10) ) >=  (dep->Dvalue - Dlow_m1) )
#else
  if ( (scaled_range =  ( ( (dep->Dhigh - Dlow_m1) * bi_ct->cum_freq[1]) / bi_ct->cum_freq[0] )) >=  (dep->Dvalue - Dlow_m1) )
#endif
  {
    symbol++;
    dep->Dhigh = Dlow_m1 + scaled_range;
    bi_ct->cum_freq[1]++;
  }
  else
    dep->Dlow += scaled_range;

  if (++bi_ct->cum_freq[0] >= bi_ct->max_cum_freq)
    rescale_cum_freq(bi_ct);

  do
  {
    if (dep->Dhigh >= HALF)
    {
      if (dep->Dlow < HALF)
        if (dep->Dlow >= FIRST_QTR && dep->Dhigh < THIRD_QTR)
        {
          dep->Dhigh -= FIRST_QTR;
          dep->Dvalue -= FIRST_QTR;
          dep->Dlow -= FIRST_QTR;
        }
        else
          break;
        else
        {
          dep->Dhigh -= HALF;
          dep->Dvalue -= HALF;
          dep->Dlow -= HALF;
        }
    }
    dep->Dlow <<= 1;
    dep->Dhigh += dep->Dhigh+1;

    if (--Dbits_to_go < 0)
      get_byte();

    dep->Dvalue += dep->Dvalue  + (Dbuffer & 1);
    Dbuffer >>= 1;
  }
  while (1);

  return symbol;
}



/*!
 ************************************************************************
 * \brief
 *    Initializes a given context with some pre-defined probabilities
 *    and a maximum symbol count for triggering the rescaling
 ************************************************************************
 */
void biari_init_context( BiContextTypePtr ctx, int ini_count_0, int ini_count_1, int max_cum_freq )
{

  ctx->in_use       = TRUE;
  ctx->max_cum_freq = max_cum_freq;


  ctx->cum_freq[1]  = ini_count_1;
  ctx->cum_freq[0]  = ini_count_0 + ini_count_1;

}


/*!
 ************************************************************************
 * \brief
 *    biari_copy_context():
 ************************************************************************
 */
void biari_copy_context( BiContextTypePtr ctx_orig, BiContextTypePtr ctx_dest )
{
  ctx_dest->in_use     =  ctx_orig->in_use;
  ctx_dest->max_cum_freq = ctx_orig->max_cum_freq;

  ctx_dest->cum_freq[1] = ctx_orig->cum_freq[1];
  ctx_dest->cum_freq[0] = ctx_orig->cum_freq[0];

  return;
}

/*!
 ***********************************************************************
 * \brief
 *    biari_print_context():
 ***********************************************************************
 */
void biari_print_context( BiContextTypePtr ctx )
{
  printf("0: %4d\t",ctx->cum_freq[0] - ctx->cum_freq[1]);
  printf("1: %4d",ctx->cum_freq[1]);

  return;
}


/*!
 ***********************************************************************
 * \brief
 *    Rescales a given context model by halvening the symbol counts
 *
 ***********************************************************************
 */
void rescale_cum_freq( BiContextTypePtr   bi_ct)
{
  int old_cum_freq_of_one = bi_ct->cum_freq[1];

  bi_ct->cum_freq[1] = (bi_ct->cum_freq[1] + 1) >> 1;
  bi_ct->cum_freq[0] = bi_ct->cum_freq[1] +
    ( ( bi_ct->cum_freq[0] - old_cum_freq_of_one + 1 ) >> 1);
}
