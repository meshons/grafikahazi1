//=============================================================================================
// Mintaprogram: Zöld háromszög. Ervenyes 2018. osztol.
//
// A beadott program csak ebben a fajlban lehet, a fajl 1 byte-os ASCII karaktereket tartalmazhat, BOM kihuzando.
// Tilos:
// - mast "beincludolni", illetve mas konyvtarat hasznalni
// - faljmuveleteket vegezni a printf-et kiveve
// - Mashonnan atvett programresszleteket forrasmegjeloles nelkul felhasznalni es
// - felesleges programsorokat a beadott programban hagyni!!!!!!!
// - felesleges kommenteket a beadott programba irni a forrasmegjelolest kommentjeit kiveve
// ---------------------------------------------------------------------------------------------
// A feladatot ANSI C++ nyelvu forditoprogrammal ellenorizzuk, a Visual Studio-hoz kepesti elteresekrol
// es a leggyakoribb hibakrol (pl. ideiglenes objektumot nem lehet referencia tipusnak ertekul adni)
// a hazibeado portal ad egy osszefoglalot.
// ---------------------------------------------------------------------------------------------
// A feladatmegoldasokban csak olyan OpenGL fuggvenyek hasznalhatok, amelyek az oran a feladatkiadasig elhangzottak
// A keretben nem szereplo GLUT fuggvenyek tiltottak.
//
// NYILATKOZAT
// ---------------------------------------------------------------------------------------------
// Nev    : Stork Gábor
// Neptun : NO047V
// ---------------------------------------------------------------------------------------------
// ezennel kijelentem, hogy a feladatot magam keszitettem, es ha barmilyen segitseget igenybe vettem vagy
// mas szellemi termeket felhasznaltam, akkor a forrast es az atvett reszt kommentekben egyertelmuen jeloltem.
// A forrasmegjeloles kotelme vonatkozik az eloadas foliakat es a targy oktatoi, illetve a
// grafhazi doktor tanacsait kiveve barmilyen csatornan (szoban, irasban, Interneten, stb.) erkezo minden egyeb
// informaciora (keplet, program, algoritmus, stb.). Kijelentem, hogy a forrasmegjelolessel atvett reszeket is ertem,
// azok helyessegere matematikai bizonyitast tudok adni. Tisztaban vagyok azzal, hogy az atvett reszek nem szamitanak
// a sajat kontribucioba, igy a feladat elfogadasarol a tobbi resz mennyisege es minosege alapjan szuletik dontes.
// Tudomasul veszem, hogy a forrasmegjeloles kotelmenek megsertese eseten a hazifeladatra adhato pontokat
// negativ elojellel szamoljak el es ezzel parhuzamosan eljaras is indul velem szemben.
//=============================================================================================
#include "framework.h"

// vertex shader in GLSL: It is a Raw string (C++11) since it contains new line characters
const char * const vertexSource = R"(
	#version 330				// Shader 3.3
	precision highp float;		// normal floats, makes no difference on desktop computers

	uniform mat4 MVP;			// uniform variable, the Model-View-Projection transformation matrix
	layout(location = 0) in vec2 vp;	// Varying input: vp = vertex position is expected in attrib array 0
	layout(location = 1) in vec3 vertexColor;	    // Attrib Array 1

    out vec3 color;

	void main() {
        color = vertexColor;
		gl_Position = vec4(vp.x, vp.y, 0, 1) * MVP;		// transform vp from modeling space to normalized device space
	}
)";

// fragment shader in GLSL
const char * const fragmentSource = R"(
	#version 330			// Shader 3.3
	precision highp float;	// normal floats, makes no difference on desktop computers

	in vec3 color;		// uniform variable, the color of the primitive
	out vec4 fragmentColor;		// output that goes to the raster memory as told by glBindFragDataLocation

	void main() {
		fragmentColor = vec4(color, 1); // extend RGB to RGBA
	}
)";


// komlex számok alapján
// http://cg.iit.bme.hu/portal/sites/default/files/oktatott%20t%C3%A1rgyak/sz%C3%A1m%C3%ADt%C3%B3g%C3%A9pes%20grafika/geometri%C3%A1k%20%C3%A9s%20algebr%C3%A1k/bmegeom.pdf
vec2 forgatas(vec2 &point, float angle, vec2 &pivot) {
    vec2 eltolt1 = point - pivot;
    vec2 forgas(cosf(angle), sinf(angle));
    return vec2(
            eltolt1.x * forgas.x - eltolt1.y * forgas.y,
            eltolt1.x * forgas.y + eltolt1.y * forgas.x
    ) + pivot;
}

