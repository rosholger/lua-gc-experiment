extern "C" {
#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>
}
#include <stdlib.h>
#include <SDL2/SDL.h>

struct GCThreadData {
    SDL_mutex *mutex;
    lua_State *state;
};

int gcThreadFunc(void *data) {
    GCThreadData *gcThreadData = (GCThreadData *)data;
    while (true) {
        SDL_LockMutex(gcThreadData->mutex);
        //printf("trying to collect\n");
        //lua_gc(gcThreadData->state, LUA_GCSETPAUSE, 200);
        //lua_gc(gcThreadData->state, LUA_GCSETSTEPMUL, 200);
        lua_gc(gcThreadData->state, LUA_GCSTEP, 200);
        //lua_gc(gcThreadData->state, LUA_GCSETPAUSE, 50*1024);
        //lua_gc(gcThreadData->state, LUA_GCSETSTEPMUL, 500);
        SDL_UnlockMutex(gcThreadData->mutex);
        SDL_Delay(0);
    }
}

int main(int argc, char* argv[]) {
    // create new Lua state
    
    lua_State *lua_state;
    lua_state = luaL_newstate();
    
    // load Lua libraries
    static const luaL_Reg lualibs[] = {{ "base", luaopen_base },
                                       { NULL, NULL}};
    
    const luaL_Reg *lib = lualibs;
    for(; lib->func != NULL; lib++) {
        lib->func(lua_state);
        lua_settop(lua_state, 0);
    }
    
    // run the Lua script
    Uint32 oldTicks = SDL_GetTicks();
    SDL_mutex *gcMutex = SDL_CreateMutex();
    GCThreadData gcThreadData;
    gcThreadData.mutex = gcMutex;
    gcThreadData.state = lua_state;
    SDL_Thread *gcThread = SDL_CreateThread(gcThreadFunc, "gc thread", &gcThreadData);
    float avgTicks = 0;
    Uint32 maxTicks = 0;
    Uint32 minTicks = 1000000;
    Uint32 numAbove16 = 0;
    Uint32 numAbove33 = 0;
    lua_gc(lua_state, LUA_GCSTOP, 0);
    for (int i = 0; i < 5000; i++) {
        SDL_LockMutex(gcMutex);
        luaL_dofile(lua_state, "helloworld.lua");
        SDL_UnlockMutex(gcMutex);
        SDL_Delay(1);
        Uint32 currTicks = SDL_GetTicks() - oldTicks;
        avgTicks += currTicks;
        if (currTicks > maxTicks) {
            maxTicks = currTicks;
        }
        if (currTicks < minTicks) {
            minTicks = currTicks;
        }
        //printf("currTicks: %u\n", currTicks);
        if (currTicks >= 16) {
            numAbove16++;
        //printf("waiting\n");
        //SDL_Delay(16 - currTicks);
        }
        if (currTicks >= 33) {
            numAbove33++;
        //printf("waiting\n");
        //SDL_Delay(16 - currTicks);
        }
        oldTicks = SDL_GetTicks();
    }
    avgTicks /= 5000.0;
    printf("avgTicks = %f\nminTicks = %u\nmaxTicks = %u\n"
           "numAbove16 = %u\nnumAbove33 = %u\n",
           avgTicks, minTicks, maxTicks, numAbove16, numAbove33);
    
    // close the Lua state
    lua_close(lua_state);
    printf("end\n");
}
