// ============================================================
//  Comoros Flag + FILA Logo – OpenGL / GLUT (C++)
//  ENHANCED:
//    • 2D fabric wave (both X and Y displacement)
//    • Wind speed control (keys: +/- or Up/Down)
//    • Flag drop-shadow
//    • Pole ball decoration
//    • Background gradient
//    • Flag transformation (scale / rotate via keys)
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
float waveOffset  = 0.0f;
float windSpeed   = 1.0f;   // multiplier; range [0.1 .. 5.0]

// Transformation (keys: R/r rotate, S/s scale, arrow keys translate)
float flagRotation   = 0.0f;   // degrees
float flagScaleFactor = 1.0f;  // uniform scale
float flagTX = 0.0f, flagTY = 0.0f; // translation offset

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
// ─────────────────────────────────────────────────────────────
//  Map SVG coords → window coords inside the FILA sub-area
// ─────────────────────────────────────────────────────────────
static Vec2 svgToGL(double sx, double sy)
{
    double rangeX = SVG_MAX_X - SVG_MIN_X;
    double rangeY = SVG_MAX_Y - SVG_MIN_Y;
    double margin = 0.06;
    double drawW  = FILA_W * (1.0 - 2.0 * margin);
    double drawH  = FILA_H * (1.0 - 2.0 * margin);
    double scale  = std::min(drawW / rangeX, drawH / rangeY) * 0.75;
    double offX   = FILA_W * margin + (drawW - rangeX * scale) / 2.0;
    double offY   = FILA_H * margin + (drawH - rangeY * scale) / 2.0 - 40.0;
    double gx = FILA_X0 + offX + (sx - SVG_MIN_X) * scale;
    double gy = FILA_Y0 + offY + (SVG_MAX_Y - sy) * scale;
    return {gx, gy};
}

// ─────────────────────────────────────────────────────────────
//  Cubic Bézier tessellation
// ─────────────────────────────────────────────────────────────
static void cubicBezier(Contour& out,
                         Vec2 p0, Vec2 p1, Vec2 p2, Vec2 p3,
                         int samples = 120)
{
    for (int i = 1; i <= samples; ++i)
    {
        double t  = (double)i / samples;
        double mt = 1.0 - t;
        double x  = mt*mt*mt*p0.x + 3*mt*mt*t*p1.x + 3*mt*t*t*p2.x + t*t*t*p3.x;
        double y  = mt*mt*mt*p0.y + 3*mt*mt*t*p1.y + 3*mt*t*t*p2.y + t*t*t*p3.y;
        out.push_back({x, y});
    }
}
//From fanuel Ayana
// ─────────────────────────────────────────────────────────────
//  SVG Path Parser  (M m L l C c Z z)
// ─────────────────────────────────────────────────────────────
static double readNum(const std::string& s, size_t& pos)
{
    while (pos < s.size() &&
           (s[pos]==' '||s[pos]=='\t'||s[pos]=='\n'||s[pos]=='\r'||s[pos]==','))
        ++pos;
    size_t start = pos;
    if (pos < s.size() && (s[pos]=='-'||s[pos]=='+')) ++pos;
    while (pos < s.size() && (isdigit(s[pos])||s[pos]=='.')) ++pos;
    if (pos < s.size() && (s[pos]=='e'||s[pos]=='E'))
    {
        ++pos;
        if (pos < s.size() && (s[pos]=='+'||s[pos]=='-')) ++pos;
        while (pos < s.size() && isdigit(s[pos])) ++pos;
    }
    return std::stod(s.substr(start, pos - start));
}

