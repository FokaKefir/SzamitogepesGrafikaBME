//=============================================================================================
// Neptun:  Q1CGY7
// Nev:     Babos David
//=============================================================================================

#include "framework.h"
#include <random>
#include <unistd.h>
#include <ctime>

#define NUM_STARS 100
#define SPLIT_INTEGRAL 300

// A konstansoknak normalizalt nem pedig valos ertekek vannak megadva
// annak erdekeben hogy a float pontatlansagat elkeruljuk
#define MIN_DISTANCE 500
#define MAX_DISTANCE 1800

#define MIN_YZ -25
#define MAX_YZ 25

#define RADIUS 3

#define C 100

#define INIT_TIME 1
#define HUBBLE 0.1f

int tx = INIT_TIME;

struct Hit {
    float t;
    float d;
    float v;
    vec3 position, normal;
    Hit() { t = -1; }
};

struct Ray {
    vec3 start, dir;
    Ray(vec3 _start, vec3 _dir) { start = _start; dir = normalize(_dir); }
};

class Intersectable {
public:
    virtual Hit intersect(const Ray& ray) = 0;
};

struct Star : public Intersectable {
    vec3 center;
    float radius;

    Star(const vec3& _center, float _radius) {
        center = _center;
        radius = _radius;
    }

    Hit intersect(const Ray& ray) {
        Hit hit;

        // Hubble torvenyt alkalmazzuk a csillagok tavolodasara
        vec3 center = this->center * exp(HUBBLE * tx);
        vec3 dist = ray.start - center;
        float a = dot(ray.dir, ray.dir);
        float b = dot(dist, ray.dir) * 2.0f;
        float c = dot(dist, dist) - radius * radius;
        float discr = b * b - 4.0f * a * c;
        if (discr < 0) return hit;
        float sqrt_discr = sqrtf(discr);
        float t1 = (-b + sqrt_discr) / 2.0f / a;	// t1 >= t2 for sure
        float t2 = (-b - sqrt_discr) / 2.0f / a;
        if (t1 <= 0) return hit;

        // a kovetkezo egyenleg jott ki amint a newton es raphson formulat levezettem
        float T = (t2 > 0) ? t2 : t1;
        float d = length(ray.dir * hit.t);
        float v = d * HUBBLE;
        hit.t = T - (d / (v + C));
        hit.position = ray.start + ray.dir * hit.t;
        hit.d = length(ray.dir * hit.t);
        hit.v = hit.d * HUBBLE; // add velocity
        hit.normal = (hit.position - center) * (1.0f / radius);
        return hit;
    }
};

struct DataPoint {
    float time;
    float value;
};

class HermiteInterpolation {
private:
    std::vector<DataPoint> data;
    float minTime, maxTime;
    float minVal, maxVal;

public:
    HermiteInterpolation(std::vector<DataPoint> data, float maxVal, float minVal) {
        this->data = data;
        this->minVal = minVal;
        this->maxVal = maxVal;
        this->minTime = data.front().time;
        this->maxTime = data.back().time;
    }

    float interpolate(float time) {
        if (time <= minTime)
            return data.front().value;
        else if (time >= maxTime)
            return data.back().value;

        float result = 0.0;
        for (int i = 0; i < data.size(); i++) {
            float term = data[i].value;
            for (int j = 0; j < data.size(); j++) {
                if (i != j) {
                    term *= (time - data[j].time) / (data[i].time - data[j].time);
                }
            }
            result += term;
        }

        if (result > maxVal)
            result = maxVal;
        else if (result < minVal)
            result = minVal;

        return result;
    }

    float getMinTime() const {
        return this->minTime;
    }

    float getMaxTime() const {
        return this->maxTime;
    }

};

class Camera {
private:
    vec3 eye, lookat, right, up;


    float calcComponentDoppler(HermiteInterpolation sInterpol, HermiteInterpolation rgbInterpol, float v) const {
        float lambdaConst = (1 + v / C);

        float sum = 0.0f;
        float minTime = sInterpol.getMinTime();
        float maxTime = sInterpol.getMaxTime();

        float step = (maxTime - minTime) / SPLIT_INTEGRAL;

        for (int i = 0; i < SPLIT_INTEGRAL; ++i) {
            float t1 = minTime + (float) i * step;
            float t2 = minTime + (float) (i + 1) * step;
            float l1 = t1 * lambdaConst;
            float l2 = t2 * lambdaConst;

            float val1 = sInterpol.interpolate(t1) * rgbInterpol.interpolate(l1);
            float val2 = sInterpol.interpolate(t1) * rgbInterpol.interpolate(l2);

            sum += (val1 + val2) / 2.0f * (t2 - t1);
        }

        return sum / SPLIT_INTEGRAL;
    }

public:
    void set(vec3 _eye, vec3 _lookat, vec3 vup, float fov) {
        eye = _eye;
        lookat = _lookat;
        vec3 w = eye - lookat;
        float focus = length(w);
        right = normalize(cross(vup, w)) * focus * tanf(fov / 2);
        up = normalize(cross(w, right)) * focus * tanf(fov / 2);
    }

