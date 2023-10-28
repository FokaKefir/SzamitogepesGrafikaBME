//=============================================================================================
// Computer Graphics Sample Program: GPU ray casting
//=============================================================================================
#include "framework.h"

// vertex shader in GLSL
const char *vertexSource = R"(
	#version 450
    precision highp float;

	uniform vec3 wLookAt, wRight, wUp;          // pos of eye

	layout(location = 0) in vec2 cCamWindowVertex;	// Attrib Array 0
	out vec3 p;

	void main() {
		gl_Position = vec4(cCamWindowVertex, 0, 1);
		p = wLookAt + wRight * cCamWindowVertex.x + wUp * cCamWindowVertex.y;
	}
)";
// fragment shader in GLSL
const char *fragmentSource = R"(
	#version 450
    precision highp float;

	struct Material {
		vec3 ka, kd, ks;
		float  shininess;
		vec3 F0;
		int rough, reflective;
	};

	struct Light {
		vec3 direction;
		vec3 Le, La;
	};

	struct Sphere {
		vec3 center;
		float radius;
	};

	struct Hit {
		float t;
		vec3 position, normal;
		int mat;	// material index
	};

	struct Ray {
		vec3 start, dir;
	};

	const int nMaxObjects = 500;

	uniform vec3 wEye;
	uniform Light light;
	uniform Material materials[2];  // diffuse, specular, ambient ref
	uniform int nObjects;
	uniform Sphere objects[nMaxObjects];

	in  vec3 p;					// point on camera window corresponding to the pixel
	out vec4 fragmentColor;		// output that goes to the raster memory as told by glBindFragDataLocation

	Hit intersect(const Sphere object, const Ray ray) {
		Hit hit;
		hit.t = -1;
		vec3 dist = ray.start - object.center;
		float a = dot(ray.dir, ray.dir);
		float b = dot(dist, ray.dir) * 2.0f;
		float c = dot(dist, dist) - object.radius * object.radius;
		float discr = b * b - 4.0f * a * c;
		if (discr < 0) return hit;
		float sqrt_discr = sqrt(discr);
		float t1 = (-b + sqrt_discr) / 2.0f / a;	// t1 >= t2 for sure
		float t2 = (-b - sqrt_discr) / 2.0f / a;
		if (t1 <= 0) return hit;
		hit.t = (t2 > 0) ? t2 : t1;
		hit.position = ray.start + ray.dir * hit.t;
		hit.normal = (hit.position - object.center) / object.radius;
		return hit;
	}

	Hit firstIntersect(Ray ray) {
		Hit bestHit;
		bestHit.t = -1;
		for (int o = 0; o < nObjects; o++) {
			Hit hit = intersect(objects[o], ray); //  hit.t < 0 if no intersection
			if (o < nObjects/2) hit.mat = 0;	 // half of the objects are rough
			else			    hit.mat = 1;     // half of the objects are reflective
			if (hit.t > 0 && (bestHit.t < 0 || hit.t < bestHit.t))  bestHit = hit;
		}
		if (dot(ray.dir, bestHit.normal) > 0) bestHit.normal = bestHit.normal * (-1);
		return bestHit;
	}

	bool shadowIntersect(Ray ray) {	// for directional lights
		for (int o = 0; o < nObjects; o++) if (intersect(objects[o], ray).t > 0) return true; //  hit.t < 0 if no intersection
		return false;
	}

	vec3 Fresnel(vec3 F0, float cosTheta) {
		return F0 + (vec3(1, 1, 1) - F0) * pow(cosTheta, 5);
	}

	const float epsilon = 0.0001f;
	const int maxdepth = 5;

	vec3 trace(Ray ray) {
		vec3 weight = vec3(1, 1, 1);
		vec3 outRadiance = vec3(0, 0, 0);
		for(int d = 0; d < maxdepth; d++) {
			Hit hit = firstIntersect(ray);
			if (hit.t < 0) return weight * light.La;
			if (materials[hit.mat].rough == 1) {
				outRadiance += weight * materials[hit.mat].ka * light.La;
				Ray shadowRay;
				shadowRay.start = hit.position + hit.normal * epsilon;
				shadowRay.dir = light.direction;
				float cosTheta = dot(hit.normal, light.direction);
				if (cosTheta > 0 && !shadowIntersect(shadowRay)) {
					outRadiance += weight * light.Le * materials[hit.mat].kd * cosTheta;
					vec3 halfway = normalize(-ray.dir + light.direction);
					float cosDelta = dot(hit.normal, halfway);
					if (cosDelta > 0) outRadiance += weight * light.Le * materials[hit.mat].ks * pow(cosDelta, materials[hit.mat].shininess);
				}
			}

			if (materials[hit.mat].reflective == 1) {
				weight *= Fresnel(materials[hit.mat].F0, dot(-ray.dir, hit.normal));
				ray.start = hit.position + hit.normal * epsilon;
				ray.dir = reflect(ray.dir, hit.normal);
			} else return outRadiance;
		}
	}

	void main() {
		Ray ray;
		ray.start = wEye;
		ray.dir = normalize(p - wEye);
		fragmentColor = vec4(trace(ray), 1);
	}
)";

