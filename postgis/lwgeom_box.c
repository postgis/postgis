#include "postgres.h"
#include "utils/geo_decls.h"

#include "lwgeom_pg.h"
#include "liblwgeom.h"


void box_to_box2df(BOX *box, BOX2DFLOAT4 *out);



/* convert postgresql BOX to BOX2D */
void
box_to_box2df(BOX *box, BOX2DFLOAT4 *out)
{
#if PARANOIA_LEVEL > 0
	if (box == NULL) return;
#endif

	out->xmin = nextDown_f(box->low.x);
	out->ymin = nextDown_f(box->low.y);

	out->xmax = nextUp_f(box->high.x);
	out->ymax = nextUp_f(box->high.x);

}

/* convert BOX2D to postgresql BOX */
void
box2df_to_box_p(BOX2DFLOAT4 *box, BOX *out)
{
#if PARANOIA_LEVEL > 0
	if (box == NULL) return;
#endif

	out->low.x = nextDown_d(box->xmin);
	out->low.y = nextDown_d(box->ymin);

	out->high.x = nextUp_d(box->xmax);
	out->high.y = nextUp_d(box->ymax);
}

