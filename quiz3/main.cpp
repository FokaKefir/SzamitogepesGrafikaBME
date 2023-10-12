#include <iostream>

using namespace std;

int main() {
    float pY = 117.0f;

    float cY = 1.0f - 2.0f * (pY - 200) / 700;
    cout << "y = " << cY << endl;
    return 0;
}
