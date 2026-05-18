// ============================================================
//  STUDENT 1 — Project Setup, Headers, Constants & Global State
//  Branch : feature/student1-globals
//  File   : student1_globals.cpp
//  Owns   : All #includes, window constants, animation state
//           variables, and the Vec2 / Contour / Shape types.
// ============================================================

#ifdef _WIN32
#include <windows.h>
#endif

#include <GL/glut.h>
#include <GL/glu.h>

#ifndef CALLBACK
#define CALLBACK
#endif

#include <cmath>
#include <vector>
#include <string>
#include <algorithm>
#include <cstdio>

// ─────────────────────────────────────────────────────────────
//  Window / layout constants
// ─────────────────────────────────────────────────────────────
static const int WIN_W = 1100;
static const int WIN_H = 420;
static const float FLAG_SCALE = 0.88f;
static const int FILA_X0 = 320;
static const int FILA_Y0 = 0;
static const int FILA_W  = WIN_W - FILA_X0 - 10;
static const int FILA_H  = WIN_H;

// ─────────────────────────────────────────────────────────────
//  Animation & interaction state
// ─────────────────────────────────────────────────────────────
float waveOffset     = 0.0f;
float windSpeed      = 1.0f;   // range [0.0 .. 5.0]
float flagRotation   = 0.0f;   // degrees
float flagScaleFactor = 1.0f;  // uniform scale
float flagTX = 0.0f, flagTY = 0.0f;

// ─────────────────────────────────────────────────────────────
//  Types for FILA tessellation
// ─────────────────────────────────────────────────────────────
struct Vec2 { double x, y; };
typedef std::vector<Vec2> Contour;
typedef std::vector<Contour> Shape;

// ─────────────────────────────────────────────────────────────
//  SVG bounding box of FILA paths
// ─────────────────────────────────────────────────────────────
static const double SVG_MIN_X =  28.0;
static const double SVG_MIN_Y = 295.0;
static const double SVG_MAX_X = 592.0;
static const double SVG_MAX_Y = 505.0;


//From Tesfaye Seifu

//  Map SVG coords → window coords inside the FILA sub-area
// ─────────────────────────────────────────────────────────────
static Vec2 svgToGL(double sx, double sy)
{
    double rangeX = SVG_MAX_X - SVG_MIN_X;
    double rangeY = SVG_MAX_Y - SVG_MIN_Y;
    double margin = 0.06;
    double drawW  = FILA_W * (1.0 - 2.0 * margin);
    double drawH  = FILA_H * (1.0 - 2.0 * margin);
    double scale  = std::min(drawW / rangeX, drawH / rangeY) * 0.75;
    double offX   = FILA_W * margin + (drawW - rangeX * scale) / 2.0;
    double offY   = FILA_H * margin + (drawH - rangeY * scale) / 2.0 - 40.0;
    double gx = FILA_X0 + offX + (sx - SVG_MIN_X) * scale;
    double gy = FILA_Y0 + offY + (SVG_MAX_Y - sy) * scale;
    return {gx, gy};
}

// ─────────────────────────────────────────────────────────────
//  Cubic Bézier tessellation
// ─────────────────────────────────────────────────────────────
static void cubicBezier(Contour& out,
                         Vec2 p0, Vec2 p1, Vec2 p2, Vec2 p3,
                         int samples = 120)
{
    for (int i = 1; i <= samples; ++i)
    {
        double t  = (double)i / samples;
        double mt = 1.0 - t;
        double x  = mt*mt*mt*p0.x + 3*mt*mt*t*p1.x + 3*mt*t*t*p2.x + t*t*t*p3.x;
        double y  = mt*mt*mt*p0.y + 3*mt*mt*t*p1.y + 3*mt*t*t*p2.y + t*t*t*p3.y;
        out.push_back({x, y});
    }
}


// ============================================================
// Nicolas 6 — Green Triangle, Crescent, Stars, drawFlag(), Background

static void setColor(float r, float g, float b) { glColor3f(r,g,b); }

