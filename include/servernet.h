/*
 * new-project/include/servernet.h  (wrapper — do not edit original)
 *
 * The original server/servernet.h ends with:
 *   #pragma pack(pop, enter_servernet_obj, 1)
 * Same bug as network.h: the trailing ',1' leaves pack=1 after the pop.
 *
 * This wrapper includes the original, then resets packing to default.
 */
#include "../../winbolo/src/server/servernet.h"
#pragma pack()   /* reset to compiler default (/Zp or none) */
