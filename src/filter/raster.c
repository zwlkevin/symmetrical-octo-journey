/*
 * "$Id: raster.c,v 1.1.1.1 2010/03/09 01:57:19 liuzhiping Exp $"
 *
 *   Raster file routines for the Common UNIX Printing System (CUPS).
 *
 *   Copyright 1997-2006 by Easy Software Products.
 *
 *   This file is part of the CUPS Imaging library.
 *
 *   These coded instructions, statements, and computer programs are the
 *   property of Easy Software Products and are protected by Federal
 *   copyright law.  Distribution and use rights are outlined in the file
 *   "LICENSE.txt" which should have been included with this file.  If this
 *   file is missing or damaged please contact Easy Software Products
 *   at:
 *
 *       Attn: CUPS Licensing Information
 *       Easy Software Products
 *       44141 Airport View Drive, Suite 204
 *       Hollywood, Maryland 20636 USA
 *
 *       Voice: (301) 373-9600
 *       EMail: cups-info@cups.org
 *         WWW: http://www.cups.org
 *
 *   This file is subject to the Apple OS-Developed Software exception.
 *
 * Contents:
 *
 *   cupsRasterClose()         - Close a raster stream.
 *   cupsRasterOpen()          - Open a raster stream.
 *   cupsRasterReadHeader()    - Read a raster page header and store it in a
 *                               V1 page header structure.
 *   cupsRasterReadHeader2()   - Read a raster page header and store it in a
 *                               V2 page header structure.
 *   cupsRasterReadPixels()    - Read raster pixels.
 *   cupsRasterWriteHeader()   - Write a raster page header from a V1 page
 *                               header structure.
 *   cupsRasterWriteHeader2()  - Write a raster page header from a V2 page
 *                               header structure.
 *   cupsRasterWritePixels()   - Write raster pixels.
 *   cups_raster_read()        - Read through the raster buffer.
 *   cups_raster_read_header() - Read a raster page header.
 *   cups_raster_update()      - Update the raster header and row count for the
 *                               current page.
 *   cups_read()               - Read bytes from a file.
 *   cups_swap()               - Swap bytes in raster data...
 *   cups_write()              - Write bytes to a file.
 */

/*
 * Include necessary headers...
 */

#include "config.h"
#include "raster.h"
#include "common.h"
#include "debug.h"
//#include "cupsinc/debug.h"
#include <stdlib.h>
#include <errno.h>
//#include "cupsinc/string.h"

#if defined(WIN32) || defined(__EMX__)
#  include <io.h>
#else
#  include <unistd.h>
#endif /* WIN32 || __EMX__ */


/*
 * Private structures...
 */

struct _cups_raster_s			/**** Raster stream data ****/
{
  unsigned		sync;		/* Sync word from start of stream */
  int			fd;		/* File descriptor */
  cups_mode_t		mode;		/* Read/write mode */
  cups_page_header2_t	header;		/* Raster header for current page */
  int			count,		/* Current row run-length count */
			remaining,	/* Remaining rows in page image */
			bpp;		/* Bytes per pixel/color */
  unsigned char		*pixels,	/* Pixels for current row */
			*pend,		/* End of pixel buffer */
			*pcurrent;	/* Current byte in pixel buffer */
  int			compressed,	/* Non-zero if data is compressed */
			swapped;	/* Non-zero if data is byte-swapped */
  unsigned char		*buffer,	/* Read/write buffer */
			*bufptr,	/* Current (read) position in buffer */
			*bufend;	/* End of current (read) buffer */
  int			bufsize;	/* Buffer size */
};


/*
 * Local functions...
 */

static unsigned	cups_raster_read_header(cups_raster_t *r);
static int	cups_raster_read(cups_raster_t *r, unsigned char *buf,
		                 int bytes);
static void	cups_raster_update(cups_raster_t *r);
static int	cups_read(int fd, unsigned char *buf, int bytes);
static void	cups_swap(unsigned char *buf, int bytes);
static int	cups_write(int fd, const unsigned char *buf, int bytes);


