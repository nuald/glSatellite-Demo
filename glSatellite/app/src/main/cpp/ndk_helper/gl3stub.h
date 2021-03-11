#ifndef __gl3_h_
#define __gl3_h_

/*
 * stub gl3.h for dynamic loading, based on:
 * gl3.h last updated on $Date: 2013-02-12 14:37:24 -0800 (Tue, 12 Feb 2013) $
 *
 * Changes:
 * - Added #include <GLES2/gl2.h>
 * - Removed duplicate OpenGL ES 2.0 declarations
 * - Converted OpenGL ES 3.0 function prototypes to function pointer
 *   declarations
 * - Added gl3stubInit() declaration
 */

#include <GLES2/gl2.h>
#include <android/api-level.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 ** Copyright (c) 2007-2013 The Khronos Group Inc.
 **
 ** Permission is hereby granted, free of charge, to any person obtaining a
 ** copy of this software and/or associated documentation files (the
 ** "Materials"), to deal in the Materials without restriction, including
 ** without limitation the rights to use, copy, modify, merge, publish,
 ** distribute, sublicense, and/or sell copies of the Materials, and to
 ** permit persons to whom the Materials are furnished to do so, subject to
 ** the following conditions:
 **
 ** The above copyright notice and this permission notice shall be included
 ** in all copies or substantial portions of the Materials.
 **
 ** THE MATERIALS ARE PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 ** EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 ** MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 ** IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 ** CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 ** TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 ** MATERIALS OR THE USE OR OTHER DEALINGS IN THE MATERIALS.
 */

/*
 * This files is for apps that want to use ES3 if present,
 * but continue to work on pre-API-18 devices. They can't just link to -lGLESv3
 *since
 * that library doesn't exist on pre-API-18 devices.
 * The function dynamically check if OpenGLES3.0 APIs are present and fill in if
 *there are.
 * Also the header defines some extra variables for OpenGLES3.0.
 *
 */

/* Call this function before calling any OpenGL ES 3.0 functions. It will
 * return GL_TRUE if the OpenGL ES 3.0 was successfully initialized, GL_FALSE
 * otherwise. */
GLboolean gl3stubInit();

/*-------------------------------------------------------------------------
 * Data type definitions
 *-----------------------------------------------------------------------*/

/* OpenGL ES 3.0 */

#if __ANDROID_API__ <= 19
typedef khronos_int64_t GLint64;
typedef khronos_uint64_t GLuint64;
typedef struct __GLsync* GLsync;
#endif

/*-------------------------------------------------------------------------
 * Entrypoint definitions
 *-----------------------------------------------------------------------*/

/* OpenGL ES 3.0 */

extern GL_APICALL void (*GL_APIENTRY glReadBuffer)(GLenum mode);
extern GL_APICALL void (*GL_APIENTRY glDrawRangeElements)(
    GLenum mode, GLuint start, GLuint end, GLsizei count, GLenum type,
    const GLvoid* indices);
extern GL_APICALL void (*GL_APIENTRY glTexImage3D)(
    GLenum target, GLint level, GLint internalformat, GLsizei width,
    GLsizei height, GLsizei depth, GLint border, GLenum format, GLenum type,
    const GLvoid* pixels);
extern GL_APICALL void (*GL_APIENTRY glTexSubImage3D)(
    GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset,
    GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLenum type,
    const GLvoid* pixels);
extern GL_APICALL void (*GL_APIENTRY glCopyTexSubImage3D)(
    GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset,
    GLint x, GLint y, GLsizei width, GLsizei height);
extern GL_APICALL void (*GL_APIENTRY glCompressedTexImage3D)(
    GLenum target, GLint level, GLenum internalformat, GLsizei width,
    GLsizei height, GLsizei depth, GLint border, GLsizei imageSize,
    const GLvoid* data);
extern GL_APICALL void (*GL_APIENTRY glCompressedTexSubImage3D)(
    GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset,
    GLsizei width, GLsizei height, GLsizei depth, GLenum format,
    GLsizei imageSize, const GLvoid* data);
