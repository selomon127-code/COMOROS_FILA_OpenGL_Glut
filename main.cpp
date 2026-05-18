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





