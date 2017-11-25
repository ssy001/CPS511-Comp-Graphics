/*******************************************************************
	       City Modelling Program
********************************************************************/
//////////
#include <windows.h>
#include <gl/gl.h>
#include <gl/glu.h>
//#include <gl/glut.h>
#include "Dependencies\freeglut\freeglut.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <utility>
#include <vector>
#include "VECTOR3D.h"

#include "TerrainGrid.h"
#include "Mesh.h"
#include "RGBpixmap.h"
#include "ComplexObj.h"

void initOpenGL();
void display(void);
void reshape(int w, int h);
void mouse(int button, int state, int x, int y);
void mouseMotionHandler(int xMouse, int yMouse);
void keyboard(unsigned char key, int x, int y);
void functionKeys(int key, int x, int y);
void timer(int value);
VECTOR3D ScreenToWorld(int x, int y);
void updateCameraPos();
void limitCameraAngle();

bool hasCollision(const Mesh* a, Mesh* b[], Mesh* c[], Mesh* d[], float offset, int sizeb, int sizec, int sized);
bool hasObjCollision(const Mesh* a, ComplexObj *cobj[], int sizee);
void createBoundaryWalls();
void createInnerWalls();
void createDoors();

static int currentButton;
static unsigned char currentKey;
#define M_PI 3.14159265358979323846
const float FPS = 30.0;

// City Interaction State Variable
enum Action {TRANSLATE, ROTATE, RAISE, SCALE, EXTRUDE, SELECT, MULTIPLESELECT, DESELECT_ALL, NAVIGATE};
enum Action currentAction = TRANSLATE;

GLfloat light_position0[] = {12.0, 6.0,-12.0,1.0};
GLfloat light_position1[] = {-12.0, 6.0,12.0,1.0};
GLfloat light_diffuse[]   = {1.0, 1.0, 1.0, 1.0};
GLfloat light_specular[]  = {1.0, 1.0, 1.0, 1.0};
GLfloat light_ambient[]   = {0.2, 0.2, 0.2, 1.0};

// Person mesh - head + body
Mesh *personBody;
GLUquadricObj *personHead;

// City terrain mesh
TerrainGrid *terrainGrid = NULL;
int gridSize = 16;

// Wall Meshes
Mesh *boundaryWalls[4]; // boundary walls that surround the world
Mesh *innerWalls[20];   // inner walls of the rooms & world
Mesh *doors[20];   // inner walls of the rooms & world
float wallHeight = 1.0f; // wall height of all (boundary, inner) walls
int numBoundaryWalls = 0, numInnerWalls = 0, numDoors = 0;
RGBpixmap floorPix[1], boundaryWallPix[1], innerWallPix[1], doorPix[2];

// Room Objects
ComplexObj *cobj[12];
//int nextobj = 0;               //index to objects array to store next cube
ComplexObj *navobj;  //selected object in NAVIGATE mode
bool objCollision = false; // state to toggle avatar and object collision

float trsize = 0.2, scsize = 0.2, rosize = 5.0;      //translation, scaling and rotation angle deltas

// Textures 
GLuint textureId;

// Camera Control
VECTOR3D lookFrom;
VECTOR3D lookAt;
VECTOR3D up;

float radius = 8;			// Camera Distance
float lookFromx = 0;		// Camera X Position
float lookFromy = 0;	// Camera Y Position
float lookFromz = radius;		// Camera Z Position

float angleTheta = 0;		// Camera X angle
float anglePhi = 80;		// Camera Y angle

float upx = 0;			// Up Vector
float upy = 1;
float upz = 0;

float lookAtx = 0;		// Camera is looking at
float lookAty = 0;
float lookAtz = 0;

float camerax = 0;		// Camera X Position
float cameray = 0;	// Camera Y Position
float cameraz = radius;		// Camera Z Position

static float zoomFactor = 1.0; 

float xbefore;			// Previous X position of tank
float zbefore;			// Previous Y position of tank

float collision_offset = 0.4;

//GLint glutWindowWidth = 750;
//GLint glutWindowHeight = 500;
GLint glutWindowWidth = 1200;
GLint glutWindowHeight = 800;
GLint viewportWidth = glutWindowWidth;
GLint viewportHeight = glutWindowHeight;

// Wolrd Boundaries
GLdouble worldLeftBase  =  -8.0;
GLdouble worldRightBase =  8.0;
GLdouble worldBottomBase=  -8.0;
GLdouble worldTopBase   =  8.0;

// World view boundaries
GLdouble wvLeftBase		=  worldLeftBase, 
         wvRightBase	=  worldRightBase,
		 wvBottomBase	=  worldBottomBase, 
		 wvTopBase		=  worldTopBase;

