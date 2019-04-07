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
const char *const vertexSource = R"(
	#version 330				// Shader 3.3
	precision highp float;		// normal floats, makes no difference on desktop computers

	uniform mat4 MVP;			// uniform variable, the Model-View-Projection transformation matrix
	layout(location = 0) in vec2 vp;	// Varying input: vp = vertex position is expected in attrib array 0
	layout(location = 1) in vec3 vertexColor;	    // Attrib Array 1
    layout(location = 2) in vec2 vertexUV;

    out vec3 color;
    out vec2 texcoord;

	void main() {
        color = vertexColor;
        texcoord = vertexUV;
		gl_Position = vec4(vp.x, vp.y, 0, 1) * MVP;		// transform vp from modeling space to normalized device space
	}
)";

// fragment shader in GLSL
const char *const fragmentSource = R"(
	#version 330			// Shader 3.3
	precision highp float;	// normal floats, makes no difference on desktop computers

	uniform sampler2D textureUnit;
    uniform int isTexture;

	in vec3 color;		// uniform variable, the color of the primitive
    in vec2 texcoord;

	out vec4 fragmentColor;		// output that goes to the raster memory as told by glBindFragDataLocation

	void main() {
		 if(isTexture==0) fragmentColor = vec4(color, 1); // extend RGB to RGBA
         else fragmentColor = texture(textureUnit, texcoord);
	}
)";

bool kovetes = false;

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
    float state;
public:
    Kerek(vec2 center, float R) : center(center), R(R), state(0) {}

    void Create();
    void Draw();
    void allapot(float allapot);
    void mozgat(vec2 eltolas);
};

class Fej {
    GLuint vao;
    GLuint vbo[2];
    vec2 pontok[40];
    vec3 szin;
    vec2 center;
    float R;
public:
    Fej(vec2 center, float R) : center(center), R(R) {}

    void Create();
    void Draw();
    void mozgat(vec2 eltolas);
};

class Ember {
    GLuint vao;
    GLuint vbo[2];
    vec2 position; // kerék középpontja
    float posOnSpline = -1.0f;
    vec2 testPontok[7];
    vec2 labfejPontok[2];
    vec3 testSzinek[7];
    Kerek kerek;
    Fej fej;
    float state;
    int irany = -1;
    float labszarhossz = 0.12f;
    float combhossz = 0.10f;
public:
    Ember() : position(0, 0), kerek(position, 0.1f), fej(position + vec2(0, 0.345f), 0.045f) {}

    void Create();
    void Animate();
    void Draw();
    vec2 center() {
        return position + vec2(0, 0.1f);
    }
};

Ember ember;

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
    Camera() : center(0, 0) {}

    void kozep() {
        if (kovetes) {
            center = ember.center() + vec2(0, 0.2f);
        }
    }

    vec2 eltolas() {
        return center*-1;
    }
};

Camera camera;
GPUProgram gpuProgram; // vertex and fragment shaders

class SplineBase{
protected:
    GLuint vao, vbo[2];
    vec2 pontok[4500];
    vec3 szinek[4500];
    std::vector<vec2> cps;
    bool modosult = false;

    vec2 v(vec2 p1, vec2 p2, vec2 p3) {
        float tenzio = -0.5f;
        return (((p3 - p2) * (1 / (p3.x - p2.x)) + ((p2 - p1) * (1 / (p2.x - p1.x)))) * 0.5f * (1 - tenzio));
    }

    vec2 point(vec2 p1, vec2 p2, vec2 p3, vec2 p4, float t) {
        vec2 v2 = v(p1, p2, p3);
        vec2 v3 = v(p2, p3, p4);

        vec2 a0 = p2;
        vec2 a1 = v2;
        vec2 a2 = (((p3 - p2) * 3.0f) * (1 / ((p3.x - p2.x) * ((p3.x - p2.x))))) -
                  ((v3 + v2 * 2.0f) * (1 / (p3.x - p2.x)));
        vec2 a3 = (((p2 - p3) * 2.0f) * (1 / ((p3.x - p2.x) * (p3.x - p2.x) * (p3.x - p2.x)))) +
                  ((v3 + v2) * (1 / ((p3.x - p2.x) * (p3.x - p2.x))));

        float t0 = t - p2.x;
        float t1 = t0 * t0;
        float t2 = t1 * t0;

        return vec2(t, vec2(a3 * t2 + a2 * t1 + a1 * t0 + a0).y);
    }

