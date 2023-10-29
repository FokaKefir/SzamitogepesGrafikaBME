#include <iostream>
#include <ctime>
#include <unistd.h>

using namespace std;


#define MIN_DISTANCE 10000000
#define MAX_DISTANCE 60000000

#define MIN_YZ -7000000
#define MAX_YZ 7000000

int rnd(int min, int max) {
    return (rand() % (max - min + 1)) + min;
}


int main() {

    srand(time(NULL));

    for (int i = 0; i < 50; ++i) {
        int x = rnd(MIN_DISTANCE, MAX_DISTANCE);
        int y = rnd(MIN_YZ, MAX_YZ);
        int z = rnd(MIN_YZ, MAX_YZ);

        printf("%d %d %d\n", x, y, z);
    }





    return 0;
}
