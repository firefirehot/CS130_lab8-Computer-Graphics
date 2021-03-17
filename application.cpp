#include "application.h"

#include <iostream>
#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <unistd.h>
#include <time.h>


#ifndef __APPLE__
#include <GL/gl.h>
#include <GL/glu.h>
#include <GL/glut.h>
#else
#include <GLUT/glut.h>
#include <OpenGL/gl.h>
#include <OpenGL/glu.h>
#endif

using namespace std;
enum { NONE, AMBIENT, DIFFUSE, SPECULAR, NUM_MODES };

void draw_grid(int dim);
void draw_obj(obj *o, const gl_image_texture_map& textures);




unsigned int p_index = 0;
unsigned int top_track = 0;
void Add_Particles(int n) {
	if(p_index < 5000)
		p_index = p_index + n;
	top_track = top_track + n;
	if(top_track >= 5000)
		top_track = top_track-5000;
	
}
class Particle{
private:
//bool draw_particle;
vec3 color;
vec3 position;
vec3 velocity;
vec3 force;
int mass;
float durration;

public:

Particle(){
	//draw_particle = false;
	color = {1.0,0,0};
	position = {0,0,0};
	velocity = {0,0,0};
	force = {0,0,0};
	durration = 0;
	mass = 1;
	}

void Euler_Step(float h){
	position = position + h*velocity;
	velocity = velocity + h*force/mass;
	durration = durration + h;
} // update v and x with an Forward Euler Step 
                         // (see particle.pdf and Part 1.1)

void Reset_Forces(){
	force = {0,0,0};
	
} // reset force to 0 vector;
void Apply_Gravity(){
	force[1] = force[1] - 9.8*mass; 
	}

void Handle_Collision(float damping, float coeff_resititution){
	if(position[1] < 0){
	position[1] = 0;
	velocity[1] = -coeff_resititution*velocity[1];
	velocity[0] = damping*velocity[0];
	velocity[2] = damping*velocity[2];
	}
	
}
// reflect particle on ground and apply damping and restitution
// (see Force Sources section of the document and Part 1.3)

void start_particle(){
	mass = 1;
	position[0] = random(-0.2,0.2);
	position[1] = 0.05;
	position[2] = random(-0.2,0.2);
	velocity[0] = 10*position[0];
	velocity[1] = random(5.0,8.0);
	velocity[2] = 10*position[2];
	color = {1,1,0};
	durration = 0;
	}
	
void draw_particle(float lag){
	if(durration < 0.1)
		glColor3f(1,1,0);
	else if(durration < 1.5)
		glColor3f(1,(2-durration)/2,color[2]);
	else if(durration < 2)
		glColor3f(1,0,0);
	else if(durration < 3)
		glColor3f(0.15*(durration-2)+1*(1-(durration-2)),0.15*(durration-2),0.15*(durration-2));
	else
		glColor3f(0.15,0.15,0.15);
glBegin(GL_LINES);
	
	glVertex3f(position[0] +lag*abs(velocity[0]), position[1] +lag*abs(velocity[1]),position[2] +lag*abs(velocity[2]) );
	
	glVertex3f(position[0],position[1],position[2]);
	
	glEnd();
}

float random(float low, float high){
return low + (high-low)*(static_cast <float>(rand()))/(static_cast <float>(RAND_MAX));
}
// returns a random float between low and high

	
};

Particle p_array[5000];


void set_pixel(int x, int y, float col[3])
{
    // write a 1x1 block of pixels of color col to framebuffer
    // coordinates (x, y)
    //glRasterPos2i(x, y);
    //glDrawPixels(1, 1, GL_RGB, GL_FLOAT, col);

    // use glVertex instead of glDrawPixels (faster)
    glBegin(GL_POINTS);
    glColor3fv(col);
    glVertex2f(x, y);
    glEnd();
}

application::application()
    : raytrace(false), rendmode(SPECULAR), paused(false), sim_t(0.0),draw_volcano(true),h(0.015)
{
}

application::~application()
{
}

