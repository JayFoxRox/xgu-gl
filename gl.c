#include <hal/debug.h>
#include <stdarg.h>
#include <stdio.h>

#include <stdbool.h>
#include <assert.h>

#include <SDL.h>

#include <windows.h>

#include "swizzle.h"

static float viewport_matrix[4*4];

#define MAX(a, b) (((a) > (b)) ? (a) : (b))

static float _max(float a, float b) {
  return (a > b) ? a : b;
}

static void _mul3(float* v, const float* a, const float* b) {
  v[0] = a[0]*b[0];
  v[1] = a[1]*b[1];
  v[2] = a[2]*b[2];
}

static float _dot3(const float* a, const float* b) {
  return a[0]*b[0]+a[1]*b[1]+a[2]*b[2];
}

static float _dot4(const float* a, const float* b) {
  return _dot3(a,b)+a[3]*b[3];
}

static void _normalize(float* v) {
  float l = sqrtf(_dot3(v,v));
  v[0] /= l;
  v[1] /= l;
  v[2] /= l;
}

static void transposeMatrix(float* out, const float* in) {
  float t[4*4];
  for(int i = 0; i < 4; i++) {
    for(int j = 0; j < 4; j++) {
      t[i*4+j] = in[i+4*j];
    }
  } 
  memcpy(out, t, sizeof(t));
}

static void mult_vec4_mat4(float* o, const float* m, const float* v) {
  float t[4*4];
  //memcpy(t, m, sizeof(t));
  transposeMatrix(t, m);
  //printf("%f\n", t[0*4+3]);
  //printf("%f\n", t[1*4+3]);
  //printf("%f\n", t[2*4+3]);
  //printf("%f\n", t[3*4+3]);
  o[0] = _dot4(&t[0*4], v);
  o[1] = _dot4(&t[1*4], v);
  o[2] = _dot4(&t[2*4], v);
  o[3] = _dot4(&t[3*4], v);
}

static unsigned int frame = 0; //FIXME: Remove
static SDL_GameController* g = NULL;

#include <hal/debug.h>
static void _unimplemented(const char* fmt, ...) {
  char buf[1024];
  va_list va;
  va_start(va, fmt);
  vsprintf(buf, fmt, va);
  va_end(va);
  printf(buf); // Log to disk
  debugPrint(buf); // Log to display
}
#define unimplemented(fmt, ...) { \
  static bool encountered = false; \
  if (!encountered) { \
    _unimplemented("%s (%s:%d) " fmt "\n", __FUNCTION__, __FILE__, __LINE__, __VA_ARGS__); \
    encountered = true; \
  } \
}

#define _debugPrint(...) debugPrint(__VA_ARGS__)

#if 0
void debugPrintFloat(float f) {
#if 0
  //FIXME: pdclib can't do this
  debugPrint("%f", f);
#endif
#if 0
  debugPrint("0x%08X", *(uint32_t*)&f);
#endif
#if 1
  bool sign = f < 0.0f;
  if (sign) { f = -f; }
  unsigned int value = (unsigned int)(f * 10000);
  unsigned int mantissa = value % 10000;
  value /= 10000;
  debugPrint("%s%u.%04u", sign ? "-" : "", value, mantissa);
#endif
}

#else
void debugPrintFloat(float f) {}
#define debugPrint(...) 0
#endif

// xgu.h
#include "xgu/xgu.h"
#include "xgu/xgux.h"


// GLES/gl.h
#include <stdint.h>
#include <string.h>
#include <pbkit/pbkit.h>

#include "GLES/gl.h"
//#include <xgu/xgu.h>


static void* AllocateResourceMemory(size_t size) {
#define MAXRAM 0x03FFAFFF
  void* addr = MmAllocateContiguousMemoryEx(size, 0, MAXRAM, 0, PAGE_READWRITE | PAGE_WRITECOMBINE);
  assert(addr != NULL);
  return addr;
}

static void FreeResourceMemory(void* ptr) {
  MmFreeContiguousMemory(ptr);
}

typedef struct {
  void* data;
} Object;

typedef struct {
  uint8_t* data;
  size_t size;
} Buffer;
#define DEFAULT_BUFFER() \
  { \
    .data = NULL, \
    .size = 0 \
  }

typedef struct {
  GLsizei width;
  unsigned int width_shift;
  GLsizei height;
  unsigned int height_shift;
  size_t pitch;
  void* data;
  GLint min_filter;
  GLint mag_filter;
  GLint wrap_s;
  GLint wrap_t;
  GLenum internal_base_format;
} Texture;

typedef struct {
  GLenum env_mode;
  GLfloat env_color[4];
  GLenum combine_rgb;
  GLenum combine_alpha;
  GLenum src_rgb[3];
  GLenum src_operand_rgb[3];
  GLenum src_alpha[3];
  GLenum src_operand_alpha[3];
} TexEnv;

//FIXME: Allow different pools
static Object* objects = NULL;
static GLuint object_count = 0;

static GLuint unused_object() {
  //FIXME: Find object slot where data is NULL
  object_count += 1;
  objects = realloc(objects, sizeof(Object) * object_count);
  return object_count - 1;
}

static void gen_objects(GLsizei n, GLuint* indices, const void* data, size_t size) {
  for(int i = 0; i < n; i++) {
    GLuint index = unused_object();

    Object* object = &objects[index];
    object->data = malloc(size);
    memcpy(object->data, data, size);

    indices[i] = 1 + index;
  }
}

static void del_objects(GLsizei n, const GLuint* indices) {
  for(int i = 0; i < n; i++) {

    //FIXME: Also ignore non-existing names
    if (indices[i] == 0) {
      continue;
    }

    GLuint index = indices[i] - 1;

    Object* object = &objects[index];

    //FIXME: Call a handler routine?

    assert(object->data != NULL);
    free(object->data);
    object->data = NULL;
  }
}

#define ARRAY_SIZE(x) (sizeof(x) / sizeof(x[0]))
#define MASK(mask, val) (((val) << (__builtin_ffs(mask)-1)) & (mask))

typedef struct {
  struct {
    bool enabled;
    GLenum gl_type;
    unsigned int size;
    size_t stride;
    const void* data;
  } array;
  float value[4];
} Attrib;

#define DEFAULT_TEXENV() \
  { \
    .env_mode = GL_MODULATE, \
    .src_rgb = { GL_TEXTURE, GL_PREVIOUS, GL_CONSTANT }, \
    .src_alpha = { GL_TEXTURE, GL_PREVIOUS, GL_CONSTANT }, \
    .src_operand_rgb = { GL_SRC_COLOR, GL_SRC_COLOR, GL_SRC_ALPHA}, \
    .src_operand_alpha = { GL_SRC_ALPHA, GL_SRC_ALPHA, GL_SRC_ALPHA }, \
    .env_color = { 0.0f, 0.0f, 0.0f, 0.0f }, \
    /*.scale_rgb = 1.0f,*/ \
    /*.scale_alpha = 1.0f*/ \
  }

static unsigned int active_texture = 0;
static unsigned int client_active_texture = 0;

TexEnv texenvs[4] = {
  DEFAULT_TEXENV(),
  DEFAULT_TEXENV(),
  DEFAULT_TEXENV(),
  DEFAULT_TEXENV()
};

typedef struct {
  bool enabled;
  bool is_dir;
  struct {
    float x;
    float y;
    float z;
    float w;
  } position;
  struct {
    float x;
    float y;
    float z;
  } spot_direction;
  float spot_exponent;
  float spot_cutoff;
  struct {
    float r;
    float g;
    float b;
    float a;
  } diffuse;
  struct {
    float r;
    float g;
    float b;
    float a;
  } specular;
  struct {
    float r;
    float g;
    float b;
    float a;
  } ambient;
  float constant_attenuation;
  float linear_attenuation;
  float quadratic_attenuation;
} Light;

#define GL_MAX_LIGHTS 8

typedef struct {
  Attrib texture_coord_array[4];
  Attrib vertex_array;
  Attrib color_array;
  Attrib normal_array;
  bool texgen_s_enabled[4];
  bool texgen_t_enabled[4];
  GLenum texgen_s[4];
  GLenum texgen_t[4];
  bool texture_2d[4]; //FIXME: Move into a texture unit array
  GLuint texture_binding_2d[4];
  Light lights[GL_MAX_LIGHTS]; //FIXME: no more needed in neverball
  bool color_material_enabled;
  GLenum color_material_front;
  GLenum color_material_back;
  struct {
    float r;
    float g;
    float b;
    float a; //FIXME: Unused?!
  } light_model_ambient;
  struct {
    struct {
      float r;
      float g;
      float b;
      float a; //FIXME: Unused?!
    } emission;
    struct {
      float r;
      float g;
      float b;
      float a; //FIXME: Unused?!
    } ambient;
    struct {
      float r;
      float g;
      float b;
      float a; //FIXME: Unused?!
    } diffuse;
    struct {
      float r;
      float g;
      float b;
      float a; //FIXME: Unused?!
    } specular;
    float shininess;
  } material[2];
} State;
static State state = {
  .texture_2d = { false, false, false, false },
  .texture_binding_2d = { 0, 0, 0, 0 }
};

typedef struct {
  bool enabled;
  float x;
  float y;
  float z;
  float w;
} ClipPlane;
static ClipPlane clip_planes[3]; //FIXME: No more needed for neverball

#include "matrix.h"


static uint32_t* set_enabled(uint32_t* p, GLenum cap, bool enabled) {
  switch(cap) {
  case GL_CLIP_PLANE0 ... GL_CLIP_PLANE0+3-1: //FIXME: Find right GL constant
    clip_planes[cap - GL_CLIP_PLANE0].enabled = enabled;
    break;
  case GL_TEXTURE_GEN_S:
    state.texgen_s_enabled[active_texture] = enabled;
    break;
  case GL_TEXTURE_GEN_T:
    state.texgen_t_enabled[active_texture] = enabled;
    break;
  case GL_ALPHA_TEST:
    p = xgu_set_alpha_test_enable(p, enabled);
    break;
  case GL_NORMALIZE:
    //FIXME: Needs more changes to matrices?
    p = xgu_set_normalization_enable(p, enabled);
    break;
  case GL_CULL_FACE:
    p = xgu_set_cull_face_enable(p, enabled);
    break;
  case GL_DEPTH_TEST:
    p = xgu_set_depth_test_enable(p, enabled);
    break;
  case GL_STENCIL_TEST:
    p = xgu_set_stencil_test_enable(p, enabled);
    break;
  case GL_TEXTURE_2D:
    state.texture_2d[active_texture] = enabled;
    break;
  case GL_BLEND:
    p = xgu_set_blend_enable(p, enabled);
    break;
  case GL_LIGHT0 ... GL_LIGHT0+GL_MAX_LIGHTS-1:
    state.lights[cap - GL_LIGHT0].enabled = enabled;
    break;
  case GL_LIGHTING:
    p = xgu_set_lighting_enable(p, enabled);
    break;
  case GL_COLOR_MATERIAL:
    state.color_material_enabled = enabled;
    break;
  case GL_POLYGON_OFFSET_FILL:
    unimplemented(); //FIXME: !!!
    break;   
  case GL_POINT_SPRITE: 
    unimplemented(); //FIXME: !!!
    break;   
  default:
    unimplemented("%d", cap);
    assert(false);
    break;
  }
  return p;
}

static void set_client_state_enabled(GLenum array, bool enabled) {
  switch(array) {
  case GL_TEXTURE_COORD_ARRAY:
    state.texture_coord_array[client_active_texture].array.enabled = enabled;
    break;
  case GL_VERTEX_ARRAY:
    state.vertex_array.array.enabled = enabled;
    break;
  case GL_COLOR_ARRAY:
    state.color_array.array.enabled = enabled;
    break;
  case GL_NORMAL_ARRAY:
    state.normal_array.array.enabled = enabled;
    break;
  default:
    unimplemented("%d", array);
    assert(false);
    break;
  }
}

static XguPrimitiveType gl_to_xgu_primitive_type(GLenum mode) {
  switch(mode) {
  case GL_POINTS:         return XGU_POINTS;
  case GL_TRIANGLES:      return XGU_TRIANGLES;
  case GL_TRIANGLE_STRIP: return XGU_TRIANGLE_STRIP;
  default:
    unimplemented("%d", mode);
    assert(false);
    return -1;
  }
}

typedef unsigned int XguTextureFilter;
 //FIXME: Shitty workaround for XGU
#define XGU_TEXTURE_FILTER_NEAREST 1
#define XGU_TEXTURE_FILTER_LINEAR 2
#define XGU_TEXTURE_FILTER_NEAREST_MIPMAP_NEAREST 3
#define XGU_TEXTURE_FILTER_LINEAR_MIPMAP_NEAREST 4
#define XGU_TEXTURE_FILTER_NEAREST_MIPMAP_LINEAR 5
#define XGU_TEXTURE_FILTER_LINEAR_MIPMAP_LINEAR 6

static XguTextureFilter gl_to_xgu_texture_filter(GLenum filter) {
  switch(filter) {
  case GL_NEAREST:               return XGU_TEXTURE_FILTER_NEAREST;
  case GL_LINEAR:                return XGU_TEXTURE_FILTER_LINEAR;
  case GL_NEAREST_MIPMAP_LINEAR: return XGU_TEXTURE_FILTER_NEAREST_MIPMAP_LINEAR;
  case GL_LINEAR_MIPMAP_LINEAR:  return XGU_TEXTURE_FILTER_LINEAR_MIPMAP_LINEAR;
  default:
    unimplemented("%d", filter);
    assert(false);
    return -1;
  }
}


static XguStencilOp gl_to_xgu_stencil_op(GLenum op) {
  switch(op) {
  case GL_KEEP: return XGU_STENCIL_OP_KEEP;
  case GL_REPLACE: return XGU_STENCIL_OP_REPLACE;
//FIXME:  case GL_INCR: return XGU_STENCIL_OP_INCR;
//FIXME:  case GL_DECR: return XGU_STENCIL_OP_DECR;
  default:
    unimplemented("%d", op);
    assert(false);
    return -1;
  }
}

static XguFuncType gl_to_xgu_func_type(GLenum func) {
  switch(func) {
  case GL_NEVER:    return XGU_FUNC_NEVER;
  case GL_LESS:     return XGU_FUNC_LESS;
  case GL_EQUAL:    return XGU_FUNC_EQUAL;
  case GL_LEQUAL:   return XGU_FUNC_LESS_OR_EQUAL;
  case GL_GREATER:  return XGU_FUNC_GREATER;
  case GL_NOTEQUAL: return XGU_FUNC_NOT_EQUAL;
  case GL_GEQUAL:   return XGU_FUNC_GREATER_OR_EQUAL;
  case GL_ALWAYS:   return XGU_FUNC_ALWAYS;
  default:
    unimplemented("%d", func);
    assert(false);
    return -1;
  }
}

static XguCullFace gl_to_xgu_cull_face(GLenum mode) {
  switch(mode) {
  case GL_FRONT:          return XGU_CULL_FRONT;
  case GL_BACK:           return XGU_CULL_BACK;
  case GL_FRONT_AND_BACK: return XGU_CULL_FRONT_AND_BACK;
  default:
    unimplemented("%d", mode);
    assert(false);
    return -1;
  }
}