static Shape parseSVGPath(const std::string& d)
{
    Shape shape;
    Contour current;
    size_t i = 0;
    char cmd = 0;
    double cx = 0, cy = 0, sx = 0, sy = 0;

    while (i < d.size())
    {
        while (i < d.size() &&
               (d[i]==' '||d[i]=='\t'||d[i]=='\n'||d[i]=='\r')) ++i;
        if (i >= d.size()) break;
        if (isalpha(d[i])) { cmd = d[i++]; continue; }

        switch (cmd)
        {
        case 'M':
            if (!current.empty()) shape.push_back(current);
            current.clear();
            cx = readNum(d,i); cy = readNum(d,i);
            sx = cx; sy = cy;
            current.push_back({cx,cy});
            cmd = 'L';
            break;
        case 'm':
            if (!current.empty()) shape.push_back(current);
            current.clear();
            cx += readNum(d,i); cy += readNum(d,i);
            sx = cx; sy = cy;
            current.push_back({cx,cy});
            cmd = 'l';
            break;
        case 'L': { double nx=readNum(d,i),ny=readNum(d,i); current.push_back({nx,ny}); cx=nx; cy=ny; break; }
        case 'l': { double dx=readNum(d,i),dy=readNum(d,i); cx+=dx; cy+=dy; current.push_back({cx,cy}); break; }
        case 'C': {
            Vec2 p0={cx,cy};
            Vec2 p1={readNum(d,i),readNum(d,i)};
            Vec2 p2={readNum(d,i),readNum(d,i)};
            Vec2 p3={readNum(d,i),readNum(d,i)};
            cubicBezier(current,p0,p1,p2,p3);
            cx=p3.x; cy=p3.y; break; }
        case 'c': {
            Vec2 p0={cx,cy};
            Vec2 p1={cx+readNum(d,i),cy+readNum(d,i)};
            Vec2 p2={cx+readNum(d,i),cy+readNum(d,i)};
            double ex=cx+readNum(d,i),ey=cy+readNum(d,i);
            cubicBezier(current,p0,p1,p2,{ex,ey});
            cx=ex; cy=ey; break; }
        case 'Z': case 'z':
            if (!current.empty()) current.push_back(current[0]);
            shape.push_back(current);
            current.clear();
            cx=sx; cy=sy;
            break;
        default: ++i;
        }
    }
    if (!current.empty()) shape.push_back(current);
    return shape;
}

// ─────────────────────────────────────────────────────────────
//  GLU Tessellation helpers
// ─────────────────────────────────────────────────────────────
struct TessData
{
    std::vector<std::vector<double>> verts;
    std::vector<GLdouble*> ptrs;
    GLenum primType;
    std::vector<Vec2> strip;
    std::vector<Vec2> filled;
};

static void CALLBACK tessVertex(void* vd, void* ud) {
    TessData* td=(TessData*)ud; GLdouble* v=(GLdouble*)vd;
    td->strip.push_back({v[0],v[1]});
}
static void CALLBACK tessBegin(GLenum t, void* ud) {
    TessData* td=(TessData*)ud; td->primType=t; td->strip.clear();
}
static void CALLBACK tessEnd(void* ud) {
    TessData* td=(TessData*)ud;
    if (td->primType==GL_TRIANGLES)
        for (auto& v:td->strip) td->filled.push_back(v);
    else if (td->primType==GL_TRIANGLE_STRIP)
        for (size_t k=2;k<td->strip.size();++k){
            if(k%2==0){td->filled.push_back(td->strip[k-2]);td->filled.push_back(td->strip[k-1]);td->filled.push_back(td->strip[k]);}
            else{td->filled.push_back(td->strip[k-1]);td->filled.push_back(td->strip[k-2]);td->filled.push_back(td->strip[k]);}
        }
    else if (td->primType==GL_TRIANGLE_FAN)
        for (size_t k=2;k<td->strip.size();++k){
            td->filled.push_back(td->strip[0]);
            td->filled.push_back(td->strip[k-1]);
            td->filled.push_back(td->strip[k]);
        }
}
static void CALLBACK tessCombine(GLdouble coords[3],void* /*vd*/[4],GLfloat /*w*/[4],void** out,void* ud) {
    TessData* td=(TessData*)ud;
    td->verts.push_back({coords[0],coords[1],coords[2]});
    *out=td->verts.back().data();
}
static void CALLBACK tessError(GLenum err) { fprintf(stderr,"Tess error: %s\n",gluErrorString(err)); }

