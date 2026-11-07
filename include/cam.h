#ifndef CAM_H
#define CAM_H

#include <stdint.h>

typedef struct {
	double x, y, zoom;
	double pitch; /* tilt angle in degrees (e.g. 50.0..60.0) */
	double yaw;   /* yaw rotation in degrees */
	int mode_3d;  /* 1 = 2.5D tilted perspective, 0 = classic 2D */
} cam_t;

#endif