class Camera {
    vec2 center;
public:
    Camera(): center(0, 0) { }

    vec2 eltolas() {
        return center;
    }
};

Camera camera;
GPUProgram gpuProgram; // vertex and fragment shaders

class Kerek {
    GLuint vao;
    GLuint vbo[2];
    vec2 center;
    float R;
    vec2 alapPontok[122];
    vec2 pontok[122];
    vec3 szinek[122];
    vec2 alapKullopontok[14];
    vec2 kullopontok[14];
    vec3 kulloszinek[14];
    float irany = -1;
    float state;
public:
    Kerek(vec2 center, float R): center(center), R(R), state(0) { }

    void Create() {
        glGenVertexArrays(1, &vao);
        glBindVertexArray(vao);

        alapPontok[0] = center + vec2(0, R);
        alapPontok[1] = center + vec2(0, R*0.8f);
        for (int i=2; i < 120; i+=2)
        {
            alapPontok[i] = forgatas(alapPontok[0], M_PI/15*i/2, center);
            alapPontok[i+1] = forgatas(alapPontok[1], M_PI/15*i/2, center);
        }
        alapPontok[120] = alapPontok[0];
        alapPontok[121] = alapPontok[1];

        alapKullopontok[0] = center + vec2(0, R*0.95f);
        alapKullopontok[1] = center - vec2(0, R*0.95f);
        for (int i=1; i < 7; ++i) {
            alapKullopontok[2*i] = forgatas(alapKullopontok[0], 2.0f/7*M_PI*i ,center);
            alapKullopontok[2*i+1] = forgatas(alapKullopontok[1], 2.0f/7*M_PI*i ,center);
        }

        glGenBuffers(2, &vbo[0]);
        glBindBuffer(GL_ARRAY_BUFFER, vbo[0]);

        glBufferData(GL_ARRAY_BUFFER, sizeof(alapPontok) + sizeof(alapKullopontok), alapPontok, GL_DYNAMIC_DRAW);
        glBufferSubData(GL_ARRAY_BUFFER, sizeof(alapPontok), sizeof(alapKullopontok), alapKullopontok);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, NULL);

        szinek[0] = {0.51f, 0.51f, 0.51f};
        szinek[1] = {0, 0, 0};

        for(int i=1; i<122/2; ++i){
            szinek[i*2] = szinek[0];
            szinek[i*2+1] = szinek[1];
        }

        for (auto & kulloszin: kulloszinek)
            kulloszin = {0.3f, 0.3f, 0.3f};

        glBindBuffer(GL_ARRAY_BUFFER, vbo[1]);
        glBufferData(GL_ARRAY_BUFFER, sizeof(szinek) + sizeof(kulloszinek), szinek, GL_STATIC_DRAW);
        glBufferSubData(GL_ARRAY_BUFFER, sizeof(szinek), sizeof(kulloszinek), kulloszinek);

        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, NULL);
    }

    void Draw() {
        glBindVertexArray(vao);

        glBindBuffer(GL_ARRAY_BUFFER, vbo[0]);
        for (int i=0; i < 122; ++i){
            pontok[i] = forgatas(alapPontok[i], irany*state*M_PI*2 , center);
        }
        for (int i=0; i < 14; ++i)
            kullopontok[i] = forgatas(alapKullopontok[i], irany*state*M_PI*2 ,center);
        glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(pontok), pontok);
        glBufferSubData(GL_ARRAY_BUFFER, sizeof(pontok), sizeof(kullopontok), kullopontok);

        glLineWidth(0.8f); // Width of lines in pixels
        glDrawArrays(GL_LINES, 122, 14);
        glLineWidth(1.0f); // Width of lines in pixels
        glDrawArrays(GL_TRIANGLE_STRIP, 0, 62);
        //glDrawArrays(GL_TRIANGLE_STRIP, 60, 62);
    }

    void allapot(float allapot){
        state=allapot;
        while(state>1.0)
            state-=1.0;
    }

    void iranyvaltas(float ujirany) {
        irany = ujirany;
    }

    void mozgat(vec2 hova){
        center = center + hova;
        for (auto & pont : alapPontok)
            pont = pont + hova;
        for (auto & pont : kullopontok)
            pont = pont + hova;
    }
};