    vec2 derivaltpoint(vec2 p1, vec2 p2, vec2 p3, vec2 p4, float t) {
        vec2 v2 = v(p1, p2, p3);
        vec2 v3 = v(p2, p3, p4);

        vec2 a1 = v2;
        vec2 a2 = (((p3 - p2) * 3.0f) * (1 / ((p3.x - p2.x) * ((p3.x - p2.x))))) -
                  ((v3 + v2 * 2.0f) * (1 / (p3.x - p2.x)));
        vec2 a3 = (((p2 - p3) * 2.0f) * (1 / ((p3.x - p2.x) * (p3.x - p2.x) * (p3.x - p2.x)))) +
                  ((v3 + v2) * (1 / ((p3.x - p2.x) * (p3.x - p2.x))));

        float t0 = t - p2.x;
        float t1 = t0 * t0;
        float t2 = t1 * t0;

        return a3 * t1 * 3.0f + a2 * t0 * 2.0f + a1;
    }
public:
    vec2 r(float t) {
        for (int i = 0; i < cps.size() - 1; ++i) {
            if (cps[i].x <= t && cps[i + 1].x >= t) {
                vec2 p1 = cps.front() - vec2(0.5f, 0.0f);
                if (i > 0)p1 = cps[i - 1];
                vec2 p2 = cps[i];
                vec2 p3 = cps[i + 1];
                vec2 p4 = cps.back() + vec2(0.5f, 0.0f);
                if (i + 2 < cps.size()) p4 = cps[i + 2];
                return point(p1, p2, p3, p4, t);
            }
        }
    }

    vec2 derivalt(float t) {
        for (int i = 0; i < cps.size() - 1; ++i) {
            if (cps[i].x <= t && cps[i + 1].x >= t) {
                vec2 p1 = cps.front() - vec2(5.0f, 0.0f);
                if (i > 0)p1 = cps[i - 1];
                vec2 p2 = cps[i];
                vec2 p3 = cps[i + 1];
                vec2 p4 = cps.back() + vec2(5.0f, 0.0f);
                if (i + 2 < cps.size()) p4 = cps[i + 2];
                return derivaltpoint(p1, p2, p3, p4, t);
            }
        }
    }

    void addPoint(vec2 p) {
        p=p-camera.eltolas();
        for (auto &cp: cps)
            if (p.x == cp.x)
                return;
        modosult = true;
        for (auto it = cps.begin(); it != cps.end(); ++it) {
            if (p.x < it->x) {
                cps.insert(it, p);
                return;
            }
        }
        cps.push_back(p);
    }
};

//http://cg.iit.bme.hu/portal/sites/default/files/oktatott%20t%C3%A1rgyak/sz%C3%A1m%C3%ADt%C3%B3g%C3%A9pes%20grafika/geometriai%20modellez%C3%A9s/bmemodel.pdf 18. oldal
class Spline : public SplineBase {
public:
    Spline() {
        cps.emplace_back(vec2(-2.3f, -0.8f));
        cps.emplace_back(vec2(2.3f, -0.8f));
    }

    void Create() {
        glGenVertexArrays(1, &vao);
        glBindVertexArray(vao);
        glGenBuffers(2, &vbo[0]);

        for (int i = 0; i < 4500; i+=2){
            pontok[i] = vec2(float(i - 2250) / 1000.f, -0.8f);
            pontok[i+1] = vec2(float(i - 2250) / 1000.f, -1.0f);
        }

        szinek[0] = {0, 0.7, 0};
        szinek[1] = {0, 0.3, 0};
        int i = 0;
        for (auto &szin : szinek)
            szin = szinek[i++%2];

        glBindBuffer(GL_ARRAY_BUFFER, vbo[0]);
        glBufferData(GL_ARRAY_BUFFER, sizeof(pontok), pontok, GL_DYNAMIC_DRAW);

        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, NULL);

