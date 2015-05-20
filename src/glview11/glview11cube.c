/*
 * Copyright (c) 2013 Samsung Electronics Co., Ltd All Rights Reserved
 *
 * Licensed under the Apache License, Version 2.0 (the License);
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an AS IS BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <math.h>
#include <Evas_GL.h>
#include <Elementary.h>

#define APPDATA_KEY "AppData"

#define ONEP  +1.0
#define ONEN  -1.0
#define ZERO   0.0

#define ELEMENTARY_GLVIEW_USE(glview) \
   Evas_GL_API *__evas_gl_glapi = elm_glview_gl_api_get(glview);

#define Z_POS_INC 0.01f

#define S(a) evas_object_show(a)

#define SX(a) do { \
   evas_object_size_hint_align_set(a, EVAS_HINT_FILL, EVAS_HINT_FILL); \
   evas_object_size_hint_weight_set(a, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND); \
   evas_object_show(a); \
   } while (0)

#define SF(a) do { \
   evas_object_size_hint_align_set(a, EVAS_HINT_FILL, EVAS_HINT_FILL); \
   evas_object_size_hint_weight_set(a, 0.00001, 0.00001); \
   evas_object_show(a); \
   } while (0)

struct _appdata_s
{
   Evas_Object *table, *bg, *img;
   Evas_Object *win;
   Evas_Object *glview;
   Ecore_Animator *anim;
   Evas_Object *conform;

   GLuint tex_ids[2];
   int current_tex_index;
};
typedef struct _appdata_s appdata_s;

extern const unsigned short IMAGE_565_128_128_1[];
extern const unsigned short IMAGE_4444_128_128_1[];

static void
set_perspective(Evas_Object *obj, float fovDegree, int w, int h, float zNear,  float zFar)
{
   float aspect;
   float fxdYMax, fxdYMin, fxdXMax, fxdXMin;
   int degree;

   ELEMENTARY_GLVIEW_USE(obj);

   if((w == 0) || (h == 0)) return;
   aspect = (w > h)? ((float)w / h): ((float)h / w);

   /* tan(double(degree) * 3.1415962 / 180.0 / 2.0); */
   static const float HALF_TAN_TABLE[91] =
   {
      0.00000f, 0.00873f, 0.01746f, 0.02619f, 0.03492f, 0.04366f, 0.05241f, 0.06116f, 0.06993f,
      0.07870f, 0.08749f, 0.09629f, 0.10510f, 0.11394f, 0.12278f, 0.13165f, 0.14054f, 0.14945f,
      0.15838f, 0.16734f, 0.17633f, 0.18534f, 0.19438f, 0.20345f, 0.21256f, 0.22169f, 0.23087f,
      0.24008f, 0.24933f, 0.25862f, 0.26795f, 0.27732f, 0.28675f, 0.29621f, 0.30573f, 0.31530f,
      0.32492f, 0.33460f, 0.34433f, 0.35412f, 0.36397f, 0.37389f, 0.38386f, 0.39391f, 0.40403f,
      0.41421f, 0.42448f, 0.43481f, 0.44523f, 0.45573f, 0.46631f, 0.47698f, 0.48773f, 0.49858f,
      0.50953f, 0.52057f, 0.53171f, 0.54296f, 0.55431f, 0.56577f, 0.57735f, 0.58905f, 0.60086f,
      0.61280f, 0.62487f, 0.63707f, 0.64941f, 0.66189f, 0.67451f, 0.68728f, 0.70021f, 0.71329f,
      0.72654f, 0.73996f, 0.75356f, 0.76733f, 0.78129f, 0.79544f, 0.80979f, 0.82434f, 0.83910f,
      0.85408f, 0.86929f, 0.88473f, 0.90041f, 0.91633f, 0.93252f, 0.94897f, 0.96569f, 0.98270f,
      1.00000f
   };

   degree = (int)(fovDegree + 0.5f);
   degree = (degree >=  0) ? degree :  0;
   degree = (degree <= 90) ? degree : 90;

   fxdYMax = zNear * HALF_TAN_TABLE[degree];
   fxdYMin = -fxdYMax;

   fxdXMax = fxdYMax * aspect;
   fxdXMin = -fxdXMax;

   __evas_gl_glapi->glViewport(0, 0, w, h);
   __evas_gl_glapi->glMatrixMode(GL_PROJECTION);
   __evas_gl_glapi->glLoadIdentity();
   __evas_gl_glapi->glFrustumf(fxdXMin, fxdXMax, fxdYMin, fxdYMax, zNear, zFar);
}

