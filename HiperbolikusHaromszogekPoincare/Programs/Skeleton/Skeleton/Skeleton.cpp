//=============================================================================================
// Beltrami-Poincare disc model of hyperbolic geometry
//=============================================================================================
#include "framework.h"

class ImmediateModeRenderer2D : public GPUProgram {
	const char* const vertexSource = R"(
		#version 330
		precision highp float;
		layout(location = 0) in vec2 vertexPosition;	// Attrib Array 0

		void main() { gl_Position = vec4(vertexPosition.x, vertexPosition.y, 0, 1); }	
	)";

	const char* const fragmentSource = R"(
		#version 330
		precision highp float;
		uniform vec3 color;
		out vec4 fragmentColor;	

		void main() { fragmentColor = vec4(color, 1); }
	)";

	unsigned int vao, vbo; // we have just a single vao and vbo for everything :-(

	int Prev(std::vector<vec2> polygon, int i) { return i > 0 ? i - 1 : polygon.size() - 1; }
	int Next(std::vector<vec2> polygon, int i) { return i < polygon.size() - 1 ? i + 1 : 0; }

	bool intersect(vec2 p1, vec2 p2, vec2 q1, vec2 q2) {
		return (dot(cross(p2 - p1, q1 - p1), cross(p2 - p1, q2 - p1)) < 0 &&
			dot(cross(q2 - q1, p1 - q1), cross(q2 - q1, p2 - q1)) < 0);
	}

	bool isEar(const std::vector<vec2>& polygon, int ear) {
		int d1 = Prev(polygon, ear), d2 = Next(polygon, ear);
		vec2 diag1 = polygon[d1], diag2 = polygon[d2];
		for (int e1 = 0; e1 < polygon.size(); e1++) { // test edges for intersection
			int e2 = Next(polygon, e1);
			vec2 edge1 = polygon[e1], edge2 = polygon[e2];
			if (d1 == e1 || d2 == e1 || d1 == e2 || d2 == e2) continue;
			if (intersect(diag1, diag2, edge1, edge2)) return false;
		}
		vec2 center = (diag1 + diag2) / 2.0f; // test middle point for being inside
		vec2 infinity(2.0f, center.y);
		int nIntersect = 0;
		for (int e1 = 0; e1 < polygon.size(); e1++) {
			int e2 = Next(polygon, e1);
			vec2 edge1 = polygon[e1], edge2 = polygon[e2];
			if (intersect(center, infinity, edge1, edge2)) nIntersect++;
		}
		return (nIntersect & 1 == 1);
	}

	void Triangulate(const std::vector<vec2>& polygon, std::vector<vec2>& triangles) {
		if (polygon.size() == 3) {
			triangles.insert(triangles.end(), polygon.begin(), polygon.begin() + 2);
			return;
		}

		std::vector<vec2> newPolygon;
		for (int i = 0; i < polygon.size(); i++) {
			if (isEar(polygon, i)) {
				triangles.push_back(polygon[Prev(polygon, i)]);
				triangles.push_back(polygon[i]);
				triangles.push_back(polygon[Next(polygon, i)]);
				newPolygon.insert(newPolygon.end(), polygon.begin() + i + 1, polygon.end());
				break;
			}
			else newPolygon.push_back(polygon[i]);
		}
		Triangulate(newPolygon, triangles); // recursive call for the rest
	}

	std::vector<vec2> Consolidate(const std::vector<vec2> polygon) {
		const float pixelThreshold = 0.01f;
		vec2 prev = polygon[0];
		std::vector<vec2> consolidatedPolygon = { prev };
		for (auto v : polygon) {
			if (length(v - prev) > pixelThreshold) {
				consolidatedPolygon.push_back(v);
				prev = v;
			}
		}
		if (consolidatedPolygon.size() > 3) {
			if (length(consolidatedPolygon.back() - consolidatedPolygon.front()) < pixelThreshold) consolidatedPolygon.pop_back();
		}
		return consolidatedPolygon;
	}

public:
	ImmediateModeRenderer2D() {
		glViewport(0, 0, windowWidth, windowHeight);
		glLineWidth(2.0f); glPointSize(10.0f);

		create(vertexSource, fragmentSource, "outColor");
		glGenVertexArrays(1, &vao); glBindVertexArray(vao);
		glGenBuffers(1, &vbo); 		glBindBuffer(GL_ARRAY_BUFFER, vbo);
		glEnableVertexAttribArray(0);  // attribute array 0
		glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), NULL);
	}

	void DrawGPU(int type, std::vector<vec2> vertices, vec3 color) {
		setUniform(color, "color");
		glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(vec2), &vertices[0], GL_DYNAMIC_DRAW);
		glDrawArrays(type, 0, vertices.size());
	}

	void DrawPolygon(std::vector<vec2> vertices, vec3 color) {
		std::vector<vec2> triangles;
		Triangulate(Consolidate(vertices), triangles);
		DrawGPU(GL_TRIANGLES, triangles, color);
	}

	~ImmediateModeRenderer2D() {
		glDeleteBuffers(1, &vbo);
		glDeleteVertexArrays(1, &vao);
	}
};

ImmediateModeRenderer2D* renderer; // vertex and fragment shaders
const int nTesselatedVertices = 30;

