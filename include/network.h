/*
 * new-project/include/network.h  (wrapper — do not edit original)
 *
 * The original bolo/network.h ends with:
 *   #pragma pack(pop, enter_network_obj, 1)
 * In MSVC, the trailing ',1' sets the packing to 1 AFTER the pop instead
 * of restoring the saved default.  Every .c file that includes network.h
 * is left with pack(1) active, corrupting Windows SDK C_ASSERT checks.
 *
 * This wrapper includes the original, then resets packing to the compiler
 * default so callers are unaffected.
 */
#include "../../winbolo/src/bolo/network.h"
#pragma pack()   /* reset to compiler default (/Zp or none) */
