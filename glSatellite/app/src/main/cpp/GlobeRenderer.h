#pragma once

#include "ndk_helper/NDKHelper.h"
#include "SatelliteMgr.h"
#include "IFileReader.h"
#include "ndk_helper/tapCamera.h"

enum SHADER_ATTRIBUTES {
    ATTRIB_VERTEX, ATTRIB_NORMAL, ATTRIB_UV,
};

enum BUFFERS {
    GEOMETRY,
    NORMALS,
    TEXCOORDS,
    INDICES,
    POINTS,
    PTS_TEX,
    BEAMS,
    BEAMS_TEX,
    BEAMS_COLOR,
    MAX_BUFFERS
};

enum SHADERS {
    GLOBE, BACKGROUND, FBO, MAX_SHADERS
};

struct SHADER_PARAMS {
    GLuint program_;
    GLuint tex_;

    GLuint matrix_projection_;
    GLuint matrix_normal_;
};

class GlobeRenderer {
    size_t num_indices_, num_points_, num_beams_;
    GLuint buffer_[MAX_BUFFERS];
    GLuint texture_;
    GLuint star_texture_;
    std::unique_ptr<size_t[]> planes_per_beam_;
    SatelliteMgr mgr_;
    bool zoom_in_enabled_, zoom_out_enabled_, read_requested_;
    ndk_helper::Vec2 read_coord_;
    std::unique_ptr<float[]> color_data_;
    GLuint fb_;

    SHADER_PARAMS shader_params_[MAX_SHADERS];
    void LoadShaders(SHADER_PARAMS* params, const char* strVsh,
        const char* strFsh);

    ndk_helper::Mat4 mat_projection_;
    ndk_helper::Mat4 mat_view_;
    ndk_helper::Mat4 mat_model_;
    ndk_helper::TapCamera* camera_;

    ndk_helper::Vec3 Coord2Vec3(float latitude, float longitude);

    void MakeSphere(int lats, int longs);
    void MakePoints(float radius, int number);
    void MakeBeams();
    void InitFBO();
    void BindAndClear(bool fbo);

    void RenderGlobe();
    void RenderBackground();
    void RenderBeams(bool fbo);

public:
    GlobeRenderer();

    virtual ~GlobeRenderer() {
        Unload();
    }

    void Bind(ndk_helper::TapCamera* camera) {
        camera_ = camera;
    }

    void InitSatelliteMgr(IFileReader& reader);
    void Init();
    void Render();
    void Update(double dTime);
    void Unload();
    void UpdateViewport();
    void RequestRead(const ndk_helper::Vec2& v);
    bool IsZoomInEnabled() {
        return zoom_in_enabled_;
    }
    bool IsZoomOutEnabled() {
        return zoom_out_enabled_;
    }
    Satellite &GetSatellite(size_t num) {
        return mgr_.GetSatellite(num);
    }
};

