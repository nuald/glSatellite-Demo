uniform highp mat4 u_modelViewProjMatrix;

attribute highp vec4 vPosition;
attribute mediump vec4 vTexCoord;
attribute highp vec3 vNormal;

varying mediump vec2 textureCoordinate;
varying highp vec3 v_color;

void main() {
    gl_Position = u_modelViewProjMatrix * vPosition;
    textureCoordinate = vTexCoord.xy;
    v_color = vNormal;
}
