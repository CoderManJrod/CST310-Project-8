// Compile:
//   g++ pacman.cpp -o pacman -lGL -lGLU -lglut -std=c++17
// Run:
//   ./pacman
//
// Controls:
//   Arrow keys  - Move Pac-Man
//   R           - Restart
//   ESC         - Quit

#ifdef __APPLE_CC__
#include <GLUT/glut.h>
#else
#include <GL/glut.h>
#endif

#include <cmath>
#include <cstdlib>
#include <ctime>
#include <vector>
#include <string>

// ─── Constants ────────────────────────────────────────────────────────────────
const int   COLS = 19, ROWS = 22;
const float CS   = 1.0f;
const float WALL_H = 0.55f;
const float PI = 3.14159265f;

// ─── Maze Layout ─────────────────────────────────────────────────────────────
// 1=wall  0=pellet  2=empty  3=power pellet  4=ghost house
int mazeBase[ROWS][COLS] = {
    {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1},
    {1,0,0,0,0,0,0,0,0,1,0,0,0,0,0,0,0,0,1},
    {1,3,1,1,0,1,1,1,0,1,0,1,1,1,0,1,1,3,1},
    {1,0,1,1,0,1,1,1,0,1,0,1,1,1,0,1,1,0,1},
    {1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
    {1,0,1,1,0,1,0,1,1,1,1,1,0,1,0,1,1,0,1},
    {1,0,0,0,0,1,0,0,0,1,0,0,0,1,0,0,0,0,1},
    {1,1,1,1,0,1,1,1,2,1,2,1,1,1,0,1,1,1,1},
    {2,2,2,1,0,1,2,2,2,2,2,2,2,1,0,1,2,2,2},
    {1,1,1,1,0,1,2,4,4,4,4,4,2,1,0,1,1,1,1},
    {2,2,2,2,0,2,2,4,4,4,4,4,2,2,0,2,2,2,2},
    {1,1,1,1,0,1,2,4,4,4,4,4,2,1,0,1,1,1,1},
    {2,2,2,1,0,1,2,2,2,2,2,2,2,1,0,1,2,2,2},
    {1,1,1,1,0,1,2,1,1,1,1,1,2,1,0,1,1,1,1},
    {1,0,0,0,0,0,0,0,0,1,0,0,0,0,0,0,0,0,1},
    {1,0,1,1,0,1,1,1,0,1,0,1,1,1,0,1,1,0,1},
    {1,3,0,1,0,0,0,0,0,2,0,0,0,0,0,1,0,3,1},
    {1,1,0,1,0,1,0,1,1,1,1,1,0,1,0,1,0,1,1},
    {1,0,0,0,0,1,0,0,0,1,0,0,0,1,0,0,0,0,1},
    {1,0,1,1,1,1,1,1,0,1,0,1,1,1,1,1,1,0,1},
    {1,0,0,0,0,0,0,0,0,1,0,0,0,0,0,0,0,0,1},
    {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1}
};

int maze[ROWS][COLS];

// ─── Entity ───────────────────────────────────────────────────────────────────
struct Entity {
    float x, y;
    int   gx, gy;
    int   dx, dy;
};

Entity pac;
Entity ghosts[4];
float  ghostColors[4][3] = {
    {1.0f, 0.0f, 0.0f},   // Red   
    {1.0f, 0.6f, 0.1f},   // Orange 
    {0.0f, 1.0f, 1.0f},   // Blue  
    {1.0f, 0.7f, 0.85f}   // Pink  
};

// ─── Ghost state machine ─────────────────────────────────────────────────────
enum GhostState { GHOST_HOUSE, GHOST_LEAVING, GHOST_ACTIVE, GHOST_EATEN };
GhostState ghostState[4];
float      ghostHouseTimer[4];
float ghostReleaseDelay[4] = {0.0f, 10.0f, 20.0f, 30.0f};

// ─── Game state ───────────────────────────────────────────────────────────────
float  mouthAngle  = 0.0f;
float  mouthDir    = 1.0f;
int    score       = 0;
int    lives       = 3;
bool   gameOver    = false;
bool   won         = false;
bool   frightened  = false;
float  frightTimer = 0.0f;
int    totalPellets= 0;
int    eaten       = 0;
int    winW = 900, winH = 720;

float  pelletBob   = 0.0f;
float  pelletBobDir= 1.0f;
float  powerPulse  = 0.0f;

// ─── Input buffering ─────────────────────────────────────────────────────────
int    nextDx = 1, nextDy = 0;
int    curDx  = 1, curDy  = 0;

// ─── Helpers ──────────────────────────────────────────────────────────────────
bool isWall(int c, int r) {
    if (c < 0 || c >= COLS || r < 0 || r >= ROWS) return true;
    return maze[r][c] == 1;
}

bool isPassable(int c, int r) {
    if (c < 0 || c >= COLS || r < 0 || r >= ROWS) return false;
    return maze[r][c] != 1;
}

float toWorldX(float c) { return c * CS - (COLS * CS) / 2.0f + CS / 2; }
float toWorldZ(float r) { return r * CS - (ROWS * CS) / 2.0f + CS / 2; }

void resetEntities() {
    pac = {9.0f, 16.0f, 9, 16, 1, 0};
    nextDx = 1; nextDy = 0;
    curDx  = 1; curDy  = 0;

    // Blinky starts outside the house, ready to go
    ghosts[0] = {9.0f, 8.0f, 9, 8, -1, 0};
    ghostState[0] = GHOST_ACTIVE;
    ghostHouseTimer[0] = 0;

    // Pinky, Inky, Clyde start inside ghost house
    ghosts[1] = {9.0f,  10.0f, 9,  10, 0, 0};
    ghosts[2] = {8.0f,  10.0f, 8,  10, 0, 0};
    ghosts[3] = {10.0f, 10.0f, 10, 10, 0, 0};
    for (int i = 1; i < 4; i++) {
        ghostState[i] = GHOST_HOUSE;
        ghostHouseTimer[i] = ghostReleaseDelay[i];
    }

    frightened = false;
    frightTimer = 0;
}

void initGame() {
    for (int r = 0; r < ROWS; r++)
        for (int c = 0; c < COLS; c++)
            maze[r][c] = mazeBase[r][c];
    totalPellets = 0;
    for (int r = 0; r < ROWS; r++)
        for (int c = 0; c < COLS; c++)
            if (maze[r][c] == 0 || maze[r][c] == 3) totalPellets++;
    score = 0; lives = 3; eaten = 0;
    gameOver = false; won = false;
    resetEntities();
}

// ─── Material helpers ─────────────────────────────────────────────────────────
void setColor(float r, float g, float b, float a = 1.0f) {
    float amb[] = {r * 0.3f, g * 0.3f, b * 0.3f, a};
    float dif[] = {r, g, b, a};
    float spc[] = {r * 0.4f, g * 0.4f, b * 0.4f, a};
    glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT,   amb);
    glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE,   dif);
    glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR,  spc);
    glMaterialf (GL_FRONT_AND_BACK, GL_SHININESS, 30.0f);
    glColor4f(r, g, b, a);
}

