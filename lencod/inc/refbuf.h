/*
***********************************************************************
* COPYRIGHT AND WARRANTY INFORMATION
*
* Copyright 2001, International Telecommunications Union, Geneva
*
* DISCLAIMER OF WARRANTY
*
* These software programs are available to the user without any
* license fee or royalty on an "as is" basis. The ITU disclaims
* any and all warranties, whether express, implied, or
* statutory, including any implied warranties of merchantability
* or of fitness for a particular purpose.  In no event shall the
* contributor or the ITU be liable for any incidental, punitive, or
* consequential damages of any kind whatsoever arising from the
* use of these programs.
*
* This disclaimer of warranty extends to the user of these programs
* and user's customers, employees, agents, transferees, successors,
* and assigns.
*
* The ITU does not represent or warrant that the programs furnished
* hereunder are free of infringement of any third-party patents.
* Commercial implementations of ITU-T Recommendations, including
* shareware, may be subject to royalty fees to patent holders.
* Information regarding the ITU-T patent policy is available from
* the ITU Web site at http://www.itu.int.
*
* THIS IS NOT A GRANT OF PATENT RIGHTS - SEE THE ITU-T PATENT POLICY.
************************************************************************
*/

/*!
 ************************************************************************
 * \file refbuf.h
 *
 * \brief
 *    Declarations of the reference frame buffer types and functions
 ************************************************************************
 */
#ifndef _REBUF_H_
#define _REBUF_H_


#include "global.h"


pel_t UMVPelY_14 (pel_t **Pic, int y, int x);
pel_t FastPelY_14 (pel_t **Pic, int y, int x);

pel_t UMVPelY_18 (pel_t **Pic, int y, int x);
pel_t FastPelY_18 (pel_t **Pic, int y, int x);

pel_t UMVPelY_11 (pel_t *Pic, int y, int x);
pel_t FastPelY_11 (pel_t *Pic, int y, int x);
pel_t *FastLine16Y_11 (pel_t *Pic, int y, int x);
pel_t *UMVLine16Y_11 (pel_t *Pic, int y, int x);

void PutPel_14 (pel_t **Pic, int y, int x, pel_t val);
void PutPel_11 (pel_t *Pic, int y, int x, pel_t val);

#endif