int main(int argc, char **argv){
  glutInit(&argc,argv);
  glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
  //glutInitWindowSize(750, 500);
  glutInitWindowSize(glutWindowWidth, glutWindowHeight);
  glutInitWindowPosition(100, 100);
  glutCreateWindow("Room Navigator");

  floorPix[0].readBMPFile("FloorWood01.bmp");
  boundaryWallPix[0].readBMPFile("WallBrick02.bmp");
  innerWallPix[0].readBMPFile("WallWood01.bmp");
  doorPix[0].readBMPFile("Door01L.bmp");
  doorPix[1].readBMPFile("Door01R.bmp");

  initOpenGL();

  glutDisplayFunc(display);
  glutReshapeFunc(reshape);
  glutMouseFunc(mouse);
  glutMotionFunc(mouseMotionHandler);
  glutKeyboardFunc(keyboard);
  glutSpecialFunc(functionKeys);
  glutTimerFunc(1000.0 / FPS, timer, 0);
  glutMainLoop();
  return 0;
}

int width, height;
VECTOR3D scale;
VECTOR3D trans;
VECTOR3D angles;

// Setup openGL */
void initOpenGL(){
  glViewport(0, 0, (GLsizei) viewportWidth, (GLsizei) viewportHeight);
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  gluPerspective(60.0*zoomFactor,(float)viewportWidth/(float)viewportHeight,0.2,80.0);
  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();

  // Set up and enable lighting
  glLightfv(GL_LIGHT0, GL_AMBIENT, light_ambient);
  glLightfv(GL_LIGHT0, GL_DIFFUSE, light_diffuse);
  glLightfv(GL_LIGHT0, GL_SPECULAR, light_specular);
  glLightfv(GL_LIGHT1, GL_AMBIENT, light_ambient);
  glLightfv(GL_LIGHT1, GL_DIFFUSE, light_diffuse);
  glLightfv(GL_LIGHT1, GL_SPECULAR, light_specular);
  
  glLightfv(GL_LIGHT0, GL_POSITION, light_position0);
  glLightfv(GL_LIGHT1, GL_POSITION, light_position1);
  glShadeModel(GL_SMOOTH);
  glEnable(GL_LIGHTING);
  glEnable(GL_LIGHT0);
  glEnable(GL_LIGHT1);

  glEnable(GL_DEPTH_TEST);
  glShadeModel(GL_SMOOTH);
  glClearColor(0.6, 0.6, 0.6, 0.0);  
  glClearDepth(1.0f);
  glEnable(GL_DEPTH_TEST);
  glEnable(GL_NORMALIZE);
  //Nice perspective.
  glHint(GL_PERSPECTIVE_CORRECTION_HINT , GL_NICEST);
  
  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();
  updateCameraPos();
  gluLookAt(lookFromx, lookFromy, lookFromz,lookAtx, lookAty, lookAtz, upx, upy, upz);
  
  //Texture
  glGenTextures(1, &textureId);
  glPixelStorei(GL_UNPACK_ALIGNMENT, 1); // store pixels by byte	
  glBindTexture(GL_TEXTURE_2D, textureId); // select current texture (0)
  glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);
  glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_DECAL);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

  // Set up Terrain Grid
  VECTOR3D origin = VECTOR3D(-16.0f, 0.0f, 16.0f);
  terrainGrid = new TerrainGrid(gridSize, 32.0); // gridsize = 16
  terrainGrid->InitGrid(gridSize, origin, 32.0, 32.0);

  createBoundaryWalls(); // Create boundary walls
  createInnerWalls(); // create inner walls
  createDoors(); // create doors

  // Create a person (Head+Body) - initially oriented along z axis direction (-ve z dir (0,0,-1) & ccw => angles.y = 0)
  scale.x = 0.5;   scale.y = 1.4;   scale.z = 0.2;
  trans.x = -2.0;   trans.y = 0;     trans.z = 8.0;
  personBody = createMesh(scale, trans, 1.0, 1);
  personBody->angles.x = personBody->angles.y = personBody->angles.z = 0.0;
  personBody->selected = true;

  // Create room objects (4 kinds)
  for (int i = 0; i < 12; i++){
	  cobj[i] = createObj();
  }
  createShield(cobj[0]); cobj[0]->translation.Set( 0.0, 2.0,  15.0); cobj[0]->angles.Set(0.0, 180.0, 0.0);
  createShield(cobj[1]); cobj[1]->translation.Set(-8.0, 2.0, -15.0);
  createShield(cobj[2]); cobj[2]->translation.Set( 8.0, 2.0, -15.0);
  createTable( cobj[3]); cobj[3]->translation.Set(14.0, 0.0, 2.0);
  createTable( cobj[4]); cobj[4]->translation.Set(14.0, 0.0,-2.0);
  createTable( cobj[5]); cobj[5]->translation.Set(-14.0,0.0,-2.0);
  createVase(  cobj[6]); cobj[6]->translation.Set(14.0, 0.0, 4.0);
  createVase(  cobj[7]); cobj[7]->translation.Set(-14.0, 0.0,-14.0);
  createVase(  cobj[8]); cobj[8]->translation.Set(14.0, 0.0,-14.0);
  createPicFrame(cobj[9]);  cobj[9]->translation.Set(8.0, 2.0, 15.0);    cobj[9]->angles.Set(0.0, 180.0, 0.0);
  createPicFrame(cobj[10]); cobj[10]->translation.Set(-15.0, 2.0, -8.0); cobj[10]->angles.Set(0.0, 90.0, 0.0);
  createPicFrame(cobj[11]); cobj[11]->translation.Set(15.0, 2.0, -8.0);  cobj[11]->angles.Set(0.0, -90.0, 0.0);
}