static XguFrontFace gl_to_xgu_front_face(GLenum mode) {
  switch(mode) {
  //FIXME: Why do I have to invert this?!
  case GL_CW:  return XGU_FRONT_CCW;
  case GL_CCW: return XGU_FRONT_CW;
  default:
    unimplemented("%d", mode);
    assert(false);
    return -1;
  }
}

static XguVertexArrayType gl_to_xgu_vertex_array_type(GLenum mode) {
  switch(mode) {
  case GL_FLOAT:         return XGU_FLOAT;
  case GL_SHORT:         return NV097_SET_VERTEX_DATA_ARRAY_FORMAT_TYPE_S32K; //FIXME: NV097_SET_VERTEX_DATA_ARRAY_FORMAT_TYPE_S1; for normals?
  case GL_UNSIGNED_BYTE: return NV097_SET_VERTEX_DATA_ARRAY_FORMAT_TYPE_UB_OGL; //FIXME XGU_U8_XYZW ?
  default:
    unimplemented("%d", mode);
    assert(false);
    return -1;
  }
}

static XguBlendFactor gl_to_xgu_blend_factor(GLenum factor) {
  switch(factor) {
  case GL_ONE:                 return XGU_FACTOR_ONE;
  case GL_SRC_ALPHA:           return XGU_FACTOR_SRC_ALPHA;
  case GL_ONE_MINUS_SRC_ALPHA: return XGU_FACTOR_ONE_MINUS_SRC_ALPHA;
  default:
    unimplemented("%d", factor);
    assert(false);
    return -1;
  }
}

static uint8_t f_to_u8(float f) {
  assert(f >= 0.0f && f <= 1.0f);
  return f * 0xFF;
}

// 3.7.10 ES 1.1 FULL
#define DEFAULT_TEXTURE() \
  { \
    /* FIXME: Set up other fields */ \
    .width = 0, \
    .height = 0, \
    .pitch = 0, \
    .data = NULL, \
    .min_filter = GL_NEAREST_MIPMAP_LINEAR, \
    .mag_filter = GL_LINEAR, \
    .wrap_s = GL_REPEAT, \
    .wrap_t = GL_REPEAT, \
    .internal_base_format = 1, \
    /* .generate_mipmaps = false */ \
  }

static Texture zero_texture = DEFAULT_TEXTURE();

static Texture* get_bound_texture(GLuint stage) {
  GLuint texture = state.texture_binding_2d[stage];

  if (texture == 0) {
    return &zero_texture;
  }

  Texture* object = objects[texture - 1].data;
  assert(object != NULL);

  return object;
}

#define _RC_ZERO 0x0
#define _RC_DISCARD 0x0
#define _RC_PRIMARY_COLOR 0x4
#define _RC_TEXTURE 0x8
#define _RC_SPARE0 0xC

#define _RC_UNSIGNED 0x0
#define _RC_UNSIGNED_INVERT 0x1


static uint32_t* setup_texenv_output(uint32_t* p,
                                     TexEnv* t, unsigned int texture, unsigned int stage, bool rgb,
                                     bool ab, bool cd, bool sum) {
  if (rgb) {
    p = pb_push1(p, NV097_SET_COMBINER_COLOR_OCW + stage * 4,
      MASK(NV097_SET_COMBINER_COLOR_OCW_AB_DST, ab ? _RC_SPARE0 : _RC_DISCARD)
      | MASK(NV097_SET_COMBINER_COLOR_OCW_CD_DST, cd ? _RC_SPARE0 : _RC_DISCARD)
      | MASK(NV097_SET_COMBINER_COLOR_OCW_SUM_DST, sum ? _RC_SPARE0 : _RC_DISCARD)
      | MASK(NV097_SET_COMBINER_COLOR_OCW_MUX_ENABLE, 0)
      | MASK(NV097_SET_COMBINER_COLOR_OCW_AB_DOT_ENABLE, 0)
      | MASK(NV097_SET_COMBINER_COLOR_OCW_CD_DOT_ENABLE, 0)
      | MASK(NV097_SET_COMBINER_COLOR_OCW_OP, NV097_SET_COMBINER_COLOR_OCW_OP_NOSHIFT));
  } else {
    p = pb_push1(p, NV097_SET_COMBINER_ALPHA_OCW + stage * 4,
      MASK(NV097_SET_COMBINER_ALPHA_OCW_AB_DST, ab ? _RC_SPARE0 : _RC_DISCARD)
      | MASK(NV097_SET_COMBINER_ALPHA_OCW_CD_DST, cd ? _RC_SPARE0 : _RC_DISCARD)
      | MASK(NV097_SET_COMBINER_ALPHA_OCW_SUM_DST, sum ? _RC_SPARE0 : _RC_DISCARD)
      | MASK(NV097_SET_COMBINER_ALPHA_OCW_MUX_ENABLE, 0)
      | MASK(NV097_SET_COMBINER_ALPHA_OCW_OP, NV097_SET_COMBINER_ALPHA_OCW_OP_NOSHIFT));
  }
  return p;
}

static unsigned int gl_to_texenv_src(TexEnv* t, unsigned int texture, bool rgb, int arg) {
  GLenum src = rgb ? t->src_rgb[arg] : t->src_alpha[arg];

  //FIXME: Move to function?
  switch(src) {
  case GL_PREVIOUS:
    if (texture == 0) {
      return _RC_PRIMARY_COLOR;
    }
    return _RC_SPARE0;
  case GL_PRIMARY_COLOR: return _RC_PRIMARY_COLOR; //FIXME: Name?
  case GL_TEXTURE:       return _RC_TEXTURE+texture; //FIXME: Get stage
  default:
    unimplemented("%d", src);
    assert(false);
    return gl_to_texenv_src(t, texture, rgb, -1);
  }
}

static unsigned int is_texenv_src_alpha(TexEnv* t, unsigned int texture, bool rgb, int arg) {
  GLenum operand = rgb ? t->src_operand_rgb[arg] : t->src_operand_alpha[arg];

  // Operand also checked for validity in `gl_to_texenv_src_map`
  return operand == GL_SOURCE_ALPHA || operand == GL_ONE_MINUS_SRC_ALPHA;
}

static bool is_texenv_src_inverted(TexEnv* t,  int texture, bool rgb, int arg) {
  bool invert;
  GLenum operand = rgb ? t->src_operand_rgb[arg] : t->src_operand_alpha[arg];

  if (rgb) {
    assert(operand == GL_SRC_COLOR ||
           operand == GL_SRC_ALPHA ||
           operand == GL_ONE_MINUS_SRC_COLOR ||
           operand == GL_ONE_MINUS_SRC_ALPHA);
    invert = (operand == GL_ONE_MINUS_SRC_COLOR || operand == GL_ONE_MINUS_SRC_ALPHA);
  } else {
    assert(operand == GL_SRC_ALPHA ||
           operand == GL_ONE_MINUS_SRC_ALPHA);
    invert = (operand == GL_ONE_MINUS_SRC_ALPHA);
  }

  return invert;
}

static uint32_t* setup_texenv_src(uint32_t* p,
                                  TexEnv* t, unsigned int texture, unsigned int stage, bool rgb,
                                  unsigned int x_a, bool a_rgb, bool a_invert,
                                  unsigned int x_b, bool b_rgb, bool b_invert,
                                  unsigned int x_c, bool c_rgb, bool c_invert,
                                  unsigned int x_d, bool d_rgb, bool d_invert) {
  if (rgb) {
    p = pb_push1(p, NV097_SET_COMBINER_COLOR_ICW + stage * 4,
          MASK(NV097_SET_COMBINER_COLOR_ICW_A_SOURCE, x_a) | MASK(NV097_SET_COMBINER_COLOR_ICW_A_ALPHA, a_rgb ? 0 : 1) | MASK(NV097_SET_COMBINER_COLOR_ICW_A_MAP, a_invert ? _RC_UNSIGNED_INVERT : _RC_UNSIGNED)
        | MASK(NV097_SET_COMBINER_COLOR_ICW_B_SOURCE, x_b) | MASK(NV097_SET_COMBINER_COLOR_ICW_B_ALPHA, b_rgb ? 0 : 1) | MASK(NV097_SET_COMBINER_COLOR_ICW_B_MAP, b_invert ? _RC_UNSIGNED_INVERT : _RC_UNSIGNED)
        | MASK(NV097_SET_COMBINER_COLOR_ICW_C_SOURCE, x_c) | MASK(NV097_SET_COMBINER_COLOR_ICW_C_ALPHA, c_rgb ? 0 : 1) | MASK(NV097_SET_COMBINER_COLOR_ICW_C_MAP, c_invert ? _RC_UNSIGNED_INVERT : _RC_UNSIGNED)
        | MASK(NV097_SET_COMBINER_COLOR_ICW_D_SOURCE, x_d) | MASK(NV097_SET_COMBINER_COLOR_ICW_D_ALPHA, d_rgb ? 0 : 1) | MASK(NV097_SET_COMBINER_COLOR_ICW_D_MAP, d_invert ? _RC_UNSIGNED_INVERT : _RC_UNSIGNED));
  } else {
    p = pb_push1(p, NV097_SET_COMBINER_ALPHA_ICW + stage * 4,
          MASK(NV097_SET_COMBINER_ALPHA_ICW_A_SOURCE, x_a) | MASK(NV097_SET_COMBINER_ALPHA_ICW_A_ALPHA, a_rgb ? 0 : 1) | MASK(NV097_SET_COMBINER_ALPHA_ICW_A_MAP, a_invert ? _RC_UNSIGNED_INVERT : _RC_UNSIGNED)
        | MASK(NV097_SET_COMBINER_ALPHA_ICW_B_SOURCE, x_b) | MASK(NV097_SET_COMBINER_ALPHA_ICW_B_ALPHA, b_rgb ? 0 : 1) | MASK(NV097_SET_COMBINER_ALPHA_ICW_B_MAP, b_invert ? _RC_UNSIGNED_INVERT : _RC_UNSIGNED)
        | MASK(NV097_SET_COMBINER_ALPHA_ICW_C_SOURCE, x_c) | MASK(NV097_SET_COMBINER_ALPHA_ICW_C_ALPHA, c_rgb ? 0 : 1) | MASK(NV097_SET_COMBINER_ALPHA_ICW_C_MAP, c_invert ? _RC_UNSIGNED_INVERT : _RC_UNSIGNED)
        | MASK(NV097_SET_COMBINER_ALPHA_ICW_D_SOURCE, x_d) | MASK(NV097_SET_COMBINER_ALPHA_ICW_D_ALPHA, d_rgb ? 0 : 1) | MASK(NV097_SET_COMBINER_ALPHA_ICW_D_MAP, d_invert ? _RC_UNSIGNED_INVERT : _RC_UNSIGNED));
  }
  return p;
}

