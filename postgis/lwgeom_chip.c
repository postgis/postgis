#include <math.h>
#include <float.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>

#include "postgres.h"
#include "access/gist.h"
#include "access/itup.h"
#include "fmgr.h"
#include "utils/elog.h"
#include "funcapi.h"

#include "lwgeom_pg.h"
#include "liblwgeom.h"


/* Define this to debug CHIP ops */
/*#define DEBUG_CHIP 1*/

typedef unsigned short int UINT16;
typedef float FLOAT32;

typedef struct PIXEL_T {
	int type; /* 1=float32, 5=int24, 6=int16 */
	uchar val[4];
} PIXEL;

typedef struct RGB_T {
	uchar red;
	uchar green;
	uchar blue;
} RGB;


/* Internal funcs */
void swap_char(char *a,char *b);
void flip_endian_double(char *d);
void flip_endian_int32(char *i);
const char* pixelOpName(int op);
const char* pixelHEX(PIXEL* p);
UINT16 pixel_readUINT16(PIXEL *p);
void pixel_writeUINT16(PIXEL *p, UINT16 i);
PIXEL pixel_readval(char *buf);
void pixel_writeval(PIXEL *p, char *buf, size_t maxlen);
void pixel_add_float32(PIXEL *where, PIXEL *what);
void pixel_add_int24(PIXEL *where, PIXEL *what);
void pixel_add_int16(PIXEL *where, PIXEL *what);
void pixel_add(PIXEL *where, PIXEL *what);
size_t chip_xy_off(CHIP *c, size_t x, size_t y);
void chip_setPixel(CHIP *c, int x, int y, PIXEL *p);
PIXEL chip_getPixel(CHIP *c, int x, int y);
void chip_draw_pixel(CHIP *chip, int x, int y, PIXEL *pixel, int op);
void chip_draw_segment(CHIP *chip, int x1, int y1, int x2, int y2, PIXEL *pixel, int op);
void chip_fill(CHIP *chip, PIXEL *pixel, int op);
CHIP * pgchip_construct(BOX3D *bvol, int SRID, int width, int height, int datatype, PIXEL *initvalue);
void chip_draw_ptarray(CHIP *chip, POINTARRAY *pa, PIXEL *pixel, int op);
void chip_draw_lwpoint(CHIP *chip, LWPOINT *lwpoint, PIXEL* pixel, int op);
void chip_draw_lwline(CHIP *chip, LWLINE *lwline, PIXEL* pixel, int op);
void chip_draw_lwgeom(CHIP *chip, LWGEOM *lwgeom, PIXEL *pixel, int op);
char * text_to_cstring(text *t);


/* Prototypes */
Datum CHIP_in(PG_FUNCTION_ARGS);
Datum CHIP_out(PG_FUNCTION_ARGS);
Datum CHIP_to_LWGEOM(PG_FUNCTION_ARGS);
Datum CHIP_getSRID(PG_FUNCTION_ARGS);
Datum CHIP_getFactor(PG_FUNCTION_ARGS);
Datum CHIP_setFactor(PG_FUNCTION_ARGS);
Datum CHIP_getDatatype(PG_FUNCTION_ARGS);
Datum CHIP_getCompression(PG_FUNCTION_ARGS);
Datum CHIP_getHeight(PG_FUNCTION_ARGS);
Datum CHIP_getWidth(PG_FUNCTION_ARGS);
Datum CHIP_setSRID(PG_FUNCTION_ARGS);
Datum CHIP_send(PG_FUNCTION_ARGS);
Datum CHIP_dump(PG_FUNCTION_ARGS);
Datum CHIP_construct(PG_FUNCTION_ARGS);
Datum CHIP_getpixel(PG_FUNCTION_ARGS);
Datum CHIP_setpixel(PG_FUNCTION_ARGS);
Datum CHIP_draw(PG_FUNCTION_ARGS);
Datum CHIP_fill(PG_FUNCTION_ARGS);


/*
 * Input is a string with hex chars in it. 
 * Convert to binary and put in the result
 */
