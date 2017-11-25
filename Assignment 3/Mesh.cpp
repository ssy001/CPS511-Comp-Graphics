#include <windows.h>
//#include <gl/glut.h>
#include "Dependencies\freeglut\freeglut.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <utility>
#include <vector>
#include "VECTOR3D.h"
#include <string.h>
#include <math.h>
#include "Mesh.h"

GLfloat baseQuad[][3]= {{-1.0, 0.0, 1.0},  // 0 - square of size 2 around origin
                        { 1.0, 0.0, 1.0},  // 1
	                    { 1.0, 0.0,-1.0},  // 2
                        {-1.0, 0.0,-1.0}}; // 3

void computeQuadNormal(GLfloat *qn, GLfloat *v1, GLfloat *v2, GLfloat *v3);

Mesh *createMesh(VECTOR3D scale, VECTOR3D translation, float extrudeDelta, int numBaseCopies) {
	Mesh *newMesh = (Mesh*)calloc(1, sizeof(Mesh));
  
	// Allocate mesh vertices and quads - just allocate max 
	newMesh->vertices   = (MVertex *)calloc(MAXVERTICES, sizeof(MVertex));
	newMesh->quads      = (MQuad   *)calloc(MAXQUADS,    sizeof(MQuad));
	newMesh->quadNormals= (MNormal *)calloc(MAXQUADS,    sizeof(MNormal));
  
	newMesh->scaleFactor.x  =  scale.x;
	newMesh->scaleFactor.y  =  scale.y;
	newMesh->scaleFactor.z  =  scale.z;
	newMesh->translation.x  =  translation.x;
	newMesh->translation.y  =  translation.y;
	newMesh->translation.z  =  translation.z;

	newMesh->extrudeDelta = extrudeDelta;
	newMesh->numBaseCopies= numBaseCopies;

	newMesh->mat_ambient[0] = 0.0;
	newMesh->mat_ambient[1] = 0.05;
	newMesh->mat_ambient[2] = 0.0;
	newMesh->mat_ambient[3] = 1.0;
	newMesh->mat_specular[0] = 0.0;
	newMesh->mat_specular[1] = 0.0;
	newMesh->mat_specular[2] = 0.004;
	newMesh->mat_specular[3] = 1.0;
	newMesh->mat_diffuse[0] = 0.5;
	newMesh->mat_diffuse[1] = 0.5;
	newMesh->mat_diffuse[2] = 0.5;
	newMesh->mat_diffuse[3] = 1.0;
	newMesh->mat_shininess[0] = 0;
  
	newMesh->highlightMat_ambient[0] = 0.0;
	newMesh->highlightMat_ambient[1] = 0.00;
	newMesh->highlightMat_ambient[2] = 0.0;
	newMesh->highlightMat_ambient[3] = 1.0;
	newMesh->highlightMat_specular[0] = 0.0;
	newMesh->highlightMat_specular[1] = 0.0;
	newMesh->highlightMat_specular[2] = 0.0;
	newMesh->highlightMat_specular[3] = 1.0;
	newMesh->highlightMat_diffuse[0] = 1.0;
	newMesh->highlightMat_diffuse[1] = 0.0;
	newMesh->highlightMat_diffuse[2] = 0.0;
	newMesh->highlightMat_diffuse[3] = 1.0;
	newMesh->highlightMat_shininess[0] = 0.0;
    
	// Set up initial base quad
	for (int i = 0; i < 4; i++) {
		newMesh->vertices[i].pos[0] = baseQuad[i][0];
		newMesh->vertices[i].pos[1] = baseQuad[i][1];
		newMesh->vertices[i].pos[2] = baseQuad[i][2];
	}
	
	int qi = 0;
	int vi = 4;
	int viprev = 0;
	
	// Copy current base, translate y for each copy by extrudeDelta, repeat for numBaseCopies (NOTE: extrudeDelta only affects height, i.e. y)
	for (int i = 1; i <= numBaseCopies; i++) {
		float height = i * extrudeDelta;
		float h      = (float) i / (float)numBaseCopies; // normalized height - NOT USED atm

		// Copy base quad and set height
		for (int j = 0; j < 4; j++) {
          newMesh->vertices[vi+j].pos[0] = baseQuad[j][0]; 
		  newMesh->vertices[vi+j].pos[2] = baseQuad[j][2];
		  newMesh->vertices[vi+j].pos[1] = height;
	    }
				
		// Create side quads
		for (int j = 0; j < 4; j++) {
			// wrap around
			if (j == 3) {
			  newMesh->quads[qi+j].vi[0] = viprev;     //newMesh->quads[3].vi[0] = 3 (j==3, qi==0)
		      newMesh->quads[qi+j].vi[1] = viprev - 3; //newMesh->quads[3].vi[1] = 0
		      newMesh->quads[qi+j].vi[2] = vi     - 3; //newMesh->quads[3].vi[2] = 4
		      newMesh->quads[qi+j].vi[3] = vi;         //newMesh->quads[3].vi[3] = 7
			}
			else {
		      newMesh->quads[qi+j].vi[0] = viprev;   //newMesh->quads[0].vi[0] = 0  (j==0), newMesh->quads[1].vi[0] = 1 (j==1)
		      newMesh->quads[qi+j].vi[1] = viprev+1; //newMesh->quads[0].vi[1] = 1        , newMesh->quads[1].vi[1] = 2
		      newMesh->quads[qi+j].vi[2] = vi+1;     //newMesh->quads[0].vi[2] = 5        , newMesh->quads[1].vi[2] = 6
		      newMesh->quads[qi+j].vi[3] = vi;       //newMesh->quads[0].vi[3] = 4        , newMesh->quads[1].vi[3] = 5
			}
			computeQuadNormal(&newMesh->quadNormals[qi+j].v[0],
				              &newMesh->vertices[newMesh->quads[qi+j].vi[0]].pos[0],
			                  &newMesh->vertices[newMesh->quads[qi+j].vi[1]].pos[0],
		                      &newMesh->vertices[newMesh->quads[qi+j].vi[2]].pos[0]);
			viprev++;
			vi++;
	    }
		// Next 
		viprev = i    * 4;
		vi     = (i+1)* 4;
		qi     = i    * 4;
	}
	newMesh->numVertices = vi;
	
	// End cap quads
	newMesh->quads[qi].vi[0] = 3;
	newMesh->quads[qi].vi[1] = 2;
	newMesh->quads[qi].vi[2] = 1;
	newMesh->quads[qi].vi[3] = 0;
	newMesh->quadNormals[qi].v[0] = 0;
	newMesh->quadNormals[qi].v[1] = -1;
	newMesh->quadNormals[qi].v[2] = 0;
	qi++;
	newMesh->quads[qi].vi[0] = vi-4;
	newMesh->quads[qi].vi[1] = vi-3;
	newMesh->quads[qi].vi[2] = vi-2;
	newMesh->quads[qi].vi[3] = vi-1;
	newMesh->quadNormals[qi].v[0] = 0;
	newMesh->quadNormals[qi].v[1] = 1;
	newMesh->quadNormals[qi].v[2] = 0;
	
	newMesh->numQuads    = ++qi;

	newMesh->personHeadPix[0].readBMPFile("WomanLego02Head.bmp");
	newMesh->personBodyPix[0].readBMPFile("WomanLego02Body.bmp");
	newMesh->headAngle.Set(0.0, 0.0, 0.0);
	newMesh->turnHead = false;

	return newMesh;
}