extern GL_APICALL void (*GL_APIENTRY glGenQueries)(GLsizei n, GLuint* ids);
extern GL_APICALL void (*GL_APIENTRY glDeleteQueries)(GLsizei n,
                                                      const GLuint* ids);
extern GL_APICALL GLboolean (*GL_APIENTRY glIsQuery)(GLuint id);
extern GL_APICALL void (*GL_APIENTRY glBeginQuery)(GLenum target, GLuint id);
extern GL_APICALL void (*GL_APIENTRY glEndQuery)(GLenum target);
extern GL_APICALL void (*GL_APIENTRY glGetQueryiv)(GLenum target, GLenum pname,
                                                   GLint* params);
extern GL_APICALL void (*GL_APIENTRY glGetQueryObjectuiv)(GLuint id,
                                                          GLenum pname,
                                                          GLuint* params);
extern GL_APICALL GLboolean (*GL_APIENTRY glUnmapBuffer)(GLenum target);
extern GL_APICALL void (*GL_APIENTRY glGetBufferPointerv)(GLenum target,
                                                          GLenum pname,
                                                          GLvoid** params);
extern GL_APICALL void (*GL_APIENTRY glDrawBuffers)(GLsizei n,
                                                    const GLenum* bufs);
extern GL_APICALL void (*GL_APIENTRY glUniformMatrix2x3fv)(
    GLint location, GLsizei count, GLboolean transpose, const GLfloat* value);
extern GL_APICALL void (*GL_APIENTRY glUniformMatrix3x2fv)(
    GLint location, GLsizei count, GLboolean transpose, const GLfloat* value);
extern GL_APICALL void (*GL_APIENTRY glUniformMatrix2x4fv)(
    GLint location, GLsizei count, GLboolean transpose, const GLfloat* value);
extern GL_APICALL void (*GL_APIENTRY glUniformMatrix4x2fv)(
    GLint location, GLsizei count, GLboolean transpose, const GLfloat* value);
extern GL_APICALL void (*GL_APIENTRY glUniformMatrix3x4fv)(
    GLint location, GLsizei count, GLboolean transpose, const GLfloat* value);
extern GL_APICALL void (*GL_APIENTRY glUniformMatrix4x3fv)(
    GLint location, GLsizei count, GLboolean transpose, const GLfloat* value);
extern GL_APICALL void (*GL_APIENTRY glBlitFramebuffer)(
    GLint srcX0, GLint srcY0, GLint srcX1, GLint srcY1, GLint dstX0,
    GLint dstY0, GLint dstX1, GLint dstY1, GLbitfield mask, GLenum filter);
extern GL_APICALL void (*GL_APIENTRY glRenderbufferStorageMultisample)(
    GLenum target, GLsizei samples, GLenum internalformat, GLsizei width,
    GLsizei height);
extern GL_APICALL void (*GL_APIENTRY glFramebufferTextureLayer)(
    GLenum target, GLenum attachment, GLuint texture, GLint level, GLint layer);
extern GL_APICALL GLvoid* (*GL_APIENTRY glMapBufferRange)(GLenum target,
                                                          GLintptr offset,
                                                          GLsizeiptr length,
                                                          GLbitfield access);
extern GL_APICALL void (*GL_APIENTRY glFlushMappedBufferRange)(
    GLenum target, GLintptr offset, GLsizeiptr length);
extern GL_APICALL void (*GL_APIENTRY glBindVertexArray)(GLuint array);
extern GL_APICALL void (*GL_APIENTRY glDeleteVertexArrays)(
    GLsizei n, const GLuint* arrays);
extern GL_APICALL void (*GL_APIENTRY glGenVertexArrays)(GLsizei n,
                                                        GLuint* arrays);
