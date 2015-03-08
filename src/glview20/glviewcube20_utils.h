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

#ifndef __glviewcube20_utils_H__
#define __glviewcube20_utils_H__

#define UPDATE_INTERVAL 1000ll

void init_matrix(float* result);

void multiply_matrix(float* result, const float matrix0[16], const float matrix1[16]);

void rotate_xyz(float* result, const float anglex, const float angley, const float anglez);

void translate_xyz(float* result, const float translatex,
		const float translatey, const float translatez);

void view_set_ortho(float* result, const float left, const float right,
		const float bottom, const float top, const float near, const float far);

void view_set_perspective(float* result, const float fovy,
		const float aspect, const float near, const float far);

long long get_ticks();

void display_fps();

#endif // __glviewcube20_utils_H__
