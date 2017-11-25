#include <windows.h>
#include <math.h>
#include <gl/gl.h>
#include <gl/glu.h>
#include "Dependencies\freeglut\freeglut.h"
#include "VECTOR3D.h"
#include "RGBpixmap.h"

#define PI 3.14159265
#define NUMOBJ 4

//GLuint objList[NUMOBJ];
GLUquadricObj *qobj;     //only need 1 pointer ???

typedef struct ComplexObj{
	VECTOR3D center;            // center.y is the bottom of object. center.x and center.z are actual object centers. 
	VECTOR3D dim;
	VECTOR3D forward;			// Normalized forward direction vector. Initialized to (0.0, 0.0, 1.0). NOTE: y is always 0.0

	VECTOR3D translation;		// Translatation Deltas
	VECTOR3D scaleFactor;		// X, Y, Z Scale Factors
	VECTOR3D angles;            // Rotation angles around x, y, z axis	

	bool selected;

	// Material properties for drawing
	float mat_ambient[4];
	float mat_specular[4];
	float mat_diffuse[4];
	float mat_shininess[1];

	// Material properties if selected
	float highlightMat_ambient[4];
	float highlightMat_specular[4];
	float highlightMat_diffuse[4];
	float highlightMat_shininess[1];

	GLuint objList;

	RGBpixmap partPix[5];

} ComplexObj;

ComplexObj *createObj(){
	ComplexObj *newObj = (ComplexObj*)calloc(1, sizeof(ComplexObj));

	newObj->center.Set(0, 0, 0);
	newObj->dim.Set(2.0f, 2.0f, 2.0f);
	newObj->forward.Set(0.0f, 0.0f, 1.0f);
	newObj->translation.Set(0.0, 0.0, 0.0);
	newObj->scaleFactor.Set(1.0, 1.0, 1.0);
	newObj->angles.Set(0.0, 0.0, 0.0);
	/*
	newObj->mat_ambient[0] = 0.0;
	newObj->mat_ambient[1] = 0.05;
	newObj->mat_ambient[2] = 0.0;
	newObj->mat_ambient[3] = 1.0;
	newObj->mat_specular[0] = 0.0;
	newObj->mat_specular[1] = 0.0;
	newObj->mat_specular[2] = 0.004;
	newObj->mat_specular[3] = 1.0;
	newObj->mat_diffuse[0] = 0.5;
	newObj->mat_diffuse[1] = 0.5;
	newObj->mat_diffuse[2] = 0.5;
	newObj->mat_diffuse[3] = 1.0;
	newObj->mat_shininess[0] = 0;
	*/
	newObj->mat_ambient[0] = 1.0;
	newObj->mat_ambient[1] = 1.0;
	newObj->mat_ambient[2] = 1.0;
	newObj->mat_ambient[3] = 1.0;
	newObj->mat_specular[0] = 1.0;
	newObj->mat_specular[1] = 1.0;
	newObj->mat_specular[2] = 1.0;
	newObj->mat_specular[3] = 1.0;
	newObj->mat_diffuse[0] = 1.0;
	newObj->mat_diffuse[1] = 1.0;
	newObj->mat_diffuse[2] = 1.0;
	newObj->mat_diffuse[3] = 1.0;
	newObj->mat_shininess[0] = 1;

	newObj->highlightMat_ambient[0] = 0.0;
	newObj->highlightMat_ambient[1] = 0.00;
	newObj->highlightMat_ambient[2] = 0.0;
	newObj->highlightMat_ambient[3] = 1.0;
	newObj->highlightMat_specular[0] = 0.0;
	newObj->highlightMat_specular[1] = 0.0;
	newObj->highlightMat_specular[2] = 0.0;
	newObj->highlightMat_specular[3] = 1.0;
	newObj->highlightMat_diffuse[0] = 1.0;
	newObj->highlightMat_diffuse[1] = 0.0;
	newObj->highlightMat_diffuse[2] = 0.0;
	newObj->highlightMat_diffuse[3] = 1.0;
	newObj->highlightMat_shininess[0] = 0.0;

	return newObj;
}