    Ray getRay(int X, int Y) {
        vec3 dir = lookat + right * (2.0f * (X + 0.5f) / windowWidth - 1) + up * (2.0f * (Y + 0.5f) / windowHeight - 1) - eye;
        return Ray(eye, dir);
    }

    vec4 getRGB(float v) const {
        std::vector<DataPoint> spectrumData = {{150.0, 0.0},{450.0, 1.0},{1600.0, 0.1}};
        HermiteInterpolation spectrumInterpolation(spectrumData, 1.0, 0.0);

        std::vector<DataPoint> redDetectorData = {{400.0, 0.0},{500.0, -0.2},{600.0, 2.5},{700.0, 0.0}};
        HermiteInterpolation redDetectorInterpolation(redDetectorData, 2.5, -0.2);

        std::vector<DataPoint> greenDetectorData = {{400.0, 0.0},{450.0, -0.1},{550.0, 1.2},{700.0, 0.0}};
        HermiteInterpolation greenDetectorInterpolation(greenDetectorData, 1.2, -0.1);

        std::vector<DataPoint> blueDetectorData = {{400.0, 0.0},{460.0, 1.0},{520.0, 0.0}};
        HermiteInterpolation blueDetectorInterpolation(blueDetectorData, 1.0, 0.0);

        float redComponent = calcComponentDoppler(spectrumInterpolation, redDetectorInterpolation, v);
        float greenComponent = calcComponentDoppler(spectrumInterpolation, greenDetectorInterpolation, v);
        float blueComponent = calcComponentDoppler(spectrumInterpolation, blueDetectorInterpolation, v);

        vec4 rgbComponent(redComponent, greenComponent, blueComponent, 1);
        return rgbComponent;
    }
};

float rnd(int min, int max) {
    return (float) ((rand() / RAND_MAX) * (max - min) + min);
    //return (float) (rand() % (max + 1 - min) + min);
}

float max3(float a, float b, float c) {
    if (a >= b && a >= c)
        return a;
    if (b >= a && b >= c)
        return b;
    return c;
}

class Scene {
    std::vector<Star*> stars;
    Camera camera;
    float globalMaxVal = -1.0;
public:
    void build() {
        float fov = 4 * M_PI / 180;
        vec3 eye = vec3(0, 0, 0), vup = vec3(0, 1, 0),
            lookat = vec3(1 / tan(fov / 2), 0, 0);
        camera.set(eye, lookat, vup, fov);

        std::random_device rd; // obtain a random number from hardware
        std::mt19937 gen(rd()); // seed the generator
        std::uniform_int_distribution<> distrX(MIN_DISTANCE, MAX_DISTANCE); // define the range
        std::uniform_int_distribution<> distrY(MIN_YZ, MAX_YZ); // define the range
        std::uniform_int_distribution<> distrZ(MIN_YZ, MAX_YZ); // define the range
        for (int i = 0; i < NUM_STARS; i++) {
            //float x = rnd(MIN_DISTANCE, MAX_DISTANCE);
            //float y = rnd(MIN_YZ, MAX_YZ);
            //float z = rnd(MIN_YZ, MAX_YZ);
            float x = distrX(gen);
            float y = distrY(gen);
            float z = distrZ(gen);

            Star* star = new Star(vec3(x, y, z), RADIUS);
            stars.push_back(star);
        }
    }

    void render(std::vector<vec4>& image) {
        std::vector<vec4> tmpimage(windowWidth * windowHeight);
        float maxVal = -1.0;
        for (int Y = 0; Y < windowHeight; Y++) {
#pragma omp parallel for
            for (int X = 0; X < windowWidth; X++) {
                Hit hit = trace(camera.getRay(X, Y));
                if (hit.t == -1) {
                    tmpimage[Y * windowWidth + X] = vec4(0.0f, 0.0f, 0.0f, 1.0f);
                } else {
                    vec4 rgb = camera.getRGB(hit.v);
                    tmpimage[Y * windowWidth + X] = rgb;
                    float maxComp = max3(rgb.x, rgb.y, rgb.z);
                    if (hit.t != -1.0 && maxVal < maxComp) {
                        maxVal = maxComp;
                    }
                }
            }
        }

        if (globalMaxVal == -1.0)
            globalMaxVal = maxVal;

        for (int Y = 0; Y < windowHeight; Y++) {
            for (int X = 0; X < windowWidth; X++) {
                vec4 color;
                if (globalMaxVal != -1.0 && tmpimage[Y * windowWidth + X].x != 0.0) {
                    color = vec4(5 * tmpimage[Y * windowWidth + X] / globalMaxVal);
                    color.w = 1;
                } else {
                    color = vec4(0, 0, 0, 1);
                }
                image[Y * windowWidth + X] = color;
            }
        }
    }