/*
 * 'cupsRasterClose()' - Close a raster stream.
 */

void
cupsRasterClose(cups_raster_t *r)	/* I - Stream to close */
{
  if (r != NULL)
  {
    if (r->buffer)
      free(r->buffer);

    if (r->pixels)
      free(r->pixels);

    free(r);
  }
}


/*
 * 'cupsRasterOpen()' - Open a raster stream.
 */

cups_raster_t *				/* O - New stream */
cupsRasterOpen(int         fd,		/* I - File descriptor */
               cups_mode_t mode)	/* I - Mode */
{
  cups_raster_t	*r;			/* New stream */


  if ((r = calloc(sizeof(cups_raster_t), 1)) == NULL)
    return (NULL);

  r->fd   = fd;
  r->mode = mode;

  if (mode == CUPS_RASTER_READ)
  {
   /*
    * Open for read - get sync word...
    */

    if (!cups_read(r->fd, (unsigned char *)&(r->sync), sizeof(r->sync)))
    {
      free(r);
      return (NULL);
    }

    if (r->sync != CUPS_RASTER_SYNC &&
        r->sync != CUPS_RASTER_REVSYNC &&
        r->sync != CUPS_RASTER_SYNCv1 &&
        r->sync != CUPS_RASTER_REVSYNCv1 &&
        r->sync != CUPS_RASTER_SYNCv2 &&
        r->sync != CUPS_RASTER_REVSYNCv2)
    {
      free(r);
      return (NULL);
    }

    if (r->sync == CUPS_RASTER_SYNCv2 ||
        r->sync == CUPS_RASTER_REVSYNCv2)
      r->compressed = 1;

    if (r->sync == CUPS_RASTER_REVSYNC ||
        r->sync == CUPS_RASTER_REVSYNCv1 ||
        r->sync == CUPS_RASTER_REVSYNCv2)
      r->swapped = 1;
  }
  else
  {
   /*
    * Open for write - put sync word...
    */

    r->sync = CUPS_RASTER_SYNC;

    if (cups_write(r->fd, (unsigned char *)&(r->sync), sizeof(r->sync))
            < sizeof(r->sync))
    {
      free(r);
      return (NULL);
    }
  }

  return (r);
}


/*
 * 'cupsRasterReadHeader()' - Read a raster page header and store it in a
 *                            V1 page header structure.
 */

unsigned				/* O - 1 on success, 0 on fail */
cupsRasterReadHeader(
    cups_raster_t      *r,		/* I - Raster stream */
    cups_page_header_t *h)		/* I - Pointer to header data */
{
 /*
  * Get the raster header...
  */

  if (!cups_raster_read_header(r))
    return (0);
  
 /*
  * Copy the header to the user-supplied buffer...
  */

  memcpy(h, &(r->header), sizeof(cups_page_header_t));

  return (1);
}


/*
 * 'cupsRasterReadHeader2()' - Read a raster page header and store it in a
 *                             V2 page header structure.
 *
 * @since CUPS 1.2@
 */

unsigned				/* O - 1 on success, 0 on fail */
cupsRasterReadHeader2(
    cups_raster_t       *r,		/* I - Raster stream */
    cups_page_header2_t *h)		/* I - Pointer to header data */
{
 /*
  * Get the raster header...
  */

  if (!cups_raster_read_header(r))
    return (0);
  
 /*
  * Copy the header to the user-supplied buffer...
  */

  memcpy(h, &(r->header), sizeof(cups_page_header2_t));

  return (1);
}


/*
 * 'cupsRasterReadPixels()' - Read raster pixels.
 */

