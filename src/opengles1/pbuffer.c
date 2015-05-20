/*
 * Copyright (C) 2008  Tunsgten Graphics,Inc.   All Rights Reserved.
 */

/*
 * Test EGL Pbuffers
 * Brian Paul
 * August 2008
 */

/*
 * gcc -g pbuffer.c -o pbuffer `pkg-config --cflags --libs elementary`
 */

#include <assert.h>
#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <Elementary.h>
#include <Evas_GL.h>

#define EVAS_GL_API_USE(gl) \
   Evas_GL_API *__evas_gl_glapi = evas_gl_context_api_get(gl, evas_gl_context);

Evas_GL *evas_gl;
Evas_GL_Surface *evas_gl_surface;
Evas_GL_Surface *evas_gl_pbuffer_surface;
Evas_GL_Context *evas_gl_context;

static int WinWidth = 360, WinHeight = 480;

static GLfloat view_rotx = 0.0, view_roty = 0.0, view_rotz = 0.0;

typedef struct appdata {
   const char *name;

   Evas_Object *win;
} appdata_s;

static void
Normal(GLfloat *n, GLfloat nx, GLfloat ny, GLfloat nz)
{
   n[0] = nx;
   n[1] = ny;
   n[2] = nz;
}

static void
Vertex(GLfloat *v, GLfloat vx, GLfloat vy, GLfloat vz)
{
   v[0] = vx;
   v[1] = vy;
   v[2] = vz;
}

static void
Texcoord(GLfloat *v, GLfloat s, GLfloat t)
{
   v[0] = s;
   v[1] = t;
}


/* Borrowed from glut, adapted */
static void
draw_torus(GLfloat r, GLfloat R, GLint nsides, GLint rings)
{
   EVAS_GL_API_USE(evas_gl);
   int i, j;
   GLfloat theta, phi, theta1;
   GLfloat cosTheta, sinTheta;
   GLfloat cosTheta1, sinTheta1;
   GLfloat ringDelta, sideDelta;
   GLfloat varray[100][3], narray[100][3], tarray[100][2];
   int vcount;

   __evas_gl_glapi->glVertexPointer(3, GL_FLOAT, 0, varray);
   __evas_gl_glapi->glNormalPointer(GL_FLOAT, 0, narray);
   __evas_gl_glapi->glTexCoordPointer(2, GL_FLOAT, 0, tarray);
   __evas_gl_glapi->glEnableClientState(GL_VERTEX_ARRAY);
   __evas_gl_glapi->glEnableClientState(GL_NORMAL_ARRAY);
   __evas_gl_glapi->glEnableClientState(GL_TEXTURE_COORD_ARRAY);
   
   ringDelta = 2.0 * M_PI / rings;
   sideDelta = 2.0 * M_PI / nsides;

   theta = 0.0;
   cosTheta = 1.0;
   sinTheta = 0.0;
   for (i = rings - 1; i >= 0; i--) {
      theta1 = theta + ringDelta;
      cosTheta1 = cos(theta1);
      sinTheta1 = sin(theta1);

      vcount = 0; /* glBegin(GL_QUAD_STRIP); */

      phi = 0.0;
      for (j = nsides; j >= 0; j--) {
         GLfloat s0, s1, t;
         GLfloat cosPhi, sinPhi, dist;

         phi += sideDelta;
         cosPhi = cos(phi);
         sinPhi = sin(phi);
         dist = R + r * cosPhi;

         s0 = 20.0 * theta / (2.0 * M_PI);
         s1 = 20.0 * theta1 / (2.0 * M_PI);
         t = 8.0 * phi / (2.0 * M_PI);

         Normal(narray[vcount], cosTheta1 * cosPhi, -sinTheta1 * cosPhi, sinPhi);
         Texcoord(tarray[vcount], s1, t);
         Vertex(varray[vcount], cosTheta1 * dist, -sinTheta1 * dist, r * sinPhi);
         vcount++;

         Normal(narray[vcount], cosTheta * cosPhi, -sinTheta * cosPhi, sinPhi);
         Texcoord(tarray[vcount], s0, t);
         Vertex(varray[vcount], cosTheta * dist, -sinTheta * dist,  r * sinPhi);
         vcount++;
      }

      /*glEnd();*/
      assert(vcount <= 100);
      __evas_gl_glapi->glDrawArrays(GL_TRIANGLE_STRIP, 0, vcount);

      theta = theta1;
      cosTheta = cosTheta1;
      sinTheta = sinTheta1;
   }

   __evas_gl_glapi->glDisableClientState(GL_VERTEX_ARRAY);
   __evas_gl_glapi->glDisableClientState(GL_NORMAL_ARRAY);
   __evas_gl_glapi->glDisableClientState(GL_TEXTURE_COORD_ARRAY);
}