PG_FUNCTION_INFO_V1(CHIP_in);
Datum CHIP_in(PG_FUNCTION_ARGS)
{
	char *str = PG_GETARG_CSTRING(0);
	CHIP *result;
	int size;
	int t;
	int input_str_len;
	int datum_size;

/*printf("chip_in called\n"); */

	input_str_len = strlen(str);

	if  ( ( ( (int)(input_str_len/2.0) ) *2.0) != input_str_len)
	{
		elog(ERROR,"CHIP_in parser - should be even number of characters!");
		PG_RETURN_NULL();
	}

 	if (strspn(str,"0123456789ABCDEF") != strlen(str) )
	{
		elog(ERROR,"CHIP_in parser - input contains bad characters.  Should only have '0123456789ABCDEF'!");
		PG_RETURN_NULL();	
	}
	size = (input_str_len/2) ;
	result = (CHIP *) palloc(size);
	
	for (t=0;t<size;t++)
	{
		((uchar *)result)[t] = parse_hex( &str[t*2]) ;
	}
/* if endian is wrong, flip it otherwise do nothing */
	result->size = size;
	if (result->size < sizeof(CHIP)-sizeof(void*) )
	{
		elog(ERROR,"CHIP_in parser - bad data (too small to be a CHIP)!");
		PG_RETURN_NULL();	
	}

	if (result->endian_hint != 1)
	{
		/*need to do an endian flip */
		flip_endian_int32( (char *)   &result->endian_hint);

		flip_endian_double((char *)  &result->bvol.xmin);
		flip_endian_double((char *)  &result->bvol.ymin);
		flip_endian_double((char *)  &result->bvol.zmin);

		flip_endian_double((char *)  &result->bvol.xmax);
		flip_endian_double((char *)  &result->bvol.ymax);
		flip_endian_double((char *)  &result->bvol.zmax);

		flip_endian_int32( (char *)  & result->SRID);	
			/*dont know what to do with future[8] ... */

		flip_endian_int32( (char *)  & result->height);	
		flip_endian_int32( (char *)  & result->width);
		flip_endian_int32( (char *)  & result->compression);
		flip_endian_int32( (char *)  & result->factor);
		flip_endian_int32( (char *)  & result->datatype);

	}
	if (result->endian_hint != 1 )
	{
		elog(ERROR,"CHIP_in parser - bad data (endian flag != 1)!");
		PG_RETURN_NULL();	
	}
	datum_size = 4;

	if ( (result->datatype == 6) || (result->datatype == 7) || (result->datatype == 106) || (result->datatype == 107) )
	{
		datum_size = 2;
	}
	if ( (result->datatype == 8) || (result->datatype == 108) )
	{
		datum_size=1;
	}

	if (result->compression ==0) /*only true for non-compressed data */
	{
		if (result->size != (sizeof(CHIP) - sizeof(void*) + datum_size * result->width*result->height) )
		{
			elog(ERROR,"CHIP_in parser - bad data (actual size [%d] != computed size [%ld])!", result->size, (sizeof(CHIP)-sizeof(void*) + datum_size * result->width*result->height) );
			PG_RETURN_NULL();
		}
	}

	PG_RETURN_POINTER(result);
}


/*given a CHIP structure, convert it to Hex and put it in a string */
PG_FUNCTION_INFO_V1(CHIP_out);
Datum CHIP_out(PG_FUNCTION_ARGS)
{
	CHIP *chip = (CHIP *)  PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
	char *result;
	int size_result;
	int t;

#if DEBUG_CHIP
	lwnotice("CHIP_out: %dx%d chip, %d in size", chip->width, chip->height, chip->size);
#endif

	size_result = (chip->size ) *2 +1; /* +1 for null char */
	result = palloc (size_result);
	result[size_result-1] = 0; /*null terminate */

	for (t=0; t< (chip->size); t++)
	{
		deparse_hex( ((uchar *) chip)[t], &result[t*2]);
	}
	PG_RETURN_CSTRING(result);	
}


/*given a CHIP structure pipe it out as raw binary */
PG_FUNCTION_INFO_V1(CHIP_send);
Datum CHIP_send(PG_FUNCTION_ARGS)
{
	CHIP *chip = (CHIP *)  PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
	bytea *result;



	result = (bytea *)palloc( chip->size );
	/*SET_VARSIZE(result, chip->size + VARHDRSZ);*/
	/*memcpy( VARDATA(result), chip, chip->size ); */
	memcpy( result, chip, chip->size );
	PG_RETURN_POINTER(result);	
	/*PG_RETURN_POINTER( (bytea *)PG_DETOAST_DATUM(PG_GETARG_DATUM(0)) );*/
}


PG_FUNCTION_INFO_V1(CHIP_to_LWGEOM);
Datum CHIP_to_LWGEOM(PG_FUNCTION_ARGS)
{
	CHIP *chip = (CHIP *)  PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
	PG_LWGEOM *result;
	POINT2D *pts = palloc(sizeof(POINT2D)*5);
	POINTARRAY *pa[1];
	LWPOLY *poly;
	uchar *ser;
	int wantbbox = false;
	
	/* Assign coordinates to POINT2D array */
	pts[0].x = chip->bvol.xmin; pts[0].y = chip->bvol.ymin;
	pts[1].x = chip->bvol.xmin; pts[1].y = chip->bvol.ymax;
	pts[2].x = chip->bvol.xmax; pts[2].y = chip->bvol.ymax;
	pts[3].x = chip->bvol.xmax; pts[3].y = chip->bvol.ymin;
	pts[4].x = chip->bvol.xmin; pts[4].y = chip->bvol.ymin;
	
	/* Construct point array */
	pa[0] = palloc(sizeof(POINTARRAY));
	pa[0]->serialized_pointlist = (uchar *)pts;
	TYPE_SETZM(pa[0]->dims, 0, 0);
	pa[0]->npoints = 5;

	/* Construct polygon */
	poly = lwpoly_construct(chip->SRID, NULL, 1, pa);

	/* Serialize polygon */
	ser = lwpoly_serialize(poly);

	/* Construct PG_LWGEOM  */
	result = PG_LWGEOM_construct(ser, chip->SRID, wantbbox);
	
	PG_RETURN_POINTER(result);

}