        glBindBuffer(GL_ARRAY_BUFFER, vbo[1]);
        glBufferData(GL_ARRAY_BUFFER, sizeof(szinek), szinek, GL_STATIC_DRAW);

        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, NULL);
    }

    void Draw() {
        glBindVertexArray(vao);
        if (modosult || kovetes) {
            for (int i = 0; i < 4500; i+=2){
                pontok[i] = r(float(i - 2250) / 1000.f) + camera.eltolas();
                pontok[i+1] = vec2(float(i - 2250) / 1000.f, -1.0f) + vec2(camera.eltolas().x,0);
            }
            glBindBuffer(GL_ARRAY_BUFFER, vbo[0]);
            glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(pontok), pontok);
            modosult = false;
        }

        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4500);
    }
};

Spline spline;

void Kerek::Create() {
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);

    alapPontok[0] = center + vec2(0, R);
    alapPontok[1] = center + vec2(0, R * 0.8f);
    for (int i = 2; i < 120; i += 2) {
        alapPontok[i] = forgatas(alapPontok[0], M_PI / 15 * i / 2, center);
        alapPontok[i + 1] = forgatas(alapPontok[1], M_PI / 15 * i / 2, center);
    }
    alapPontok[120] = alapPontok[0];
    alapPontok[121] = alapPontok[1];

    alapKullopontok[0] = center + vec2(0, R * 0.95f);
    alapKullopontok[1] = center - vec2(0, R * 0.95f);
    for (int i = 1; i < 7; ++i) {
        alapKullopontok[2 * i] = forgatas(alapKullopontok[0], 2.0f / 7 * M_PI * i, center);
        alapKullopontok[2 * i + 1] = forgatas(alapKullopontok[1], 2.0f / 7 * M_PI * i, center);
    }

    glGenBuffers(2, &vbo[0]);
    glBindBuffer(GL_ARRAY_BUFFER, vbo[0]);

    glBufferData(GL_ARRAY_BUFFER, sizeof(alapPontok) + sizeof(alapKullopontok), alapPontok, GL_DYNAMIC_DRAW);
    glBufferSubData(GL_ARRAY_BUFFER, sizeof(alapPontok), sizeof(alapKullopontok), alapKullopontok);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, NULL);

    szinek[0] = {0.51f, 0.51f, 0.51f};
    szinek[1] = {0, 0, 0};

    for (int i = 1; i < 122 / 2; ++i) {
        szinek[i * 2] = szinek[0];
        szinek[i * 2 + 1] = szinek[1];
    }

    for (auto &kulloszin: kulloszinek)
        kulloszin = {0.3f, 0.3f, 0.3f};

    glBindBuffer(GL_ARRAY_BUFFER, vbo[1]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(szinek) + sizeof(kulloszinek), szinek, GL_STATIC_DRAW);
    glBufferSubData(GL_ARRAY_BUFFER, sizeof(szinek), sizeof(kulloszinek), kulloszinek);

    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, NULL);
}

void Kerek::Draw() {
    glBindVertexArray(vao);

    glBindBuffer(GL_ARRAY_BUFFER, vbo[0]);
    for (int i = 0; i < 122; ++i) {
        pontok[i] = forgatas(alapPontok[i], -1 * state * M_PI * 2, center) + camera.eltolas();
    }
    for (int i = 0; i < 14; ++i)
        kullopontok[i] = forgatas(alapKullopontok[i], -1 * state * M_PI * 2, center) + camera.eltolas();
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(pontok), pontok);
    glBufferSubData(GL_ARRAY_BUFFER, sizeof(pontok), sizeof(kullopontok), kullopontok);

    glLineWidth(0.8f); // Width of lines in pixels
    glDrawArrays(GL_LINES, 122, 14);
    glLineWidth(1.0f); // Width of lines in pixels
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 62);
    //glDrawArrays(GL_TRIANGLE_STRIP, 60, 62);
}

void Kerek::allapot(float allapot) {
    state = allapot;
    while (state > 1.0)
        state -= 1.0;
}

void Kerek::mozgat(vec2 eltolas) {
    center = center + eltolas;
    for (auto &pont : alapPontok)
        pont = pont + eltolas;
    for (auto &pont : alapKullopontok)
        pont = pont + eltolas;
}