    Hit firstIntersect(Ray ray) {
        Hit bestHit;
        for (Intersectable * object : stars) {
            Hit hit = object->intersect(ray); //  hit.t < 0 if no intersection
            if (hit.t > 0 && (bestHit.t < 0 || hit.t < bestHit.t))
                bestHit = hit;
        }
        if (dot(ray.dir, bestHit.normal) > 0)
            bestHit.normal = bestHit.normal * (-1);
        return bestHit;
    }

    Hit trace(Ray ray) {
        Hit hit = firstIntersect(ray);
        return hit;
    }
};

GPUProgram gpuProgram; // vertex and fragment shaders
Scene scene;

// vertex shader in GLSL
const char *vertexSource = R"(
	#version 330
    precision highp float;

	layout(location = 0) in vec2 cVertexPosition;	// Attrib Array 0
	out vec2 texcoord;

	void main() {
		texcoord = (cVertexPosition + vec2(1, 1))/2;							// -1,1 to 0,1
		gl_Position = vec4(cVertexPosition.x, cVertexPosition.y, 0, 1); 		// transform to clipping space
	}
)";

// fragment shader in GLSL
const char *fragmentSource = R"(
	#version 330
    precision highp float;

	uniform sampler2D textureUnit;
	in  vec2 texcoord;			// interpolated texture coordinates
	out vec4 fragmentColor;		// output that goes to the raster memory as told by glBindFragDataLocation

	void main() {
		fragmentColor = texture(textureUnit, texcoord); 
	}
)";

class FullScreenTexturedQuad {
    unsigned int vao;	// vertex array object id and texture id
    Texture texture;
public:
    FullScreenTexturedQuad(int windowWidth, int windowHeight, std::vector<vec4>& image)
            : texture(windowWidth, windowHeight, image)
    {
        glGenVertexArrays(1, &vao);	// create 1 vertex array object
        glBindVertexArray(vao);		// make it active

        unsigned int vbo;		// vertex buffer stars
        glGenBuffers(1, &vbo);	// Generate 1 vertex buffer stars

        // vertex coordinates: vbo0 -> Attrib Array 0 -> vertexPosition of the vertex shader
        glBindBuffer(GL_ARRAY_BUFFER, vbo); // make it active, it is an array
        float vertexCoords[] = { -1, -1,  1, -1,  1, 1,  -1, 1 };	// two triangles forming a quad
        glBufferData(GL_ARRAY_BUFFER, sizeof(vertexCoords), vertexCoords, GL_STATIC_DRAW);	   // copy to that part of the memory which is not modified
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, NULL);     // stride and offset: it is tightly packed
    }

    void Draw() {
        glBindVertexArray(vao);	// make the vao and its vbos active playing the role of the data source
        gpuProgram.setUniform(texture, "textureUnit");
        glDrawArrays(GL_TRIANGLE_FAN, 0, 4);	// draw two triangles forming a quad
    }
};

FullScreenTexturedQuad * fullScreenTexturedQuad;
std::vector<vec4> image(windowWidth * windowHeight);

// Initialization, create an OpenGL context
void onInitialization() {
    srand(time(NULL));

    glViewport(0, 0, windowWidth, windowHeight);
    scene.build();
    scene.render(image);

    // create program for the GPU
    gpuProgram.create(vertexSource, fragmentSource, "fragmentColor");
}

// Window has become invalid: Redraw
void onDisplay() {
    long timeStart = glutGet(GLUT_ELAPSED_TIME);
    scene.render(image);
    long timeEnd = glutGet(GLUT_ELAPSED_TIME);
    printf("Rendering time: %ld milliseconds\n", (timeEnd - timeStart));

    fullScreenTexturedQuad = new FullScreenTexturedQuad(windowWidth, windowHeight, image);

    fullScreenTexturedQuad->Draw();
    glutSwapBuffers();									// exchange the two buffers
}

// Key of ASCII code pressed
void onKeyboard(unsigned char key, int pX, int pY) {
    if ('0' <= key and key <= '9') {
        int k = key - '0';
        tx = 2 * k;
        printf("%d milliard ev\n", tx);
        glutPostRedisplay();
    }
}

// Key of ASCII code released
void onKeyboardUp(unsigned char key, int pX, int pY) {

}

// Mouse click event
void onMouse(int button, int state, int pX, int pY) {
}

// Move mouse with key pressed
void onMouseMotion(int pX, int pY) {
}

// Idle event indicating that some tx elapsed: do animation here
void onIdle() {
}