// Given a cobj mesh, compute it's current bounding box and return in vectors min and max
// i.e. compute min.x,min.y,min.z,max.x,max.y,max.z
// Use this function for collision detection of cobj and walls/floor
void getBBox(ComplexObj *cobj, VECTOR3D *min, VECTOR3D *max){
	float xabs, zabs;
	cobj->center = cobj->center + VECTOR3D(cobj->translation.x, cobj->translation.y, cobj->translation.z);
	xabs = fabs(cobj->dim.x/2.0 * cobj->scaleFactor.x * cos(cobj->angles.y*PI / 180.0)) + fabs(cobj->dim.z/2.0 * cobj->scaleFactor.z*sin(cobj->angles.y*PI / 180.0));      //cos(angles.y)*length cobj makes with x-axis + sin(z-dir thickness)
	zabs = fabs(cobj->dim.x/2.0 * cobj->scaleFactor.x * sin(cobj->angles.y*PI / 180.0)) + fabs(cobj->dim.z/2.0 * cobj->scaleFactor.z*cos(cobj->angles.y*PI / 180.0));      //sin(angles.y)*length cobj makes with x-axis + cos(z-dir thickness)
	min->x = cobj->center.x - xabs;
	max->x = cobj->center.x + xabs;
	min->z = cobj->center.z - zabs;
	max->z = cobj->center.z + zabs;
	min->y = cobj->center.y;  //center.y is set to y = 0.0 @ initialization
	max->y = cobj->center.y + cobj->dim.y * cobj->scaleFactor.y;
}

void drawObj(ComplexObj *cobj){
	if (cobj->selected){
		// Setup the material and lights used for the cobj
		glMaterialfv(GL_FRONT, GL_AMBIENT, cobj->highlightMat_ambient);
		glMaterialfv(GL_FRONT, GL_SPECULAR, cobj->highlightMat_specular);
		glMaterialfv(GL_FRONT, GL_DIFFUSE, cobj->highlightMat_diffuse);
		glMaterialfv(GL_FRONT, GL_SHININESS, cobj->highlightMat_shininess);
	}
	else{
		// Setup the material and lights used for the cobj
		glMaterialfv(GL_FRONT, GL_AMBIENT, cobj->mat_ambient);
		glMaterialfv(GL_FRONT, GL_SPECULAR, cobj->mat_specular);
		glMaterialfv(GL_FRONT, GL_DIFFUSE, cobj->mat_diffuse);
		glMaterialfv(GL_FRONT, GL_SHININESS, cobj->mat_shininess);
	}

	glPushMatrix(); //Here, CTM is V = RT
	glTranslatef(cobj->translation.x, cobj->translation.y, cobj->translation.z); //CTM is V * T(x,y,z)
	glRotatef(cobj->angles.y, 0, 1, 0); //CTM is V * T(x,y,z) * R_y(angle)
	glRotatef(cobj->angles.x, 1, 0, 0);
	glRotatef(cobj->angles.z, 0, 0, 1);
	glScalef(cobj->scaleFactor.x, cobj->scaleFactor.y, cobj->scaleFactor.z); //CTM is V * T(x,y,z) * R_y(angle) * S(x,y,z) 

	glCallList(cobj->objList);
	glPopMatrix();
}