// triggered once after the OpenGL context is initialized
void application::init_event()
{

    cout << "CAMERA CONTROLS: \n  LMB: Rotate \n  MMB: Move \n  RMB: Zoom" << endl;
    cout << "KEYBOARD CONTROLS: \n";
    cout << "  'p': Pause simulation\n";
    cout << "  'v': Toggle draw volcano" << endl;

    const GLfloat ambient[] = { 0.0, 0.0, 0.0, 1.0 };
    const GLfloat diffuse[] = { 1.0, 1.0, 1.0, 1.0 };
    const GLfloat specular[] = { 1.0, 1.0, 1.0, 1.0 };

    // enable a light
    glLightfv(GL_LIGHT1, GL_AMBIENT, ambient);
    glLightfv(GL_LIGHT1, GL_DIFFUSE, diffuse);
    glLightfv(GL_LIGHT1, GL_SPECULAR, specular);
    glEnable(GL_LIGHT1);

    // set global ambient lighting
    GLfloat amb[] = { 0.4, 0.4, 0.4, 1.0 };
    glLightModelfv(GL_LIGHT_MODEL_AMBIENT, amb);

    // enable depth-testing, colored materials, and lighting
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);
    glDisable(GL_COLOR_MATERIAL);
    glEnable(GL_LIGHTING);

    // normalize normals so lighting calculations are correct
    // when using GLUT primitives
    glEnable(GL_RESCALE_NORMAL);

    // enable smooth shading
    glShadeModel(GL_SMOOTH);

    glClearColor(0,0,0,0);

    set_camera_for_box(vec3(-3,-2,-3),vec3(3,5,3));

    t.reset();
    o.load("crater.obj");

    // loads up all the textures referenced by the .mtl file
    const std::map<std::string, obj::material>& mats = o.get_materials();
    for (std::map<std::string, obj::material>::const_iterator i = mats.begin();
        i != mats.end(); ++i
        )
    {
        if (!i->second.map_kd.empty()) {
            string filename = i->second.map_kd;

            // add texture if we have not already loaded it
            if (texs.find(filename) == texs.end()) {
                gl_image_texture *tex = new gl_image_texture();
                if (tex->load_texture(filename) != SUCCESS) {
                    cout << "could not load texture file: " << filename << endl;
                    exit(0);
                }
                texs[filename] = tex;
            }
        }
    }
}

// triggered each time the application needs to redraw
void application::draw_event()
{
    apply_gl_transform();

    const GLfloat light_pos1[] = { 0.0, 10.0, 0.0, 1 };
    glLightfv(GL_LIGHT1, GL_POSITION, light_pos1);

    if (!paused) {
        //
        //ADD NEW PARTICLES
        int numparticles = 10;
        for(unsigned int i = top_track; i <top_track + numparticles; i++){
				p_array[i].start_particle();
		}
		Add_Particles(numparticles);
		for(unsigned int i = 0;i <  p_index;i++){
			p_array[i].Apply_Gravity();
			p_array[i].Handle_Collision(0.5,0.5);
			p_array[i].Euler_Step(h);
			p_array[i].Reset_Forces();
		}
        //
        //
        // SIMULATE YOUR PARTICLE HERE.
        //
        //
        //
        // UPDATE THE COLOR OF THE PARTICLE DYNAMICALLY
        //
    }

    glLineWidth(2.0);
    glEnable(GL_COLOR_MATERIAL);
    
   // glBegin(GL_LINES);
        //
        //
        for(unsigned int i = 0;i <  p_index;i++){
			p_array[i].draw_particle(0.04);
		}
        // DRAW YOUR PARTICLE USING GL_LINES HERE
        //
        // glVertex3f(...) endpoint 1
        // glVertex3f(...) endpoint 2
        //
        //
    //glEnd();

    // draw the volcano
    if(draw_volcano){
        glEnable(GL_LIGHTING);
        glPushMatrix();
        glScalef(0.2,0.3,0.2);
        draw_obj(&o, texs);
        glPopMatrix();
        glDisable(GL_LIGHTING);
    }


    glColor3f(0.15, 0.15, 0.15);
    draw_grid(40);

    //
    // This makes sure that the frame rate is locked to close to 60 fps.
    // For each call to draw_event you will want to run your integrate for 0.015s
    // worth of time.
    //
    float elap = t.elapsed();
    if (elap < h) {
        usleep(1e6*(h-elap));
		//sleep(h-elap);
    }
    t.reset();
}

// triggered when mouse is clicked
void application::mouse_click_event(int button, int button_state, int x, int y)
{
}

// triggered when mouse button is held down and the mouse is
// moved
void application::mouse_move_event(int x, int y)
{
}