PG_FUNCTION_INFO_V1(CHIP_getSRID);
Datum CHIP_getSRID(PG_FUNCTION_ARGS)
{ 
	CHIP *c = (CHIP *)  PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
	PG_RETURN_INT32(c->SRID);
}

PG_FUNCTION_INFO_V1(CHIP_getFactor);
Datum CHIP_getFactor(PG_FUNCTION_ARGS)
{ 
	CHIP *c = (CHIP *)  PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
	PG_RETURN_FLOAT4(c->factor);
}

PG_FUNCTION_INFO_V1(CHIP_setFactor);
Datum CHIP_setFactor(PG_FUNCTION_ARGS)
{ 
	CHIP 	*c = (CHIP *)  PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
	float	factor = PG_GETARG_FLOAT4(1);
	CHIP  *result;

	result = (CHIP *) palloc(c->size);

	memcpy(result,c,c->size);
	result->factor = factor;

	PG_RETURN_POINTER(result);
}



PG_FUNCTION_INFO_V1(CHIP_getDatatype);
Datum CHIP_getDatatype(PG_FUNCTION_ARGS)
{ 
	CHIP *c = (CHIP *)  PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
	PG_RETURN_INT32(c->datatype);
}

PG_FUNCTION_INFO_V1(CHIP_getCompression);
Datum CHIP_getCompression(PG_FUNCTION_ARGS)
{ 
	CHIP *c = (CHIP *)  PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
	PG_RETURN_INT32(c->compression);
}

PG_FUNCTION_INFO_V1(CHIP_getHeight);
Datum CHIP_getHeight(PG_FUNCTION_ARGS)
{ 
	CHIP *c = (CHIP *)  PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
	PG_RETURN_INT32(c->height);
}

PG_FUNCTION_INFO_V1(CHIP_getWidth);
Datum CHIP_getWidth(PG_FUNCTION_ARGS)
{ 
	CHIP *c = (CHIP *)  PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
	PG_RETURN_INT32(c->width);
}


PG_FUNCTION_INFO_V1(CHIP_setSRID);
Datum CHIP_setSRID(PG_FUNCTION_ARGS)
{ 
	CHIP *c = (CHIP *)  PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
	int32 new_srid = PG_GETARG_INT32(1);
	CHIP *result;

	result = (CHIP *) palloc(c->size);

	memcpy(result, c, c->size);
	result->SRID = new_srid;

	PG_RETURN_POINTER(result);
}

void	flip_endian_double(char 	*d)
{
	swap_char(d+7, d);
	swap_char(d+6, d+1);
	swap_char(d+5, d+2);
	swap_char(d+4, d+3);
}

void swap_char(char	*a,char *b)
{
	char c;

	c = *a;
	*a=*b;
	*b=c;
}

void		flip_endian_int32(char		*i)
{
	swap_char (i+3,i);
	swap_char (i+2,i+1);
}

/********************************************************************
 *
 * NEW chip code
 * 
 ********************************************************************/

#define CHIP_BOUNDS_CHECKS 1


/* Macros to find size (in geographic units) of a CHIP cell (or pixel) */
#define CHIP_CELL_WIDTH(x) ( ((x)->bvol.xmax-(x)->bvol.xmin)/(x)->width )
#define CHIP_CELL_HEIGHT(x) ( ((x)->bvol.ymax-(x)->bvol.ymin)/(x)->height )

/* Return the size of values for pixel of the given datatype */
size_t chip_pixel_value_size(int datatype);

/********************************************************************
 *
 * IO primitives
 * 
 ********************************************************************/

#define PIXELOP_OVERWRITE 1
#define PIXELOP_ADD 2
#define PIXELOP_MULTIPLY 3

char *pixelop_name[] = {
	"Unknown",
	"Overwrite",
	"Add",
	"Multiply"
};

const char*
pixelOpName(int op)
{
	if ( op < 0 || op > 3 )
	{
		return "Invalid";
	}
	return pixelop_name[op];
}

const char*
pixelHEX(PIXEL* p)
{
	static char buf[256];
	size_t ps = chip_pixel_value_size(p->type);
	int i,j;
	static const char* hex = "0123456789ABCDEF";

	for (i=0, j=0; i<ps; ++i)
	{
		uchar val = p->val[i];
		int upper = (val & 0xF0) >> 4;
		int lower = val & 0x0F;

		buf[j++] = hex[ upper ];
		buf[j++] = hex[ lower ];
	}
	buf[j]=0;

	return buf;
}

UINT16
pixel_readUINT16(PIXEL *p)
{
	UINT16 i=0;
	memcpy(&i, &(p->val), 2);
	//i = p->val[0] | (p->val[1] << 8);
	return i;
}

