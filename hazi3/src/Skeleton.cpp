//=============================================================================================

//=============================================================================================
#include "framework.h"

//---------------------------
struct Camera { // 3D camera
//---------------------------
    vec3 wEye, wLookat, wVup;   // extrinsic
    float fov, asp, fp, bp;		// intrinsic
public:
    Camera() {
        asp = (float)windowWidth / windowHeight;
        fov = 75.0f * (float)M_PI / 180.0f;
        fp = 1; bp = 20;
    }
    mat4 V() { // view matrix: translates the center to the origin
        vec3 w = normalize(wEye - wLookat);
        vec3 u = normalize(cross(wVup, w));
        vec3 v = cross(w, u);
        return TranslateMatrix(wEye * (-1)) * mat4(u.x, v.x, w.x, 0,
                                                   u.y, v.y, w.y, 0,
                                                   u.z, v.z, w.z, 0,
                                                   0,   0,   0,   1);
    }

    mat4 P() { // projection matrix
        return mat4(1 / (tan(fov / 2)*asp), 0,                0,                      0,
                    0,                      1 / tan(fov / 2), 0,                      0,
                    0,                      0,                -(fp + bp) / (bp - fp), -1,
                    0,                      0,                -2 * fp*bp / (bp - fp),  0);
    }
};

//---------------------------
struct Material {
//---------------------------
    vec3 kd, ks, ka;
    float shininess;
};

//---------------------------
struct Light {
//---------------------------
    vec3 La, Le;
    vec4 wLightPos; // homogeneous coordinates, can be at ideal point
};

//---------------------------
struct RenderState {
//---------------------------
    mat4	           MVP, M, Minv, V, P;
    Material *         material;
    std::vector<Light> lights;
    vec3	           wEye;
};

//---------------------------
class Shader : public GPUProgram {
//---------------------------
public:
    virtual void Bind(RenderState state) = 0;

    void setUniformMaterial(const Material& material, const std::string& name) {
        setUniform(material.kd, name + ".kd");
        setUniform(material.ks, name + ".ks");
        setUniform(material.ka, name + ".ka");
        setUniform(material.shininess, name + ".shininess");
    }

    void setUniformLight(const Light& light, const std::string& name) {
        setUniform(light.La, name + ".La");
        setUniform(light.Le, name + ".Le");
        setUniform(light.wLightPos, name + ".wLightPos");
    }
};

class CustomShader : public Shader {
//---------------------------
    const char *vertexSource = R"(
		#version 330
		precision highp float;

		struct Light {
			vec3 La, Le;
			vec4 wLightPos;
		};

		struct Material {
			vec3 kd, ks, ka;
			float shininess;
		};

		uniform mat4  MVP, M, Minv;  // MVP, Model, Model-inverse
		uniform Light[8] lights;     // light source direction
		uniform int   nLights;		 // number of light sources
		uniform vec3  wEye;          // pos of eye
		uniform Material  material;  // diffuse, specular, ambient ref

		layout(location = 0) in vec3  vtxPos;            // pos in modeling space
		layout(location = 1) in vec3  vtxNorm;      	 // normal in modeling space

		out vec3 radiance;		    // reflected radiance

		void main() {
			gl_Position = vec4(vtxPos, 1) * MVP; // to NDC
			// radiance computation
			vec4 wPos = vec4(vtxPos, 1) * M;
			vec3 V = normalize(wEye * wPos.w - wPos.xyz);
			vec3 N = normalize((Minv * vec4(vtxNorm, 0)).xyz);
			if (dot(N, V) < 0) N = -N;	// prepare for one-sided surfaces like Mobius or Klein

			radiance = vec3(0, 0, 0);
			for(int i = 0; i < nLights; i++) {
				vec3 L = normalize(lights[i].wLightPos.xyz * wPos.w - wPos.xyz * lights[i].wLightPos.w);
				vec3 H = normalize(L + V);
				float cost = max(dot(N,L), 0), cosd = max(dot(N,H), 0);
				radiance += material.ka * lights[i].La + (material.kd * cost + material.ks * pow(cosd, material.shininess)) * lights[i].Le;
			}
		}
	)";

    // fragment shader in GLSL
    const char *fragmentSource = R"(
		#version 330
		precision highp float;

		in  vec3 radiance;      // interpolated radiance
		out vec4 fragmentColor; // output goes to frame buffer

		void main() {
			fragmentColor = vec4(radiance, 1);
		}
	)";
