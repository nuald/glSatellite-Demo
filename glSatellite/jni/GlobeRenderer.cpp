#include <cstdlib>
#include "GlobeRenderer.h"
#include "DebugUtils.h"
#include "MessageQueue.h"

using namespace ndk_helper;
using namespace std;

const int PTS_PER_STAR = 4;
const int PTS_PER_BEAM = 4;
const float CAM_NEAR = 5.f;
const float CAM_FAR = 10000.f;
const float CAM_X = 0.f;
const float CAM_Y = 0.f;
const float CAM_Z = 700.f;
const float STAR_BLINK_FREQ = 0.02f;
const float GLOBE_RADIUS = 35;
const float MAX_STAR_D = 3.f;
const float BEAM_WIDTH = 1.0f;
const float BEAM_MAX_PLANES = 300;
const float BEAM_PLANE_DIFF = 0.1f;
const float INITIAL_LONGITUDE = 90;
const float INITIAL_LATITUDE = 90;
const float CAM_STOP_MIN = -1000;
const float CAM_STOP_MAX = 500;
// Debug mode for color picker
const bool DEBUG_FBO = false;

GlobeRenderer::GlobeRenderer() :
            zoom_in_enabled_(true),
            zoom_out_enabled_(true),
            read_requested_(false) {
    for (size_t i = 0; i < MAX_BUFFERS; ++i) {
        buffer_[i] = 0;
    }

    for (size_t i = 0; i < MAX_SHADERS; ++i) {
        SHADER_PARAMS *params = &shader_params_[i];
        params->program_ = 0;
    }
}

Vec3 GlobeRenderer::Coord2Vec3(float latitude, float longitude) {
    auto theta = latitude * M_PI / 180;
    auto phi = longitude * M_PI / 180;
    auto sinTheta = sin(theta);
    auto sinPhi = sin(phi);
    auto cosTheta = cos(theta);
    auto cosPhi = cos(phi);

    auto x = cosPhi * sinTheta;
    auto y = cosTheta;
    auto z = sinPhi * sinTheta;
    return Vec3(x, y, z);
}

// Create a sphere with the passed number of latitude and longitude bands and
// the passed radius.
// Sphere has vertices, normals and texCoords.
// Create VBOs for each as well as the index array.
// Return an object with the following properties:
void GlobeRenderer::MakeSphere(int lats, int longs) {
    auto num_vertices = (lats + 1) * (longs + 1);
    auto index = 0, ti = 0;
    num_indices_ = 6 * lats * longs;
    auto num_geometry = 3 * num_vertices;
    auto num_normals = num_geometry;
    auto num_texcoords = 2 * num_vertices;

    unique_ptr<float[]> geometry_data(new float[num_geometry]);
    unique_ptr<float[]> normal_data(new float[num_normals]);
    unique_ptr<float[]> texcoord_data(new float[num_texcoords]);
    unique_ptr<uint16_t[]> index_data(new uint16_t[num_indices_]);

    for (auto latNumber = 0.f; latNumber <= lats; ++latNumber) {
        for (auto longNumber = 0.f; longNumber <= longs; ++longNumber) {
            float x, y, z;
            auto u = longNumber / longs;
            auto v = latNumber / lats;
            Vec3 coord = Coord2Vec3(180 * v, 360 * u);
            coord.Value(x, y, z);

            normal_data[index] = x;
            normal_data[index + 1] = y;
            normal_data[index + 2] = z;
            geometry_data[index] = GLOBE_RADIUS * x;
            geometry_data[index + 1] = GLOBE_RADIUS * y;
            geometry_data[index + 2] = GLOBE_RADIUS * z;
            texcoord_data[ti++] = 1 - u;
            texcoord_data[ti++] = v;

            index += 3;
        }
    }

    index = 0;
    for (auto latNumber = 0; latNumber < lats; ++latNumber) {
        for (auto longNumber = 0; longNumber < longs; ++longNumber) {
            auto first = (latNumber * (longs + 1)) + longNumber;
            auto second = first + longs + 1;
            index_data[index++] = first;
            index_data[index++] = second;
            index_data[index++] = first + 1;

            index_data[index++] = second;
            index_data[index++] = second + 1;
            index_data[index++] = first + 1;
        }
    }

    glBindBuffer(GL_ARRAY_BUFFER, buffer_[NORMALS]);
    glBufferData(GL_ARRAY_BUFFER, num_normals * sizeof(float),
        normal_data.get(), GL_STATIC_DRAW);

    glBindBuffer(GL_ARRAY_BUFFER, buffer_[GEOMETRY]);
    glBufferData(GL_ARRAY_BUFFER, num_geometry * sizeof(float),
        geometry_data.get(), GL_STATIC_DRAW);

    glBindBuffer(GL_ARRAY_BUFFER, buffer_[TEXCOORDS]);
    glBufferData(GL_ARRAY_BUFFER, num_texcoords * sizeof(float),
        texcoord_data.get(), GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, buffer_[INDICES]);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(uint16_t) * num_indices_,
        index_data.get(), GL_STATIC_DRAW);
}

