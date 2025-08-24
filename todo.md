
## do
- convert color to a proper type like vec3_t
- destructor support for objects
	- type_set_destructor(type_t type, void (*)(object_t* o))
	- void texture_init() { type_set_destructor(type_texture, texture_destructor); }

## learn
- VirtualAlloc

## thoughts
- can we have the import just create a manifest of all assets and then we can preallocate for them.
- if in the future we need to support paging assets in/out we could create "bundles"
- we should be using our allocators more 
- allocator passed to the object_create 
- when loading by a name just scan for matching keys in the texture list
- threaded loading of assets (thread safe allocator for it)