static void setup_texenv() {

  uint32_t* p = pb_begin();

  //FIXME: This is an assumption on how this should be handled
  //       The GL ES 1.1 Full spec claims that only texture 0 uses Cp=Cf.
  //       However, what if unit 0 is inactive? Cp=Cf? Cp=undefined?
  //       Couldn't find an answer when skimming over spec.
  GLuint rc_previous = _RC_PRIMARY_COLOR;

#define _Cf _RC_PRIMARY_COLOR, true, false
#define _Af _RC_PRIMARY_COLOR, false, false
#define _Cs _RC_TEXTURE+texture, true, false
#define _As _RC_TEXTURE+texture, false, false
#define _Cp rc_previous, true, false
#define _Ap rc_previous, false, false
#define _Zero _RC_ZERO, true, false //FIXME: make dependent on portion (RGB/ALPHA)?
#define _One _RC_ZERO, true, true //FIXME: make dependent on portion (RGB/ALPHA)?

  // This function is meant to create a register combiner from texenvs
  unsigned int stage = 0;
  for(int texture = 0; texture < 4; texture++) {

    // Skip stage if texture is disabled
    if (!state.texture_2d[texture]) {
      continue;
    }

    Texture* tx = get_bound_texture(texture);
    TexEnv* t = &texenvs[texture];

    switch(t->env_mode) {
    case GL_REPLACE:
      switch(tx->internal_base_format) {
      case GL_ALPHA:
        unimplemented();
        assert(false);
        break;   
      case GL_LUMINANCE: case 1:
        unimplemented();
        assert(false);
        break;        
      case GL_LUMINANCE_ALPHA: case 2:
        unimplemented();
        assert(false);
        break;        
      case GL_RGB: case 3:
        unimplemented();
        assert(false);
        break;
      case GL_RGBA: case 4:
        unimplemented();
        assert(false);
        break;
      default:
        unimplemented("%d\n", tx->internal_base_format);
        assert(false);
        break;
      }
      break;
    case GL_MODULATE:
      switch(tx->internal_base_format) {

      case GL_ALPHA:
        unimplemented();
        assert(false);
        break;   

      case GL_LUMINANCE: case 1:     
      case GL_RGB: case 3:
      
        // Cv = Cp * Cs
        // Av = Ap
        p = setup_texenv_output(p, t, texture, stage, true, true, false, false);
        p = setup_texenv_src(p, t, texture, stage, true,
                             _Cp, _Cs,
                             _Zero, _Zero);
        p = setup_texenv_output(p, t, texture, stage, false, true, false, false);
        p = setup_texenv_src(p, t, texture, stage, false,
                             _Ap, _One,
                             _Zero, _Zero);
        break;

      case GL_LUMINANCE_ALPHA: case 2:
      case GL_RGBA: case 4:

        // Cv = Cp * Cs
        // Av = Ap * As
        p = setup_texenv_output(p, t, texture, stage, true, true, false, false);
        p = setup_texenv_src(p, t, texture, stage, true,
                             _Cp, _Cs,
                             _Zero, _Zero);
        p = setup_texenv_output(p, t, texture, stage, false, true, false, false);
        p = setup_texenv_src(p, t, texture, stage, false,
                             _Ap, _As,
                             _Zero, _Zero);
        break;

      default:
        unimplemented("%d\n", tx->internal_base_format);
        assert(false);
        break;
      }
      break;
    case GL_COMBINE: {

      bool rgb;

#define _Arg(i) \
  gl_to_texenv_src(t, texture, rgb, (i)), \
  is_texenv_src_alpha(t, texture, rgb, (i)), \
  is_texenv_src_inverted(t, texture, rgb, (i))

#define _OneMinusArg(i) \
  gl_to_texenv_src(t, texture, rgb, (i)), \
  is_texenv_src_alpha(t, texture, rgb, (i)), \
  !is_texenv_src_inverted(t, texture, rgb, (i))

      rgb = true;
      switch(t->combine_rgb) {
      case GL_MODULATE:
        //FIXME: Can potentially merge into previous stage
        // Setup `spare0 = a * b`
        p = setup_texenv_output(p, t, texture, stage, rgb, true, false, false);
        p = setup_texenv_src(p, t, texture, stage, rgb,
                             _Arg(0), _Arg(1),
                             _Zero, _Zero);
        break;
      case GL_INTERPOLATE:
        // Setup `spare0 = a * b + c * d`
        p = setup_texenv_output(p, t, texture, stage, rgb, false, false, true);
        p = setup_texenv_src(p, t, texture, stage, rgb,
                             _Arg(0), _Arg(2),
                             _Arg(1), _OneMinusArg(2));
        break;     
      default:
        assert(false);
        break;     
      }

      rgb = false;
      switch(t->combine_alpha) {
      case GL_REPLACE:
        //FIXME: Can potentially merge into previous stage
        // Setup `spare0 = a`
        p = setup_texenv_output(p, t, texture, stage, rgb, true, false, false);
        p = setup_texenv_src(p, t, texture, stage, rgb,
                             _Arg(0), _One,
                             _Zero, _Zero);
        break;      
      default:
        assert(false);
        break;      
      }

      break;
    }
    default:
      assert(false);
      break;
    }

    rc_previous = _RC_SPARE0;
    stage++;
  }

  // Add at least 1 dummy stage (spare0 = fragment color)
  if (stage == 0) {
    p = setup_texenv_output(p, NULL, 0, stage, true, true, false, false);
    p = setup_texenv_src(p, NULL, 0, stage, true,
                         _Cf, _One,
                         _Zero, _Zero);
    p = setup_texenv_output(p, NULL, 0, stage, false, true, false, false);
    p = setup_texenv_src(p, NULL, 0, stage, false,
                         _Af, _One,
                         _Zero, _Zero);
    stage++;
  }

  // Set up final combiner
  //FIXME: This could merge an earlier stage into final_product
  //FIXME: Ensure this is `out.rgb = spare0.rgb; out.a = spare0.a;`
  p = pb_push1(p, NV097_SET_COMBINER_CONTROL,
        MASK(NV097_SET_COMBINER_CONTROL_FACTOR0, NV097_SET_COMBINER_CONTROL_FACTOR0_SAME_FACTOR_ALL)
      | MASK(NV097_SET_COMBINER_CONTROL_FACTOR1, NV097_SET_COMBINER_CONTROL_FACTOR1_SAME_FACTOR_ALL)
      | MASK(NV097_SET_COMBINER_CONTROL_ITERATION_COUNT, stage));
  p = pb_push1(p, NV097_SET_COMBINER_SPECULAR_FOG_CW0,
        MASK(NV097_SET_COMBINER_SPECULAR_FOG_CW0_A_SOURCE, _RC_ZERO)   | MASK(NV097_SET_COMBINER_SPECULAR_FOG_CW0_A_ALPHA, 0) | MASK(NV097_SET_COMBINER_SPECULAR_FOG_CW0_A_INVERSE, 0)
      | MASK(NV097_SET_COMBINER_SPECULAR_FOG_CW0_B_SOURCE, _RC_ZERO)   | MASK(NV097_SET_COMBINER_SPECULAR_FOG_CW0_B_ALPHA, 0) | MASK(NV097_SET_COMBINER_SPECULAR_FOG_CW0_B_INVERSE, 0)
      | MASK(NV097_SET_COMBINER_SPECULAR_FOG_CW0_C_SOURCE, _RC_ZERO)   | MASK(NV097_SET_COMBINER_SPECULAR_FOG_CW0_C_ALPHA, 0) | MASK(NV097_SET_COMBINER_SPECULAR_FOG_CW0_C_INVERSE, 0)
      | MASK(NV097_SET_COMBINER_SPECULAR_FOG_CW0_D_SOURCE, _RC_SPARE0) | MASK(NV097_SET_COMBINER_SPECULAR_FOG_CW0_D_ALPHA, 0) | MASK(NV097_SET_COMBINER_SPECULAR_FOG_CW0_D_INVERSE, 0));
  p = pb_push1(p, NV097_SET_COMBINER_SPECULAR_FOG_CW1,
        MASK(NV097_SET_COMBINER_SPECULAR_FOG_CW1_E_SOURCE, _RC_ZERO)   | MASK(NV097_SET_COMBINER_SPECULAR_FOG_CW1_E_ALPHA, 0) | MASK(NV097_SET_COMBINER_SPECULAR_FOG_CW1_E_INVERSE, 0)
      | MASK(NV097_SET_COMBINER_SPECULAR_FOG_CW1_F_SOURCE, _RC_ZERO)   | MASK(NV097_SET_COMBINER_SPECULAR_FOG_CW1_F_ALPHA, 0) | MASK(NV097_SET_COMBINER_SPECULAR_FOG_CW1_F_INVERSE, 0)
      | MASK(NV097_SET_COMBINER_SPECULAR_FOG_CW1_G_SOURCE, _RC_SPARE0) | MASK(NV097_SET_COMBINER_SPECULAR_FOG_CW1_G_ALPHA, 1) | MASK(NV097_SET_COMBINER_SPECULAR_FOG_CW1_G_INVERSE, 0)
      | MASK(NV097_SET_COMBINER_SPECULAR_FOG_CW1_SPECULAR_CLAMP, 0));

  pb_end(p);
}


static GLuint gl_array_buffer = 0;
static GLuint gl_element_array_buffer = 0;

static GLuint* get_bound_buffer_store(GLenum target) {
  switch(target) {
  case GL_ARRAY_BUFFER:
    return &gl_array_buffer;
  case GL_ELEMENT_ARRAY_BUFFER:
    return &gl_element_array_buffer;
  default:
    unimplemented("%d", target);
    assert(false);
    break;
  }
  return NULL;  
}

#define DEFAULT_MATRIX() \
  { \
        1.0f, 0.0f, 0.0f, 0.0f, \
        0.0f, 1.0f, 0.0f, 0.0f, \
        0.0f, 0.0f, 1.0f, 0.0f, \
        0.0f, 0.0f, 0.0f, 1.0f \
  }

static float matrix_mv[16 * 4*4] = DEFAULT_MATRIX();
static unsigned int matrix_mv_slot = 0;
static float matrix_p[16 * 4*4] = DEFAULT_MATRIX();
static unsigned int matrix_p_slot = 0;
static float matrix_t[4][16 * 4*4] = {
  DEFAULT_MATRIX(),
  DEFAULT_MATRIX(),
  DEFAULT_MATRIX(),
  DEFAULT_MATRIX()
};
static unsigned int matrix_t_slot[4] = { 0, 0, 0, 0 };

//FIXME: This code assumes that matrix_mv_slot is 0
static unsigned int* matrix_slot = &matrix_mv_slot;
static float* matrix = &matrix_mv[0];
static GLenum matrix_mode = GL_MODELVIEW;




static void print_attrib(XguVertexArray array, Attrib* attrib, unsigned int start, unsigned int count, bool submit) {
  unsigned int end = start + count;
  if (!attrib->array.enabled) {
    debugPrint("\narray %d disabled\n", array, attrib->array.stride, attrib->array.data);
    return;
  }
  if (attrib->array.gl_type == GL_SHORT) {
    debugPrint("\narray %d as GL_SHORT, stride %d at %p\n", array, attrib->array.stride, attrib->array.data);
    for(int i = start; i < end; i++) {
      int16_t* v = i * attrib->array.stride + (uintptr_t)attrib->array.data;
      debugPrint("[%d]:", i);
      for(int j = 0; j < attrib->array.size; j++) {
        debugPrint(" %d", (int)v[j]);
      }
      debugPrint("\n");
      if (submit) {
        assert(array == XGU_VERTEX_ARRAY);
        assert(attrib->array.size == 2);
        uint32_t* p = pb_begin();
        p = xgu_vertex4f(p,  v[0], v[1], 0.0f, 1.0f);
        pb_end(p);
      }
    }
  } else if (attrib->array.gl_type == GL_FLOAT) {
    debugPrint("\narray %d as GL_FLOAT, stride %d at %p\n", array, attrib->array.stride, attrib->array.data);
    for(int i = start; i < end; i++) {
      float* v = i * attrib->array.stride + (uintptr_t)attrib->array.data;
      debugPrint("[%d]:", i);
      for(int j = 0; j < attrib->array.size; j++) {
        debugPrint(" ");
        debugPrintFloat(v[j]);
      }
      debugPrint("\n");
      if (submit) {
        assert(array == XGU_VERTEX_ARRAY);
        uint32_t* p = pb_begin();
        if (attrib->array.size == 2){
          p = xgu_vertex4f(p,  v[0], v[1], 0.0f, 1.0f);
        } else if (attrib->array.size == 3){
          p = xgu_vertex4f(p,  v[0], v[1], v[2], 1.0f);
        } else {
          assert(false);
        }
        pb_end(p);
      }
    }
  }
}

static void setup_attrib(XguVertexArray array, Attrib* attrib) {
  if (!attrib->array.enabled) {
    uint32_t* p = pb_begin();
    //FIXME: Somehow XGU should check for XGU_FLOAT if size is 0? type=FLOAT + size=0 means disabled
    p = xgu_set_vertex_data_array_format(p, array, XGU_FLOAT, 0, 0);
    p = xgu_set_vertex_data4f(p, array, attrib->value[0], attrib->value[1], attrib->value[2], attrib->value[3]);
    pb_end(p);
    return;
  }
  assert(attrib->array.size > 0);
  assert(attrib->array.stride > 0);
  xgux_set_attrib_pointer(array, gl_to_xgu_vertex_array_type(attrib->array.gl_type), attrib->array.size, attrib->array.stride, attrib->array.data);
}

static bool is_texture_complete(Texture* tx) {
  if (tx->width == 0) { return false; }
  if (tx->height == 0) { return false; }
  unimplemented(); //FIXME: Check mipmaps, filters, ..
  return true;
}

static unsigned int gl_to_xgu_texgen(GLenum texgen) {
  switch(texgen) {
  case GL_SPHERE_MAP: return XGU_TEXGEN_SPHERE_MAP;
  default:
    unimplemented("%d", texgen);
    assert(false);
    break;
  }
  return -1;
}

static unsigned int gl_to_xgu_texture_address(GLenum wrap) {
  switch(wrap) {
  case GL_REPEAT:          return XGU_WRAP;
//  case GL_MIRROR_REPEAT:   return XGU_MIRROR;
  case GL_CLAMP_TO_EDGE:   return XGU_CLAMP_TO_EDGE;
//  case GL_CLAMP_TO_BORDER: return XGU_CLAMP_TO_BORDER;
//  case GL_CLAMP:           return XGU_CLAMP;
  default:
    unimplemented("%d", wrap);
    assert(false);
    break;
  }
  return -1;
}

static unsigned int gl_to_xgu_texture_format(GLenum internalformat) {
  switch(internalformat) {
  case GL_LUMINANCE:       return XGU_TEXTURE_FORMAT_Y8_SWIZZLED;
  case GL_LUMINANCE_ALPHA: return XGU_TEXTURE_FORMAT_A8Y8_SWIZZLED;
  case GL_RGB:             return XGU_TEXTURE_FORMAT_X8R8G8B8_SWIZZLED;
  case GL_RGBA:            return XGU_TEXTURE_FORMAT_A8B8G8R8_SWIZZLED;
  default:
    unimplemented("%d", internalformat);
    assert(false);
    break;
  }
  return -1;
}

static void setup_textures() {

  uint32_t* p;

  // Disable texture dependencies (unused)
  //FIXME: init once?
  p = pb_begin();
  p = pb_push1(p, NV097_SET_SHADER_OTHER_STAGE_INPUT,
        MASK(NV097_SET_SHADER_OTHER_STAGE_INPUT_STAGE1, 0)
      | MASK(NV097_SET_SHADER_OTHER_STAGE_INPUT_STAGE2, 0)
      | MASK(NV097_SET_SHADER_OTHER_STAGE_INPUT_STAGE3, 0));
  pb_end(p);

  unsigned int clip_plane_index = 0;
  unsigned int shaders[4];

  for(int i = 0; i < 4; i++) {

    // Find texture object
    Texture* tx = get_bound_texture(i);
    
    // Check if this unit is disabled
    if (!state.texture_2d[i] || !is_texture_complete(tx)) {

      // Disable texture
      p = pb_begin();
      p = xgu_set_texture_control0(p, i, false, 0, 0);
      //FIXME: pbkit also sets wrap/addressing and filter stuff for disabled textures?!
      pb_end(p);

      // Find the next used clip plane
      while(clip_plane_index < ARRAY_SIZE(clip_planes)) {
        if (clip_planes[clip_plane_index].enabled) {
          break;
        }
        clip_plane_index++;
      }

      // Set clip plane if one was found
      if (clip_plane_index < ARRAY_SIZE(clip_planes)) {
        const ClipPlane* clip_plane = &clip_planes[clip_plane_index];

        // Set vertex attributes for the texture coordinates
        //FIXME: This is wrong? It has to inherit the vertex position
        //       Should be using texgen probably
        Attrib attrib = {
          .array = { .enabled = false },
          .value = { clip_plane->x, clip_plane->y, clip_plane->z, clip_plane->w }
        };
        setup_attrib(XGU_TEXCOORD0_ARRAY+i, &attrib);

        // Setup shader
        //FIXME: Can use 4 clip planes per texture unit by using texture-matrix?
        //FIXME: Also disable texture matrix?!
        unimplemented(); //FIXME: This should be NV097_SET_SHADER_STAGE_PROGRAM_STAGE0_CLIP_PLANE
        shaders[i] = NV097_SET_SHADER_STAGE_PROGRAM_STAGE0_PROGRAM_NONE;

        clip_plane_index++;
        continue;
      }

      // Disable texture shader if nothing depends on it
      shaders[i] = NV097_SET_SHADER_STAGE_PROGRAM_STAGE0_PROGRAM_NONE;

      continue;
    }

    // Sanity check texture
    assert(tx->width != 0);
    assert(tx->height != 0);
    assert(tx->pitch != 0);
    debugPrint("%d x %d [%d]\n", tx->width, tx->height, tx->pitch);
    assert(tx->data != NULL);

    // Setup texture
    shaders[i] = NV097_SET_SHADER_STAGE_PROGRAM_STAGE0_2D_PROJECTIVE;
    p = pb_begin();

    //FIXME: I'd rather have XGU_TEXTURE_2D and XGU_TEXTURE_CUBEMAP for these
    bool cubemap_enable = false;
    unsigned int dimensionality = 2;

    unsigned int context_dma = 2; //FIXME: Which one did pbkit use?
    XguBorderSrc border = XGU_SOURCE_COLOR;

    unsigned int mipmap_levels = MAX(tx->width_shift, tx->height_shift) + 1;
    unsigned int min_lod = 0;
    unsigned int max_lod = mipmap_levels - 1;
    unsigned int lod_bias = 0;

    p = xgu_set_texture_offset(p, i, (uintptr_t)tx->data & 0x03ffffff);
    p = xgu_set_texture_format(p, i, context_dma, cubemap_enable, border, dimensionality,
                                     gl_to_xgu_texture_format(tx->internal_base_format), mipmap_levels,
                                     tx->width_shift,tx->height_shift,0);
    p = xgu_set_texture_address(p, i, gl_to_xgu_texture_address(tx->wrap_s), false,
                                      gl_to_xgu_texture_address(tx->wrap_t), false,
                                      XGU_CLAMP_TO_EDGE, false,
                                      false);
    p = xgu_set_texture_control0(p, i, true, min_lod, max_lod);
//    p = xgu_set_texture_control1(p, i, 0x800000 /*tx->pitch*/);
    p = xgu_set_texture_filter(p, i, lod_bias, XGU_TEXTURE_CONVOLUTION_QUINCUNX,
                                        gl_to_xgu_texture_filter(tx->min_filter),
                                        gl_to_xgu_texture_filter(tx->mag_filter),
                                        false, false, false, false);
//    p = xgu_set_texture_image_rect(p, i, 0 /*tx->width*/, 0 /*tx->height*/);

#if 0
    //FIXME: Use NV097_SET_TEXTURE_FORMAT and friends
    p = pb_push2(p,NV20_TCL_PRIMITIVE_3D_TX_OFFSET(i), (uintptr_t)tx->data & 0x03ffffff, 0x0001002A | (gl_to_xgu_texture_format(tx->internal_base_format) << 8)); //set stage 0 texture address & format
    p = pb_push1(p,NV20_TCL_PRIMITIVE_3D_TX_NPOT_PITCH(i), tx->pitch<<16); //set stage 0 texture pitch (pitch<<16)
    p = pb_push1(p,NV20_TCL_PRIMITIVE_3D_TX_NPOT_SIZE(i), (tx->width<<16) | tx->height); //set stage 0 texture width & height ((witdh<<16)|height)

    p = pb_push1(p,NV20_TCL_PRIMITIVE_3D_TX_WRAP(i),0x00030303);//set stage 0 texture modes (0x0W0V0U wrapping: 1=wrap 2=mirror 3=clamp 4=border 5=clamp to edge)
    p = pb_push1(p,NV20_TCL_PRIMITIVE_3D_TX_ENABLE(i),0x4003ffc0); //set stage 0 texture enable flags

    p = pb_push1(p,NV20_TCL_PRIMITIVE_3D_TX_FILTER(i),0x04074000); //set stage 0 texture filters (AA!)
#endif

    p = xgu_set_texgen_s(p, i, state.texgen_s_enabled[i] ? gl_to_xgu_texgen(state.texgen_s[i])
                                                         : XGU_TEXGEN_DISABLE);
    p = xgu_set_texgen_t(p, i, state.texgen_t_enabled[i] ? gl_to_xgu_texgen(state.texgen_t[i])
                                                         : XGU_TEXGEN_DISABLE);
    p = xgu_set_texgen_r(p, i, XGU_TEXGEN_DISABLE);
    p = xgu_set_texgen_q(p, i, XGU_TEXGEN_DISABLE);
    p = xgu_set_texture_matrix_enable(p, i, true); //FIXME: See if this makes a perf difference and only enable if not identity?
    unimplemented(); //FIXME: Not hitting pixel centers yet?!
    p = xgu_set_texture_matrix(p, i, &matrix_t[i][0]);

    pb_end(p);
  }

  // Setup texture shader
  p = pb_begin();
  p = pb_push1(p, NV097_SET_SHADER_STAGE_PROGRAM,
        MASK(NV097_SET_SHADER_STAGE_PROGRAM_STAGE0, shaders[0])
      | MASK(NV097_SET_SHADER_STAGE_PROGRAM_STAGE1, shaders[1])
      | MASK(NV097_SET_SHADER_STAGE_PROGRAM_STAGE2, shaders[2])
      | MASK(NV097_SET_SHADER_STAGE_PROGRAM_STAGE3, shaders[3]));
  pb_end(p);


  // Assert that we handled all clip planes
  while(clip_plane_index < ARRAY_SIZE(clip_planes)) {
    assert(!clip_planes[clip_plane_index].enabled);
    clip_plane_index++;
  }
}

