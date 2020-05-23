typedef float GLfloat;
typedef int GLboolean;
typedef int GLenum;
typedef int GLint;
typedef int GLsizei;
typedef short GLshort;
typedef unsigned char GLubyte;
typedef unsigned int GLuint;
typedef unsigned short GLushort;
typedef void GLvoid;
typedef int GLsizeiptr;
typedef int GLintptr;
typedef unsigned int GLbitfield;

#define GL_FALSE 0
#define GL_TRUE 1



// Hopefully each of these is unique..
#define GL_ALPHA_TEST 10001

#define GL_ALWAYS 10002

#define GL_AMBIENT 10003

#define GL_BACK 10004

#define GL_CCW 10005

#define GL_CLAMP_TO_EDGE 10006

#define GL_CLIP_PLANE0 10007
#define GL_CLIP_PLANE1 10008
#define GL_CLIP_PLANE2 10009

#define GL_COLOR_BUFFER_BIT 10010

#define GL_COLOR_MATERIAL 10011

#define GL_CULL_FACE 10012

#define GL_CW 10013

#define GL_DEPTH_BUFFER_BIT 10014
#define GL_DEPTH_TEST 10015

#define GL_DIFFUSE 10016
#define GL_EMISSION 10017

#define GL_EQUAL 10018

#define GL_EXTENSIONS 10019

#define GL_REPEAT 10020

#define GL_FLOAT 10021

#define GL_FRONT 10022
#define GL_FRONT_AND_BACK 10023

#define GL_GEQUAL 10024
#define GL_GREATER 10025

#define GL_KEEP 10026

#define GL_LEQUAL 10027
#define GL_LESS 10028

#define GL_STENCIL_TEST 10029

#define GL_LIGHTING 10030
#define GL_LIGHT_MODEL_AMBIENT 10031
#define GL_LIGHT_MODEL_TWO_SIDE 10032

#define GL_LINEAR 10033

#define GL_LUMINANCE 10034
#define GL_LUMINANCE_ALPHA 10035

#define GL_MAX_TEXTURE_SIZE 10036
#define GL_MAX_TEXTURE_UNITS 10037

#define GL_MODELVIEW 10038

#define GL_NEVER 10039

#define GL_NORMAL_ARRAY 10040

#define GL_NOTEQUAL 10041

#define GL_ONE 10042
#define GL_ONE_MINUS_SRC_ALPHA 10043

#define GL_PACK_ALIGNMENT 10044

#define GL_POINTS 10045

#define GL_POLYGON_OFFSET_FILL 10046

#define GL_POSITION 10047

#define GL_PROJECTION 10048

#define GL_RENDERER 10049

#define GL_REPLACE 10050

#define GL_RGB 10051
#define GL_RGBA 10052

#define GL_SHININESS 10053
#define GL_SPECULAR 10054
#define GL_SRC_ALPHA 10055

#define GL_STENCIL_BUFFER_BIT 10056

#define GL_TEXTURE 10057
#define GL_TEXTURE_2D 10058
#define GL_TEXTURE_COORD_ARRAY 10059

#define GL_TEXTURE_MAG_FILTER 10060
#define GL_TEXTURE_MIN_FILTER 10061
#define GL_TEXTURE_WRAP_S 10062
#define GL_TEXTURE_WRAP_T 10063

#define GL_TRIANGLES 10064
#define GL_TRIANGLE_STRIP 10065

#define GL_UNPACK_ALIGNMENT 10066

#define GL_UNSIGNED_BYTE 10067
#define GL_UNSIGNED_SHORT 10068

#define GL_VENDOR 10069
#define GL_VERSION 10070

#define GL_VERTEX_ARRAY 10071
#define GL_COLOR_ARRAY 10072

#define GL_SHORT 10073

#define GL_NORMALIZE 10074
#define GL_BLEND 10075

#define GL_LIGHT0 10076
#define GL_LIGHT1 10077
#define GL_LIGHT2 10078
#define GL_LIGHT3 10079
#define GL_LIGHT4 10080
#define GL_LIGHT5 10081
#define GL_LIGHT6 10082
#define GL_LIGHT7 10083
//FIXME: This also needs to reserve many other slots until at least GL_LIGHT7

#define GL_NEAREST 10084

#define GL_OPERAND0_ALPHA 10085

#define GL_OPERAND0_RGB 10086
#define GL_OPERAND1_RGB 10087
#define GL_OPERAND2_RGB 10088

#define GL_PREVIOUS 10089
#define GL_PRIMARY_COLOR 10090

#define GL_SRC_COLOR 10091

#define GL_TEXTURE_ENV 10092
#define GL_TEXTURE_ENV_MODE 10093

#define GL_COMBINE 10094
#define GL_COMBINE_ALPHA 10095
#define GL_COMBINE_RGB 10096

#define GL_INTERPOLATE 10097
#define GL_MODULATE 10098



