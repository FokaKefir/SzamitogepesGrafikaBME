//=============================================================================================
// Nev:		Babos David
// Neptun:	Q1CGY7
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
		glLineWidth(2.0f); glPointSize(10.0f);

		create(vertexSource, fragmentSource, "outColor");
		glGenVertexArrays(1, &vao); glBindVertexArray(vao);
		glGenBuffers(1, &vbo); 		glBindBuffer(GL_ARRAY_BUFFER, vbo);
		glEnableVertexAttribArray(0);  // attribute array 0
		glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), NULL);
	}

	void DrawGPU(int type, std::vector<vec2> vertices, vec3 color) {
		glBindVertexArray(vao);
		glBindBuffer(GL_ARRAY_BUFFER, vbo);

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

const int nTesselatedVertices = 50;

class Hyperbolic {
public:
	// Poincare
	static vec3 poincareToHyperbolic(vec2 poincarePoint) {
		float d = 1 - poincarePoint.x * poincarePoint.x - poincarePoint.y * poincarePoint.y;
		vec3 hypPoint;
		hypPoint.x = 2 * poincarePoint.x / d;
		hypPoint.y = 2 * poincarePoint.y / d;
		hypPoint.z = 2 / d - 1;
		return hypPoint;
	}

	static vec2 hyperbolicToPoinceare(vec3 hypPoint) {
		vec2 poincarePoint;
		poincarePoint.x = hypPoint.x / (hypPoint.z + 1);
		poincarePoint.y = hypPoint.y / (hypPoint.z + 1);
		return poincarePoint;
	}

	static std::vector<vec3> poincareToHyperbolic(std::vector<vec2> poincarePoints) {
		std::vector<vec3> hypPoints;
		for (vec2 poincarePoint : poincarePoints) {
			hypPoints.push_back(poincareToHyperbolic(poincarePoint));
		}
		return hypPoints;
	}

	static std::vector<vec2> hyperbolicToPoinceare(std::vector<vec3> hypPoints) {
		std::vector<vec2> poincarePoints;
		for (vec3 hypPoint : hypPoints) {
			poincarePoints.push_back(hyperbolicToPoinceare(hypPoint));
		}
		return poincarePoints;
	}

	// Klein
	static vec3 kleinToHyperbolic(vec2 kleinPoint) {
		float d = sqrtf(1.0 - kleinPoint.x * kleinPoint.x - kleinPoint.y * kleinPoint.y);
		vec3 hypPoint;
		hypPoint.x = kleinPoint.x / d;
		hypPoint.y = kleinPoint.y / d;
		hypPoint.z = 1.0 / d;
		return hypPoint;
	}

	static vec2 hyperbolicToKlein(vec3 hypPoint) {
		vec2 kleinPoint;
		kleinPoint.x = hypPoint.x / hypPoint.z;
		kleinPoint.y = hypPoint.y / hypPoint.z;
		return kleinPoint;
	}

	static std::vector<vec3> kleinToHyperbolic(std::vector<vec2> kleinPoints) {
		std::vector<vec3> hypPoints;
		for (vec2 kleinPoint : kleinPoints) {
			hypPoints.push_back(kleinToHyperbolic(kleinPoint));
		}
		return hypPoints;
	}

	static std::vector<vec2> hyperbolicToKlein(std::vector<vec3> hypPoints) {
		std::vector<vec2> kleinPoints;
		for (vec3 hypPoint : hypPoints) {
			kleinPoints.push_back(hyperbolicToKlein(hypPoint));
		}
		return kleinPoints;
	}

	// SideView

	// BottomView

	
	static vec3 bottomToHyperbolic(vec2 bottomPoint) {
		return vec3(bottomPoint.x, bottomPoint.y, sqrtf(1 + bottomPoint.x * bottomPoint.x + bottomPoint.y * bottomPoint.y));
	}
	
	
	static vec2 hyperbolicToBottom(vec3 hypPoint) {
		return vec2(hypPoint.x, hypPoint.y);
	}

	
	static std::vector<vec3> bottomToHyperbolic(std::vector<vec2> bottomPoints) {
		std::vector<vec3> hypPoints;
		for (vec2 bottomPoint : bottomPoints) {
			hypPoints.push_back(bottomToHyperbolic(bottomPoint));
		}
		return hypPoints;

	}

	static std::vector<vec2> hyperbolicToBottom(std::vector<vec3> hypPoints) {
		std::vector<vec2> bottomPoints;
		for (vec3 hypPoint : hypPoints) {
			bottomPoints.push_back(hyperbolicToBottom(hypPoint));
		}
		return bottomPoints;
	}


};


class HyperbolicCircle {
private:
	vec2 mid; // Poincare coord
	float rad; // Poincare 
	bool ok;
public:
	HyperbolicCircle(vec3 hypp, vec3 hypq, vec3 hypr) {
		vec2 p = Hyperbolic::hyperbolicToPoinceare(hypp),
			q = Hyperbolic::hyperbolicToPoinceare(hypq),
			r = Hyperbolic::hyperbolicToPoinceare(hypr);
		vec2 pq((p.x + q.x) / 2, (p.y + q.y) / 2);
		float mpq = (p.y - q.y) / (p.x - q.x);
		float bpq = pq.y - mpq * pq.x;
		float npq = -1 / mpq;
		float cpq = pq.y - npq * pq.x;

		vec2 pr((p.x + r.x) / 2, (p.y + r.y) / 2);
		float mpr = (p.y - r.y) / (p.x - r.x);
		float bpr = pr.y - mpr * pr.x;
		float npr = -1 / mpr;
		float cpr = pr.y - npr * pr.x;

		float midX = (cpr - cpq) / (npq - npr);
		float midY = npq * midX + cpq;

		mid.x = midX;
		mid.y = midY;

		rad = length(mid - p);

		if (length(mid) >= 1 - rad)
			ok = false;
		else
			ok = true;
	}

	std::vector<vec3> getPolygonPoints() {
		std::vector<vec2> circlePoints; // Poincare
		for (int i = 0; i < nTesselatedVertices; i++) {
			float phi = i * 2.0f * M_PI / nTesselatedVertices;
			circlePoints.push_back(
				vec2(
					mid.x + rad * cos(phi),
					mid.y + rad * sin(phi))
			);
		}
		return Hyperbolic::poincareToHyperbolic(circlePoints);
	}

	bool isOk() {
		return ok;
	}
};

class HyperbolicLine {
private:
	vec2 p, q; // Klein coords
public:
	HyperbolicLine(vec3 hypPoint1, vec3 hypPoint2) {
		vec2 point1 = Hyperbolic::hyperbolicToKlein(hypPoint1);
		vec2 point2 = Hyperbolic::hyperbolicToKlein(hypPoint2);

		float rad = 0.99;

		float m = (point1.y - point2.y) / (point1.x - point2.x);
		float x1 = point1.x;
		float y1 = point1.y;

		float a = (m * m + 1);
		float b = (2 * m * y1 - 2 * x1 * m * m);
		float c = (m * m * x1 * x1 - 2 * m * y1 * x1 + y1 * y1 - rad * rad);

		float delta = b * b - 4 * a * c;
		p.x = (-b - sqrtf(delta)) / (2 * a);
		p.y = m * (p.x - x1) + y1;
		q.x = (-b + sqrtf(delta)) / (2 * a);
		q.y = m * (q.x - x1) + y1;
	}

	std::vector<vec3> getPolygonPoints() {
		std::vector<vec2> linePoints; // Klein
		float m = (q.y - p.y) / (q.x - p.x);
		for (int i = 0; i <= nTesselatedVertices; i++) {
			vec2 ppoint;
			ppoint.x = p.x + i * (q.x - p.x) / nTesselatedVertices;
			ppoint.y = m * (ppoint.x - p.x) + p.y;
			linePoints.push_back(ppoint);
		}
		return Hyperbolic::kleinToHyperbolic(linePoints);
	}
};

class Poincare {
private:
	ImmediateModeRenderer2D* renderer;
	std::vector<vec2> circlePoints;
	int x, y, width, height;

public:
	Poincare(int _x, int _y, int _width, int _height) : x(_x), y(_y), width(_width), height(_height) {
		renderer = new ImmediateModeRenderer2D(); // vertex and fragment shaders
		for (int i = 0; i < nTesselatedVertices; i++) {
			float phi = i * 2.0f * M_PI / nTesselatedVertices;
			circlePoints.push_back(vec2(cosf(phi), sinf(phi)));
		}
	}

	~Poincare() {
		delete renderer;
	}

	void DrawInit() {
		glViewport(x, y, width, height);
		renderer->DrawGPU(GL_TRIANGLE_FAN, circlePoints, vec3(0.5f, 0.5f, 0.5f));
	}

	void DrawPoints(std::vector<vec3> hypUserPoints, vec3 color) {
		renderer->DrawGPU(GL_POINTS, Hyperbolic::hyperbolicToPoinceare(hypUserPoints), color);
	}

	void DrawCircles(std::vector<HyperbolicCircle> circles, vec3 circleColor, vec3 lineColor) {
		for (HyperbolicCircle circle : circles) {
			std::vector<vec3> hypPolygon = circle.getPolygonPoints();
			std::vector<vec2> poincarePolygon = Hyperbolic::hyperbolicToPoinceare(hypPolygon);
			renderer->DrawPolygon(poincarePolygon, circleColor);
			renderer->DrawGPU(GL_LINE_LOOP, poincarePolygon, lineColor);
		}
	}

	void DrawLines(std::vector<HyperbolicLine> lines, vec3 lineColor) {
		for (HyperbolicLine line: lines) {
			std::vector<vec3> hypPolygon = line.getPolygonPoints();
			std::vector<vec2> poincarePolygon = Hyperbolic::hyperbolicToPoinceare(hypPolygon);
			renderer->DrawGPU(GL_LINE_STRIP, poincarePolygon, lineColor);
		}
	}
};

class Klein {
private:
	ImmediateModeRenderer2D* renderer;
	std::vector<vec2> circlePoints;
	int x, y, width, height;

public:
	Klein(int _x, int _y, int _width, int _height) : x(_x), y(_y), width(_width), height(_height) {
		renderer = new ImmediateModeRenderer2D(); // vertex and fragment shaders
		for (int i = 0; i < nTesselatedVertices; i++) {
			float phi = i * 2.0f * M_PI / nTesselatedVertices;
			circlePoints.push_back(vec2(cosf(phi), sinf(phi)));
		}
	}

	~Klein() {
		delete renderer;
	}

	void DrawInit() {
		glViewport(x, y, width, height);
		renderer->DrawGPU(GL_TRIANGLE_FAN, circlePoints, vec3(0.5f, 0.5f, 0.5f));
	}

	void DrawPoints(std::vector<vec3> hypUserPoints, vec3 color) {
		renderer->DrawGPU(GL_POINTS, Hyperbolic::hyperbolicToKlein(hypUserPoints), color);
	}

	void DrawCircles(std::vector<HyperbolicCircle> circles, vec3 circleColor, vec3 lineColor) {
		for (HyperbolicCircle circle : circles) {
			std::vector<vec3> hypPolygon = circle.getPolygonPoints();
			std::vector<vec2> kleinPolygon = Hyperbolic::hyperbolicToKlein(hypPolygon);
			renderer->DrawPolygon(kleinPolygon, circleColor);
			renderer->DrawGPU(GL_LINE_LOOP, kleinPolygon, lineColor);
		}
	}

	void DrawLines(std::vector<HyperbolicLine> lines, vec3 lineColor) {
		for (HyperbolicLine line : lines) {
			std::vector<vec3> hypPolygon = line.getPolygonPoints();
			std::vector<vec2> kleinePolygon = Hyperbolic::hyperbolicToKlein(hypPolygon);
			renderer->DrawGPU(GL_LINE_STRIP, kleinePolygon, lineColor);
		}
	}
};

class SideView {
	ImmediateModeRenderer2D* renderer;
	int x, y, width, height;

public:
	SideView(int _x, int _y, int _width, int _height) : x(_x), y(_y), width(_width), height(_height){
		renderer = new ImmediateModeRenderer2D(); // vertex and fragment shaders
		
	}

	~SideView() {
		delete renderer;
	}

	void DrawInit() {
		glViewport(x, y, width, height);
	}

	void DrawPoints(std::vector<vec3> hypUserPoints, vec3 color) {}

	void DrawCircle(std::vector<HyperbolicCircle> circles) {}
};

class BottomView {
	ImmediateModeRenderer2D* renderer;
	int x, y, width, height;

public:
	BottomView(int _x, int _y, int _width, int _height) : x(_x), y(_y), width(_width), height(_height) {
		renderer = new ImmediateModeRenderer2D(); // vertex and fragment shaders
	}

	~BottomView() {
		delete renderer;
	}

	void DrawInit() {
		glViewport(x, y, width, height);
	}

	void DrawPoints(std::vector<vec3> hypUserPoints, vec3 color) {
		renderer->DrawGPU(GL_POINTS, Hyperbolic::hyperbolicToBottom(hypUserPoints), color);
	}

	void DrawCircles(std::vector<HyperbolicCircle> circles, vec3 circleColor, vec3 lineColor) {
		for (HyperbolicCircle circle : circles) {
			std::vector<vec3> hypPolygon = circle.getPolygonPoints();
			std::vector<vec2> bottomPolygon = Hyperbolic::hyperbolicToBottom(hypPolygon);
			renderer->DrawPolygon(bottomPolygon, circleColor);
			renderer->DrawGPU(GL_LINE_LOOP, bottomPolygon, lineColor);
		}
	}

	void DrawLines(std::vector<HyperbolicLine> lines, vec3 lineColor) {
		for (HyperbolicLine line : lines) {
			std::vector<vec3> hypPolygon = line.getPolygonPoints();
			std::vector<vec2> bottomPolygon = Hyperbolic::hyperbolicToBottom(hypPolygon);
			renderer->DrawGPU(GL_LINE_STRIP, bottomPolygon, lineColor);
		}
	}
};



// The virtual world
std::vector<vec3> userPoints;
std::vector<vec3> placedPoints;
std::vector<HyperbolicCircle> circles;
std::vector<HyperbolicLine> lines;

// Views
Poincare* poincare;
Klein* klein;
SideView* side;
BottomView* bottom;


// Initialization, create an OpenGL context
void onInitialization() {
	poincare = new Poincare(0, windowHeight / 2, windowWidth / 2, windowHeight / 2);
	klein = new Klein(windowWidth / 2, windowHeight / 2, windowWidth / 2, windowHeight / 2);
	side = new SideView(0, 0, windowWidth / 2, windowHeight / 2);
	bottom = new BottomView(windowWidth / 2, 0, windowWidth / 2, windowHeight / 2);
}

// Window has become invalid: Redraw
void onDisplay() {
	glClearColor(0.8f, 0.8f, 0.8f, 0);					// background color 
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); // clear the screen

	poincare->DrawInit();
	poincare->DrawCircles(circles, vec3(0.439216, 0.858824, 0.858824), vec3(1, 1, 1));
	poincare->DrawPoints(userPoints, vec3(0, 0, 1));
	poincare->DrawPoints(placedPoints, vec3(1, 0, 0));
	poincare->DrawLines(lines, vec3(1, 1, 1));

	klein->DrawInit();
	klein->DrawCircles(circles, vec3(0.439216, 0.858824, 0.858824), vec3(1, 1, 1));
	klein->DrawPoints(userPoints, vec3(0, 0, 1));
	klein->DrawPoints(placedPoints, vec3(1, 0, 0));
	klein->DrawLines(lines, vec3(1, 1, 1));

	side->DrawInit();

	bottom->DrawInit();
	bottom->DrawCircles(circles, vec3(0.439216, 0.858824, 0.858824), vec3(1, 1, 1));
	bottom->DrawPoints(userPoints, vec3(0, 0, 1));
	bottom->DrawPoints(placedPoints, vec3(1, 0, 0));
	bottom->DrawLines(lines, vec3(1, 1, 1));
	
	glutSwapBuffers();									// exchange the two buffers
}

