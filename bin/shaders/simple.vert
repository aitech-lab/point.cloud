#version 330

uniform mat4 mvp;
uniform mat4 rot;
uniform int render_id;
uniform float off;
uniform float min;
uniform float max;

uniform float point_size;
uniform float alpha_1;
uniform float alpha_2;

layout (location = 0) in vec4 pos;
layout (location = 1) in float data;

flat out vec4 out_col;

// All components are in the range [0…1], including hue.
vec3 hsv2rgb(vec3 c) {
    vec4 K = vec4(1.0, 2.0 / 3.0, 1.0 / 3.0, 3.0);
    vec3 p = abs(fract(c.xxx + K.xyz) * 6.0 - K.www);
    return c.z * mix(K.xxx, clamp(p - K.xxx, 0.0, 1.0), c.y);
}

// Кодируем uint в vec4
vec4 encode_id(int id) {
    
    return vec4(
        float((uint(id) >> 24) & 0xFFu) / 255.0,
        float((uint(id) >> 16) & 0xFFu) / 255.0,
        float((uint(id) >>  8) & 0xFFu) / 255.0,
        // float((uint(id)      ) & 0xFFu) / 255.0,
        1.0 
    );
}

void main() {
    vec4 a = vec4(1.0, 0.0, 0.0, 0.5);
    vec4 b = vec4(0.0, 0.0, 1.0, 0.5);
    
    int i = 1;
    float k = 0.0;
    if(render_id==0) {
        if(data<min || data>max) {
            out_col = vec4(0.2,0.2,0.2,alpha_1);
        } else {
            k = (data-min)/(max-min);
            // out_col = a*k + b*(1-k);
            out_col = vec4(hsv2rgb(vec3(k*0.75, 1.0, 1.0)), alpha_2);
        }
        gl_PointSize = point_size;
    } else {
        out_col = encode_id(gl_VertexID<<8);
        gl_PointSize = 10.0;
    }
    // gl_Position = mvp*(vec4(0.0, 0.0, 0.0, off)+rot*pos);
    gl_Position = mvp*vec4(pos.xyz, 1.0);
}