static void setup_matrices() {


    //FIXME: one time init?
    uint32_t* p = pb_begin();

    /* A generic identity matrix */
    const float m_identity[4*4] = DEFAULT_MATRIX();

    p = xgu_set_skin_mode(p, XGU_SKIN_MODE_OFF);
    p = xgu_set_normalization_enable(p, false);



    pb_end(p);

    int width = pb_back_buffer_width();
    int height = pb_back_buffer_height();





    // Update matrices

    //FIXME: Keep dirty bits
    float* matrix_p_now = &matrix_p[matrix_p_slot * 4*4];
    float* matrix_mv_now = &matrix_mv[matrix_mv_slot * 4*4];


#if 0
debugPrint("\ndraw:\n");
  PRINT_MATRIX(matrix_p_now);
#endif

#if 1
  {
    static float m[4*4];
    memcpy(m, matrix_mv_now, sizeof(m));
    matrix_mv_now = m;
  } {
    static float m[4*4];
    memcpy(m, matrix_p_now, sizeof(m));
    matrix_p_now = m;
  }





  float t[4*4];
  memcpy(t, matrix_p_now, sizeof(t));
  matmul4(matrix_p_now, viewport_matrix, t);



#if 1
debugPrint("\ndraw (xbox):\n");
  PRINT_MATRIX(matrix_p_now);
#endif

//Sleep(100);
#endif


// Transpose
#define TRANSPOSE(__m) \
  { \
    float* m_old = __m; \
    float m_new[4*4]; \
    for(int i = 0; i < 4; i++) { \
      for(int j = 0; j < 4; j++) { \
        m_new[i+4*j] = m_old[i*4+j]; \
      } \
    } \
    memcpy(m_old, m_new, sizeof(m_new)); \
  }
TRANSPOSE(matrix_p_now)
TRANSPOSE(matrix_mv_now)



  //FIXME: Could be wrong
  float matrix_c_now[4*4];
  matmul4(matrix_c_now, matrix_mv_now, matrix_p_now);

  CHECK_MATRIX(matrix_p_now);
  CHECK_MATRIX(matrix_mv_now);
  CHECK_MATRIX(matrix_c_now);


#if 0
  for(int i = 0; i < XGU_WEIGHT_COUNT; i++) {
    p = xgu_set_model_view_matrix(p, i, m_identity); //FIXME: Not sure when used?
    p = xgu_set_inverse_model_view_matrix(p, i, m_identity); //FIXME: Not sure when used?
  }
#endif
  
  
  /* Set up all states for hardware vertex pipeline */
  //FIXME: Might need inverse or some additional transform
  p = pb_begin();
  p = xgu_set_transform_execution_mode(p, XGU_FIXED, XGU_RANGE_MODE_PRIVATE);
  //FIXME: p = xgu_set_fog_enable(p, false);

  //FIXME: Probably should include the viewort matrix
  p = xgu_set_projection_matrix(p, matrix_p_now); //FIXME: Unused in XQEMU

  p = xgu_set_composite_matrix(p, matrix_c_now); //FIXME: Always used in XQEMU?

  for(int i = 0; i < XGU_WEIGHT_COUNT; i++) {
#if 1
      // Only required if skinning is enabled?
      transposeMatrix(t, matrix_mv_now);
      p = xgu_set_model_view_matrix(p, i, t); //FIXME: Not sure when used?
#endif
#if 1
      // Required for lighting

      invert(t, matrix_mv_now); //FIXME: This is affected if we want to normalize normals
      transposeMatrix(t, t);
      //FIXME: mesa only uploads a 4x3 / 3x4 matrix
      p = xgu_set_inverse_model_view_matrix(p, i, t); //FIXME: Not sure when used?
#endif
  }

  p = xgu_set_viewport_offset(p, 0.0f, 0.0f, 0.0f, 0.0f);
  p = xgu_set_viewport_scale(p, 1.0f, 1.0f, 1.0f, 1.0f); //FIXME: Ignored?!
  pb_end(p);

}

static void gl_to_xgu_material_source(GLenum mode, XguMaterialSource* emissive, XguMaterialSource* ambient, XguMaterialSource* diffuse, XguMaterialSource* specular) {
  switch(mode) {
  case GL_EMISSION:
    *emissive = XGU_MATERIAL_SOURCE_VERTEX_DIFFUSE;
    *ambient = XGU_MATERIAL_SOURCE_DISABLE;
    *diffuse = XGU_MATERIAL_SOURCE_DISABLE;
    *specular = XGU_MATERIAL_SOURCE_DISABLE;
    break;
  case GL_AMBIENT:
    *emissive = XGU_MATERIAL_SOURCE_DISABLE;
    *ambient = XGU_MATERIAL_SOURCE_VERTEX_DIFFUSE;
    *diffuse = XGU_MATERIAL_SOURCE_DISABLE;
    *specular = XGU_MATERIAL_SOURCE_DISABLE;
    break;
  case GL_DIFFUSE:
    *emissive = XGU_MATERIAL_SOURCE_DISABLE;
    *ambient = XGU_MATERIAL_SOURCE_DISABLE;
    *diffuse = XGU_MATERIAL_SOURCE_VERTEX_DIFFUSE;
    *specular = XGU_MATERIAL_SOURCE_DISABLE;
    break;
  case GL_SPECULAR:
    *emissive = XGU_MATERIAL_SOURCE_DISABLE;
    *ambient = XGU_MATERIAL_SOURCE_DISABLE;
    *diffuse = XGU_MATERIAL_SOURCE_DISABLE;
    *specular = XGU_MATERIAL_SOURCE_VERTEX_DIFFUSE;
    break;
  case GL_AMBIENT_AND_DIFFUSE:
    *emissive = XGU_MATERIAL_SOURCE_DISABLE;
    *ambient = XGU_MATERIAL_SOURCE_VERTEX_DIFFUSE;
    *diffuse = XGU_MATERIAL_SOURCE_VERTEX_DIFFUSE;
    *specular = XGU_MATERIAL_SOURCE_DISABLE;
    break;
  default:
    unimplemented("%d", mode);
    assert(false);
    break;
  }
}

static void setup_lighting() {
  XguMaterialSource emissive_source[2] = { XGU_MATERIAL_SOURCE_DISABLE, XGU_MATERIAL_SOURCE_DISABLE };
  XguMaterialSource ambient_source[2]  = { XGU_MATERIAL_SOURCE_DISABLE, XGU_MATERIAL_SOURCE_DISABLE };
  XguMaterialSource diffuse_source[2]  = { XGU_MATERIAL_SOURCE_DISABLE, XGU_MATERIAL_SOURCE_DISABLE };
  XguMaterialSource specular_source[2] = { XGU_MATERIAL_SOURCE_DISABLE, XGU_MATERIAL_SOURCE_DISABLE };

  if (state.color_material_enabled) {
    gl_to_xgu_material_source(state.color_material_front, &emissive_source[0], &ambient_source[0], &diffuse_source[0], &specular_source[0]);
    gl_to_xgu_material_source(state.color_material_back,  &emissive_source[1], &ambient_source[1], &diffuse_source[1], &specular_source[1]);
  }

  uint32_t* p = pb_begin();

  //FIXME: What if only one-sided lighting is enabled? Will it still use <property>[1]?
  // Note: This sets the color source, not the actual color
  p = xgu_set_color_material(p, emissive_source[0], ambient_source[0], diffuse_source[0], specular_source[0],
                                emissive_source[1], ambient_source[1], diffuse_source[1], specular_source[1]);

  int sides = 2;

  // Reverse engineered from Futurama PAL
  for(int side = 0; side < sides; side++) {
    float f1[3];
    float f2[3];
    if (ambient_source[side] != XGU_MATERIAL_SOURCE_DISABLE) {
      // Ambient = Vertex
      // Emission = ???

      f1[0] = state.material[side].emission.r;
      f1[1] = state.material[side].emission.g;
      f1[2] = state.material[side].emission.b;

      // Note: I don't think GL can do two-sided for this one?
      f2[0] = state.light_model_ambient.r;
      f2[1] = state.light_model_ambient.g;
      f2[2] = state.light_model_ambient.b;

      if (emissive_source[side] != XGU_MATERIAL_SOURCE_DISABLE) {
        // Ambient = Vertex
        // Emission = Vertex

        //FIXME: = f1 + f2 * vertex ???
        //       = material.emission + light_model.ambient * vertex

        //FIXME: What about replacing the emission?
        //       Shouldn't this be something like:
        //       = vertex + light_model.ambient * vertex ?
        //       Will the GPU automatically use vertex instead of f1?
        //       Is this an invalid GPU state? (GL API makes this impossible)
        assert(false); 
                       
      } else {
        // Ambient = Vertex
        // Emission = Material

        // Confusingly:
        // - f1 = SCENE_AMBIENT_COLOR [but used for material.emission here]
        // - f2 = MATERIAL_EMISSION [but used for light_model.ambient here]

        //FIXME: = f1 + f2 * vertex ???
        //       = material.emission + light_model.ambient * vertex
      }
    } else if (emissive_source[side] != XGU_MATERIAL_SOURCE_DISABLE) {
      assert(ambient_source[side] == XGU_MATERIAL_SOURCE_DISABLE);
      // Ambient = Material
      // Emission = Vertex

      f1[0] = state.light_model_ambient.r * state.material[side].ambient.r;
      f1[1] = state.light_model_ambient.g * state.material[side].ambient.g;
      f1[2] = state.light_model_ambient.b * state.material[side].ambient.b;

      f2[0] = 1.0f;
      f2[1] = 1.0f;
      f2[2] = 1.0f;

      //FIXME: = f1 + f2 * vertex ???
      //       = (light_model.ambient * material.ambient) + vertex ???
    } else {
      assert(ambient_source[side] == XGU_MATERIAL_SOURCE_DISABLE);
      assert(emissive_source[side] == XGU_MATERIAL_SOURCE_DISABLE);
      // Ambient = Material
      // Emission = Material

      f1[0] = state.light_model_ambient.r * state.material[side].ambient.r + state.material[side].emission.r;
      f1[1] = state.light_model_ambient.g * state.material[side].ambient.g + state.material[side].emission.g;
      f1[2] = state.light_model_ambient.b * state.material[side].ambient.b + state.material[side].emission.b;

      f2[0] = 0.0f;
      f2[1] = 0.0f;
      f2[2] = 0.0f;

      //FIXME: = f1 + f2 * vertex ???
      //       = (light_model.ambient * material.ambient + material.emission) + 0 ???
    }
    if (side == 0) {
      p = xgu_set_scene_ambient_color(p, f1[0], f1[1], f1[2]);
      p = xgu_set_material_emission(p,   f2[0], f2[1], f2[2]);
    } else {
      p = xgu_set_back_scene_ambient_color(p, f1[0], f1[1], f1[2]);
      p = xgu_set_back_material_emission(p,   f2[0], f2[1], f2[2]);
    }
  }

  //FIXME: How does this contribute?
  p = xgu_set_material_alpha(p,      state.material[0].diffuse.a);
  p = xgu_set_back_material_alpha(p, state.material[1].diffuse.a);

  //FIXME: When to do this?
  bool specular_enabled = false;
  p = xgu_set_specular_enable(p, specular_enabled);

  bool separate_specular = false; //FIXME: Respect GL
  bool localeye = false; //FIXME: Respect GL
  XguSout sout = XGU_SOUT_ZERO_OUT; //FIXME: What is this?
                                    //       - D3D VP always uses PASSTHROUGH?
                                    //       - D3D FFP always uses ZERO_OUT?
  if (SDL_GameControllerGetButton(g, SDL_CONTROLLER_BUTTON_DPAD_LEFT)) {
    sout = XGU_SOUT_PASSTHROUGH; //FIXME: What is this?
    pb_print("PT;");
  } else {
    pb_print("ZO;");
  }
  p = xgu_set_light_control(p, separate_specular, localeye, sout);

  XguLightMask mask[8];
  for(int i = 0; i < 8; i++) {
      mask[i] = XGU_LMASK_OFF;
  }
  assert(GL_MAX_LIGHTS <= 8);
  for(int i = 0; i < GL_MAX_LIGHTS; i++) {
    Light* l = &state.lights[i];
    if (!l->enabled) {
      continue;
    }

    if (l->is_dir) {
      mask[i] = XGU_LMASK_INFINITE;
      //printf("directional light\n");

      XguVec3 v = { l->position.x, l->position.y, l->position.z };
      //printf("light-dir: %f %f %f\n", v.x, v.y, v.z);
      p = xgu_set_light_infinite_direction(p, i, v);

    } else {
      if (l->spot_cutoff == 180.0f) {
        mask[i] = XGU_LMASK_LOCAL;
        //printf("point light\n");
      } else {
        mask[i] = XGU_LMASK_SPOT;
        //printf("spot light\n");
      }

      {
        XguVec3 v = { l->position.x, l->position.y, l->position.z };
        p = xgu_set_light_local_position(p, i, v);
      }

      p = xgu_set_light_local_attenuation(p, i, l->constant_attenuation, l->linear_attenuation, l->quadratic_attenuation);

      pb_end(p);

      XguVec3 direction = {
        l->spot_direction.x,
        l->spot_direction.y,
        l->spot_direction.z
      };
      _normalize(&direction.x);
#if 0
      //FIXME: Do cos outside of function?
      float theta = 0.0f;
      float phi = l->spot_cutoff / 180.0f * M_PI;
      float falloff = 1.0f;
      // D3D direction is flipped
      direction.x *= -1.0f;
      direction.y *= -1.0f;
      direction.z *= -1.0f;
      xgux_set_light_spot_d3d(i, theta, phi, falloff, direction);
#else
      //FIXME: Do cos inside the function?
      float cutoff = l->spot_cutoff / 180.0f * M_PI; //FIXME: Accept values in degree to be closer to GL?
      xgux_set_light_spot_gl(i, l->spot_exponent, cutoff, direction);
#endif
      p = pb_begin();
    }



    for(int side = 0; side < sides; side++) {
      float f[3];

      if (diffuse_source[side] == XGU_MATERIAL_SOURCE_DISABLE) {
        _mul3(f, &l->diffuse, &state.material[side].diffuse);
      } else {
        memcpy(f, &l->diffuse, sizeof(f));
      }
      if (side == 0) {
        p = xgu_set_light_diffuse_color(p, i, f[0], f[1], f[2]);
      } else {
        p = xgu_set_back_light_diffuse_color(p, i, f[0], f[1], f[2]);
      }

      // We'd be incorrectly mixing colors in the shader, so it's too bright?
      // Hence, we disable some lights for now.
      //FIXME: Do proper mixing in shader.
#if 1
      if (ambient_source[side] == XGU_MATERIAL_SOURCE_DISABLE) {
        _mul3(f, &l->ambient, &state.material[side].ambient);
      } else {
        memcpy(f, &l->ambient, sizeof(f));
      }
      if (side == 0) {
        p = xgu_set_light_ambient_color(p, i, f[0], f[1], f[2]);
      } else {
        p = xgu_set_back_light_ambient_color(p, i, f[0], f[1], f[2]);
      }

      if (specular_source[side] == XGU_MATERIAL_SOURCE_DISABLE) {
        _mul3(f, &l->specular, &state.material[side].specular);
      } else {
        memcpy(f, &l->specular, sizeof(f));
      }
      if (side == 0) {
        p = xgu_set_light_specular_color(p, i, f[0], f[1], f[2]);
      } else {
        p = xgu_set_back_light_specular_color(p, i, f[0], f[1], f[2]);
      }
    }

#else
    }

    p = xgu_set_light_ambient_color(p, i, 0,0,0);
    p = xgu_set_back_light_ambient_color(p, i, 0,0,0);

    p = xgu_set_light_specular_color(p, i, 0,0,0);
    p = xgu_set_back_light_specular_color(p, i, 0,0,0);
#endif
  }
  p = xgu_set_light_enable_mask(p, mask[0],
                                   mask[1],
                                   mask[2],
                                   mask[3],
                                   mask[4],
                                   mask[5],
                                   mask[6],
                                   mask[7]);

  pb_end(p);

  // Set material
  xgux_set_specular_gl(state.material[0].shininess);
  xgux_set_back_specular_gl(state.material[1].shininess);
}