// Not used in neverball, only for completeness
#define GL_ONE_MINUS_SRC_COLOR 20000
#define GL_SOURCE_ALPHA 20001
#define GL_CONSTANT 20002
#define GL_ALPHA 20003
#define GL_NEAREST_MIPMAP_LINEAR 20004



// Not used in neverball, only for debugging
#define GL_MATRIX_MODE 30000
#define GL_MODELVIEW_MATRIX 30001
#define GL_PROJECTION_MATRIX 30002



// Used in neverball conditionally
#define GL_GENERATE_MIPMAP_SGIS 25000
#define GL_LINEAR_MIPMAP_LINEAR 25001



// Defined in Neverball share/glext.h
#if 0
#define GL_COLOR_ATTACHMENT0          0x8CE0
#define GL_COMPILE_STATUS             0x8B81
#define GL_DEPTH24_STENCIL8           0x88F0
#define GL_DEPTH_ATTACHMENT           0x8D00
#define GL_DEPTH_STENCIL              0x84F9
#define GL_DYNAMIC_DRAW               0x88E8
#define GL_FRAGMENT_SHADER            0x8B30
#define GL_FRAMEBUFFER                0x8D40
#define GL_FRAMEBUFFER_COMPLETE       0x8CD5
#define GL_INFO_LOG_LENGTH            0x8B84
#define GL_LINK_STATUS                0x8B82
#define GL_MULTISAMPLE                0x809D
#define GL_POINT_DISTANCE_ATTENUATION 0x8129
#define GL_POINT_SIZE_MAX             0x8127
#define GL_POINT_SIZE_MIN             0x8126
#define GL_STATIC_DRAW                0x88E4
#define GL_STENCIL_ATTACHMENT         0x8D20
#define GL_UNSIGNED_INT_24_8          0x84FA
#define GL_VERTEX_SHADER              0x8B31
#else
#define GL_TEXTURE0                   0x84C0
#define GL_TEXTURE1                   0x84C1
#define GL_TEXTURE2                   0x84C2
#define GL_SRC0_RGB                   0x8580
#define GL_SRC1_RGB                   0x8581
#define GL_SRC2_RGB                   0x8582
#define GL_SRC0_ALPHA                 0x8588
#define GL_POINT_SPRITE               0x8861
#define GL_COORD_REPLACE              0x8862
#define GL_ARRAY_BUFFER               0x8892
#define GL_ELEMENT_ARRAY_BUFFER       0x8893
#endif




// Funtion specifiers
#define GL_API
#define GL_APIENTRY


// Getters
GL_API void GL_APIENTRY glGetIntegerv (GLenum pname, GLint *data);
GL_API const GLubyte *GL_APIENTRY glGetString (GLenum name);

// Clearing
GL_API void GL_APIENTRY glClear (GLbitfield mask);
GL_API void GL_APIENTRY glClearColor (GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha);

// Buffers
GL_API void GL_APIENTRY glGenBuffers (GLsizei n, GLuint *buffers);
GL_API void GL_APIENTRY glBindBuffer (GLenum target, GLuint buffer);
GL_API void GL_APIENTRY glBufferData (GLenum target, GLsizeiptr size, const void *data, GLenum usage);
GL_API void GL_APIENTRY glBufferSubData (GLenum target, GLintptr offset, GLsizeiptr size, const void *data);
GL_API void GL_APIENTRY glDeleteBuffers (GLsizei n, const GLuint *buffers);

// Vertex buffers
GL_API void GL_APIENTRY glTexCoordPointer (GLint size, GLenum type, GLsizei stride, const void *pointer);
GL_API void GL_APIENTRY glColorPointer (GLint size, GLenum type, GLsizei stride, const void *pointer);
GL_API void GL_APIENTRY glNormalPointer (GLenum type, GLsizei stride, const void *pointer);
GL_API void GL_APIENTRY glVertexPointer (GLint size, GLenum type, GLsizei stride, const void *pointer);

// Draw calls
GL_API void GL_APIENTRY glDrawArrays (GLenum mode, GLint first, GLsizei count);
GL_API void GL_APIENTRY glDrawElements (GLenum mode, GLsizei count, GLenum type, const void *indices);

// Matrix functions
GL_API void GL_APIENTRY glMatrixMode (GLenum mode);
GL_API void GL_APIENTRY glLoadIdentity (void);
GL_API void GL_APIENTRY glMultMatrixf (const GLfloat *m);
//#define glMultMatrixf(m) { debugPrint("%s:%d\n", __FILE__, __LINE__); glMultMatrixf(m); }
GL_API void GL_APIENTRY glOrthof (GLfloat l, GLfloat r, GLfloat b, GLfloat t, GLfloat n, GLfloat f);
GL_API void GL_APIENTRY glTranslatef (GLfloat x, GLfloat y, GLfloat z);
GL_API void GL_APIENTRY glRotatef (GLfloat angle, GLfloat x, GLfloat y, GLfloat z);
GL_API void GL_APIENTRY glScalef (GLfloat x, GLfloat y, GLfloat z);
GL_API void GL_APIENTRY glPopMatrix (void);
GL_API void GL_APIENTRY glPushMatrix (void);