static void
draw(void)
{
   EVAS_GL_API_USE(evas_gl);
   __evas_gl_glapi->glClearColor(0.4, 0.4, 0.4, 1.0);
   __evas_gl_glapi->glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

   __evas_gl_glapi->glPushMatrix();
   __evas_gl_glapi->glRotatef(view_rotx, 1, 0, 0);
   __evas_gl_glapi->glRotatef(view_roty, 0, 1, 0);
   __evas_gl_glapi->glRotatef(view_rotz, 0, 0, 1);
   __evas_gl_glapi->glScalef(0.5, 0.5, 0.5);

   draw_torus(1.0, 3.0, 30, 60);

   __evas_gl_glapi->glPopMatrix();

   __evas_gl_glapi->glFinish();
}


/**
 * Draw to both the window and pbuffer and compare results.
 */
static void
draw_both()
{
   EVAS_GL_API_USE(evas_gl);
   unsigned *wbuf, *pbuf;
   int x = 100, y = 110;
   int i, dif;

   wbuf = (unsigned *) calloc(1, WinWidth * WinHeight * 4);
   pbuf = (unsigned *) calloc(1, WinWidth * WinHeight * 4);

   __evas_gl_glapi->glPixelStorei(GL_PACK_ALIGNMENT, 1);

   /* first draw to window */
   if (!evas_gl_make_current(evas_gl, evas_gl_surface, evas_gl_context)) {
      printf("Error: eglMakeCurrent(window) failed\n");
      return;
   }
   draw();
   __evas_gl_glapi->glReadPixels(0, 0, WinWidth, WinHeight, GL_RGBA, GL_UNSIGNED_BYTE, wbuf);
   printf("Window[%d,%d] = 0x%08x\n", x, y, wbuf[y*WinWidth+x]);

   /* then draw to pbuffer */
   if (!evas_gl_make_current(evas_gl, evas_gl_pbuffer_surface, evas_gl_context)) {
      printf("Error: eglMakeCurrent(pbuffer) failed\n");
      return;
   }

   draw();
   __evas_gl_glapi->glReadPixels(0, 0, WinWidth, WinHeight, GL_RGBA, GL_UNSIGNED_BYTE, pbuf);
   printf("Pbuffer[%d,%d] = 0x%08x\n", x, y, pbuf[y*WinWidth+x]);


   /* compare renderings */
   for (dif = i = 0; i < WinWidth * WinHeight; i++) {
      if (wbuf[i] != pbuf[i]) {
         dif = 1;
         break;
      }
   }

   if (dif)
      printf("Difference at %d: 0x%08x vs. 0x%08x\n", i, wbuf[i], pbuf[i]);
   else
      printf("Window rendering matches Pbuffer rendering!\n");

   free(wbuf);
   free(pbuf);

   view_rotx++;
   view_roty++;
   view_rotz++;
}

/* new window size or exposure */
static void
reshape(int width, int height)
{
   printf("reshape(w %d, h %d)\n", width, height);
   EVAS_GL_API_USE(evas_gl);
   GLfloat ar = (GLfloat) width / (GLfloat) height;

   WinWidth = width;
   WinHeight = height;

   __evas_gl_glapi->glViewport(0, 0, (GLint) width, (GLint) height);

   __evas_gl_glapi->glMatrixMode(GL_PROJECTION);
   __evas_gl_glapi->glLoadIdentity();

   __evas_gl_glapi->glFrustumf(-ar, ar, -1, 1, 5.0, 60.0);
   
   __evas_gl_glapi->glMatrixMode(GL_MODELVIEW);
   __evas_gl_glapi->glLoadIdentity();
   __evas_gl_glapi->glTranslatef(0.0, 0.0, -15.0);
}


