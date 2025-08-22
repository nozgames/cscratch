#include <stdio.h>
#include <cglm/cglm.h>

int main() {
    // Create vectors
    vec3 position = {1.0f, 2.0f, 3.0f};
    vec3 rotation = {0.0f, 45.0f, 0.0f}; // degrees
    vec3 scale = {2.0f, 2.0f, 2.0f};
    
    // Create matrices
    mat4 model = GLM_MAT4_IDENTITY_INIT;
    mat4 view = GLM_MAT4_IDENTITY_INIT;
    mat4 projection = GLM_MAT4_IDENTITY_INIT;
    
    // Build transformation matrix
    glm_translate(model, position);
    glm_rotate_y(model, glm_rad(rotation[1]), model);
    glm_scale(model, scale);
    
    // Create view matrix (camera at origin looking down -Z)
    vec3 eye = {0.0f, 0.0f, 5.0f};
    vec3 center = {0.0f, 0.0f, 0.0f};
    vec3 up = {0.0f, 1.0f, 0.0f};
    glm_lookat(eye, center, up, view);
    
    // Create perspective projection
    glm_perspective(glm_rad(45.0f), 16.0f/9.0f, 0.1f, 100.0f, projection);
    
    // Combine matrices: MVP = projection * view * model
    mat4 mvp;
    glm_mat4_mul(projection, view, mvp);
    glm_mat4_mul(mvp, model, mvp);
    
    // Vector operations
    vec3 a = {1.0f, 0.0f, 0.0f};
    vec3 b = {0.0f, 1.0f, 0.0f};
    vec3 cross_product;
    glm_vec3_cross(a, b, cross_product);
    
    float dot_product = glm_vec3_dot(a, b);
    float length = glm_vec3_norm(a);
    
    printf("Cross product: [%.2f, %.2f, %.2f]\n", 
           cross_product[0], cross_product[1], cross_product[2]);
    printf("Dot product: %.2f\n", dot_product);
    printf("Vector length: %.2f\n", length);
    
    return 0;
}