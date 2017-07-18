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

bool shader::CompileShader(
    GLuint *shader, const GLenum type, const char *str_file_name,
    const std::map<std::string, std::string> &map_parameters) {
  std::vector<uint8_t> data;
  if (!JNIHelper::GetInstance()->ReadFile(str_file_name, &data)) {
    LOGI("Can not open a file:%s", str_file_name);
    return false;
  }

  const char REPLACEMENT_TAG = '*';
  // Fill-in parameters
  std::string str(data.begin(), data.end());
  std::string str_replacement_map(data.size(), ' ');

  std::map<std::string, std::string>::const_iterator it =
      map_parameters.begin();
  std::map<std::string, std::string>::const_iterator itEnd =
      map_parameters.end();
  while (it != itEnd) {
    size_t pos = 0;
    while ((pos = str.find(it->first, pos)) != std::string::npos) {
      // Check if the sub string is already touched

      size_t replaced_pos = str_replacement_map.find(REPLACEMENT_TAG, pos);
      if (replaced_pos == std::string::npos || replaced_pos > pos) {
        str.replace(pos, it->first.length(), it->second);
        str_replacement_map.replace(pos, it->first.length(), it->first.length(),
                                    REPLACEMENT_TAG);
        pos += it->second.length();
      } else {
        // The replacement target has been touched by other tag, skipping them
        pos += it->second.length();
      }
    }
    it++;
  }

  LOGI("Patched Shdader:\n%s", str.c_str());

  std::vector<uint8_t> v(str.begin(), str.end());
  str.clear();
  return shader::CompileShader(shader, type, v);
}

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

bool shader::ValidateProgram(const GLuint prog) {
  GLint status;

  glValidateProgram(prog);
  printLog(prog);

  glGetProgramiv(prog, GL_VALIDATE_STATUS, &status);
  return status != 0;

}

}  // namespace ndkHelper