// ─────────────────────────────────────────────────────────────
//  Five-pointed star centred at (cx,cy)
// ─────────────────────────────────────────────────────────────
static void drawStar5(float cx, float cy, float outer, float inner, float tilt)
{
    const int N = 5;
    float vx[10], vy[10];
    for (int i=0;i<N;i++){
        float ao=tilt+2.f*(float)M_PI*i/N, ai=ao+(float)M_PI/N;
        vx[2*i]=cx+outer*cosf(ao); vy[2*i]=cy+outer*sinf(ao);
        vx[2*i+1]=cx+inner*cosf(ai); vy[2*i+1]=cy+inner*sinf(ai);
    }
    glBegin(GL_TRIANGLE_FAN);
    glVertex2f(cx,cy);
    for (int i=0;i<10;i++) glVertex2f(vx[i],vy[i]);
    glVertex2f(vx[0],vy[0]);
    glEnd();
}

// ─────────────────────────────────────────────────────────────
//  Crescent moon (uses stencil buffer)
// ─────────────────────────────────────────────────────────────
static void drawCrescent(float cx, float cy, float R, float cutOff, float cutR)
{
    glEnable(GL_STENCIL_TEST);
    glStencilFunc(GL_ALWAYS,1,0xFF);
    glStencilOp(GL_KEEP,GL_KEEP,GL_REPLACE);
    glColorMask(GL_FALSE,GL_FALSE,GL_FALSE,GL_FALSE);
    glBegin(GL_TRIANGLE_FAN); glVertex2f(cx,cy);
    for (int i=0;i<=120;i++){ float a=2.f*(float)M_PI*i/120; glVertex2f(cx+R*cosf(a),cy+R*sinf(a)); }
    glEnd();

    glStencilFunc(GL_ALWAYS,0,0xFF);
    glBegin(GL_TRIANGLE_FAN); glVertex2f(cx+cutOff,cy);
    for (int i=0;i<=120;i++){ float a=2.f*(float)M_PI*i/120; glVertex2f(cx+cutOff+cutR*cosf(a),cy+cutR*sinf(a)); }
    glEnd();

    glColorMask(GL_TRUE,GL_TRUE,GL_TRUE,GL_TRUE);
    glStencilFunc(GL_EQUAL,1,0xFF);
    setColor(1,1,1);
    glBegin(GL_TRIANGLE_FAN); glVertex2f(cx,cy);
    for (int i=0;i<=120;i++){ float a=2.f*(float)M_PI*i/120; glVertex2f(cx+R*cosf(a),cy+R*sinf(a)); }
    glEnd();
    glDisable(GL_STENCIL_TEST);
    glClear(GL_STENCIL_BUFFER_BIT);
}
// ─────────────────────────────────────────────────────────────
//  Main flag drawing function
// ─────────────────────────────────────────────────────────────
static void drawFlag()
{
    const float FX = 30.0f * FLAG_SCALE;
    const float FY = 70.0f * FLAG_SCALE;
    const float FW = 440.0f * FLAG_SCALE;
    const float FH = 280.0f * FLAG_SCALE;
    const int   NX = 55, NY = 35;

    // Apply transformation
    float pivotX = FX + FW*0.5f + flagTX;
    float pivotY = FY + FH*0.5f + flagTY;
    glPushMatrix();
    glTranslatef(pivotX,pivotY,0.f);
    glRotatef(flagRotation,0.f,0.f,1.f);
    glScalef(flagScaleFactor,flagScaleFactor,1.f);
    glTranslatef(-pivotX+flagTX,-pivotY+flagTY,0.f);

    // Pole + ball + shadow + stripes (Student 5's code)
    drawPoleAndStripes(FX,FY,FW,FH,NX,NY);

    // ── Smooth waved green triangle ──────────────────────────
    // Three edges are sampled through fabricWave so they ripple
    // with the cloth — no staircase on the diagonal edges.
    setColor(0.00f,0.48f,0.13f);
    float triWidth = FW * 0.45f;
    float triNX    = triWidth / FW;
    const int EDGE_STEPS = 80;

    struct Pt2 { float x, y; };
    auto wavedPt = [&](float nx_, float ny_) -> Pt2 {
        float bx=FX+nx_*FW, by=FY+ny_*FH, dx,dy;
        fabricWave(nx_,ny_,FW,FH,dx,dy);
        return {bx+dx,by+dy};
    };

    std::vector<Pt2> triPoly;
    // Edge 1: left edge top→bottom
    for (int i=0;i<=EDGE_STEPS;i++)
        triPoly.push_back(wavedPt(0.f, 1.f-(float)i/EDGE_STEPS));
    // Edge 2: bottom-left → tip
    for (int i=1;i<=EDGE_STEPS;i++){
        float t=(float)i/EDGE_STEPS;
        triPoly.push_back(wavedPt(t*triNX, t*0.5f));
    }
    // Edge 3: tip → top-left
    for (int i=1;i<=EDGE_STEPS;i++){
        float t=(float)i/EDGE_STEPS;
        triPoly.push_back(wavedPt(triNX*(1.f-t), 0.5f+t*0.5f));
    }

    float cx_=0,cy_=0;
    for (auto& p:triPoly){cx_+=p.x;cy_+=p.y;}
    cx_/=triPoly.size(); cy_/=triPoly.size();

    glBegin(GL_TRIANGLE_FAN);
    glVertex2f(cx_,cy_);
    for (auto& p:triPoly) glVertex2f(p.x,p.y);
    glVertex2f(triPoly[0].x,triPoly[0].y);
    glEnd();

    // ── Crescent & 4 stars ───────────────────────────────────
    float moonR  = triWidth*0.33f;
    float moonCX = FX+moonR+10.f*FLAG_SCALE;
    float moonCY = FY+FH*0.5f;
    drawCrescent(moonCX,moonCY,moonR,moonR*0.4f,moonR*0.98f);

    float starOuter=moonR*0.20f, starInner=starOuter*0.40f;
    float startY   =moonCY-moonR*0.6f;
    setColor(1,1,1);
    for (int i=0;i<4;i++)
        drawStar5(moonCX+moonR*0.3f, startY+i*(moonR*0.45f),
                  starOuter,starInner,-(float)M_PI/2.6f);

    glPopMatrix();
}