void display(void){
	glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_DECAL);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  glLoadIdentity();

  if (GetAsyncKeyState(VK_END)) {
	  radius+=0.25;
	  zoomFactor += 0.02;
  }
  if (GetAsyncKeyState(VK_HOME)) {
	  if (radius > 2)
		  radius-=0.25;
	  zoomFactor -= 0.02;
  }
  // Zoom by changing view frustum
  /*
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  gluPerspective(60.0*zoomFactor,(float)width/(float)height,0.2,80.0);
  glMatrixMode(GL_MODELVIEW);
  */

  if (currentAction == NAVIGATE)
	  ;
  else
    updateCameraPos();
  gluLookAt(lookFromx, lookFromy, lookFromz, lookAtx, lookAty, lookAtz, upx, upy, upz);				
  
  glEnable(GL_TEXTURE_2D);

  // Draw Boundary Walls
  glTexImage2D( // initialize texture
	  GL_TEXTURE_2D, // texture is 2-d
	  0, // resolution level 0
	  GL_RGB, // internal format
	  boundaryWallPix[0].nCols, // image width
	  boundaryWallPix[0].nRows, // image height
	  0, // no border
	  GL_RGB, // my format
	  GL_UNSIGNED_BYTE, // my type
	  boundaryWallPix[0].pixel); // the pixels
  for (int i = 0; i < numBoundaryWalls; i++) {
	  drawMesh(boundaryWalls[i]);
  }

  // Draw Inner Walls
  glTexImage2D( GL_TEXTURE_2D, 0, GL_RGB, innerWallPix[0].nCols, innerWallPix[0].nRows, 0, GL_RGB, GL_UNSIGNED_BYTE, innerWallPix[0].pixel); 
  for (int i = 0; i < numInnerWalls; i++) {
	  drawMesh(innerWalls[i]);
  }

  VECTOR3D temp = VECTOR3D(personBody->translation.x + 7.0, personBody->translation.y, personBody->translation.z);
  if (temp.GetQuaddLength() < 9){
	  doors[0]->angles.y = 90.0;
	  doors[0]->translation.x = -10.0;
	  doors[0]->translation.z = -1.5;
	  doors[1]->angles.y = -90.0;
	  doors[1]->translation.x = -4.0;
	  doors[1]->translation.z = -1.5;
  }
  else {
	  doors[0]->angles.y = 0.0;
	  doors[0]->translation.x = -8.5;
	  doors[0]->translation.z = 0.0;
	  doors[1]->angles.y = 0.0;
	  doors[1]->translation.x = -5.5;
	  doors[1]->translation.z = 0.0;
  }
  temp = VECTOR3D(personBody->translation.x, personBody->translation.y, personBody->translation.z+8.0);
  if (temp.GetQuaddLength() < 9){
	  doors[2]->angles.y = 90.0;
	  doors[2]->translation.x = 1.0;
	  doors[2]->translation.z = -10.0;
	  doors[3]->angles.y = -90.0;
	  doors[3]->translation.x = 1.0;
	  doors[3]->translation.z = -6.0;
	  //cout << "Distance is < 4.0" << endl;
  }
  else {
	  doors[2]->angles.y = 0.0;
	  doors[2]->translation.x = 0.0;
	  doors[2]->translation.z = -9.0;
	  doors[3]->angles.y = 0.0;
	  doors[3]->translation.x = 0.0;
	  doors[3]->translation.z = -7.0;
	  //cout << "Distance is greater than 1" << endl;
  }
  // Draw left doors
  glTexImage2D( GL_TEXTURE_2D, 0, GL_RGB, doorPix[0].nCols, doorPix[0].nRows, 0, GL_RGB, GL_UNSIGNED_BYTE, doorPix[0].pixel); 
  drawMesh(doors[0]);
  drawMesh(doors[2]);
  // Draw right doors
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, doorPix[1].nCols, doorPix[1].nRows, 0, GL_RGB, GL_UNSIGNED_BYTE, doorPix[1].pixel);
  drawMesh(doors[1]);
  drawMesh(doors[3]);
  
  // Draw Floor
  glTexImage2D( GL_TEXTURE_2D, 0, GL_RGB, floorPix[0].nCols, floorPix[0].nRows, 0, GL_RGB, GL_UNSIGNED_BYTE, floorPix[0].pixel); 
  terrainGrid->DrawGrid(gridSize);
  
  // Draw Person
  drawMesh(personBody, personHead);

  glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);

  for (int i = 0; i < 12; i++){
	  drawObj(cobj[i]);
  }

  glDisable(GL_TEXTURE_2D);
  glutSwapBuffers();
}

