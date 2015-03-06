/**
 * Simple Elementary's <b>GLView widget</b> example, illustrating its
 * creation.
 *
 * See stdout/stderr for output. Compile with:
 *
 * @verbatim
 * gcc -o glview_example_01 glview_example_01.c -g `pkg-config --cflags --libs elementary`
 * @endverbatim
 */
#include <Elementary.h>
#include <Evas_GL.h>
#include <stdio.h>
#include <assert.h>

/* for tcLog */
///////////////////////////////
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
//#include <dlog/dlog.h>

FILE* LogFile;

#ifdef LOG_TAG
#undef LOG_TAG
#endif
#define LOG_TAG "TIZEN_GTS"

#define LOG_PRINTF 1
#define LOG_DLOG 0 

void tcLog(const char* format, ...)
{
	va_list args;
	va_start(args, format);

#if LOG_PRINTF == 1
	vprintf(format, args);
#endif

#if LOG_DLOG == 1
	__dlog_vprint(LOG_ID_MAIN, DLOG_INFO, LOG_TAG, format, args);
#endif

	if (LogFile)
	{
		vfprintf(LogFile, format, args);
	}

	va_end(args);
}
/////////////////////////////////


typedef struct _GLData GLData;
#define side 10
#define numVertices side * side * side
static const float PI = 3.1415926535897932384626433832795f;

#define CHECK_GL_ERROR { int _err = gl->glGetError(); if (_err != GL_NO_ERROR) { tcLog("GL error %#0.4x, file %s, line %d\n", _err, __FILE__, __LINE__); /*ret = 0; goto finish;*/ } }

static GLuint load_shader( GLData *gld, GLenum type, const char *shader_src );

typedef struct FloatPoint{
	GLfloat 	x;
	GLfloat   y;
}FloatPoint;


// GL related data here..
struct _GLData {
	Evas_GL_API *glapi;
/*	GLuint program;
	GLuint vtx_shader;
	GLuint vtx_FEEDBACKshader;
	GLuint fgmt_shader;
	GLuint vbo;*/
	int initialized :1;

	GLuint m_tfProgramObject;
	GLuint m_renderProgramObject;
	GLint m_indexPositionTF;
	GLint m_indexPositionR;
	GLint m_indexForce;
	GLint m_indexTextureSample;
	GLint m_indexMVP;
	GLint m_indexTouchPosition;

	// simulator touch coordinates
	GLfloat m_x;
	GLfloat m_y;

	GLint m_width;
	GLint m_height;

	GLuint m_feedbackQuery;
	GLuint m_feedbackObject;

	GLuint m_feedbackBuffer[2];

	GLuint m_textureId;
	int w; // window width
	int h; // window height
};

GLuint load_shader( GLData *gld, GLenum type, const char *shader_src );

// This vertext shader is used to update the vertex position and calculate new force value only
// in transform feedback

static const char TRANSFORM_VERTEX_TEXT[] =
	"#version 300 es\n"
	"precision highp float;\n"
	"layout (location = 0) in highp vec3 aPosition;\n"
	"layout (location = 1) in highp vec3 aForce;\n"
	"uniform highp mat4 uPositionMatrix;\n"
	"uniform highp vec2 uTouchPosition;\n"
	"out highp vec3 oPosition;\n"
	"out highp vec3 oForce;\n"
	"void main()\n"
	"{\n"
	"    gl_Position = uPositionMatrix * (vec4(aPosition, 0.0) + vec4(aForce, 0.0));\n"
	"    oPosition = gl_Position.xyz;\n"
	"    float diff = 0.001;\n"
	"    vec3 direction = normalize(vec3(uTouchPosition.x, uTouchPosition.y, 0.0) - vec3(oPosition.x, oPosition.y, 0.0));\n"
	"    oForce = aForce * (1.0 - diff) + direction * diff;\n"
	"}";


//This vertext shader is used for rendering in transform feedback
static const char RENDER_VERTEX_TEXT[] =
	"#version 300 es\n"
	"precision highp float;\n"
	"layout (location = 2) in highp vec4 aPosition1;\n"
	"void main()\n"
	"{\n"
	"    gl_Position = aPosition1;\n"
	"    gl_PointSize = clamp(0.5 + gl_Position.z, 0.0, 100000000.0) * 100.0;\n"
	"}";


//A simple fragment shader for texturing only
static const char FRAGMENT_TEXT[] =
	"#version 300 es\n"
	"precision highp float;\n"
	"uniform sampler2D sTexture;\n"
	"layout(location = 0) out lowp vec4 oColour;\n"
	"void main()\n"
	"{\n"
	"	vec2 textureCoord = abs(gl_PointCoord)*2.0;\n"
	"	oColour = texture(sTexture, textureCoord);\n"
	"}";




