/*!
 ************************************************************************
 * \file io_image.h
 *
 * \brief
 *    Image I/O 
 *
 * \author
 *     - Alexis Michael Tourapis         <alexismt@ieee.org>
 *
 ************************************************************************
 */

#ifndef _IO_IMAGE_H_
#define _IO_IMAGE_H_

#include "defines.h"
#include "frame.h"

typedef struct
{
  FrameFormat format;               //!< image format
  // Standard data
  imgpel **frm_data[MAX_PLANE];     //!< Frame Data
  imgpel **top_data[MAX_PLANE];     //!< pointers to top field data
  imgpel **bot_data[MAX_PLANE];     //!< pointers to bottom field data
  
  //! Optional data (could also add uint8 data in case imgpel is of type unsigned short)
  //! These can be useful for enabling input/conversion of content of different types
  //! while keeping optimal processing size.
  uint16 **frm_uint16[MAX_PLANE];   //!< optional frame Data for uint16
  uint16 **top_uint16[MAX_PLANE];   //!< optional pointers to top field data
  uint16 **bot_uint16[MAX_PLANE];   //!< optional pointers to bottom field data
} ImageData;

#endif

