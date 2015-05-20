/*
 * Copyright (C) 2009 Chia-I Wu <olv@0xlab.org>
 *
 * Based on eglgears by
 * Copyright (C) 1999-2001  Brian Paul   All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * BRIAN PAUL BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
 * AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <assert.h>
#include <Elementary.h>
#include <Evas_GL.h>

#define EVAS_GL_API_USE(gl) \
   Evas_GL_API *__evas_gl_glapi = evas_gl_context_api_get(gl, evas_gl_context);

Evas_GL *evas_gl;
Evas_GL_Surface *evas_gl_surface;
Evas_GL_Context *evas_gl_context;

static int WinWidth = 300, WinHeight = 300;

typedef struct appdata {
   const char *name;

   Evas_Object *win;
} appdata_s;

#ifndef M_PI
#define M_PI 3.14159265
#endif


struct gear {
   GLuint vbo;
   GLfloat *vertices;
   GLsizei stride;

   GLint num_teeth;
};

static GLfloat view_rotx = 20.0, view_roty = 30.0, view_rotz = 0.0;
static struct gear gears[3];
static GLfloat angle = 0.0;

/*
 *  Initialize a gear wheel.
 *
 *  Input:  gear - gear to initialize
 *          inner_radius - radius of hole at center
 *          outer_radius - radius at center of teeth
 *          width - width of gear
 *          teeth - number of teeth
 *          tooth_depth - depth of tooth
 */
static void
init_gear(struct gear *gear, GLfloat inner_radius, GLfloat outer_radius,
          GLfloat width, GLint teeth, GLfloat tooth_depth)
{
   EVAS_GL_API_USE(evas_gl);
   GLfloat r0, r1, r2;
   GLfloat a0, da;
   GLint verts_per_tooth, total_verts, total_size;
   GLint count, i;
   GLfloat *verts;

   r0 = inner_radius;
   r1 = outer_radius - tooth_depth / 2.0;
   r2 = outer_radius + tooth_depth / 2.0;

   a0 = 2.0 * M_PI / teeth;
   da = a0 / 4.0;

   gear->vbo = 0;
   gear->vertices = NULL;
   gear->stride = sizeof(GLfloat) * 6; /* XYZ + normal */
   gear->num_teeth = teeth;

   verts_per_tooth = 10 + 4;
   total_verts = teeth * verts_per_tooth;
   total_size = total_verts * gear->stride;

   verts = malloc(total_size);
   if (!verts) {
      printf("failed to allocate vertices\n");
      return;
   }

#define GEAR_VERT(r, n, sign)                      \
   do {                                            \
      verts[count * 6 + 0] = (r) * vx[n];          \
      verts[count * 6 + 1] = (r) * vy[n];          \
      verts[count * 6 + 2] = (sign) * width * 0.5; \
      verts[count * 6 + 3] = normal[0];            \
      verts[count * 6 + 4] = normal[1];            \
      verts[count * 6 + 5] = normal[2];            \
      count++;                                     \
   } while (0)

   count = 0;
   for (i = 0; i < teeth; i++) {
      GLfloat normal[3];
      GLfloat vx[5], vy[5];
      GLfloat u, v;

      normal[0] = 0.0;
      normal[1] = 0.0;
      normal[2] = 0.0;

      vx[0] = cos(i * a0 + 0 * da);
      vy[0] = sin(i * a0 + 0 * da);
      vx[1] = cos(i * a0 + 1 * da);
      vy[1] = sin(i * a0 + 1 * da);
      vx[2] = cos(i * a0 + 2 * da);
      vy[2] = sin(i * a0 + 2 * da);
      vx[3] = cos(i * a0 + 3 * da);
      vy[3] = sin(i * a0 + 3 * da);
      vx[4] = cos(i * a0 + 4 * da);
      vy[4] = sin(i * a0 + 4 * da);

      /* outward faces of a tooth, 10 verts */
      normal[0] = vx[0];
      normal[1] = vy[0];
      GEAR_VERT(r1, 0,  1);
      GEAR_VERT(r1, 0, -1);

      u = r2 * vx[1] - r1 * vx[0];
      v = r2 * vy[1] - r1 * vy[0];
      normal[0] = v;
      normal[1] = -u;
      GEAR_VERT(r2, 1,  1);
      GEAR_VERT(r2, 1, -1);

      normal[0] = vx[0];
      normal[1] = vy[0];
      GEAR_VERT(r2, 2,  1);
      GEAR_VERT(r2, 2, -1);

      u = r1 * vx[3] - r2 * vx[2];
      v = r1 * vy[3] - r2 * vy[2];
      normal[0] = v;
      normal[1] = -u;
      GEAR_VERT(r1, 3,  1);
      GEAR_VERT(r1, 3, -1);

      normal[0] = vx[0];
      normal[1] = vy[0];
      GEAR_VERT(r1, 4,  1);
      GEAR_VERT(r1, 4, -1);

      /* inside radius cylinder, 4 verts */
      normal[0] = -vx[4];
      normal[1] = -vy[4];
      GEAR_VERT(r0, 4,  1);
      GEAR_VERT(r0, 4, -1);

      normal[0] = -vx[0];
      normal[1] = -vy[0];
      GEAR_VERT(r0, 0,  1);
      GEAR_VERT(r0, 0, -1);

      assert(count % verts_per_tooth == 0);
   }
   assert(count == total_verts);
#undef GEAR_VERT

   gear->vertices = verts;

   /* setup VBO */
   __evas_gl_glapi->glGenBuffers(1, &gear->vbo);
   if (gear->vbo) {
      __evas_gl_glapi->glBindBuffer(GL_ARRAY_BUFFER, gear->vbo);
      __evas_gl_glapi->glBufferData(GL_ARRAY_BUFFER, total_size, verts, GL_STATIC_DRAW);
   }
}


