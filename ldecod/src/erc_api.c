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
 *************************************************************************************
 * \file erc_api.c
 *
 * \brief
 *    External (still inside video decoder) interface for error concealment module
 *
 *  \author
 *     - Ari Hourunranta                <ari.hourunranta@nokia.com>
 *     - Viktor Varsa                     <viktor.varsa@nokia.com>
 *     - Ye-Kui Wang                   <wangy@cs.tut.fi>
 *
 *************************************************************************************
 */


#include <stdlib.h>
#include <memory.h>

#include "global.h"
#include "memalloc.h"
#include "erc_api.h"

objectBuffer_t *erc_object_list;
ercVariables_t *erc_errorVar;
frame erc_recfr;
int erc_mvperMB;

/*!
 ************************************************************************
 * \brief
 *    Initinize the error concealment module
 ************************************************************************
 */
void ercInit(int pic_sizex, int pic_sizey, int flag)
{
  erc_object_list = (objectBuffer_t *) calloc((pic_sizex * pic_sizey) >> 6, sizeof(objectBuffer_t));
  if (erc_object_list == NULL) no_mem_exit("ercInit: erc_object_list");
  
  /* the error concelament instance is allocated */
  erc_errorVar = ercOpen();
  
  /* set error concealment ON */
  ercSetErrorConcealment(erc_errorVar, flag);
}

/*!
 ************************************************************************
 * \brief
 *      Allocates data structures used in error concealment.
 *\return
 *      The allocated ercVariables_t is returned.
 ************************************************************************
 */
ercVariables_t *ercOpen( void )
{
  ercVariables_t *errorVar = NULL;
  
  errorVar = (ercVariables_t *)malloc( sizeof(ercVariables_t));
  if ( errorVar == NULL ) no_mem_exit("ercOpen: errorVar");

  errorVar->nOfMBs = 0;
  errorVar->segments = NULL;
  errorVar->currSegment = 0;
  errorVar->yCondition = NULL;
  errorVar->uCondition = NULL;
  errorVar->vCondition = NULL;
  errorVar->prevFrameYCondition = NULL;
  
  errorVar->concealment = 1;
  
  return errorVar;
}

/*!
 ************************************************************************
 * \brief
 *      Resets the variables used in error detection. 
 *      Should be called always when starting to decode a new frame.
 * \param errorVar
 *      Variables for error concealment
 * \param nOfMBs
 *      Number of macroblocks in a frame
 * \param numOfSegments
 *    Estimated number of segments (memory reserved)
 ************************************************************************
 */
void ercReset( ercVariables_t *errorVar, int nOfMBs, int numOfSegments, int32 picSizeX )
{
  int *tmp = NULL;
  int i = 0, j = 0;
  
  if ( errorVar && errorVar->concealment ) 
  {
    /* If frame size has been changed */
    if ( nOfMBs != errorVar->nOfMBs && errorVar->yCondition != NULL ) 
    {
      free( errorVar->yCondition );
      errorVar->yCondition = NULL;
      free( errorVar->prevFrameYCondition );
      errorVar->prevFrameYCondition = NULL;
      free( errorVar->uCondition );
      errorVar->uCondition = NULL;
      free( errorVar->vCondition );
      errorVar->vCondition = NULL;
      free( errorVar->segments );
      errorVar->segments = NULL;
    }
    
    /* If the structures are uninitialized (first frame, or frame size is chaned) */
    if ( errorVar->yCondition == NULL ) 
    {
      errorVar->segments = (ercSegment_t *)malloc( numOfSegments*sizeof(ercSegment_t) );
      if ( errorVar->segments == NULL ) no_mem_exit("ercReset: errorVar->segments");
      memset( errorVar->segments, 0, numOfSegments*sizeof(ercSegment_t));
      errorVar->nOfSegments = numOfSegments;
      
      errorVar->yCondition = (int *)malloc( 4*nOfMBs*sizeof(int) );
      if ( errorVar->yCondition == NULL ) no_mem_exit("ercReset: errorVar->yCondition");
      errorVar->prevFrameYCondition = (int *)malloc( 4*nOfMBs*sizeof(int) );
      if ( errorVar->prevFrameYCondition == NULL ) no_mem_exit("ercReset: errorVar->prevFrameYCondition");
      errorVar->uCondition = (int *)malloc( nOfMBs*sizeof(int) );
      if ( errorVar->uCondition == NULL ) no_mem_exit("ercReset: errorVar->uCondition");
      errorVar->vCondition = (int *)malloc( nOfMBs*sizeof(int) );
      if ( errorVar->vCondition == NULL ) no_mem_exit("ercReset: errorVar->vCondition");
      errorVar->nOfMBs = nOfMBs;
    }
    else 
    {
      /* Store the yCondition struct of the previous frame */
      tmp = errorVar->prevFrameYCondition;
      errorVar->prevFrameYCondition = errorVar->yCondition;
      errorVar->yCondition = tmp;
    }
    
    /* Reset tables and parameters */
    memset( errorVar->yCondition, 0, 4*nOfMBs*sizeof(*errorVar->yCondition));
    memset( errorVar->uCondition, 0, nOfMBs*sizeof(*errorVar->uCondition));
    memset( errorVar->vCondition, 0, nOfMBs*sizeof(*errorVar->vCondition));
    
    if (errorVar->nOfSegments != numOfSegments) 
    {
      free( errorVar->segments );
      errorVar->segments = NULL;
      errorVar->segments = (ercSegment_t *)malloc( numOfSegments*sizeof(ercSegment_t) );
      if ( errorVar->segments == NULL ) no_mem_exit("ercReset: errorVar->segments");
      errorVar->nOfSegments = numOfSegments;
    }
    
    memset( errorVar->segments, 0, errorVar->nOfSegments*sizeof(ercSegment_t));

    for ( i = 0; i < errorVar->nOfSegments; i++ ) 
    {
      // mark all the Blocks as empty 
      for ( j = 0; j < nOfMBs; j++ ) 
      {
        errorVar->yCondition[MBNum2YBlock (j, 0, picSizeX)] = ERC_BLOCK_EMPTY;
        errorVar->yCondition[MBNum2YBlock (j, 1, picSizeX)] = ERC_BLOCK_EMPTY;
        errorVar->yCondition[MBNum2YBlock (j, 2, picSizeX)] = ERC_BLOCK_EMPTY;
        errorVar->yCondition[MBNum2YBlock (j, 3, picSizeX)] = ERC_BLOCK_EMPTY;
        errorVar->uCondition[j] = ERC_BLOCK_EMPTY;
        errorVar->vCondition[j] = ERC_BLOCK_EMPTY;
      }
      errorVar->segments[i].fCorrupted = 1; //! mark segments as corrupted
      errorVar->segments[i].startMBPos = 0;
      errorVar->segments[i].endMBPos = nOfMBs;
    }
    
    errorVar->currSegment = 0;
    errorVar->nOfCorruptedSegments = 0;
  }
}

