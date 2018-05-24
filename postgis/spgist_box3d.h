
#include <postgres.h>
#include <liblwgeom.h>
#include <math.h>

#include <access/spgist.h>
#include <access/stratnum.h>
#include <catalog/namespace.h>
#include <catalog/pg_type.h>
#include <utils/builtins.h>
#include <utils/geo_decls.h>

/*****************************************************************************
 * Operator strategy numbers used in the GiST and SP-GiST temporal opclasses
 *****************************************************************************/

#define RTOverBeforeStrategyNumber		28		/* for &</ */
#define RTBeforeStrategyNumber			29		/* for <</ */
#define RTAfterStrategyNumber			30		/* for />> */
#define RTOverAfterStrategyNumber		31		/* for /&> */

/*****************************************************************************
 * BOX3D operators
 *****************************************************************************/


bool BOX3D_contains_internal(BOX3D *box1, BOX3D *box2);
bool BOX3D_contained_internal(BOX3D *box1, BOX3D *box2);
bool BOX3D_overlaps_internal(BOX3D *box1, BOX3D *box2);
bool BOX3D_same_internal(BOX3D *box1, BOX3D *box2);
bool BOX3D_left_internal(BOX3D *box1, BOX3D *box2);
bool BOX3D_overleft_internal(BOX3D *box1, BOX3D *box2);
bool BOX3D_right_internal(BOX3D *box1, BOX3D *box2);
bool BOX3D_overright_internal(BOX3D *box1, BOX3D *box2);
bool BOX3D_below_internal(BOX3D *box1, BOX3D *box2);
bool BOX3D_overbelow_internal(BOX3D *box1, BOX3D *box2);
bool BOX3D_above_internal(BOX3D *box1, BOX3D *box2);
bool BOX3D_overabove_internal(BOX3D *box1, BOX3D *box2);
bool BOX3D_before_internal(BOX3D *box1, BOX3D *box2);
bool BOX3D_overbefore_internal(BOX3D *box1, BOX3D *box2);
bool BOX3D_after_internal(BOX3D *box1, BOX3D *box2);
bool BOX3D_overafter_internal(BOX3D *box1, BOX3D *box2);
double BOX3D_distance_internal(BOX3D *box1, BOX3D *box2);
