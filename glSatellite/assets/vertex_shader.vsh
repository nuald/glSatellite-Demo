uniform highp mat4 u_modelViewProjMatrix;
uniform highp mat4 u_normalMatrix;

attribute highp vec3 vNormal;
attribute mediump vec4 vTexCoord;
attribute highp vec4 vPosition;

varying highp float v_Dot;
varying mediump vec2 v_texCoord;

void main() {
    gl_Position = u_modelViewProjMatrix * vPosition;
    v_texCoord = vTexCoord.st;
    mediump vec4 transNormal = u_normalMatrix * vec4(vNormal, 1.0);
    highp vec3 light = vec3(0.3, 0.3, 0.9);
    v_Dot = max(dot(transNormal.xyz, light), 0.0);
}