static size_t p_size = 0;
static ULONGLONG gpu_duration = 0;
static ULONGLONG cpu_duration = 0;
static void reset_pb() {

  // Measure time since last call to this function
  static ULONGLONG cpu_last = 0;
  ULONGLONG cpu_now = KeQueryPerformanceCounter();
  ULONGLONG cpu_duration_batch = cpu_now - cpu_last;

  // Get pb pointer at end
  uint32_t* p_tail = pb_begin(); pb_end(p_tail);

  // Finish pb
  while(pb_busy());

  // Go to head of pb and get pointer
  pb_reset();
  uint32_t* p_head = pb_begin(); pb_end(p_head);
  size_t p_size_batch = (p_tail - p_head) * 4;

  // Measure time that we required for GPU synchronization
  cpu_last = KeQueryPerformanceCounter();
  ULONGLONG gpu_duration_batch = cpu_last - cpu_now;

  // Accumulate statistics
  p_size += p_size_batch;
  cpu_duration += cpu_duration_batch;
  gpu_duration += gpu_duration_batch;
}

static unsigned int drawcall_count = 0;
static void prepare_drawing() {
  drawcall_count += 1;

  unimplemented(); //FIXME: Measure length of buffer and resize so we can handle large levels
                   //       `reset_pb` is too expensive.
  reset_pb();

  debugPrint("Preparing to draw\n");

  //FIXME: Track dirty bits

  // Setup lights from state.light[].enabled
  //FIXME: This uses a mask: p = xgu_set_light_enable(p, );

  // Setup attributes
#if 1
  uint32_t* p = pb_begin();
  for(int i = 0; i < 16; i++) {
    p = xgu_set_vertex_data_array_format(p, i, XGU_FLOAT, 0, 0);
  }
  pb_end(p);
#endif
  setup_attrib(XGU_VERTEX_ARRAY, &state.vertex_array);
  setup_attrib(XGU_COLOR_ARRAY, &state.color_array);
  setup_attrib(XGU_NORMAL_ARRAY, &state.normal_array);
  setup_attrib(XGU_TEXCOORD0_ARRAY, &state.texture_coord_array[0]);
  setup_attrib(XGU_TEXCOORD1_ARRAY, &state.texture_coord_array[1]);
  setup_attrib(XGU_TEXCOORD2_ARRAY, &state.texture_coord_array[2]);
  setup_attrib(XGU_TEXCOORD3_ARRAY, &state.texture_coord_array[3]);

  // Set up all matrices etc.
  setup_matrices();

  // Setup lighting
  setup_lighting();

#if 1
  if (SDL_GameControllerGetButton(g, SDL_CONTROLLER_BUTTON_RIGHTSHOULDER)) {
    pb_print("Lighting disabled\n");
    uint32_t* p = pb_begin();
    p = xgu_set_lighting_enable(p, false);
    pb_end(p);
  }
#endif

  // Setup textures
  setup_textures();

debugPrint("texenv setup");
  // Set the register combiner
  setup_texenv();
debugPrint("debug stuff");

#if 0
  {
    // Set some safe state
    uint32_t* p = pb_begin();
    p = xgu_set_cull_face_enable(p, false);
    p = xgu_set_depth_test_enable(p, false);
    p = xgu_set_lighting_enable(p, false);
    pb_end(p);
  }
#endif

#if 0
  {
    // Set some safe alpha state
    uint32_t* p = pb_begin();
    p = xgu_set_alpha_test_enable(p, false);
    p = xgu_set_blend_enable(p, false);
    pb_end(p);
  }
#endif

#if 0
  {
    // Disco lighting
    uint32_t* p = pb_begin();
    p = xgu_set_vertex_data_array_format(p, XGU_COLOR_ARRAY, XGU_FLOAT, 0, 0);
    p = xgu_set_vertex_data4ub(p, XGU_COLOR_ARRAY, rand() & 0xFF, rand() & 0xFF, rand() & 0xFF, rand() & 0xFF);
    pb_end(p);
  }
#endif

#if 1
  if (SDL_GameControllerGetButton(g, SDL_CONTROLLER_BUTTON_DPAD_RIGHT)) {
    pb_print("BLEND;\n");
    uint32_t* p = pb_begin();
    p = xgu_set_blend_func_sfactor(p, XGU_FACTOR_SRC_ALPHA);
    p = xgu_set_blend_func_dfactor(p, XGU_FACTOR_ONE_MINUS_SRC_ALPHA);
    p = xgu_set_alpha_test_enable(p, false);
    p = xgu_set_blend_enable(p, true);
    p = pb_push1(p, NV097_SET_COMBINER_SPECULAR_FOG_CW1,
          MASK(NV097_SET_COMBINER_SPECULAR_FOG_CW1_E_SOURCE, _RC_ZERO)   | MASK(NV097_SET_COMBINER_SPECULAR_FOG_CW1_E_ALPHA, 0) | MASK(NV097_SET_COMBINER_SPECULAR_FOG_CW1_E_INVERSE, 0)
        | MASK(NV097_SET_COMBINER_SPECULAR_FOG_CW1_F_SOURCE, _RC_ZERO)   | MASK(NV097_SET_COMBINER_SPECULAR_FOG_CW1_F_ALPHA, 0) | MASK(NV097_SET_COMBINER_SPECULAR_FOG_CW1_F_INVERSE, 0)
        | MASK(NV097_SET_COMBINER_SPECULAR_FOG_CW1_G_SOURCE, _RC_PRIMARY_COLOR) | MASK(NV097_SET_COMBINER_SPECULAR_FOG_CW1_G_ALPHA, 1) | MASK(NV097_SET_COMBINER_SPECULAR_FOG_CW1_G_INVERSE, 0)
        | MASK(NV097_SET_COMBINER_SPECULAR_FOG_CW1_SPECULAR_CLAMP, 0));
    pb_end(p);
  }
#endif

#if 1
  if (SDL_GameControllerGetButton(g, SDL_CONTROLLER_BUTTON_DPAD_UP) || SDL_GameControllerGetButton(g, SDL_CONTROLLER_BUTTON_DPAD_DOWN)) {
    // Output selector
    unsigned int source = _RC_PRIMARY_COLOR; //_RC_TEXTURE+0;
    int alpha = SDL_GameControllerGetButton(g, SDL_CONTROLLER_BUTTON_DPAD_UP) ? 1 : 0;
    pb_print("%s;", alpha ? "A" : "RGB");
    uint32_t* p = pb_begin();
    p = pb_push1(p, NV097_SET_COMBINER_SPECULAR_FOG_CW0,
          MASK(NV097_SET_COMBINER_SPECULAR_FOG_CW0_A_SOURCE, _RC_ZERO)   | MASK(NV097_SET_COMBINER_SPECULAR_FOG_CW0_A_ALPHA, 0) | MASK(NV097_SET_COMBINER_SPECULAR_FOG_CW0_A_INVERSE, 0)
        | MASK(NV097_SET_COMBINER_SPECULAR_FOG_CW0_B_SOURCE, _RC_ZERO)   | MASK(NV097_SET_COMBINER_SPECULAR_FOG_CW0_B_ALPHA, 0) | MASK(NV097_SET_COMBINER_SPECULAR_FOG_CW0_B_INVERSE, 0)
        | MASK(NV097_SET_COMBINER_SPECULAR_FOG_CW0_C_SOURCE, _RC_ZERO)   | MASK(NV097_SET_COMBINER_SPECULAR_FOG_CW0_C_ALPHA, 0) | MASK(NV097_SET_COMBINER_SPECULAR_FOG_CW0_C_INVERSE, 0)
        | MASK(NV097_SET_COMBINER_SPECULAR_FOG_CW0_D_SOURCE, source) | MASK(NV097_SET_COMBINER_SPECULAR_FOG_CW0_D_ALPHA, alpha) | MASK(NV097_SET_COMBINER_SPECULAR_FOG_CW0_D_INVERSE, 0));
    p = pb_push1(p, NV097_SET_COMBINER_SPECULAR_FOG_CW1,
          MASK(NV097_SET_COMBINER_SPECULAR_FOG_CW1_E_SOURCE, _RC_ZERO)   | MASK(NV097_SET_COMBINER_SPECULAR_FOG_CW1_E_ALPHA, 0) | MASK(NV097_SET_COMBINER_SPECULAR_FOG_CW1_E_INVERSE, 0)
        | MASK(NV097_SET_COMBINER_SPECULAR_FOG_CW1_F_SOURCE, _RC_ZERO)   | MASK(NV097_SET_COMBINER_SPECULAR_FOG_CW1_F_ALPHA, 0) | MASK(NV097_SET_COMBINER_SPECULAR_FOG_CW1_F_INVERSE, 0)
        | MASK(NV097_SET_COMBINER_SPECULAR_FOG_CW1_G_SOURCE, _RC_ZERO) | MASK(NV097_SET_COMBINER_SPECULAR_FOG_CW1_G_ALPHA, 0) | MASK(NV097_SET_COMBINER_SPECULAR_FOG_CW1_G_INVERSE, 1)
        | MASK(NV097_SET_COMBINER_SPECULAR_FOG_CW1_SPECULAR_CLAMP, 0));
    pb_end(p);
  }
#endif
_debugPrint("gone");
}


GL_API void GL_APIENTRY glTexGeni (GLenum coord, GLenum pname, GLint param) {
  assert(pname == GL_TEXTURE_GEN_MODE);
  switch(coord) {
  case GL_S:
    state.texgen_s[active_texture] = param;
    break;
  case GL_T:
    state.texgen_t[active_texture] = param;
    break;
  default:
    unimplemented("%d", coord);
    assert(false);
    break;
  }
}



GL_API void GL_APIENTRY glGetIntegerv (GLenum pname, GLint *data) {
  switch(pname) {
  case GL_MAX_TEXTURE_SIZE:
    data[0] = 1024; //FIXME: We can do more probably
    break;
  case GL_MAX_TEXTURE_UNITS:
    data[0] = 4;
    break;
  case GL_MATRIX_MODE:
    data[0] = matrix_mode;
    break;
  default:
    unimplemented("%d", pname);
    assert(false);
    break;
  }
}