/*!
 ************************************************************************
 * \brief
 *      Resets the variables used in error detection. 
 *      Should be called always when starting to decode a new frame.
 * \param errorVar
 *      Variables for error concealment
 ************************************************************************
 */
void ercClose( ercVariables_t *errorVar )
{
  if ( errorVar != NULL ) 
  {
    if (errorVar->yCondition != NULL) 
    {
      free( errorVar->segments );
      free( errorVar->yCondition );
      free( errorVar->uCondition );
      free( errorVar->vCondition );
      free( errorVar->prevFrameYCondition );
    }
    free( errorVar );
  }
  
  free(erc_object_list);
  
}

/*!
 ************************************************************************
 * \brief
 *      Sets error concealment ON/OFF. Can be invoked only between frames, not during a frame
 * \param errorVar
 *      Variables for error concealment
 * \param value
 *      New value
 ************************************************************************
 */
void ercSetErrorConcealment( ercVariables_t *errorVar, int value )
{
  if ( errorVar != NULL )
    errorVar->concealment = value;
}

/*!
 ************************************************************************
 * \brief
 *      Creates a new segment in the segment-list, and marks the start MB and bit position.
 *      If the end of the previous segment was not explicitly marked by "ercStopSegment",
 *      also marks the end of the previous segment.
 *      If needed, it reallocates the segment-list for a larger storage place.
 * \param currMBNum
 *      The MB number where the new slice/segment starts
 * \param segment
 *      Segment/Slice No. counted by the caller
 * \param bitPos
 *      Bitstream pointer: number of bits read from the buffer.
 * \param errorVar
 *      Variables for error detector
 ************************************************************************
 */
void ercStartSegment( int currMBNum, int segment, u_int32 bitPos, ercVariables_t *errorVar )
{
  if ( errorVar && errorVar->concealment ) 
  {
    errorVar->currSegmentCorrupted = 0;
        
    errorVar->segments[ segment ].fCorrupted = 0;
    errorVar->segments[ segment ].startMBPos = currMBNum;
    
  }   
}

/*!
 ************************************************************************
 * \brief
 *      Marks the end position of a segment.
 * \param currMBNum
 *      The last MB number of the previous segment
 * \param segment
 *      Segment/Slice No. counted by the caller
 *      If (segment<0) the internal segment counter is used.
 * \param bitPos
 *      Bitstream pointer: number of bits read from the buffer.
 * \param errorVar
 *      Variables for error detector
 ************************************************************************
 */
void ercStopSegment( int currMBNum, int segment, u_int32 bitPos, ercVariables_t *errorVar )
{
  if ( errorVar && errorVar->concealment ) 
  {
    errorVar->segments[ segment ].endMBPos = currMBNum; //! Changed TO 12.11.2001
    errorVar->currSegment++;
  }
}