void
pixel_writeUINT16(PIXEL *p, UINT16 i)
{
	//memcpy(&(p->val), &i, 2);
	p->val[0] = i&0xFF;
	p->val[1] = i>>8;
	p->val[2] = 0;
	p->val[3] = 0;
}

/*
 * Write text representation of a PIXEL value into provided
 * buffer.
 */
void
pixel_writeval(PIXEL *p, char *buf, size_t maxlen)
{
	FLOAT32 f32=0.0;
	UINT16 i16=0;

	switch (p->type)
	{
		case 1: /* float32 */
			memcpy(&f32, p->val, 4);
			sprintf(buf, "%g", f32);
			break;
		case 5: /* int24 (rgb) */
			buf[0] = '#';
			deparse_hex(p->val[0], &buf[1]);
			deparse_hex(p->val[1], &buf[3]);
			deparse_hex(p->val[2], &buf[5]);
			buf[7]='\0';
			break;
		case 6: /* int16 */
			i16=pixel_readUINT16(p);
			snprintf(buf, maxlen, "%u", i16);
			break;
		default:
			lwerror("Unsupported PIXEL value %d", p->type);
	}
}

PIXEL
pixel_readval(char *buf)
{
	FLOAT32 f32=0.0;
	long int i16=0;
	RGB rgb;
	char *ptr;
	PIXEL p;

	/* RGB */
	if ( buf[0] == '#' )
	{
		if ( strlen(buf) < 7 ) lwerror("RGB value too short");
		p.type = 5; /* rgb */
		ptr=buf+1;
		rgb.red = parse_hex(ptr);
		ptr+=2;
		rgb.green = parse_hex(ptr);
		ptr+=2;
		rgb.blue = parse_hex(ptr);
		memcpy(p.val, &rgb, 3);

		return p;
	}

	/* float32 */
	if ( strchr(buf, '.') )
	{
		f32 = strtod(buf, &ptr);
		if ( ptr != buf+strlen(buf) )
			lwerror("Malformed float value");
		p.type = 1; /* float32 */
		memcpy(p.val, &f32, 4);

		return p;
	}

	/* int16 */
	i16 = strtol(buf, &ptr, 0);
	if ( ptr != buf+strlen(buf) ) lwerror("Malformed integer value");
	if ( i16 > 65535 ) lwerror("Integer too high for an int16");
	p.type = 6; /* int16 */
	memcpy(p.val, &i16, 2);
	p.val[2]=0;
	p.val[3]=0;
	return p;
}

void
pixel_add_float32(PIXEL *where, PIXEL *what)
{
	FLOAT32 f1=0.0, f2=0.0;
	memcpy(&f1, where->val, 4);
	memcpy(&f2, what->val, 4);
	f1 += f2;
	memcpy(where->val, &f1, 4);
}

void
pixel_add_int24(PIXEL *where, PIXEL *what)
{
	unsigned int red, green, blue;
	RGB rgb1, rgb2;

	memcpy(&rgb1, where->val, 3);
	memcpy(&rgb2, what->val, 3);
	red = rgb1.red + rgb2.red;
	green = rgb1.green + rgb2.green;
	blue = rgb1.blue + rgb2.blue;
	if ( red > 255 )
	{
		lwnotice("Red channel saturated by add operation");
		red = 255;
	}
	if ( green > 255 )
	{
		lwnotice("Green channel saturated by add operation");
		green = 255;
	}
	if ( blue > 255 )
	{
		lwnotice("Blue channel saturated by add operation");
		blue = 255;
	}
	rgb1.red = red;
	rgb1.green = green;
	rgb1.blue = blue;
	memcpy(where->val, &rgb1, 3);
}

void
pixel_add_int16(PIXEL *where, PIXEL *what)
{
	UINT16 i1, i2;
	unsigned long int tmp;

	i1 = pixel_readUINT16(where);
	i2 = pixel_readUINT16(what);
	tmp = (unsigned long)i1 + i2;
	if ( tmp > 65535 )
	{
		lwnotice("UInt16 Pixel saturated by add operation (%u+%u=%u)",
			i1, i2, tmp);
		tmp = 65535;
	}
	i1 = tmp;
	pixel_writeUINT16(where, i1);
}

void
pixel_add(PIXEL *where, PIXEL *what)
{
	if ( where->type != what->type )
		lwerror("Can't add pixels of different types");

	switch (where->type)
	{
		case 1: /*float32*/
			pixel_add_float32(where, what);
			break;

		case 5: /*int24*/
			pixel_add_int24(where, what);
			break;

		case 6: /*int16*/
			pixel_add_int16(where, what);
			break;

		default:
			lwerror("pixel_add: unkown pixel type %d",
				where->type);
	}

}

/*
 * Return size of pixel values in bytes
 */
size_t
chip_pixel_value_size(int datatype)
{
	switch(datatype)
	{
		case 1:
		case 101:
			return 4;
		case 5:
		case 105:
			return 3;
		case 6:
		case 7:
		case 106:
		case 107:
			return 2;
		case 8:
		case 108:
			return 1;
		default:
			lwerror("Unknown CHIP datatype: %d", datatype);
			return 0;
	}
}

