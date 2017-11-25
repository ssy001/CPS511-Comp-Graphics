/*******************************************************************
	       Scene Modelling Program
********************************************************************/

#include <windows.h>
#include <gl/gl.h>
#include <gl/glu.h>
#include "Dependencies\freeglut\freeglut.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <utility>
#include <vector>
#include "VECTOR3D.h"
#include "QuadMesh.h"
#include "ComplexObj.h"

#define MAXOBJECTS 10
#define WINDOWSIZE 1000
#define WINDOWPOSX 100
#define WINDOWPOSY 100
#define FPSINTERVAL 1000  //frames per sec for animation timer function (in milliseconds)
#define TIMERFRAMES 10  //number of timer function frames/calls to render - for smooth animation

void initOpenGL(int w, int h);
void display(void);
void reshape(int w, int h);
void mouse(int button, int state, int x, int y);
void mouseMotionHandler(int xMouse, int yMouse);
void keyboard(unsigned char key, int x, int y);
void functionKeys(int key, int x, int y);
VECTOR3D ScreenToWorld(int x, int y);

static int currentButton;
static unsigned char currentKey;
VECTOR3D lookfrom = VECTOR3D( 0.0, 10.0, 20.0 );   //initial camera/eye coordinates
VECTOR3D lookat   = VECTOR3D( 0.0, 0.0, 0.0);
int oldx = 0, oldy = 0;
bool navmode = false;

GLfloat light_position0[] = {-6.0,  12.0, 0.0,1.0};
GLfloat light_position1[] = { 6.0,  12.0, 0.0,1.0};
GLfloat light_diffuse[]   = {1.0, 1.0, 1.0, 1.0};
GLfloat light_specular[]  = {1.0, 1.0, 1.0, 1.0};
GLfloat light_ambient[]   = {0.2, 0.2, 0.2, 1.0};


// Cube Mesh Array variables and initialization
// ...
// also add a variable to keep track of current cube mesh
ComplexObj *objects[MAXOBJECTS];      //Array of ComplexObj pointers
int nextobj = 0;               //index to objects array to store next cube
ComplexObj *navobj;  //selected object in NAVIGATE mode

float trsize = 0.2, scsize = 0.2, rosize = 5.0;      //translation, scaling and rotation angle deltas
int fps = 20;  //frames per seconds for timer animation
int timerdelay = FPSINTERVAL / (fps * TIMERFRAMES);  //delay for timer function in milliseconds
int funckey;  //stores functionKeys()'s key value for use in translate, rotate animation timer functions

// Interaction State Variable
enum Action {TRANSLATE, ROTATE, SCALE, EXTRUDE, RAISE, SELECT, MULTIPLESELECT, DESELECT_ALL, NAVIGATE};
enum Action currentAction = TRANSLATE;

QuadMesh *floorMesh = NULL;
// Wall Mesh variables here
// ...
QuadMesh *leftMesh = NULL;
QuadMesh *rightMesh = NULL;
QuadMesh *backMesh = NULL;
QuadMesh *topMesh = NULL;

struct BoundingBox{
  VECTOR3D min;
  VECTOR3D max;
} BBox;

// Default Mesh Size
int meshSize = 16;

int main(int argc, char **argv)
{
  glutInit(&argc,argv);
  glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
  glutInitWindowSize(WINDOWSIZE, WINDOWSIZE);
  glutInitWindowPosition(WINDOWPOSX, WINDOWPOSY);
  int pID = glutCreateWindow("Scene Modeller");
  //int cID = glutCreateSubWindow(pID, 0, 0, 250, 50);
  glutSetWindow(pID);

  initOpenGL(WINDOWSIZE, WINDOWSIZE);

  glutDisplayFunc(display);
  glutReshapeFunc(reshape);
  glutMouseFunc(mouse);
  glutMotionFunc(mouseMotionHandler);
  glutKeyboardFunc(keyboard);
  glutSpecialFunc(functionKeys);
  glutMainLoop();
  return 0;
}