//---------------------------
struct Material {
//---------------------------
	vec3 ka, kd, ks;
	float  shininess;
	vec3 F0;
	int rough, reflective;
};

//---------------------------
struct RoughMaterial : Material {
//---------------------------
	RoughMaterial(vec3 _kd, vec3 _ks, float _shininess) {
		ka = _kd * M_PI;
		kd = _kd;
		ks = _ks;
		shininess = _shininess;
		rough = true;
		reflective = false;
	}
};

//---------------------------
struct SmoothMaterial : Material {
//---------------------------
	SmoothMaterial(vec3 _F0) {
		F0 = _F0;
		rough = false;
		reflective = true;
	}
};

//---------------------------
struct Sphere {
//---------------------------
	vec3 center;
	float radius;

	Sphere(const vec3& _center, float _radius) { center = _center; radius = _radius; }
};

//---------------------------
struct Camera {
//---------------------------
	vec3 eye, lookat, right, up;
	float fov;
public:
	void set(vec3 _eye, vec3 _lookat, vec3 vup, float _fov) {
		eye = _eye;
		lookat = _lookat;
		fov = _fov;
		vec3 w = eye - lookat;
		float f = length(w);
		right = normalize(cross(vup, w)) * f * tanf(fov / 2);
		up = normalize(cross(w, right)) * f * tanf(fov / 2);
	}
	void Animate(float dt) {
		eye = vec3((eye.x - lookat.x) * cos(dt) + (eye.z - lookat.z) * sin(dt) + lookat.x,
			eye.y,
			-(eye.x - lookat.x) * sin(dt) + (eye.z - lookat.z) * cos(dt) + lookat.z);
		set(eye, lookat, up, fov);
	}
};

//---------------------------
struct Light {
//---------------------------
	vec3 direction;
	vec3 Le, La;
	Light(vec3 _direction, vec3 _Le, vec3 _La) {
		direction = normalize(_direction);
		Le = _Le; La = _La;
	}
};

//---------------------------
class Shader : public GPUProgram {
//---------------------------
public:
	void setUniformMaterials(const std::vector<Material*>& materials) {
		char name[256];
		for (unsigned int mat = 0; mat < materials.size(); mat++) {
			sprintf(name, "materials[%d].ka", mat); setUniform(materials[mat]->ka, name);
			sprintf(name, "materials[%d].kd", mat); setUniform(materials[mat]->kd, name);
			sprintf(name, "materials[%d].ks", mat); setUniform(materials[mat]->ks, name);
			sprintf(name, "materials[%d].shininess", mat); setUniform(materials[mat]->shininess, name);
			sprintf(name, "materials[%d].F0", mat); setUniform(materials[mat]->F0, name);
			sprintf(name, "materials[%d].rough", mat); setUniform(materials[mat]->rough, name);
			sprintf(name, "materials[%d].reflective", mat); setUniform(materials[mat]->reflective, name);
		}
	}

	void setUniformLight(Light* light) {
		setUniform(light->La, "light.La");
		setUniform(light->Le, "light.Le");
		setUniform(light->direction, "light.direction");
	}

	void setUniformCamera(const Camera& camera) {
		setUniform(camera.eye, "wEye");
		setUniform(camera.lookat, "wLookAt");
		setUniform(camera.right, "wRight");
		setUniform(camera.up, "wUp");
	}