/*
 * Returns offset of pixel at X,Y
 */
#define CHIP_XY_OFF(c, x, y) ((x)+((y)*(c)->width))
size_t
chip_xy_off(CHIP *c, size_t x, size_t y)
{
#ifdef CHIP_BOUNDS_CHECKS
	if ( x < 0 || x >= c->width || y < 0 || y >= c->height )
	{
		lwerror("Coordinates ouf of range");
		return 0;
	}
#endif
	return x+(y*c->width);
}

/*
 * Set pixel value.
 *
 * CHIP is assumed to be:
 *	- uncompressed
 *	- in machine's byte order
 */
void
chip_setPixel(CHIP *c, int x, int y, PIXEL *p)
{
	void *where;
	size_t ps;
	size_t off;

#if DEBUG_CHIP
	lwnotice("chip_setPixel([CHIP %p], %d, %d) called", c, x, y);
#endif
	if ( c->datatype != p->type ) lwerror("Pixel datatype mismatch");

	ps = chip_pixel_value_size(c->datatype);
	off = chip_xy_off(c, x, y)*ps;
	if ( off > c->size + sizeof(CHIP) )
	{
		lwerror("Pixel offset out of CHIP size bounds");
	}

	where = ((char*)&(c->data))+off;

#if DEBUG_CHIP
	lwnotice("Writing %d bytes (%s) at offset %d (%p)",
		ps, pixelHEX(p), off, where);
#endif

	memcpy(where, &(p->val), ps);
}

/*
 * Get pixel value.
 *
 * CHIP is assumed to be:
 *	- uncompressed
 *	- in machine's byte order
 */
PIXEL
chip_getPixel(CHIP *c, int x, int y)
{
	PIXEL p;
	size_t ps = chip_pixel_value_size(c->datatype);
	void *where = ((char*)&(c->data))+chip_xy_off(c, x, y)*ps;
	p.type = c->datatype;
	memset(p.val, '\0', 4);
	memcpy(p.val, where, ps);

	return p;
}

/********************************************************************
 *
 * Drawing primitives
 * 
 ********************************************************************/

/*
 * Coordinates are in CHIP offset, so callers must
 * appropriately translate.
 */
void
chip_draw_pixel(CHIP *chip, int x, int y, PIXEL *pixel, int op)
{

#if DEBUG_CHIP
	lwnotice("chip_draw_pixel([CHIP %p], %d, %d, [PIXEL %p], %s) called",
		(void*)chip, x, y, (void*)pixel, pixelOpName(op));
#endif

	if ( x < 0 || x >= chip->width || y < 0 || y >= chip->height )
	{
/* should this be a warning ? */
lwnotice("chip_draw_pixel called with out-of-range coordinates (%d,%d)", x, y);
return;
	}

	/*
	 * Translate x and y values, which are in map units, to
	 * CHIP units
	 */

	switch ( op )
	{
		PIXEL p;

		case PIXELOP_OVERWRITE:
			chip_setPixel(chip, x, y, pixel);
			break;

		case PIXELOP_ADD:
			p = chip_getPixel(chip, x, y);
			pixel_add(&p, pixel);
			chip_setPixel(chip, x, y, &p);
			break;

		default:
			lwerror("Unsupported PIXELOP: %d", op);
	}

}

/* 
 * Bresenham Line Algorithm
 *  http://www.edepot.com/linebresenham.html
 */
void
chip_draw_segment(CHIP *chip,
	int x1, int y1, int x2, int y2,
	PIXEL *pixel, int op)
{
	int x, y;
	int dx, dy;
	int incx, incy;
	int balance;


	if (x2 >= x1)
	{
		dx = x2 - x1;
		incx = 1;
	}
	else
	{
		dx = x1 - x2;
		incx = -1;
	}

	if (y2 >= y1)
	{
		dy = y2 - y1;
		incy = 1;
	}
	else
	{
		dy = y1 - y2;
		incy = -1;
	}

	x = x1;
	y = y1;

	if (dx >= dy)
	{
		dy <<= 1;
		balance = dy - dx;
		dx <<= 1;

		while (x != x2)
		{
			chip_draw_pixel(chip, x, y, pixel, op);
			if (balance >= 0)
			{
				y += incy;
				balance -= dx;
			}
			balance += dy;
			x += incx;
		} chip_draw_pixel(chip, x, y, pixel, op);
	}
	else
	{
		dx <<= 1;
		balance = dx - dy;
		dy <<= 1;

		while (y != y2)
		{
			chip_draw_pixel(chip, x, y, pixel, op);
			if (balance >= 0)
			{
				x += incx;
				balance -= dy;
			}
			balance += dx;
			y += incy;
		} chip_draw_pixel(chip, x, y, pixel, op);
	}
}

void
chip_fill(CHIP *chip, PIXEL *pixel, int op)
{
	int x, y;

#if DEBUG_CHIP
	lwnotice("chip_fill called");
#endif

	for (x=0; x<chip->width; x++)
	{
		for (y=0; y<chip->height; y++)
		{
			chip_draw_pixel(chip, x, y, pixel, op);
		}
	}
}