void setColorFlat(float r, float g, float b) {
    float amb[] = {r, g, b, 1};
    float dif[] = {r, g, b, 1};
    float zero[] = {0, 0, 0, 1};
    glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT,  amb);
    glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE,  dif);
    glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, zero);
    glMaterialf (GL_FRONT_AND_BACK, GL_SHININESS, 0);
    glColor3f(r, g, b);
}

// ─── Full 3D box ──────────────────────────────────────────────────────────────
void drawFullBox(float cx, float cz, float sx, float sz, float h,
                 float r, float g, float b) {
    float hw = sx / 2, hd = sz / 2;
    setColor(r, g, b);
    glBegin(GL_QUADS);
    glNormal3f(0, 1, 0);
    glVertex3f(cx - hw, h, cz - hd);
    glVertex3f(cx + hw, h, cz - hd);
    glVertex3f(cx + hw, h, cz + hd);
    glVertex3f(cx - hw, h, cz + hd);
    glEnd();
    setColor(r * 0.72f, g * 0.72f, b * 0.72f);
    glBegin(GL_QUADS);
    glNormal3f(0, 0, 1);
    glVertex3f(cx - hw, 0, cz + hd);
    glVertex3f(cx + hw, 0, cz + hd);
    glVertex3f(cx + hw, h, cz + hd);
    glVertex3f(cx - hw, h, cz + hd);
    glEnd();
    glBegin(GL_QUADS);
    glNormal3f(0, 0, -1);
    glVertex3f(cx + hw, 0, cz - hd);
    glVertex3f(cx - hw, 0, cz - hd);
    glVertex3f(cx - hw, h, cz - hd);
    glVertex3f(cx + hw, h, cz - hd);
    glEnd();
    setColor(r * 0.55f, g * 0.55f, b * 0.55f);
    glBegin(GL_QUADS);
    glNormal3f(-1, 0, 0);
    glVertex3f(cx - hw, 0, cz + hd);
    glVertex3f(cx - hw, 0, cz - hd);
    glVertex3f(cx - hw, h, cz - hd);
    glVertex3f(cx - hw, h, cz + hd);
    glEnd();
    glBegin(GL_QUADS);
    glNormal3f(1, 0, 0);
    glVertex3f(cx + hw, 0, cz - hd);
    glVertex3f(cx + hw, 0, cz + hd);
    glVertex3f(cx + hw, h, cz + hd);
    glVertex3f(cx + hw, h, cz - hd);
    glEnd();
}