GL_API void GL_APIENTRY glGetFloatv (GLenum pname, GLfloat *data) {
  switch(pname) {
  case GL_MODELVIEW_MATRIX:
    memcpy(data, &matrix_mv[matrix_mv_slot * 16], 16 * sizeof(GLfloat));
    break;
  case GL_PROJECTION_MATRIX:
    memcpy(data, &matrix_p[matrix_p_slot * 16], 16 * sizeof(GLfloat));
    break;
  default:
    unimplemented("%d", pname);
    assert(false);
    break;
  }
}


GL_API const GLubyte *GL_APIENTRY glGetString (GLenum name) {
  const char* result = "";
  switch(name) {
  case GL_EXTENSIONS:
    result = "";
    break;
  case GL_VERSION:
    result = "OpenGL ES-CL 1.1";
    break;
  case GL_RENDERER:
    result = "XGU";
    break;
  case GL_VENDOR:
    result = "Jannik Vogel";
    break;
  default:
    unimplemented("%d", name);
    assert(false);
    break;
  }
  return (GLubyte*)result;
}


// Clearing
GL_API void GL_APIENTRY glClear (GLbitfield mask) {

  XguClearSurface flags = 0;
  if (mask & GL_COLOR_BUFFER_BIT)   { flags |= XGU_CLEAR_COLOR;   }
  if (mask & GL_DEPTH_BUFFER_BIT)   { flags |= XGU_CLEAR_Z;       }
  if (mask & GL_STENCIL_BUFFER_BIT) { flags |= XGU_CLEAR_STENCIL; }

  //FIXME: Store size we used to init
  int width = pb_back_buffer_width();
  int height = pb_back_buffer_height();

  uint32_t* p = pb_begin();
  p = xgu_set_clear_rect_horizontal(p, 0, width);
  p = xgu_set_clear_rect_vertical(p, 0, height);
  p = xgu_set_zstencil_clear_value(p, 0xFFFFFF00); //FIXME: This assumes Z24S8
  p = xgu_clear_surface(p, flags);
  pb_end(p);



  //FIXME: Remove this hack!
  //pb_erase_depth_stencil_buffer(0, 0, width, height); //FIXME: Do in XGU
  //pb_fill(0, 0, width, height, 0x80808080); //FIXME: Do in XGU

}

GL_API void GL_APIENTRY glClearColor (GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha) {

  uint32_t color = 0;
  //FIXME: Verify order
  color |= f_to_u8(red) << 16;
  color |= f_to_u8(green) << 8;
  color |= f_to_u8(blue) << 0;
  color |= f_to_u8(alpha) << 24;

  uint32_t* p = pb_begin();
  p = xgu_set_color_clear_value(p, color);
  pb_end(p);
}


// Buffers
GL_API void GL_APIENTRY glGenBuffers (GLsizei n, GLuint *buffers) {
  Buffer buffer = DEFAULT_BUFFER();
  gen_objects(n, buffers, &buffer, sizeof(Buffer));
}

GL_API void GL_APIENTRY glBindBuffer (GLenum target, GLuint buffer) {
  GLuint* bound_buffer_store = get_bound_buffer_store(target);
  *bound_buffer_store = buffer;
}

GL_API void GL_APIENTRY glBufferData (GLenum target, GLsizeiptr size, const void *data, GLenum usage) {
  Buffer* buffer = objects[*get_bound_buffer_store(target)-1].data;
  if (buffer->data != NULL) {
    //FIXME: Re-use existing buffer if it's a good fit?
    assert(false); //FIXME: Assert that this memory is no longer used
    FreeResourceMemory(buffer->data);
  }
  buffer->data = AllocateResourceMemory(size);
  buffer->size = size;
  assert(buffer->data != NULL);
  if (data != NULL) {
    memcpy(buffer->data, data, size);
  } else {
    memset(buffer->data, 0x00, size);
  }
}

GL_API void GL_APIENTRY glBufferSubData (GLenum target, GLintptr offset, GLsizeiptr size, const void *data) {
  Buffer* buffer = objects[*get_bound_buffer_store(target)-1].data;
  assert(buffer->data != NULL);
  assert(buffer->size >= (offset + size));
  assert(data != NULL);
  memcpy(&buffer->data[offset], data, size);
debugPrint("Set %d bytes at %d in %p\n", size, offset, &buffer->data[offset]);
}

GL_API void GL_APIENTRY glDeleteBuffers (GLsizei n, const GLuint *buffers) {
  for(int i = 0; i < n; i++) {

    //FIXME: Also ignore non-existing names
    if (buffers[i] == 0) {
      continue;
    }

    Buffer* buffer = objects[buffers[i]-1].data;
    if (buffer->data != NULL) {
      unimplemented(); //FIXME: Assert that the data is no longer used
      FreeResourceMemory(buffer->data);
    }
  }
  del_objects(n, buffers);
}

static void store_attrib_pointer(Attrib* attrib, GLenum gl_type, unsigned int size, size_t stride, const void* pointer) {
  uintptr_t base;
  if (gl_array_buffer == 0) {
    base = 0;
  } else {
    Buffer* buffer = objects[gl_array_buffer-1].data;
    base = (uintptr_t)buffer->data;
    assert(base != 0);
  }
  attrib->array.gl_type = gl_type;
  attrib->array.size = size;
  attrib->array.stride = stride;
  attrib->array.data = (const void*)(base + (uintptr_t)pointer);
}

// Vertex buffers
GL_API void GL_APIENTRY glTexCoordPointer (GLint size, GLenum type, GLsizei stride, const void *pointer) {
  store_attrib_pointer(&state.texture_coord_array[client_active_texture], type, size, stride, pointer);
}

GL_API void GL_APIENTRY glColorPointer (GLint size, GLenum type, GLsizei stride, const void *pointer) {
  store_attrib_pointer(&state.color_array, type, size, stride, pointer);
}

GL_API void GL_APIENTRY glNormalPointer (GLenum type, GLsizei stride, const void *pointer) {
  store_attrib_pointer(&state.normal_array, type, 3, stride, pointer);
}

GL_API void GL_APIENTRY glVertexPointer (GLint size, GLenum type, GLsizei stride, const void *pointer) {
  store_attrib_pointer(&state.vertex_array, type, size, stride, pointer);
}

static uint32_t* borders(uint32_t* p) {

/*
  p = xgu_begin(p, XGU_LINE_STRIP);
  p = xgux_set_color3f(p, 1.0f, 0.0f, 0.0f);
  p = xgu_vertex4f(p,  10,  10, 1.0f, 1.0f);
  p = xgu_vertex4f(p,  10, 470, 1.0f, 1.0f);
  p = xgu_vertex4f(p, 630, 470, 1.0f, 1.0f);
  p = xgu_vertex4f(p, 630,  10, 1.0f, 1.0f);
  p = xgu_vertex4f(p,  10,  10, 1.0f, 1.0f);
  p = xgu_end(p);

  p = xgu_begin(p, XGU_LINE_STRIP);
  p = xgux_set_color3f(p, 0.0f, 0.0f, 1.0f);
  p = xgu_vertex4f(p, -310, -230, 1.0f, 1.0f);
  p = xgu_vertex4f(p, -310,  230, 1.0f, 1.0f);
  p = xgu_vertex4f(p,  310,  230, 1.0f, 1.0f);
  p = xgu_vertex4f(p,  310, -230, 1.0f, 1.0f);
  p = xgu_vertex4f(p, -310, -230, 1.0f, 1.0f);
  p = xgu_end(p);
*/

/*
  p = xgu_begin(p, XGU_TRIANGLE_STRIP);
  p = xgux_set_color3f(p, 1.0f, 1.0f, 1.0f);
  for(int i = -1; i < 4; i++)
  p = xgux_set_texcoord3f(p, i, 0.0f, 0.0f, 1.0f); p = xgu_vertex4f(p, -310, -230, 1.0f, 1.0f);
  for(int i = -1; i < 4; i++)
  p = xgux_set_texcoord3f(p, i, 32.0f, 0.0f, 1.0f); p = xgu_vertex4f(p,  310, -230, 1.0f, 1.0f);
  for(int i = -1; i < 4; i++)
  p = xgux_set_texcoord3f(p, i, 0.0f, 32.0f, 1.0f); p = xgu_vertex4f(p, -310,  230, 1.0f, 1.0f);
  for(int i = -1; i < 4; i++)
  p = xgux_set_texcoord3f(p, i, 32.0f, 32.0f, 1.0f); p = xgu_vertex4f(p,  310,  230, 1.0f, 1.0f);
  p = xgu_end(p);
*/

  return p;
}


// Draw calls
GL_API void GL_APIENTRY glDrawArrays (GLenum mode, GLint first, GLsizei count) {

#if 0
unimplemented("disabled due to errors on physical hardware");
return; //FIXME: !!!
#endif

  static unsigned int f = -1;
  if (f == frame) {
    //return;
  }
  f = frame;

  prepare_drawing();

debugPrint("drawarrays");
  xgux_draw_arrays(gl_to_xgu_primitive_type(mode), first, count);

#if 1
  uint32_t* p = pb_begin();
#if 0
  p = xgu_begin(p, XGU_LINE_STRIP); //gl_to_xgu_primitive_type(mode));
  pb_end(p);
  print_attrib(XGU_VERTEX_ARRAY, &state.vertex_array, first, count, true);
  p = pb_begin();
  p = xgu_end(p);
#endif
p = borders(p);
  pb_end(p);
#endif
}

GL_API void GL_APIENTRY glDrawElements (GLenum mode, GLsizei count, GLenum type, const void *indices) {

//return;

  static unsigned int f = -1;
  if (f == frame) {
    //return;
  }
  f = frame;

  prepare_drawing();
debugPrint("elements ");

  uintptr_t base;
  if (gl_element_array_buffer == 0) {
    base = 0;
  } else {
    Buffer* buffer = objects[gl_element_array_buffer-1].data;
    base = (uintptr_t)buffer->data;
    assert(base != 0);
  }

  // This function only handles the 16 bit variant for now
  switch(type) {
  case GL_UNSIGNED_SHORT: {
    const uint16_t* indices_ptr = (const uint16_t*)(base + (uintptr_t)indices);
    xgux_draw_elements16(gl_to_xgu_primitive_type(mode), indices_ptr, count);
#if 1
    uint32_t* p = pb_begin();
#if 0
    p = xgu_begin(p, XGU_LINE_STRIP); //gl_to_xgu_primitive_type(mode));
    p = xgu_vertex3f(p, 0, 0, 1);
    pb_end(p);
    for(unsigned int i = 0; i < count; i++) {
      print_attrib(XGU_VERTEX_ARRAY, &state.vertex_array, indices_ptr[i], 1, true); 
    }
    p = pb_begin();
    p = xgu_end(p);
#endif
p = borders(p);
    pb_end(p);
#endif
    break;
  }
#if 0
  //FIXME: Not declared in gl.h + untested
  case GL_UNSIGNED_INT: {
    const uint32_t* indices_ptr = (const uint32_t*)(base + (uintptr_t)indices);
    xgux_draw_elements32(gl_to_xgu_primitive_type(mode), indices_ptr, count);
    break;
  }
#endif
  default:
    unimplemented("%d", type);
    assert(false);
    break;
  }
}


// Matrix functions
GL_API void GL_APIENTRY glMatrixMode (GLenum mode) {
  CHECK_MATRIX(matrix);

  // Remember mode
  matrix_mode = mode;

  switch(mode) {
  case GL_PROJECTION:
    matrix = &matrix_p[0];
    matrix_slot = &matrix_p_slot;
    break;
  case GL_MODELVIEW:
    matrix = &matrix_mv[0];
    matrix_slot = &matrix_mv_slot;
    break;
  case GL_TEXTURE:
    matrix = &matrix_t[client_active_texture][0];
    matrix_slot = &matrix_t_slot[client_active_texture];
    break;
  default:
    unimplemented("%d", mode);
    assert(false);
    break;
  }

  // Go to the current slot
  matrix = &matrix[*matrix_slot * 4*4];

  CHECK_MATRIX(matrix);
}

GL_API void GL_APIENTRY glLoadIdentity (void) {
  matrix_identity(matrix);

  CHECK_MATRIX(matrix);
}

#undef glMultMatrixf

static void _glMultMatrixf (const GLfloat *m) {

debugPrint("_glMultMatrixf:\n");
  CHECK_MATRIX(matrix);
//PRINT_MATRIX(matrix);
  PRINT_MATRIX(m);

  float t[4 * 4];
  matmul4(t, matrix, m);
  memcpy(matrix, t, sizeof(t));

  CHECK_MATRIX(matrix);
}

GL_API void GL_APIENTRY glMultMatrixf (const GLfloat *m) {

  float t[4 * 4];
  memcpy(t, m, sizeof(t));

//TRANSPOSE(t);

  _glMultMatrixf(t);
}

GL_API void GL_APIENTRY glOrthof (GLfloat l, GLfloat r, GLfloat b, GLfloat t, GLfloat n, GLfloat f) {
  float m[4*4];
  ortho(m, l, r, b, t, n, f);
  CHECK_MATRIX(m);
  _glMultMatrixf(m);
  CHECK_MATRIX(matrix);
}

GL_API void GL_APIENTRY glTranslatef (GLfloat x, GLfloat y, GLfloat z) {
  float m[4*4];
  matrix_identity(m);
  _math_matrix_translate(m, x, y, z);

#if 0
debugPrint("\ntrans: ");
debugPrintFloat(x); debugPrint(" ");
debugPrintFloat(y); debugPrint(" ");
debugPrintFloat(z); debugPrint("\n");
PRINT_MATRIX(m);
#endif

  CHECK_MATRIX(m);
  _glMultMatrixf(m);

  CHECK_MATRIX(matrix);
}

GL_API void GL_APIENTRY glRotatef (GLfloat angle, GLfloat x, GLfloat y, GLfloat z) {
  float m[4*4];
  matrix_identity(m);
  _math_matrix_rotate(m, angle, x, y, z);
  CHECK_MATRIX(m);
  _glMultMatrixf(m);

  CHECK_MATRIX(matrix);
}

GL_API void GL_APIENTRY glScalef (GLfloat x, GLfloat y, GLfloat z) {
  float m[4*4];
  matrix_identity(m);
  _math_matrix_scale(m, x, y, z);
  CHECK_MATRIX(m);

#if 0
debugPrint("\nscale: ");
debugPrintFloat(x); debugPrint(" ");
debugPrintFloat(y); debugPrint(" ");
debugPrintFloat(z); debugPrint("\n");
PRINT_MATRIX(m);
#endif

  _glMultMatrixf(m);
  CHECK_MATRIX(matrix);
}

GL_API void GL_APIENTRY glPopMatrix (void) {
  assert(*matrix_slot > 0);
  matrix -= 4*4;
  *matrix_slot -= 1;

  CHECK_MATRIX(matrix);
}

GL_API void GL_APIENTRY glPushMatrix (void) {
  CHECK_MATRIX(matrix);

  float* new_matrix = matrix;
  new_matrix += 4*4;
  memcpy(new_matrix, matrix, 4*4*sizeof(float));
  matrix = new_matrix;
  *matrix_slot += 1;
  assert(*matrix_slot < 16);
}


