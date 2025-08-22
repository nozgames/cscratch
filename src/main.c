#include "renderer.h"
#include "scene.h"

int main(int argc, char* argv[])
{
    renderer_init();
    scene_init();

    camera_t camera = camera_create();
    vec3 pos;
    entity_position((entity_t)camera, pos);

    scene_uninit();
    renderer_uninit();

	return 0;
}
