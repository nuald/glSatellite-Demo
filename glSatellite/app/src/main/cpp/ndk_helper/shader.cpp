/*
 * Copyright 2013 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#include <cstdlib>
#include <EGL/egl.h>
#include <GLES2/gl2.h>

#include "shader.h"
#include "JNIHelper.h"

namespace ndk_helper {

#define DEBUG

bool shader::CompileShader(GLuint *shader, const GLenum type,
                           const GLchar *source, const int32_t iSize) {
  if (source == NULL || iSize <= 0) {
      return false;
  }

  *shader = glCreateShader(type);
  glShaderSource(*shader, 1, &source, &iSize);  // Not specifying 3rd parameter
                                                // (size) could be troublesome..

  glCompileShader(*shader);

  GLint isCompiled;
  glGetShaderiv(*shader, GL_COMPILE_STATUS, &isCompiled);
  if (isCompiled == GL_FALSE) {
    LOGI("Shader compile failed\n");
#ifdef DEBUG
    GLint maxLength = 0;
    glGetShaderiv(*shader, GL_INFO_LOG_LENGTH, &maxLength);

    //The maxLength includes the NULL character
    std::vector<GLchar> infoLog(maxLength);
    glGetShaderInfoLog(*shader, maxLength, &maxLength, &infoLog[0]);
    std::string log(infoLog.begin(), infoLog.end());
    LOGI("Shader compile log:\n%s", log.c_str());
#endif
    glDeleteShader(*shader);
    return false;
  }

  return true;
}

bool shader::CompileShader(GLuint *shader, const GLenum type,
                           std::vector<uint8_t> &data) {
  if (!data.size()) return false;

  const GLchar *source = (GLchar *)&data[0];
  int32_t iSize = data.size();
  return shader::CompileShader(shader, type, source, iSize);
}

bool shader::CompileShader(GLuint *shader, const GLenum type,
                           const char *strFileName) {
  std::vector<uint8_t> data;
  bool b = JNIHelper::GetInstance()->ReadFile(strFileName, &data);
  if (!b) {
    LOGI("Can not open a file: %s", strFileName);
    return false;
  }

  return shader::CompileShader(shader, type, data);
}

void printLog(const GLuint prog) {
  GLint maxLength = 0;
  glGetProgramiv(prog, GL_INFO_LOG_LENGTH, &maxLength);
  std::vector<GLchar> infoLog(maxLength);
  glGetProgramInfoLog(prog, maxLength, &maxLength, &infoLog[0]);
  std::string log(infoLog.begin(), infoLog.end());
  LOGI("Program link log:\n%s", log.c_str());
}

bool shader::LinkProgram(const GLuint prog) {

  glLinkProgram(prog);

  GLint isLinked = 0;
  glGetProgramiv(prog, GL_LINK_STATUS, &isLinked);
  if (isLinked == GL_FALSE)
  {
    LOGI("Program link failed\n");
#ifdef DEBUG
    printLog(prog);
#endif
    glDeleteProgram(prog);
    return false;
  }

  return true;
}

}  // namespace ndkHelper
