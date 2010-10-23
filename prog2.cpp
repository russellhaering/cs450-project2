#include <stdlib.h> /* Must come first to avoid redef error */
#include <stdio.h>

#ifdef __APPLE__
#include <GLUI/glui.h>
#else
#include <GL/glui.h>
#endif

#include "obj.h"

/** Constants and enums **/
enum buttonTypes {OBJ_TEXTFIELD = 0, LOAD_BUTTON};
enum colors {RED, GREEN, BLUE};
enum projections {ORTHO, PERSP, FOV};

#define WIN_WIDTH         500
#define WIN_HEIGHT        500

#define ORTHO_DENOMINATOR 100

/** These are the live variables modified by the GUI ***/
int main_window;
int red = 255;
int green = 255;
int blue = 255;
int fov = 90;
int win_x;
int win_y;
int projType = ORTHO;

/** Globals **/
struct obj_data *data = NULL;
GLUI *glui;
GLUI_EditText *objFileNameTextField;
GLUI_Spinner *fovSpinner;

void update_projection()
{
  float x_offset;
  float y_offset;
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();

  switch (projType) {
  case ORTHO:
    x_offset = ((float) win_x / ORTHO_DENOMINATOR);
    y_offset = ((float) win_y / ORTHO_DENOMINATOR);
    glOrtho(-x_offset, x_offset, -y_offset, y_offset, 4, 15);
    break;

  case PERSP:
    gluPerspective(fov, ((float) win_x / (float) win_y), 0.1, 0.5);
    gluLookAt(0., 0., 0.3, 0., 0.1, 0., 0., 1., 0.);
    break;

  case FOV:
    // This is used once for something apparently unrelated to the enum its in
    printf("Uh Oh\n");
    break;
  }
}

void projCB(int id)
{
  update_projection();
  glutPostRedisplay();
}

void textCB(int id)
{

}

void buttonCB(int control)
{
  struct obj_data *d, *curr;
  d = load_obj_file(objFileNameTextField->get_text());

  if (data == NULL) {
    data = d;
  }
  else {
    curr = data;
    while (curr->next != NULL) {
      curr = curr->next;
    }
    curr->next = d;
    fprintf(stderr, "Foobar\n");
  }
  glutPostRedisplay();
}

void colorCB(int id)
{

}

void myGlutDisplay(void)
{
  int i, j;
  struct face *f;
  struct obj_data *curr;
  struct vertex *v;
  struct vertex_normal *vn;
  glClear(GL_COLOR_BUFFER_BIT  | GL_DEPTH_BUFFER_BIT);
  //Use color as the color to use when shading, combined with light
  glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
  //Draw the scene here...you fill in the rest

  curr = data;
  while (curr != NULL) {
    // For now we are only supporting triangles
    glBegin(GL_TRIANGLES);

    for (i = 0; i < curr->faces->count; i++) {
      f = (struct face *) curr->faces->items[i];
      if (f->count != 3) {
        printf("Skipping non-triangle face at index %d\n", i);
        continue;
      }
      for (j = 0; j < f->count; j++) {
        // Set the normal if there is one
        if (f->vns != NULL) {
          vn = (struct vertex_normal *) f->vns[j];
          glNormal3f(vn->x, vn->y, vn->z);
        }

        // Set the vertex (there should always be one)
        v = (struct vertex *) f->vs[j];
        glVertex3f(v->x, v->y, v->z);
      }
    }

    printf("Rendered %d faces\n", i);
    glEnd();
    curr = curr->next;
  }
  glFlush();
  glutSwapBuffers();
}

void myGlutReshape (int x, int y)
{
  // Code here to create a reshape that avoids distortion.  This means
  // the AR of the view volume matches the AR of the viewport
  glViewport(0, 0, x, y);
  win_x = x;
  win_y = y;
  update_projection();
  glutPostRedisplay();
}



void myGlutMouse (int button, int button_state, int x, int y)
{


}

void initScene()
{
  // This stuff is for lighting and materials. We'll learn more about
  // it later.
  float light0_pos[] = {0.0, 3.0, 0.0, 1.0};
  float diffuse0[] = {1.0, 1.0, 1.0, 0.5};
  float ambient0[] = {0.1, 0.1, 0.1, 1.0};
  float specular0[] = {1.0, 1.0, 1.0, 0.5};

  glEnable(GL_LIGHTING);
  glEnable(GL_LIGHT0);
  glShadeModel(GL_SMOOTH);
  glEnable(GL_COLOR_MATERIAL);
  glLightfv(GL_LIGHT0, GL_POSITION, light0_pos);
  glLightfv(GL_LIGHT0, GL_DIFFUSE, diffuse0);
  glLightfv(GL_LIGHT0, GL_AMBIENT, ambient0);
  glLightfv(GL_LIGHT0, GL_SPECULAR, specular0);


  // You need to add the rest of the important GL state inits
  glEnable(GL_DEPTH_TEST);
}

int main(int argc, char **argv)
{
  // setup glut
  glutInit(&argc, argv);
  glutInitDisplayMode(GLUT_RGB | GLUT_DOUBLE | GLUT_DEPTH);
  glutInitWindowPosition(50, 50);
  glutInitWindowSize(WIN_WIDTH, WIN_HEIGHT);

  //You'll need a handle for your main window for GLUI
  main_window = glutCreateWindow("OBJ Loader");
  glutDisplayFunc(myGlutDisplay);
  glutReshapeFunc(myGlutReshape);
  glutMouseFunc(myGlutMouse);

  // Initialize my Scene
  initScene();

  //Build the GU
  glui = GLUI_Master.create_glui("OBJ Loader GUI", 0);

  GLUI_Panel *objPanel = glui->add_panel("Obj Files");
  objFileNameTextField = glui->add_edittext_to_panel(objPanel, "Filename:", GLUI_EDITTEXT_TEXT, 0, OBJ_TEXTFIELD, textCB);
  objFileNameTextField->set_text(argv[1]);
  glui->add_button_to_panel(objPanel, "Load", LOAD_BUTTON, buttonCB);
  glui->add_separator();

  GLUI_Panel *projPanel = glui->add_panel("Projection");
  GLUI_RadioGroup *projGroup = glui->add_radiogroup_to_panel(projPanel, &projType, -1, projCB);
  glui->add_radiobutton_to_group(projGroup, "Orthographic");
  glui->add_radiobutton_to_group(projGroup, "Perspective");
  GLUI_Spinner *fovSpinner =glui->add_spinner_to_panel(projPanel, "FOV", GLUI_SPINNER_INT, &fov, FOV, projCB);
  fovSpinner->set_int_limits(0, 90);

  GLUI_Panel *colorPanel = glui->add_panel("Color");
  /* These should be done with floats but the speed won't work */
  GLUI_Spinner *redValue = glui->add_spinner_to_panel(colorPanel, "Red", 2, &red, RED, colorCB);
  redValue->set_int_limits(0, 255);
  GLUI_Spinner *greenValue = glui->add_spinner_to_panel(colorPanel, "Green", 2, &green, GREEN, colorCB);
  greenValue->set_int_limits(0, 255);
  GLUI_Spinner *blueValue = glui->add_spinner_to_panel(colorPanel, "Blue", 2, &blue, BLUE, colorCB);
  blueValue->set_int_limits(0, 255);
  glui->set_main_gfx_window(main_window);

  // We register the idle callback with GLUI, *not* with GLUT
  //GLUI_Master.set_glutIdleFunc(myGlutIdle);
  GLUI_Master.set_glutIdleFunc(NULL);
  glutMainLoop();
  return EXIT_SUCCESS;
}