// Setup openGL */
void initOpenGL(int w, int h)
{
  // Set up viewport, projection, then change to modelview matrix mode - 
  // display function will then set up camera and modeling transforms
  glViewport(0, 0, (GLsizei) w, (GLsizei) h);
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  gluPerspective(60.0,1.0,0.2,80.0);
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
  
  // Other OpenGL setup
  glEnable(GL_DEPTH_TEST);
  glShadeModel(GL_SMOOTH);
  glClearColor(0.6, 0.6, 0.6, 0.0);  
  glClearDepth(1.0f);
  glEnable(GL_DEPTH_TEST);
  // This one is important - renormalize normal vectors 
  glEnable(GL_NORMALIZE);
  
  //Nice perspective.
  glHint(GL_PERSPECTIVE_CORRECTION_HINT , GL_NICEST);
  
  // Set up meshes
  VECTOR3D origin  = VECTOR3D(-8.0f,0.0f,8.0f);
  VECTOR3D dir1v   = VECTOR3D(1.0f, 0.0f, 0.0f);
  VECTOR3D dir2v   = VECTOR3D(0.0f, 0.0f,-1.0f);
  floorMesh = new QuadMesh(meshSize, 16.0);
  floorMesh->InitMesh(meshSize, origin, 16.0, 16.0, dir1v, dir2v);
  
  VECTOR3D ambient = VECTOR3D(0.0f,0.0f,0.0f);
  VECTOR3D specular= VECTOR3D(0.0f,0.0f,0.0f);
  VECTOR3D diffuse= VECTOR3D(0.9f,0.5f,0.0f);
  float shininess = 0.0;
  floorMesh->SetMaterial(ambient,diffuse,specular,shininess);

  // Set up wall meshes
  // Make sure direction vectors are such that the normals are pointing into the room
  // Use the right-hand-rule (cross product) 
  // If you are confused about this, ask in class
  origin = VECTOR3D(-8.0f, 6.0f, 8.0f);
  dir1v = VECTOR3D(0.0f, -1.0f, 0.0f);
  dir2v = VECTOR3D(0.0f, 0.0f, -1.0f);
  leftMesh = new QuadMesh(meshSize, 16.0);
  leftMesh->InitMesh(meshSize, origin, 6.0, 16.0, dir1v, dir2v);

  ambient = VECTOR3D(0.2f, 0.2f, 0.2f);
  specular = VECTOR3D(0.0f, 0.0f, 0.0f);
  diffuse = VECTOR3D(0.8f, 0.8f, 0.8f);
  shininess = 0.5;
  leftMesh->SetMaterial(ambient, diffuse, specular, shininess);

  origin = VECTOR3D(8.0f, 0.0f, 8.0f);
  dir1v = VECTOR3D(0.0f, 1.0f, 0.0f);
  dir2v = VECTOR3D(0.0f, 0.0f, -1.0f);
  rightMesh = new QuadMesh(meshSize, 16.0);
  rightMesh->InitMesh(meshSize, origin, 6.0, 16.0, dir1v, dir2v);

  rightMesh->SetMaterial(ambient, diffuse, specular, shininess);

  origin = VECTOR3D(-8.0f, 6.0f, -8.0f);
  dir1v = VECTOR3D(0.0f, -1.0f, 0.0f);
  dir2v = VECTOR3D(1.0f, 0.0f, 0.0f);
  backMesh = new QuadMesh(meshSize, 16.0);
  backMesh->InitMesh(meshSize, origin, 6.0, 16.0, dir1v, dir2v);

  backMesh->SetMaterial(ambient, diffuse, specular, shininess);

  // Set up the bounding box of the room
  // Change this if you change your floor/wall dimensions
  BBox.min.Set(-8.0f, 0.0, -8.0);
  BBox.max.Set( 8.0f, 6.0,  8.0);

}



void display(void)
{
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  
  glLoadIdentity();
  
  // Set up the camera
  gluLookAt(lookfrom.x, lookfrom.y, lookfrom.z, lookat.x, lookat.y, lookat.z, 0.0, 1.0, 0.0);     //this sets up the eye/camera/view transformation (eye, center, up)
  //CTM is V matrix = RT

  // Draw all objects
  for (int i = 0; i < MAXOBJECTS; i++){
	  if (objects[i]){
		  drawObj(objects[i]);
	  }
  }
  
  // Draw floor and wall meshes
  floorMesh->DrawMesh(meshSize);
  leftMesh->DrawMesh(meshSize);
  rightMesh->DrawMesh(meshSize);
  backMesh->DrawMesh(meshSize);

  glutSwapBuffers();
}


// Called at initialization and whenever user resizes the window */
void reshape(int w, int h)
{
  glViewport(0, 0, (GLsizei) w, (GLsizei) h);      //maps normalized device coordinates into window coordinates
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  gluPerspective(60.0,1.0,0.2,40.0);               //maps eye coordinates into clip coordinates and then into normalized device coordinates
  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();

}