////////////////////////
// helper functions
////////////////////////
void tfSetPosition(GLData *gld, FloatPoint end){
	gld->m_x = 2.0f * (end.x / gld->m_width - 0.5f);
	gld->m_y = -2.0f * (end.y / gld->m_height - 0.5);

}

int isFloatEqual(float a, float b, float epsilon)
{
	float absA = fabs(a);
	float absB = fabs(b);
	float diff = fabs(a - b);

	if (a == b) {
		// shortcut, handles infinities
		return 1;
	} else if ( diff < FLT_MIN) {
		return 1;
	} else if(  diff / (absA + absB) < epsilon){
			return 1;
	}else{

		tcLog("isFloatEqual: absA= %f, absB = %f, diff= %f, ratio=%f, epsilon= %f \n",absA, absB, diff, diff / (absA + absB), epsilon);
		return 0;
	}
}



////////////////////////////////////////////////
// Initialisation functions
////////////////////////////////////////////////

//--------------------------------//
// a helper function to load shaders from a shader source
GLuint load_shader( GLData *gld, GLenum type, const char *shader_src )
{
   Evas_GL_API *gl = gld->glapi;
   GLuint shader;
   GLint compiled;

   // Create the shader object
   shader = gl->glCreateShader(type);
   if (shader==0)
      return 0;

   // Load/Compile shader source
   gl->glShaderSource(shader, 1, &shader_src, NULL);
   gl->glCompileShader(shader);
   gl->glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);

   if (!compiled)
     {
        GLint info_len = 0;
        gl->glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &info_len);
        if (info_len > 1)
          {
             char* info_log = malloc(sizeof(char) * info_len);

             gl->glGetShaderInfoLog(shader, info_len, NULL, info_log);
             printf("Error compiling shader:\n%s\n======\n%s\n======\n", info_log, shader_src );
             free(info_log);
          }
        gl->glDeleteShader(shader);
        return 0;
     }

   return shader;
}