public:
    CustomShader() { create(vertexSource, fragmentSource, "fragmentColor"); }

    void Bind(RenderState state) {
        Use();        // make this program run
        setUniform(state.MVP, "MVP");
        setUniform(state.M, "M");
        setUniform(state.Minv, "Minv");
        setUniform(state.wEye, "wEye");
        setUniformMaterial(*state.material, "material");

        setUniform((int) state.lights.size(), "nLights");
        for (unsigned int i = 0; i < state.lights.size(); i++) {
            setUniformLight(state.lights[i], std::string("lights[") + std::to_string(i) + std::string("]"));
        }
    }
};


//---------------------------
class Geometry {
//---------------------------
protected:
    unsigned int vao, vbo;        // vertex array object
public:
    Geometry() {
        glGenVertexArrays(1, &vao);
        glBindVertexArray(vao);
        glGenBuffers(1, &vbo); // Generate 1 vertex buffer object
        glBindBuffer(GL_ARRAY_BUFFER, vbo);
    }
    ~Geometry() {
        glDeleteBuffers(1, &vbo);
        glDeleteVertexArrays(1, &vao);
    }

    virtual void Draw() = 0;

    virtual void display() = 0;

};

class Pyramid :  public Geometry {
    int size;
    struct VertexData {
        vec3 position, normal;
    };
    std::vector<VertexData> vtxData;

    void add(vec3 v1, vec3 v2, vec3 v3, vec3 normal) {
        VertexData vtx1;
        vtx1.position = v1;
        vtx1.normal = normal;
        VertexData vtx2;
        vtx2.position = v2;
        vtx2.normal = normal;
        VertexData vtx3;
        vtx3.position = v3;
        vtx3.normal = normal;
        vtxData.push_back(vtx1);
        vtxData.push_back(vtx2);
        vtxData.push_back(vtx3);
    }

    void init() {
        glBufferData(GL_ARRAY_BUFFER, size * sizeof(VertexData), &vtxData[0], GL_STATIC_DRAW);

        glEnableVertexAttribArray(0);  // attribute array 0 = POSITION
        glEnableVertexAttribArray(1);  // attribute array 1 = NORMAL
        // attribute array, components/attribute, component type, normalize?, stride, offset
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(VertexData), (void *) offsetof(VertexData, position));
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(VertexData), (void *) offsetof(VertexData, normal));
    }
public:
    Pyramid() : Geometry() {
        // pyramid vertices
        vec3 v1(0.0f, 0.0f, 0.0f);
        vec3 v2(1.0f, 0.0f, 0.0f);
        vec3 v3(1.0f, 0.0f, 1.0f);
        vec3 v4(0.0f, 0.0f, 1.0f);
        vec3 v5(0.5f, 1.0f, 0.5f);

        // pyramid normals
        vec3 nbase(1.0, 0.0, 1.0);
        vec3 nside1(1.0, 0.0, 0.5);
        vec3 nside2(0.0, 0.0, 0.5);
        vec3 nside3(-1.0, 0.0, -0.5);
        vec3 nside4(0.0, 0.0, -0.5);

        add(v1, v2, v3, nbase);
        add(v1, v4, v3, nbase);
        add(v1, v2, v5, nside1);
        add(v2, v3, v5, nside2);
        add(v3, v4, v5, nside3);
        add(v4, v1, v5, nside4);

        size = vtxData.size();
        init();
    }

    void Draw() {
        glBindVertexArray(vao);
        glDrawArrays(GL_TRIANGLES, 0, size);
    }

    void display() {
        printf("pyramid size: %d\n", vtxData.size());
    }
};

class Base : public Geometry {
    int size;
    struct VertexData {
        vec3 position, normal;
    };
    std::vector<VertexData> vtxData;

    void add(vec3 v1, vec3 v2, vec3 v3, vec3 normal) {
        VertexData vtx1;
        vtx1.position = v1;
        vtx1.normal = normal;
        VertexData vtx2;
        vtx2.position = v2;
        vtx2.normal = normal;
        VertexData vtx3;
        vtx3.position = v3;
        vtx3.normal = normal;
        vtxData.push_back(vtx1);
        vtxData.push_back(vtx2);
        vtxData.push_back(vtx3);
    }

