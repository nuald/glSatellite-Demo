uniform sampler2D tex0;

varying mediump vec2 textureCoordinate;
varying highp vec3 v_color;

void main() {
    // texture is the alpha-mask too
    lowp vec4 tex_color = texture2D(tex0, textureCoordinate);
    lowp vec4 color = tex_color * vec4(v_color, 1.0);
    gl_FragColor = color;
}