	void setUniformObjects(const std::vector<Sphere*>& objects) {
		setUniform((int)objects.size(), "nObjects");
		char name[256];
		for (unsigned int o = 0; o < objects.size(); o++) {
			sprintf(name, "objects[%d].center", o);  setUniform(objects[o]->center, name);
			sprintf(name, "objects[%d].radius", o);  setUniform(objects[o]->radius, name);
		}
	}
};

float rnd() { return (float)rand() / RAND_MAX; }

//---------------------------
class Scene {
//---------------------------
	std::vector<Sphere *> objects;
	std::vector<Light *> lights;
	Camera camera;
	std::vector<Material *> materials;
public:
	void build() {
		vec3 eye = vec3(0, 0, 2);
		vec3 vup = vec3(0, 1, 0);
		vec3 lookat = vec3(0, 0, 0);
		float fov = 45 * (float)M_PI / 180;
		camera.set(eye, lookat, vup, fov);

		lights.push_back(new Light(vec3(1, 1, 1), vec3(3, 3, 3), vec3(0.4f, 0.3f, 0.3f)));

		vec3 kd(0.3f, 0.2f, 0.1f), ks(10, 10, 10);
		materials.push_back(new RoughMaterial(kd, ks, 50));
		materials.push_back(new SmoothMaterial(vec3(0.9f, 0.85f, 0.8f)));

		for (int i = 0; i < 500; i++)
			objects.push_back(new Sphere(vec3(rnd() - 0.5f, rnd() - 0.5f, rnd() - 0.5f), rnd() * 0.1f));

	}

	void setUniform(Shader& shader) {
		shader.setUniformObjects(objects);
		shader.setUniformMaterials(materials);
		shader.setUniformLight(lights[0]);
		shader.setUniformCamera(camera);
	}

	void Animate(float dt) { camera.Animate(dt); }
};

Shader shader; // vertex and fragment shaders
Scene scene;

//---------------------------
class FullScreenTexturedQuad {
//---------------------------
	unsigned int vao = 0;	// vertex array object id and texture id
public:
	void create() {
		glGenVertexArrays(1, &vao);	// create 1 vertex array object
		glBindVertexArray(vao);		// make it active

		unsigned int vbo;		// vertex buffer objects
		glGenBuffers(1, &vbo);	// Generate 1 vertex buffer objects

		// vertex coordinates: vbo0 -> Attrib Array 0 -> vertexPosition of the vertex shader
		glBindBuffer(GL_ARRAY_BUFFER, vbo); // make it active, it is an array
		float vertexCoords[] = { -1, -1,  1, -1,  1, 1,  -1, 1 };	// two triangles forming a quad
		glBufferData(GL_ARRAY_BUFFER, sizeof(vertexCoords), vertexCoords, GL_STATIC_DRAW);	   // copy to that part of the memory which is not modified
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, NULL);     // stride and offset: it is tightly packed
	}

	void Draw() {
		glBindVertexArray(vao);	// make the vao and its vbos active playing the role of the data source
		glDrawArrays(GL_TRIANGLE_FAN, 0, 4);	// draw two triangles forming a quad
	}
};

FullScreenTexturedQuad fullScreenTexturedQuad;

// Initialization, create an OpenGL context
void onInitialization() {
	glViewport(0, 0, windowWidth, windowHeight);
	scene.build();
	fullScreenTexturedQuad.create();

	// create program for the GPU
	shader.create(vertexSource, fragmentSource, "fragmentColor");
	shader.Use();
}

// Window has become invalid: Redraw
void onDisplay() {
	static int nFrames = 0;
	nFrames++;
	static long tStart = glutGet(GLUT_ELAPSED_TIME);
	long tEnd = glutGet(GLUT_ELAPSED_TIME);
	printf("%d msec\r", (tEnd - tStart) / nFrames);

	glClearColor(1.0f, 0.5f, 0.8f, 1.0f);							// background color
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); // clear the screen

	scene.setUniform(shader);
	fullScreenTexturedQuad.Draw();

	glutSwapBuffers();									// exchange the two buffers
}

// Key of ASCII code pressed
void onKeyboard(unsigned char key, int pX, int pY) {
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

// Idle event indicating that some time elapsed: do animation here
void onIdle() {
	scene.Animate(0.01f);
	glutPostRedisplay();
}