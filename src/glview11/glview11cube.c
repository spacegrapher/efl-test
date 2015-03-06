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

#include "glview11cube.h"

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

   elm_glview_mode_set(obj,
                       ELM_GLVIEW_ALPHA
                       //| ELM_GLVIEW_DEPTH
                       | ELM_GLVIEW_DIRECT
                       );
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
   elm_config_accel_preference_set("opengl");

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