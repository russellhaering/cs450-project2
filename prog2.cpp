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

#define SBUF_SIZE         64

#define MAXD              0xFFFFFFFF

#define MAX_COLOR         255
#define WIREFRAME_OFFSET  -10

#define CLIPPING_NEAR     0.1
#define CLIPPING_FAR      20

/** These are the live variables modified by the GUI ***/
int main_window;
int red = MAX_COLOR;
int green = MAX_COLOR;
int blue = MAX_COLOR;
int fov = 90;
int projType = ORTHO;

/** Globals **/
struct obj_data *data = NULL;
GLUI *glui;
GLUI_EditText *objFileNameTextField;
GLUI_Spinner *fovSpinner;
int selected = -1;


/**
 * update_projection - configure the specifed projection mode on the current
 * matrix (you probably want GL_PROJECTION).
 */
void update_projection()
{
  GLint viewport[4];
  float x_offset;
  float y_offset;

  // Load the viewport size
  glGetIntegerv(GL_VIEWPORT, viewport);

  switch (projType) {
  case ORTHO:
    // Configure orthographic projection
    x_offset = ((float) viewport[2] / ORTHO_DENOMINATOR);
    y_offset = ((float) viewport[3] / ORTHO_DENOMINATOR);
    glOrtho(-x_offset, x_offset, -y_offset, y_offset, CLIPPING_NEAR, CLIPPING_FAR);
    break;

  case PERSP:
    // Configure perspective projection
    gluPerspective(fov, ((float) viewport[2] / (float) viewport[3]), CLIPPING_NEAR, CLIPPING_FAR);
    gluLookAt(0., 0., 0.3, 0., 0.1, 0., 0., 1., 0.);
    break;

  case FOV:
    // This is used once for something apparently unrelated to the enum its in

  default:
    printf("Uh Oh\n");
  }
}

/**
 * projection_callback - called when projection (or color, for soem reason)
 * parameters are modified, updates the projection mode and fires a redisplay
 */
void projection_callback(int id)
{
  // Set up the projection matrix
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  update_projection();

  // Set up the modelview matrix
  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();

  // Redisplay
  glutPostRedisplay();
}

/**
 * text_callback - fired when the filename text is modified, not much reason
 * to do anything in here.
 */
void text_callback(int id)
{
}

/**
 * button_callback - fired when the load file button is pressed, loads the
 * currently specified file and fires a redisplay
 */
void button_callback(int control)
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
  }
  glutPostRedisplay();
}

/**
 * color_callback - fired when the color parameters change, but so is
 * the projection callback. This doesn't do anything to avoid a double
 * redisplay.
 */
void color_callback(int id)
{
}

/**
 * draw_object - draw the specified object in the current polygon mode
 */
void draw_object(struct obj_data *curr)
{
    int i, j;
    struct face *f;
    struct vertex *v;
    struct vertex_normal *vn;

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
}

/**
 * draw_objects - draw all loaded objects, with a wireframe and color on the
 * selected object.
 */
void draw_objects(void)
{
  int i;
  struct obj_data *curr;

  curr = data;
  i = 0;
  while (curr != NULL) {
    glPushName(i);

    // Set the color - white for non-selected, colored for selected
    if (i == selected) {
      glColor3f(((float) red / MAX_COLOR), ((float) green / MAX_COLOR), ((float) blue / MAX_COLOR));
    }
    else {
      glColor3f(1, 1, 1);
    }

    // Draw the object
    draw_object(curr);

    // Draw the wireframe on selected objects
    if (i == selected) {
      glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
      glEnable(GL_POLYGON_OFFSET_LINE);
      glPolygonOffset(0, WIREFRAME_OFFSET);
      glColor3f(1, 0, 0);
      draw_object(curr);
      glPolygonOffset(0, 0);
      glDisable(GL_POLYGON_OFFSET_LINE);
      glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    }

    glPopName();

    curr = curr->next;
    i++;
  }
  printf("Rendered %d objects\n", i);
}

/**
 * display_callback - the display callback, called to draw the scene. Clears
 * the scene, draws the objects then flushes and swaps the bufers.
 */
void display_callback()
{
  glClear(GL_COLOR_BUFFER_BIT  | GL_DEPTH_BUFFER_BIT);
  glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
  draw_objects();
  glFlush();
  glutSwapBuffers();
}