unsigned				/* O - Number of bytes read */
cupsRasterReadPixels(cups_raster_t *r,	/* I - Raster stream */
                     unsigned char *p,	/* I - Pointer to pixel buffer */
		     unsigned      len)	/* I - Number of bytes to read */
{
  int		bytes;			/* Bytes read */
  unsigned	cupsBytesPerLine;	/* cupsBytesPerLine value */
  unsigned	remaining;		/* Bytes remaining */
  unsigned char	*ptr,			/* Pointer to read buffer */
		byte,			/* Byte from file */
		*temp;			/* Pointer into buffer */
  int		count;			/* Repetition count */


  if (r == NULL || r->mode != CUPS_RASTER_READ || r->remaining == 0)
    return (0);

  if (!r->compressed)
  {
   /*
    * Read without compression...
    */

    r->remaining -= len / r->header.cupsBytesPerLine;

    if (!cups_read(r->fd, p, len))
      return (0);

   /*
    * Swap bytes as needed...
    */

    if ((r->header.cupsBitsPerColor == 16 ||
         r->header.cupsBitsPerPixel == 12 ||
         r->header.cupsBitsPerPixel == 16) &&
        r->swapped)
      cups_swap(p, len);

   /*
    * Return...
    */

    return (len);
  }

 /*
  * Read compressed data...
  */

  remaining        = len;
  cupsBytesPerLine = r->header.cupsBytesPerLine;

  while (remaining > 0 && r->remaining > 0)
  {
    if (r->count == 0)
    {
     /*
      * Need to read a new row...
      */

      if (remaining == cupsBytesPerLine)
	ptr = p;
      else
	ptr = r->pixels;

     /*
      * Read using a modified TIFF "packbits" compression...
      */

      if (!cups_raster_read(r, &byte, 1))
	return (0);

      r->count = byte + 1;

      if (r->count > 1)
	ptr = r->pixels;

      temp  = ptr;
      bytes = cupsBytesPerLine;

      while (bytes > 0)
      {
       /*
	* Get a new repeat count...
	*/

        if (!cups_raster_read(r, &byte, 1))
	  return (0);

	if (byte & 128)
	{
	 /*
	  * Copy N literal pixels...
	  */

	  count = (257 - byte) * r->bpp;

          if (count > bytes)
	    count = bytes;

          if (!cups_raster_read(r, temp, count))
	    return (0);

	  temp  += count;
	  bytes -= count;
	}
	else
	{
	 /*
	  * Repeat the next N bytes...
	  */

          count = (byte + 1) * r->bpp;
          if (count > bytes)
	    count = bytes;

          if (count < r->bpp)
	    break;

	  bytes -= count;

          if (!cups_raster_read(r, temp, r->bpp))
	    return (0);

	  temp  += r->bpp;
	  count -= r->bpp;

	  while (count > 0)
	  {
	    memcpy(temp, temp - r->bpp, r->bpp);
	    temp  += r->bpp;
	    count -= r->bpp;
          }
	}
      }

     /*
      * Swap bytes as needed...
      */

      if ((r->header.cupsBitsPerColor == 16 ||
           r->header.cupsBitsPerPixel == 12 ||
           r->header.cupsBitsPerPixel == 16) &&
          r->swapped)
        cups_swap(ptr, bytes);

     /*
      * Update pointers...
      */

      if (remaining >= cupsBytesPerLine)
      {
	bytes       = cupsBytesPerLine;
        r->pcurrent = r->pixels;
	r->count --;
	r->remaining --;
      }
      else
      {
	bytes       = remaining;
        r->pcurrent = r->pixels + bytes;
      }

     /*
      * Copy data as needed...
      */

      if (ptr != p)
        memcpy(p, ptr, bytes);
    }
    else
    {
     /*
      * Copy fragment from buffer...
      */

      if ((bytes = r->pend - r->pcurrent) > remaining)
        bytes = remaining;

      memcpy(p, r->pcurrent, bytes);
      r->pcurrent += bytes;

      if (r->pcurrent >= r->pend)
      {
        r->pcurrent = r->pixels;
	r->count --;
	r->remaining --;
      }
    }

    remaining -= bytes;
    p         += bytes;
  }

  return (len);
}


/*
 * 'cupsRasterWriteHeader()' - Write a raster page header from a V1 page
 *                             header structure.
 */
 