void Fej::Create() {
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);

    pontok[0] = center + vec2(0, R);
    for (int i = 1; i < 40; ++i) {
        pontok[i] = forgatas(pontok[0], M_PI / 20 * i, center);
    }

    glGenBuffers(2, &vbo[0]);
    glBindBuffer(GL_ARRAY_BUFFER, vbo[0]);

    glBufferData(GL_ARRAY_BUFFER, sizeof(pontok), pontok, GL_DYNAMIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, NULL);

    szin = {0, 0, 0};

    glBindBuffer(GL_ARRAY_BUFFER, vbo[1]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(szin) * 40, nullptr, GL_STATIC_DRAW);

    for (int i = 0; i < 40; ++i)
        glBufferSubData(GL_ARRAY_BUFFER, sizeof(szin) * i, sizeof(szin), &szin);

    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, NULL);
}

void Fej::Draw() {
    glBindVertexArray(vao);

    vec2 pontok2[40];
    int i=0;
    for(auto & pont : pontok)
        pontok2[i++] = pont + camera.eltolas();

    glBindBuffer(GL_ARRAY_BUFFER, vbo[0]);
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(pontok2), pontok2);

    glLineWidth(2.0f);
    glDrawArrays(GL_TRIANGLE_FAN, 0, 40);
}

void Fej::mozgat(vec2 eltolas) {
    center = center + eltolas;
    for (auto &pont : pontok)
        pont = pont + eltolas;
}

void Ember::Create() {
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);

    testPontok[0] = {position.x, position.y + 0.3f};
    testPontok[1] = {position.x, position.y + 0.14f};

    for (auto &testSzin: testSzinek)
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

//http://cg.iit.bme.hu/portal/sites/default/files/oktatott%20t%C3%A1rgyak/sz%C3%A1m%C3%ADt%C3%B3g%C3%A9pes%20grafika/grafikus%20alap%20hw/sw/bme2dsys.pdf 35. oldal
void Ember::Animate() {
    float F = 795;
    float m = 67;
    float g = 9.98;
    float p = F * 2.0;
    static bool fordulniakell = false;
    if (fordulniakell) {
        irany *= -1;
        fordulniakell = false;
    }

    static float tend = 0;
    const float dt = 0.01;
    float tstart = tend;
    tend = glutGet(GLUT_ELAPSED_TIME) / 1000.f;
    for (float t = tstart; t < tend; t += dt) {
        float Dt = fmin(dt, tend - t);
        vec2 seged(1.0f, 0.0f);
        vec2 deri = spline.derivalt(posOnSpline);
        float alpha = acosf((dot(deri, seged)) / (length(seged) * length(deri)));
        if (seged.y > deri.y)alpha *= -1.0f;
        if (irany == 1) {
            alpha += M_PI;
        }
        float v = (F - m * g * sinf(alpha)) / p;
        float Ds = -1 * irany * v * Dt;
        float Dszogelfordulas = Ds / 0.1f;
        float Dx = Ds / (sqrtf(1.0f + (spline.derivalt(posOnSpline).y * spline.derivalt(posOnSpline).y))); //maybe
        posOnSpline += Dx;
        if (irany == -1 && posOnSpline >= 1.0f) {
            fordulniakell = true;
            posOnSpline = 1.0f;
            break;
        } else if (irany == 1 && posOnSpline <= -1.0f) {
            fordulniakell = true;
            posOnSpline = -1.0f;
            break;
        } else {
            state += Dszogelfordulas / (2 * M_PI);
            state = fmod(state, 1.0f);
            kerek.allapot(state);
        }
    }
}