// ─────────────────────────────────────────────────────────────
//  Background gradient  (sky-blue top → warm cream bottom)
// ─────────────────────────────────────────────────────────────
static void drawBackground()
{
    glBegin(GL_QUADS);
    glColor3f(0.72f,0.85f,0.96f); glVertex2f(0,    WIN_H);
    glColor3f(0.72f,0.85f,0.96f); glVertex2f(WIN_W,WIN_H);
    glColor3f(0.97f,0.94f,0.88f); glVertex2f(WIN_W,0);
    glColor3f(0.97f,0.94f,0.88f); glVertex2f(0,    0);
    glEnd();
}

void display()
{
    glClear(GL_COLOR_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluOrtho2D(0, WIN_W, 0, WIN_H);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    drawBackground();
    drawFlag();
    drawFILA();

    glutSwapBuffers();
}

// ─────────────────────────────────────────────────────────────
//  Main
// ─────────────────────────────────────────────────────────────
int main(int argc, char** argv)
{
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_STENCIL | GLUT_MULTISAMPLE);
    glutInitWindowSize(WIN_W, WIN_H);
    glutCreateWindow("Comoros Flag + FILA Logo  [Enhanced]");

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_LINE_SMOOTH);
    glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);
    glEnable(GL_POLYGON_SMOOTH);
    glHint(GL_POLYGON_SMOOTH_HINT, GL_NICEST);

    g_trisFBody  = buildPath(PATH_F_BODY);
    g_trisFCross = buildPath(PATH_F_CROSSBAR);
    g_trisI      = buildPath(PATH_I);
    g_trisLA     = buildPath(PATH_LA);

    glutDisplayFunc(display);
    glutKeyboardFunc(keyboard);
    glutSpecialFunc(specialKey);
    glutTimerFunc(16, update, 0);

    glutMainLoop();
    return 0;
}





