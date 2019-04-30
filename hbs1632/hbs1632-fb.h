/*
 * Header file for TI DA8XX LCD controller platform data.
 *
 * Copyright (C) 2008-2009 MontaVista Software Inc.
 * Copyright (C) 2008-2009 Texas Instruments Inc
 *
 * This file is licensed under the terms of the GNU General Public License
 * version 2. This program is licensed "as is" without any warranty of any
 * kind, whether express or implied.
 */

#ifndef _HBS1632_FB_H_
#define _HBS1632_FB_H_

/* ioctls */
#define FBIOGET_BRIGHTNESS	_IOR('F', 0x31, int)
#define FBIOPUT_BRIGHTNESS	_IOW('F', 0x32, int)
#define FBIOGET_BLINK		_IOR('F', 0x33, int)
#define FBIOPUT_BLINK		_IOW('F', 0x34, int)

#endif  /* ifndef _HBS1632_FB_H_ */