void Ember::Draw() {
    glBindVertexArray(vao);

    vec2 regiposition = position;

    vec2 deri = normalize(spline.derivalt(posOnSpline));
    vec2 normal = vec2(deri.y * -1.f, deri.x) * 0.1f;
    position = normal + spline.r(posOnSpline);
    vec2 poseltolas = position - regiposition;
    for (auto &p : testPontok) p = p + poseltolas;
    for (auto &p : labfejPontok) p = p + poseltolas;
    kerek.mozgat(poseltolas);
    fej.mozgat(poseltolas);

    vec2 fasz = spline.r(-1.0f);

    testPontok[3] = forgatas(labfejPontok[0], -1 * state * M_PI * 2, position);
    testPontok[6] = forgatas(labfejPontok[1], -1 * state * M_PI * 2, position);
    testPontok[4] = testPontok[1];

    float labszarfenektav1 = sqrtf(
            (testPontok[1].x - testPontok[3].x) * (testPontok[1].x - testPontok[3].x) +
            (testPontok[1].y - testPontok[3].y) * (testPontok[1].y - testPontok[3].y)
    );
    float labszarfenektav2 = sqrtf(
            (testPontok[1].x - testPontok[6].x) * (testPontok[1].x - testPontok[6].x) +
            (testPontok[1].y - testPontok[6].y) * (testPontok[1].y - testPontok[6].y)
    );
    float valami = (labszarfenektav1 * labszarfenektav1 + combhossz * combhossz - labszarhossz * labszarhossz) /
                   (2 * labszarfenektav1 * combhossz);
    float szog1 = acosf(valami);
    float valami2 = (labszarfenektav2 * labszarfenektav2 + combhossz * combhossz - labszarhossz * labszarhossz) /
                    (2 * labszarfenektav2 * combhossz);
    float szog2 = acosf(valami2);

    testPontok[2] = normalize(testPontok[3] - testPontok[1]) * combhossz + testPontok[1];
    testPontok[5] = normalize(testPontok[6] - testPontok[1]) * combhossz + testPontok[1];
    testPontok[2] = forgatas(testPontok[2], -1 * irany * szog1, testPontok[1]);
    testPontok[5] = forgatas(testPontok[5], -1 * irany * szog2, testPontok[1]);

    vec2 testPontok2[7];
    int i=0;
    for (auto & pont: testPontok)
        testPontok2[i++] = pont  + camera.eltolas();

    glBindBuffer(GL_ARRAY_BUFFER, vbo[0]);
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(testPontok2), testPontok2);

    glLineWidth(3.0f); // Width of lines in pixels
    glDrawArrays(GL_LINE_STRIP, 0, 2);
    glDrawArrays(GL_LINE_STRIP, 1, 3);
    kerek.Draw();
    glBindVertexArray(vao);
    glLineWidth(3.0f); // Width of lines in pixels
    glDrawArrays(GL_LINE_STRIP, 4, 3);
    fej.Draw();
}

// http://cg.iit.bme.hu/portal/sites/default/files/oktatott%20t%C3%A1rgyak/sz%C3%A1m%C3%ADt%C3%B3g%C3%A9pes%20grafika/grafikus%20alap%20hw/sw/bme2dsys.pdf
// textúrás rész
class Hatter: public SplineBase {
    vec4 image[500*500];
    GLuint vao, vbo[2], textureId;
    vec2 coord[4], uv[4];
    vec4 egAlja, egTeteje;
    vec4 hegyAlja, hegyTeteje;

    vec4 atmenet(vec4 kezdet, vec4 vege, float szazalek) {
        vec4 szin = {0,0,0,1};
        szin.x = kezdet.x + (vege.x-kezdet.x)*szazalek;
        szin.y = kezdet.y + (vege.y-kezdet.y)*szazalek;
        szin.z = kezdet.z + (vege.z-kezdet.z)*szazalek;
        return szin;
    }
public:
    Hatter() {
        cps.emplace_back(vec2(-2.3f, -0.8f));
        cps.emplace_back(vec2(-0.73f, 0.58f));
        cps.emplace_back(vec2(-0.03f, -0.47f));
        cps.emplace_back(vec2(0.7f, 0.7f));
        cps.emplace_back(vec2(2.3f, -0.8f));

        coord[0] = {1,1};
        coord[1] = {-1,1};
        coord[2] = {1,-1};
        coord[3] = {-1,-1};

        uv[0] = {0,0};
        uv[1] = {1,0};
        uv[2] = {0,1};
        uv[3] = {1,1};

        egAlja = {143.0f/256, 188.0f/256, 216.0f/256, 1};
        egTeteje = {2.0f/256,87.0f/256,140.0f/256, 1};

        hegyAlja = { 140.0f/256, 93.0f/256, 52.0f/256, 1};
        hegyTeteje = {174.0f/256, 131.0f/256, 98.0f/256, 1};

        for(int y=0; y<500; ++y)
            for(int x=0; x<500; ++x)
            {
                if(r((x-250)/250.0f).y-0.005f < -1*(y-250)/250.0f && r((x-250)/250.0f).y > -1*(y-250)/250.0f)
                    image[500*y+x] = {0,0,0,1};
                else if(r((x-250)/250.0f).y-0.005f > -1*(y-250)/250.0f)
                    image[500*y+x] = atmenet(hegyAlja, hegyTeteje, -1*((-1*(y-250)/250.0f))/((r((x-250)/250.0f).y)-1.0f));
                else
                    image[500*y+x] = atmenet(egAlja, egTeteje, -1*(y-250)/250.0f);
            }

    }