void drawMesh(Mesh *mesh){
  if (mesh->selected){
	// Setup the material and lights used for the cube
    glMaterialfv(GL_FRONT, GL_AMBIENT, mesh->highlightMat_ambient);
    glMaterialfv(GL_FRONT, GL_SPECULAR, mesh->highlightMat_specular);
    glMaterialfv(GL_FRONT, GL_DIFFUSE, mesh->highlightMat_diffuse);
    glMaterialfv(GL_FRONT, GL_SHININESS, mesh->highlightMat_shininess);
  }
  else{
	// Setup the material and lights used for the cube
    glMaterialfv(GL_FRONT, GL_AMBIENT, mesh->mat_ambient);
    glMaterialfv(GL_FRONT, GL_SPECULAR, mesh->mat_specular);
    glMaterialfv(GL_FRONT, GL_DIFFUSE, mesh->mat_diffuse);
    glMaterialfv(GL_FRONT, GL_SHININESS, mesh->mat_shininess);
  }
  // Transform and Draw Mesh   
  glPushMatrix();
  glTranslatef(mesh->translation.x, mesh->translation.y, mesh->translation.z);
  glRotatef(mesh->angles.y, 0, 1, 0);
  glRotatef(mesh->angles.x, 1, 0, 0);
  glRotatef(mesh->angles.z, 0, 0, 1);
  glScalef(mesh->scaleFactor.x, mesh->scaleFactor.y, mesh->scaleFactor.z); 
  
  glBegin(GL_QUADS);
  for (int i = 0; i < mesh->numQuads; i++) {
    glNormal3f(mesh->quadNormals[i].v[0],mesh->quadNormals[i].v[1],mesh->quadNormals[i].v[2]); 
	glTexCoord2f(0.0f, 0.0f); 
	glVertex3f(mesh->vertices[mesh->quads[i].vi[0]].pos[0],
		       mesh->vertices[mesh->quads[i].vi[0]].pos[1],
			   mesh->vertices[mesh->quads[i].vi[0]].pos[2]);
	glTexCoord2f(1.0f, 0.0f);
	glVertex3f(mesh->vertices[mesh->quads[i].vi[1]].pos[0],
               mesh->vertices[mesh->quads[i].vi[1]].pos[1],
               mesh->vertices[mesh->quads[i].vi[1]].pos[2]);
	glTexCoord2f(1.0f, 1.0f);
	glVertex3f(mesh->vertices[mesh->quads[i].vi[2]].pos[0],
               mesh->vertices[mesh->quads[i].vi[2]].pos[1],
               mesh->vertices[mesh->quads[i].vi[2]].pos[2]);
	glTexCoord2f(0.0f, 1.0f);
	glVertex3f(mesh->vertices[mesh->quads[i].vi[3]].pos[0],
               mesh->vertices[mesh->quads[i].vi[3]].pos[1],
               mesh->vertices[mesh->quads[i].vi[3]].pos[2]);
  }
  glEnd();
  glPopMatrix();
}