unsigned				/* O - 1 on success, 0 on failure */
cupsRasterWriteHeader(
    cups_raster_t      *r,		/* I - Raster stream */
    cups_page_header_t *h)		/* I - Raster page header */
{
  if (r == NULL || r->mode != CUPS_RASTER_WRITE)
    return (0);

 /*
  * Make a copy of the header, and compute the number of raster
  * lines in the page image...
  */

  memset(&(r->header), 0, sizeof(r->header));
  memcpy(&(r->header), h, sizeof(cups_page_header_t));

  cups_raster_update(r);

 /*
  * Write the raster header...
  */

  return (cups_write(r->fd, (unsigned char *)&(r->header), sizeof(r->header))
              > 0);
}


/*
 * 'cupsRasterWriteHeader2()' - Write a raster page header from a V2 page
 *                              header structure.
 *
 * @since CUPS 1.2@
 */
 
unsigned				/* O - 1 on success, 0 on failure */
cupsRasterWriteHeader2(
    cups_raster_t       *r,		/* I - Raster stream */
    cups_page_header2_t *h)		/* I - Raster page header */
{
  if (r == NULL || r->mode != CUPS_RASTER_WRITE)
    return (0);

 /*
  * Make a copy of the header, and compute the number of raster
  * lines in the page image...
  */

  memcpy(&(r->header), h, sizeof(cups_page_header2_t));

  cups_raster_update(r);

 /*
  * Write the raster header...
  */

  return (cups_write(r->fd, (unsigned char *)&(r->header), sizeof(r->header))
              > 0);
}


/*
 * 'cupsRasterWritePixels()' - Write raster pixels.
 */

unsigned				/* O - Number of bytes written */
cupsRasterWritePixels(cups_raster_t *r,	/* I - Raster stream */
                      unsigned char *p,	/* I - Bytes to write */
		      unsigned      len)/* I - Number of bytes to write */
{
#ifdef DEBUG
  fprintf(stderr, "cupsRasterWritePixels(r=%p, p=%p, len=%u), remaining=%u\n",
          r, p, len, r->remaining);
#endif /* DEBUG */

  if (r == NULL || r->mode != CUPS_RASTER_WRITE || r->remaining == 0)
    return (0);

 /*
  * No write compression, just write the raster data raw...
  */

  r->remaining -= len / r->header.cupsBytesPerLine;

  return (cups_write(r->fd, p, len));
}


/*
 * 'cups_raster_read_header()' - Read a raster page header.
 */

static unsigned				/* O - 1 on success, 0 on fail */
cups_raster_read_header(
    cups_raster_t *r)			/* I - Raster stream */
{
  int		len;			/* Number of words to swap */
  union swap_s				/* Swapping structure */
  {
    unsigned char	b[4];
    unsigned		v;
  }		*s;


  if (r == NULL || r->mode != CUPS_RASTER_READ)
    return (0);

 /*
  * Get the length of the raster header...
  */

  if (r->sync == CUPS_RASTER_SYNCv1 || r->sync == CUPS_RASTER_REVSYNCv1)
    len = sizeof(cups_page_header_t);
  else
    len = sizeof(cups_page_header2_t);

 /*
  * Read the header...
  */

  memset(&(r->header), 0, sizeof(r->header));

  if (cups_raster_read(r, (unsigned char *)&(r->header), len) < len)
    return (0);

 /*
  * Swap bytes as needed...
  */

  if (r->swapped)
    for (len = 81, s = (union swap_s *)&(r->header.AdvanceDistance);
	 len > 0;
	 len --, s ++)
      s->v = (((((s->b[3] << 8) | s->b[2]) << 8) | s->b[1]) << 8) | s->b[0];

 /*
  * Update the header and row count...
  */

  cups_raster_update(r);

  return (1);
}


/*
 * 'cups_raster_read()' - Read through the raster buffer.
 */