// Called at initialization and whenever user resizes the window */
void reshape(int w, int h) {
  glutWindowWidth = w; glutWindowHeight = h;

  viewportWidth  = glutWindowWidth;
  viewportHeight = glutWindowHeight;

  glViewport(0, 0, (GLsizei) viewportWidth, (GLsizei) viewportHeight);
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  // keep same aspect ratio as screen window
  gluPerspective(60.0*zoomFactor,(float)viewportWidth/(float)viewportHeight,0.2,80.0);
  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();
}

VECTOR3D pos = VECTOR3D(0,0,0);

// Mouse Control coordinates
int prevx, prevy;
int selectedControlPoint = -1;
void mouse(int button, int state, int x, int y){
  currentButton = button;

  switch(button) {
    case GLUT_LEFT_BUTTON:
      if (state == GLUT_DOWN){
        prevx = x;
        prevy = y;
      }
      break;
	case GLUT_RIGHT_BUTTON:
		if (state == GLUT_DOWN)	{
			//zoom in/out
		}
		break;
	case 3:   //zoom in
		if (radius > 2)
		  radius-=0.25;
		//zoomFactor -= 0.02;
		break;
	case 4:   //zoom out
		radius+=0.25;
		//zoomFactor += 0.02;
		break;
	default:
      break;
  }
  glutPostRedisplay();
}


/**************************************************************************
 * Mouse Control
 **************************************************************************/
void mouseMotionHandler(int xMouse, int yMouse){
  if (currentButton == GLUT_LEFT_BUTTON){
    VECTOR3D wpos;
  
    angleTheta += prevx - xMouse;
    prevx = xMouse;

    anglePhi += prevy - yMouse;
    prevy = yMouse;

    if (angleTheta < -180)
      angleTheta += 360;
    if (angleTheta > 180)
      angleTheta -= 360;
    //while (anglePhi < 0)
    //  anglePhi += 360;

    limitCameraAngle();
  }
  //glutPostRedisplay();
	return;
}

/**************************************************************************
 * Timer function to limit Frames Per Second
 **************************************************************************/
void timer(int value){
	glutTimerFunc(1000.0 / FPS, timer, 0);
	glutPostRedisplay();
}


/**************************************************************************
 * Utility Functions
 **************************************************************************/
float degToRad(float degrees){
	return degrees / 180 * M_PI; 
}

float radToDeg(float radians){
	return radians * 180 / M_PI;
}

void updateCameraPos(){
	// Spherical coordinates formula
	lookFromx = lookAtx + radius * sin(anglePhi*0.0174532) * sin(angleTheta*0.0174532); 
	lookFromy = lookAty + radius * cos(anglePhi*0.0174532);
	lookFromz = lookAtz + radius * sin(anglePhi*0.0174532) * cos(angleTheta*0.0174532); 
}


/**************************************************************************
 * Limit Camera angle
 **************************************************************************/
void limitCameraAngle(){
	if (anglePhi > 60)
		anglePhi = 60;
	if (anglePhi < 2)
		anglePhi = 2;
	//if (angleTheta < 10)
		//angleTheta = 10;
}

VECTOR3D ScreenToWorld(int x, int y){
	GLint viewport[4];
	GLdouble modelview[16];
	GLdouble projection[16];
	GLfloat winX, winY, winZ;
	GLdouble posX, posY, posZ;
	
	glGetDoublev( GL_MODELVIEW_MATRIX, modelview );
	glGetDoublev( GL_PROJECTION_MATRIX, projection );
	glGetIntegerv( GL_VIEWPORT, viewport );

	winX = (float)x;
	winY = (float)viewport[3] - (float)y;
	// Read all pixels at given screen XY from the Depth Buffer
	glReadPixels( x, int(winY), 1, 1, GL_DEPTH_COMPONENT, GL_FLOAT, &winZ );
	gluUnProject( winX, winY, winZ, modelview, projection, viewport, &posX, &posY, &posZ);
	return VECTOR3D((float)posX, (float)posY, (float)posZ);
}// ScreenToWorld()

/**************************************************************************
* Action Methods
**************************************************************************/
void deselectAll(void){
	for (int currobj = 0; currobj < 12; currobj++){  //check if there is at least one item selected
		cobj[currobj]->selected = false;
	}
}