static std::vector<Vec2> tessellateShape(const Shape& shape)
{
    TessData td;
    GLUtesselator* tess = gluNewTess();
    gluTessCallback(tess,GLU_TESS_VERTEX_DATA,(void(CALLBACK*)())tessVertex);
    gluTessCallback(tess,GLU_TESS_BEGIN_DATA,(void(CALLBACK*)())tessBegin);
    gluTessCallback(tess,GLU_TESS_END_DATA,(void(CALLBACK*)())tessEnd);
    gluTessCallback(tess,GLU_TESS_COMBINE_DATA,(void(CALLBACK*)())tessCombine);
    gluTessCallback(tess,GLU_TESS_ERROR,(void(CALLBACK*)())tessError);
    gluTessProperty(tess,GLU_TESS_WINDING_RULE,GLU_TESS_WINDING_NONZERO);
    gluTessBeginPolygon(tess,&td);
    for (auto& contour:shape){
        gluTessBeginContour(tess);
        for (auto& pt:contour){
            td.verts.push_back({pt.x,pt.y,0.0});
            GLdouble* p=td.verts.back().data();
            td.ptrs.push_back(p);
            gluTessVertex(tess,p,p);
        }
        gluTessEndContour(tess);
    }
    gluTessEndPolygon(tess);
    gluDeleteTess(tess);
    return td.filled;
}

static void drawShape(const std::vector<Vec2>& tris) {
    glBegin(GL_TRIANGLES);
    for (auto& v:tris) glVertex2d(v.x,v.y);
    glEnd();
}

static std::vector<Vec2> buildPath(const std::string& d) {
    Shape raw=parseSVGPath(d);
    Shape glShape;
    for (auto& contour:raw){
        Contour gc;
        for (auto& p:contour) gc.push_back(svgToGL(p.x,p.y));
        glShape.push_back(gc);
    }
    return tessellateShape(glShape);
}
//From Asratie 
// ─────────────────────────────────────────────────────────────
//  SVG Path Data (FILA)
// ─────────────────────────────────────────────────────────────
static const std::string PATH_F_BODY =
    "m 46.202392,497.83448 "
    "c -7.88389,-2.81348 -14.37451,-9.82426 -16.73234,-18.07325 -0.79414,-2.77836 -1.04747,-15.60555 -0.82948,-42 "
    "l 0.31385,-38 2.78415,-5.28415 "
    "c 3.71445,-7.0498 9.68657,-12.84169 16.56789,-16.06789 "
    "l 5.64796,-2.64796 52.850428,-0.29176 "
    "c 59.71829,-0.32968 59.83504,-0.31806 67.86703,6.75197 6.04647,5.32232 8.92383,11.59166 8.9697,19.54368 0.0626,10.85302 -4.95503,19.16818 -14.68716,24.33936 "
    "l -5,2.65675 -40.66837,0.5 "
    "c -22.36761,0.275 -41.042608,0.87367 -41.499998,1.33039 -0.4574,0.45671 -1.05663,11.70671 -1.33163,25 "
    "l -0.5,24.16961 -2.96638,5.35555 "
    "c -6.43741,11.62223 -19.14505,16.87182 -30.78565,12.7177 z";

static const std::string PATH_F_CROSSBAR =
    "M 44.778602,356.90306 "
    "c -6.4318,-2.13636 -11.06927,-5.41098 -14.28779,-10.08893 -9.796046,-14.23807 -4.508094,-32.54457 11.46361,-39.68615 4.96613,-2.22056 5.42384,-2.23867 67.568808,-2.67486 69.06311,-0.48473 69.07494,-0.48386 77.4947,5.71512 15.5611,11.45672 12.46124,37.2436 -5.40078,44.92764 -4.62259,1.98859 -7.04361,2.08115 -68.72705,2.6277 -52.976528,0.4694 -64.651448,0.32875 -68.111498,-0.82052 z";

static const std::string PATH_I =
    "m 227.759630,496.79358 "
    "c -5.68446,-2.4721 -12.06214,-9.26674 -14.29189,-15.22631 -2.02492,-5.41208 -2.05583,-6.85022 -1.78694,-83.15668 "
    "l 0.27362,-77.64936 2.71557,-5.14882 "
    "c 8.80676,-16.69791 30.7819,-19.09616 42.68291,-4.65818 6.27893,7.61744 6.10249,5.01829 6.09175,89.73514 "
    "l -0.01,77.07186 -2.33331,5 "
    "c -2.57837,5.52514 -9.04358,11.93204 -14.22547,14.09717 -4.26684,1.7828 -14.95125,1.74657 -19.11647,-0.0648 z";

