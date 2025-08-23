//
//  NoZ Game Engine - Copyright(c) 2025 NoZ Games, LLC
//

#include <noz/file_watcher.h>
#include <noz/hash.h>
#include <noz/map.h>

NOZ_WARNINGS_DISABLE()
#include <SDL3/SDL.h>
#include <string.h>
#include <stdio.h>
#include <assert.h>
NOZ_WARNINGS_ENABLE()

#ifndef nullptr
#define nullptr NULL
#endif

#define MAX_WATCHED_DIRS 32
#define MAX_EVENTS_QUEUE 4096
#define MAX_TRACKED_FILES 16384

// File info for tracking changes
typedef struct file_info
{
    path_t path;
    uint64_t mtime;  // Modification time
    uint64_t size;   // File size
} file_info_t;

// Event queue (circular buffer)
typedef struct event_queue
{
    file_change_event_t events[MAX_EVENTS_QUEUE];
    size_t head;
    size_t tail;
    size_t count;
    SDL_Mutex* mutex;
} event_queue_t;

// Global file watcher state
static struct
{
    // Configuration
    int poll_interval_ms;
    
    // Watched directories
    char watched_dirs[MAX_WATCHED_DIRS][1024];
    size_t watched_dir_count;
    
    // File tracking
    map_t* file_map;  // Maps file path hash -> file_info_t*
    file_info_t* file_pool;
    size_t file_pool_size;
    size_t file_pool_used;
    
    // Event queue
    event_queue_t queue;
    
    // Threading
    SDL_Thread* thread;
    SDL_AtomicInt should_stop;
    SDL_Mutex* dirs_mutex;
    
    // State
    bool initialized;
    bool running;
} g_watcher = {0};

// Forward declarations
static int file_watcher_thread(void* data);
static void scan_directory_recursive(const char* dir_path);
static void process_file(const char* file_path, const file_stat_t* st);
static void queue_event(const path_t* path, file_change_type_t type);
static file_info_t* alloc_file_info(void);
static void free_file_info(file_info_t* info);

// Initialize the file watcher system
void file_watcher_init(int poll_interval_ms)
{
    if (g_watcher.initialized)
    {
        return;
    }
    
    g_watcher.poll_interval_ms = poll_interval_ms > 0 ? poll_interval_ms : 1000;
    g_watcher.watched_dir_count = 0;
    
    // Initialize file tracking
    g_watcher.file_map = map_alloc(NULL, MAX_TRACKED_FILES);
    g_watcher.file_pool = (file_info_t*)calloc(MAX_TRACKED_FILES, sizeof(file_info_t));
    g_watcher.file_pool_size = MAX_TRACKED_FILES;
    g_watcher.file_pool_used = 0;
    
    // Initialize queue
    g_watcher.queue.head = 0;
    g_watcher.queue.tail = 0;
    g_watcher.queue.count = 0;
    g_watcher.queue.mutex = SDL_CreateMutex();
    
    // Initialize synchronization
    SDL_SetAtomicInt(&g_watcher.should_stop, 0);
    g_watcher.dirs_mutex = SDL_CreateMutex();
    
    g_watcher.initialized = true;
    g_watcher.running = false;
}

// Shutdown the file watcher system
void file_watcher_shutdown(void)
{
    if (!g_watcher.initialized)
    {
        return;
    }
    
    // Stop watching
    file_watcher_stop();
    
    // Clean up file tracking
    if (g_watcher.file_map)
    {
        object_free(g_watcher.file_map);
        g_watcher.file_map = nullptr;
    }
    
    if (g_watcher.file_pool)
    {
        free(g_watcher.file_pool);
        g_watcher.file_pool = nullptr;
    }
    
    // Clean up synchronization
    if (g_watcher.queue.mutex)
    {
        SDL_DestroyMutex(g_watcher.queue.mutex);
        g_watcher.queue.mutex = nullptr;
    }
    
    if (g_watcher.dirs_mutex)
    {
        SDL_DestroyMutex(g_watcher.dirs_mutex);
        g_watcher.dirs_mutex = nullptr;
    }
    
    g_watcher.initialized = false;
}