/* Handles input from the keyboard, non-arrow keys */
void keyboard(unsigned char key, int x, int y){
  double xtmp, ztmp, xnew, znew; 
  switch (key) { // Navigate
    case 'n':
      if (currentAction != NAVIGATE){        
        currentAction = NAVIGATE;
        camerax = lookFromx; // save world view camera positions
        cameray = lookFromy;
        cameraz = lookFromz;
        ztmp = personBody->scaleFactor.z+0.1;
        xnew = ztmp * sin (degToRad(personBody->angles.y));
        znew = ztmp * cos (degToRad(personBody->angles.y));
        lookFromx = personBody->translation.x;
		lookFromy = personBody->scaleFactor.y + 0.7;
        lookFromz = personBody->translation.z;
        lookAtx = personBody->translation.x - 2 * xnew;
        lookAty = lookFromy;
        lookAtz = personBody->translation.z - 2 * znew;
      }
      else{
        currentAction = TRANSLATE;
        lookFromx = camerax;
        lookFromy = cameray;
        lookFromz = cameraz;
      }
      break;
	case 'm':
		if (currentAction == NAVIGATE && !personBody->turnHead){ // turn mode only activates if at state NAVIGATE
			personBody->turnHead = true;
			ztmp = personBody->scaleFactor.z + 0.1;
			xnew = ztmp * sin(degToRad(personBody->angles.y + personBody->headAngle.y));
			znew = ztmp * cos(degToRad(personBody->angles.y + personBody->headAngle.y));
			lookFromx = personBody->translation.x;
			lookFromy = personBody->scaleFactor.y + 0.7;
			lookFromz = personBody->translation.z;
			lookAtx = personBody->translation.x - 2 * xnew;
			lookAty = lookFromy;
			lookAtz = personBody->translation.z - 2 * znew;
		}
		else if (currentAction == NAVIGATE && personBody->turnHead){
			personBody->turnHead = false;
			personBody->headAngle.x = personBody->headAngle.y = personBody->headAngle.z = 0.0;
			ztmp = personBody->scaleFactor.z + 0.1;
			xnew = ztmp * sin(degToRad(personBody->angles.y));
			znew = ztmp * cos(degToRad(personBody->angles.y));
			lookFromx = personBody->translation.x;
			lookFromy = personBody->scaleFactor.y + 0.7;
			lookFromz = personBody->translation.z;
			lookAtx = personBody->translation.x - 2 * xnew;
			lookAty = lookFromy;
			lookAtz = personBody->translation.z - 2 * znew;
		}
		break;
	case 't':
		currentAction = TRANSLATE;
		break;
	//case 's':
		//currentAction = SCALE;
		//break;
	case 'r':
		currentAction = ROTATE;
		break;
	//case 'e':
		//currentAction = EXTRUDE;
		//break;
	case 'h':
		currentAction = RAISE;
		break;
	case 'c':
		currentAction = SELECT;
		break;
	case '+':
		currentAction = MULTIPLESELECT;
		break;
	case '-':
		currentAction = DESELECT_ALL;
		deselectAll();
		break;
	case 'b':
		objCollision = !objCollision;
		break;
	case 'q':
		exit(0);
  }
  glutPostRedisplay();
}