extern GL_APICALL GLboolean (*GL_APIENTRY glIsVertexArray)(GLuint array);
extern GL_APICALL void (*GL_APIENTRY glGetIntegeri_v)(GLenum target,
                                                      GLuint index,
                                                      GLint* data);
extern GL_APICALL void (*GL_APIENTRY glBeginTransformFeedback)(
    GLenum primitiveMode);
extern GL_APICALL void (*GL_APIENTRY glEndTransformFeedback)(void);
extern GL_APICALL void (*GL_APIENTRY glBindBufferRange)(GLenum target,
                                                        GLuint index,
                                                        GLuint buffer,
                                                        GLintptr offset,
                                                        GLsizeiptr size);
extern GL_APICALL void (*GL_APIENTRY glBindBufferBase)(GLenum target,
                                                       GLuint index,
                                                       GLuint buffer);
extern GL_APICALL void (*GL_APIENTRY glTransformFeedbackVaryings)(
    GLuint program, GLsizei count, const GLchar* const* varyings,
    GLenum bufferMode);
extern GL_APICALL void (*GL_APIENTRY glGetTransformFeedbackVarying)(
    GLuint program, GLuint index, GLsizei bufSize, GLsizei* length,
    GLsizei* size, GLenum* type, GLchar* name);
extern GL_APICALL void (*GL_APIENTRY glVertexAttribIPointer)(
    GLuint index, GLint size, GLenum type, GLsizei stride,
    const GLvoid* pointer);
extern GL_APICALL void (*GL_APIENTRY glGetVertexAttribIiv)(GLuint index,
                                                           GLenum pname,
                                                           GLint* params);
extern GL_APICALL void (*GL_APIENTRY glGetVertexAttribIuiv)(GLuint index,
                                                            GLenum pname,
                                                            GLuint* params);
extern GL_APICALL void (*GL_APIENTRY glVertexAttribI4i)(GLuint index, GLint x,
                                                        GLint y, GLint z,
                                                        GLint w);
extern GL_APICALL void (*GL_APIENTRY glVertexAttribI4ui)(GLuint index, GLuint x,
                                                         GLuint y, GLuint z,
                                                         GLuint w);
extern GL_APICALL void (*GL_APIENTRY glVertexAttribI4iv)(GLuint index,
                                                         const GLint* v);
extern GL_APICALL void (*GL_APIENTRY glVertexAttribI4uiv)(GLuint index,
                                                          const GLuint* v);
extern GL_APICALL void (*GL_APIENTRY glGetUniformuiv)(GLuint program,
                                                      GLint location,
                                                      GLuint* params);
extern GL_APICALL GLint (*GL_APIENTRY glGetFragDataLocation)(
    GLuint program, const GLchar* name);
extern GL_APICALL void (*GL_APIENTRY glUniform1ui)(GLint location, GLuint v0);
extern GL_APICALL void (*GL_APIENTRY glUniform2ui)(GLint location, GLuint v0,
                                                   GLuint v1);
extern GL_APICALL void (*GL_APIENTRY glUniform3ui)(GLint location, GLuint v0,
                                                   GLuint v1, GLuint v2);
extern GL_APICALL void (*GL_APIENTRY glUniform4ui)(GLint location, GLuint v0,
                                                   GLuint v1, GLuint v2,
                                                   GLuint v3);
extern GL_APICALL void (*GL_APIENTRY glUniform1uiv)(GLint location,
                                                    GLsizei count,
                                                    const GLuint* value);
extern GL_APICALL void (*GL_APIENTRY glUniform2uiv)(GLint location,
                                                    GLsizei count,
                                                    const GLuint* value);
extern GL_APICALL void (*GL_APIENTRY glUniform3uiv)(GLint location,
                                                    GLsizei count,
                                                    const GLuint* value);
extern GL_APICALL void (*GL_APIENTRY glUniform4uiv)(GLint location,
                                                    GLsizei count,
                                                    const GLuint* value);
extern GL_APICALL void (*GL_APIENTRY glClearBufferiv)(GLenum buffer,
                                                      GLint drawbuffer,
                                                      const GLint* value);