void createShield(ComplexObj *cobj){
	//Create a display list(s) for a glu quadric
	double clipEq1[4] = { 0.0, 0.0, 1.0, 0.0 };
	double clipEq2[4] = { 0.0, 0.0, 1.0, -1.3 };

	cobj->partPix[0].readBMPFile("Shield02.bmp");
	cobj->partPix[1].readBMPFile("Shield01.bmp");
	cobj->partPix[2].readBMPFile("MetalRust01.bmp");
	cobj->dim.Set(1.6, 1.6, 0.4);  //set dimensions

	cobj->objList = glGenLists(1);
	qobj = gluNewQuadric();
	gluQuadricDrawStyle(qobj, GLU_FILL);
	gluQuadricNormals(qobj, GLU_SMOOTH);

	glNewList(cobj->objList, GL_COMPILE);
	//Create and init shield (torus + sphere + cylinder
	glEnable(GL_TEXTURE_2D);
	glPushMatrix();     //base of shield
	//glTranslatef(0.0, 0.0, 0.0);
	glEnable(GL_CLIP_PLANE0);
	glClipPlane(GL_CLIP_PLANE0, clipEq1);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, cobj->partPix[0].nCols, cobj->partPix[0].nRows, 0, GL_RGB, GL_UNSIGNED_BYTE, cobj->partPix[0].pixel);
	gluQuadricTexture(qobj, GLU_TRUE);
	gluCylinder(qobj, 0.8, 0.6, 0.1, 20, 50);
	glDisable(GL_CLIP_PLANE0);
	glPopMatrix();
	glPushMatrix();     //central dome of shield 
	glTranslatef(0.0, 0.0, -1.3);
	glEnable(GL_CLIP_PLANE1);
	glClipPlane(GL_CLIP_PLANE1, clipEq2);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, cobj->partPix[1].nCols, cobj->partPix[1].nRows, 0, GL_RGB, GL_UNSIGNED_BYTE, cobj->partPix[1].pixel);
	gluQuadricTexture(qobj, GLU_TRUE);
	gluSphere(qobj, 1.5, 20, 10);
	glDisable(GL_CLIP_PLANE1);
	glPopMatrix();
	glPushMatrix();     //centre spike 
	glTranslatef(0.0, 0.0, 0.2);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, cobj->partPix[2].nCols, cobj->partPix[2].nRows, 0, GL_RGB, GL_UNSIGNED_BYTE, cobj->partPix[2].pixel);
	gluQuadricTexture(qobj, GLU_TRUE);
	gluCylinder(qobj, 0.25, 0.0, 0.25, 20, 10);
	glPopMatrix();
	glDisable(GL_TEXTURE_2D);

	glEndList();

	gluDeleteQuadric(qobj);
}

void createTable(ComplexObj *cobj){
	//Create a display list(s) for a glu quadric

	cobj->partPix[0].readBMPFile("Bamboo01.bmp");
	cobj->dim.Set(1.6, 0.9, 1.6);  //set dimensions 

	cobj->objList = glGenLists(1);
	qobj = gluNewQuadric();
	gluQuadricDrawStyle(qobj, GLU_FILL);
	gluQuadricNormals(qobj, GLU_SMOOTH);

	glNewList(cobj->objList, GL_COMPILE);
	glEnable(GL_TEXTURE_2D);
	glPushMatrix();     //table edge
	glTranslatef(0.0, 0.8, 0.0);
	glRotatef(90.0, 1.0, 0.0, 0.0);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, cobj->partPix[0].nCols, cobj->partPix[0].nRows, 0, GL_RGB, GL_UNSIGNED_BYTE, cobj->partPix[0].pixel);
	gluQuadricTexture(qobj, GLU_TRUE);
	gluCylinder(qobj, 0.8, 0.8, 0.1, 20, 50);
	glPopMatrix();
	glPushMatrix();     //table top
	glTranslatef(0.0, 0.8, 0.0);
	glRotatef(90.0, 1.0, 0.0, 0.0);
	gluQuadricTexture(qobj, GLU_TRUE);
	gluDisk(qobj, 0.0, 0.8, 20, 50);
	glPopMatrix();
	glPushMatrix();     //table leg
	glTranslatef(0.5, 0.8, 0.5);
	glRotatef(90.0, 1.0, 0.0, 0.0);
	gluQuadricTexture(qobj, GLU_TRUE);
	gluCylinder(qobj, 0.1, 0.1, 0.8, 20, 50);
	glPopMatrix();
	glPushMatrix();     //table leg
	glTranslatef(0.5, 0.8, -0.5);
	glRotatef(90.0, 1.0, 0.0, 0.0);
	gluQuadricTexture(qobj, GLU_TRUE);
	gluCylinder(qobj, 0.1, 0.1, 0.8, 20, 50);
	glPopMatrix();
	glPushMatrix();     //table leg
	glTranslatef(-0.5, 0.8, -0.5);
	glRotatef(90.0, 1.0, 0.0, 0.0);
	gluQuadricTexture(qobj, GLU_TRUE);
	gluCylinder(qobj, 0.1, 0.1, 0.8, 20, 50);
	glPopMatrix();
	glPushMatrix();     //table leg
	glTranslatef(-0.5, 0.8, 0.5);
	glRotatef(90.0, 1.0, 0.0, 0.0);
	gluQuadricTexture(qobj, GLU_TRUE);
	gluCylinder(qobj, 0.1, 0.1, 0.8, 20, 50);
	glPopMatrix();
	glDisable(GL_TEXTURE_2D);

	glEndList();

	gluDeleteQuadric(qobj);
}