/********************************************************************
 *
 * CHIP constructors
 * 
 ********************************************************************/

CHIP *
pgchip_construct(BOX3D *bvol, int SRID, int width, int height,
	int datatype, PIXEL *initvalue)
{
	size_t pixsize = chip_pixel_value_size(datatype);
	size_t datasize = pixsize*width*height;
	size_t size = sizeof(CHIP)-sizeof(void*)+datasize;
	CHIP *chip = lwalloc(size);

#if DEBUG_CHIP
	lwnotice(" sizeof(CHIP):%d, pixelsize:%d, datasize:%d, total size:%d", sizeof(CHIP), pixsize, datasize, size);
#endif

	chip->size=size;
	chip->endian_hint=1;
	memcpy(&(chip->bvol), bvol, sizeof(BOX3D));
	chip->SRID=SRID;
	memset(chip->future, '\0', 4);
	chip->factor=1.0;
	chip->datatype=datatype;
	chip->height=height;
	chip->width=width;
	chip->compression=0; /* no compression */
	if ( ! initvalue ) {
		memset(&(chip->data), '\0', datasize);
	} else {
		chip_fill(chip, initvalue, PIXELOP_OVERWRITE);
	}
	return chip;
} 


/********************************************************************
 *
 * Drawing functions
 * 
 ********************************************************************/


/*
 * Transform a POINT from geo coordinates to CHIP offsets.
 * Can be optimized by caching CHIP info somewhere and using
 * multiplications rather then divisions.
 */
static void
transform_point(CHIP* chip, POINT2D* p)
{
	/* geo size of full grid/chip */
	double geowidth = chip->bvol.xmax - chip->bvol.xmin;
	double geoheight = chip->bvol.ymax - chip->bvol.ymin;

	/* geo size of a cell/pixel */
	double xscale = geowidth / chip->width; 
	double yscale = geoheight / chip->height; 

	double xoff = ( chip->bvol.xmin + xscale );
	double yoff = ( chip->bvol.ymin + yscale );

#if 0
	double ox = p->x;
	double oy = p->y;
#endif

	p->x = rint ( ( p->x - xoff ) / xscale );
	p->y = rint ( ( p->y - yoff ) / yscale );

#if 0
	lwnotice("Old x: %g, new x: %g", ox, p->x);
#endif

}

void
chip_draw_ptarray(CHIP *chip, POINTARRAY *pa, PIXEL *pixel, int op)
{
	POINT2D A, B;
	int i;
	int x1, x2, y1, y2;

	for (i=1; i<pa->npoints; i++)
	{
		getPoint2d_p(pa, i-1, &A);
		getPoint2d_p(pa, i, &B);

		transform_point(chip, &A);
		transform_point(chip, &B);

		x1 = A.x;
		y1 = A.y;
		x2 = B.x;
		y2 = B.y;

		chip_draw_segment(chip, x1, y1, x2, y2, pixel, op);
	}
}

void
chip_draw_lwpoint(CHIP *chip, LWPOINT *lwpoint, PIXEL* pixel, int op)
{
	POINTARRAY *pa;
	POINT2D point;

	pa = lwpoint->point;
	getPoint2d_p(pa, 0, &point);

	/* translate to CHIP plane */
	transform_point(chip, &point);

	chip_draw_pixel(chip, point.x, point.y, pixel, op);
}

void
chip_draw_lwline(CHIP *chip, LWLINE *lwline, PIXEL* pixel, int op)
{
	POINTARRAY *pa;

	pa = lwline->points;
	chip_draw_ptarray(chip, pa, pixel, op);
	return;
}

void
chip_draw_lwgeom(CHIP *chip, LWGEOM *lwgeom, PIXEL *pixel, int op)
{
	int i;
	LWCOLLECTION *coll;

	/* Check wheter we should completely skip this geometry */
	if ( lwgeom->bbox )
	{
		if ( chip->bvol.xmax < lwgeom->bbox->xmin ) return;
		if ( chip->bvol.xmin > lwgeom->bbox->xmax ) return;
		if ( chip->bvol.ymax < lwgeom->bbox->ymin ) return;
		if ( chip->bvol.ymin > lwgeom->bbox->ymax ) return;
	}

	switch (TYPE_GETTYPE(lwgeom->type) )
	{
		case POINTTYPE:
			chip_draw_lwpoint(chip, (LWPOINT*)lwgeom, pixel, op);
			return;
		case LINETYPE:
			chip_draw_lwline(chip, (LWLINE*)lwgeom, pixel, op);
			return;
		case POLYGONTYPE:
			lwerror("%s geometry unsupported by draw operation",
				lwgeom_typename(TYPE_GETTYPE(lwgeom->type)));
		case MULTIPOINTTYPE:
		case MULTILINETYPE:
		case MULTIPOLYGONTYPE:
		case COLLECTIONTYPE:
			coll = (LWCOLLECTION *)lwgeom;
			for (i=0; i<coll->ngeoms; i++)
			{
				chip_draw_lwgeom(chip, coll->geoms[i],
					pixel, op);
			}
			return;
		default:
			lwerror("Unknown geometry type: %d", lwgeom->type);
	}
}