extern GL_APICALL void (*GL_APIENTRY glClearBufferuiv)(GLenum buffer,
                                                       GLint drawbuffer,
                                                       const GLuint* value);
extern GL_APICALL void (*GL_APIENTRY glClearBufferfv)(GLenum buffer,
                                                      GLint drawbuffer,
                                                      const GLfloat* value);
extern GL_APICALL void (*GL_APIENTRY glClearBufferfi)(GLenum buffer,
                                                      GLint drawbuffer,
                                                      GLfloat depth,
                                                      GLint stencil);
extern GL_APICALL const GLubyte* (*GL_APIENTRY glGetStringi)(GLenum name,
                                                             GLuint index);
extern GL_APICALL void (*GL_APIENTRY glCopyBufferSubData)(GLenum readTarget,
                                                          GLenum writeTarget,
                                                          GLintptr readOffset,
                                                          GLintptr writeOffset,
                                                          GLsizeiptr size);
extern GL_APICALL void (*GL_APIENTRY glGetUniformIndices)(
    GLuint program, GLsizei uniformCount, const GLchar* const* uniformNames,
    GLuint* uniformIndices);
extern GL_APICALL void (*GL_APIENTRY glGetActiveUniformsiv)(
    GLuint program, GLsizei uniformCount, const GLuint* uniformIndices,
    GLenum pname, GLint* params);
extern GL_APICALL GLuint (*GL_APIENTRY glGetUniformBlockIndex)(
    GLuint program, const GLchar* uniformBlockName);
extern GL_APICALL void (*GL_APIENTRY glGetActiveUniformBlockiv)(
    GLuint program, GLuint uniformBlockIndex, GLenum pname, GLint* params);
extern GL_APICALL void (*GL_APIENTRY glGetActiveUniformBlockName)(
    GLuint program, GLuint uniformBlockIndex, GLsizei bufSize, GLsizei* length,
    GLchar* uniformBlockName);
extern GL_APICALL void (*GL_APIENTRY glUniformBlockBinding)(
    GLuint program, GLuint uniformBlockIndex, GLuint uniformBlockBinding);
extern GL_APICALL void (*GL_APIENTRY glDrawArraysInstanced)(
    GLenum mode, GLint first, GLsizei count, GLsizei instanceCount);
extern GL_APICALL void (*GL_APIENTRY glDrawElementsInstanced)(
    GLenum mode, GLsizei count, GLenum type, const GLvoid* indices,
    GLsizei instanceCount);
extern GL_APICALL GLsync (*GL_APIENTRY glFenceSync)(GLenum condition,
                                                    GLbitfield flags);
extern GL_APICALL GLboolean (*GL_APIENTRY glIsSync)(GLsync sync);
extern GL_APICALL void (*GL_APIENTRY glDeleteSync)(GLsync sync);
extern GL_APICALL GLenum (*GL_APIENTRY glClientWaitSync)(GLsync sync,
                                                         GLbitfield flags,
                                                         GLuint64 timeout);
extern GL_APICALL void (*GL_APIENTRY glWaitSync)(GLsync sync, GLbitfield flags,
                                                 GLuint64 timeout);
extern GL_APICALL void (*GL_APIENTRY glGetInteger64v)(GLenum pname,
                                                      GLint64* params);
extern GL_APICALL void (*GL_APIENTRY glGetSynciv)(GLsync sync, GLenum pname,
                                                  GLsizei bufSize,
                                                  GLsizei* length,
                                                  GLint* values);
extern GL_APICALL void (*GL_APIENTRY glGetInteger64i_v)(GLenum target,
                                                        GLuint index,
                                                        GLint64* data);
extern GL_APICALL void (*GL_APIENTRY glGetBufferParameteri64v)(GLenum target,
                                                               GLenum pname,
                                                               GLint64* params);
