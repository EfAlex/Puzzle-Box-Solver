#version 330 core

layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aNormal;
layout(location = 2) in vec3 iPos;      // Instance position
layout(location = 3) in vec4 iColor;    // Instance color (RGBA)
layout(location = 4) in vec3 iRot0;     // Instance rotation matrix row 0
layout(location = 5) in vec3 iRot1;     // Instance rotation matrix row 1
layout(location = 6) in vec3 iRot2;     // Instance rotation matrix row 2
layout(location = 7) in vec2 aUV;       // UV coordinates for edge detection (Tetris3D-inspired)

out vec3 v_normal;
out vec3 v_world_pos;
out vec4 v_color;
out vec2 v_uv;           // UV for edge detection
out vec3 v_aPos;
out vec3 v_aNormal;
out vec3 v_iPos;
out vec4 v_iColor;

uniform mat4 u_proj;
uniform mat4 u_view;
uniform mat4 u_model;

void main() {
    mat3 localRot = mat3(iRot0, iRot1, iRot2);

    vec3 localPos = localRot * aPos;
    vec3 localNormal = localRot * aNormal;

    vec3 worldPos = mat3(u_model) * (localRot * aPos + iPos);
    vec3 worldNormal = mat3(u_model) * localNormal;

    gl_Position = u_proj * u_view * vec4(worldPos, 1.0);

    v_normal = worldNormal;
    v_world_pos = worldPos;
    v_color = iColor;  // Pass RGBA color to fragment shader
    v_uv = aUV;  // Pass UV to fragment shader for edge detection

    v_aPos = aPos;
    v_aNormal = aNormal;
    v_iPos = iPos;
    v_iColor = iColor;
}