/********************************************************************
 *
 * PGsql interfaces
 * 
 ********************************************************************/

typedef struct CHIPDUMPSTATE_T {
	CHIP *chip;
	int x;
	int y;
	char *values[3];
	char fmt[8];
} CHIPDUMPSTATE;

/*
 * Convert a TEXT to a C-String.
 * Remember to lwfree the returned object.
 */
char *
text_to_cstring(text *t)
{
	char *s;
	size_t len;

	len = VARSIZE(t)-VARHDRSZ;
	s = lwalloc(len+1);
	memcpy(s, VARDATA(t), len);
	s[len] = '\0';

	return s;
}

PG_FUNCTION_INFO_V1(CHIP_dump);
Datum CHIP_dump(PG_FUNCTION_ARGS)
{
	CHIP *chip;
	FuncCallContext *funcctx;
	MemoryContext oldcontext, newcontext;
	CHIPDUMPSTATE *state;
	TupleDesc tupdesc;
	AttInMetadata *attinmeta;
	HeapTuple tuple;
	TupleTableSlot *slot;
	Datum result;

	if (SRF_IS_FIRSTCALL())
	{
		funcctx = SRF_FIRSTCALL_INIT();
		newcontext = funcctx->multi_call_memory_ctx;

		oldcontext = MemoryContextSwitchTo(newcontext);

		chip = (CHIP *)PG_DETOAST_DATUM_COPY(PG_GETARG_DATUM(0));

		/* Create function state */
		state = lwalloc(sizeof(CHIPDUMPSTATE));
		state->chip = chip;
		state->x=0;
		state->y=0;
		state->values[0] = lwalloc(256);
		state->values[1] = lwalloc(256);
		state->values[2] = lwalloc(256);

		funcctx->user_fctx = state;

		/*
		 * Build a tuple description for an
		 * geometry_dump tuple
		 */
		tupdesc = RelationNameGetTupleDesc("chip_dump");

		/* allocate a slot for a tuple with this tupdesc */
		slot = TupleDescGetSlot(tupdesc);

		/* allocate a slot for a tuple with this tupdesc */
		slot = TupleDescGetSlot(tupdesc);

		/* assign slot to function context */
		funcctx->slot = slot;

		/*
		 * generate attribute metadata needed later to produce
		 * tuples from raw C strings
		 */
		attinmeta = TupleDescGetAttInMetadata(tupdesc);
		funcctx->attinmeta = attinmeta;

		MemoryContextSwitchTo(oldcontext);
	}

	/* stuff done on every call of the function */
	funcctx = SRF_PERCALL_SETUP();
	newcontext = funcctx->multi_call_memory_ctx;

	/* get state */
	state = funcctx->user_fctx;

	/* Handled simple geometries */
	while ( state->y < state->chip->height &&
		state->x < state->chip->width )
	{
		char buf[256];
		PIXEL p;

		if ( ! state->chip ) lwerror("state->chip corrupted");
		p = chip_getPixel(state->chip, state->x, state->y);
		pixel_writeval(&p, buf, 255);

		sprintf(state->values[0], "%d", state->x);
		sprintf(state->values[1], "%d", state->y);
		sprintf(state->values[2], "%s", buf);

		tuple = BuildTupleFromCStrings(funcctx->attinmeta,
			state->values);
		result = TupleGetDatum(funcctx->slot, tuple);
		
		if ( state->x < state->chip->width-1 )
		{
			state->x++;
		}
		else
		{
			state->x=0;
			state->y++;
		}

		SRF_RETURN_NEXT(funcctx, result);

	}

	SRF_RETURN_DONE(funcctx);
}

PG_FUNCTION_INFO_V1(CHIP_construct);
Datum CHIP_construct(PG_FUNCTION_ARGS)
{
	CHIP *chip;
	BOX3D *box = (BOX3D *)PG_GETARG_POINTER(0);
	int SRID = PG_GETARG_INT32(1);
	int width = PG_GETARG_INT32(2);
	int height = PG_GETARG_INT32(3);
	text *pixel_text = PG_GETARG_TEXT_P(4);
	char *pixel = text_to_cstring(pixel_text);
	PIXEL pix = pixel_readval(pixel);

#if DEBUG_CHIP
	lwnotice("CHIP_construct called");
#endif

	if ( width <= 0 || height <= 0 )
	{
		lwerror("Invalid values for width or height");
		PG_RETURN_NULL();
	}

	chip = pgchip_construct(box, SRID, width, height, pix.type, &pix);

#if DEBUG_CHIP
	lwnotice("Created %dx%d chip type", chip->width, chip->height);
#endif

	PG_RETURN_POINTER(chip);
}

