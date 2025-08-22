void entity_init();

void scene_init()
{
    entity_init();
    camera_init();
}

void scene_uninit()
{
    camera_uninit();
}