void changeViewAngle(int winx, int winy){
	int deltax, deltay;
	double radiussq, radius, theta, psi; //theta = zenith angle, psi = azimuth angle
	//NOTE: theta range: {-90.0, 90.0}, psi range: {-180.0, 180.0}

	deltax = winx - oldx;
	deltay = winy - oldy;
	radiussq = pow(lookfrom.x, 2.0) + pow(lookfrom.y, 2.0) + pow(lookfrom.z, 2.0);
	radius = sqrt(radiussq);
	theta = acos(lookfrom.y / radius) * 180 / PI;     //in degrees
	psi = atan2(lookfrom.x, lookfrom.z) * 180 / PI;   //in degrees

	//update angles
	theta += (double)deltay;
	if (theta < 5.0) theta = 5.0;
	else if (theta > 90.0) theta = 90.0;

	psi -= (double)deltax;
	if (psi > 180.0) psi -= 360.0;
	else if (psi < -180.0) psi += 360.0;

	//calculate new x,y,z
	lookfrom.y = radius * cos(theta * PI / 180);
	lookfrom.x = radius * sin(theta * PI / 180) * sin(psi * PI / 180);
	lookfrom.z = radius * sin(theta * PI / 180) * cos(psi * PI / 180);
}

void changeZoom(char dir){
	double radiussq, radius, theta, psi;
	double incr = 0.25;

	if (dir == '-') incr = -0.25;

	radiussq = pow(lookfrom.x, 2.0) + pow(lookfrom.y, 2.0) + pow(lookfrom.z, 2.0);
	radius = sqrt(radiussq);
	theta = acos(lookfrom.y / radius) * 180 / PI;     //in degrees
	psi = atan2(lookfrom.x, lookfrom.z) * 180 / PI;   //in degrees

	radius += incr;
	//calculate new x,y,z
	lookfrom.y = radius * cos(theta * PI / 180);
	lookfrom.x = radius * sin(theta * PI / 180) * sin(psi * PI / 180);
	lookfrom.z = radius * sin(theta * PI / 180) * cos(psi * PI / 180);
}

//VECTOR3D pos = VECTOR3D(0,0,0);

// Mouse button callback - use only if you want to 
void mouse(int button, int state, int x, int y)
{
	currentButton = button;

	switch(button) {
		case GLUT_LEFT_BUTTON:
			if (state == GLUT_DOWN)	{
				oldx = x;
				oldy = y;	  
			}
		break;
		case GLUT_RIGHT_BUTTON:
			if (state == GLUT_DOWN)	{
				//zoom in/out
			}
		break;
		case 3:   //zoom in
			changeZoom('-');
			break;
		case 4:   //zoom out
			changeZoom('+');
			break;
		default:
		break;
	}
	glutPostRedisplay();
}

// Mouse motion callback - use only if you want to 
void mouseMotionHandler(int xMouse, int yMouse)
{
  if (currentButton == GLUT_LEFT_BUTTON) {
	  changeViewAngle(xMouse, yMouse);     //scroll latitude/longitude
	  oldx = xMouse;
	  oldy = yMouse;
  }
  glutPostRedisplay();
}


VECTOR3D ScreenToWorld(int x, int y)
{
	// you will need this if you use the mouse
	return VECTOR3D(0);
}// ScreenToWorld()

void deselectAll(void){
	for (int currobj = 0; currobj < nextobj; currobj++){  //check if there is at least one item selected
		objects[currobj]->selected = false;
	}
}

/* Handles input from the keyboard, non-arrow keys */
void keyboard(unsigned char key, int x, int y)
{
	switch (key) 
	{
	case 't':
		currentAction = TRANSLATE;
		break;
	case 's':
		currentAction = SCALE;
		break;
	case 'r':
		currentAction = ROTATE;
		break;
	case 'e':
		currentAction = EXTRUDE;
		break;
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
	case 'n':      
		break;
	case 'N':
		break;
	case 'q':
		exit(0);
	}
	glutPostRedisplay();
}