int tfInit_TransformFeedback(GLData *gld){

	GLint linked = GL_FALSE;
	int ret=1;
	time_t t;
	Evas_GL_API *gl = gld->glapi;
	GLuint vertShader=0;
	GLuint fragShader=0;

	vertShader = load_shader(gld, GL_VERTEX_SHADER, TRANSFORM_VERTEX_TEXT);
	fragShader = load_shader(gld, GL_FRAGMENT_SHADER, FRAGMENT_TEXT);
	if (vertShader == 0 || fragShader == 0)
	{
		tcLog("load_shader failed, vertShader: %d, fragShader: %d\n", vertShader, fragShader);
		 ret=0;
		goto finish;
	}

	gld->m_tfProgramObject = gl->glCreateProgram();
	CHECK_GL_ERROR;
	gl->glAttachShader(gld->m_tfProgramObject, vertShader);
	CHECK_GL_ERROR;
	gl->glAttachShader(gld->m_tfProgramObject, fragShader);
	CHECK_GL_ERROR;

	// Set transform feedback varyings that will be written into the transform feedback buffer
	{
		const char* feedbackVaryings[] = {"oPosition", "oForce"};
		gl->glTransformFeedbackVaryings(gld->m_tfProgramObject, 2, feedbackVaryings, GL_INTERLEAVED_ATTRIBS);
		CHECK_GL_ERROR;
	}

	gl->glLinkProgram(gld->m_tfProgramObject);
	CHECK_GL_ERROR;

	gl->glGetProgramiv(gld->m_tfProgramObject, GL_LINK_STATUS, &linked);

	if (linked == GL_FALSE)
	{
		GLint infoLen = 0;
		gl->glGetProgramiv(gld->m_tfProgramObject, GL_INFO_LOG_LENGTH, &infoLen);

		if (infoLen > 1)
		{
			char* infoLog = malloc(infoLen);
			gl->glGetProgramInfoLog(gld->m_tfProgramObject, infoLen, NULL, infoLog);
			tcLog("Info log: %s\n", infoLog);
			free(infoLog);
		}
		ret=0;
		goto finish;
	}

	gld->m_indexPositionTF = gl->glGetAttribLocation(gld->m_tfProgramObject, "aPosition");
	CHECK_GL_ERROR;
	gld->m_indexForce = gl->glGetAttribLocation(gld->m_tfProgramObject, "aForce");
	CHECK_GL_ERROR;
	gld->m_indexMVP = gl->glGetUniformLocation(gld->m_tfProgramObject, "uPositionMatrix");
	CHECK_GL_ERROR;
	gld->m_indexTouchPosition = gl->glGetUniformLocation(gld->m_tfProgramObject, "uTouchPosition");
	CHECK_GL_ERROR;

	gl->glGenBuffers(2, gld->m_feedbackBuffer);
	CHECK_GL_ERROR;
	gl->glGenTransformFeedbacks(1, &gld->m_feedbackObject);
	CHECK_GL_ERROR;
	gl->glGenQueries(1, &gld->m_feedbackQuery);
	CHECK_GL_ERROR;

	// generate vertex data
	{
		float* pBufferVertex;
		float* pBuffer ;
		int index;

		pBufferVertex = malloc( sizeof(float)* numVertices * 3);
		pBuffer = malloc( sizeof(float)* numVertices * 3 * 2);

		for (index = 0; index < numVertices; index++)
		{
			pBufferVertex[index * 3 + 0] = -0.5f + (float)(index % side) / (side - 1);
			pBufferVertex[index * 3 + 1] = -0.5f + (float)((index / side) % side) / (side - 1);
			pBufferVertex[index * 3 + 2] = -0.5f + (float)((index / side / side) % side) / (side - 1);
		}

		// vertex position and force are interleaved in the same buffer
		for (index = 0; index < numVertices; index++)
		{
			pBuffer[index * 3 * 2 + 0] = pBufferVertex[index * 3 + 0];
			pBuffer[index * 3 * 2 + 1] = pBufferVertex[index * 3 + 1];
			pBuffer[index * 3 * 2 + 2] = pBufferVertex[index * 3 + 2];
			pBuffer[index * 3 * 2 + 3] = 0.0f;
			pBuffer[index * 3 * 2 + 4] = 0.0f;
			pBuffer[index * 3 * 2 + 5] = 0.0f;
		}

		gl->glBindBuffer(GL_ARRAY_BUFFER, gld->m_feedbackBuffer[0]);
		CHECK_GL_ERROR;
		gl->glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat) * numVertices * 6, pBuffer, GL_DYNAMIC_DRAW);
		CHECK_GL_ERROR;
		gl->glBindBuffer(GL_ARRAY_BUFFER, 0);
		CHECK_GL_ERROR;

		free( pBufferVertex);
		free( pBuffer);
	}

	gl->glBindBuffer(GL_ARRAY_BUFFER, gld->m_feedbackBuffer[1]);
	CHECK_GL_ERROR;
	gl->glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat) * numVertices * 6, NULL, GL_DYNAMIC_DRAW);
	CHECK_GL_ERROR;
	gl->glBindBuffer(GL_ARRAY_BUFFER, 0);
	CHECK_GL_ERROR;

	// initialize random number generator to randomly change the position
	srand((unsigned) time(&t));

finish:
	if( 0 != vertShader)
		gl->glDeleteShader(vertShader);
	if( 0 != fragShader)
		gl->glDeleteShader(fragShader);

	return ret;
}

