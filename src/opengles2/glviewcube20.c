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
#include <Elementary.h>

#define UPDATE_INTERVAL 1000ll

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

#define ELEMENTARY_GLVIEW_USE(glview) \
   Evas_GL_API *__evas_gl_glapi = elm_glview_gl_api_get(glview);

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

const float cube_vertices[] =
{
	-0.5f, -0.5f, -0.5f,
	-0.5f, -0.5f,  0.5f,
	 0.5f, -0.5f,  0.5f,
	 0.5f, -0.5f, -0.5f,
	-0.5f,  0.5f, -0.5f,
	-0.5f,  0.5f,  0.5f,
	 0.5f,  0.5f,  0.5f,
	 0.5f,  0.5f, -0.5f,
	-0.5f, -0.5f, -0.5f,
	-0.5f,  0.5f, -0.5f,
	 0.5f,  0.5f, -0.5f,
	 0.5f, -0.5f, -0.5f,
	-0.5f, -0.5f,  0.5f,
	-0.5f,  0.5f,  0.5f,
	 0.5f,  0.5f,  0.5f,
	 0.5f, -0.5f,  0.5f,
	-0.5f, -0.5f, -0.5f,
	-0.5f, -0.5f,  0.5f,
	-0.5f,  0.5f,  0.5f,
	-0.5f,  0.5f, -0.5f,
	 0.5f, -0.5f, -0.5f,
	 0.5f, -0.5f,  0.5f,
	 0.5f,  0.5f,  0.5f,
	 0.5f,  0.5f, -0.5f
};

const int cube_indices_count = 36;
const unsigned short cube_indices[] =
{
	 0,  2,  1,
	 0,  3,  2,
	 4,  5,  6,
	 4,  6,  7,
	 8,  9, 10,
	 8, 10, 11,
	12, 15, 14,
	12, 14, 13,
	16, 17, 18,
	16, 18, 19,
	20, 23, 22,
	20, 22, 21
};

const float cube_colors[] =
{
	1.0, 0.0, 0.0, 1.0,
	0.0, 1.0, 0.0, 1.0,
	0.0, 0.0, 1.0, 1.0,
	1.0, 0.0, 0.0, 1.0,
	0.0, 1.0, 0.0, 1.0,
	0.0, 0.0, 1.0, 1.0,
	1.0, 0.0, 0.0, 1.0,
	0.0, 1.0, 0.0, 1.0,
	0.0, 0.0, 1.0, 1.0,
	1.0, 0.0, 0.0, 1.0,
	0.0, 1.0, 0.0, 1.0,
	0.0, 0.0, 1.0, 1.0,
	1.0, 0.0, 0.0, 1.0,
	0.0, 1.0, 0.0, 1.0,
	0.0, 0.0, 1.0, 1.0,
	1.0, 0.0, 0.0, 1.0,
	0.0, 1.0, 0.0, 1.0,
	0.0, 0.0, 1.0, 1.0,
	1.0, 0.0, 0.0, 1.0,
	0.0, 1.0, 0.0, 1.0,
	0.0, 0.0, 1.0, 1.0,
	1.0, 0.0, 0.0, 1.0,
	0.0, 1.0, 0.0, 1.0,
	0.0, 0.0, 1.0, 1.0
};

/* Vertex Shader Source */
static const char vertex_shader[] =
		"uniform mat4 u_mvpMatrix;\n"
		"attribute vec4 a_position;\n"
		"attribute vec4 a_color;\n"
		"varying vec4 v_color;\n"
		"\n"
		"void main()\n"
		"{\n"
		"    v_color = a_color;\n"
		"    gl_Position = u_mvpMatrix * a_position;\n"
		"}";

/* Fragment Shader Source */
static const char fragment_shader[] =
		"#ifdef GL_ES\n"
		"precision mediump float;\n"
		"#endif\n"
		"varying vec4 v_color;\n"
		"\n"
		"void main (void)\n"
		"{\n"
		"    gl_FragColor = v_color;\n"
		"}";

static void win_back_cb(void *data, Evas_Object *obj, void *event_info) {
	appdata_s *ad = data;
	/* Let window go to hidden state. */
	elm_win_lower(ad->win);
}