    void init() {
        glBufferData(GL_ARRAY_BUFFER, size * sizeof(VertexData), &vtxData[0], GL_STATIC_DRAW);

        glEnableVertexAttribArray(0);  // attribute array 0 = POSITION
        glEnableVertexAttribArray(1);  // attribute array 1 = NORMAL
        // attribute array, components/attribute, component type, normalize?, stride, offset
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(VertexData), (void *) offsetof(VertexData, position));
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(VertexData), (void *) offsetof(VertexData, normal));
    }
public:
    Base() : Geometry() {
        // base vertices
        vec3 v1(-0.5f, 0.0f, -0.5f);
        vec3 v2(0.5f, 0.0f, -0.5f);
        vec3 v3(0.5f, 0.0f, 0.5f);
        vec3 v4(-0.5f, 0.0f, 0.5f);

        // base normals
        vec3 nbase(0.0, 1.0, 0.0);

        add(v1, v2, v3, nbase);
        add(v1, v4, v3, nbase);

        size = vtxData.size();

        init();
    }

    void Draw() {
        glBindVertexArray(vao);
        glDrawArrays(GL_TRIANGLES, 0, size);
    }

    void display() {

    }
};

class TrackSegment : public Geometry {
    int size;
    struct VertexData {
        vec3 position, normal;
    };
    std::vector<VertexData> vtxData;

    void add(vec3 v1, vec3 v2, vec3 v3, vec3 normal) {
        VertexData vtx1;
        vtx1.position = v1;
        vtx1.normal = normal;
        VertexData vtx2;
        vtx2.position = v2;
        vtx2.normal = normal;
        VertexData vtx3;
        vtx3.position = v3;
        vtx3.normal = normal;
        vtxData.push_back(vtx1);
        vtxData.push_back(vtx2);
        vtxData.push_back(vtx3);
    }

    void init() {
        glBufferData(GL_ARRAY_BUFFER, size * sizeof(VertexData), &vtxData[0], GL_STATIC_DRAW);

        glEnableVertexAttribArray(0);  // attribute array 0 = POSITION
        glEnableVertexAttribArray(1);  // attribute array 1 = NORMAL
        // attribute array, components/attribute, component type, normalize?, stride, offset
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(VertexData), (void *) offsetof(VertexData, position));
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(VertexData), (void *) offsetof(VertexData, normal));
    }
public:
    TrackSegment() : Geometry() {

        vtxData =  std::vector<VertexData>();

        // base vertices
        vec3 v1(-0.5f, 0.0f, -0.5f);
        vec3 v2(0.5f, 0.0f, -0.5f);
        vec3 v3(0.5f, 0.0f, 0.5f);
        vec3 v4(-0.5f, 0.0f, 0.5f);

        // base normals
        vec3 nbase(0.0, 1.0, 0.0);

        add(v1, v2, v3, nbase);
        add(v1, v4, v3, nbase);

        size = vtxData.size();

        init();
    }

    void Draw() {
        glBindVertexArray(vao);
        glDrawArrays(GL_TRIANGLES, 0,  size);
    }

    void display() {
        printf("ts size: %d\n", vtxData.size());
    }
};

//---------------------------
struct Object {
//---------------------------
    Shader *   shader;
    Material * material;
    Geometry * geometry;
    vec3 scale, translation, rotationAxis;
    float rotationAngle;
public:
    Object(Shader * _shader, Material * _material, Geometry * _geometry) :
            scale(vec3(1, 1, 1)), translation(vec3(0, 0, 0)), rotationAxis(0, 0, 1), rotationAngle(0) {
        shader = _shader;
        material = _material;
        geometry = _geometry;
    }

    virtual void SetModelingTransform(mat4& M, mat4& Minv) {
        M = ScaleMatrix(scale) * RotationMatrix(rotationAngle, rotationAxis) * TranslateMatrix(translation);
        Minv = TranslateMatrix(-translation) * RotationMatrix(-rotationAngle, rotationAxis) * ScaleMatrix(vec3(1 / scale.x, 1 / scale.y, 1 / scale.z));
    }