int tfInit_Render(GLData *gld)
{
	GLint linked = GL_FALSE;
	int ret=1;
	Evas_GL_API *gl = gld->glapi;
	GLuint fragShader=0;
	GLuint vertShader=0;

	vertShader = load_shader(gld, GL_VERTEX_SHADER, RENDER_VERTEX_TEXT);
	fragShader = load_shader(gld, GL_FRAGMENT_SHADER, FRAGMENT_TEXT);
	if (vertShader == 0 || fragShader == 0)
	{
		tcLog("load_shader failed, vertShader: %d, fragShader: %d\n", vertShader, fragShader);
		 ret=0;
		goto finish;
	}

	gld->m_renderProgramObject = gl->glCreateProgram();
	CHECK_GL_ERROR;
	gl->glAttachShader(gld->m_renderProgramObject, fragShader);
	CHECK_GL_ERROR;
	gl->glAttachShader(gld->m_renderProgramObject, vertShader);
	CHECK_GL_ERROR;

	gl->glLinkProgram(gld->m_renderProgramObject);
	CHECK_GL_ERROR;
	gl->glGetProgramiv(gld->m_renderProgramObject, GL_LINK_STATUS, &linked);

	if (linked == GL_FALSE)
	{
		GLint infoLen = 0;
		gl->glGetProgramiv(gld->m_renderProgramObject, GL_INFO_LOG_LENGTH, &infoLen);

		if (infoLen > 1)
		{
			char* infoLog = malloc(infoLen);
			gl->glGetProgramInfoLog(gld->m_renderProgramObject, infoLen, NULL, infoLog);
			tcLog("Info log: %s\n", infoLog);
			free(infoLog);
		}
		 ret=0;
		goto finish;

	}

	gld->m_indexPositionR = gl->glGetAttribLocation(gld->m_renderProgramObject, "aPosition1");
	CHECK_GL_ERROR;
	gld->m_indexTextureSample = gl->glGetUniformLocation(gld->m_renderProgramObject, "sTexture");
	CHECK_GL_ERROR;

	// generate random texture data here
	{
		int size = 16;
		unsigned int* pTemp ;
		int h, w;

		gl->glGenTextures(1, &gld->m_textureId);
		CHECK_GL_ERROR;
		gl->glActiveTexture(GL_TEXTURE0);
		CHECK_GL_ERROR;
		gl->glBindTexture(GL_TEXTURE_2D, gld->m_textureId);
		CHECK_GL_ERROR;

		pTemp = (unsigned int*) malloc( size * size * sizeof(unsigned int));
		for ( h = 0; h < size; h++)
		{
			for ( w = 0; w < size; w++)
			{
				unsigned int r = w * 0xff / (size - 1);
				unsigned int g = 0xff;
				unsigned int b = h * 0xff / (size - 1);
				float distance = sqrt(pow(size - w, 2) + pow(size - h, 2)) / size;
				if (distance >= 1.0f)
				{
					distance = 1.0f;
				}

				float oneMinusAlpha = 1.0f- distance;
				unsigned int a = (unsigned int)(pow(oneMinusAlpha, 2) * 255.0f);
				pTemp[w + h * size] = (a << 24) | (b << 16) | (g << 8) | (r); ///ABGR
			}
		}

		gl->glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, size, size, 0, GL_RGBA, GL_UNSIGNED_BYTE, (GLvoid*)pTemp);
		CHECK_GL_ERROR;

		gl->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		gl->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		gl->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_MIRRORED_REPEAT);
		gl->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_MIRRORED_REPEAT);

		free( pTemp);
	}
	return ret;
finish:
	if( 0!= vertShader)
		gl->glDeleteShader(vertShader);
	if( 0 != fragShader)
		gl->glDeleteShader(fragShader);

	return ret;
}

///////////////////////////////////////
// some maths functions here
///////////////////////////////////////
void
tsTranslate4x4f(float pResult[], float tx, float ty, float tz)
{
	float temp[4][4];

	memcpy((float*)&temp[0][0], (float*)pResult, sizeof(float)*16);

	temp[3][0] += (temp[0][0] * tx + temp[1][0] * ty + temp[2][0] * tz);
	temp[3][1] += (temp[0][1] * tx + temp[1][1] * ty + temp[2][1] * tz);
	temp[3][2] += (temp[0][2] * tx + temp[1][2] * ty + temp[2][2] * tz);
	temp[3][3] += (temp[0][3] * tx + temp[1][3] * ty + temp[2][3] * tz);
	memcpy((float*)pResult, (float*)&temp[0][0],  sizeof(float)*16);
}

void
tsRotate4x4f(float pResult[], float angle, float x, float y, float z)
{
	float rotate[4][4];

	float cos = cosf(angle * PI / 180.0f);
	float sin = sinf(angle * PI / 180.0f);
	float cos1 = 1.0f - cos;

	float magnitude= sqrt (x*x+ y*y + z*z);
	if( magnitude)
	{
		x/=magnitude;
		y/=magnitude;
		z=magnitude;
	}

	rotate[0][0] = (x * x) * cos1 + cos;
	rotate[0][1] = (x * y) * cos1 - z * sin;
	rotate[0][2] = (z * x) * cos1 + y * sin;
	rotate[0][3] = 0.0f;

	rotate[1][0] = (x * y) * cos1 + z * sin;
	rotate[1][1] = (y * y) * cos1 + cos;
	rotate[1][2] = (y * z) * cos1 - x * sin;
	rotate[1][3] = 0.0f;

	rotate[2][0] = (z * x) * cos1 - y * sin;
	rotate[2][1] = (y * z) * cos1 + x * sin;
	rotate[2][2] = (z * z) * cos1 + cos;

	rotate[2][3] = rotate[3][0] = rotate[3][1] = rotate[3][2] = 0.0f;
	rotate[3][3] = 1.0f;

	memcpy((float*)pResult,(float*) &rotate[0][0], sizeof(float)*16);
}

void
tsScale4x4f(float pResult[], float x, float y, float z)
{

	pResult[0] = x;
	pResult[5] = y;
	pResult[10] = z;

}

void MatrixMultiply(float* inMatrix, float* inVector, float* outVector, int size)
{
	int i,j;
	assert(inMatrix);
	assert(inVector);
	assert(outVector);
	assert(size);

	for( i = 0; i < size; i++)
	{
		outVector[i]=0;
		for( j=0; j < size; j++)
			outVector[i]+= inMatrix[i*size+j] * inVector[j];
	}
}


