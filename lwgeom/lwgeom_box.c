#include "postgres.h"
#include "utils/geo_decls.h"

#include "lwgeom_pg.h"
#include "liblwgeom.h"



//convert postgresql BOX to BOX2D
BOX2DFLOAT4 *
box_to_box2df(BOX *box)
{
	BOX2DFLOAT4 *result = (BOX2DFLOAT4*) lwalloc(sizeof(BOX2DFLOAT4));

	if (box == NULL)
		return result;

	result->xmin = nextDown_f(box->low.x);
	result->ymin = nextDown_f(box->low.y);

	result->xmax = nextUp_f(box->high.x);
	result->ymax = nextUp_f(box->high.x);

	return result;
}

// convert BOX2D to postgresql BOX
BOX   box2df_to_box(BOX2DFLOAT4 *box)
{
	BOX result;

	if (box == NULL)
		return result;

	result.low.x = nextDown_d(box->xmin);
	result.low.y = nextDown_d(box->ymin);

	result.high.x = nextUp_d(box->xmax);
	result.high.y = nextUp_d(box->ymax);

	return result;
}

// convert BOX2D to postgresql BOX
void
box2df_to_box_p(BOX2DFLOAT4 *box, BOX *out)
{
	if (box == NULL) return;

	out->low.x = nextDown_d(box->xmin);
	out->low.y = nextDown_d(box->ymin);

	out->high.x = nextUp_d(box->xmax);
	out->high.y = nextUp_d(box->ymax);
}