void GlobeRenderer::MakePoints(float radius, int number) {
    num_points_ = number * PTS_PER_STAR;
    auto num_geometry = 3 * num_points_;
    auto num_uv = 2 * num_points_;
    unique_ptr<float[]> geometry_data(new float[num_geometry]);
    unique_ptr<float[]> tex_data(new float[num_uv]);
    auto index = 0, ti = 0;
    const int STEP_NUM = 4;
    int geo_manip_x[STEP_NUM] = {1, 1, -1, -1};
    int geo_manip_y[STEP_NUM] = {1, -1, 1, -1};
    int tex_manip_u[STEP_NUM] = {1, 1, 0, 0};
    int tex_manip_v[STEP_NUM] = {1, 0, 1, 0};

    for (int i = 0; i < number; ++i) {
        auto x = radius * (1.f - 2.f * random() / RAND_MAX);
        auto y = radius * (1.f - 2.f * random() / RAND_MAX);
        auto diff = MAX_STAR_D * random() / RAND_MAX;

        for (auto step = 0; step < STEP_NUM; ++step) {
            geometry_data[index++] = x + diff * geo_manip_x[step];
            geometry_data[index++] = y + diff * geo_manip_y[step];
            geometry_data[index++] = CAM_STOP_MIN;
            tex_data[ti++] = tex_manip_u[step];
            tex_data[ti++] = tex_manip_v[step];
        }
    }

    glBindBuffer(GL_ARRAY_BUFFER, buffer_[POINTS]);
    glBufferData(GL_ARRAY_BUFFER, num_geometry * sizeof(float),
        geometry_data.get(), GL_STATIC_DRAW);

    glBindBuffer(GL_ARRAY_BUFFER, buffer_[PTS_TEX]);
    glBufferData(GL_ARRAY_BUFFER, num_uv * sizeof(float), tex_data.get(),
        GL_STATIC_DRAW);
}