bool hasCollideWithWalls(struct BoundingBox *objbbox)
{
	if (objbbox->min.x < BBox.min.x - 0.00001f) return true;
	if (objbbox->max.x > BBox.max.x + 0.00001f) return true;
	if (objbbox->min.y < BBox.min.y - 0.00001f) return true;
	if (objbbox->max.y > BBox.max.y + 0.00001f) return true;
	if (objbbox->min.z < BBox.min.z - 0.00001f) return true;
	if (objbbox->max.z > BBox.max.z + 0.00001f) return true;
	return false;
}

//update cobj->center.y for extrude
void adjustYCoordForExtrudeUp(ComplexObj *cobj){
	float yinc = scsize;

	if ((cobj->center.y + cobj->dim.y * cobj->sfy + scsize ) > BBox.max.y + 0.00001)
		cobj->center.y -= yinc;  //if top of object is near ceiling, need to translate obj down to extrude
	else if ((cobj->center.y > yinc / 2.0 - 0.00001) && (cobj->center.y + cobj->dim.y * cobj->sfy + yinc / 2.0) < BBox.max.y + 0.00001)
		cobj->center.y -= yinc / 2.0;  //if bottom of object has room to accomodate half of extrude expansion, then translate obj down by half of expansion increment
}

bool checkWallsForCollision(float tx, float ty, float tz, float sfx, float sfy, float sfz, float angle){
	bool colFlag;
	ComplexObj tempObj;
	struct BoundingBox objBBox;

	for (int i = 0; i < MAXOBJECTS; i++){            //test to see if translate will collide with walls
		if (objects[i] && objects[i]->selected){
			tempObj.center  = VECTOR3D(objects[i]->center);
			tempObj.dim = VECTOR3D(objects[i]->dim);
			tempObj.forward = VECTOR3D(objects[i]->forward);
			tempObj.tx = tx;
			tempObj.ty = ty;
			tempObj.tz = tz;
			tempObj.sfx = objects[i]->sfx + sfx / tempObj.dim.x;
			tempObj.sfy = objects[i]->sfy;
			if(sfy > 0.01)
				adjustYCoordForExtrudeUp(&tempObj);
			tempObj.sfy += sfy / tempObj.dim.y;
			tempObj.sfz = objects[i]->sfz + sfz / tempObj.dim.z;
			tempObj.angle = objects[i]->angle + angle;
			if (tempObj.angle > 180.0) tempObj.angle -= 360.0;
			else if (tempObj.angle < -180.0) tempObj.angle += 360.0;

			getBBox(&tempObj, &(objBBox.min), &(objBBox.max));
			colFlag = hasCollideWithWalls(&objBBox);
			if (colFlag)
				return true;
		}
	}
	return false;
}

bool checkWallsForCollisionNav(float tx, float ty, float tz, float sfx, float sfy, float sfz, float angle){
	bool colFlag;
	ComplexObj tempObj;
	struct BoundingBox objBBox;

	tempObj.center = VECTOR3D(navobj->center);
	tempObj.dim = VECTOR3D(navobj->dim);
	tempObj.forward = VECTOR3D(navobj->forward);
	tempObj.tx = tx;
	tempObj.ty = ty;
	tempObj.tz = tz;
	tempObj.sfx = navobj->sfx + sfx / tempObj.dim.x;
	tempObj.sfy = navobj->sfy;
	if (sfy > 0.01)
		adjustYCoordForExtrudeUp(&tempObj);
	tempObj.sfy += sfy / tempObj.dim.y;
	tempObj.sfz = navobj->sfz + sfz / tempObj.dim.z;
	tempObj.angle = navobj->angle + angle;
	if (tempObj.angle > 180.0) tempObj.angle -= 360.0;
	else if (tempObj.angle < -180.0) tempObj.angle += 360.0;

	getBBox(&tempObj, &(objBBox.min), &(objBBox.max));
	colFlag = hasCollideWithWalls(&objBBox);
	if (colFlag)
		return true;
	return false;
}

//finds the first selected object for NAVIGATE mode, deselect the rest
ComplexObj * findSelectedNavObj(void){
	int j;
	for (int i = 0; i < MAXOBJECTS; i++){
		if (objects[i] && objects[i]->selected){
			j = i+1;
			while (objects[j] && j < MAXOBJECTS){  //deselect the rest of the objects
				objects[j]->selected = false;
				j++;
			}
			return objects[i];
		}
	}
	return (ComplexObj *)NULL;
}