/**
 * reshape_callback - called when the window is reshaped, updates the viewport
 * and projection then redraws the scene.
 */
void reshape_callback (int x, int y)
{
  // Update the viewport
  glViewport(0, 0, x, y);

  // Set up the projection matrix
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  update_projection();

  // Set up the modelview matrix
  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();
}

/**
 * mouse_callback - called on mouse clicks, updates the global 'selected'
 * ID to reflect which object was clicked, then redraws the scene.
 */
void mouse_calback (int button, int state, int x, int y)
{
  int hits, i, names;
  GLuint selectBuf[SBUF_SIZE] = {0};
  GLuint min, *cur;
  GLint viewport[4];

  if (button == GLUT_LEFT_BUTTON && state == GLUT_DOWN) {
    // Get the viewport parameters
    glGetIntegerv(GL_VIEWPORT, viewport);
    glSelectBuffer(SBUF_SIZE, selectBuf);

    // Go into selection mode and set up the projection and modeliew matrices
    glRenderMode(GL_SELECT);
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();

    gluPickMatrix(x, (viewport[3] - y), 2, 2, viewport);
    update_projection();

    glMatrixMode(GL_MODELVIEW);

    // Set up the names stack then draw the scene
    glInitNames();
    draw_objects();

    // Restore render mode and associated matrices
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
    glFlush();

    hits = glRenderMode(GL_RENDER);

    printf("%d hits\n", hits);

    min = MAXD;
    cur = selectBuf;

    // Find which object was hit if one was
    if (hits > 0) {
      for (i = 0; i < hits; i++) {
        names = *cur;

        if (*(cur + 1) < min) {
          min = *(cur + 1);
          if (names > 0) {
            selected = *(cur + 3);
          }
        }

        cur += (3 + names);
      }
    }
    // Otherwise no objects are selected
    else {
      selected = -1;
    }

    // Redraw the scene to update color/wireframe
    glutPostRedisplay();
  }
}

void init_scene()
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
  glutDisplayFunc(display_callback);
  glutReshapeFunc(reshape_callback);
  glutMouseFunc(mouse_calback);

  // Initialize my Scene
  init_scene();

  //Build the GU
  glui = GLUI_Master.create_glui("OBJ Loader GUI", 0);

  GLUI_Panel *objPanel = glui->add_panel("Obj Files");
  objFileNameTextField = glui->add_edittext_to_panel(objPanel, "Filename:", GLUI_EDITTEXT_TEXT, 0, OBJ_TEXTFIELD, text_callback);
  objFileNameTextField->set_text(argv[1]);
  glui->add_button_to_panel(objPanel, "Load", LOAD_BUTTON, button_callback);
  glui->add_separator();

  GLUI_Panel *projPanel = glui->add_panel("Projection");
  GLUI_RadioGroup *projGroup = glui->add_radiogroup_to_panel(projPanel, &projType, -1, projection_callback);
  glui->add_radiobutton_to_group(projGroup, "Orthographic");
  glui->add_radiobutton_to_group(projGroup, "Perspective");
  GLUI_Spinner *fovSpinner =glui->add_spinner_to_panel(projPanel, "FOV", GLUI_SPINNER_INT, &fov, FOV, projection_callback);
  fovSpinner->set_int_limits(0, 90);

  GLUI_Panel *colorPanel = glui->add_panel("Color");
  /* These should be done with floats but the speed won't work */
  GLUI_Spinner *redValue = glui->add_spinner_to_panel(colorPanel, "Red", 2, &red, RED, color_callback);
  redValue->set_int_limits(0, MAX_COLOR);
  GLUI_Spinner *greenValue = glui->add_spinner_to_panel(colorPanel, "Green", 2, &green, GREEN, color_callback);
  greenValue->set_int_limits(0, MAX_COLOR);
  GLUI_Spinner *blueValue = glui->add_spinner_to_panel(colorPanel, "Blue", 2, &blue, BLUE, color_callback);
  blueValue->set_int_limits(0, MAX_COLOR);
  glui->set_main_gfx_window(main_window);

  // We register the idle callback with GLUI, *not* with GLUT
  //GLUI_Master.set_glutIdleFunc(myGlutIdle);
  GLUI_Master.set_glutIdleFunc(NULL);
  glutMainLoop();
  return EXIT_SUCCESS;
}