// Add a directory to watch
bool file_watcher_add_directory(const char* directory)
{
    if (!g_watcher.initialized || !directory)
    {
        return false;
    }
    
    SDL_LockMutex(g_watcher.dirs_mutex);
    
    // Check if already watching this directory
    for (size_t i = 0; i < g_watcher.watched_dir_count; i++)
    {
        if (strcmp(g_watcher.watched_dirs[i], directory) == 0)
        {
            SDL_UnlockMutex(g_watcher.dirs_mutex);
            return true;  // Already watching
        }
    }
    
    // Add new directory
    if (g_watcher.watched_dir_count >= MAX_WATCHED_DIRS)
    {
        SDL_UnlockMutex(g_watcher.dirs_mutex);
        return false;  // Too many directories
    }
    
    strncpy(g_watcher.watched_dirs[g_watcher.watched_dir_count], directory, 1023);
    g_watcher.watched_dirs[g_watcher.watched_dir_count][1023] = '\0';
    g_watcher.watched_dir_count++;
    
    SDL_UnlockMutex(g_watcher.dirs_mutex);
    
    // If already running, do an initial scan of the new directory
    if (g_watcher.running)
    {
        scan_directory_recursive(directory);
    }
    
    return true;
}

// Remove a directory from watching
bool file_watcher_remove_directory(const char* directory)
{
    if (!g_watcher.initialized || !directory)
    {
        return false;
    }
    
    SDL_LockMutex(g_watcher.dirs_mutex);
    
    for (size_t i = 0; i < g_watcher.watched_dir_count; i++)
    {
        if (strcmp(g_watcher.watched_dirs[i], directory) == 0)
        {
            // Shift remaining directories down
            for (size_t j = i; j < g_watcher.watched_dir_count - 1; j++)
            {
                strcpy(g_watcher.watched_dirs[j], g_watcher.watched_dirs[j + 1]);
            }
            g_watcher.watched_dir_count--;
            SDL_UnlockMutex(g_watcher.dirs_mutex);
            return true;
        }
    }
    
    SDL_UnlockMutex(g_watcher.dirs_mutex);
    return false;
}

// Start watching
bool file_watcher_start(void)
{
    if (!g_watcher.initialized || g_watcher.running)
    {
        return false;
    }
    
    if (g_watcher.watched_dir_count == 0)
    {
        return false;  // No directories to watch
    }
    
    // Do initial scan of all directories
    SDL_LockMutex(g_watcher.dirs_mutex);
    for (size_t i = 0; i < g_watcher.watched_dir_count; i++)
    {
        scan_directory_recursive(g_watcher.watched_dirs[i]);
    }
    SDL_UnlockMutex(g_watcher.dirs_mutex);
    
    // Start the watching thread
    SDL_SetAtomicInt(&g_watcher.should_stop, 0);
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable: 4191) // SDL_CreateThread causes this on Windows
#endif
    g_watcher.thread = SDL_CreateThread(file_watcher_thread, "FileWatcher", NULL);
#ifdef _MSC_VER
#pragma warning(pop)
#endif
    
    if (!g_watcher.thread)
    {
        return false;
    }
    
    g_watcher.running = true;
    return true;
}

// Stop watching
void file_watcher_stop(void)
{
    if (!g_watcher.initialized || !g_watcher.running)
    {
        return;
    }
    
    // Signal thread to stop
    SDL_SetAtomicInt(&g_watcher.should_stop, 1);
    
    // Wait for thread to finish
    if (g_watcher.thread)
    {
        SDL_WaitThread(g_watcher.thread, nullptr);
        g_watcher.thread = nullptr;
    }
    
    g_watcher.running = false;
}

// Poll for file changes
bool file_watcher_poll(file_change_event_t* event)
{
    if (!g_watcher.initialized || !event)
    {
        return false;
    }
    
    SDL_LockMutex(g_watcher.queue.mutex);
    
    if (g_watcher.queue.count == 0)
    {
        SDL_UnlockMutex(g_watcher.queue.mutex);
        return false;
    }
    
    // Dequeue event
    *event = g_watcher.queue.events[g_watcher.queue.head];
    g_watcher.queue.head = (g_watcher.queue.head + 1) % MAX_EVENTS_QUEUE;
    g_watcher.queue.count--;
    
    SDL_UnlockMutex(g_watcher.queue.mutex);
    return true;
}

// Get pending event count
size_t file_watcher_pending_count(void)
{
    if (!g_watcher.initialized)
    {
        return 0;
    }
    
    SDL_LockMutex(g_watcher.queue.mutex);
    size_t count = g_watcher.queue.count;
    SDL_UnlockMutex(g_watcher.queue.mutex);
    
    return count;
}

// Clear all pending events
void file_watcher_clear_queue(void)
{
    if (!g_watcher.initialized)
    {
        return;
    }
    
    SDL_LockMutex(g_watcher.queue.mutex);
    g_watcher.queue.head = 0;
    g_watcher.queue.tail = 0;
    g_watcher.queue.count = 0;
    SDL_UnlockMutex(g_watcher.queue.mutex);
}

// Check if running
bool file_watcher_is_running(void)
{
    return g_watcher.initialized && g_watcher.running;
}