// Framebuffer setup
GL_API void GL_APIENTRY glViewport (GLint x, GLint y, GLsizei width, GLsizei height) {
  //FIXME: Switch to xgu variant to avoid side-effects
  debugPrint("%d %d %d %d\n", x, y, width, height);
  assert(x == 0);
  assert(y == 0);
  assert(width == 640); //FIXME: Set up game correctly
  assert(height == 480); //FIXME: Set up game correctly
  if (width > 640) { width = 640; } //FIXME: Remove!
  if (height > 480) { height = 480; } //FIXME: Remove!
  pb_set_viewport(x, y, width, height, 0.0f, 1.0f);
}


// Textures
GL_API void GL_APIENTRY glGenTextures (GLsizei n, GLuint *textures) {
  Texture texture = DEFAULT_TEXTURE();
  gen_objects(n, textures, &texture, sizeof(Texture));
}

GL_API void GL_APIENTRY glBindTexture (GLenum target, GLuint texture) {
  assert(target == GL_TEXTURE_2D);
  state.texture_binding_2d[active_texture] = texture;
}

GL_API void GL_APIENTRY glActiveTexture (GLenum texture) {
  active_texture = texture - GL_TEXTURE0;
}

GL_API void GL_APIENTRY glClientActiveTexture (GLenum texture) {
  client_active_texture = texture - GL_TEXTURE0;
  glMatrixMode(matrix_mode); // Necessary because the matrix pointers are cached
}

GL_API void GL_APIENTRY glDeleteTextures (GLsizei n, const GLuint *textures) {
  for(int i = 0; i < n; i++) {

    //FIXME: Also ignore non-existing names
    if (textures[i] == 0) {
      continue;
    }

    Texture* texture = objects[textures[i]-1].data;
    if (texture->data != NULL) {
      unimplemented(); //FIXME: Assert that the data is no longer used
      FreeResourceMemory(texture->data);
    }
  }
  del_objects(n, textures);
}

static void blend_2x2_bytes(unsigned int channels, const uint8_t* src_pixels, uint8_t* dst_pixels, unsigned int x, unsigned int y, size_t src_pitch, size_t dst_pitch) {
  for(int channel = 0; channel < channels; channel++) {
    dst_pixels[y * dst_pitch + x*channels + channel] = (src_pixels[(y*2+0) * src_pitch + (x*2+0)*channels + channel] +
                                                        src_pixels[(y*2+0) * src_pitch + (x*2+1)*channels + channel] +
                                                        src_pixels[(y*2+1) * src_pitch + (x*2+0)*channels + channel] +
                                                        src_pixels[(y*2+1) * src_pitch + (x*2+1)*channels + channel]) / 4;
  }
}

GL_API void GL_APIENTRY glTexImage2D (GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type, const void *pixels) {
  assert(target == GL_TEXTURE_2D);
  assert(border == 0);

  Texture* tx = get_bound_texture(active_texture);

  if (level > 0) {
    unimplemented();
    return;
  }

  unsigned int bpp;

  switch(internalformat) {
  case GL_LUMINANCE:
    assert(format == GL_LUMINANCE);
    assert(type == GL_UNSIGNED_BYTE);
    tx->internal_base_format = GL_LUMINANCE;
    bpp = 1*8;
    break;
  case GL_LUMINANCE_ALPHA:
    assert(format == GL_LUMINANCE_ALPHA);
    assert(type == GL_UNSIGNED_BYTE);
    tx->internal_base_format = GL_LUMINANCE_ALPHA;
    bpp = 2*8;
    break;
  case GL_RGB:
    assert(format == GL_RGB);
    assert(type == GL_UNSIGNED_BYTE);
    tx->internal_base_format = GL_RGB;
    bpp = 4*8; // 3*8 in GL, but we have to pad this
    break;
  case GL_RGBA:
    assert(format == GL_RGBA);
    assert(type == GL_UNSIGNED_BYTE);
    tx->internal_base_format = GL_RGBA;
    bpp = 4*8;
    break;
  default:
    tx->internal_base_format = -1;
    bpp = 0;
    unimplemented("%d", internalformat);
    assert(false);
    return;
  }

  tx->width_shift = __builtin_ffs(width) - 1;
  tx->height_shift = __builtin_ffs(height) - 1;
  debugPrint("%d = %d [%d]\n", width, 1 << tx->width_shift, tx->width_shift);
  debugPrint("%d = %d [%d]\n", height, 1 << tx->height_shift, tx->height_shift);
  assert(width == (1 << tx->width_shift));
  assert(height == (1 << tx->height_shift));

  tx->width = width;
  tx->height = height;
  tx->pitch = width * bpp / 8;
  if (tx->data != NULL) {
    //FIXME: Re-use existing buffer if it's a good fit?
    //FIXME: Assert that this memory is no longer used
    FreeResourceMemory(tx->data);
  }

  // Calculate required size for mipmaps
  unsigned int mip_levels = MAX(tx->width_shift, tx->height_shift) + 1;
  unsigned int width_shift = tx->width_shift;
  unsigned int height_shift = tx->height_shift;
  size_t size = 0;
  for(unsigned int mip_level = 0; mip_level < mip_levels; mip_level++) {
    debugPrint("%d %d %d\n", mip_level, width_shift, height_shift);
    size += (1 << (width_shift + height_shift)) * (bpp / 8);
    if (width_shift  > 0) { width_shift--;  }
    if (height_shift > 0) { height_shift--; }
  }
  tx->data = AllocateResourceMemory(size);

  // Allocate temporary space for generating mipmaps and swizzling source
  uint8_t* tmp = malloc(tx->width * tx->height * (bpp / 8));

  // Copy pixels
  //FIXME: Respect GL pixel packing stuff
  assert(pixels != NULL);
  if (tx->internal_base_format == GL_RGB) {
    assert(format == GL_RGB);
    assert(type == GL_UNSIGNED_BYTE);
    const uint8_t* src = pixels;
    uint8_t* dst = tmp;
    assert(tx->pitch == tx->width * 4);
    for(int y = 0; y < tx->height; y++) {
      for(int x = 0; x < tx->width; x++) {
        dst[0] = src[2];
        dst[1] = src[1];
        dst[2] = src[0];
        dst[3] = 0xFF;
        dst += 4;
        src += 3;
      }
    }
  } else {
    memcpy(tmp, pixels, tx->width * tx->height * (bpp / 8));
  }

  // Generate mipmaps
  unimplemented(); //FIXME: Make dependent on GL_GENERATE_MIPMAP_SGIS
  uint8_t* swizzled_pixels = (uint8_t*)tx->data;
  unsigned int w = 1 << tx->width_shift;
  unsigned int h = 1 << tx->height_shift;
  size_t pitch = w * (bpp / 8);
  unsigned int mip_level = 0;
  while(true) {

    // Swizzle data
    _debugPrint("Swizzling %dx%d [pitch %d, %dbpp] (%d / %d)\n", w, h, pitch, bpp, mip_level, mip_levels);
    swizzle_rect(tmp, w, h, swizzled_pixels, pitch, bpp / 8);

    // Check if there is another mipmap level
    mip_level++;
    if (mip_level >= mip_levels) {
      _debugPrint("Generated all levels!\n");
      break;
    }

    // Calculate offset and size of next mipmap level
    swizzled_pixels += pitch * h;
    size_t src_pitch = pitch;
    if (w > 1) { w /= 2; pitch /= 2; }
    if (h > 1) { h /= 2; }

    // We can run the downscale in place
    _debugPrint("Downscaling to %dx%d\n", w, h);
    for(unsigned int y = 0; y < h; y++) {
      for(unsigned int x = 0; x < w; x++) {
        blend_2x2_bytes(bpp / 8, tmp, tmp, x, y, src_pitch, pitch);
      }
    }



  }

  free(tmp);
}

GL_API void GL_APIENTRY glTexParameteri (GLenum target, GLenum pname, GLint param) {
  assert(target == GL_TEXTURE_2D);

  Texture* tx = get_bound_texture(active_texture);

  switch(pname) {
  case GL_GENERATE_MIPMAP_SGIS:
    unimplemented(); //FIXME: Make this optional (currently assumed true)
    break;
  case GL_TEXTURE_MIN_FILTER:
    tx->min_filter = param;
    break;
  case GL_TEXTURE_MAG_FILTER:
    tx->mag_filter = param;
    break;
  case GL_TEXTURE_WRAP_S:
    tx->wrap_s = param;
    break;
  case GL_TEXTURE_WRAP_T:
    tx->wrap_t = param;
    break;
  default:
    unimplemented("%d", pname);
    assert(false);
    return;
  }
}


// Renderstates
GL_API void GL_APIENTRY glAlphaFunc (GLenum func, GLfloat ref) {
#if 0
  //FIXME: https://github.com/dracc/nxdk/pull/4/files#r357990989
  //       https://github.com/dracc/nxdk/pull/4/files#r357990961
  uint32_t* p = pb_begin();
  p = xgu_set_alpha_func(p, gl_to_xgu_alpha_func(func));
  p = xgu_set_alpha_ref(p, f_to_u8(ref));
  pb_end(p);
#endif
}

GL_API void GL_APIENTRY glBlendFunc (GLenum sfactor, GLenum dfactor) {
  uint32_t* p = pb_begin();
  p = xgu_set_blend_func_sfactor(p, gl_to_xgu_blend_factor(sfactor));
  p = xgu_set_blend_func_dfactor(p, gl_to_xgu_blend_factor(dfactor));
  pb_end(p);
}

GL_API void GL_APIENTRY glClipPlanef (GLenum p, const GLfloat *eqn) {
  //FIXME: Use a free texture stage and set it to cull mode
  GLuint index = p - GL_CLIP_PLANE0;
  assert(index < ARRAY_SIZE(clip_planes));

  ClipPlane* clip_plane = &clip_planes[index];

  float v[4];
  float m[4*4];
  bool ret = invert(m, &matrix_mv[matrix_mv_slot * 4*4]);
  mult_vec4_mat4(v, m, eqn); //FIXME: Multiply eqn by m to get the v

  clip_plane->x = v[0];
  clip_plane->y = v[1];
  clip_plane->z = v[2];
  clip_plane->w = v[3];
}

GL_API void GL_APIENTRY glColor4ub (GLubyte red, GLubyte green, GLubyte blue, GLubyte alpha) {
  //FIXME: Could save bandwidth using other commands; however, this is currently easier to maintain
  //FIXME: Won't work between begin/end
  state.color_array.value[0] = red / 255.0f;
  state.color_array.value[1] = green / 255.0f;
  state.color_array.value[2] = blue / 255.0f;
  state.color_array.value[3] = alpha / 255.0f;
}

GL_API void GL_APIENTRY glColor4f (GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha) {
  //FIXME: Won't work between begin/end
  state.color_array.value[0] = red;
  state.color_array.value[1] = green;
  state.color_array.value[2] = blue;
  state.color_array.value[3] = alpha;
}

GL_API void GL_APIENTRY glColorMask (GLboolean red, GLboolean green, GLboolean blue, GLboolean alpha) {

  XguColorMask mask = 0;
  if (red   != GL_FALSE) { mask |= XGU_RED;   }
  if (green != GL_FALSE) { mask |= XGU_GREEN; }
  if (blue  != GL_FALSE) { mask |= XGU_BLUE;  }
  if (alpha != GL_FALSE) { mask |= XGU_ALPHA; }

  uint32_t* p = pb_begin();
  p = xgu_set_color_mask(p, mask);
  pb_end(p);
}

GL_API void GL_APIENTRY glDepthFunc (GLenum func) {
  uint32_t* p = pb_begin();
  p = xgu_set_depth_func(p, gl_to_xgu_func_type(func));
  pb_end(p);
}

GL_API void GL_APIENTRY glDepthMask (GLboolean flag) {
  uint32_t* p = pb_begin();
  p = xgu_set_depth_mask(p, flag);
  pb_end(p);  
}

GL_API void GL_APIENTRY glEnable (GLenum cap) {
  uint32_t* p = pb_begin();
  p = set_enabled(p, cap, true);
  pb_end(p);
}

GL_API void GL_APIENTRY glDisable (GLenum cap) {
  uint32_t* p = pb_begin();
  p = set_enabled(p, cap, false);
  pb_end(p);
}

GL_API void GL_APIENTRY glEnableClientState (GLenum array) {
  set_client_state_enabled(array, true);
}

GL_API void GL_APIENTRY glDisableClientState (GLenum array) {
  set_client_state_enabled(array, false);
}

GL_API void GL_APIENTRY glCullFace (GLenum mode) {
  uint32_t* p = pb_begin();
  p = xgu_set_cull_face(p, gl_to_xgu_cull_face(mode));
  pb_end(p);
}

GL_API void GL_APIENTRY glFrontFace (GLenum mode) {
  uint32_t* p = pb_begin();
  p = xgu_set_front_face(p, gl_to_xgu_front_face(mode));
  pb_end(p);
}


// Stencil actions
GL_API void GL_APIENTRY glStencilFunc (GLenum func, GLint ref, GLuint mask) {
  uint32_t* p = pb_begin();
  p = xgu_set_stencil_func(p, gl_to_xgu_func_type(func));
  p = xgu_set_stencil_func_ref(p, ref);
  p = xgu_set_stencil_func_mask(p, mask);
  pb_end(p);
}

GL_API void GL_APIENTRY glStencilOp (GLenum fail, GLenum zfail, GLenum zpass) {
  uint32_t* p = pb_begin();
  p = xgu_set_stencil_op_fail(p, gl_to_xgu_stencil_op(fail));
  p = xgu_set_stencil_op_zfail(p, gl_to_xgu_stencil_op(zfail));
  p = xgu_set_stencil_op_zpass(p, gl_to_xgu_stencil_op(zpass));
  pb_end(p);
}


// Misc.
GL_API void GL_APIENTRY glPointParameterf (GLenum pname, GLfloat param) {
  unimplemented(); //FIXME: Bad in XGU
}

GL_API void GL_APIENTRY glPointParameterfv (GLenum pname, const GLfloat *params) {
  unimplemented(); //FIXME: Bad in XGU
}

GL_API void GL_APIENTRY glPointSize (GLfloat size) {
  unimplemented(); //FIXME: Bad in XGU
}

GL_API void GL_APIENTRY glPolygonOffset (GLfloat factor, GLfloat units) {
  unimplemented(); //FIXME: Missing from XGU
}


// TexEnv
GL_API void GL_APIENTRY glTexEnvi (GLenum target, GLenum pname, GLint param) {

  // Deal with the weird pointsprite target first
  if (target == GL_POINT_SPRITE) {
    assert(pname == GL_COORD_REPLACE);
    unimplemented(); //FIXME: !!! Supported on Xbox?!
    return;
  }

  // Deal with actual texenv stuff now

  assert(target == GL_TEXTURE_ENV);

  TexEnv* t = &texenvs[active_texture];

  switch(pname) {

  case GL_TEXTURE_ENV_MODE:
    t->env_mode = param;
    break;

  case GL_COMBINE_RGB:
    t->combine_rgb = param;
    break;
  case GL_COMBINE_ALPHA:
    t->combine_alpha = param;
    break;

  case GL_SRC0_RGB:
    t->src_rgb[0] = param;
    break;
  case GL_SRC1_RGB:
    t->src_rgb[1] = param;
    break;
  case GL_SRC2_RGB:
    t->src_rgb[2] = param;
    break;

  case GL_SRC0_ALPHA:
    t->src_alpha[0] = param;
    break;

  case GL_OPERAND0_RGB:
    t->src_operand_rgb[0] = param;
    break;

  case GL_OPERAND1_RGB:
    t->src_operand_rgb[1] = param;
    break;

  case GL_OPERAND2_RGB:
    t->src_operand_rgb[2] = param;
    break;

  case GL_OPERAND0_ALPHA:
    t->src_operand_alpha[0] = param;
    break;

  default:
    unimplemented("%d", pname);
    assert(false);
    return;
  }
}