////////////////////////////////////////////////
// Function to test transform feedback vertex shader
////////////////////////////////////////////////
int tfVertexShader(float* aPosition, float* aForce, float* uPositionMatrix, float* uTouchPosition, float* oPosition, float* oForce){

	assert(aPosition);
	assert(aForce);
	assert(uPositionMatrix);
	assert(uTouchPosition);
	assert(oPosition);
	assert(oForce);

	float		aPositionAndForce[3];
	float		direction[3];
	float		normalised_direction[3];
	int i;
	float diff = 0.001f;


	for(i = 0; i < 3; i++)
		aPositionAndForce[i]=aPosition[i] + aForce[i];

	MatrixMultiply(uPositionMatrix, aPositionAndForce, oPosition, 3);


	direction[0]=uTouchPosition[0] - oPosition[0];
	direction[1]=uTouchPosition[1] - oPosition[1];
	direction[2]=0.0;

	{
		float mag=sqrt ( direction[0]*direction[0] + direction[1]*direction[1] + direction[2]*direction[2])  ;
		if( mag != 0.0)
		{
			normalised_direction[0]=direction[0]/mag;
			normalised_direction[1]=direction[1]/mag;
			normalised_direction[2]=direction[2]/mag;
		}
	}
	for(i = 0; i < 3; i++)
		oForce[i]=aForce[i] * (1.0 - diff) + normalised_direction[i] * diff;

	return 1;
}