    void Draw(RenderState state) {
        mat4 M, Minv;
        SetModelingTransform(M, Minv);
        state.M = M;
        state.Minv = Minv;
        state.MVP = state.M * state.V * state.P;
        state.material = material;
        shader->Bind(state);
        geometry->Draw();
    }

    virtual void Animate(float tstart, float tend) {
        rotationAngle = 0.8f * tend;
        geometry->display();
    }
};

//---------------------------
class Scene {
//---------------------------
    std::vector<Object *> objects;
    Camera camera; // 3D camera
    std::vector<Light> lights;
public:
    void Build() {
        // Shaders
        Shader * shader = new CustomShader();

        // Materials
        Material * material0 = new Material;
        material0->kd = vec3(0.6f, 0.4f, 0.2f);
        material0->ks = vec3(0.2f, 0.2f, 0.2f);
        material0->shininess = 5;

        Material *materialPyramid = new Material;
        materialPyramid->kd = vec3(0.7f, 0.7f, 0.7f);
        materialPyramid->ks = vec3(0.2f, 0.2f, 0.2f);
        materialPyramid->shininess = 5;

        Material *materialBase = new Material;
        materialBase->kd = vec3(0.76f, 0.682f, 0.117f);
        materialBase->ks = vec3(0.2f, 0.2f, 0.2f);
        materialBase->shininess = 0;

        // Geometries

        Geometry *pyramid = new Pyramid();
        Geometry *base = new Base();
        Geometry *trackSegment1 = new TrackSegment();


        // TODO: Add custom objects

        // Create objects by setting up their vertex data on the GPU
        Object * pyramidObject = new Object(shader, materialPyramid, pyramid);
        pyramidObject->translation = vec3(0, 0, 0);
        pyramidObject->scale = vec3(2.0f, 5.0f, 2.0f);
        pyramidObject->rotationAxis = vec3(0, 1, 0);
        objects.push_back(pyramidObject);

        Object *baseObject = new Object(shader, materialBase, base);
        baseObject->translation = vec3(0, 0, 0);
        baseObject->scale = vec3(10.0f, 10.0f, 10.0f);
        baseObject->rotationAxis = vec3(0, 1, 0);
        objects.push_back(baseObject);

        Object *tsObject = new Object(shader, material0, trackSegment1);
        tsObject->translation = vec3(0, 1, 0);
        tsObject->scale = vec3(1.0f, 1.0f, 1.0f);
        objects.push_back(tsObject);

        // Camera
        camera.wEye = vec3(-8, 2, 0);
        camera.wLookat = vec3(0, 0, 0);
        camera.wVup = vec3(0, 1, 0);

        // Lights
        lights.resize(1);
        lights[0].wLightPos = vec4(5, 5, 4, 0);    // ideal point -> directional light source
        lights[0].Le = vec3(2, 2, 2);
    }

    void Render() {
        RenderState state;
        state.wEye = camera.wEye;
        state.V = camera.V();
        state.P = camera.P();
        state.lights = lights;
        for (Object * obj : objects) obj->Draw(state);
    }

    void Animate(float tstart, float tend) {
        for (Object * obj : objects) obj->Animate(tstart, tend);
    }
};

Scene scene;

// Initialization, create an OpenGL context
void onInitialization() {
    glViewport(0, 0, windowWidth, windowHeight);
    glEnable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);
    scene.Build();
}

// Window has become invalid: Redraw
void onDisplay() {
    glClearColor(0.5f, 0.5f, 0.8f, 1.0f);							// background color
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); // clear the screen
    scene.Render();
    glutSwapBuffers();									// exchange the two buffers
}

// Key of ASCII code pressed
void onKeyboard(unsigned char key, int pX, int pY) { }

// Key of ASCII code released
void onKeyboardUp(unsigned char key, int pX, int pY) { }

// Mouse click event
void onMouse(int button, int state, int pX, int pY) { }

// Move mouse with key pressed
void onMouseMotion(int pX, int pY) {
}

// Idle event indicating that some time elapsed: do animation here
void onIdle() {
    static float tend = 0;
    const float dt = 0.1f; // dt is ”infinitesimal”
    float tstart = tend;
    tend = glutGet(GLUT_ELAPSED_TIME) / 1000.0f;

    for (float t = tstart; t < tend; t += dt) {
        float Dt = fmin(dt, tend - t);
        scene.Animate(t, t + Dt);
    }
    glutPostRedisplay();
}