// Pixel readback
GL_API void GL_APIENTRY glReadPixels (GLint x, GLint y, GLsizei width, GLsizei height, GLenum format, GLenum type, void *pixels) {
  // Not implemented, only used in screenshots
  unimplemented();
}


// Lighting
GL_API void GL_APIENTRY glLightModelf (GLenum pname, GLfloat param) {
  switch(pname) {
  case GL_LIGHT_MODEL_TWO_SIDE:
    //FIXME: Why is this never called by neverball?
    uint32_t* p = pb_begin();
    p = xgu_set_two_side_light_enable(p, (bool)param);
    pb_end(p);
    break;
  case GL_LIGHT_MODEL_LOCAL_VIEWER:
    unimplemented(); //FIXME: Not provided by XGU yet
    //uint32_t* p = pb_begin();
    //p = xgu_set_local_viewer_enable(p, (bool)param);
    //pb_end(p);
    break;
  default:
    unimplemented("%d", pname);
    assert(false);
    return;
  }
}

GL_API void GL_APIENTRY glLightModelfv (GLenum pname, const GLfloat *params) {
  switch(pname) {
  case GL_LIGHT_MODEL_AMBIENT:
    state.light_model_ambient.r = params[0];
    state.light_model_ambient.g = params[1];
    state.light_model_ambient.b = params[2];
    state.light_model_ambient.a = params[3];
    break;
  default:
    unimplemented("%d", pname);
    assert(false);
    return;
  }
}

GL_API void GL_APIENTRY glLightfv (GLenum light, GLenum pname, const GLfloat *params) {
  unsigned int light_index = light - GL_LIGHT0;
  assert(light_index < GL_MAX_LIGHTS); //FIXME: Not sure how many lights Xbox has; there's probably some constant we can use
  Light* l = &state.lights[light_index];
  const float* modelViewMatrix = &matrix_mv[matrix_mv_slot * 4*4];
  switch(pname) {
  case GL_POSITION:
    mult_vec4_mat4(&l->position.x, modelViewMatrix, params);
    l->is_dir = (params[3] == 0.0f);
    //memcpy(&l->position.x, param, sizeof(l->position));
    break;
  case GL_AMBIENT:
    l->ambient.r = params[0];
    l->ambient.g = params[1];
    l->ambient.b = params[2];
    l->ambient.a = params[3];
    break;
  case GL_SPECULAR:
    l->specular.r = params[0];
    l->specular.g = params[1];
    l->specular.b = params[2];
    l->specular.a = params[3];
    break;
  case GL_DIFFUSE:
    l->diffuse.r = params[0];
    l->diffuse.g = params[1];
    l->diffuse.b = params[2];
    l->diffuse.a = params[3];
    break;
  case GL_SPOT_DIRECTION: {
    float t[4] = {
      params[0],
      params[1],
      params[2],
      0.0f
    };
    float f[4];
    mult_vec4_mat4(f, modelViewMatrix, t);
    l->spot_direction.x = f[0];
    l->spot_direction.y = f[1];
    l->spot_direction.z = f[2];
    break;
  }
  case GL_SPOT_CUTOFF:
    l->spot_cutoff = params[0];
    break;
  case GL_SPOT_EXPONENT:
    l->spot_exponent = params[0];
    break;
  case GL_CONSTANT_ATTENUATION:
    l->constant_attenuation = params[0];
    break;
  case GL_LINEAR_ATTENUATION:
    l->linear_attenuation = params[0];
    break;
  case GL_QUADRATIC_ATTENUATION:
    l->quadratic_attenuation = params[0];
    break;
  default:
    unimplemented("%d", pname);
    assert(false);
    return;
  }
}


// Materials

GL_API void GL_APIENTRY glColorMaterial (GLenum face, GLenum mode) {

  if (face == GL_FRONT_AND_BACK) {
    glColorMaterial(GL_FRONT, mode);
    glColorMaterial(GL_BACK, mode);
    return;
  }

  assert(face == GL_FRONT || face == GL_BACK);
  switch(face) {
  case GL_FRONT:
    state.color_material_front = mode;
    break;
  case GL_BACK:
    state.color_material_back = mode;
    break;
  default:
    unimplemented("%d", face);
    assert(false);
    return;
  }
}

GL_API void GL_APIENTRY glMaterialfv (GLenum face, GLenum pname, const GLfloat *params) {

  if (face == GL_FRONT_AND_BACK) {
    glMaterialfv(GL_FRONT, pname, params);
    glMaterialfv(GL_BACK, pname, params);
    return;
  }

  if (pname == GL_AMBIENT_AND_DIFFUSE) {
    glMaterialfv(face, GL_AMBIENT, params);
    glMaterialfv(face, GL_DIFFUSE, params);
    return;
  }

  assert(face == GL_FRONT || face == GL_BACK);
  unsigned int face_index = (face == GL_FRONT) ? 0 : 1;
  switch(pname) {
  case GL_SHININESS:
    state.material[face_index].shininess = params[0];
    break;
  case GL_EMISSION:
    state.material[face_index].emission.r = params[0];
    state.material[face_index].emission.g = params[1];
    state.material[face_index].emission.b = params[2];
    state.material[face_index].emission.a = params[3];
    break;
  case GL_AMBIENT:
    state.material[face_index].ambient.r = params[0];
    state.material[face_index].ambient.g = params[1];
    state.material[face_index].ambient.b = params[2];
    state.material[face_index].ambient.a = params[3];
    break;
  case GL_DIFFUSE:
    state.material[face_index].diffuse.r = params[0];
    state.material[face_index].diffuse.g = params[1];
    state.material[face_index].diffuse.b = params[2];
    state.material[face_index].diffuse.a = params[3];
    break;
  case GL_SPECULAR:
    state.material[face_index].specular.r = params[0];
    state.material[face_index].specular.g = params[1];
    state.material[face_index].specular.b = params[2];
    state.material[face_index].specular.a = params[3];
    break;
  default:
    unimplemented("%d", pname);
    assert(false);
    return;
  }
}


// Pixel pushing
GL_API void GL_APIENTRY glPixelStorei (GLenum pname, GLint param) {
  switch(pname) {
  case GL_PACK_ALIGNMENT:
    assert(param == 1);
    break;
  case GL_UNPACK_ALIGNMENT:
    assert(param == 1);
    break;
  default:
    unimplemented("%d", pname);
    assert(false);
    break;
  }
}





// SDL GL hooks via EGL

#include <EGL/egl.h>

#if 1
EGLAPI EGLBoolean EGLAPIENTRY eglSwapBuffers (EGLDisplay dpy, EGLSurface surface) {


  frame++;

#if 1
  //FIXME: This doesn't work for some reason
  static bool draw_debug = false;
  SDL_GameControllerUpdate();
  if (g == NULL) {
    g = SDL_GameControllerOpen(0);
    if (g == NULL) {
      debugPrint("failed to open gamepad\n");
      Sleep(100);
    }
  }
  if (g != NULL) {
    static bool pressed = true;
    bool button = SDL_GameControllerGetButton(g, SDL_CONTROLLER_BUTTON_BACK);
    if (button && !pressed) {
      draw_debug = !draw_debug;

      if (draw_debug) {
        pb_show_front_screen();
      } else {
        pb_show_debug_screen();
      }

    }
    pressed = button;
  }
#endif



  debugPrint("Going to swap buffers\n");

  /* Draw some text on the screen */
  static unsigned int frame = 0;
  frame++;  
  MM_STATISTICS mem_stats;
  mem_stats.Length = sizeof(mem_stats);
  MmQueryStatistics(&mem_stats);
  ULONGLONG duration_frequency = KeQueryPerformanceFrequency();
  pb_print("Frame: %u; memory: %uMiB / %uMiB; drawcalls: %u; pb: %ukiB; cpu: %ums; gpu: %ums\n",
           frame,
           ((mem_stats.TotalPhysicalPages - mem_stats.AvailablePages) * 4) / 1024,
           (mem_stats.TotalPhysicalPages * 4) / 1024,
           drawcall_count,
           p_size / 1024,
           (unsigned int)((cpu_duration * 1000ULL) / duration_frequency),
           (unsigned int)((gpu_duration * 1000ULL) / duration_frequency));
  pb_draw_text_screen();
  drawcall_count = 0;
  p_size = 0;
  cpu_duration = 0;
  gpu_duration = 0;

  // Wait for GPU and go to start of pushbuffer
  reset_pb();

  // Swap buffers
  while(pb_finished());

  // Prepare next frame
  //pb_wait_for_vbl();
  pb_target_back_buffer();
  pb_erase_text_screen();

  return EGL_TRUE;
}
#endif





// Initialization

#include <hal/xbox.h>
#include <pbkit/pbkit.h>
#include <xboxkrnl/xboxkrnl.h>

__attribute__((constructor(0xFFFFFFFF))) static void gl_init(void) {

  //FIXME: Bump GPU in right state?
  uint32_t* p = pb_begin();
  uint32_t control0 = 0;
#define NV097_SET_CONTROL0_TEXTUREPERSPECTIVE 0x100000
  control0 |= NV097_SET_CONTROL0_TEXTUREPERSPECTIVE;
  control0 |= NV097_SET_CONTROL0_STENCIL_WRITE_ENABLE;

  float max_z = 0xFFFFFF;
#if 0
  // W-buffer
  p=pb_push1(p,NV097_SET_ZMIN_MAX_CONTROL,0); //CULL_NEAR_FAR_EN_FALSE | ZCLAMP_EN_CULL | CULL_IGNORE_W_FALSE

  float w_far = 1.0f;
  float f = max_z / w_far;

  //FIXME: Does not work yet

  float tmp[4*4] = {
      320.0f*f,      0.0f,           0.0f, 0.0f,
        0.0f,   -240.0f*f,           0.0f, 0.0f,
        0.0f,        0.0f, max_z / 2.0f*f, 0.0f,
      320.0f*f,  240.0f*f, max_z / 2.0f*f,    f,
  };

  control0 |= NV097_SET_CONTROL0_Z_PERSPECTIVE_ENABLE;

#else
  // Z-buffer
  p=pb_push1(p,NV097_SET_ZMIN_MAX_CONTROL,1); //CULL_NEAR_FAR_EN_TRUE | ZCLAMP_EN_CULL | CULL_IGNORE_W_FALSE

  float tmp[4*4] = {
      320.0f,    0.0f,         0.0f, 0.0f,
        0.0f, -240.0f,         0.0f, 0.0f,
        0.0f,    0.0f, max_z / 2.0f, 0.0f,
      320.0f,  240.0f, max_z / 2.0f, 1.0f,
  };
  memcpy(viewport_matrix, tmp, sizeof(tmp));

#endif
  //control0 |= NV097_SET_CONTROL0_Z_FORMAT; // Float if present; fixed otherwise
  p = pb_push1(p, NV097_SET_CONTROL0, control0);
  p = xgu_set_clip_min(p, (float)0x000000);
  p = xgu_set_clip_max(p, (float)0xFFFFFF);
  p = pb_push1(p,NV097_SET_COMPRESS_ZBUFFER_EN,1); //FIXME: Does this work?
  pb_end(p);


  // Set up some defaults
  glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
  //FIXME: backport these from nv2a-re:
  //glClearDepth(1.0);
  //glClearStencil(0);

  glDisable(GL_STENCIL_TEST);
  //FIXME: backport this from nv2a-re:
  //glStencilMask(~0);
  glStencilFunc(GL_ALWAYS, 0, ~0);
  glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);

  glDisable(GL_DEPTH_TEST);
  glDepthFunc(GL_LESS);

  glDisable(GL_NORMALIZE);

  glMatrixMode(GL_MODELVIEW);

  glColor4f(1.0f, 1.0f, 1.0f, 1.0f);

  glDisable(GL_LIGHTING);
  { float f[] = { 0.2f, 0.2f, 0.2f, 1.0f }; glLightModelfv(GL_LIGHT_MODEL_AMBIENT, f); }

  { float f[] = { 0.2f, 0.2f, 0.2f, 1.0f }; glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, f); }
  { float f[] = { 0.8f, 0.8f, 0.8f, 1.0f }; glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, f); }
  { float f[] = { 0.0f, 0.0f, 0.0f, 1.0f }; glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, f); }
  { float f[] = { 0.0f, 0.0f, 0.0f, 1.0f }; glMaterialfv(GL_FRONT_AND_BACK, GL_EMISSION, f); }
  { float f[] = { 0.0f }; glMaterialfv(GL_FRONT_AND_BACK, GL_SHININESS, f); }

  glLightModelf(GL_LIGHT_MODEL_LOCAL_VIEWER, 0);
  glLightModelf(GL_LIGHT_MODEL_TWO_SIDE, 0);

  for(int i = 0; i < GL_MAX_LIGHTS; i++) {
    glDisable(GL_LIGHT0 + i);
    { float f[] = {0,0,0,1}; glLightfv(GL_LIGHT0 + i, GL_AMBIENT, f); }
    if (i == 0) {
      { float f[] = {1,1,1,1}; glLightfv(GL_LIGHT0 + i, GL_DIFFUSE, f); }
      { float f[] = {1,1,1,1}; glLightfv(GL_LIGHT0 + i, GL_SPECULAR, f); }
    } else {
      { float f[] = {0,0,0,1}; glLightfv(GL_LIGHT0 + i, GL_DIFFUSE, f); }
      { float f[] = {0,0,0,1}; glLightfv(GL_LIGHT0 + i, GL_SPECULAR, f); }
    }
    { float f[] = {0,0,1,0}; glLightfv(GL_LIGHT0 + i, GL_POSITION, f); }
    { float f[] = {0,0,-1}; glLightfv(GL_LIGHT0 + i, GL_SPOT_DIRECTION, f); }
    { float f[] = {180}; glLightfv(GL_LIGHT0 + i, GL_SPOT_CUTOFF, f); }
    { float f[] = {0}; glLightfv(GL_LIGHT0 + i, GL_SPOT_EXPONENT, f); }
    { float f[] = {1}; glLightfv(GL_LIGHT0 + i, GL_CONSTANT_ATTENUATION, f); }
    { float f[] = {0}; glLightfv(GL_LIGHT0 + i, GL_LINEAR_ATTENUATION, f); }
    { float f[] = {0}; glLightfv(GL_LIGHT0 + i, GL_QUADRATIC_ATTENUATION, f); }
  }

  glFrontFace(GL_CW);

  glDisable(GL_COLOR_MATERIAL);
  glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);

}