static const std::string PATH_LA =
    "m 311.383770,493.89386 "
    "c -10.19192,-2.74803 -17.15706,-8.69051 -21.83919,-18.63263 "
    "l -2.59016,-5.5 -0.28233,-73.5 "
    "c -0.31521,-82.05749 -0.61125,-78.22878 6.6737,-86.31051 12.26413,-13.60551 33.23026,-10.75846 41.90779,5.69077 "
    "l 2.70084,5.11974 0.5,58.22313 0.5,58.22312 2.2764,2.27688 "
    "c 2.25594,2.2564 2.54817,2.27919 32.5,2.53458 22.7669,0.19413 30.81204,-0.0526 32.60866,-1 1.41091,-0.74401 5.14309,-6.15897 9.13768,-13.25771 64.88169,-115.30042 62.82172,-111.78431 68.92076,-117.63938 7.19039,-6.90278 13.44399,-9.35595 23.5565,-9.24077 8.68827,0.099 16.40113,3.45177 22.7464,9.88796 3.14334,3.18838 14.49395,22.36837 40.95307,69.20155 20.13246,35.63485 37.40272,66.36564 38.37835,68.29064 2.59447,5.11906 3.12226,18.37618 0.98747,24.80344 -3.516,10.58567 -14.7214,18.90673 -28.56529,21.2124 -4.19447,0.69858 -22.46137,0.97643 -51.5,0.78336 -43.93665,-0.29213 -45.09452,-0.3503 -49,-2.46157 -12.69743,-6.86414 -17.3413,-20.96601 -11.1031,-33.71642 1.50162,-3.06922 4.38912,-6.9525 6.41666,-8.62952 6.5205,-5.39325 10.59643,-5.99169 40.80892,-5.99169 14.99272,0 27.49442,-0.38013 27.78156,-0.84474 0.89012,-1.44023 -37.41964,-69.31673 -38.85297,-68.83895 -1.26349,0.42116 -1.62263,1.03963 -34.36753,59.18369 -14.86765,26.4 -28.40914,49.64887 -30.0922,51.66415 -1.68307,2.01528 -5.83902,5.05278 -9.23545,6.75 "
    "l -6.17533,3.08585 -55.59028,-0.0675 "
    "c -40.72221,-0.0494 -56.81274,-0.39711 -60.16093,-1.29988 z";

// ─────────────────────────────────────────────────────────────
//  Precomputed FILA triangle lists
// ─────────────────────────────────────────────────────────────
static std::vector<Vec2> g_trisFBody;
static std::vector<Vec2> g_trisFCross;
static std::vector<Vec2> g_trisI;
static std::vector<Vec2> g_trisLA;
//From Melaku tafere
// ─────────────────────────────────────────────────────────────
//  2D Fabric Wave function
//  Returns (waveX, waveY) displacement given normalized position
//  in flag (nx = 0..1 left..right, ny = 0..1 top..bottom)
// ─────────────────────────────────────────────────────────────
static void fabricWave(float nx, float ny,
                        float fW, float fH,
                        float& outDX, float& outDY)
{
    // Amplitude grows from left (pinned) to right edge
    float amp = nx * nx * 14.0f * windSpeed;

    // Primary horizontal wave (ripples along X)
    float phase = waveOffset - nx * 4.5f;
    float waveY_primary = sinf(phase) * amp;

    // Secondary vertical wave (fabric sag / flutter in Y)
    float ampY = nx * 5.0f * windSpeed;
    float waveY_secondary = sinf(phase * 1.3f + ny * (float)M_PI) * ampY;

    // Diagonal crumple – small high-frequency component
    float crumple = sinf(phase * 2.7f + ny * 6.0f) * nx * 2.5f * windSpeed;

    // Vertical (along-edge) wave – makes top/bottom edges undulate
    float edgeWave = cosf(waveOffset * 0.8f + ny * 3.5f) * nx * 3.5f * windSpeed;

    outDX = edgeWave * 0.3f;                       // small horizontal ripple
    outDY = waveY_primary + waveY_secondary + crumple;
}
//From Nicholas
// ─────────────────────────────────────────────────────────────
//  Flag helpers
// ─────────────────────────────────────────────────────────────
static void setColor(float r, float g, float b) { glColor3f(r,g,b); }

