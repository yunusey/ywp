#version 310 es
precision highp float;

uniform vec2 u_viewport;
uniform int  u_num_bars;
uniform float u_thickness;

layout(std430, binding = 0) buffer SplineBuffer {
    float control_points[];
};

#define CATPPUCCIN_SIZE 14
vec3 catppuccin_mocha[CATPPUCCIN_SIZE] = vec3[](
    vec3(0.961, 0.878, 0.863), // Rosewater  
    vec3(0.949, 0.804, 0.804), // Flamingo  
    vec3(0.961, 0.761, 0.906), // Pink  
    vec3(0.796, 0.651, 0.969), // Mauve  
    vec3(0.953, 0.545, 0.659), // Red  
    vec3(0.922, 0.627, 0.675), // Maroon  
    vec3(0.980, 0.702, 0.529), // Peach  
    vec3(0.976, 0.886, 0.686), // Yellow  
    vec3(0.651, 0.890, 0.631), // Green  
    vec3(0.580, 0.886, 0.835), // Teal  
    vec3(0.537, 0.863, 0.922), // Sky  
    vec3(0.455, 0.780, 0.925), // Sapphire  
    vec3(0.537, 0.706, 0.980), // Blue  
    vec3(0.706, 0.745, 0.996)  // Lavender  
);

vec4 background_color = vec4(0.118, 0.118, 0.180, 1.0);

out vec4 fragColor;

float catmullRom(float p0, float p1, float p2, float p3, float t) {
    float t2 = t * t;
    float t3 = t2 * t;
    return 0.5 * (
        (2.0 * p1) +
        (-p0 + p2) * t +
        (2.0 * p0 - 5.0 * p1 + 4.0 * p2 - p3) * t2 +
        (-p0 + 3.0 * p1 - 3.0 * p2 + p3) * t3
    );
}

void main() {
    float x_norm = gl_FragCoord.x / u_viewport.x;
    float y_norm = gl_FragCoord.y / u_viewport.y;

    float global_pos = x_norm * float(u_num_bars - 1);
    int idx = int(global_pos);
    float t = fract(global_pos);

    int i0 = clamp(idx - 1, 0, u_num_bars - 1);
    int i1 = clamp(idx,     0, u_num_bars - 1);
    int i2 = clamp(idx + 1, 0, u_num_bars - 1);
    int i3 = clamp(idx + 2, 0, u_num_bars - 1);

    // Read safely from SSBO
    float p0 = control_points[i0];
    float p1 = control_points[i1];
    float p2 = control_points[i2];
    float p3 = control_points[i3];

    float splineY = catmullRom(p0, p1, p2, p3, t);

    // Background above spline
    if (y_norm > splineY) {
        fragColor = background_color;
        return;
    }

    // Below spline → apply Catppuccin color based on x coordinate
	float color_f = x_norm * float(CATPPUCCIN_SIZE - 1);
    int color_idx0 = int(floor(color_f));
	int color_idx1 = min(color_idx0 + 1, CATPPUCCIN_SIZE - 1);
	float blend = fract(color_f);
	// float blend = 0.0; // No blend

    vec3 baseColor = mix(catppuccin_mocha[color_idx0], catppuccin_mocha[color_idx1], blend);

    fragColor = vec4(baseColor, 1.0);
}