static int				/* O - Number of bytes read */
cups_raster_read(cups_raster_t *r,	/* I - Raster stream */
                 unsigned char *buf,	/* I - Buffer */
                 int           bytes)	/* I - Number of bytes to read */
{
  int		count,			/* Number of bytes read */
		remaining,		/* Remaining bytes in buffer */
		total;			/* Total bytes read */


//  DEBUG_printf(("cups_raster_read(r=%p, buf=%p, bytes=%d)\n", r, buf, bytes));

  if (!r->compressed)
    return (cups_read(r->fd, buf, bytes));

 /*
  * Allocate a read buffer as needed...
  */

  count = 2 * r->header.cupsBytesPerLine;

  if (count > r->bufsize)
  {
    int offset = r->bufptr - r->buffer;	/* Offset to current start of buffer */
    int end = r->bufend - r->buffer;	/* Offset to current end of buffer */
    unsigned char *rptr;		/* Pointer in read buffer */

    if (r->buffer)
      rptr = realloc(r->buffer, count);
    else
      rptr = malloc(count);

    if (!rptr)
      return (0);

    r->buffer  = rptr;
    r->bufptr  = rptr + offset;
    r->bufend  = rptr + end;
    r->bufsize = count;
  }

 /*
  * Loop until we have read everything...
  */

  for (total = 0, remaining = r->bufend - r->bufptr;
       total < bytes;
       total += count, buf += count)
  {
    count = bytes - total;

//    DEBUG_printf(("count=%d, remaining=%d, buf=%p, bufptr=%p, bufend=%p...\n",
//                  count, remaining, buf, r->bufptr, r->bufend));

    if (remaining == 0)
    {
      if (count < 16)
      {
       /*
        * Read into the raster buffer and then copy...
	*/

        remaining = cups_read(r->fd, r->buffer, r->bufsize);
	if (remaining <= 0)
	  return (0);

	r->bufptr = r->buffer;
	r->bufend = r->buffer + remaining;
      }
      else
      {
       /*
        * Read directly into "buf"...
	*/

	count = cups_read(r->fd, buf, count);

	if (count <= 0)
	  return (0);

	continue;
      }
    }

   /*
    * Copy bytes from raster buffer to "buf"...
    */

    if (count > remaining)
      count = remaining;

    if (count == 1)
    {
     /*
      * Copy 1 byte...
      */

      *buf = *(r->bufptr)++;
      remaining --;
    }
    else if (count < 128)
    {
     /*
      * Copy up to 127 bytes without using memcpy(); this is
      * faster because it avoids an extra function call and is
      * often further optimized by the compiler...
      */

      unsigned char	*bufptr;	/* Temporary buffer pointer */


      remaining -= count;

      for (bufptr = r->bufptr; count > 0; count --, total ++)
	*buf++ = *bufptr++;

      r->bufptr = bufptr;
    }
    else
    {
     /*
      * Use memcpy() for a large read...
      */

      memcpy(buf, r->bufptr, count);
      r->bufptr += count;
      remaining -= count;
    }
  }

  return (total);
}


/*
 * 'cups_raster_update()' - Update the raster header and row count for the
 *                          current page.
 */