static void drawStar5(float cx, float cy, float outer, float inner, float tilt)
{
    const int N = 5;
    float vx[10], vy[10];
    for (int i=0;i<N;i++){
        float ao=tilt+2.0f*(float)M_PI*i/N, ai=ao+(float)M_PI/N;
        vx[2*i]=cx+outer*cosf(ao); vy[2*i]=cy+outer*sinf(ao);
        vx[2*i+1]=cx+inner*cosf(ai); vy[2*i+1]=cy+inner*sinf(ai);
    }
    glBegin(GL_TRIANGLE_FAN);
    glVertex2f(cx,cy);
    for (int i=0;i<10;i++) glVertex2f(vx[i],vy[i]);
    glVertex2f(vx[0],vy[0]);
    glEnd();
}

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
//  Draw the Comoros flag (with 2D fabric wave + shadow)
// ─────────────────────────────────────────────────────────────
static void drawFlag()
{
    const float FX = 30.0f * FLAG_SCALE;
    const float FY = 70.0f * FLAG_SCALE;
    const float FW = 440.0f * FLAG_SCALE;
    const float FH = 280.0f * FLAG_SCALE;
    const int   NX = 55, NY = 35;   // grid resolution

    // ── Transformation ──────────────────────────────────────
    // Pivot around flag centre
    float pivotX = FX + FW * 0.5f + flagTX;
    float pivotY = FY + FH * 0.5f + flagTY;
    glPushMatrix();
    glTranslatef(pivotX, pivotY, 0.f);
    glRotatef(flagRotation, 0.f, 0.f, 1.f);
    glScalef(flagScaleFactor, flagScaleFactor, 1.f);
    glTranslatef(-pivotX + flagTX, -pivotY + flagTY, 0.f);

    // ── Flag pole ────────────────────────────────────────────
    // Gradient pole (dark gray → lighter gray)
    glLineWidth(8);
    glBegin(GL_LINES);
    glColor3f(0.25f,0.25f,0.25f);  glVertex2f(FX-8.f, FY-20.f);
    glColor3f(0.55f,0.55f,0.55f);  glVertex2f(FX-8.f, FY+FH+20.f);
    glEnd();

    // ── Pole ball decoration ─────────────────────────────────
    float ballCX = FX - 8.f;
    float ballCY = FY + FH + 22.f;
    float ballR  = 10.f;

    // Shiny gold ball
    glBegin(GL_TRIANGLE_FAN);
    glColor3f(1.0f, 0.9f, 0.3f);   // bright gold center
    glVertex2f(ballCX - ballR*0.25f, ballCY + ballR*0.3f);
    for (int i=0;i<=60;i++){
        float a = 2.f*(float)M_PI*i/60;
        float t = (float)i/60;
        // shade from gold to dark gold around the circle
        float r = 1.0f - t*0.5f;
        float g2= 0.9f - t*0.4f;
        float b = 0.1f + t*0.1f;
        glColor3f(r,g2,b);
        glVertex2f(ballCX + ballR*cosf(a), ballCY + ballR*sinf(a));
    }
    glEnd();
    // Specular highlight
    glBegin(GL_TRIANGLE_FAN);
    glColor4f(1,1,1,0.7f);
    glVertex2f(ballCX - ballR*0.2f, ballCY + ballR*0.35f);
    for (int i=0;i<=30;i++){
        float a = 2.f*(float)M_PI*i/30;
        glColor4f(1,1,1,0.0f);
        glVertex2f(ballCX - ballR*0.2f + ballR*0.4f*cosf(a),
                   ballCY + ballR*0.35f + ballR*0.4f*sinf(a));
    }
    glEnd();

    // ── Build waving grid lookup ─────────────────────────────
    // grid[iy][ix] = waved screen position
    struct Pt2 { float x, y; };
    std::vector<std::vector<Pt2>> grid(NY+1, std::vector<Pt2>(NX+1));
    for (int iy=0; iy<=NY; iy++){
        for (int ix=0; ix<=NX; ix++){
            float nx = (float)ix / NX;
            float ny = (float)iy / NY;
            float bx = FX + nx * FW;
            float by = FY + ny * FH;
            float dx, dy;
            fabricWave(nx, ny, FW, FH, dx, dy);
            grid[iy][ix] = {bx + dx, by + dy};
        }
    }

    // Helper: interpolate color per stripe
    float sc[4][3] = {
        {0.00f, 0.18f, 0.68f},
        {0.82f, 0.08f, 0.08f},
        {0.96f, 0.96f, 0.96f},
        {0.96f, 0.80f, 0.00f}
    };

    // ── Drop shadow ──────────────────────────────────────────
    float shadowOff = 7.0f;
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    for (int iy=0; iy<NY; iy++){
        for (int ix=0; ix<NX; ix++){
            Pt2 bl=grid[iy  ][ix  ]; Pt2 br=grid[iy  ][ix+1];
            Pt2 tl=grid[iy+1][ix  ]; Pt2 tr=grid[iy+1][ix+1];
            glColor4f(0.0f,0.0f,0.0f,0.18f);
            glBegin(GL_QUADS);
            glVertex2f(bl.x+shadowOff, bl.y-shadowOff);
            glVertex2f(br.x+shadowOff, br.y-shadowOff);
            glVertex2f(tr.x+shadowOff, tr.y-shadowOff);
            glVertex2f(tl.x+shadowOff, tl.y-shadowOff);
            glEnd();
        }
    }

    // ── Waving stripes ───────────────────────────────────────
    for (int s=0; s<4; s++){
        int   iyMin = (s  * NY) / 4;
        int   iyMax = ((s+1) * NY) / 4;
        glColor3f(sc[s][0], sc[s][1], sc[s][2]);
        for (int iy=iyMin; iy<iyMax; iy++){
            glBegin(GL_QUAD_STRIP);
            for (int ix=0; ix<=NX; ix++){
                glVertex2f(grid[iy  ][ix].x, grid[iy  ][ix].y);
                glVertex2f(grid[iy+1][ix].x, grid[iy+1][ix].y);
            }
            glEnd();
        }
    }

    // ── Green triangle (left) ─────────────────────────────────
    // Build the three edges of the triangle as smooth waved polylines,
    // then fill the whole shape as a triangle fan.  No grid clipping →
    // no staircase / zigzag on the diagonal edges.
    setColor(0.00f, 0.48f, 0.13f);
    float triWidth = FW * 0.45f;

    // Helper: map (nx,ny) normalised flag coords → waved screen pos
    auto wavedPt = [&](float nx_, float ny_) -> Pt2 {
        float bx = FX + nx_ * FW;
        float by = FY + ny_ * FH;
        float dx, dy;
        fabricWave(nx_, ny_, FW, FH, dx, dy);
        return {bx + dx, by + dy};
    };

    const float triNX = triWidth / FW; // normalised x of the tip
    const int   EDGE_STEPS = 80;       // samples per edge → perfectly smooth

    // Collect vertices going around the triangle boundary:
    //   bottom-left corner (0,0)  → bottom-right corner (triNX along bottom edge... actually
    //   the triangle is: left-top(0,1), left-bottom(0,0), tip(triNX, 0.5)
    //   Walk: left edge top→bottom, then bottom-left→tip, then tip→top-left
    std::vector<Pt2> triPoly;

    // Edge 1: left edge, top (ny=1) → bottom (ny=0)  (nx=0 throughout)
    for (int i = 0; i <= EDGE_STEPS; i++){
        float ny_ = 1.0f - (float)i / EDGE_STEPS;   // 1 → 0
        triPoly.push_back(wavedPt(0.f, ny_));
    }
    // Edge 2: bottom-left corner (0,0) → tip (triNX, 0.5)
    for (int i = 1; i <= EDGE_STEPS; i++){
        float t   = (float)i / EDGE_STEPS;
        float nx_ = t * triNX;
        float ny_ = t * 0.5f;
        triPoly.push_back(wavedPt(nx_, ny_));
    }
    // Edge 3: tip (triNX, 0.5) → top-left corner (0,1)
    for (int i = 1; i <= EDGE_STEPS; i++){
        float t   = (float)i / EDGE_STEPS;
        float nx_ = triNX * (1.0f - t);
        float ny_ = 0.5f  + t * 0.5f;
        triPoly.push_back(wavedPt(nx_, ny_));
    }

    // Compute centroid for the fan pivot
    float cx_ = 0, cy_ = 0;
    for (auto& p : triPoly){ cx_ += p.x; cy_ += p.y; }
    cx_ /= triPoly.size(); cy_ /= triPoly.size();

    glBegin(GL_TRIANGLE_FAN);
    glVertex2f(cx_, cy_);
    for (auto& p : triPoly) glVertex2f(p.x, p.y);
    glVertex2f(triPoly[0].x, triPoly[0].y);  // close
    glEnd();

    // ── Crescent & stars ─────────────────────────────────────
    float moonR  = triWidth * 0.33f;
    float moonCX = FX + moonR + 10.f * FLAG_SCALE;
    float moonCY = FY + FH * 0.5f;
    drawCrescent(moonCX, moonCY, moonR, moonR*0.4f, moonR*0.98f);

    float starOuter = moonR * 0.20f;
    float starInner = starOuter * 0.40f;
    float startY    = moonCY - moonR * 0.6f;
    setColor(1,1,1);
    for (int i=0;i<4;i++)
        drawStar5(moonCX + moonR*0.3f,
                  startY + i*(moonR*0.45f),
                  starOuter, starInner,
                  -(float)M_PI/2.6f);

    glPopMatrix();
}