class Fej {
    GLuint vao;
    GLuint vbo[2];
    vec2 pontok[40];
    vec3 szin;
    vec2 center;
    float R;
public:
    Fej(vec2 center, float R): center(center), R(R) {}

    void Create() {
        glGenVertexArrays(1, &vao);
        glBindVertexArray(vao);

        pontok[0] = center + vec2(0, R);
        for (int i=1; i < 40; ++i)
        {
            pontok[i] = forgatas(pontok[0], M_PI/20*i ,center);
        }

        glGenBuffers(2, &vbo[0]);
        glBindBuffer(GL_ARRAY_BUFFER, vbo[0]);

        glBufferData(GL_ARRAY_BUFFER, sizeof(pontok), pontok, GL_DYNAMIC_DRAW);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, NULL);

        szin = {0, 1, 0};

        glBindBuffer(GL_ARRAY_BUFFER, vbo[1]);
        glBufferData(GL_ARRAY_BUFFER, sizeof(szin)*40, nullptr, GL_STATIC_DRAW);

        for (int i=0; i<40; ++i)
            glBufferSubData(GL_ARRAY_BUFFER, sizeof(szin)*i, sizeof(szin), &szin);

        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, NULL);
    }

    void Draw() {
        glBindVertexArray(vao);

        glBindBuffer(GL_ARRAY_BUFFER, vbo[0]);

        glLineWidth(2.0f);
        glDrawArrays(GL_LINE_LOOP, 0, 40);
    }

    void mozgat(vec2 hova){
        center = center + hova;
        for (auto & pont : pontok)
            pont = pont + hova;
    }
};

class Ember {
    GLuint vao;
    GLuint vbo[2];
    vec2 position; // kerék középpontja
    vec2 testPontok[7];
    vec2 labfejPontok[2];
    vec3 testSzinek[7];
    Kerek kerek;
    Fej fej;
    float state = 0;
    float irany = -1;
    float labszarhossz = 0.10f;
    float combhossz = 0.12f;
public:
    Ember(): position(0,0), kerek(position, 0.10f), fej(position+vec2(0, 0.345f), 0.045f) {}

    void Create() {
        glGenVertexArrays(1, &vao);
        glBindVertexArray(vao);

        testPontok[0] = {position.x, position.y+0.3f};
        testPontok[1] = {position.x, position.y+0.14f};

        for (auto & testSzin: testSzinek)
            testSzin = {0, 0, 0};

        labfejPontok[0] = position + vec2(0, 0.03f);
        labfejPontok[1] = position - vec2(0, 0.03f);

        glGenBuffers(2, &vbo[0]);
        glBindBuffer(GL_ARRAY_BUFFER, vbo[0]);

        glBufferData(GL_ARRAY_BUFFER, sizeof(testPontok), testPontok, GL_DYNAMIC_DRAW);

        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, NULL);