// triggered when a key is pressed on the keyboard
void application::keyboard_event(unsigned char key, int x, int y)
{

    if (key == 'r') {
        sim_t = 0;
    } else if (key == 'p') {
        paused = !paused;
    } else if (key == 'v') {
        draw_volcano=!draw_volcano;
    } else if (key == 'q') {
        exit(0);
    }
}

void draw_grid(int dim)
{
    glLineWidth(2.0);


    //
    // Draws a grid along the x-z plane
    //
    glLineWidth(1.0);
    glBegin(GL_LINES);

    int ncells = dim;
    int ncells2 = ncells/2;

    for (int i= 0; i <= ncells; i++)
    {
        int k = -ncells2;
        k +=i;
        glVertex3f(ncells2,0,k);
        glVertex3f(-ncells2,0,k);
        glVertex3f(k,0,ncells2);
        glVertex3f(k,0,-ncells2);
    }
    glEnd();

    //
    // Draws the coordinate frame at origin
    //
    glPushMatrix();
    glScalef(1.0, 1.0, 1.0);
    glBegin(GL_LINES);

    // x-axis
    glColor3f(1.0, 0.0, 0.0);
    glVertex3f(0.0, 0.0, 0.0);
    glVertex3f(1.0, 0.0, 0.0);

    // y-axis
    glColor3f(0.0, 1.0, 0.0);
    glVertex3f(0.0, 0.0, 0.0);
    glVertex3f(0.0, 1.0, 0.0);

    // z-axis
    glColor3f(0.0, 0.0, 1.0);
    glVertex3f(0.0, 0.0, 0.0);
    glVertex3f(0.0, 0.0, 1.0);
    glEnd();
    glPopMatrix();
}

void draw_obj(obj *o, const gl_image_texture_map& textures)
{
    glDisable(GL_COLOR_MATERIAL);

    // draw each polygon of the mesh
    size_t nfaces = o->get_face_count();
    for (size_t i = 0; i < nfaces; ++i)
    {
        const obj::face& f = o->get_face(i);

        // sets the material properties of the face
        const obj::material& mat = o->get_material(f.mat);
        if (!mat.map_kd.empty()) {
            gl_image_texture_map::const_iterator it = textures.find(mat.map_kd);
            if (it != textures.end()) {
                gl_image_texture* tex = it->second;
                tex->bind();
            }
            GLfloat mat_amb[] = { 1, 1, 1, 1 };
            GLfloat mat_dif[] = { mat.kd[0], mat.kd[1], mat.kd[2], 1 };
            GLfloat mat_spec[] = { mat.ks[0], mat.ks[1], mat.ks[2], 1 };
            glMaterialfv(GL_FRONT, GL_AMBIENT, mat_amb);
            glMaterialfv(GL_FRONT, GL_DIFFUSE, mat_dif);
            glMaterialfv(GL_FRONT, GL_SPECULAR, mat_spec);
        } else {
            GLfloat mat_amb[] = { mat.ka[0], mat.ka[1], mat.ka[2], 1 };
            GLfloat mat_dif[] = { mat.kd[0], mat.kd[1], mat.kd[2], 1 };
            GLfloat mat_spec[] = { mat.ks[0], mat.ks[1], mat.ks[2], 1 };
            glMaterialfv(GL_FRONT, GL_AMBIENT, mat_amb);
            glMaterialfv(GL_FRONT, GL_DIFFUSE, mat_dif);
            glMaterialfv(GL_FRONT, GL_SPECULAR, mat_spec);
        }
        glMaterialf(GL_FRONT, GL_SHININESS, mat.ns);

        if (!glIsEnabled(GL_TEXTURE_2D)) glEnable(GL_TEXTURE_2D);

        // draws a single polygon
        glBegin(GL_POLYGON);
        for (size_t j = 0; j < f.vind.size(); ++j)
        {
            // vertex normal
            if (f.nind.size() == f.vind.size()) {
                const float *norm = o->get_normal(f.nind[j]);
                glNormal3fv(norm);
            }

            // vertex UV coordinate
            if (f.tex.size() > 0) {
                const float* tex = o->get_texture_indices(f.tex[j]);
                glTexCoord2fv(tex);
            }

            // vertex coordinates
            const float *vert = o->get_vertex(f.vind[j]);
            glVertex3fv(vert);
        }
        glEnd();
    }
    glDisable(GL_TEXTURE_2D);
    glEnable(GL_COLOR_MATERIAL);
}
