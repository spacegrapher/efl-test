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

#include <math.h>
#include <sys/time.h>
//#include <tizen_type.h>
#include "glviewcube20.h"
#include "glviewcube20_utils.h"

void init_matrix(float* result) {
	result[0] = 1.0f;
	result[1] = 0.0f;
	result[2] = 0.0f;
	result[3] = 0.0f;

	result[4] = 0.0f;
	result[5] = 1.0f;
	result[6] = 0.0f;
	result[7] = 0.0f;

	result[8] = 0.0f;
	result[9] = 0.0f;
	result[10] = 1.0f;
	result[11] = 0.0f;

	result[12] = 0.0f;
	result[13] = 0.0f;
	result[14] = 0.0f;
	result[15] = 1.0f;
}

void multiply_matrix(float* result, const float *matrix0,
		const float *matrix1) {
	int i, row, column;
	float temp[16];

	for (column = 0; column < 4; column++) {
		for (row = 0; row < 4; row++) {
			temp[column * 4 + row] = 0.0f;
			for (i = 0; i < 4; i++)
				temp[column * 4 + row] += matrix0[i * 4 + row]
						* matrix1[column * 4 + i];
		}
	}

	for (i = 0; i < 16; i++)
		result[i] = temp[i];
}

void rotate_xyz(float* result, const float anglex, const float angley, const float anglez){
	float temp[16];
	float rz = 2.0f * M_PI * anglez / 360.0f;
	float rx = 2.0f * M_PI * anglex / 360.0f;
	float ry = 2.0f * M_PI * angley / 360.0f;
	float sy = sinf(ry);
	float cy = cosf(ry);
	float sx = sinf(rx);
	float cx = cosf(rx);
	float sz = sinf(rz);
	float cz = cosf(rz);

	init_matrix(temp);

	temp[0] = cy * cz - sx * sy * sz;
	temp[1] = cz * sx * sy + cy * sz;
	temp[2] = -cx * sy;

	temp[4] = -cx * sz;
	temp[5] = cx * cz;
	temp[6] = sx;

	temp[8] = cz * sy + cy * sx * sz;
	temp[9] = -cy * cz * sx + sy * sz;
	temp[10] = cx * cy;

	multiply_matrix(result, result, temp);
}

void translate_xyz(float* result, const float translatex,
		const float translatey, const float translatez) {
	result[12] += result[0] * translatex + result[4] * translatey
			+ result[8] * translatez;
	result[13] += result[1] * translatex + result[5] * translatey
			+ result[9] * translatez;
	result[14] += result[2] * translatex + result[6] * translatey
			+ result[10] * translatez;
	result[15] += result[3] * translatex + result[7] * translatey
			+ result[11] * translatez;
}

void view_set_ortho(float* result, const float left, const float right,
		const float bottom, const float top, const float near, const float far) {
	float diffx = right - left;
	float diffy = top - bottom;
	float diffz = far - near;

	if ((near <= 0.0f) || (far <= 0.0f) || (diffx <= 0.0f) || (diffy <= 0.0f)
			|| (diffz <= 0.0f))
		return;

	result[0] = 2.0f * near / diffx;
	result[1] = 0.0f;
	result[2] = 0.0f;
	result[3] = 0.0f;

	result[4] = 0.0f;
	result[5] = 2.0f * near / diffy;
	result[6] = 0.0f;
	result[7] = 0.0f;

	result[8] = (right + left) / diffx;
	result[9] = (top + bottom) / diffy;
	result[10] = -(near + far) / diffz;
	result[11] = -1.0f;

	result[12] = 0.0f;
	result[13] = 0.0f;
	result[14] = -2.0f * near * far / diffz;
	result[15] = 0.0f;
}

void view_set_perspective(float* result, const float fovy,
		const float aspect, const float near, const float far) {
	float fovradian = fovy / 360.0f * M_PI;
	float top = tanf(fovradian) * near;
	float right = top * aspect;

	view_set_ortho(result, -right, right, -top, top, near, far);
}

long long get_ticks(void) {
	struct timeval time_value;
	long long ticks;

	gettimeofday(&time_value, NULL);

	ticks = (long long) time_value.tv_sec * 1000ll;
	ticks += (long long) time_value.tv_usec / 1000ll;

	return ticks;
}

void display_fps(void){
	static long long last_update_time = 0.0f;
	static int frame_count = 0;
	static Eina_Bool first = EINA_TRUE;
	long long current_tick;
	long long elapsed;

	if (first) {
		last_update_time = get_ticks();
		first = EINA_FALSE;
	}

	frame_count++;
	current_tick = get_ticks();

	elapsed = current_tick - last_update_time;

	if (elapsed >= UPDATE_INTERVAL) {
		float fps = 0.0f;
		fps = ((float) frame_count / (float) elapsed) * 1000.0f;
		//dlog_print(DLOG_INFO, LOG_TAG, "FPS: %.2f frames/sec", fps);

		frame_count = 0;
		last_update_time = current_tick;
	}
}