// ─────────────────────────────────────────────────────────────
//  Draw background gradient
// ─────────────────────────────────────────────────────────────
static void drawBackground()
{
    glBegin(GL_QUADS);
    // Top: pale sky blue
    glColor3f(0.72f, 0.85f, 0.96f); glVertex2f(0,     WIN_H);
    glColor3f(0.72f, 0.85f, 0.96f); glVertex2f(WIN_W, WIN_H);
    // Bottom: warm cream
    glColor3f(0.97f, 0.94f, 0.88f); glVertex2f(WIN_W, 0);
    glColor3f(0.97f, 0.94f, 0.88f); glVertex2f(0,     0);
    glEnd();
}

//From Selomon
// ─────────────────────────────────────────────────────────────
//  Draw the FILA logo
// ─────────────────────────────────────────────────────────────
static void drawFILA()
{
    glColor4f(0.05f, 0.07f, 0.38f, 1.0f);
    drawShape(g_trisFBody);
    drawShape(g_trisI);
    drawShape(g_trisLA);
    glColor4f(0.89f, 0.00f, 0.13f, 1.0f);
    drawShape(g_trisFCross);
}

// ─────────────────────────────────────────────────────────────
//  Keyboard handlers
// ─────────────────────────────────────────────────────────────
void keyboard(unsigned char key, int /*x*/, int /*y*/)
{
    switch (key)
    {
    case '+': case '=': windSpeed = std::min(5.0f, windSpeed + 0.2f); break;
    case '-': case '_': windSpeed = std::max(0.0f, windSpeed - 0.2f); break;
    case 'R': flagRotation = fmodf(flagRotation + 5.f, 360.f); break;
    case 'r': flagRotation = fmodf(flagRotation - 5.f + 360.f, 360.f); break;
    case 'S': flagScaleFactor = std::min(2.5f, flagScaleFactor + 0.05f); break;
    case 's': flagScaleFactor = std::max(0.3f, flagScaleFactor - 0.05f); break;
    case 27:  exit(0); break;
    }
    glutPostRedisplay();
}

void specialKey(int key, int /*x*/, int /*y*/)
{
    switch (key)
    {
    case GLUT_KEY_UP:    flagTY += 10.f; break;
    case GLUT_KEY_DOWN:  flagTY -= 10.f; break;
    case GLUT_KEY_LEFT:  flagTX -= 10.f; break;
    case GLUT_KEY_RIGHT: flagTX += 10.f; break;
    }
    glutPostRedisplay();
}

// ─────────────────────────────────────────────────────────────
//  Animation timer
// ─────────────────────────────────────────────────────────────
void update(int value)
{
    waveOffset += 0.06f * windSpeed;
    glutPostRedisplay();
    glutTimerFunc(16, update, 0);
}

// ─────────────────────────────────────────────────────────────
//  Display
// ─────────────────────────────────────────────────────────────
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