////////////////////////////////////////////////////////////////////
// This function is called every frame to update the vertex position and force based on the random touch position
// Update happens in the vertex shader and output is stored in transform feedback buffer
////////////////////////////////////////////////////////////////////
int
tfUpdate(GLData *gld)
{
	float matIdentity[16];
	float matIdentity3x3[9];
	GLuint tmpbuffer;
	FloatPoint randomPos;
	int ret=1;
  Evas_GL_API *gl = gld->glapi;

	// mimic screen touch by using a random number to set the position
	randomPos.x= rand() % gld->m_width;
	randomPos.y= rand() % gld->m_height;
	tfSetPosition(gld, randomPos);

	memset(matIdentity, 0, sizeof(matIdentity));
	memset(matIdentity3x3, 0, sizeof(matIdentity3x3));
	matIdentity[0]=matIdentity[5]=matIdentity[10]=matIdentity[15]=1.0f;
	matIdentity3x3[0]=matIdentity3x3[4]=matIdentity3x3[8]=1.0f;

	gl->glUseProgram(gld->m_tfProgramObject);
	CHECK_GL_ERROR;

	gl->glViewport(0, 0, gld->m_width, gld->m_height);
	CHECK_GL_ERROR;


	gl->glDisable(GL_DEPTH_TEST);
	CHECK_GL_ERROR;

	gl->glDisable(GL_CULL_FACE);
	CHECK_GL_ERROR;

	// We want to stop rendering after vertex shader as we are only interested in transform feedback
	gl->glEnable(GL_RASTERIZER_DISCARD);
	CHECK_GL_ERROR;

	// m_feedbackBuffer[0] is used as input and m_feedbackBuffer[1] as output for transform feedback
	// at the end of this function, the buffers are interchanged and buffer 0 will have updated vertices
	gl->glBindBuffer(GL_ARRAY_BUFFER, gld->m_feedbackBuffer[0]);
	CHECK_GL_ERROR;
	gl->glEnableVertexAttribArray(gld->m_indexPositionTF);
	CHECK_GL_ERROR;
	gl->glEnableVertexAttribArray(gld->m_indexForce);
	CHECK_GL_ERROR;
	gl->glVertexAttribPointer(gld->m_indexPositionTF, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(GLfloat), 0);
	CHECK_GL_ERROR;
	gl->glVertexAttribPointer(gld->m_indexForce, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(GLfloat), (const void*)(sizeof(GLfloat) * 3));
	CHECK_GL_ERROR;

	gl->glBindTransformFeedback(GL_TRANSFORM_FEEDBACK, gld->m_feedbackObject);
	CHECK_GL_ERROR;
	gl->glBindBuffer(GL_ARRAY_BUFFER, gld->m_feedbackBuffer[1]);
	CHECK_GL_ERROR;
	gl->glBindBufferBase(GL_TRANSFORM_FEEDBACK_BUFFER, 0, gld->m_feedbackBuffer[1]);
	CHECK_GL_ERROR;

	gl->glUniformMatrix4fv(gld->m_indexMVP, 1, GL_FALSE, (GLfloat*)matIdentity);
	CHECK_GL_ERROR;
	gl->glUniform2f(gld->m_indexTouchPosition, gld->m_x, gld->m_y);
	CHECK_GL_ERROR;

	gl->glBeginQuery(GL_TRANSFORM_FEEDBACK_PRIMITIVES_WRITTEN, gld->m_feedbackQuery);
	CHECK_GL_ERROR;
	gl->glBeginTransformFeedback(GL_POINTS);
	CHECK_GL_ERROR;
	gl->glDrawArrays(GL_POINTS, 0, numVertices);
	CHECK_GL_ERROR;
	gl->glEndTransformFeedback();
	CHECK_GL_ERROR;
	gl->glEndQuery(GL_TRANSFORM_FEEDBACK_PRIMITIVES_WRITTEN);
	CHECK_GL_ERROR;

	gl->glFlush();
	CHECK_GL_ERROR;

/////////////////////////////////////////////////////
// Results Verification: Start
/////////////////////////////////////////////////////

	//Check the number of vertices that were processed in transform feedback
	{
		GLuint	tfVertexCount=0;
		float		*inputVertexArray, *outputVertexArray;
		int i;

		// check the processed vertex count
		gl->glGetQueryObjectuiv(gld->m_feedbackQuery, GL_QUERY_RESULT, &tfVertexCount);
		CHECK_GL_ERROR;
		if( numVertices != tfVertexCount){
			tcLog("Transform Feedback vertex count does not match, input vertex count = %u, \t output vextex count = %u \n", numVertices, tfVertexCount);
			ret = 0;
		}

		// get pointers to input and output arrays
		gl->glBindBuffer(GL_ARRAY_BUFFER, gld->m_feedbackBuffer[0]);
		CHECK_GL_ERROR;
		inputVertexArray=gl->glMapBufferRange(GL_ARRAY_BUFFER, 0, sizeof(GLfloat) * numVertices * 6, GL_MAP_READ_BIT);
		CHECK_GL_ERROR;
		outputVertexArray=gl->glMapBufferRange(GL_TRANSFORM_FEEDBACK_BUFFER, 0, sizeof(GLfloat) * numVertices * 6, GL_MAP_READ_BIT);
		CHECK_GL_ERROR;
		for ( i=0; i < numVertices; i++){
			float		oPosition[3];
			float		oForce[3];
			float		uTouchPosition[2];
			int j;
			float epsilon=0.001;

			uTouchPosition[0]=gld->m_x;
			uTouchPosition[1]=gld->m_y;
			tfVertexShader(&inputVertexArray[i*3*2], &inputVertexArray[i*3*2+3], matIdentity3x3, uTouchPosition, oPosition, oForce);
			for( j =0; j < 3; j++){
				if( !isFloatEqual(outputVertexArray[i*3*2+j], oPosition[j], epsilon) ){
					tcLog("Transform Feedback vertex position does not match, vertex %d, coordinate %d, \t TF vertex position = %f, \t expected vextex position = %f \n",i, j, outputVertexArray[i*3*2+j], oPosition[j]);
					ret = 0;
				}
				if( !isFloatEqual(outputVertexArray[i*3*2+3+j], oForce[j], epsilon) ){
					tcLog("Transform Feedback force does not match, vertex %d, coordinate %d, \t TF force = %f, \t expected force = %f \n",i, j, outputVertexArray[i*3*2+3+j], oForce[j]);
					ret = 0;
				}
			}

		}

		//unmap the buffers
		gl->glUnmapBuffer(GL_ARRAY_BUFFER);
		CHECK_GL_ERROR;
		gl->glUnmapBuffer(GL_TRANSFORM_FEEDBACK_BUFFER);
		CHECK_GL_ERROR;
	}
/////////////////////////////////////////////////////
// Results Verification: End
/////////////////////////////////////////////////////

	gl->glDisable(GL_RASTERIZER_DISCARD);
	CHECK_GL_ERROR;

	gl->glEnable(GL_DEPTH_TEST);
	CHECK_GL_ERROR;

	gl->glEnable(GL_CULL_FACE);
	CHECK_GL_ERROR;

	gl->glBindBuffer(GL_ARRAY_BUFFER, 0);
	gl->glDisableVertexAttribArray(gld->m_indexPositionTF);
	gl->glDisableVertexAttribArray(gld->m_indexForce);

	// Exchange buffers. m_feedbackBuffer[0] will have transformed vertices after exchange and will be used for rendering
	tmpbuffer = gld->m_feedbackBuffer[1];
	gld->m_feedbackBuffer[1] = gld->m_feedbackBuffer[0];
	gld->m_feedbackBuffer[0] = tmpbuffer;

finish:
	return ret;
}