void functionKeys(int key, int x, int y){
  double xtmp, ztmp, xnew, znew; 
  //int currObj = 0;

  if (currentAction == NAVIGATE && !personBody->turnHead){
	switch (key){
		case GLUT_KEY_UP:
			personBody->translation.x -= 0.2 * sin(degToRad(personBody->angles.y));
			personBody->translation.z -= 0.2 * cos(degToRad(personBody->angles.y));
			if (hasCollision(personBody, boundaryWalls, innerWalls, doors, collision_offset, numBoundaryWalls, numInnerWalls, numDoors) || (objCollision && hasObjCollision(personBody, cobj, 12)) ){
				personBody->translation.x += 0.2 * sin(degToRad(personBody->angles.y));
				personBody->translation.z += 0.2 * cos(degToRad(personBody->angles.y));
			}
			break;
		case GLUT_KEY_DOWN:
		  personBody->translation.x += 0.2 * sin (degToRad(personBody->angles.y));
		  personBody->translation.z += 0.2 * cos (degToRad(personBody->angles.y));
		  if (hasCollision(personBody, boundaryWalls, innerWalls, doors, collision_offset, numBoundaryWalls, numInnerWalls, numDoors) || (objCollision && hasObjCollision(personBody, cobj, 12)) ){
			  personBody->translation.x -= 0.2 * sin(degToRad(personBody->angles.y));
			  personBody->translation.z -= 0.2 * cos(degToRad(personBody->angles.y));
		  }
		  break;
		case GLUT_KEY_LEFT:
		  personBody->angles.y += 2.0;
		  if (hasCollision(personBody, boundaryWalls, innerWalls, doors, collision_offset, numBoundaryWalls, numInnerWalls, numDoors) || (objCollision && hasObjCollision(personBody, cobj, 12)) ){
			  personBody->angles.y -= 2.0;
		  }
		  break;
		case GLUT_KEY_RIGHT:
			personBody->angles.y -= 2.0;
			if (hasCollision(personBody, boundaryWalls, innerWalls, doors, collision_offset, numBoundaryWalls, numInnerWalls, numDoors) || (objCollision && hasObjCollision(personBody, cobj, 12)) ){
				personBody->angles.y += 2.0;
			}
			break;
	}
	ztmp = personBody->scaleFactor.z+0.1;
	xnew = ztmp * sin (degToRad(personBody->angles.y));
	znew = ztmp * cos (degToRad(personBody->angles.y));
	lookFromx = personBody->translation.x;
	lookFromy = personBody->scaleFactor.y + 0.7;
	lookFromz = personBody->translation.z;
	lookAtx = personBody->translation.x - 2*xnew;
	lookAty = lookFromy;
	lookAtz = personBody->translation.z - 2*znew;
  }
  else if (currentAction == NAVIGATE && personBody->turnHead){
	  double ynew, ytmp;
	  double xzAngle;
	  switch (key){
		  case GLUT_KEY_UP: // 't' resets head position to 0.0
			  personBody->headAngle.x -= 2.0;
			  if (personBody->headAngle.x < -45) personBody->headAngle.x = -45;
			  break;
		  case GLUT_KEY_DOWN: // 't' resets head position to 0.0
			  personBody->headAngle.x += 2.0;
			  if (personBody->headAngle.x > 60) personBody->headAngle.x = 60;
			  break;
		  case GLUT_KEY_LEFT:
			  personBody->headAngle.y += 2.0;
			  if (personBody->headAngle.y > 60) personBody->headAngle.y = 60;
			  break;
		  case GLUT_KEY_RIGHT:
			  personBody->headAngle.y -= 2.0;
			  if (personBody->headAngle.y < -60) personBody->headAngle.y = -60;
			  break;
	  }
		ztmp = personBody->scaleFactor.z + 0.8;
		xzAngle = degToRad(personBody->angles.y + personBody->headAngle.y);
		xnew = ztmp * sin(xzAngle);
		znew = ztmp * cos(xzAngle);
		ytmp = 0.64; // = 1.6 * 0.4
		ynew = ytmp * sin(degToRad(personBody->headAngle.x));
		lookFromx = personBody->translation.x + ynew * sin(xzAngle);
		lookFromy = personBody->scaleFactor.y + 0.7;
		lookFromz = personBody->translation.z + ynew * cos(xzAngle);
		lookAtx = personBody->translation.x - 2 * xnew;
		lookAty = lookFromy + ynew;
		lookAtz = personBody->translation.z - 2 * znew;
	}
  else if (currentAction == TRANSLATE){
	  switch (key){
	  case GLUT_KEY_UP:
		  for (int i = 0; i < 12; i++){
			  if (cobj[i]->selected){
				  cobj[i]->translation.z -= trsize;
			  }
		  }
		  break;
	  case GLUT_KEY_DOWN:
		  for (int i = 0; i < 12; i++){
			  if (cobj[i]->selected){
				  cobj[i]->translation.z += trsize;
			  }
		  }
		  break;
	  case GLUT_KEY_LEFT:
		  for (int i = 0; i < 12; i++){
			  if (cobj[i]->selected){
				  cobj[i]->translation.x -= trsize;
			  }
		  }
		  break;
	  case GLUT_KEY_RIGHT:
		  for (int i = 0; i < 12; i++){
			  if (cobj[i]->selected){
				  cobj[i]->translation.x += trsize;
			  }
		  }
		  break;
	  }
  }
  else if (currentAction == ROTATE){
	  switch (key){
	  case GLUT_KEY_UP:
		  break;
	  case GLUT_KEY_DOWN:
		  break;
	  case GLUT_KEY_LEFT:
		  for (int i = 0; i < 12; i++){
			  if (cobj[i]->selected){
				  cobj[i]->angles.y += rosize;
				  if (cobj[i]->angles.y > 180.0) cobj[i]->angles.y -= 360.0;
				  //else if (cobj[i]->angles.y < -180.0) cobj[i]->angles.y += 360.0;
			  }
		  }
		  break;
	  case GLUT_KEY_RIGHT:
		  for (int i = 0; i < 12; i++){
			  if (cobj[i]->selected){
				  cobj[i]->angles.y -= rosize;
				  //if (cobj[i]->angles.y > 180.0) cobj[i]->angles.y -= 360.0;
				  if (cobj[i]->angles.y < -180.0) cobj[i]->angles.y += 360.0;
			  }
		  }
		  break;
	  }
  }
  else if (currentAction == RAISE){
	  switch (key){
	  case GLUT_KEY_UP:
		  for (int i = 0; i < 12; i++){
			  if (cobj[i]->selected){
				  cobj[i]->translation.y += trsize;
			  }
		  }
		  break;
	  case GLUT_KEY_DOWN:
		  for (int i = 0; i < 12; i++){
			  if (cobj[i]->selected){
				  cobj[i]->translation.y -= trsize;
			  }
		  }
		  break;
	  case GLUT_KEY_LEFT:
		  break;
	  case GLUT_KEY_RIGHT:
		  break;
	  }
  }
  else if (currentAction == SELECT){
	  int currObj = 0;
	  switch (key){
	  case GLUT_KEY_UP: 
		  break;
	  case GLUT_KEY_DOWN:
		  break;
	  case GLUT_KEY_LEFT:
		  for (currObj = 11; currObj >= 0; currObj--){  //check if there is at least one item selected
			  if (cobj[currObj]->selected)
				  break;
		  }
		  if (currObj > 0){              //if there are more objects
			  cobj[currObj]->selected = false;     //deselect the current one
			  cobj[currObj - 1]->selected = true;  //select the next one
			  for (int j = currObj - 2; j >= 0; j--)  //deselect the rest
				  cobj[j]->selected = false;
		  }
		  else if (currObj == 0){        //the last cube is selected
			  cobj[currObj]->selected = false;     //deselect the current one
			  cobj[11]->selected = true;      //wrap around and select the first cube
		  }
		  else if (currObj == -1){            //if none are selected
			  cobj[0]->selected = true;
		  }
		  break;
	  case GLUT_KEY_RIGHT:
		  for (currObj = 0; currObj <= 11; currObj++){  //check if there is at least one item selected
			  if (cobj[currObj]->selected)
				  break;
		  }
		  if (currObj < 11){              //if there are more objects
			  cobj[currObj]->selected = false;     //deselect the current one
			  cobj[currObj + 1]->selected = true;  //select the next one
			  for (int j = currObj + 2; j < 12; j++)  //deselect the rest
				  cobj[j]->selected = false;
		  }
		  else if (currObj == 11){        //the last cube is selected
			  cobj[currObj]->selected = false;     //deselect the current one
			  cobj[0]->selected = true;      //wrap around and select the first cube
		  }
		  else if (currObj == 12){            //if none are selected
			  cobj[0]->selected = true;
		  }
		  break;
	  }
  }
  else if (currentAction == MULTIPLESELECT){
	  int currObj = 0, firstObj;
	  switch (key){
	  case GLUT_KEY_UP: 
		  break;
	  case GLUT_KEY_DOWN:
		  break;
	  case GLUT_KEY_LEFT:
		  for (currObj = 0; currObj < 12; currObj++){  //check if there is at least one item selected
			  if (cobj[currObj]->selected)
				  break;
		  }
		  // go backward and select the next unselected object
		  firstObj = currObj;
		  do{
			  if (currObj == 0)
				  currObj = 11;
			  else
				  currObj--;
		  } while (cobj[currObj]->selected && currObj != firstObj);
		  if (currObj != firstObj)
			  cobj[currObj]->selected = true;
		  break;
	  case GLUT_KEY_RIGHT:
		  for (currObj = 11; currObj >= 0; currObj--){  //check if there is at least one item selected
			  if (cobj[currObj]->selected)
				  break;
		  }
		  // go forward and select the next unselected object
		  firstObj = currObj;
		  do{
			  if (currObj == 11)
				  currObj = 0;
			  else
				  currObj++;
		  } while (cobj[currObj]->selected && currObj != firstObj);
		  if (currObj != firstObj)
			  cobj[currObj]->selected = true;
		  break;
	  }
  }

}