void createVase(ComplexObj *cobj){
	//Create a display list(s) for a glu quadric

	cobj->partPix[0].readBMPFile("WoodWall01.bmp");
	cobj->dim.Set(0.6, 0.6, 0.9);  //set dimensions 

	cobj->objList = glGenLists(1);
	qobj = gluNewQuadric();
	gluQuadricDrawStyle(qobj, GLU_FILL);
	gluQuadricNormals(qobj, GLU_SMOOTH);

	glNewList(cobj->objList, GL_COMPILE);
	glEnable(GL_TEXTURE_2D);
	glPushMatrix();     //vase base
	glRotatef(-90.0, 1.0, 0.0, 0.0);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, cobj->partPix[0].nCols, cobj->partPix[0].nRows, 0, GL_RGB, GL_UNSIGNED_BYTE, cobj->partPix[0].pixel);
	gluQuadricTexture(qobj, GLU_TRUE);
	gluCylinder(qobj, 0.3, 0.2, 0.2, 20, 50);
	glPopMatrix();
	glPushMatrix();     //vase body
	glTranslatef(0.0, 0.3, 0.0);
	gluQuadricTexture(qobj, GLU_TRUE);
	gluSphere(qobj, 0.3, 20, 50);
	glPopMatrix();
	glPushMatrix();     //vase top
	glTranslatef(0.0, 0.5, 0.0);
	glRotatef(-90.0, 1.0, 0.0, 0.0);
	gluQuadricTexture(qobj, GLU_TRUE);
	gluCylinder(qobj, 0.2, 0.3, 0.3, 20, 50);
	glPopMatrix();
	glDisable(GL_TEXTURE_2D);

	glEndList();

	gluDeleteQuadric(qobj);
}

