#include <iostream>
#include "framework.h"

using namespace std;

void affin() {
    vec2 t00(4, 3);
    vec2 t10(5, 5);
    vec2 t01(4, 2);

    float x = 4.0f, y = 5.0f;
    vec2 txy = t00 + x * (t10 - t00) + y * (t01 - t00);
    cout << "x = " << txy.x << " y = " << txy.y << endl;

}

void normalAfterDisplacement() {
    vec4 eq(8, 7, 3, 2);
    vec4 k(4, 3, 5, 0);
    eq += k;
    cout << eq.x / eq.y << endl;
}

void normalAfterGain() {
    vec4 eq(9, 7, 2, 2);
    eq.y = 2 * eq.y;
    cout << eq.x / eq.y << endl;
}

void normal() {
    vec4 eq(8.5, 7, 4.4, 2.1);
    cout << eq.x / eq.y << endl;
}

/**
 * Kvaternion szamolo, tedd at productra
 * https://www.omnicalculator.com/math/quaternion
 */

int main() {
    //affin();
    //normalAfterDisplacement();
    //normalAfterGain();
    //normal();
    return 0;
}