/**************************************************************************
* Helper functions
**************************************************************************/
bool hasCollision(const Mesh* a, Mesh* b[], Mesh* c[], Mesh* d[], float offset, int sizeb, int sizec, int sized){
	for (int i = 0; i < sizeb; i++){
		//check the X axis
		if (labs(a->translation.x - b[i]->translation.x) + offset  <  a->scaleFactor.x + b[i]->scaleFactor.x){
			//check the Y axis
			//if (labs(a->translation.y - b[i]->translation.y) + offset  <  a->scaleFactor.y + b[i]->scaleFactor.y){
				//check the Z axis
				if (labs(a->translation.z - b[i]->translation.z) + offset  <  a->scaleFactor.z + b[i]->scaleFactor.z){
					return true;
				}
			//}
		}
	}
	for (int i = 0; i < sizec; i++){
		//check the X axis
		if (labs(a->translation.x - c[i]->translation.x) + offset  <  a->scaleFactor.x + c[i]->scaleFactor.x){
			//check the Y axis
			//if (labs(a->translation.y - c[i]->translation.y) + offset  <  a->scaleFactor.y + c[i]->scaleFactor.y){
			//check the Z axis
			if (labs(a->translation.z - c[i]->translation.z) + offset  <  a->scaleFactor.z + c[i]->scaleFactor.z){
				return true;
			}
			//}
		}
	}
	for (int i = 0; i < sized; i++){
		//check the X axis
		if (labs(a->translation.x - d[i]->translation.x) + offset  <  a->scaleFactor.x + d[i]->scaleFactor.x){
			//check the Y axis
			//if (labs(a->translation.y - c[i]->translation.y) + offset  <  a->scaleFactor.y + c[i]->scaleFactor.y){
			//check the Z axis
			if (labs(a->translation.z - d[i]->translation.z) + offset  <  a->scaleFactor.z + d[i]->scaleFactor.z){
				return true;
			}
			//}
		}
	}

	return false;
}