void createPicFrame(ComplexObj *cobj){
	//Create a display list(s) for a glu quadric

	cobj->partPix[0].readBMPFile("Creation01.bmp");
	cobj->partPix[1].readBMPFile("FrameGold01Top.bmp");
	cobj->partPix[2].readBMPFile("FrameGold01Right.bmp");
	cobj->partPix[3].readBMPFile("FrameGold01Bottom.bmp");
	cobj->partPix[4].readBMPFile("FrameGold01Left.bmp");
	cobj->dim.Set(2.2, 1.2, 0.3);  //set dimensions

	cobj->objList = glGenLists(1);

	glNewList(cobj->objList, GL_COMPILE);
	glEnable(GL_TEXTURE_2D);
	glPushMatrix();     //picture
	//glBindTexture(GL_TEXTURE_2D, 2000);
	//glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	//glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTranslatef(0.0, 0.1, 0.0);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, cobj->partPix[0].nCols, cobj->partPix[0].nRows, 0, GL_RGB, GL_UNSIGNED_BYTE, cobj->partPix[0].pixel);
	glBegin(GL_QUADS);
	glTexCoord2f(0.0, 0.0);
	glVertex3f(-1.0f, 0.0f, 0.0f);
	glTexCoord2f(0.0, 1.0);
	glVertex3f(-1.0f, 1.0f, 0.0f);
	glTexCoord2f(1.0, 1.0);
	glVertex3f(1.0f, 1.0f, 0.0f);
	glTexCoord2f(1.0, 0.0);
	glVertex3f(1.0f, 0.0f, 0.0f);
	glEnd();
	glPopMatrix();
	glPushMatrix();     //frame top
	glTranslatef(0.0, 1.1, 0.0);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, cobj->partPix[1].nCols, cobj->partPix[1].nRows, 0, GL_RGB, GL_UNSIGNED_BYTE, cobj->partPix[1].pixel);
	glBegin(GL_QUADS);
	glTexCoord2f(0.0, 0.0);
	glVertex3f(-1.1f, 0.0f, 0.0f);
	glTexCoord2f(0.0, 1.0);
	glVertex3f(-1.1f, 0.1f, 0.0f);
	glTexCoord2f(1.0, 1.0);
	glVertex3f(1.1f, 0.1f, 0.0f);
	glTexCoord2f(1.0, 0.0);
	glVertex3f(1.1f, 0.0f, 0.0f);
	glEnd();
	glPopMatrix();
	glPushMatrix();     //frame right
	glTranslatef(1.0, 0.0, 0.0);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, cobj->partPix[2].nCols, cobj->partPix[2].nRows, 0, GL_RGB, GL_UNSIGNED_BYTE, cobj->partPix[2].pixel);
	glBegin(GL_QUADS);
	glTexCoord2f(0.0, 0.0);
	glVertex3f(0.0f, 0.0f, 0.0f);
	glTexCoord2f(0.0, 1.0);
	glVertex3f(0.0f, 1.2f, 0.0f);
	glTexCoord2f(1.0, 1.0);
	glVertex3f(0.1f, 1.2f, 0.0f);
	glTexCoord2f(1.0, 0.0);
	glVertex3f(0.1f, 0.0f, 0.0f);
	glEnd();
	glPopMatrix();
	glPushMatrix();     //frame bottom
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, cobj->partPix[3].nCols, cobj->partPix[3].nRows, 0, GL_RGB, GL_UNSIGNED_BYTE, cobj->partPix[3].pixel);
	glBegin(GL_QUADS);
	glTexCoord2f(0.0, 0.0);
	glVertex3f(-1.1f, 0.0f, 0.0f);
	glTexCoord2f(0.0, 1.0);
	glVertex3f(-1.1f, 0.1f, 0.0f);
	glTexCoord2f(1.0, 1.0);
	glVertex3f(1.1f, 0.1f, 0.0f);
	glTexCoord2f(1.0, 0.0);
	glVertex3f(1.1f, 0.0f, 0.0f);
	glEnd();
	glPopMatrix();
	glPushMatrix();     //frame left
	glTranslatef(-1.0, 0.0, 0.0);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, cobj->partPix[4].nCols, cobj->partPix[4].nRows, 0, GL_RGB, GL_UNSIGNED_BYTE, cobj->partPix[4].pixel);
	glBegin(GL_QUADS);
	glTexCoord2f(0.0, 0.0);
	glVertex3f(-0.1f, 0.0f, 0.0f);
	glTexCoord2f(0.0, 1.0);
	glVertex3f(-0.1f, 1.1f, 0.0f);
	glTexCoord2f(1.0, 1.0);
	glVertex3f(0.0f, 1.1f, 0.0f);
	glTexCoord2f(1.0, 0.0);
	glVertex3f(0.0f, 0.0f, 0.0f);
	glEnd();
	glPopMatrix();

	glDisable(GL_TEXTURE_2D);
	glEndList();

}