static void init_shaders(Evas_Object *obj) {
	ELEMENTARY_GLVIEW_USE(obj);
	appdata_s *ad = evas_object_data_get(obj, "ad");
	const char *p;

	p = vertex_shader;
	ad->vtx_shader = __evas_gl_glapi->glCreateShader(GL_VERTEX_SHADER);
	__evas_gl_glapi->glShaderSource(ad->vtx_shader, 1, &p, NULL);
	__evas_gl_glapi->glCompileShader(ad->vtx_shader);

	p = fragment_shader;
	ad->fgmt_shader = __evas_gl_glapi->glCreateShader(GL_FRAGMENT_SHADER);
	__evas_gl_glapi->glShaderSource(ad->fgmt_shader, 1, &p, NULL);
	__evas_gl_glapi->glCompileShader(ad->fgmt_shader);

	ad->program = __evas_gl_glapi->glCreateProgram();
	__evas_gl_glapi->glAttachShader(ad->program, ad->vtx_shader);
	__evas_gl_glapi->glAttachShader(ad->program, ad->fgmt_shader);

	__evas_gl_glapi->glLinkProgram(ad->program);

	ad->idx_position = __evas_gl_glapi->glGetAttribLocation(ad->program, "a_position");
	ad->idx_color = __evas_gl_glapi->glGetAttribLocation(ad->program, "a_color");
	ad->idx_mvp = __evas_gl_glapi->glGetUniformLocation(ad->program, "u_mvpMatrix");

	__evas_gl_glapi->glUseProgram(ad->program);
}

static void
mouse_down_cb(void *data, Evas *e , Evas_Object *obj , void *event_info)
{
	appdata_s *ad = data;
	ad->mouse_down = EINA_TRUE;
}

static void
mouse_move_cb(void *data, Evas *e , Evas_Object *obj , void *event_info)
{
	Evas_Event_Mouse_Move *ev;
	ev = (Evas_Event_Mouse_Move *)event_info;
	appdata_s *ad = data;
	float dx = 0, dy = 0;

	if(ad->mouse_down) {
		dx = ev->cur.canvas.x - ev->prev.canvas.x;
		dy = ev->cur.canvas.y - ev->prev.canvas.y;
		ad->xangle += dy;
		ad->yangle += dx;
	}
}

static void
mouse_up_cb(void *data, Evas *e , Evas_Object *obj , void *event_info)
{
	appdata_s *ad = data;
	ad->mouse_down = EINA_FALSE;
}

static void draw_gl(Evas_Object *obj) {
	ELEMENTARY_GLVIEW_USE(obj);
	appdata_s *ad = evas_object_data_get(obj, "ad");
	float model[16], view[16];
	float aspect;
	int w, h;

	if (!ad)
		return;

	init_matrix(model);
	init_matrix(view);

	elm_glview_size_get(obj, &w, &h);
	if (!h)
		return;

	aspect = (float) w / (float) h;
	view_set_perspective(view, 60.0f, aspect, 1.0f, 20.0f);

	translate_xyz(model, 0.0f, 0.0f, -2.5f);
	rotate_xyz(model, ad->xangle, ad->yangle, 0.0f);

	multiply_matrix(ad->mvp, view, model);

	__evas_gl_glapi->glViewport(0, 0, w, h);

	__evas_gl_glapi->glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	__evas_gl_glapi->glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	__evas_gl_glapi->glVertexAttribPointer(ad->idx_position, 3, GL_FLOAT, GL_FALSE,
			3 * sizeof(float), cube_vertices);
	__evas_gl_glapi->glVertexAttribPointer(ad->idx_color, 4, GL_FLOAT, GL_FALSE,
			4 * sizeof(float), cube_colors);

	__evas_gl_glapi->glEnableVertexAttribArray(ad->idx_position);
	__evas_gl_glapi->glEnableVertexAttribArray(ad->idx_color);

	__evas_gl_glapi->glUniformMatrix4fv(ad->idx_mvp, 1, GL_FALSE, ad->mvp);

	__evas_gl_glapi->glDrawElements(GL_TRIANGLES, cube_indices_count, GL_UNSIGNED_SHORT,
			cube_indices);

	__evas_gl_glapi->glFlush();

	display_fps();
}

static void del_gl(Evas_Object *obj) {
	ELEMENTARY_GLVIEW_USE(obj);
	appdata_s *ad = evas_object_data_get(obj, "ad");

	__evas_gl_glapi->glDeleteShader(ad->vtx_shader);
	__evas_gl_glapi->glDeleteShader(ad->fgmt_shader);
	__evas_gl_glapi->glDeleteProgram(ad->program);

	evas_object_data_del((Evas_Object*) obj, "ad");
}

static void del_anim(void *data, Evas *evas, Evas_Object *obj, void *event_info)
{
	Ecore_Animator *ani = evas_object_data_get(obj, "ani");
	ecore_animator_del(ani);
}

static Eina_Bool anim(void *data) {
	elm_glview_changed_set(data);
	return EINA_TRUE;
}