////////////////////////////////////////////////
// Function to render the transformed vertices
////////////////////////////////////////////////
int
tfRender(GLData *gld)
{

	int ret=1;
  Evas_GL_API *gl = gld->glapi;

	gl->glUseProgram(gld->m_renderProgramObject);

	gl->glViewport(0, 0, gld->m_width, gld->m_height);
	CHECK_GL_ERROR;

	gl->glClearColor(1.0f, 0.0f, 0.0f, 1.0f);
	gl->glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	CHECK_GL_ERROR;

	gl->glDisable(GL_DEPTH_TEST);
	CHECK_GL_ERROR;

	gl->glEnable(GL_BLEND);
	CHECK_GL_ERROR;

	gl->glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	CHECK_GL_ERROR;

	// Use transform feedback buffer 0 for rendering as it has updated vertices position
	gl->glBindBuffer(GL_ARRAY_BUFFER, gld->m_feedbackBuffer[0]);
	CHECK_GL_ERROR;
	gl->glEnableVertexAttribArray(gld->m_indexPositionR);
	CHECK_GL_ERROR;
	gl->glVertexAttribPointer(gld->m_indexPositionR, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(GLfloat), 0);
	CHECK_GL_ERROR;

	gl->glUniform1i(gld->m_indexTextureSample, 0);
	CHECK_GL_ERROR;

	gl->glBindTexture(GL_TEXTURE_2D, gld->m_textureId);
	CHECK_GL_ERROR;

	gl->glDrawArrays(GL_POINTS, 0, numVertices);
	CHECK_GL_ERROR;

	gl->glBindBuffer(GL_ARRAY_BUFFER, 0);
	gl->glDisableVertexAttribArray(gld->m_indexPositionR);

	gl->glEnable(GL_DEPTH_TEST);
	CHECK_GL_ERROR;

	gl->glDisable(GL_BLEND);
	CHECK_GL_ERROR;

finish:
	return ret;

}

// Callbacks
// intialize callback that gets called once for intialization
static void
_init_gl(Evas_Object *obj)
{
   GLData *gld = evas_object_data_get(obj, "gld");
   Evas_GL_API *gl = gld->glapi;

	gld->m_width = 720;
	gld->m_height = 1280;

	tfInit_TransformFeedback(gld);
	tfInit_Render( gld);

}

// delete callback gets called when glview is deleted
static void
_del_gl(Evas_Object *obj)
{
   GLData *gld = evas_object_data_get(obj, "gld");
   if (!gld)
     {
        printf("Unable to get GLData. \n");
        return;
     }
   Evas_GL_API *gl = gld->glapi;

/*
   gl->glDeleteShader(gld->vtx_shader);
   gl->glDeleteShader(gld->vtx_FEEDBACKshader);
   gl->glDeleteShader(gld->fgmt_shader);
   gl->glDeleteProgram(gld->program);
   gl->glDeleteBuffers(1, &gld->vbo);
*/
   gl->glDeleteProgram(gld->m_tfProgramObject);
   gl->glDeleteProgram(gld->m_renderProgramObject);

   evas_object_data_del((Evas_Object*)obj, "..gld");
   free(gld);
}

// resize callback gets called every time object is resized
static void
_resize_gl(Evas_Object *obj)
{
   int w, h;
   GLData *gld = evas_object_data_get(obj, "gld");
   Evas_GL_API *gl = gld->glapi;

   elm_glview_size_get(obj, &gld->w, &gld->h);

   // GL Viewport stuff. you can avoid doing this if viewport is all the
   // same as last frame if you want
   gl->glViewport(0, 0, gld->w, gld->h);
}


// draw callback is where all the main GL rendering happens
static void
_draw_gl(Evas_Object *obj)
{
   Evas_GL_API *gl = elm_glview_gl_api_get(obj);
   GLData *gld = evas_object_data_get(obj, "gld");
   if (!gld) return;

	elm_glview_size_get(obj, &gld->w, &gld->h);

	if (!tfUpdate(gld)) {
		printf("tfUpdate failed \n");
//		ret = 0;
//		goto finish;
	}

	if (!tfRender(gld)) {
		printf("tfRender failed \n");
//		ret = 0;
//		goto finish;
	}
}

// just need to notify that glview has changed so it can render
static Eina_Bool
_anim(void *data)
{
   elm_glview_changed_set(data);
   return EINA_TRUE;
}

static void
_on_done(void *data, Evas_Object *obj, void *event_info)
{
   evas_object_del((Evas_Object*)data);
   elm_exit();
}