// ─── Floor ────────────────────────────────────────────────────────────────────
void drawFloor() {
    float hw = (COLS * CS) / 2.0f + 0.5f;
    float hd = (ROWS * CS) / 2.0f + 0.5f;
    setColorFlat(0.02f, 0.02f, 0.06f);
    glBegin(GL_QUADS);
    glNormal3f(0, 1, 0);
    glVertex3f(-hw, -0.01f, -hd);
    glVertex3f( hw, -0.01f, -hd);
    glVertex3f( hw, -0.01f,  hd);
    glVertex3f(-hw, -0.01f,  hd);
    glEnd();
    setColorFlat(0.06f, 0.06f, 0.12f);
    glLineWidth(1.0f);
    glBegin(GL_LINES);
    for (int r = 0; r < ROWS; r++) {
        for (int c = 0; c < COLS; c++) {
            if (maze[r][c] != 1) {
                float wx = toWorldX(c);
                float wz = toWorldZ(r);
                float hs = CS / 2;
                glVertex3f(wx - hs * 0.3f, 0.001f, wz);
                glVertex3f(wx + hs * 0.3f, 0.001f, wz);
                glVertex3f(wx, 0.001f, wz - hs * 0.3f);
                glVertex3f(wx, 0.001f, wz + hs * 0.3f);
            }
        }
    }
    glEnd();
}

// ─── Ghost house gate ─────────────────────────────────────────────────────────
void drawGhostHouseGate() {
    float x1 = toWorldX(7)  - CS / 2;
    float x2 = toWorldX(11) + CS / 2;
    float z  = toWorldZ(9)  - CS / 2;
    setColor(1.0f, 0.6f, 0.8f);
    glBegin(GL_QUADS);
    glNormal3f(0, 0, -1);
    glVertex3f(x1, 0.0f,  z);
    glVertex3f(x2, 0.0f,  z);
    glVertex3f(x2, WALL_H * 0.45f, z);
    glVertex3f(x1, WALL_H * 0.45f, z);
    glEnd();
}

// ─── Maze ─────────────────────────────────────────────────────────────────────
void drawMaze() {
    for (int r = 0; r < ROWS; r++) {
        for (int c = 0; c < COLS; c++) {
            if (maze[r][c] == 1) {
                float wx = toWorldX(c);
                float wz = toWorldZ(r);
                bool border = (r == 0 || r == ROWS - 1 || c == 0 || c == COLS - 1);
                float br = border ? 0.15f : 0.08f;
                float bg = border ? 0.15f : 0.08f;
                float bb = border ? 1.0f  : 0.85f;
                drawFullBox(wx, wz, CS, CS, WALL_H, br, bg, bb);
            }
        }
    }
}