// Framebuffer setup
GL_API void GL_APIENTRY glViewport (GLint x, GLint y, GLsizei width, GLsizei height);

// Textures
GL_API void GL_APIENTRY glGenTextures (GLsizei n, GLuint *textures);
GL_API void GL_APIENTRY glBindTexture (GLenum target, GLuint texture);
GL_API void GL_APIENTRY glActiveTexture (GLenum texture);
GL_API void GL_APIENTRY glClientActiveTexture (GLenum texture);
GL_API void GL_APIENTRY glDeleteTextures (GLsizei n, const GLuint *textures);
GL_API void GL_APIENTRY glTexImage2D (GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type, const void *pixels);
GL_API void GL_APIENTRY glTexParameteri (GLenum target, GLenum pname, GLint param);

// Renderstates
GL_API void GL_APIENTRY glAlphaFunc (GLenum func, GLfloat ref);
GL_API void GL_APIENTRY glBlendFunc (GLenum sfactor, GLenum dfactor);
GL_API void GL_APIENTRY glClipPlanef (GLenum p, const GLfloat *eqn);
GL_API void GL_APIENTRY glColor4ub (GLubyte red, GLubyte green, GLubyte blue, GLubyte alpha);
GL_API void GL_APIENTRY glColor4f (GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha);
GL_API void GL_APIENTRY glColorMask (GLboolean red, GLboolean green, GLboolean blue, GLboolean alpha);
GL_API void GL_APIENTRY glDepthFunc (GLenum func);
GL_API void GL_APIENTRY glDepthMask (GLboolean flag);
GL_API void GL_APIENTRY glEnable (GLenum cap);
GL_API void GL_APIENTRY glDisable (GLenum cap);
GL_API void GL_APIENTRY glEnableClientState (GLenum array);
GL_API void GL_APIENTRY glDisableClientState (GLenum array);
GL_API void GL_APIENTRY glCullFace (GLenum mode);
GL_API void GL_APIENTRY glFrontFace (GLenum mode);

// Stencil actions
GL_API void GL_APIENTRY glStencilFunc (GLenum func, GLint ref, GLuint mask);
GL_API void GL_APIENTRY glStencilOp (GLenum fail, GLenum zfail, GLenum zpass);

// Misc.
GL_API void GL_APIENTRY glPointParameterf (GLenum pname, GLfloat param);
GL_API void GL_APIENTRY glPointParameterfv (GLenum pname, const GLfloat *params);
GL_API void GL_APIENTRY glPointSize (GLfloat size);
GL_API void GL_APIENTRY glPolygonOffset (GLfloat factor, GLfloat units);

// TexEnv
GL_API void GL_APIENTRY glTexEnvi (GLenum target, GLenum pname, GLint param);

// Pixel readback
GL_API void GL_APIENTRY glReadPixels (GLint x, GLint y, GLsizei width, GLsizei height, GLenum format, GLenum type, void *pixels);

// Lighting
GL_API void GL_APIENTRY glLightModelf (GLenum pname, GLfloat param);
GL_API void GL_APIENTRY glLightModelfv (GLenum pname, const GLfloat *params);
GL_API void GL_APIENTRY glLightfv (GLenum light, GLenum pname, const GLfloat *params);

// Materials
GL_API void GL_APIENTRY glMaterialfv (GLenum face, GLenum pname, const GLfloat *params);

// Pixel pushing
GL_API void GL_APIENTRY glPixelStorei (GLenum pname, GLint param);

// Not used in neverball, only for debugging
GL_API void GL_APIENTRY glGetFloatv (GLenum pname, GLfloat *data);







// Not in GLES, but added for Xbox
#define GL_TEXTURE_GEN_MODE 40000
#define GL_SPHERE_MAP 40001
#define GL_S 40002
#define GL_T 40003
#define GL_TEXTURE_GEN_S 40004
#define GL_TEXTURE_GEN_T 40005
GL_API void GL_APIENTRY glTexGeni (GLenum coord, GLenum pname, GLint param);



// Backport from nv2a-re
#define GL_AMBIENT_AND_DIFFUSE 60
#define GL_CONSTANT_ATTENUATION 100
#define GL_LINEAR_ATTENUATION 110
#define GL_QUADRATIC_ATTENUATION 120
#define GL_SPOT_EXPONENT 130
#define GL_SPOT_CUTOFF 140
#define GL_SPOT_DIRECTION 150
#define GL_LIGHT_MODEL_LOCAL_VIEWER 210