extern GL_APICALL void (*GL_APIENTRY glGenSamplers)(GLsizei count,
                                                    GLuint* samplers);
extern GL_APICALL void (*GL_APIENTRY glDeleteSamplers)(GLsizei count,
                                                       const GLuint* samplers);
extern GL_APICALL GLboolean (*GL_APIENTRY glIsSampler)(GLuint sampler);
extern GL_APICALL void (*GL_APIENTRY glBindSampler)(GLuint unit,
                                                    GLuint sampler);
extern GL_APICALL void (*GL_APIENTRY glSamplerParameteri)(GLuint sampler,
                                                          GLenum pname,
                                                          GLint param);
extern GL_APICALL void (*GL_APIENTRY glSamplerParameteriv)(GLuint sampler,
                                                           GLenum pname,
                                                           const GLint* param);
extern GL_APICALL void (*GL_APIENTRY glSamplerParameterf)(GLuint sampler,
                                                          GLenum pname,
                                                          GLfloat param);
extern GL_APICALL void (*GL_APIENTRY glSamplerParameterfv)(
    GLuint sampler, GLenum pname, const GLfloat* param);
extern GL_APICALL void (*GL_APIENTRY glGetSamplerParameteriv)(GLuint sampler,
                                                              GLenum pname,
                                                              GLint* params);
extern GL_APICALL void (*GL_APIENTRY glGetSamplerParameterfv)(GLuint sampler,
                                                              GLenum pname,
                                                              GLfloat* params);
extern GL_APICALL void (*GL_APIENTRY glVertexAttribDivisor)(GLuint index,
                                                            GLuint divisor);
extern GL_APICALL void (*GL_APIENTRY glBindTransformFeedback)(GLenum target,
                                                              GLuint id);
extern GL_APICALL void (*GL_APIENTRY glDeleteTransformFeedbacks)(
    GLsizei n, const GLuint* ids);
extern GL_APICALL void (*GL_APIENTRY glGenTransformFeedbacks)(GLsizei n,
                                                              GLuint* ids);
extern GL_APICALL GLboolean (*GL_APIENTRY glIsTransformFeedback)(GLuint id);
extern GL_APICALL void (*GL_APIENTRY glPauseTransformFeedback)(void);
extern GL_APICALL void (*GL_APIENTRY glResumeTransformFeedback)(void);
extern GL_APICALL void (*GL_APIENTRY glGetProgramBinary)(GLuint program,
                                                         GLsizei bufSize,
                                                         GLsizei* length,
                                                         GLenum* binaryFormat,
                                                         GLvoid* binary);
extern GL_APICALL void (*GL_APIENTRY glProgramBinary)(GLuint program,
                                                      GLenum binaryFormat,
                                                      const GLvoid* binary,
                                                      GLsizei length);
extern GL_APICALL void (*GL_APIENTRY glProgramParameteri)(GLuint program,
                                                          GLenum pname,
                                                          GLint value);
extern GL_APICALL void (*GL_APIENTRY glInvalidateFramebuffer)(
    GLenum target, GLsizei numAttachments, const GLenum* attachments);
extern GL_APICALL void (*GL_APIENTRY glInvalidateSubFramebuffer)(
    GLenum target, GLsizei numAttachments, const GLenum* attachments, GLint x,
    GLint y, GLsizei width, GLsizei height);
extern GL_APICALL void (*GL_APIENTRY glTexStorage2D)(GLenum target,
                                                     GLsizei levels,
                                                     GLenum internalformat,
                                                     GLsizei width,
                                                     GLsizei height);
extern GL_APICALL void (*GL_APIENTRY glTexStorage3D)(
    GLenum target, GLsizei levels, GLenum internalformat, GLsizei width,
    GLsizei height, GLsizei depth);
extern GL_APICALL void (*GL_APIENTRY glGetInternalformativ)(
    GLenum target, GLenum internalformat, GLenum pname, GLsizei bufSize,
    GLint* params);

#ifdef __cplusplus
}
#endif

#endif