// ─── Pellets ──────────────────────────────────────────────────────────────────
void drawPellets() {
    for (int r = 0; r < ROWS; r++) {
        for (int c = 0; c < COLS; c++) {
            float wx = toWorldX(c);
            float wz = toWorldZ(r);
            if (maze[r][c] == 0) {
                setColor(1.0f, 0.88f, 0.65f);
                glPushMatrix();
                glTranslatef(wx, 0.12f + pelletBob * 0.015f, wz);
                glutSolidSphere(0.07, 8, 8);
                glPopMatrix();
            } else if (maze[r][c] == 3) {
                float pulse = 0.85f + 0.15f * sinf(powerPulse);
                setColor(pulse, pulse, 0.85f * pulse);
                glPushMatrix();
                glTranslatef(wx, 0.18f + pelletBob * 0.03f, wz);
                glutSolidSphere(0.16f + 0.03f * sinf(powerPulse), 12, 12);
                glPopMatrix();
            }
        }
    }
}

// ─── Pac-Man ──────────────────────────────────────────────────────────────────
void drawPacmanBody(float openDeg) {
    int slices = 24, stacks = 16;
    float radius = 0.38f;
    float mouthRad = openDeg * PI / 180.0f;
    for (int i = 0; i < stacks; i++) {
        float phi0 = PI * i / stacks;
        float phi1 = PI * (i + 1) / stacks;
        glBegin(GL_QUAD_STRIP);
        for (int j = 0; j <= slices; j++) {
            float theta = 2.0f * PI * j / slices;
            for (int k = 1; k >= 0; k--) {
                float phi = (k == 1) ? phi0 : phi1;
                float x = radius * sinf(phi) * cosf(theta);
                float y = radius * cosf(phi);
                float z = radius * sinf(phi) * sinf(theta);
                float angle = atan2f(y, x);
                if (fabsf(angle) < mouthRad / 2.0f && x > 0) {
                    x *= 0.05f; y *= 0.05f; z *= 0.05f;
                }
                glNormal3f(x, y, z);
                glVertex3f(x, y, z);
            }
        }
        glEnd();
    }
}

void drawPacman() {
    float wx = pac.x * CS - (COLS * CS) / 2.0f + CS / 2;
    float wz = pac.y * CS - (ROWS * CS) / 2.0f + CS / 2;
    setColor(1.0f, 0.92f, 0.0f);
    glPushMatrix();
    glTranslatef(wx, 0.38f, wz);
    float ang = 0;
    if (curDx ==  1) ang = 0;
    if (curDx == -1) ang = 180;
    if (curDy ==  1) ang = -90;
    if (curDy == -1) ang = 90;
    glRotatef(ang, 0, 1, 0);
    drawPacmanBody(mouthAngle);
    glPopMatrix();
}

// ─── Ghost ────────────────────────────────────────────────────────────────────
void drawGhostBody() {
    int slices = 20, stacks = 12;
    float radius = 0.36f;
    for (int i = 0; i < stacks; i++) {
        float phi0 = (PI / 2.0f) * i / stacks;
        float phi1 = (PI / 2.0f) * (i + 1) / stacks;
        glBegin(GL_QUAD_STRIP);
        for (int j = 0; j <= slices; j++) {
            float theta = 2.0f * PI * j / slices;
            for (int k = 0; k < 2; k++) {
                float phi = (k == 0) ? phi0 : phi1;
                float x = radius * cosf(phi) * cosf(theta);
                float y = radius * sinf(phi);
                float z = radius * cosf(phi) * sinf(theta);
                glNormal3f(x, y, z);
                glVertex3f(x, y + radius * 0.4f, z);
            }
        }
        glEnd();
    }
    float skirtH = radius * 0.7f;
    glBegin(GL_QUAD_STRIP);
    for (int j = 0; j <= slices; j++) {
        float theta = 2.0f * PI * j / slices;
        float x = radius * cosf(theta);
        float z = radius * sinf(theta);
        glNormal3f(x, 0, z);
        glVertex3f(x, radius * 0.4f, z);
        float wave = 0.04f * sinf(theta * 3.0f);
        glVertex3f(x, -skirtH + wave, z);
    }
    glEnd();
}