void GlobeRenderer::MakeBeams() {
    num_beams_ = mgr_.GetNumber();
    if (num_beams_ == 0) {
        return;
    }

    // Calculate geometry first
    int overall_planes = 0;
    planes_per_beam_.reset(new int[num_beams_]);

    // Update all positions
    mgr_.UpdateAll();
    double min_alt = mgr_.GetMinAltitude();
    double max_alt = mgr_.GetMaxAltitude();
    double alt_diff = max_alt - min_alt;
    if (alt_diff < 0.001) {
        alt_diff = 0.001;
    }

    for (size_t i = 0; i < num_beams_; ++i) {
        Satellite &sat = mgr_.GetSatellite(i);
        double alt = sat.GetAltitude();

        int planes = 1 + BEAM_MAX_PLANES * (alt - min_alt) / alt_diff;
        planes_per_beam_[i] = planes;
        overall_planes += planes;
    }

    auto num_geometry = 3 * overall_planes * PTS_PER_BEAM;
    auto num_colors = 3 * overall_planes * PTS_PER_BEAM;
    auto num_uv = 2 * overall_planes * PTS_PER_BEAM;
    unique_ptr<float[]> geometry_data(new float[num_geometry]);
    unique_ptr<float[]> tex_data(new float[num_uv]);
    auto index = 0, ti = 0;
    const int STEP_NUM = 4;
    int geo_manip_x[STEP_NUM] = {1, 1, -1, -1};
    int geo_manip_y[STEP_NUM] = {1, -1, 1, -1};
    int tex_manip_u[STEP_NUM] = {1, 1, 0, 0};
    int tex_manip_v[STEP_NUM] = {1, 0, 1, 0};

    for (size_t i = 0; i < num_beams_; ++i) {
        float latitude = INITIAL_LATITUDE;
        float longitude = INITIAL_LONGITUDE;

        auto width = BEAM_WIDTH;
        float x, y, z;

        for (int j = 0; j < planes_per_beam_[i]; ++j) {
            for (int step = 0; step < STEP_NUM; ++step) {
                int _step = step;
                Vec3 coord = Coord2Vec3(latitude + width * geo_manip_y[_step],
                    longitude + width * geo_manip_x[_step]);
                coord *= (GLOBE_RADIUS + 0.5 + BEAM_PLANE_DIFF * j);
                coord.Value(x, y, z);

                geometry_data[index++] = x;
                geometry_data[index++] = y;
                geometry_data[index++] = z;

                tex_data[ti++] = tex_manip_u[_step];
                tex_data[ti++] = tex_manip_v[_step];
            }
        }
    }

    glBindBuffer(GL_ARRAY_BUFFER, buffer_[BEAMS]);
    glBufferData(GL_ARRAY_BUFFER, num_geometry * sizeof(float),
        geometry_data.get(), GL_STATIC_DRAW);

    glBindBuffer(GL_ARRAY_BUFFER, buffer_[BEAMS_TEX]);
    glBufferData(GL_ARRAY_BUFFER, num_uv * sizeof(float), tex_data.get(),
        GL_STATIC_DRAW);

    color_data_.reset(new float[num_colors]);
    index = 0;

    // WARNING: android NDK log2 implementation is wrong
    // (probably for C++0x only)
    unsigned tuple_size = ceil(log(num_beams_ + 1) / log(2) / 3);
    for (size_t i = 0; i < num_beams_; ++i) {
        int plane_num = planes_per_beam_[i] * PTS_PER_BEAM;
        unsigned first_tuple = (1 << tuple_size) - 1;
        unsigned color_r = (i + 1) & first_tuple;
        unsigned second_tuple = (1 << (tuple_size * 2)) - 1 - first_tuple;
        unsigned color_g = ((i + 1) & second_tuple) >> tuple_size;
        unsigned third_tuple = (1 << (tuple_size * 3)) - 1 - first_tuple
                - second_tuple;
        unsigned color_b = ((i + 1) & third_tuple) >> (2 * tuple_size);
        for (int j = 0; j < plane_num; ++j) {
            color_data_[index + j * 3] = 1.f * color_r / first_tuple;
            color_data_[index + 1 + j * 3] = 1.f * color_g / first_tuple;
            color_data_[index + 2 + j * 3] = 1.f * color_b / first_tuple;
        }
        index += plane_num * 3;
    }

    glBindBuffer(GL_ARRAY_BUFFER, buffer_[BEAMS_COLOR]);
    glBufferData(GL_ARRAY_BUFFER, num_colors * sizeof(float), color_data_.get(),
        GL_STATIC_DRAW);
}

