
#define vec_copy(dest, a) (dest)[0] = (a)[0]; (dest)[1] = (a)[1]; (dest)[2] = (a)[2]
#define vec_add(dest, a, b) (dest)[0] = (a)[0] + (b)[0]; (dest)[1] = (a)[1] + (b)[1]; (dest)[2] = (a)[2] + (b)[2]
#define vec_subtract(dest, a, b) (dest)[0] = (a)[0] - (b)[0]; (dest)[1] = (a)[1] - (b)[1]; (dest)[2] = (a)[2] - (b)[2]
#define vec_dot(a, b) ((a)[0] * (b)[0] + (a)[1] * (b)[1] + (a)[2] * (b)[2])
#define vec_cross(dest, a, b) (dest)[0] = (a)[1] * (b)[2] - (a)[2] * (b)[1]; (dest)[1] = (a)[2] * (b)[0] - (a)[0] * (b)[2]; (dest)[2] = (a)[0] * (b)[1] - (a)[1] * (b)[0]
#define vec_sqr_length(a) vec_dot(a, a)
#define vec_length(a) (sqrt(vec_sqr_length(a)))
#define vec_scale(dest, a, s) (dest)[0] = (a)[0] * s; (dest)[1] = (a)[1] * s; (dest)[2] = (a)[2] * s
#define vec_divide(dest, a, d) (dest)[0] = (a)[0] / d; (dest)[1] = (a)[1] / d; (dest)[2] = (a)[2] / d