void drawGhost(int i) {
    float wx = ghosts[i].x * CS - (COLS * CS) / 2.0f + CS / 2;
    float wz = ghosts[i].y * CS - (ROWS * CS) / 2.0f + CS / 2;

    // Eaten ghosts: just draw floating eyes heading back to house
    if (ghostState[i] == GHOST_EATEN) {
        glPushMatrix();
        glTranslatef(wx, 0.36f, wz);
        for (int side = -1; side <= 1; side += 2) {
            setColor(1, 1, 1);
            glPushMatrix();
            glTranslatef(side * 0.12f, 0.14f, 0.28f);
            glutSolidSphere(0.09f, 8, 8);
            setColor(0.05f, 0.05f, 0.2f);
            glTranslatef(0, 0, 0.05f);
            glutSolidSphere(0.05f, 8, 8);
            glPopMatrix();
        }
        glPopMatrix();
        return;
    }

    glPushMatrix();
    glTranslatef(wx, 0.36f, wz);

    if (frightened && ghostState[i] == GHOST_ACTIVE) {
        float flash = (frightTimer < 2.0f) ? (0.5f + 0.5f * sinf(frightTimer * 12.0f)) : 1.0f;
        setColor(0.1f, 0.1f, 0.9f * flash);
    } else {
        setColor(ghostColors[i][0], ghostColors[i][1], ghostColors[i][2]);
    }
    drawGhostBody();

    // Eyes with tracking pupils
    for (int side = -1; side <= 1; side += 2) {
        setColor(1, 1, 1);
        glPushMatrix();
        glTranslatef(side * 0.12f, 0.14f, 0.28f);
        glutSolidSphere(0.09f, 8, 8);
        setColor(0.05f, 0.05f, 0.2f);
        glTranslatef(ghosts[i].dx * 0.03f, 0.0f, ghosts[i].dy * -0.03f + 0.05f);
        glutSolidSphere(0.05f, 8, 8);
        glPopMatrix();
    }
    glPopMatrix();
}

// ─── HUD ──────────────────────────────────────────────────────────────────────
void drawHUD() {
    glDisable(GL_LIGHTING);
    glMatrixMode(GL_PROJECTION);
    glPushMatrix(); glLoadIdentity();
    gluOrtho2D(0, winW, 0, winH);
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix(); glLoadIdentity();

    glColor3f(1, 1, 0);
    glRasterPos2i(12, winH - 24);
    std::string s = "SCORE: " + std::to_string(score) + "   LIVES: " + std::to_string(lives);
    for (char ch : s) glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, ch);

    if (gameOver) {
        glColor3f(1, 0.15f, 0.15f);
        glRasterPos2i(winW / 2 - 110, winH / 2);
        std::string g = "GAME OVER  (R to restart)";
        for (char ch : g) glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, ch);
    }
    if (won) {
        glColor3f(0.2f, 1, 0.2f);
        glRasterPos2i(winW / 2 - 100, winH / 2);
        std::string w = "YOU WIN!  (R to restart)";
        for (char ch : w) glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, ch);
    }

    glMatrixMode(GL_PROJECTION); glPopMatrix();
    glMatrixMode(GL_MODELVIEW);  glPopMatrix();
    glEnable(GL_LIGHTING);
}

