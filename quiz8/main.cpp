#include <iostream>
#include "framework.h"

int main() {
    vec3 r1(83, 70, 0.3);
    vec3 r2(13, 58, 0.3);
    vec3 r3(17, 66, 0.7);
    //vec3 r1(68, 34, 0.7);
    //vec3 r2(19, 89, 0.1);
    //vec3 r3(86, 61, 0.7);

    vec3 ra = (r3 - r1);
    vec3 rb = (r2 - r1);
    printf("%f, %f, %f\n", ra.x, ra.y, ra.z);
    printf("%f, %f, %f\n", rb.x, rb.y, rb.z);

    vec3 n = cross(ra, rb);
    printf("%f, %f, %f\n", n.x, n.y, n.z);
    float a = - n.x / n.z;
    printf("%f", a);

    return 0;
}
