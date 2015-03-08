/*
 * Copyright (c) 2014 Samsung Electronics Co., Ltd All Rights Reserved
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

#ifndef __glviewcube20_H__
#define __glviewcube20_H__

//#include <dlog.h>
#include <Elementary.h>


typedef struct appdata {
	const char *name;

	Evas_Object *win;
	Evas_Object *conform;

	/* GL related data here... */
	unsigned int program;
	unsigned int vtx_shader;
	unsigned int fgmt_shader;

	float xangle;
	float yangle;

	unsigned int idx_position;
	unsigned int idx_color;
	int idx_mvp;

	float mvp[16];

	Eina_Bool mouse_down : 1;
	Eina_Bool initialized :1;
} appdata_s;

#endif /* __glviewcube20_H__ */