static void
cups_raster_update(cups_raster_t *r)	/* I - Raster stream */
{
  if (r->sync == CUPS_RASTER_SYNCv1 || r->sync == CUPS_RASTER_REVSYNCv1 ||
      r->header.cupsNumColors == 0)
  {
   /*
    * Set the "cupsNumColors" field according to the colorspace...
    */

    switch (r->header.cupsColorSpace)
    {
      case CUPS_CSPACE_W :
      case CUPS_CSPACE_K :
      case CUPS_CSPACE_WHITE :
      case CUPS_CSPACE_GOLD :
      case CUPS_CSPACE_SILVER :
          r->header.cupsNumColors = 1;
	  break;

      case CUPS_CSPACE_RGB :
      case CUPS_CSPACE_CMY :
      case CUPS_CSPACE_YMC :
      case CUPS_CSPACE_CIEXYZ :
      case CUPS_CSPACE_CIELab :
      case CUPS_CSPACE_ICC1 :
      case CUPS_CSPACE_ICC2 :
      case CUPS_CSPACE_ICC3 :
      case CUPS_CSPACE_ICC4 :
      case CUPS_CSPACE_ICC5 :
      case CUPS_CSPACE_ICC6 :
      case CUPS_CSPACE_ICC7 :
      case CUPS_CSPACE_ICC8 :
      case CUPS_CSPACE_ICC9 :
      case CUPS_CSPACE_ICCA :
      case CUPS_CSPACE_ICCB :
      case CUPS_CSPACE_ICCC :
      case CUPS_CSPACE_ICCD :
      case CUPS_CSPACE_ICCE :
      case CUPS_CSPACE_ICCF :
          r->header.cupsNumColors = 3;
	  break;

      case CUPS_CSPACE_RGBA :
      case CUPS_CSPACE_RGBW :
      case CUPS_CSPACE_CMYK :
      case CUPS_CSPACE_YMCK :
      case CUPS_CSPACE_KCMY :
      case CUPS_CSPACE_GMCK :
      case CUPS_CSPACE_GMCS :
          r->header.cupsNumColors = 4;
	  break;

      case CUPS_CSPACE_KCMYcm :
          if (r->header.cupsBitsPerPixel < 8)
            r->header.cupsNumColors = 6;
	  else
            r->header.cupsNumColors = 4;
	  break;
    }
  }

 /*
  * Set the number of bytes per pixel/color...
  */

  if (r->header.cupsColorOrder == CUPS_ORDER_CHUNKED)
    r->bpp = (r->header.cupsBitsPerPixel + 7) / 8;
  else
    r->bpp = (r->header.cupsBitsPerColor + 7) / 8;

 /*
  * Set the number of remaining rows...
  */

  if (r->header.cupsColorOrder == CUPS_ORDER_PLANAR)
    r->remaining = r->header.cupsHeight * r->header.cupsNumColors;
  else
    r->remaining = r->header.cupsHeight;

 /*
  * Allocate the compression buffer...
  */

  if (r->compressed)
  {
    if (r->pixels != NULL)
      free(r->pixels);

    r->pixels   = calloc(r->header.cupsBytesPerLine, 1);
    r->pcurrent = r->pixels;
    r->pend     = r->pixels + r->header.cupsBytesPerLine;
    r->count    = 0;
  }
}


/*
 * 'cups_read()' - Read bytes from a file.
 */

static int				/* O - Bytes read or -1 */
cups_read(int           fd,		/* I - File descriptor */
          unsigned char *buf,		/* I - Buffer for read */
	  int           bytes)		/* I - Number of bytes to read */
{
  int	count,				/* Number of bytes read */
	total;				/* Total bytes read */


  for (total = 0; total < bytes; total += count, buf += count)
  {
    count = read(fd, buf, bytes - total);

    if (count == 0)
      return (0);
    else if (count < 0)
    {
      if (errno == EINTR)
        count = 0;
      else
        return (-1);
    }
  }

  return (total);
}


/*
 * 'cups_swap()' - Swap bytes in raster data...
 */

static void
cups_swap(unsigned char *buf,		/* I - Buffer to swap */
          int           bytes)		/* I - Number of bytes to swap */
{
  unsigned char	even, odd;		/* Temporary variables */


  bytes /= 2;

  while (bytes > 0)
  {
    even   = buf[0];
    odd    = buf[1];
    buf[0] = odd;
    buf[1] = even;

    buf += 2;
    bytes --;
  }
}


/*
 * 'cups_write()' - Write bytes to a file.
 */

static int				/* O - Bytes written or -1 */
cups_write(int                 fd,	/* I - File descriptor */
           const unsigned char *buf,	/* I - Bytes to write */
	   int                 bytes)	/* I - Number of bytes to write */
{
  int	count,				/* Number of bytes written */
	total;				/* Total bytes written */


  for (total = 0; total < bytes; total += count, buf += count)
  {
    count = write(fd, buf, bytes - total);

    if (count < 0)
    {
      if (errno == EINTR)
        count = 0;
      else
        return (-1);
    }
  }

  return (total);
}


/*
 * End of "$Id: raster.c,v 1.1.1.1 2010/03/09 01:57:19 liuzhiping Exp $".
 */