/*!
 ************************************************************************
 * \brief
 *      Marks the current segment (the one which has the "currMBNum" MB in it)
 *      as lost: all the blocks of the MBs in the segment as corrupted.
 * \param currMBNum
 *      Selects the segment where this MB number is in.
 * \param picSizeX
 *      Width of the frame in pixels.
 * \param errorVar
 *      Variables for error detector
 ************************************************************************
 */
void ercMarkCurrSegmentLost(int32 picSizeX, ercVariables_t *errorVar )
{
  int i = 0, j = 0;
  int current_segment;
  
  current_segment = errorVar->currSegment-1;
  if ( errorVar && errorVar->concealment ) 
  {
    if (errorVar->currSegmentCorrupted == 0) 
    {
      errorVar->nOfCorruptedSegments++;
      errorVar->currSegmentCorrupted = 1;
    }
     
    for ( j = errorVar->segments[current_segment].startMBPos; j < errorVar->segments[current_segment].endMBPos; j++ ) 
    {
      errorVar->yCondition[MBNum2YBlock (j, 0, picSizeX)] = ERC_BLOCK_CORRUPTED;
      errorVar->yCondition[MBNum2YBlock (j, 1, picSizeX)] = ERC_BLOCK_CORRUPTED;
      errorVar->yCondition[MBNum2YBlock (j, 2, picSizeX)] = ERC_BLOCK_CORRUPTED;
      errorVar->yCondition[MBNum2YBlock (j, 3, picSizeX)] = ERC_BLOCK_CORRUPTED;
      errorVar->uCondition[j] = ERC_BLOCK_CORRUPTED;
      errorVar->vCondition[j] = ERC_BLOCK_CORRUPTED;
    }
    errorVar->segments[current_segment].fCorrupted = 1;
  }
}

/*!
 ************************************************************************
 * \brief
 *      Marks the current segment (the one which has the "currMBNum" MB in it)
 *      as OK: all the blocks of the MBs in the segment as OK.
 * \param currMBNum
 *      Selects the segment where this MB number is in.
 * \param picSizeX
 *      Width of the frame in pixels.
 * \param errorVar
 *      Variables for error detector
 ************************************************************************
 */
void ercMarkCurrSegmentOK(int32 picSizeX, ercVariables_t *errorVar )
{
  int i = 0, j = 0;
  int current_segment;
  
  current_segment = errorVar->currSegment-1;
  if ( errorVar && errorVar->concealment ) 
  {
    // mark all the Blocks belonging to the segment as OK */
    for ( j = errorVar->segments[current_segment].startMBPos; j < errorVar->segments[current_segment].endMBPos; j++ ) 
    {
      errorVar->yCondition[MBNum2YBlock (j, 0, picSizeX)] = ERC_BLOCK_OK;
      errorVar->yCondition[MBNum2YBlock (j, 1, picSizeX)] = ERC_BLOCK_OK;
      errorVar->yCondition[MBNum2YBlock (j, 2, picSizeX)] = ERC_BLOCK_OK;
      errorVar->yCondition[MBNum2YBlock (j, 3, picSizeX)] = ERC_BLOCK_OK;
      errorVar->uCondition[j] = ERC_BLOCK_OK;
      errorVar->vCondition[j] = ERC_BLOCK_OK;
    }
    errorVar->segments[current_segment].fCorrupted = 0;
  }
}

/*!
 ************************************************************************
 * \brief
 *      Marks the Blocks of the given component (YUV) of the current MB as concealed.
 * \param currMBNum
 *      Selects the segment where this MB number is in.
 * \param comp
 *      Component to mark (0:Y, 1:U, 2:V, <0:All)
 * \param picSizeX
 *      Width of the frame in pixels.
 * \param errorVar
 *      Variables for error detector
 ************************************************************************
 */
void ercMarkCurrMBConcealed( int currMBNum, int comp, int32 picSizeX, ercVariables_t *errorVar )
{
  int setAll = 0;
  
  if ( errorVar && errorVar->concealment ) 
  {
    if (comp < 0) 
    {
      setAll = 1;
      comp = 0;
    }
    
    switch (comp) 
    {
    case 0:
      errorVar->yCondition[MBNum2YBlock (currMBNum, 0, picSizeX)] = ERC_BLOCK_CONCEALED;
      errorVar->yCondition[MBNum2YBlock (currMBNum, 1, picSizeX)] = ERC_BLOCK_CONCEALED;
      errorVar->yCondition[MBNum2YBlock (currMBNum, 2, picSizeX)] = ERC_BLOCK_CONCEALED;
      errorVar->yCondition[MBNum2YBlock (currMBNum, 3, picSizeX)] = ERC_BLOCK_CONCEALED;
      if (!setAll)
        break;
    case 1:
      errorVar->uCondition[currMBNum] = ERC_BLOCK_CONCEALED;
      if (!setAll)
        break;
    case 2:
      errorVar->vCondition[currMBNum] = ERC_BLOCK_CONCEALED;
    }
  }
}