// Checks for collision between avatar and room objects
bool hasObjCollision(const Mesh* a, ComplexObj *cobj[], int sizee){
	VECTOR3D personMin, personMax;
	float rad = degToRad(a->angles.y);
	float xabs = fabs(0.25 * cos(rad) + 0.1 * sin(rad));
	float zabs = fabs(0.25 * sin(rad) + 0.1 * cos(rad));
	personMin.x = a->translation.x - xabs;
	personMin.y = 0.0;
	personMin.z = a->translation.z - zabs;
	personMax.x = a->translation.x + xabs;
	personMax.y = 2.17;
	personMax.z = a->translation.z + zabs;

	for (int i = 0; i < sizee; i++){
		float radobj = degToRad(cobj[i]->angles.y);
		float xabsobj = fabs(cobj[i]->dim.x / 2 * cos(radobj) + cobj[i]->dim.z / 2 * sin(radobj));
		float yabsobj = cobj[i]->dim.y;
		float zabsobj = fabs(cobj[i]->dim.x / 2 * sin(radobj) + cobj[i]->dim.z / 2 * cos(radobj));
		VECTOR3D objMin, objMax;
		objMin.x = cobj[i]->translation.x - xabsobj;
		objMin.y = cobj[i]->translation.y;
		objMin.z = cobj[i]->translation.z - zabsobj;
		objMax.x = cobj[i]->translation.x + xabsobj;
		objMax.y = cobj[i]->translation.y + yabsobj;
		objMax.z = cobj[i]->translation.z + zabsobj;

		if ( (personMin.x < objMax.x && objMin.x < personMax.x) && 
			 (personMin.z < objMax.z && objMin.z < personMax.z) && 
			 (personMin.y < objMax.y && objMin.z < personMax.z) ) 
			return true;

		/*
		if (fabs(a->translation.x - cobj[i]->translation.x) < a->scaleFactor.x + cobj[i]->dim.x / 2){ //check the X-axis
			if (fabs(a->translation.y - cobj[i]->translation.y) < a->scaleFactor.y + cobj[i]->dim.y / 2){ //check the X-axis
				if (fabs(a->translation.z - cobj[i]->translation.z) < a->scaleFactor.z + cobj[i]->dim.z / 2){ //check the X-axis
					return true;
				}
			}
		}
		*/
	}
	return false;
}

void createBoundaryWalls(){
	// create in counterclockwise direction/sequence
	scale.x = 0.5f;   scale.y = 4.0f; scale.z = 16.0f;
	trans.x = -16.0f; trans.y = 0.0f; trans.z = 0.0f;
	boundaryWalls[0] = createMesh(scale, trans, wallHeight, 1); // left wall

	scale.x = 16.0f;  scale.y = 4.0f; scale.z = 0.5f;
	trans.x = 0.0f;   trans.y = 0.0f; trans.z = 16.0f;
	boundaryWalls[1] = createMesh(scale, trans, wallHeight, 1); // front wall

	scale.x = 0.5f;   scale.y = 4.0f; scale.z = 16.0f;
	trans.x = 16.0f;  trans.y = 0.0f; trans.z = 0.0f;
	boundaryWalls[2] = createMesh(scale, trans, wallHeight, 1); // right wall

	scale.x = 16.0f;  scale.y = 4.0f; scale.z = 0.5f;
	trans.x = 0.0f;   trans.y = 0.0f; trans.z = -16.0f;
	boundaryWalls[3] = createMesh(scale, trans, wallHeight, 1); // back wall

	numBoundaryWalls = 4;
}

void createInnerWalls(){
	// create large room walls
	scale.x = 5.0f;   scale.y = 4.0f; scale.z = 0.5f;
	trans.x = 11.0f;    trans.y = 0.0f; trans.z = 0.0f;
	innerWalls[0] = createMesh(scale, trans, wallHeight, 1); // left wall

	scale.x = 5.0f;   scale.y = 4.0f; scale.z = 0.5f;
	trans.x = 1.0f;    trans.y = 0.0f; trans.z = 0.0f;
	innerWalls[1] = createMesh(scale, trans, wallHeight, 1); // left wall

	scale.x = 3.0f;   scale.y = 4.0f; scale.z = 0.5f;
	trans.x = -13.0f; trans.y = 0.0f;  trans.z = 0.0f;
	innerWalls[2] = createMesh(scale, trans, wallHeight, 1); // front wall

	// create small room walls
	scale.x = 0.5f;   scale.y = 4.0f; scale.z = 3.0f;
	trans.x = 0.0f;  trans.y = 0.0f; trans.z = -13.0f;
	innerWalls[3] = createMesh(scale, trans, wallHeight, 1); // right wall

	scale.x = 0.5f;   scale.y = 4.0f; scale.z = 3.0f;
	trans.x = 0.0f;  trans.y = 0.0f; trans.z = -3.0f;
	innerWalls[4] = createMesh(scale, trans, wallHeight, 1); // right wall

	numInnerWalls = 5;
}

void createDoors(){
	// create large room door
	scale.x = 1.5f;   scale.y = 4.0f; scale.z = 0.5f;
	trans.x = -8.5f;    trans.y = 0.0f; trans.z = 0.0f;
	doors[0] = createMesh(scale, trans, wallHeight, 1); // left door

	scale.x = 1.5f;   scale.y = 4.0f; scale.z = 0.5f;
	trans.x = -5.5f;    trans.y = 0.0f; trans.z = 0.0f;
	doors[1] = createMesh(scale, trans, wallHeight, 1); // right wall

	// create small room door
	scale.x = 0.5f;   scale.y = 4.0f; scale.z = 1.0f;
	trans.x = 0.0f; trans.y = 0.0f;  trans.z = -9.0f;
	doors[2] = createMesh(scale, trans, wallHeight, 1); // left door

	scale.x = 0.5f;   scale.y = 4.0f; scale.z = 1.0f;
	trans.x = 0.0f; trans.y = 0.0f;  trans.z = -7.0f;
	doors[3] = createMesh(scale, trans, wallHeight, 1); // right door

	numDoors = 4;
}