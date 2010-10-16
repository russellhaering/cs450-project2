#include <stdlib.h> /* Must come first to avoid redef error */

#ifdef __APPLE__
#include <GLUI/glui.h>
#else
#include <GL/glui.h>
#endif

#include "obj.h"

/** Constants and enums **/
enum buttonTypes {OBJ_TEXTFIELD = 0, LOAD_BUTTON};
enum colors { RED, GREEN, BLUE};
enum projections { ORTHO, PERSP, FOV};

const int WIN_WIDTH = 500;
const int WIN_HEIGHT = 500;


/** These are the live variables modified by the GUI ***/
int main_window;
int red = 255;
int green = 255;
int blue = 255;
int fov = 30;
int projType = ORTHO;

/** Globals **/
struct obj_data *data;
GLUI *glui;
GLUI_EditText *objFileNameTextField;


void projCB(int id)
{

}

void textCB(int id)
{
}
void buttonCB(int control)
{
}


void colorCB(int id)
{

}


void myGlutDisplay(void)
{

  glClear(GL_COLOR_BUFFER_BIT  | GL_DEPTH_BUFFER_BIT);
  //Use color as the color to use when shading, combined with light
  glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
  //Draw the scene here...you fill in the rest

}


void myGlutReshape(int x, int y)
{
  // Code here to create a reshape that avoids distortion.  This means
  // the AR of the view volume matches the AR of the viewport
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

}

int main(int argc, char **argv)
{
  data = load_obj_file(argv[1]);

   // setup glut
  glutInit(&argc, argv);
  glutInitDisplayMode(GLUT_RGB | GLUT_DOUBLE);
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
  glui = GLUI_Master.create_glui("OBJ Loader GUI", 0, 600, 50);

  GLUI_Panel *objPanel = glui->add_panel("Obj Files");
  objFileNameTextField = glui->add_edittext_to_panel(objPanel, "Filename:", GLUI_EDITTEXT_TEXT, 0, OBJ_TEXTFIELD, textCB);
  glui->add_button_to_panel(objPanel, "Load", LOAD_BUTTON, buttonCB);
  glui->add_separator();

  GLUI_Panel *projPanel = glui->add_panel("Projection");
  GLUI_RadioGroup *projGroup = glui->add_radiogroup_to_panel(projPanel, &projType, -1, projCB);
  glui->add_radiobutton_to_group(projGroup, "Orthographic");
  glui->add_radiobutton_to_group(projGroup, "Perspective");
  GLUI_Spinner *fovSpinner = glui->add_spinner_to_panel(projPanel, "FOV", GLUI_SPINNER_INT, &fov, FOV, projCB);
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