        glBindBuffer(GL_ARRAY_BUFFER, vbo[1]);
        glBufferData(GL_ARRAY_BUFFER, sizeof(testSzinek), testSzinek, GL_STATIC_DRAW);

        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, NULL);

        kerek.Create();
        fej.Create();
    }

    void Animate(long time) {
        state = float(time % 10000) / 10000.f;
        kerek.allapot(state);
    }

    void Draw() {
        glBindVertexArray(vao);

        testPontok[3] = forgatas(labfejPontok[0], irany*state*M_PI*2, position);
        testPontok[6] = forgatas(labfejPontok[1], irany*state*M_PI*2, position);
        testPontok[4] = testPontok[1];

        float labszarfenektav1 = sqrtf(
                (testPontok[1].x - testPontok[3].x) * (testPontok[1].x - testPontok[3].x) +
                        (testPontok[1].y - testPontok[3].y) * (testPontok[1].y - testPontok[3].y)
        );
        float labszarfenektav2 = sqrtf(
                (testPontok[1].x - testPontok[6].x) * (testPontok[1].x - testPontok[6].x) +
                (testPontok[1].y - testPontok[6].y) * (testPontok[1].y - testPontok[6].y)
        );
        float valami = (labszarfenektav1*labszarfenektav1 + combhossz*combhossz - labszarhossz*labszarhossz)/
                       (2*labszarfenektav1*combhossz);
        float szog1 = acosf(valami);
        float valami2 = (labszarfenektav2*labszarfenektav2 + combhossz*combhossz - labszarhossz*labszarhossz)/
                (2*labszarfenektav2*combhossz);
        float szog2 = acosf(valami2);

        testPontok[2] = normalize(testPontok[3] - testPontok[1]) * combhossz + testPontok[1];
        testPontok[5] = normalize(testPontok[6] - testPontok[1]) * combhossz + testPontok[1];
        testPontok[2] = forgatas(testPontok[2], szog1, testPontok[1]);
        testPontok[5] = forgatas(testPontok[5], szog2, testPontok[1]);

        glBindBuffer(GL_ARRAY_BUFFER, vbo[0]);
        glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(testPontok), testPontok);

        glLineWidth(3.0f); // Width of lines in pixels
        glDrawArrays(GL_LINE_STRIP, 0, 2);
        glDrawArrays(GL_LINE_STRIP, 1, 3);
        kerek.Draw();
        glBindVertexArray(vao);
        glLineWidth(3.0f); // Width of lines in pixels
        glDrawArrays(GL_LINE_STRIP, 4, 3);
        fej.Draw();
    }

};

Ember ember;

// Initialization, create an OpenGL context
void onInitialization() {
    glViewport(0, 0, windowWidth, windowHeight);

    ember.Create();

    // create program for the GPU
    gpuProgram.Create(vertexSource, fragmentSource, "fragmentColor");
}

// Window has become invalid: Redraw
void onDisplay() {
    glClearColor(1, 1, 1, 1);     // background color
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); // clear frame buffer

    float MVPtransf[4][4] = { 1, 0, 0, 0,    // MVP matrix,
                              0, 1, 0, 0,    // row-major!
                              0, 0, 1, 0,
                              0, 0, 0, 1 };

    int location = glGetUniformLocation(gpuProgram.getId(), "MVP");	// Get the GPU location of uniform variable MVP
    glUniformMatrix4fv(location, 1, GL_TRUE, &MVPtransf[0][0]);	// Load a 4x4 row-major float matrix to the specified location

    ember.Draw();

    glutSwapBuffers(); // exchange buffers for double buffering
}

// Key of ASCII code pressed
void onKeyboard(unsigned char key, int pX, int pY) {
    if (key == 'd') glutPostRedisplay();         // if d, invalidate display, i.e. redraw
}

// Key of ASCII code released
void onKeyboardUp(unsigned char key, int pX, int pY) {
}

// Move mouse with key pressed
void onMouseMotion(int pX, int pY) {	// pX, pY are the pixel coordinates of the cursor in the coordinate system of the operation system
    // Convert to normalized device space
    float cX = 2.0f * pX / windowWidth - 1;	// flip y axis
    float cY = 1.0f - 2.0f * pY / windowHeight;
    printf("Mouse moved to (%3.2f, %3.2f)\n", cX, cY);
}

// Mouse click event
void onMouse(int button, int state, int pX, int pY) { // pX, pY are the pixel coordinates of the cursor in the coordinate system of the operation system
    // Convert to normalized device space
    float cX = 2.0f * pX / windowWidth - 1;	// flip y axis
    float cY = 1.0f - 2.0f * pY / windowHeight;

    char * buttonStat;
    switch (state) {
        case GLUT_DOWN: buttonStat = "pressed"; break;
        case GLUT_UP:   buttonStat = "released"; break;
    }

    switch (button) {
        case GLUT_LEFT_BUTTON:   printf("Left button %s at (%3.2f, %3.2f)\n", buttonStat, cX, cY);   break;
        case GLUT_MIDDLE_BUTTON: printf("Middle button %s at (%3.2f, %3.2f)\n", buttonStat, cX, cY); break;
        case GLUT_RIGHT_BUTTON:  printf("Right button %s at (%3.2f, %3.2f)\n", buttonStat, cX, cY);  break;
    }
}

// Idle event indicating that some time elapsed: do animation here
void onIdle() {
    long time = glutGet(GLUT_ELAPSED_TIME); // elapsed time since the start of the program
    ember.Animate(time);
    glutPostRedisplay();
}