static void init_gl(Evas_Object *obj) {
	ELEMENTARY_GLVIEW_USE(obj);
	appdata_s *ad = evas_object_data_get(obj, "ad");

	if (!ad->initialized) {
		init_shaders(obj);
		__evas_gl_glapi->glEnable(GL_DEPTH_TEST);
		ad->initialized = EINA_TRUE;
	}

	ad->xangle = 45.0f;
	ad->yangle = 45.0f;
}

static void
win_delete_request_cb(void *data , Evas_Object *obj , void *event_info)
{
	elm_shutdown();
}

static void create_indicator(appdata_s *ad) {
	elm_win_conformant_set(ad->win, EINA_TRUE);

	elm_win_indicator_mode_set(ad->win, ELM_WIN_INDICATOR_SHOW);
	elm_win_indicator_opacity_set(ad->win, ELM_WIN_INDICATOR_TRANSPARENT);

	ad->conform = elm_conformant_add(ad->win);
	evas_object_size_hint_weight_set(ad->conform, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
	elm_win_resize_object_add(ad->win, ad->conform);
	evas_object_show(ad->conform);
}

static Evas_Object* add_win(const char *name) {
	Evas_Object *win;

	elm_config_accel_preference_set("opengl:depth24");
	win = elm_win_util_standard_add(name, "OpenGL example: Cube");
	evas_object_resize(win, 360, 480);

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

	Evas_Object *gl;
	Ecore_Animator *ani;
	appdata_s *ad = data;

	if (!data)
		return EINA_FALSE;

	/* Create the window */
	ad->win = add_win(ad->name);

	if (!ad->win)
		return EINA_FALSE;

	create_indicator(ad);
	evas_object_smart_callback_add(ad->win, "delete,request", win_delete_request_cb, NULL);
	//eext_object_event_callback_add(ad->win, EEXT_CALLBACK_BACK, win_back_cb, ad);

	/* Create and initialize GLView */
	gl = elm_glview_add(ad->conform);
	//ELEMENTARY_GLVIEW_GLOBAL_USE(gl);
	evas_object_size_hint_align_set(gl, EVAS_HINT_FILL, EVAS_HINT_FILL);
	evas_object_size_hint_weight_set(gl, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);

	/* Request a surface with alpha and a depth buffer */
        if (getenv("DIRECT"))
	  elm_glview_mode_set(gl, ELM_GLVIEW_DEPTH|ELM_GLVIEW_DIRECT);
        else
	  elm_glview_mode_set(gl, ELM_GLVIEW_DEPTH);

	/* The resize policy tells GLView what to do with the surface when it
	 * resizes. ELM_GLVIEW_RESIZE_POLICY_RECREATE will tell it to
	 * destroy the current surface and recreate it to the new size.
	 */
	elm_glview_resize_policy_set(gl, ELM_GLVIEW_RESIZE_POLICY_RECREATE);

	/* The render policy sets how GLView should render GL code.
	 * ELM_GLVIEW_RENDER_POLICY_ON_DEMAND will have the GL callback
	 * called only when the object is visible.
	 * ELM_GLVIEW_RENDER_POLICY_ALWAYS would cause the callback to be
	 * called even if the object were hidden.
	 */
	elm_glview_render_policy_set(gl, ELM_GLVIEW_RENDER_POLICY_ON_DEMAND);

	/* The initialize callback function gets registered here */
	elm_glview_init_func_set(gl, init_gl);

	/* The delete callback function gets registered here */
	elm_glview_del_func_set(gl, del_gl);

	/* The render callback function gets registered here */
	elm_glview_render_func_set(gl, draw_gl);

	/* Add the GLView to the conformant and show it */
	elm_object_content_set(ad->conform, gl);
	evas_object_show(gl);

	elm_object_focus_set(gl, EINA_TRUE);

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

	evas_object_event_callback_add(gl, EVAS_CALLBACK_MOUSE_DOWN, mouse_down_cb, ad);
	evas_object_event_callback_add(gl, EVAS_CALLBACK_MOUSE_UP, mouse_up_cb, ad);
	evas_object_event_callback_add(gl, EVAS_CALLBACK_MOUSE_MOVE, mouse_move_cb, ad);

	evas_object_show(ad->win);

	/* Return true: the main loop will now start running */
	return EINA_TRUE;
}
#if 0
static void app_control(app_control_h app_control, void *data) {
	/* Handle the launch request. */
}

static void app_pause(void *data) {
	/* Take necessary actions when application becomes invisible. */
}

static void app_resume(void *data) {
	/* Take necessary actions when application becomes visible. */
}

static void app_terminate(void *data) {
	/* Release all resources. */
}
#endif
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

