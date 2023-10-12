#include <iostream>
#include "framework.h"

using namespace std;

// region 1. Bezier

class BezierCurve {
    vector<vec3> cps; // control pts
    float B(int i, float t) {
        int n = cps.size()-1; // n+1 pts!
        float choose = 1;
        for(int j = 1; j <= i; j++)
            choose *= (float)(n-j+1)/j;
        return choose * pow(t, i) * pow(1-t, n-i);
    }
public:
    void AddControlPoint(vec3 cp) { cps.push_back(cp); }
    vec3 r(float t) {
        vec3 rt(0, 0, 0);
        for(int i=0; i < cps.size(); i++)
            rt = rt + cps[i] * B(i,t);
        return rt;
    }
};


void bezierCurveCalc() {
    BezierCurve bezierCurve;
    bezierCurve.AddControlPoint(vec3(3, 4, 0));
    bezierCurve.AddControlPoint(vec3(4, 8, 0));
    bezierCurve.AddControlPoint(vec3(5, 4, 0));
    vec3 rt = bezierCurve.r(1.0);
    cout << rt.x;
}

// endregion

// region 2. Catmull

struct Segment{
    vec2 a;
    vec2 b;
    vec2 c;
    vec2 d;
};

void catmullCurveCalc() {
    vec2 p0(4, 8);
    vec2 p1(7, 9);
    vec2 p2(4, 4);
    vec2 p3(7, 3);
    float t = 1.5;

    float alpha = 0.5f;
    float tension = 0.0f;

    float t0 = 0.0f;
    float t1 = 1.0f;
    float t2 = 2.0f;
    float t3 = 3.0f;

    vec2 m1 = (1.0f - tension) * (t2 - t1) *
              ((p1 - p0) / (t1 - t0) - (p2 - p0) / (t2 - t0) + (p2 - p1) / (t2 - t1));
    vec2 m2 = (1.0f - tension) * (t2 - t1) *
              ((p2 - p1) / (t2 - t1) - (p3 - p1) / (t3 - t1) + (p3 - p2) / (t3 - t2));

    Segment segment;
    segment.a = 2.0f * (p1 - p2) + m1 + m2;
    segment.b = -3.0f * (p1 - p2) - m1 - m1 - m2;
    segment.c = m1;
    segment.d = p1;

    vec2 point = segment.a * t * t * t +
                 segment.b * t * t +
                 segment.c * t +
                 segment.d;

    cout << point.x;
}

// endregion

// region 3. Lagrange

class LagrangeCurve {
    vector<vec3> cps; // control pts
    vector<float> ts; // knots
    float L(int i, float t) {
        float Li = 1.0f;
        for(int j = 0; j < cps.size(); j++)
            if (j != i)
                Li *= (t - ts[j]) / (ts[i] - ts[j]);
        return Li;
    }
public:
    void AddControlPoint(vec3 cp) {
        float ti = cps.size(); // or something better
        cps.push_back(cp); ts.push_back(ti);
    }
    vec3 r(float t) {
        vec3 rt(0, 0, 0);
        for(int i = 0; i < cps.size(); i++)
            rt = rt + cps[i] * L(i,t);
        return rt;
    }
};

void lagrangeCurveCalc() {
    LagrangeCurve lagrangeCurve;
    lagrangeCurve.AddControlPoint(vec3(3, 4, 0));
    lagrangeCurve.AddControlPoint(vec3(4, 8, 0));
    lagrangeCurve.AddControlPoint(vec3(5, 4, 0));
    float t = 1.5f;
    vec3 rt = lagrangeCurve.r(t);
    cout << rt.x;
}

// endregion

// region 4. Implicit surface

void implicitSurfaceCalc() {
    vec3 p(4, 3, 4);
    float nx = p.x / 2;
    float ny =  2 * p.y / 3;
    cout << nx / ny;
}

// endregion

int main() {

    //bezierCurveCalc();
    //catmullCurveCalc();
    lagrangeCurveCalc();
    //implicitSurfaceCalc();

    return 0;
}