// Worker thread function
static int file_watcher_thread(void* data)
{
    (void)data;
    
    while (SDL_GetAtomicInt(&g_watcher.should_stop) == 0)
    {
        // Mark all existing files as "not seen"
        // (We'll use size = 0 to indicate not seen in this pass)
        for (size_t i = 0; i < g_watcher.file_pool_used; i++)
        {
            if (!path_is_empty(&g_watcher.file_pool[i].path))
            {
                g_watcher.file_pool[i].size = 0;  // Mark as not seen
            }
        }
        
        // Scan all watched directories
        SDL_LockMutex(g_watcher.dirs_mutex);
        for (size_t i = 0; i < g_watcher.watched_dir_count; i++)
        {
            scan_directory_recursive(g_watcher.watched_dirs[i]);
        }
        SDL_UnlockMutex(g_watcher.dirs_mutex);
        
        // Check for deleted files (files that weren't seen in this pass)
        for (size_t i = 0; i < g_watcher.file_pool_used; i++)
        {
            file_info_t* info = &g_watcher.file_pool[i];
            if (!path_is_empty(&info->path) && info->size == 0)
            {
                // File was not seen in this pass, it was deleted
                queue_event(&info->path, file_change_type_deleted);
                
                // Remove from map
                uint64_t key = hash_string(info->path.value);
                map_remove(g_watcher.file_map, key);
                
                // Clear the file info
                path_clear(&info->path);
            }
        }
        
        // Sleep for poll interval
        SDL_Delay(g_watcher.poll_interval_ms);
    }
    
    return 0;
}

// Callback function for platform directory scanning
static void file_scan_callback(const char* file_path, const file_stat_t* stat, void* user_data)
{
    (void)user_data; // Unused
    
    // Only process regular files
    if (stat->is_regular_file)
    {
        process_file(file_path, stat);
    }
}

// Cross-platform implementation using platform abstraction
static void scan_directory_recursive(const char* dir_path)
{
    platform_enum_files(dir_path, file_scan_callback, NULL);
}

// Process a single file (cross-platform)
static void process_file(const char* file_path, const file_stat_t* st)
{
    path_t path;
    path_set(&path, file_path);
    
    uint64_t key = hash_string(file_path);
    file_info_t* existing = (file_info_t*)map_get(g_watcher.file_map, key);
    
    if (existing)
    {
        // File already tracked, check for modifications
        if (existing->mtime != st->mtime)
        {
            // File was modified
            queue_event(&existing->path, file_change_type_modified);
            existing->mtime = st->mtime;
        }
        // Mark as seen by restoring the size
        existing->size = st->size;
    }
    else
    {
        // New file
        file_info_t* info = alloc_file_info();
        if (info)
        {
            path_copy(&info->path, &path);
            info->mtime = st->mtime;
            info->size = st->size;
            
            map_set(g_watcher.file_map, key, info);
            queue_event(&info->path, file_change_type_added);
        }
    }
}

// Queue an event
static void queue_event(const path_t* path, file_change_type_t type)
{
    SDL_LockMutex(g_watcher.queue.mutex);
    
    if (g_watcher.queue.count >= MAX_EVENTS_QUEUE)
    {
        // Queue is full, drop oldest event
        g_watcher.queue.head = (g_watcher.queue.head + 1) % MAX_EVENTS_QUEUE;
        g_watcher.queue.count--;
    }
    
    // Add new event
    file_change_event_t* event = &g_watcher.queue.events[g_watcher.queue.tail];
    path_copy(&event->path, path);
    event->type = type;
    event->timestamp = SDL_GetTicks();
    
    g_watcher.queue.tail = (g_watcher.queue.tail + 1) % MAX_EVENTS_QUEUE;
    g_watcher.queue.count++;
    
    SDL_UnlockMutex(g_watcher.queue.mutex);
}

// Allocate a file info from the pool
static file_info_t* alloc_file_info(void)
{
    // First try to find a free slot
    for (size_t i = 0; i < g_watcher.file_pool_used; i++)
    {
        if (path_is_empty(&g_watcher.file_pool[i].path))
        {
            return &g_watcher.file_pool[i];
        }
    }
    
    // Allocate new slot
    if (g_watcher.file_pool_used < g_watcher.file_pool_size)
    {
        return &g_watcher.file_pool[g_watcher.file_pool_used++];
    }
    
    return nullptr;  // Pool is full
}

// Free a file info (just clear it) - currently unused but may be needed later
#if 0
static void free_file_info(file_info_t* info)
{
    if (info)
    {
        path_clear(&info->path);
    }
}
#endif