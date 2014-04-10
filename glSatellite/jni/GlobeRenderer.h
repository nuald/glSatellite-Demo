#pragma once

#include "NDKHelper.h"
#include "SatelliteMgr.h"
#include "IFileReader.h"

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
    GLOBE, BACKGROUND, MAX_SHADERS
};

struct SHADER_PARAMS {
    GLuint program_;
    GLuint tex_;

    GLuint matrix_projection_;
    GLuint matrix_normal_;
};

class GlobeRenderer {
    int32_t num_indices_, num_points_, num_beams_;
    GLuint buffer_[MAX_BUFFERS];
    GLuint texture_;
    GLuint star_texture_;
    std::unique_ptr<int[]> planes_per_beam_;
    SatelliteMgr mgr_;
    bool zoom_in_enabled_, zoom_out_enabled_;

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

    void RenderGlobe();
    void RenderBackground();
    void RenderBeams();

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
    void Update(float dTime);
    void Unload();
    void UpdateViewport();
    bool IsZoomInEnabled() { return zoom_in_enabled_; }
    bool IsZoomOutEnabled() { return zoom_out_enabled_; }
};