void
init_gles(Evas_Object *obj)
{
   int w, h;
   appdata_s *ad;
printf("%s\n", __func__);
   ELEMENTARY_GLVIEW_USE(obj);
   ad = evas_object_data_get(obj, APPDATA_KEY);

   fprintf(stderr, "Extension: %s\n", evas_gl_string_query(elm_glview_evas_gl_get(obj), EVAS_GL_EXTENSIONS));
   __evas_gl_glapi->glGenTextures(2, ad->tex_ids);

   /* Create and map texture 1 */
   __evas_gl_glapi->glBindTexture(GL_TEXTURE_2D, ad->tex_ids[0]);
   __evas_gl_glapi->glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 128, 128, 0, GL_RGBA, GL_UNSIGNED_SHORT_4_4_4_4, IMAGE_4444_128_128_1);
   __evas_gl_glapi->glTexParameterx(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
   __evas_gl_glapi->glTexParameterx(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
   __evas_gl_glapi->glTexParameterx(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
   __evas_gl_glapi->glTexParameterx(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

   /* Create and map texture 2 */
   __evas_gl_glapi->glBindTexture(GL_TEXTURE_2D, ad->tex_ids[1]);
   __evas_gl_glapi->glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 128, 128, 0, GL_RGB, GL_UNSIGNED_SHORT_5_6_5, IMAGE_565_128_128_1);
   __evas_gl_glapi->glTexParameterx(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
   __evas_gl_glapi->glTexParameterx(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
   __evas_gl_glapi->glTexParameterx(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
   __evas_gl_glapi->glTexParameterx(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

   ad->current_tex_index = 0;

   __evas_gl_glapi->glShadeModel(GL_SMOOTH);

   __evas_gl_glapi->glEnable(GL_CULL_FACE);
   __evas_gl_glapi->glCullFace(GL_BACK);

   __evas_gl_glapi->glEnable(GL_DEPTH_TEST);
   __evas_gl_glapi->glDepthFunc(GL_LESS);

   elm_glview_size_get(obj, &w, &h);
   set_perspective(obj, 60.0f, w, h, 1.0f, 400.0f);
}

void
destroy_gles(Evas_Object *obj)
{
   appdata_s *ad;

   ELEMENTARY_GLVIEW_USE(obj);
   ad = evas_object_data_get(obj, APPDATA_KEY);

   if (ad->tex_ids[0])
   {
      __evas_gl_glapi->glDeleteTextures(1, &(ad->tex_ids[0]));
      ad->tex_ids[0] = 0;
   }

   if (ad->tex_ids[1])
   {
      __evas_gl_glapi->glDeleteTextures(1, &(ad->tex_ids[1]));
      ad->tex_ids[1] = 0;
   }
}

void
resize_gl(Evas_Object *obj)
{
   int w, h;
   elm_glview_size_get(obj, &w, &h);
   set_perspective(obj, 60.0f, w, h, 1.0f, 400.0f);
   printf("%s (w %d, h %d)\n", __func__, w, h);
}

static void
draw_cube1(Evas_Object *obj)
{
   static int angle = 0;

   ELEMENTARY_GLVIEW_USE(obj);

   static const float VERTICES[] =
   {
      ONEN, ONEP, ONEN, // 0
      ONEP, ONEP, ONEN, // 1
      ONEN, ONEN, ONEN, // 2
      ONEP, ONEN, ONEN, // 3
      ONEN, ONEP, ONEP, // 4
      ONEP, ONEP, ONEP, // 5
      ONEN, ONEN, ONEP, // 6
      ONEP, ONEN, ONEP  // 7
   };

   static const float VERTEX_COLOR[] =
   {
      ONEP, ZERO, ONEP, ONEP,
      ONEP, ONEP, ZERO, ONEP,
      ZERO, ONEP, ONEP, ONEP,
      ONEP, ZERO, ZERO, ONEP,
      ZERO, ZERO, ONEP, ONEP,
      ZERO, ONEP, ZERO, ONEP,
      ONEP, ONEP, ONEP, ONEP,
      ZERO, ZERO, ZERO, ONEP
   };

   static const unsigned short INDEX_BUFFER[] =
   {
      0, 1, 2, 2, 1, 3,
      1, 5, 3, 3, 5, 7,
      5, 4, 7, 7, 4, 6,
      4, 0, 6, 6, 0, 2,
      4, 5, 0, 0, 5, 1,
      2, 3, 6, 6, 3, 7
   };

   __evas_gl_glapi->glEnableClientState(GL_VERTEX_ARRAY);
   __evas_gl_glapi->glVertexPointer(3, GL_FLOAT, 0, VERTICES);

   __evas_gl_glapi->glEnableClientState(GL_COLOR_ARRAY);
   __evas_gl_glapi->glColorPointer(4, GL_FLOAT, 0, VERTEX_COLOR);

   __evas_gl_glapi->glMatrixMode(GL_MODELVIEW);
   __evas_gl_glapi->glLoadIdentity();
   __evas_gl_glapi->glTranslatef(0, -0.7f, -5.0f);

   angle = (angle + 1) % (360 * 3);
   __evas_gl_glapi->glRotatef((float)angle / 3, 1.0f, 0, 0);
   __evas_gl_glapi->glRotatef((float)angle, 0, 0, 1.0f);

   __evas_gl_glapi->glDrawElements(GL_TRIANGLES, 6 * (3 * 2), GL_UNSIGNED_SHORT, &INDEX_BUFFER[0]);

   __evas_gl_glapi->glDisableClientState(GL_VERTEX_ARRAY);
   __evas_gl_glapi->glDisableClientState(GL_COLOR_ARRAY);
}

static void
draw_cube2(Evas_Object *obj)
{
   int i;
   appdata_s *ad;
   static float zPos = -5.0f;
   static float zPosInc = Z_POS_INC;
   static int angle = 0;

   ELEMENTARY_GLVIEW_USE(obj);
   ad = evas_object_data_get(obj, APPDATA_KEY);

   static const float VERTICES[] =
   {
      ONEN, ONEN, ONEP, ONEP, ONEN, ONEP, ONEN, ONEP, ONEP, ONEP, ONEP, ONEP,
      ONEN, ONEN, ONEN, ONEN, ONEP, ONEN, ONEP, ONEN, ONEN, ONEP, ONEP, ONEN,
      ONEN, ONEN, ONEP, ONEN, ONEP, ONEP, ONEN, ONEN, ONEN, ONEN, ONEP, ONEN,
      ONEP, ONEN, ONEN, ONEP, ONEP, ONEN, ONEP, ONEN, ONEP, ONEP, ONEP, ONEP,
      ONEN, ONEP, ONEP, ONEP, ONEP, ONEP, ONEN, ONEP, ONEN, ONEP, ONEP, ONEN,
      ONEN, ONEN, ONEP, ONEN, ONEN, ONEN, ONEP, ONEN, ONEP, ONEP, ONEN, ONEN
   };

   static const float TEXTURE_COORD[] =
   {
      ONEP, ZERO, ZERO, ZERO, ONEP, ONEP, ZERO, ONEP,
      ONEP, ZERO, ZERO, ZERO, ONEP, ONEP, ZERO, ONEP,
      ONEP, ZERO, ZERO, ZERO, ONEP, ONEP, ZERO, ONEP,
      ONEP, ZERO, ZERO, ZERO, ONEP, ONEP, ZERO, ONEP,
      ONEP, ZERO, ZERO, ZERO, ONEP, ONEP, ZERO, ONEP,
      ONEP, ZERO, ZERO, ZERO, ONEP, ONEP, ZERO, ONEP
   };

   __evas_gl_glapi->glEnableClientState(GL_VERTEX_ARRAY);
   __evas_gl_glapi->glVertexPointer(3, GL_FLOAT, 0, VERTICES);

   __evas_gl_glapi->glEnableClientState(GL_TEXTURE_COORD_ARRAY);
   __evas_gl_glapi->glTexCoordPointer(2, GL_FLOAT, 0, TEXTURE_COORD);

   __evas_gl_glapi->glEnable(GL_TEXTURE_2D);
   __evas_gl_glapi->glBindTexture(GL_TEXTURE_2D, ad->tex_ids[ad->current_tex_index]);

   __evas_gl_glapi->glMatrixMode(GL_MODELVIEW);

   zPos += zPosInc;

   /* Keep switching textures in cube 2 */
   if (zPos < -8.0f)
   {
      zPosInc = Z_POS_INC;
      ad->current_tex_index = 1 - (ad->current_tex_index);
   }

   if (zPos > -5.0f)
      zPosInc = -Z_POS_INC;

   __evas_gl_glapi->glLoadIdentity();
   __evas_gl_glapi->glTranslatef(0, 1.2f, zPos);

   angle = (angle + 1) % (360 * 3);
   __evas_gl_glapi->glRotatef((float)angle / 3, 0, 0, 1.0f);
   __evas_gl_glapi->glRotatef((float)angle, 0, 1.0f, 0);

   for(i = 0; i < 6; i++)
      __evas_gl_glapi->glDrawArrays(GL_TRIANGLE_STRIP, (4 * i), 4);

   __evas_gl_glapi->glDisable(GL_TEXTURE_2D);
   __evas_gl_glapi->glDisableClientState(GL_VERTEX_ARRAY);
   __evas_gl_glapi->glDisableClientState(GL_TEXTURE_COORD_ARRAY);
}


void
draw_gl(Evas_Object *obj)
{
   ELEMENTARY_GLVIEW_USE(obj);
   //printf("%s\n", __func__);
   int w, h;
   __evas_gl_glapi->glShadeModel(GL_SMOOTH);

      __evas_gl_glapi->glEnable(GL_CULL_FACE);
      __evas_gl_glapi->glCullFace(GL_BACK);

      __evas_gl_glapi->glEnable(GL_DEPTH_TEST);
      __evas_gl_glapi->glDepthFunc(GL_LESS);
   elm_glview_size_get(obj, &w, &h);
      set_perspective(obj, 60.0f, w, h, 1.0f, 400.0f);

   __evas_gl_glapi->glClearColor(1.0f, 0.0f, 0.0f, 1.0f);
   __evas_gl_glapi->glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

   draw_cube1(obj);
   draw_cube2(obj);
}


static Evas_Object*
_glview_create(appdata_s *ad)
{
   Evas_Object *obj;
printf("%s\n", __func__);
   /* Create a GLView with an OpenGL-ES 1.1 context */
   obj = elm_glview_version_add(ad->win, EVAS_GL_GLES_1_X);
   evas_object_resize(obj, 320, 480);
   //elm_table_pack(ad->table, obj, 1, 1, 3, 1);
   evas_object_data_set(obj, APPDATA_KEY, ad);

   if (!getenv("DIRECT"))
     elm_glview_mode_set(obj, ELM_GLVIEW_DEPTH);
   else
     elm_glview_mode_set(obj, ELM_GLVIEW_DEPTH | ELM_GLVIEW_DIRECT);

   elm_glview_resize_policy_set(obj, ELM_GLVIEW_RESIZE_POLICY_RECREATE);
   elm_glview_render_policy_set(obj, ELM_GLVIEW_RENDER_POLICY_ON_DEMAND);

   elm_glview_init_func_set(obj, init_gles);
   elm_glview_del_func_set(obj, destroy_gles);
   elm_glview_resize_func_set(obj, resize_gl);
   elm_glview_render_func_set(obj, draw_gl);

   return obj;
}

static void
_win_resize_cb(void *data, Evas *e EINA_UNUSED,
               Evas_Object *obj EINA_UNUSED, void *event_info EINA_UNUSED)
{
   int w, h;
   appdata_s *ad = data;

   evas_object_geometry_get(ad->win, NULL, NULL, &w, &h);
   printf("resize %d, %d\n", w, h);
   evas_object_resize(ad->table, w, h);
   evas_object_resize(ad->bg, w, h);
}

static Eina_Bool
_anim_cb(void *data)
{
   appdata_s *ad = data;
   static int cnt = 0;

   //dlog_print(DLOG_ERROR, LOG_TAG, "elm_glview_changed_set (frame %d)", cnt);
//int w, h;
//evas_object_geometry_get(ad->win, NULL, NULL, &w, &h);
//evas_damage_rectangle_add(evas_object_evas_get(ad->win), 0, 0, w, h);
   //evas_object_color_set(ad->img, drand48()*255, drand48()*255, drand48()*255, 255);
   //evas_object_hide(ad->img);
   //evas_damage_rectangle_add(evas_object_evas_get(ad->img), 0, 0, 720, 1280);
   //if (cnt<100 || cnt>200)
   elm_glview_changed_set(ad->glview);
   cnt++;
   return ECORE_CALLBACK_RENEW;
}

static void
_destroy_anim(void *data, Evas *evas, Evas_Object *obj, void *event_info)
{
   Ecore_Animator *ani = data;
   ecore_animator_del(ani);
}


static void
_close_cb(void *data EINA_UNUSED,
          Evas_Object *obj EINA_UNUSED, void *event_info EINA_UNUSED)
{
   elm_shutdown();
}

int main(int argc, char **argv)
{
   appdata_s add = {0,};
   Evas_Object *o, *t;
   appdata_s *ad = &add;

   /* Force OpenGL engine */
   elm_init(argc, argv);
   elm_config_accel_preference_set("opengl:depth24");

   /* Add a window */
   ad->win = o = elm_win_add(NULL, "glview", ELM_WIN_BASIC);
   elm_win_title_set(o, "GLView Simple");
   elm_win_autodel_set(o, EINA_TRUE);
   //evas_object_smart_callback_add(o, "delete,request", _close_cb, ad);
   evas_object_event_callback_add(o, EVAS_CALLBACK_RESIZE, _win_resize_cb, ad);
   evas_object_resize(ad->win, 320, 480);
   //eext_object_event_callback_add(o, EEXT_CALLBACK_BACK, _close_cb, ad);
   S(o);

   /* Add a background */
   ad->bg = o = elm_bg_add(ad->win);
   //elm_win_resize_object_add(ad->win, ad->bg);
   evas_object_resize(o, 320, 480);
   elm_bg_color_set(o, 255, 68, 68);
   S(o);

   /* Add a resize conformant */
   ad->conform = o = elm_conformant_add(ad->win);
   //elm_win_resize_object_add(ad->win, ad->conform);
   evas_object_resize(o, 320, 480);
   SX(o);

   ad->table = t = elm_table_add(ad->win);
   S(t);

   o = elm_label_add(ad->win);
   elm_object_text_set(o, "Gles 1.1 Cube");
   elm_table_pack(t, o, 1, 0, 3, 1);
   SF(o);

   o = elm_button_add(ad->win);
   elm_object_text_set(o, "Quit");
   evas_object_smart_callback_add(o, "clicked", _close_cb, ad);
   elm_table_pack(t, o, 1, 9, 3, 1);
   SF(o);

   ad->glview = o = _glview_create(ad);
   SX(o);

   //ad->img = evas_object_rectangle_add(evas_object_evas_get(ad->win));
   //evas_object_color_set(ad->img, 255, 0, 0, 255);
   //evas_object_resize(ad->img, 480, 300);
   //evas_object_show(ad->img);

   /* Add an animator to call _anim_cb_() every (1/60) seconds
    * _anim_cb() indicates that glview has changed, which eventually triggers
    * function(draw_gl() here) to redraw glview surface
    */
   ecore_animator_frametime_set(1.0 / 60.0);
   ad->anim = ecore_animator_add(_anim_cb, ad);
   evas_object_event_callback_add(ad->glview, EVAS_CALLBACK_DEL, _destroy_anim, ad->anim);

   elm_run();
   elm_shutdown();
   return 0;
}
#if 0
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
#endif
