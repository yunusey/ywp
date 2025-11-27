#version 310 es
precision highp float;

#define PI 3.14159265358979323846
#define INNER_CIRCLE_RADIUS 0.3
#define OUTER_CIRCLE_RADIUS 0.9

uniform vec2 u_viewport;
uniform int u_num_bars;
uniform float u_time;

layout(std430, binding = 0) buffer CavaBuffer {
    double cava_out[];
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


vec4 circle_color = vec4(0.804, 0.804, 0.957, 1.0);
vec4 background_color = vec4(0.118, 0.118, 0.180, 1.0);

out vec4 fragColor;

void main() {
    float x_norm = gl_FragCoord.x / u_viewport.x;
    float y_norm = gl_FragCoord.y / u_viewport.y;
	vec2 center = vec2(u_viewport.x / 2.0, u_viewport.y / 2.0);
	float largest_square = min(u_viewport.x, u_viewport.y);

	// Draw circle
	float dist_to_center = distance(gl_FragCoord.xy, center) / (largest_square / 2.0);
	if (dist_to_center < INNER_CIRCLE_RADIUS) {
		fragColor = circle_color;
		return;
	}

	// We want to draw a rectangle from the circle edge outwards using idx and val
	float angle = atan(gl_FragCoord.y - center.y, gl_FragCoord.x - center.x);
	if (angle < 0.0) {
		angle += 2.0 * PI;
	}
	float normalized_angle = angle / (2.0 * PI);
	float global_pos = normalized_angle * float(u_num_bars - 1);
	int idx = int(global_pos);
	float val = float(cava_out[idx]);

	float radius = INNER_CIRCLE_RADIUS + (val * (OUTER_CIRCLE_RADIUS - INNER_CIRCLE_RADIUS));
	float dist_from_center = distance(gl_FragCoord.xy, center) / (largest_square / 2.0);

	if (dist_from_center <= radius) {
		int color_idx = idx % CATPPUCCIN_SIZE;
		vec3 color = catppuccin_mocha[color_idx];
		fragColor = vec4(color, 1.0);
	} else {
		fragColor = background_color;
	}
}