void GlobeRenderer::InitFBO() {
    // create framebuffer
    glGenFramebuffers(1, &fb_);
    glBindFramebuffer(GL_FRAMEBUFFER, fb_);

    int32_t viewport[4];
    glGetIntegerv(GL_VIEWPORT, viewport);
    const int width = viewport[2], height = viewport[3];

    // attach renderbuffer to fb so that depth-sorting works
    GLuint rb = 0;
    GLuint fb_tex = 0;
    glGenRenderbuffers(1, &rb);
    glBindRenderbuffer(GL_RENDERBUFFER, rb);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT16, width, height);

    // create texture to use for rendering second pass
    glGenTextures(1, &fb_tex);
    glBindTexture(GL_TEXTURE_2D, fb_tex);
    // make the texture the same size as the viewport
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA,
        GL_UNSIGNED_BYTE, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    // // attach render buffer (depth) and texture (colour) to fb
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,
        GL_RENDERBUFFER, rb);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D,
        fb_tex, 0);

    GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if (status == GL_FRAMEBUFFER_COMPLETE) {
        LOGI("Single FBO setup successfully.");
    } else {
        LOGI("Problem in setup FBO texture: %x.", status);
    }

    // bind default fb (number 0) so that we render normally next time
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void GlobeRenderer::Init() {
    //Settings
    glFrontFace (GL_CW);

    LoadShaders(&shader_params_[GLOBE], "vertex_shader.vsh",
        "fragment_shader.fsh");
    LoadShaders(&shader_params_[BACKGROUND], "bg_vshader.vsh",
        "bg_fshader.fsh");
    LoadShaders(&shader_params_[FBO], "bg_vshader.vsh", "fbo_fshader.fsh");

    texture_ = JNIHelper::GetInstance()->LoadTexture("earth.png");
    star_texture_ = JNIHelper::GetInstance()->LoadTexture("star.png");

    glGenBuffers(MAX_BUFFERS, buffer_);
    MakeSphere(30, 30);
    MakePoints(CAM_Z, 500);
    InitFBO();

    UpdateViewport();

    mat_model_ = Mat4::Translation(0, 0, 1);
}

void GlobeRenderer::UpdateViewport() {
    int32_t viewport[4];
    glGetIntegerv(GL_VIEWPORT, viewport);
    auto ratio = (float)viewport[2] / (float)viewport[3];

    mat_projection_ = Mat4::Perspective(ratio, 1.0f, CAM_NEAR, CAM_FAR);
    Mat4 mat_look = Mat4::LookAt(Vec3(CAM_X, CAM_Y, CAM_Z), Vec3(0.f, 0.f, 0.f),
        Vec3(0.f, 1.f, 0.f));

    mat_projection_ = mat_projection_ * mat_look;
}

void GlobeRenderer::Unload() {
    glDeleteBuffers(MAX_BUFFERS, buffer_);

    for (size_t i = 0; i < MAX_SHADERS; ++i) {
        SHADER_PARAMS *params = &shader_params_[i];
        if (params->program_) {
            glDeleteProgram(params->program_);
            params->program_ = 0;
        }
    }

}

void GlobeRenderer::Update(float fTime) {
    camera_->Update();
    Mat4 mat_tranform = camera_->GetTransformMatrix();
    float cam_z = mat_tranform.Ptr()[14];
    zoom_in_enabled_ = cam_z < CAM_STOP_MAX;
    zoom_out_enabled_ = cam_z > CAM_STOP_MIN;
    if (!zoom_in_enabled_ || !zoom_out_enabled_) {
        camera_->BeginStop();
    } else {
        camera_->EndStop();
    }

    mat_view_ = mat_tranform * camera_->GetRotationMatrix() * mat_model_;
}