PG_FUNCTION_INFO_V1(CHIP_getpixel);
Datum CHIP_getpixel(PG_FUNCTION_ARGS)
{
	CHIP *chip = (CHIP *) PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
	int x = PG_GETARG_INT32(1);
	int y = PG_GETARG_INT32(2);
	char buf[256];
	size_t len;
	text *ret;
	PIXEL p;

	if ( x < 0 || x >= chip->width )
	{
		lwerror("X out of range %d..%d",
			0, chip->width-1);
		PG_RETURN_NULL();
	}
	if ( y < 0 || y >= chip->height )
	{
		lwerror("Y out of range %d..%d",
			0, chip->height-1);
		PG_RETURN_NULL();
	}

	p = chip_getPixel(chip, x, y);
	pixel_writeval(&p, buf, 255);
	len = strlen(buf);
	ret = lwalloc(len+VARHDRSZ);
	SET_VARSIZE(ret, len+VARHDRSZ);
	memcpy(VARDATA(ret), buf, len);

	PG_RETURN_POINTER(ret);
}

PG_FUNCTION_INFO_V1(CHIP_setpixel);
Datum CHIP_setpixel(PG_FUNCTION_ARGS)
{
	CHIP *chip = (CHIP *) PG_DETOAST_DATUM_COPY(PG_GETARG_DATUM(0));
	int x = PG_GETARG_INT32(1);
	int y = PG_GETARG_INT32(2);
	text *init_val_text = PG_GETARG_TEXT_P(3);
	char *init_val;
	PIXEL pixel;

	/* Parse pixel */
	init_val = text_to_cstring(init_val_text);
	pixel = pixel_readval(init_val);

	if ( chip->datatype != pixel.type )
	{
		lwerror("Pixel datatype %d mismatches chip datatype %d",
			pixel.type, chip->datatype);
	}

	chip_setPixel(chip, x, y, &pixel);

	PG_RETURN_POINTER(chip);

}

PG_FUNCTION_INFO_V1(CHIP_draw);
Datum CHIP_draw(PG_FUNCTION_ARGS)
{
	CHIP *chip = (CHIP *) PG_DETOAST_DATUM_COPY(PG_GETARG_DATUM(0));
	PG_LWGEOM *geom = (PG_LWGEOM *)PG_DETOAST_DATUM(PG_GETARG_DATUM(1));
	LWGEOM *lwgeom = pglwgeom_deserialize(geom);
	text *pixel_text = PG_GETARG_TEXT_P(2);
	char *pixel_str;
	text *pixelop_text;
	char *pixelop_str;
	int pixelop = PIXELOP_OVERWRITE;
	PIXEL pixel;

	/* Check SRID match */
	if ( chip->SRID != lwgeom->SRID )
	{
		lwerror("Operation on mixed SRID objects");
	}

	if ( PG_NARGS() > 3 )
	{
		pixelop_text = PG_GETARG_TEXT_P(3);
		pixelop_str = text_to_cstring(pixelop_text);
		if ( pixelop_str[0] == 'o' )
		{
			pixelop = PIXELOP_OVERWRITE;
		}
		else if ( pixelop_str[0] == 'a' )
		{
			pixelop = PIXELOP_ADD;
		}
		else
		{
			lwerror("Unsupported pixel operation %s", pixelop_str);
		}
	}

	/* Parse pixel */
	pixel_str = text_to_cstring(pixel_text);
	pixel = pixel_readval(pixel_str);
	lwfree(pixel_str);

	if ( pixel.type != chip->datatype )
	{
		lwerror("Pixel/Chip datatype mismatch");
	}

	/* Perform drawing */
	chip_draw_lwgeom(chip, lwgeom, &pixel, pixelop);

	PG_RETURN_POINTER(chip);
}

PG_FUNCTION_INFO_V1(CHIP_fill);
Datum CHIP_fill(PG_FUNCTION_ARGS)
{
	CHIP *chip = (CHIP *) PG_DETOAST_DATUM_COPY(PG_GETARG_DATUM(0));
	text *pixel_text = PG_GETARG_TEXT_P(1);
	char *pixel_str;
	text *pixelop_text;
	char *pixelop_str;
	int pixelop = PIXELOP_OVERWRITE;
	PIXEL pixel;

	if ( PG_NARGS() > 2 )
	{
		pixelop_text = PG_GETARG_TEXT_P(2);
		pixelop_str = text_to_cstring(pixelop_text);
		if ( pixelop_str[0] == 'o' )
		{
			pixelop = PIXELOP_OVERWRITE;
		}
		else if ( pixelop_str[0] == 'a' )
		{
			pixelop = PIXELOP_ADD;
		}
		else
		{
			lwerror("Unsupported pixel operation %s", pixelop_str);
		}
	}

	/* Parse pixel */
	pixel_str = text_to_cstring(pixel_text);
	pixel = pixel_readval(pixel_str);
	lwfree(pixel_str);

	if ( pixel.type != chip->datatype )
	{
		lwerror("Pixel/Chip datatype mismatch");
	}

	/* Perform fill */
	chip_fill(chip, &pixel, pixelop);

	PG_RETURN_POINTER(chip);
}