    void Create() {
        glGenTextures(1, &textureId);
        glBindTexture(GL_TEXTURE_2D, textureId);

        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 500, 500, 0, GL_RGBA, GL_FLOAT, image);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        glGenVertexArrays(1, &vao);
        glBindVertexArray(vao);
        glGenBuffers(2, &vbo[0]);

        glBindBuffer(GL_ARRAY_BUFFER, vbo[0]);
        glBufferData(GL_ARRAY_BUFFER, sizeof(coord), coord, GL_STATIC_DRAW);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, NULL);

        glBindBuffer(GL_ARRAY_BUFFER, vbo[1]);
        glBufferData(GL_ARRAY_BUFFER, sizeof(uv), uv, GL_STATIC_DRAW);
        glEnableVertexAttribArray(2);
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 0, NULL);
    }

    void Draw(){
        int location = glGetUniformLocation(gpuProgram.getId(), "isTexture");
        glUniform1i(location, 1);
        location = glGetUniformLocation(gpuProgram.getId(), "textureUnit");
        glUniform1i(location, 0);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, textureId);

        glBindVertexArray(vao);
        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
        location = glGetUniformLocation(gpuProgram.getId(), "isTexture");
        glUniform1i(location, 0);
    }
};

Hatter hatter;

// Initialization, create an OpenGL context
void onInitialization() {
    glViewport(0, 0, windowWidth, windowHeight);

    int location = glGetUniformLocation(gpuProgram.getId(), "isTexture");
    glUniform1i(location, 0);

    ember.Create();
    spline.Create();
    hatter.Create();

    // create program for the GPU
    gpuProgram.Create(vertexSource, fragmentSource, "fragmentColor");
}

// Window has become invalid: Redraw
void onDisplay() {
    glClearColor(0.529, 0.808, 0.98, 1);     // background color
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); // clear frame buffer

    float MVPtransf[4][4] = {1, 0, 0, 0,    // MVP matrix,
                             0, 1, 0, 0,    // row-major!
                             0, 0, 1, 0,
                             0, 0, 0, 1};

    int location = glGetUniformLocation(gpuProgram.getId(), "MVP");    // Get the GPU location of uniform variable MVP
    glUniformMatrix4fv(location, 1, GL_TRUE,
                       &MVPtransf[0][0]);    // Load a 4x4 row-major float matrix to the specified location

    hatter.Draw();
    spline.Draw();
    ember.Draw();

    glutSwapBuffers(); // exchange buffers for double buffering
}

// Key of ASCII code pressed
void onKeyboard(unsigned char key, int pX, int pY) {
    if (key == ' ') kovetes=true;
}

// Key of ASCII code released
void onKeyboardUp(unsigned char key, int pX, int pY) {
}

// Move mouse with key pressed
void onMouseMotion(int pX,
                   int pY) {}

// Mouse click event
void onMouse(int button, int state, int pX,
             int pY) { // pX, pY are the pixel coordinates of the cursor in the coordinate system of the operation system
    // Convert to normalized device space
    float cX = 2.0f * pX / windowWidth - 1;    // flip y axis
    float cY = 1.0f - 2.0f * pY / windowHeight;

    switch (button) {
        case GLUT_LEFT_BUTTON:
            if (state == GLUT_DOWN)
                spline.addPoint({cX, cY});
            break;
    }
}

// Idle event indicating that some time elapsed: do animation here
void onIdle() {
    ember.Animate();
    camera.kozep();
    glutPostRedisplay();
}