static void
draw_gear(const struct gear *gear)
{
   EVAS_GL_API_USE(evas_gl);
   GLint i;

   if (!gear->vbo && !gear->vertices) {
      printf("nothing to be drawn\n");
      return;
   }

   if (gear->vbo) {
      __evas_gl_glapi->glBindBuffer(GL_ARRAY_BUFFER, gear->vbo);
      __evas_gl_glapi->glVertexPointer(3, GL_FLOAT, gear->stride, (const GLvoid *) 0);
      __evas_gl_glapi->glNormalPointer(GL_FLOAT, gear->stride, (const GLvoid *) (sizeof(GLfloat) * 3));
   } else {
      __evas_gl_glapi->glBindBuffer(GL_ARRAY_BUFFER, 0);
      __evas_gl_glapi->glVertexPointer(3, GL_FLOAT, gear->stride, gear->vertices);
      __evas_gl_glapi->glNormalPointer(GL_FLOAT, gear->stride, gear->vertices + 3);
   }

   __evas_gl_glapi->glEnableClientState(GL_VERTEX_ARRAY);

   for (i = 0; i < gear->num_teeth; i++) {
      const GLint base = (10 + 4) * i;
      GLushort indices[7];

      __evas_gl_glapi->glShadeModel(GL_FLAT);

      /* front face */
      indices[0] = base + 12;
      indices[1] = base +  0;
      indices[2] = base +  2;
      indices[3] = base +  4;
      indices[4] = base +  6;
      indices[5] = base +  8;
      indices[6] = base + 10;

      __evas_gl_glapi->glNormal3f(0.0, 0.0, 1.0);
      __evas_gl_glapi->glDrawElements(GL_TRIANGLE_FAN, 7, GL_UNSIGNED_SHORT, indices);

      /* back face */
      indices[0] = base + 13;
      indices[1] = base + 11;
      indices[2] = base +  9;
      indices[3] = base +  7;
      indices[4] = base +  5;
      indices[5] = base +  3;
      indices[6] = base +  1;

      __evas_gl_glapi->glNormal3f(0.0, 0.0, -1.0);
      __evas_gl_glapi->glDrawElements(GL_TRIANGLE_FAN, 7, GL_UNSIGNED_SHORT, indices);

      __evas_gl_glapi->glEnableClientState(GL_NORMAL_ARRAY);

      /* outward face of a tooth */
      __evas_gl_glapi->glDrawArrays(GL_TRIANGLE_STRIP, base, 10);

      /* inside radius cylinder */
      __evas_gl_glapi->glShadeModel(GL_SMOOTH);
      __evas_gl_glapi->glDrawArrays(GL_TRIANGLE_STRIP, base + 10, 4);

      __evas_gl_glapi->glDisableClientState(GL_NORMAL_ARRAY);
   }

   __evas_gl_glapi->glDisableClientState(GL_VERTEX_ARRAY);
}