static void
make_texture(void)
{
   EVAS_GL_API_USE(evas_gl);
#define SZ 64
   GLenum Filter = GL_LINEAR;
   GLubyte image[SZ][SZ][4];
   GLuint i, j;
   GLuint tex;

   for (i = 0; i < SZ; i++) {
      for (j = 0; j < SZ; j++) {
         GLfloat d = (i - SZ/2) * (i - SZ/2) + (j - SZ/2) * (j - SZ/2);
         d = sqrt(d);
         if (d < SZ/3) {
            image[i][j][0] = 255;
            image[i][j][1] = 255;
            image[i][j][2] = 255;
            image[i][j][3] = 255;
         }
         else {
            image[i][j][0] = 127;
            image[i][j][1] = 127;
            image[i][j][2] = 127;
            image[i][j][3] = 255;
         }
      }
   }

   __evas_gl_glapi->glActiveTexture(GL_TEXTURE0); /* unit 0 */
   __evas_gl_glapi->glGenTextures(1, &tex);
   __evas_gl_glapi->glBindTexture(GL_TEXTURE_2D, tex);
   __evas_gl_glapi->glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, SZ, SZ, 0,
                GL_RGBA, GL_UNSIGNED_BYTE, image);
   __evas_gl_glapi->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, Filter);
   __evas_gl_glapi->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, Filter);
   __evas_gl_glapi->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
   __evas_gl_glapi->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
#undef SZ
}



static void
init(void)
{
   EVAS_GL_API_USE(evas_gl);
   static const GLfloat red[4] = {1, 0, 0, 0};
   static const GLfloat white[4] = {1.0, 1.0, 1.0, 1.0};
   static const GLfloat diffuse[4] = {0.7, 0.7, 0.7, 1.0};
   static const GLfloat specular[4] = {0.001, 0.001, 0.001, 1.0};
   static const GLfloat pos[4] = {20, 20, 50, 1};

   __evas_gl_glapi->glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE, red);
   __evas_gl_glapi->glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, white);
   __evas_gl_glapi->glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, 9.0);

   __evas_gl_glapi->glEnable(GL_LIGHTING);
   __evas_gl_glapi->glEnable(GL_LIGHT0);
   __evas_gl_glapi->glLightfv(GL_LIGHT0, GL_POSITION, pos);
   __evas_gl_glapi->glLightfv(GL_LIGHT0, GL_DIFFUSE, diffuse);
   __evas_gl_glapi->glLightfv(GL_LIGHT0, GL_SPECULAR, specular);

   __evas_gl_glapi->glClearColor(0.4, 0.4, 0.4, 1.0);
   __evas_gl_glapi->glEnable(GL_DEPTH_TEST);

   make_texture();
   __evas_gl_glapi->glEnable(GL_TEXTURE_2D);
}



void on_pixels(void *data, Evas_Object *o)
{
   static int frame = 0;   

   evas_gl_make_current(evas_gl, evas_gl_surface, evas_gl_context);

   if (frame == 0)
   {
      init();
      reshape(WinWidth, WinHeight);
   }

   
   draw_both();

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

   reshape(w, h);
}

static Evas_Object* add_win(const char *name) {
   Evas_Object *win;

   elm_config_accel_preference_set("opengl:depth24");
   win = elm_win_util_standard_add(name, "EvasGL 1.1 example: Pbuffer");
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

   evas_gl_config = evas_gl_config_new();
   evas_gl_config->color_format = EVAS_GL_NO_FBO;
   evas_gl_config->depth_bits = EVAS_GL_DEPTH_BIT_8;
   evas_gl_config->stencil_bits = EVAS_GL_STENCIL_NONE;
   evas_gl_config->options_bits = EVAS_GL_OPTIONS_NONE;
   evas_gl_pbuffer_surface = evas_gl_pbuffer_surface_create(evas_gl, evas_gl_config, WinWidth, WinHeight, NULL);

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
