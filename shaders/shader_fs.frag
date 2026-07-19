#version 330 core

in vec3 v_normal;
in vec3 v_world_pos;
in vec4 v_color;           // RGBA (updated from vec3 to pass instance alpha)
in vec2 v_uv;              // UV for edge detection (Tetris3D-inspired)
in vec3 v_aPos;            // Debug: vertex position
in vec3 v_aNormal;         // Debug: vertex normal
in vec3 v_iPos;            // Debug: instance position
in vec4 v_iColor;          // Debug: instance color (RGBA)

out vec4 FragColor;

uniform vec3   u_solid_color;
uniform int    u_debug_mode;     // 0 = normal, 1 = instance position, 2 = vertex position, 3 = vertex normal
uniform vec3   u_lightPosition;  // Light direction (from Tetris3D: configurable)
uniform int    u_renderingMode;  // 0 = neon glow, 1 = solid lit
uniform float  u_neonIntensity;  // Glow brightness (from Tetris3D)

void main() {

    // Debug mode: Display different vertex attributes as colors when enabled
    if (u_debug_mode != 0) {
        if (u_debug_mode == 1) {
            // Show instance position as color (normalized to [0,1] range)
            FragColor = vec4((v_iPos + 0.5) * 0.5, 1.0);
        } else if (u_debug_mode == 2) {
            // Show vertex position as color (normalized to [0,1] range)
            FragColor = vec4((v_aPos + 0.5) * 0.5, 1.0);
        } else if (u_debug_mode == 3) {
            // Show vertex normal as color (normalized to [0,1] range)
            FragColor = vec4((v_aNormal + 1.0) * 0.5, 1.0);
        }
        return;
    }

    // Edge detection: distance from each edge of the face [0,1]x[0,1]
    float edgeSize = 0.06;        // Tight edge (Tetris3D uses 0.05)
    float smoothWidth = 0.015;    // Slightly softer than Tetris3D's 0.01

    float distX = min(v_uv.x, 1.0 - v_uv.x);
    float distY = min(v_uv.y, 1.0 - v_uv.y);

    float edgeFactorX = 1.0 - smoothstep(edgeSize - smoothWidth, edgeSize + smoothWidth, distX);
    float edgeFactorY = 1.0 - smoothstep(edgeSize - smoothWidth, edgeSize + smoothWidth, distY);
    float edgeFactor = max(edgeFactorX, edgeFactorY);


    if (u_renderingMode == 0) {
        // ================================================================
        //  NEON GLOW MODE (from Tetris3D)
        //  UV-based edge detection + additive blending
        // ================================================================

        // SAFEGUARD: ensure intensity never drops to zero
        float safeIntensity = max(u_neonIntensity, 0.01);

        // Face color: tinted by intensity (Tetris3D: Color.rgb * safeIntensity)
        // Edge color: white-hot with slight alpha boost on edge (Tetris3D: Color.a + 0.4)
        vec3 faceColor = v_color.rgb * safeIntensity;
        vec3 edgeColor = vec3(1.0) * safeIntensity;

        // Mix between face and edge based on proximity to edge
        vec3 finalColor = mix(faceColor, edgeColor, edgeFactor);

        // Use original instance alpha for face, boost edge alpha slightly
        // Tetris3D: face alpha = Color.a, edge alpha = Color.a + 0.4
        float finalAlpha = mix(v_color.a, v_color.a + 0.15, edgeFactor);

        FragColor = vec4(finalColor, finalAlpha);

    } else {
        // ================================================================
        //  SOLID LIT MODE WITH DARK EDGES
        // ================================================================

        vec3 N = normalize(v_normal);
        // Tetris3D uses fixed light direction vec3(0.5, 1.0, 0.5)
        vec3 L = normalize(vec3(0.5, 1.0, 0.5));

        float diff = max(dot(N, L), 0.0);
        float ambient = 0.3;

        // Calculate the standard diffuse lighting
        vec3 baseLitColor = v_color.rgb * (ambient + diff);

        // Define how dark the edges should be (0.2 = 80% darker, 0.0 = pitch black)
        vec3 darkEdgeColor = baseLitColor * 0.2; 

        // Mix between the base lit color and the dark edge color based on UV proximity
        vec3 finalColor = mix(baseLitColor, darkEdgeColor, edgeFactor);

        FragColor = vec4(finalColor, 1.0);
    }
}