//this function calculates LookAt vector as -15 degrees below the height of the object (NOTE: FOV is 90 degrees wide)
VECTOR3D calculateLookat(VECTOR3D fwd){
	VECTOR3D res;
	float anglerad = 15.0 * PI / 180.0;  //look down y-angle is -15 degrees
	res.x = fwd.x * cos(anglerad);
	res.z = fwd.z * cos(anglerad);
	res.y = -sin(anglerad);
	return res;
}

//timer function to create smooth rotation animation
static void rotatetimer(int t_val){

	if (t_val){
		float roinc = rosize / TIMERFRAMES;

		if (funckey == GLUT_KEY_RIGHT){
			roinc *= -1;  //reverse rotation direction if RIGHT key pressed
		}
		navobj->angle += roinc;
		if (navobj->angle > 180.0) navobj->angle -= 360.0;
		else if (navobj->angle < -180.0) navobj->angle += 360.0;

		navobj->forward.x = sin(navobj->angle * PI / 180.0);
		navobj->forward.z = cos(navobj->angle * PI / 180.0);
		lookfrom.Set(navobj->center.x, (navobj->center.y + navobj->dim.y * navobj->sfy) + 1.0, navobj->center.z);      //switch to object's first-person view
		//calculate lookat vector to look at centre of horizon (y = 3.0) based on object's height
		lookat = calculateLookat(navobj->forward);
		lookat += navobj->center + VECTOR3D(0.0, navobj->dim.y * navobj->sfy + 1.0, 0.0);

		glutPostRedisplay();
		glutTimerFunc(timerdelay, rotatetimer, t_val - 1);
	}
}

//timer function to create smooth translation animation
static void translatetimer(int t_val){

	// NOTE: trsize is 0.2, will call glutdisplay in increments of 0.02 (i.e. 10x).
	// animation will end in 0.1 secs (100 ms). Thus interval is 100/5 = 20 ms, framerate is 1000/20 = 50 fps
	if (t_val){
		float xinc = trsize * navobj->forward.x / TIMERFRAMES;
		float zinc = trsize * navobj->forward.z / TIMERFRAMES;

		if (funckey == GLUT_KEY_DOWN){
			xinc *= -1;  //reverse translate direction if DOWN key pressed
			zinc *= -1;
		}
		//translate center of object by trsize/10 unit of forward vector dir
		navobj->tx = xinc;
		navobj->tz = zinc;
		lookfrom.Set(navobj->center.x + xinc, (navobj->center.y + navobj->dim.y * navobj->sfy) + 1.0, navobj->center.z + zinc);      //switch to object's first-person view
		//calculate lookat vector to look at centre of horizon (y = 3.0) based on object's height
		lookat = calculateLookat(navobj->forward);
		lookat += navobj->center + VECTOR3D(xinc, navobj->dim.y * navobj->sfy + 1.0, zinc);

		glutPostRedisplay();
		glutTimerFunc(timerdelay, translatetimer, t_val - 1);
	}
}


