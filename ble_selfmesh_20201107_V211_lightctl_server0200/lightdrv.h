/**
 * Copyright (c) 2017 - 2019, DFT Tech Co.,Ltd
 *
 * All rights reserved.
 *
 *
 */

#ifndef LIGHTDRV_H
#define LIGHTDRV_H
extern int lightdrv_init(void);

extern int lightdrv_lightness_set(int lightness);

extern int lightdrv_start(void);
	
extern int lightdrv_stop(void);

#endif //LIGHTDRV_H
