#pragma once


// Because mutexes and semaphores are refcounted in gdextension we need this wrapper, otherwise the destructor for them never gets called for some reason
#ifdef GDEXTENSION

#include <godot_cpp/classes/mutex.hpp>
#include <godot_cpp/classes/semaphore.hpp>

namespace CoreBind {
    using Mutex = godot::Mutex;
    using Semaphore = godot::Semaphore;
};

#endif