// ─── Display ──────────────────────────────────────────────────────────────────
void display() {
    glClearColor(0.0f, 0.0f, 0.02f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glLoadIdentity();

    gluLookAt(0.0, 24.0, 10.0,  0.0, 0.0, -1.0,  0.0, 1.0, 0.0);

    drawFloor();
    drawMaze();
    drawGhostHouseGate();
    drawPellets();
    drawPacman();
    for (int i = 0; i < 4; i++) drawGhost(i);
    drawHUD();
    glutSwapBuffers();
}

// ─── Pac-Man movement with input buffering ────────────────────────────────────
void movePacman() {
    // Try buffered direction first (player's latest input)
    int nx = pac.gx + nextDx;
    int ny = pac.gy + nextDy;
    if (nx < 0) nx = COLS - 1;
    if (nx >= COLS) nx = 0;

    if (isPassable(nx, ny)) {
        curDx = nextDx; curDy = nextDy;
        pac.gx = nx; pac.gy = ny;
    } else {
        // Buffered blocked — keep going current direction
        nx = pac.gx + curDx;
        ny = pac.gy + curDy;
        if (nx < 0) nx = COLS - 1;
        if (nx >= COLS) nx = 0;
        if (isPassable(nx, ny)) {
            pac.gx = nx; pac.gy = ny;
        }
    }

    // Collect
    int pr = pac.gy, pc = pac.gx;
    if (maze[pr][pc] == 0) { score += 10; eaten++; maze[pr][pc] = 2; }
    if (maze[pr][pc] == 3) {
        score += 50; eaten++; maze[pr][pc] = 2;
        frightened = true; frightTimer = 7.0f;
    }
    if (eaten >= totalPellets) won = true;
}

// ─── Ghost house release / respawn logic ──────────────────────────────────────
void updateGhostHouse(int i, float dt) {
    switch (ghostState[i]) {
        case GHOST_HOUSE:
            ghostHouseTimer[i] -= dt;
            if (ghostHouseTimer[i] <= 0) {
                ghostState[i] = GHOST_LEAVING;
            }
            break;

        case GHOST_LEAVING:
            // Navigate out: center on col 9, then move up to row 8
            if (ghosts[i].gx < 9)      { ghosts[i].gx++; ghosts[i].dx =  1; ghosts[i].dy = 0; }
            else if (ghosts[i].gx > 9)  { ghosts[i].gx--; ghosts[i].dx = -1; ghosts[i].dy = 0; }
            else if (ghosts[i].gy > 8)  { ghosts[i].gy--; ghosts[i].dx =  0; ghosts[i].dy = -1; }
            else {
                ghostState[i] = GHOST_ACTIVE;
                ghosts[i].dx = (rand() % 2) ? 1 : -1;
                ghosts[i].dy = 0;
            }
            break;

        case GHOST_EATEN:
            // Navigate back to house center (col 9, row 10)
            if (ghosts[i].gx < 9)       { ghosts[i].gx++; ghosts[i].dx =  1; ghosts[i].dy = 0; }
            else if (ghosts[i].gx > 9)   { ghosts[i].gx--; ghosts[i].dx = -1; ghosts[i].dy = 0; }
            else if (ghosts[i].gy < 10)  { ghosts[i].gy++; ghosts[i].dx =  0; ghosts[i].dy =  1; }
            else if (ghosts[i].gy > 10)  { ghosts[i].gy--; ghosts[i].dx =  0; ghosts[i].dy = -1; }
            else {
                // Back home — wait 3 seconds before re-emerging
                ghostState[i] = GHOST_HOUSE;
                ghostHouseTimer[i] = 3.0f;
            }
            break;

        case GHOST_ACTIVE:
            break;
    }
}

// ─── Ghost active movement ───────────────────────────────────────────────────
// ─── Ghost active movement ───────────────────────────────────────────────────
void moveGhost(int i) {
    // Only move active or frightened ghosts
    if (ghostState[i] != GHOST_ACTIVE && ghostState[i] != frightened) return;

    int dirs[4][2] = {{1,0},{-1,0},{0,1},{0,-1}};

    int bestDx = ghosts[i].dx;
    int bestDy = ghosts[i].dy;

    int targetX = pac.gx;
    int targetY = pac.gy;

    // ─── FRIGHTENED MODE FOR ALL GHOSTS ───
    if (ghostState[i] == frightened && ghostHouseTimer[i] >= ghostReleaseDelay[i]) {
        std::vector<int> options;
        float bestDist = -1.0f; // move to increase distance from Pacman

        for (int d = 0; d < 4; d++) {
            int nx = ghosts[i].gx + dirs[d][0];
            int ny = ghosts[i].gy + dirs[d][1];

            if (nx < 0) nx = COLS - 1;
            if (nx >= COLS) nx = 0;

            if (!isPassable(nx, ny) || maze[ny][nx] == 4) continue;
            if (dirs[d][0] == -ghosts[i].dx && dirs[d][1] == -ghosts[i].dy) continue;

            float dist = (nx - pac.gx)*(nx - pac.gx) + (ny - pac.gy)*(ny - pac.gy);
            if (dist > bestDist) {
                bestDist = dist;
                options.clear();
                options.push_back(d);
            } else if (dist == bestDist) {
                options.push_back(d);
            }
        }

        if (!options.empty()) {
            int r = options[rand() % options.size()];
            ghosts[i].dx = dirs[r][0];
            ghosts[i].dy = dirs[r][1];
        }

        ghosts[i].gx += ghosts[i].dx;
        ghosts[i].gy += ghosts[i].dy;

        // Wraparound
        if (ghosts[i].gx < 0) ghosts[i].gx = COLS - 1;
        if (ghosts[i].gx >= COLS) ghosts[i].gx = 0;

        return; // done for frightened ghosts
    }

    // ─── PERSONALITIES ───
    if (i == 0) { // 🔴 RED → chase behind Pacman
        targetX = pac.gx - curDx * 2;
        targetY = pac.gy - curDy * 2;
    } 
    else if (i == 1) { // 🟠 ORANGE → random at intersections
        std::vector<int> options;
        for (int d = 0; d < 4; d++) {
            int tx = ghosts[i].gx + dirs[d][0];
            int ty = ghosts[i].gy + dirs[d][1];

            if (tx < 0) tx = COLS - 1;
            if (tx >= COLS) tx = 0;

            if (isPassable(tx, ty) && maze[ty][tx] != 4) {
                if (dirs[d][0] != -ghosts[i].dx || dirs[d][1] != -ghosts[i].dy) {
                    options.push_back(d);
                }
            }
        }

        if (!options.empty()) {
            int r = options[rand() % options.size()];
            ghosts[i].dx = dirs[r][0];
            ghosts[i].dy = dirs[r][1];
        }

        ghosts[i].gx += ghosts[i].dx;
        ghosts[i].gy += ghosts[i].dy;

        // Wraparound
        if (ghosts[i].gx < 0) ghosts[i].gx = COLS - 1;
        if (ghosts[i].gx >= COLS) ghosts[i].gx = 0;

        return;
    }
    else if (i == 2) { // 🔵 BLUE → ahead of Pacman
        targetX = pac.gx + curDx * 2;
        targetY = pac.gy + curDy * 2;
    }
    else if (i == 3) { // 🟣 PINK → direct chase
        targetX = pac.gx;
        targetY = pac.gy;
    }

    // ─── Standard chasing logic for Red, Blue, Pink ───
    float bestDist = 1e9;

    for (int d = 0; d < 4; d++) {
        int nx = ghosts[i].gx + dirs[d][0];
        int ny = ghosts[i].gy + dirs[d][1];

        if (nx < 0) nx = COLS - 1;
        if (nx >= COLS) nx = 0;

        if (!isPassable(nx, ny) || maze[ny][nx] == 4) continue;

        float dist = (nx - targetX)*(nx - targetX) + (ny - targetY)*(ny - targetY);

        if (dist < bestDist) {
            bestDist = dist;
            bestDx = dirs[d][0];
            bestDy = dirs[d][1];
        }
    }

    ghosts[i].dx = bestDx;
    ghosts[i].dy = bestDy;

    int nx = ghosts[i].gx + ghosts[i].dx;
    int ny = ghosts[i].gy + ghosts[i].dy;

    if (nx < 0) nx = COLS - 1;
    if (nx >= COLS) nx = 0;

    if (isPassable(nx, ny) && maze[ny][nx] != 4) {
        ghosts[i].gx = nx;
        ghosts[i].gy = ny;
    }
}

// ─── Collision ────────────────────────────────────────────────────────────────
void checkGhostCollision() {
    for (int i = 0; i < 4; i++) {
        if (ghostState[i] != GHOST_ACTIVE) continue;

        float ddx = ghosts[i].x - pac.x;
        float ddy = ghosts[i].y - pac.y;
        if (fabsf(ddx) < 0.8f && fabsf(ddy) < 0.8f) {
            if (frightened) {
                score += 200;
                ghostState[i] = GHOST_EATEN;
            } else {
                lives--;
                if (lives <= 0) gameOver = true;
                else resetEntities();
            }
        }
    }
}

// ─── Smooth lerp ──────────────────────────────────────────────────────────────
void lerpEntities(float speed) {
    pac.x += (pac.gx - pac.x) * speed;
    pac.y += (pac.gy - pac.y) * speed;
    for (int i = 0; i < 4; i++) {
        float gs = (ghostState[i] == GHOST_EATEN) ? 0.45f : speed;
        ghosts[i].x += (ghosts[i].gx - ghosts[i].x) * gs;
        ghosts[i].y += (ghosts[i].gy - ghosts[i].y) * gs;
    }
}

// ─── Timer ────────────────────────────────────────────────────────────────────
int moveCounter = 0;
void timer(int) {
    float dt = 0.016f;

    if (!gameOver && !won) {
        mouthAngle += 3.5f * mouthDir;
        if (mouthAngle > 45 || mouthAngle < 0) mouthDir *= -1;

        pelletBob += 0.08f * pelletBobDir;
        if (pelletBob > 1.0f || pelletBob < -1.0f) pelletBobDir *= -1;
        powerPulse += 0.12f;

        // Ghost house timers tick every frame
        for (int i = 0; i < 4; i++) {
            if (ghostState[i] == GHOST_HOUSE)
                updateGhostHouse(i, dt);
        }

        moveCounter++;

        // Pac-Man moves every 6 frames
        if (moveCounter % 6 == 0) movePacman();

        // Active ghosts move every 5 frames
        if (moveCounter % 8 == 0) {
            for (int i = 0; i < 4; i++) moveGhost(i);
        }

        // Leaving / eaten ghosts step every 6 frames
        if (moveCounter % 6 == 0) {
            for (int i = 0; i < 4; i++) {
                if (ghostState[i] == GHOST_LEAVING || ghostState[i] == GHOST_EATEN)
                    updateGhostHouse(i, 0);
            }
        }

        lerpEntities(0.35f);

        if (frightened) {
            frightTimer -= dt;
            if (frightTimer <= 0) frightened = false;
        }
        checkGhostCollision();
    }
    glutPostRedisplay();
    glutTimerFunc(16, timer, 0);
}

// ─── Keyboard ─────────────────────────────────────────────────────────────────
void special(int key, int, int) {
    if (gameOver || won) return;
    switch (key) {
        case GLUT_KEY_RIGHT: nextDx =  1; nextDy = 0; break;
        case GLUT_KEY_LEFT:  nextDx = -1; nextDy = 0; break;
        case GLUT_KEY_DOWN:  nextDx =  0; nextDy = 1; break;
        case GLUT_KEY_UP:    nextDx =  0; nextDy =-1; break;
    }
}

void keyboard(unsigned char key, int, int) {
    if (key == 'r' || key == 'R') initGame();
    if (key == 27) exit(0);
}

// ─── Reshape ──────────────────────────────────────────────────────────────────
void reshape(int w, int h) {
    winW = w; winH = (h == 0) ? 1 : h;
    glViewport(0, 0, winW, winH);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(50, (float)winW / winH, 0.1, 100);
    glMatrixMode(GL_MODELVIEW);
}

// ─── Lighting ─────────────────────────────────────────────────────────────────
void initLighting() {
    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);
    glEnable(GL_LIGHT1);
    glEnable(GL_COLOR_MATERIAL);
    glEnable(GL_NORMALIZE);
    glShadeModel(GL_SMOOTH);

    float pos0[] = {0.0f, 20.0f, 10.0f, 1.0f};
    float amb0[] = {0.15f, 0.15f, 0.18f, 1.0f};
    float dif0[] = {0.85f, 0.85f, 0.95f, 1.0f};
    float spc0[] = {0.5f, 0.5f, 0.6f, 1.0f};
    glLightfv(GL_LIGHT0, GL_POSITION, pos0);
    glLightfv(GL_LIGHT0, GL_AMBIENT,  amb0);
    glLightfv(GL_LIGHT0, GL_DIFFUSE,  dif0);
    glLightfv(GL_LIGHT0, GL_SPECULAR, spc0);

    float pos1[] = {-12.0f, 8.0f, -8.0f, 1.0f};
    float dif1[] = {0.25f, 0.25f, 0.35f, 1.0f};
    float zero[] = {0, 0, 0, 1};
    glLightfv(GL_LIGHT1, GL_POSITION, pos1);
    glLightfv(GL_LIGHT1, GL_AMBIENT,  zero);
    glLightfv(GL_LIGHT1, GL_DIFFUSE,  dif1);
    glLightfv(GL_LIGHT1, GL_SPECULAR, zero);
}

// ─── Main ─────────────────────────────────────────────────────────────────────
int main(int argc, char** argv) {
    srand((unsigned)time(nullptr));
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
    glutInitWindowSize(winW, winH);
    glutCreateWindow("Pac-Man 2.5D");

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    initLighting();
    initGame();

    glutDisplayFunc(display);
    glutReshapeFunc(reshape);
    glutSpecialFunc(special);
    glutKeyboardFunc(keyboard);
    glutTimerFunc(0, timer, 0);
    glutMainLoop();
}
