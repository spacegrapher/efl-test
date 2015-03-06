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
#include <math.h>

extern const unsigned short IMAGE_565_128_128_1[];
extern const unsigned short IMAGE_4444_128_128_1[];

#define ELEMENTARY_GLVIEW_USE(glview) \
   Evas_GL_API *__evas_gl_glapi = elm_glview_gl_api_get(glview);

#define Z_POS_INC 0.01f

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