static void
_del(void *data, Evas *evas, Evas_Object *obj, void *event_info)
{
   Ecore_Animator *ani = evas_object_data_get(obj, "ani");
   ecore_animator_del(ani);
}


EAPI_MAIN int
elm_main(int argc, char **argv)
{
   Evas_Object *win, *bg, *bx, *bt, *gl;
   Ecore_Animator *ani;
   GLData *gld = NULL;

   if (!(gld = calloc(1, sizeof(GLData)))) return 1;

   // set the preferred engine to opengl_x11. if it isnt' available it
   // may use another transparently
   elm_config_preferred_engine_set("opengl_x11");
   
   win = elm_win_add(NULL, "glview simple", ELM_WIN_BASIC);
   elm_win_title_set(win, "GLView Simple");
   elm_win_autodel_set(win, EINA_TRUE);
   elm_policy_set(ELM_POLICY_QUIT, ELM_POLICY_QUIT_LAST_WINDOW_CLOSED);

   bg = elm_bg_add(win);
   elm_win_resize_object_add(win, bg);
   evas_object_size_hint_weight_set(bg, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_show(bg);

   bx = elm_box_add(win);
   evas_object_size_hint_weight_set(bx, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   elm_win_resize_object_add(win, bx);
   evas_object_show(bx);

   //-//-//-// THIS IS WHERE GL INIT STUFF HAPPENS (ALA EGL)
   //-//
   // create a new glview object
   gl = elm_glview_version_add(win,EVAS_GL_GLES_3_X );
   gld->glapi = elm_glview_gl_api_get(gl);
   evas_object_size_hint_align_set(gl, EVAS_HINT_FILL, EVAS_HINT_FILL);
   evas_object_size_hint_weight_set(gl, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   // mode is simply for supporting alpha, depth buffering, and stencil
   // buffering.
   //if(!getenv("TESTAPP_INDIRECT")) 
   elm_glview_mode_set(gl, ELM_GLVIEW_ALPHA /*| ELM_GLVIEW_DEPTH | ELM_GLVIEW_DIRECT*/);
   //else
   //elm_glview_mode_set(gl, ELM_GLVIEW_ALPHA | ELM_GLVIEW_DEPTH);
   // resize policy tells glview what to do with the surface when it
   // resizes.  ELM_GLVIEW_RESIZE_POLICY_RECREATE will tell it to
   // destroy the current surface and recreate it to the new size
   elm_glview_resize_policy_set(gl, ELM_GLVIEW_RESIZE_POLICY_RECREATE);
   // render policy tells glview how it would like glview to render
   // gl code. ELM_GLVIEW_RENDER_POLICY_ON_DEMAND will have the gl
   // calls called in the pixel_get callback, which only gets called
   // if the object is visible, hence ON_DEMAND.  ALWAYS mode renders
   // it despite the visibility of the object.
   elm_glview_render_policy_set(gl, ELM_GLVIEW_RENDER_POLICY_ON_DEMAND);
   // initialize callback function gets registered here
   elm_glview_init_func_set(gl, _init_gl);
   // delete callback function gets registered here
   elm_glview_del_func_set(gl, _del_gl);
   elm_glview_resize_func_set(gl, _resize_gl);
   elm_glview_render_func_set(gl, _draw_gl);
   //-//
   //-//-//-// END GL INIT BLOB

   elm_box_pack_end(bx, gl);
   evas_object_show(gl);

   elm_object_focus_set(gl, EINA_TRUE);

   // animating - just a demo. as long as you trigger an update on the image
   // object via elm_glview_changed_set() it will be updated.
   //
   // NOTE: if you delete gl, this animator will keep running trying to access
   // gl so you'd better delete this animator with ecore_animator_del().
   ani = ecore_animator_add(_anim, gl);

   evas_object_data_set(gl, "ani", ani);
   evas_object_data_set(gl, "gld", gld);
   evas_object_event_callback_add(gl, EVAS_CALLBACK_DEL, _del, gl);

   // add an 'OK' button to end the program
   bt = elm_button_add(win);
   elm_object_text_set(bt, "OK");
   evas_object_size_hint_align_set(bt, EVAS_HINT_FILL, EVAS_HINT_FILL);
   evas_object_size_hint_weight_set(bt, EVAS_HINT_EXPAND, 0.0);
   elm_box_pack_end(bx, bt);
   evas_object_show(bt);
   evas_object_smart_callback_add(bt, "clicked", _on_done, win);

   evas_object_resize(win, 320, 480);
   evas_object_show(win);

   // run the mainloop and process events and callbacks
   elm_run();
   elm_shutdown();

   return 0;
}
ELM_MAIN()
