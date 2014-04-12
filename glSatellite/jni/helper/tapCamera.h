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

#pragma once
#include <vector>
#include <string>
#include <GLES2/gl2.h>

#include "JNIHelper.h"
#include "vecmath.h"
#include "interpolator.h"

namespace helper
{

/******************************************************************
 * Camera control helper class with a tap gesture
 * This class is mainly used for 3D space camera control in samples.
 *
 */
class TapCamera
{
private:
    //Trackball
    ndk_helper::Vec2 vec_ball_center_;
    float ball_radius_;
    ndk_helper::Quaternion quat_ball_now_;
    ndk_helper::Quaternion quat_ball_down_;
    ndk_helper::Vec2 vec_ball_now_;
    ndk_helper::Vec2 vec_ball_down_;
    ndk_helper::Quaternion quat_ball_rot_;

    bool dragging_;
    bool pinching_;

    //Pinch related info
    ndk_helper::Vec2 vec_pinch_start_;
    ndk_helper::Vec2 vec_pinch_start_center_;
    float pinch_start_distance_SQ_;

    //Camera shift
    ndk_helper::Vec3 vec_offset_;
    ndk_helper::Vec3 vec_offset_now_;

    //Camera Rotation
    float camera_rotation_;
    float camera_rotation_start_;
    float camera_rotation_now_;

    //Momentum support
    bool momentum_;
    ndk_helper::Vec2 vec_drag_delta_;
    ndk_helper::Vec2 vec_last_input_;
    ndk_helper::Vec3 vec_offset_last_;
    ndk_helper::Vec3 vec_offset_delta_;
    float momentum_steps_;

    ndk_helper::Vec2 vec_flip_;
    float flip_z_;

    ndk_helper::Mat4 mat_rotation_;
    ndk_helper::Mat4 mat_transform_;

    ndk_helper::Vec3 vec_pinch_transform_factor_;

    bool stopping_;

    ndk_helper::Vec3 PointOnSphere(ndk_helper::Vec2& point);
    void BallUpdate();
    void InitParameters();
public:
    TapCamera();
    virtual ~TapCamera();
    void BeginDrag( const ndk_helper::Vec2& vec );
    void EndDrag();
    void Drag( const ndk_helper::Vec2& vec );
    void Update();
    void BeginStop();
    void EndStop();

    ndk_helper::Mat4& GetRotationMatrix();
    ndk_helper::Mat4& GetTransformMatrix();

    void BeginPinch( const ndk_helper::Vec2& v1, const ndk_helper::Vec2& v2 );
    void EndPinch();
    void Pinch( const ndk_helper::Vec2& v1, const ndk_helper::Vec2& v2 );

    void SetFlip( const float x, const float y, const float z )
    {
        vec_flip_ = ndk_helper::Vec2( x, y );
        flip_z_ = z;
    }

    void SetPinchTransformFactor( const float x, const float y, const float z )
    {
        vec_pinch_transform_factor_ = ndk_helper::Vec3( x, y, z );
    }

    void Reset( const bool bAnimate );

};

} //namespace helper
