uniform sampler2D tex0;

varying highp float v_Dot;
varying mediump vec2 v_texCoord;

void main() {
    lowp vec4 color = texture2D(tex0, v_texCoord);
    color += vec4(0.1, 0.1, 0.1, 1);
    lowp vec4 ambient_color = vec4(0.1, 0.1, 0.1, 1) * color;
    lowp vec4 full_color = vec4(color.xyz * v_Dot, color.a) + ambient_color;
    gl_FragColor = full_color;
}