float convertLinear(float a, float b, float c, float d, float x) {
	return ((d - c) / (b - a)) * (x - a) + c;
}

// Mouse click event
void onMouse(int button, int state, int pX, int pY) {
	if (button == GLUT_LEFT_BUTTON && state == GLUT_DOWN) {  // GLUT_LEFT_BUTTON / GLUT_RIGHT_BUTTON and GLUT_DOWN / GLUT_UP
		printf("px: %d, py: %d\n", pX, pY);
		float cX = 2.0f * pX / windowWidth - 1;
		float cY = 1.0f - 2.0f * pY / windowHeight;

		vec3 hypPoint;

		//Kepernyo 4-be osztasa
		if (cX < 0 && cY < 0) { // Oldalnezet
			float x = convertLinear(-1, 0, -1, 1, cX);
			float y = convertLinear(-1, 0, -1, 1, cY);
			printf("Oldalnezet: %f %f\n", x, y);
			return;
		}
		else if (cX > 0 && cY < 0) { // Alulnezet
			float x = convertLinear(0, 1, -1, 1, cX);
			float y = convertLinear(-1, 0, -1, 1, cY);
			hypPoint = Hyperbolic::bottomToHyperbolic(vec2(x, y));
		}
		else if (cX < 0 && cY > 0) { // Poincare
			float x = convertLinear(-1, 0, -1, 1, cX);
			float y = convertLinear(0, 1, -1, 1, cY);
			if (x * x + y * y >= 1)
				return;
			hypPoint = Hyperbolic::poincareToHyperbolic(vec2(x, y));
		}
		else if (cX > 0 && cY > 0) { // Klein
			float x = convertLinear(0, 1, -1, 1, cX);
			float y = convertLinear(0, 1, -1, 1, cY);
			if (x* x + y * y >= 1)
				return;
			hypPoint = Hyperbolic::kleinToHyperbolic(vec2(x, y));
		}
		
		userPoints.push_back(hypPoint);
		
		glutPostRedisplay();     // redraw
	}

	if (button == GLUT_RIGHT_BUTTON && state == GLUT_DOWN) {
		if (userPoints.empty()) {
			return;
		}

		int n = userPoints.size();
		if (n == 1) { // Place point
			vec3 point = userPoints[0];
			userPoints.clear();
			placedPoints.push_back(point);
		}
		else if (n == 2) { // Place line
			vec3 point1 = userPoints[0];
			vec3 point2 = userPoints[1];
			userPoints.clear();
			lines.push_back(HyperbolicLine(point1, point2));
			placedPoints.push_back(point1);
			placedPoints.push_back(point2);
		}
		else if (n >= 3) { // Place circle
			vec3 point1 = userPoints[n - 1];
			vec3 point2 = userPoints[n - 2];
			vec3 point3 = userPoints[n - 3];
			userPoints.pop_back();
			userPoints.pop_back();
			userPoints.pop_back();
			
			HyperbolicCircle circle = HyperbolicCircle(point1, point2, point3);
			if (circle.isOk() == false) {
				glutPostRedisplay();
				return;
			}

			circles.push_back(circle);
			placedPoints.push_back(point1);
			placedPoints.push_back(point2);
			placedPoints.push_back(point3);
		}
		glutPostRedisplay();
	}
	
}

// Key of ASCII code pressed
void onKeyboard(unsigned char key, int pX, int pY) {}
// Key of ASCII code released
void onKeyboardUp(unsigned char key, int pX, int pY) {}
// Move mouse with key pressed
void onMouseMotion(int pX, int pY) {}
// Idle event indicating that some time elapsed: do animation here
void onIdle() { }