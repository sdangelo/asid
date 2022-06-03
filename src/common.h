/*
 * A-SID - C64 bandpass filter + LFO
 *
 * Copyright (C) 2022 Orastron srl unipersonale
 *
 * A-SID is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, version 3 of the License.
 *
 * A-SID is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 *
 * File author: Stefano D'Angelo
 */

#ifndef _ORDSP_COMMON_H
#define _ORDSP_COMMON_H

#include <stdlib.h>
#ifndef INFINITY
# include <math.h>
#endif

#ifndef ORDSP_MALLOC
# define ORDSP_MALLOC malloc
#endif
#ifndef ORDSP_REALLOC
# define ORDSP_REALLOC realloc
#endif
#ifndef ORDSP_FREE
# define ORDSP_FREE free
#endif

#endif