void drawMesh(Mesh *mesh, GLUquadricObj *head){
	if (mesh->selected){
		// Setup the material and lights used for the cube
		glMaterialfv(GL_FRONT, GL_AMBIENT, mesh->highlightMat_ambient);
		glMaterialfv(GL_FRONT, GL_SPECULAR, mesh->highlightMat_specular);
		glMaterialfv(GL_FRONT, GL_DIFFUSE, mesh->highlightMat_diffuse);
		glMaterialfv(GL_FRONT, GL_SHININESS, mesh->highlightMat_shininess);
	}
	else{
		// Setup the material and lights used for the cube
		glMaterialfv(GL_FRONT, GL_AMBIENT, mesh->mat_ambient);
		glMaterialfv(GL_FRONT, GL_SPECULAR, mesh->mat_specular);
		glMaterialfv(GL_FRONT, GL_DIFFUSE, mesh->mat_diffuse);
		glMaterialfv(GL_FRONT, GL_SHININESS, mesh->mat_shininess);
	}
	// Transform and Draw body
	glPushMatrix();
	glTranslatef(mesh->translation.x, mesh->translation.y, mesh->translation.z);
	glRotatef(mesh->angles.y, 0, 1, 0);
	glRotatef(mesh->angles.x, 1, 0, 0);
	glRotatef(mesh->angles.z, 0, 0, 1);
	glScalef(mesh->scaleFactor.x, mesh->scaleFactor.y, mesh->scaleFactor.z);

	glEnable(GL_TEXTURE_2D);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, mesh->personBodyPix[0].nCols, mesh->personBodyPix[0].nRows, 0, GL_RGB, GL_UNSIGNED_BYTE, mesh->personBodyPix[0].pixel);
	glBegin(GL_QUADS);
	for (int i = 0; i < mesh->numQuads; i++) {
		glNormal3f(mesh->quadNormals[i].v[0], mesh->quadNormals[i].v[1], mesh->quadNormals[i].v[2]);
		glTexCoord2f(0.0f, 0.0f);
		glVertex3f(mesh->vertices[mesh->quads[i].vi[0]].pos[0],
			mesh->vertices[mesh->quads[i].vi[0]].pos[1],
			mesh->vertices[mesh->quads[i].vi[0]].pos[2]);
		glTexCoord2f(1.0f, 0.0f);
		glVertex3f(mesh->vertices[mesh->quads[i].vi[1]].pos[0],
			mesh->vertices[mesh->quads[i].vi[1]].pos[1],
			mesh->vertices[mesh->quads[i].vi[1]].pos[2]);
		glTexCoord2f(1.0f, 1.0f);
		glVertex3f(mesh->vertices[mesh->quads[i].vi[2]].pos[0],
			mesh->vertices[mesh->quads[i].vi[2]].pos[1],
			mesh->vertices[mesh->quads[i].vi[2]].pos[2]);
		glTexCoord2f(0.0f, 1.0f);
		glVertex3f(mesh->vertices[mesh->quads[i].vi[3]].pos[0],
			mesh->vertices[mesh->quads[i].vi[3]].pos[1],
			mesh->vertices[mesh->quads[i].vi[3]].pos[2]);
	}
	glEnd();
	glPopMatrix();

	// draw head
	glPushMatrix();
	glTranslatef(mesh->translation.x, mesh->translation.y + mesh->scaleFactor.y, mesh->translation.z);
	glRotatef(mesh->angles.y + mesh->headAngle.y, 0, 1, 0);
	glRotatef(mesh->angles.x + mesh->headAngle.x, 1, 0, 0);
	glRotatef(mesh->angles.z + mesh->headAngle.z, 0, 0, 1);
	glRotatef(-90.0, 0, 1, 0); //turn head -90 - for face texture alignment
	glRotatef(-90.0, 1, 0, 0); //turn head up - from z-axis to y-axis
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, mesh->personHeadPix[0].nCols, mesh->personHeadPix[0].nRows, 0, GL_RGB, GL_UNSIGNED_BYTE, mesh->personHeadPix[0].pixel);
	head = gluNewQuadric();
	gluQuadricTexture(head, true);
	gluCylinder(head, mesh->scaleFactor.x * 0.55, mesh->scaleFactor.x * 0.55, mesh->scaleFactor.y*0.4, 20, 10);

	glPopMatrix();
	glDisable(GL_TEXTURE_2D);
}


// takes 
void computeQuadNormal(GLfloat *qn, GLfloat *v1, GLfloat *v2, GLfloat *v3){
	GLfloat vector1[3];
	GLfloat vector2[3];

	// calculate RH rule vectors for cross product
	vector1[0] = v1[0] - v2[0];
	vector1[1] = v1[1] - v2[1];
	vector1[2] = v1[2] - v2[2];

	vector2[0] = v3[0] - v2[0];
	vector2[1] = v3[1] - v2[1];
	vector2[2] = v3[2] - v2[2];

	// calculate cross product
	qn[0] = vector2[1]*vector1[2] - vector2[2]*vector1[1]; 
	qn[1] = vector2[2]*vector1[0] - vector2[0]*vector1[2]; 
	qn[2] = vector2[0]*vector1[1] - vector2[1]*vector1[0];

	// normalize cross product
	float norm = (float)sqrt(qn[0]*qn[0]+qn[1]*qn[1]+qn[2]*qn[2]);
	if(norm > 0){ // test in case |norm| == 0
	  qn[0] /= norm; qn[1] /= norm; qn[2] /= norm;
	}
}