class HyperbolicLine {
	vec2 center;
	float radius, phi_p, phi_q;
public:
	HyperbolicLine(vec2 p, vec2 q) {
		float p2 = dot(p, p), q2 = dot(q, q), pq = dot(p, q);
		float a = (p2 + 1) / 2.0f, b = (q2 + 1) / 2.0f;
		float denom = (p2 * q2 - pq * pq);
		if (fabs(denom) > 1e-7) center = (p * (q2 * a - pq * b) + q * (p2 * b - pq * a)) / denom;

		vec2 center2p = p - center, center2q = q - center;
		radius = length(center2p);
		phi_p = atan2f(center2p.y, center2p.x);
		phi_q = atan2f(center2q.y, center2q.x);
		if (phi_p - phi_q >= M_PI) phi_p -= 2 * M_PI;
		else if (phi_q - phi_p >= M_PI) phi_q -= 2 * M_PI;
	}

	std::vector<vec2> getTessellation() {
		std::vector<vec2> points(nTesselatedVertices);
		for (int i = 0; i < nTesselatedVertices; i++) {
			float phi = phi_p + (phi_q - phi_p) * (float)i / (nTesselatedVertices - 1.0f);
			points[i] = center + vec2(cosf(phi), sinf(phi)) * radius;
		}
		return points;
	}

	vec2 startDir(vec2 p) { return phi_q > phi_p ? normalize(center - p) : -normalize(center - p); }

	float getLength() {
		float l = -1;
		vec2 pprev;
		for (auto p : getTessellation()) {
			if (l < 0) l = 0;
			else       l += length(p - pprev) / (1 - dot((p + pprev) / 2, (p + pprev) / 2));
			pprev = p;
		}
		return l;
	}
};

class HyperbolicTriangle {
	vec2 p, q, r;
	HyperbolicLine line1, line2, line3;
public:
	HyperbolicTriangle(vec2 _p, vec2 _q, vec2 _r) : line1(_p, _q), line2(_q, _r), line3(_r, _p) {
		p = _p; q = _q; r = _r;
		float alpha = acos(dot(line1.startDir(p), -line3.startDir(p))) * 180 / M_PI;
		float beta = acos(dot(line1.startDir(q), -line2.startDir(q))) * 180 / M_PI;
		float gamma = acos(dot(line2.startDir(r), -line3.startDir(r))) * 180 / M_PI;
		printf("Alpha: %f, Beta: %f, Gamma: %f, Angle sum: %f\n", alpha, beta, gamma, alpha + beta + gamma);
		printf("a: %f, b: %f, c: %f\n", line2.getLength(), line3.getLength(), line1.getLength());
	}

	void Draw() {
		std::vector<vec2> polygon = line1.getTessellation(), l2 = line2.getTessellation(), l3 = line3.getTessellation();
		polygon.insert(polygon.end(), l2.begin(), l2.end());
		polygon.insert(polygon.end(), l3.begin(), l3.end());
		renderer->DrawPolygon(polygon, vec3(0.0f, 0.8f, 0.8f));
		renderer->DrawGPU(GL_LINE_LOOP, polygon, vec3(1, 0.8f, 0.0f));
	}
};

// The virtual world
std::vector<vec2> circlePoints, userPoints;
std::vector<HyperbolicTriangle> triangles;

// Initialization, create an OpenGL context
void onInitialization() {
	renderer = new ImmediateModeRenderer2D();
	for (int i = 0; i < nTesselatedVertices; i++) {
		float phi = i * 2.0f * M_PI / nTesselatedVertices;
		circlePoints.push_back(vec2(cosf(phi), sinf(phi)));
	}
}

// Window has become invalid: Redraw
void onDisplay() {
	glClearColor(0, 0, 0, 0);							// background color 
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); // clear the screen
	renderer->DrawGPU(GL_TRIANGLE_FAN, circlePoints, vec3(0.5f, 0.5f, 0.5f));
	renderer->DrawGPU(GL_POINTS, userPoints, vec3(1, 0, 0));
	for (auto t : triangles) t.Draw();
	glutSwapBuffers();									// exchange the two buffers
}

// Mouse click event
void onMouse(int button, int state, int pX, int pY) {
	if (button == GLUT_LEFT_BUTTON && state == GLUT_DOWN) {  // GLUT_LEFT_BUTTON / GLUT_RIGHT_BUTTON and GLUT_DOWN / GLUT_UP
		printf("px: %d, py: %d\n", pX, pY);
		float cX = 2.0f * pX / windowWidth - 1;	// flip y axis
		float cY = 1.0f - 2.0f * pY / windowHeight;
		if (cX * cX + cY * cY >= 1) return;
		userPoints.push_back(vec2(cX, cY));
		int n = userPoints.size() - 1;
		if (n > 0 && (n + 1) % 3 == 0)
			triangles.push_back(HyperbolicTriangle(userPoints[n], userPoints[n - 1], userPoints[n - 2]));
		glutPostRedisplay();     // redraw
	}
}

// Key of ASCII code pressed
void onKeyboard(unsigned char key, int pX, int pY) {}
// Key of ASCII code released
void onKeyboardUp(unsigned char key, int pX, int pY) {}
// Move mouse with key pressed
void onMouseMotion(int pX, int pY) { }
// Idle event indicating that some time elapsed: do animation here
void onIdle() { }