void functionKeys(int key, int x, int y)
{
  VECTOR3D min, max;
  int currObj = 0;
  bool collisionFlag = false;

  if (key == GLUT_KEY_F1){		// Create and init object 1 - clown head (cone (hat) + sphere (head) + disk (collar) + cylinder (nose)
	  if (nextobj < MAXOBJECTS){
		  ComplexObj *newObj = createObj();
		  createMageHead(newObj);
		  newObj->selected = true;          //select the new cube
		  objects[nextobj] = newObj;
		  for (int i = 0; i < nextobj; i++)    //deselect all others
			  objects[i]->selected = false;
		  nextobj++;
	  }
	  else{      //MAXOBJECTS created - cannot make more objects
	  }
	  currentAction = TRANSLATE;
  }
  else if (key == GLUT_KEY_F2){     //Create and init object 2 - tri-rocket (cone (top) + cylinder (body) + inverted half cone + half cone 
	  if (nextobj < MAXOBJECTS){
		  ComplexObj *newObj = createObj();
		  createTriRocket(newObj);
		  newObj->selected = true;          //select the new cube
		  objects[nextobj] = newObj;
		  for (int i = 0; i < nextobj; i++)    //deselect all others
			  objects[i]->selected = false;
		  nextobj++;
	  }
	  else{      //MAXOBJECTS created - cannot make more objects
	  }
	  currentAction = TRANSLATE;
  }
  else if (key == GLUT_KEY_F3){     //Create and init object 3 - bracelet (cylinder + disk x2 + icosahedron, dodecahedron, tetrahedron, octahedron, 
	  if (nextobj < MAXOBJECTS){
		  ComplexObj *newObj = createObj();
		  createBracelet(newObj);
		  newObj->selected = true;          //select the new cube
		  objects[nextobj] = newObj;
		  for (int i = 0; i < nextobj; i++)    //deselect all others
			  objects[i]->selected = false;
		  nextobj++;
	  }
	  else{      //MAXOBJECTS created - cannot make more objects
	  }
	  currentAction = TRANSLATE;
  }
  //Create and init object 4 - android! (cylinder + 4x small cylinder + hemisphere + 2x micro cylinder + 2x micro sphere)
  //Create and init object 5 - condo (cube + randomized small cubes along the face of the main cube)
  else if (key == GLUT_KEY_F4){     

  }

  else if (key == GLUT_KEY_F5){  //enter first-person navigation mode
		currentAction = NAVIGATE;
		navobj = findSelectedNavObj();                    //find the first selected object, deselect the rest

		if (navobj){
			lookfrom.Set(navobj->center.x, (navobj->center.y + navobj->dim.y * navobj->sfy) + 1.0, navobj->center.z);      //switch to object's first-person view
			//calculate lookat vector to look at centre of horizon (y = 3.0) based on object's height
			lookat = calculateLookat(navobj->forward);
			lookat += navobj->center + VECTOR3D(0.0, navobj->dim.y * navobj->sfy + 1.0, 0.0);
		}
		else{
			currentAction = TRANSLATE;
			lookfrom.Set(0.0, 10.0, 20.0);
			lookat.Set(0.0, 0.0, 0.0);
		}
		glutPostRedisplay();
  }
  else if (key == GLUT_KEY_F6){  //enter world mode
		currentAction = TRANSLATE;
		lookfrom.Set(0.0, 10.0, 20.0);      //switch back to default camera view
		lookat.Set(0.0, 0.0, 0.0);
		navobj = findSelectedNavObj();                    //find the first selected object, deselect the rest
		if (navobj){
			navobj->selected = true;
		}
  }
  // Do transformation code with arrow keys
  // GLUT_KEY_DOWN, GLUT_KEY_UP,GLUT_KEY_RIGHT, GLUT_KEY_LEFT
  else if (key == GLUT_KEY_UP){
	  switch (currentAction){
		case TRANSLATE:
			collisionFlag = checkWallsForCollision(0.0, 0.0, -trsize, 0.0, 0.0, 0.0, 0.0);
			if (!collisionFlag){
				for (int i = 0; i < MAXOBJECTS; i++){
					if (objects[i] && objects[i]->selected){
						objects[i]->tz = -trsize;
					}
				}
			}
			break;
		case EXTRUDE:
			collisionFlag = checkWallsForCollision(0.0, 0.0, 0.0, 0.0, scsize, 0.0, 0.0);
			if (!collisionFlag){
				for (int i = 0; i < MAXOBJECTS; i++){
					if (objects[i] && objects[i]->selected){
						adjustYCoordForExtrudeUp(objects[i]);
						objects[i]->sfy += scsize / objects[i]->dim.y;
					}
				}
			}
			break;
		case SCALE:
			collisionFlag = checkWallsForCollision(0.0, 0.0, 0.0, 0.0, 0.0, scsize, 0.0);
			if (!collisionFlag){
				for (int i = 0; i < MAXOBJECTS; i++){
					if (objects[i] && objects[i]->selected){
						objects[i]->sfz += scsize / objects[i]->dim.z;
					}
				}
			}
			break;
		case RAISE:
			collisionFlag = checkWallsForCollision(0.0, trsize, 0.0, 0.0, 0.0, 0.0, 0.0);
			if (!collisionFlag){
				for (int i = 0; i < MAXOBJECTS; i++){
					if (objects[i] && objects[i]->selected){
						objects[i]->ty = trsize;
					}
				}
			}
			break;
		case NAVIGATE:
			float xinc = trsize * navobj->forward.x;
			float zinc = trsize * navobj->forward.z;
			collisionFlag = checkWallsForCollisionNav(xinc, 0.0, zinc, 0.0, 0.0, 0.0, 0.0);
			//translate center of object by 0.2 unit of forward vector dir
			if (!collisionFlag){
				funckey = key;
				glutTimerFunc(timerdelay, translatetimer, TIMERFRAMES);
			}
			break;
	  }
  }
  else if (key == GLUT_KEY_DOWN){
	  switch (currentAction){
		case TRANSLATE:
			collisionFlag = checkWallsForCollision(0.0, 0.0, trsize, 0.0, 0.0, 0.0, 0.0);
			if (!collisionFlag){
				for (int i = 0; i < MAXOBJECTS; i++){
					if (objects[i] && objects[i]->selected){
						objects[i]->tz = trsize;
					}
				}
			}
			break;
		case EXTRUDE:
			collisionFlag = checkWallsForCollision(0.0, 0.0, 0.0, 0.0, -scsize, 0.0, 0.0);
			if (!collisionFlag){
				for (int i = 0; i < MAXOBJECTS; i++){
					if (objects[i] && objects[i]->selected){
						if (objects[i]->sfy >= 0.001)     //prevent -ve scale values
							objects[i]->sfy -= scsize / objects[i]->dim.y;
					}
				}
			}
			break;
		case SCALE:
			collisionFlag = checkWallsForCollision(0.0, 0.0, 0.0, 0.0, 0.0, -scsize, 0.0);
			if (!collisionFlag){
				for (int i = 0; i < MAXOBJECTS; i++){
					if (objects[i] && objects[i]->selected){
						if (objects[i]->sfz >= 0.001)
							objects[i]->sfz -= scsize / objects[i]->dim.z;
					}
				}
			}
			break;
		case RAISE:
			collisionFlag = checkWallsForCollision(0.0, -trsize, 0.0, 0.0, 0.0, 0.0, 0.0);
			if (!collisionFlag){
				for (int i = 0; i < MAXOBJECTS; i++){
					if (objects[i] && objects[i]->selected){
						objects[i]->ty = -trsize;
					}
				}
			}
			break;
		case NAVIGATE:
			float xinc = trsize * navobj->forward.x;
			float zinc = trsize * navobj->forward.z;
			collisionFlag = checkWallsForCollisionNav(-xinc, 0.0, -zinc, 0.0, 0.0, 0.0, 0.0);
			//translate center of object by 0.2 unit of forward vector dir
			if (!collisionFlag){
				funckey = key;
				glutTimerFunc(timerdelay, translatetimer, TIMERFRAMES);
			}
			break;
	  }
  }
  else if (key == GLUT_KEY_LEFT){
	  switch (currentAction){
		case TRANSLATE:
			collisionFlag = checkWallsForCollision(-trsize, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0);
			if (!collisionFlag){
				for (int i = 0; i < MAXOBJECTS; i++){
					if (objects[i] && objects[i]->selected){
						objects[i]->tx = -trsize;
					}
				}
			}
			break;
		case SCALE:
			collisionFlag = checkWallsForCollision(0.0, 0.0, 0.0, -scsize, 0.0, 0.0, 0.0);
			if (!collisionFlag){
				for (int i = 0; i < MAXOBJECTS; i++){
					if (objects[i] && objects[i]->selected){
						if (objects[i]->sfx >= 0.001)
							objects[i]->sfx -= scsize / objects[i]->dim.x;
					}
				}
			}
			break;
		case ROTATE:                             //rotate wrt y-axis
			collisionFlag = checkWallsForCollision(0.0, 0.0, 0.0, 0.0, 0.0, 0.0, rosize);
			if (!collisionFlag){
				for (int i = 0; i < MAXOBJECTS; i++){
					if (objects[i] && objects[i]->selected){
						objects[i]->angle += rosize;
						if (objects[i]->angle > 180.0) objects[i]->angle -= 360.0;
						else if (objects[i]->angle < -180.0) objects[i]->angle += 360.0;
					}
				}
			}
			break;
		case SELECT:
			if (nextobj > 0){                  //if there is at least one cube
				for (currObj = nextobj-1; currObj >= 0; currObj--){  //check if there is at least one item selected
					if (objects[currObj]->selected)
						break;
				}
				if (currObj > 0){              //if there are more objects
					objects[currObj]->selected = false;     //deselect the current one
					objects[currObj - 1]->selected = true;  //select the next one
					for (int j = currObj - 2; j >= 0; j--)  //deselect the rest
						objects[j]->selected = false;
				}
				else if (currObj == 0){        //the last cube is selected
					objects[currObj]->selected = false;     //deselect the current one
					objects[nextobj-1]->selected = true;      //wrap around and select the first cube
				}
				else if (currObj == -1){            //if none are selected
					objects[0]->selected = true;
				}
			}
			break;
		case MULTIPLESELECT:
			if (nextobj > 0){                        //check if there is a cube created
				for (currObj = 0; currObj < nextobj; currObj++){  //check if there is at least one item selected
					if (objects[currObj]->selected)
						break;
				}
				int firstCube = currObj;
				do{
					if (currObj == 0)
						currObj = nextobj-1;
					else
						currObj--;
				} while (objects[currObj]->selected && currObj != firstCube);
				if (currObj != firstCube)
					objects[currObj]->selected = true;
			}
			break;
		case NAVIGATE:
			collisionFlag = checkWallsForCollisionNav(0.0, 0.0, 0.0, 0.0, 0.0, 0.0, rosize);
			//rotate forward vector of object by 0.2 unit of forward vector dir
			if (!collisionFlag){
				funckey = key;
				glutTimerFunc(timerdelay, rotatetimer, TIMERFRAMES);
			}
			break;
	  }
  }
  else if (key == GLUT_KEY_RIGHT){
	  switch (currentAction){
	  case TRANSLATE:
		  collisionFlag = checkWallsForCollision(trsize, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0);
		  if (!collisionFlag){
			  for (int i = 0; i < MAXOBJECTS; i++){
				  if (objects[i] && objects[i]->selected){
					  objects[i]->tx = trsize;
				  }
			  }
		  }
		  break;
	  case SCALE:
		  collisionFlag = checkWallsForCollision(0.0, 0.0, 0.0, scsize, 0.0, 0.0, 0.0);
		  if (!collisionFlag){
			  for (int i = 0; i < MAXOBJECTS; i++){
				  if (objects[i] && objects[i]->selected){
					  objects[i]->sfx += scsize / objects[i]->dim.x;
				  }
			  }
		  }
		  break;
	  case ROTATE:
		  collisionFlag = checkWallsForCollision(0.0, 0.0, 0.0, 0.0, 0.0, 0.0, -rosize);
		  if (!collisionFlag){
			  for (int i = 0; i < MAXOBJECTS; i++){
				  if (objects[i] && objects[i]->selected){
					  objects[i]->angle -= rosize;
					  if (objects[i]->angle > 180.0) objects[i]->angle -= 360.0;
					  else if (objects[i]->angle < -180.0) objects[i]->angle += 360.0;
				  }
			  }
		  }
		  break;
	  case SELECT:
		  if (nextobj > 0){                   //if there is at least one cube
			  for (currObj = 0; currObj < nextobj; currObj++){  //check if there is at least one item selected
				  if (objects[currObj]->selected)
					  break;
			  }
			  if (currObj < nextobj - 1){              //if there are more objects
				  objects[currObj]->selected = false;     //deselect the current one
				  objects[currObj + 1]->selected = true;  //select the next one
				  for (int j = currObj + 2; j < nextobj; j++)  //deselect the rest
					  objects[j]->selected = false;
			  }
			  else if (currObj == nextobj - 1){        //the last cube is selected
				  objects[currObj]->selected = false;     //deselect the current one
				  objects[0]->selected = true;      //wrap around and select the first cube
			  }
			  else if (currObj == nextobj){            //if none are selected
				  objects[0]->selected = true;
			  }
		  }
		  break;
	  case MULTIPLESELECT:
		  if (nextobj > 0){                        //check if there is a cube created
			  for (currObj = nextobj-1; currObj >= 0; currObj--){  //check if there is at least one item selected
				  if (objects[currObj]->selected)
					  break;
			  }
			  int firstCube = currObj;
			  do{
				  if (currObj == nextobj - 1)
					  currObj = 0;
				  else
					  currObj++;
			  } while (objects[currObj]->selected && currObj != firstCube);
			  
			  if (currObj != firstCube)
				  objects[currObj]->selected = true;
		  }
		  break;
	  case NAVIGATE:
		  collisionFlag = checkWallsForCollisionNav(0.0, 0.0, 0.0, 0.0, 0.0, 0.0, -rosize);
		  //rotate forward vector of object by 0.2 unit of forward vector dir
		  if (!collisionFlag){
			  funckey = key;
			  glutTimerFunc(timerdelay, rotatetimer, TIMERFRAMES);
		  }
		  break;
	  }
  }

  if (currentAction != NAVIGATE)
	  glutPostRedisplay();
}