static void
gears_draw(void)
{
   EVAS_GL_API_USE(evas_gl);
   static const GLfloat red[4] = { 0.8, 0.1, 0.0, 1.0 };
   static const GLfloat green[4] = { 0.0, 0.8, 0.2, 1.0 };
   static const GLfloat blue[4] = { 0.2, 0.2, 1.0, 1.0 };

   __evas_gl_glapi->glClearColor(0,0,0,1);
   __evas_gl_glapi->glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

   __evas_gl_glapi->glPushMatrix();
   __evas_gl_glapi->glRotatef(view_rotx, 1.0, 0.0, 0.0);
   __evas_gl_glapi->glRotatef(view_roty, 0.0, 1.0, 0.0);
   __evas_gl_glapi->glRotatef(view_rotz, 0.0, 0.0, 1.0);

   __evas_gl_glapi->glPushMatrix();
   __evas_gl_glapi->glTranslatef(-3.0, -2.0, 0.0);
   __evas_gl_glapi->glRotatef(angle, 0.0, 0.0, 1.0);

   __evas_gl_glapi->glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE, red);
   draw_gear(&gears[0]);

   __evas_gl_glapi->glPopMatrix();

   __evas_gl_glapi->glPushMatrix();
   __evas_gl_glapi->glTranslatef(3.1, -2.0, 0.0);
   __evas_gl_glapi->glRotatef(-2.0 * angle - 9.0, 0.0, 0.0, 1.0);

   __evas_gl_glapi->glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE, green);
   draw_gear(&gears[1]);

   __evas_gl_glapi->glPopMatrix();

   __evas_gl_glapi->glPushMatrix();
   __evas_gl_glapi->glTranslatef(-3.1, 4.2, 0.0);
   __evas_gl_glapi->glRotatef(-2.0 * angle - 25.0, 0.0, 0.0, 1.0);

   __evas_gl_glapi->glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE, blue);
   draw_gear(&gears[2]);

   __evas_gl_glapi->glPopMatrix();

   __evas_gl_glapi->glPopMatrix();
}


static void gears_fini(void)
{
   EVAS_GL_API_USE(evas_gl);
   GLint i;
   for (i = 0; i < 3; i++) {
      struct gear *gear = &gears[i];
      if (gear->vbo) {
         __evas_gl_glapi->glDeleteBuffers(1, &gear->vbo);
         gear->vbo = 0;
      }
      if (gear->vertices) {
         free(gear->vertices);
         gear->vertices = NULL;
      }
   }
}


static void gears_init(void)
{
   EVAS_GL_API_USE(evas_gl);
   static const GLfloat pos[4] = { 5.0, 5.0, 10.0, 0.0 };

   __evas_gl_glapi->glLightfv(GL_LIGHT0, GL_POSITION, pos);
   __evas_gl_glapi->glEnable(GL_CULL_FACE);
   __evas_gl_glapi->glEnable(GL_LIGHTING);
   __evas_gl_glapi->glEnable(GL_LIGHT0);
   __evas_gl_glapi->glEnable(GL_DEPTH_TEST);
   __evas_gl_glapi->glEnable(GL_NORMALIZE);

   init_gear(&gears[0], 1.0, 4.0, 1.0, 20, 0.7);
   init_gear(&gears[1], 0.5, 2.0, 2.0, 10, 0.7);
   init_gear(&gears[2], 1.3, 2.0, 0.5, 10, 0.7);
}


/* new window size or exposure */
static void
gears_reshape(int width, int height)
{
   EVAS_GL_API_USE(evas_gl);
   GLfloat h = (GLfloat) height / (GLfloat) width;

   __evas_gl_glapi->glViewport(0, 0, (GLint) width, (GLint) height);

   __evas_gl_glapi->glMatrixMode(GL_PROJECTION);
   __evas_gl_glapi->glLoadIdentity();
   __evas_gl_glapi->glFrustumf(-1.0, 1.0, -h, h, 5.0, 60.0);

   __evas_gl_glapi->glMatrixMode(GL_MODELVIEW);
   __evas_gl_glapi->glLoadIdentity();
   __evas_gl_glapi->glTranslatef(0.0, 0.0, -40.0);
}


static void
gears_idle(void)
{
  static double t0 = -1.;
  static int frame = 0;
  double dt, t = frame / 1000.0;
  if (t0 < 0.0)
    t0 = t;
  dt = t - t0;
  t0 = t;

  angle += 70.0 * dt;  /* 70 degrees per second */
  angle = fmod(angle, 360.0); /* prevents eventual overflow */
  frame+=10;
}