void GlobeRenderer::RenderGlobe() {
    // Feed Projection and Model View matrices to the shaders
    auto mat_vp = mat_projection_ * mat_view_;

    // Bind the VBO
    glBindBuffer(GL_ARRAY_BUFFER, buffer_[GEOMETRY]);
    // Pass the vertex data
    glVertexAttribPointer(ATTRIB_VERTEX, 3, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(ATTRIB_VERTEX);

    // Bind the VBO
    glBindBuffer(GL_ARRAY_BUFFER, buffer_[NORMALS]);
    // Pass the vertex data
    glVertexAttribPointer(ATTRIB_NORMAL, 3, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(ATTRIB_NORMAL);

    // Bind the VBO
    glBindBuffer(GL_ARRAY_BUFFER, buffer_[TEXCOORDS]);
    // Pass the vertex data
    glVertexAttribPointer(ATTRIB_UV, 2, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(ATTRIB_UV);

    // Bind the IB
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, buffer_[INDICES]);

    SHADER_PARAMS shader_param_ = shader_params_[GLOBE];
    glUseProgram(shader_param_.program_);

    glUniformMatrix4fv(shader_param_.matrix_projection_, 1, GL_FALSE,
        mat_vp.Ptr());

    auto mat_normal = mat_view_;
    mat_normal.Inverse();
    mat_normal.Transpose();
    glUniformMatrix4fv(shader_param_.matrix_normal_, 1, GL_FALSE,
        mat_normal.Ptr());

    glActiveTexture (GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texture_);
    glUniform1i(shader_param_.tex_, 0);

    glDrawElements(GL_TRIANGLES, num_indices_, GL_UNSIGNED_SHORT, 0);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}

void GlobeRenderer::RenderBackground() {
    SHADER_PARAMS bg_shader_param_ = shader_params_[BACKGROUND];
    glUseProgram(bg_shader_param_.program_);

    ndk_helper::Mat4 mat_fixed = mat_projection_ * mat_model_;
    glUniformMatrix4fv(bg_shader_param_.matrix_projection_, 1, GL_FALSE,
        mat_fixed.Ptr());

    glActiveTexture (GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, star_texture_);
    glUniform1i(bg_shader_param_.tex_, 0);

    // Bind the VBO
    glBindBuffer(GL_ARRAY_BUFFER, buffer_[POINTS]);
    // Pass the vertex data
    glVertexAttribPointer(ATTRIB_VERTEX, 3, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(ATTRIB_VERTEX);

    // Bind the VBO
    glBindBuffer(GL_ARRAY_BUFFER, buffer_[PTS_TEX]);
    // Pass the vertex data
    glVertexAttribPointer(ATTRIB_UV, 2, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(ATTRIB_UV);

    unique_ptr<float[]> color_data(new float[3 * num_points_]);
    for (int i = 0; i < num_points_ / PTS_PER_STAR; ++i) {
        float color = 1.f;
        if (1.0f * random() / RAND_MAX < STAR_BLINK_FREQ) {
            color = 1.f * random() / RAND_MAX;
        }
        for (int j = 0; j < PTS_PER_STAR * 3; ++j) {
            color_data[j + i * PTS_PER_STAR * 3] = color;
        }
    }

    // To reduce code duplication, we just put color data to the vNormal
    GLuint vbo;
    glGenBuffers(1, &vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, num_points_ * 3 * sizeof(float),
        color_data.get(), GL_STATIC_DRAW);
    glVertexAttribPointer(ATTRIB_NORMAL, 3, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(ATTRIB_NORMAL);

    for (int i = 0; i < num_points_; i += PTS_PER_STAR) {
        glDrawArrays(GL_TRIANGLE_STRIP, i, PTS_PER_STAR);
    }

    glDeleteBuffers(1, &vbo);
}

void GlobeRenderer::RenderBeams(bool fbo = false) {
    SHADER_PARAMS bg_shader_param_ = shader_params_[fbo ? FBO : BACKGROUND];
    glUseProgram(bg_shader_param_.program_);

    glActiveTexture (GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, star_texture_);
    glUniform1i(bg_shader_param_.tex_, 0);

    // Bind the VBO
    glBindBuffer(GL_ARRAY_BUFFER, buffer_[BEAMS]);
    // Pass the vertex data
    glVertexAttribPointer(ATTRIB_VERTEX, 3, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(ATTRIB_VERTEX);

    // Bind the VBO
    glBindBuffer(GL_ARRAY_BUFFER, buffer_[BEAMS_TEX]);
    // Pass the vertex data
    glVertexAttribPointer(ATTRIB_UV, 2, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(ATTRIB_UV);

    // To reduce code duplication, we just put color data to the vNormal
    glBindBuffer(GL_ARRAY_BUFFER, buffer_[BEAMS_COLOR]);
    glVertexAttribPointer(ATTRIB_NORMAL, 3, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(ATTRIB_NORMAL);

    int index = 0;
    Vec3 vec_from = Coord2Vec3(INITIAL_LATITUDE, INITIAL_LONGITUDE).Normalize();
    // Update all positions
    mgr_.UpdateAll();
    for (int i = 0; i < num_beams_; ++i) {
        Satellite &sat = mgr_.GetSatellite(i);
        auto latitude = 90 - sat.GetLatitude();
        auto longitude = sat.GetLongitude() - 90;
        Vec3 vec_to = Coord2Vec3(latitude, longitude).Normalize();
        Mat4 rotate_matrix;

        Vec3 vec_sum = (vec_from + vec_to).Normalize();
        Vec3 vec = vec_sum.Cross(vec_to);
        float w = vec_sum.Dot(vec_to);
        Quaternion quat = Quaternion(vec, w);
        quat.ToMatrix(rotate_matrix);

        auto mat_vp = mat_projection_ * mat_view_ * rotate_matrix;
        glUniformMatrix4fv(bg_shader_param_.matrix_projection_, 1, GL_FALSE,
            mat_vp.Ptr());

        int plane_num = planes_per_beam_[i] * PTS_PER_BEAM;
        glDrawArrays(GL_TRIANGLE_STRIP, index, plane_num);
        index += plane_num;
    }
}

void GlobeRenderer::BindAndClear(bool fbo = false) {
    glBindFramebuffer(GL_FRAMEBUFFER, fbo ? fb_ : 0);
    glClearColor(0, 0, 0, 1.f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void GlobeRenderer::Render() {
    // Render FBO
    BindAndClear(true);
    RenderBeams(true);

    // Render scene
    BindAndClear();
    if (DEBUG_FBO) {
        RenderBeams(true);
    } else {
        RenderBackground();
        RenderGlobe();

        glEnable (GL_BLEND);
        glBlendFunc(GL_ONE, GL_ONE);
        RenderBeams();
        glDisable(GL_BLEND);
    }

    if (read_requested_) {
        glBindFramebuffer(GL_FRAMEBUFFER, fb_);
        float x, y;
        read_coord_.Value(x, y);
        unsigned char data[4] = {};
        glReadPixels(x, y, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, &data);
        int index = 0;
        for (int i = 0; i < num_beams_; ++i) {
            int plane_num = planes_per_beam_[i] * PTS_PER_BEAM;
            bool found = true;
            for (int j = 0; j < 3; ++j) {
                found = found
                        && fabs(255 * color_data_[index + j] - data[j]) <= 1;
            }
            if (found) {
                Message msg = {SHOW_BEAM, reinterpret_cast<void*>(i)};
                PostMessage(msg);
                break;
            }
            index += plane_num * 3;
        }
        read_requested_ = false;
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }
}

void GlobeRenderer::LoadShaders(SHADER_PARAMS *params, const char *strVsh,
    const char *strFsh) {
    // Create shader program
    auto program = glCreateProgram();
    LOGI("Created Shader %d", program);

    class ShaderHelper {
        GLuint shader;
    public:
        ShaderHelper(const char *path, GLuint program, GLenum shader_type) {
            if (!shader::CompileShader(&shader, shader_type, path)) {
                glDeleteProgram(program);
                throw RuntimeError(AT, "Failed to compile shader from %s",
                    path);
            }
            glAttachShader(program, shader);
        }
        ;
        ~ShaderHelper() {
            glDeleteShader(shader);
        }
        operator GLuint() {
            return shader;
        }
    };

    ShaderHelper vert_shader(strVsh, program, GL_VERTEX_SHADER);
    ShaderHelper frag_shader(strFsh, program, GL_FRAGMENT_SHADER);

    // Bind attribute locations
    // this needs to be done prior to linking
    glBindAttribLocation(program, ATTRIB_VERTEX, "vPosition");
    glBindAttribLocation(program, ATTRIB_NORMAL, "vNormal");
    glBindAttribLocation(program, ATTRIB_UV, "vTexCoord");

    // Link program
    if (!shader::LinkProgram(program)) {
        if (program) {
            glDeleteProgram(program);
        }
        throw RuntimeError(AT, "Failed to link program: %d", program);
    }

    // Get uniform locations
    params->matrix_projection_ = glGetUniformLocation(program,
        "u_modelViewProjMatrix");
    params->matrix_normal_ = glGetUniformLocation(program, "u_normalMatrix");
    params->tex_ = glGetUniformLocation(program, "tex0");

    params->program_ = program;
}

void GlobeRenderer::InitSatelliteMgr(IFileReader& reader) {
    mgr_.Init(reader);
    MakeBeams();
}

void GlobeRenderer::RequestRead(const Vec2& v) {
    read_coord_ = v;
    read_requested_ = true;
}