void on_pixels(void *data, Evas_Object *o)
{
   static int frame = 0;   

   evas_gl_make_current(evas_gl, evas_gl_surface, evas_gl_context);

   if (frame == 0)
   {
      gears_init();
      gears_reshape(WinWidth, WinHeight);
   }

   gears_idle();
   gears_draw();

   frame++;
}

static Eina_Bool anim(void *data) {
   evas_object_image_pixels_dirty_set(data, EINA_TRUE);
   return EINA_TRUE;
}

static void del_anim(void *data, Evas *evas, Evas_Object *obj, void *event_info)
{
   Ecore_Animator *ani = evas_object_data_get(obj, "ani");
   ecore_animator_del(ani);
}

static void
_win_resize_cb(void *data, Evas *e EINA_UNUSED, Evas_Object *obj, void *event_info EINA_UNUSED)
{
   float aspect;
   Evas_Coord w,h;
   evas_object_geometry_get( obj, NULL, NULL, &w, &h);

   gears_reshape(w, h);
}

static Evas_Object* add_win(const char *name) {
   Evas_Object *win;

   elm_config_accel_preference_set("opengl:depth24");
   win = elm_win_util_standard_add(name, "EvasGL 1.1 example: Gears");
   evas_object_resize(win, WinWidth, WinHeight);

   if (!win)
      return NULL;

   if (elm_win_wm_rotation_supported_get(win)) {
      int rots[4] = { 0, 90, 180, 270 };
      elm_win_wm_rotation_available_rotations_set(win, rots, 4);
   }

   evas_object_show(win);

   return win;
}

static Eina_Bool app_create(void *data) {
   /* Hook to take necessary actions before main event loop starts
    * Initialize UI resources and application's data
    * If this function returns true, the main loop of application starts
    * If this function returns false, the application is terminated. */

   Evas *e;
   Evas_Object *gl;
   
   Evas_GL_Config *evas_gl_config;
   Ecore_Animator *ani;
   appdata_s *ad = data;

   if (!data)
      return EINA_FALSE;

   /* Create the window */
   ad->win = add_win(ad->name);

   if (!ad->win)
      return EINA_FALSE;

   e = evas_object_evas_get(ad->win);
   gl = evas_object_image_filled_add(e);
   evas_object_image_size_set(gl, WinWidth, WinHeight);
   evas_object_image_alpha_set(gl, EINA_FALSE);
   evas_object_size_hint_align_set(gl, EVAS_HINT_FILL, EVAS_HINT_FILL);
   evas_object_size_hint_weight_set(gl, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_resize(gl, WinWidth, WinHeight);
   evas_object_show(gl);

   evas_gl = evas_gl_new(e);
   evas_gl_config = evas_gl_config_new();
   evas_gl_config->color_format = EVAS_GL_RGBA_8888;
   evas_gl_config->depth_bits = EVAS_GL_DEPTH_BIT_8;
   evas_gl_config->stencil_bits = EVAS_GL_STENCIL_NONE;
   evas_gl_config->options_bits = EVAS_GL_OPTIONS_DIRECT;
   evas_gl_surface = evas_gl_surface_create(evas_gl, evas_gl_config, WinWidth, WinHeight);
   evas_gl_context = evas_gl_context_version_create(evas_gl, NULL, EVAS_GL_GLES_1_X);
   evas_gl_config_free(evas_gl_config);

   Evas_Native_Surface ns;
   evas_gl_native_surface_get(evas_gl, evas_gl_surface, &ns);
   evas_object_image_native_surface_set(gl, &ns);
   evas_object_image_pixels_get_callback_set(gl, on_pixels, NULL);

   evas_object_show(gl);

   /* This adds an animator so that the app will regularly
    * trigger updates of the GLView using elm_glview_changed_set().
    *
    * NOTE: If you delete GL, this animator will keep running trying to access
    * GL so this animator needs to be deleted with ecore_animator_del().
    */
   ani = ecore_animator_add(anim, gl);
   evas_object_data_set(gl, "ani", ani);
   evas_object_data_set(gl, "ad", ad);
   evas_object_event_callback_add(gl, EVAS_CALLBACK_DEL, del_anim, gl);
   evas_object_event_callback_add(gl, EVAS_CALLBACK_RESIZE, _win_resize_cb, NULL);

   evas_object_show(ad->win);

   /* Return true: the main loop will now start running */
   return EINA_TRUE;
}

EAPI_MAIN int
elm_main(int argc, char **argv)
{
   appdata_s ad = {0,};
   app_create(&ad);

   elm_run();
   elm_shutdown();
   return 0;
}
ELM_MAIN()
