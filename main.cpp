extern "C" {
#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>
}
#include <stdlib.h>
#include <math.h>
#include <SDL2/SDL.h>

#define arrayLength(arr) (sizeof(arr)/sizeof(*(arr)))
#define GC_THREAD_ON 1
#if GC_THREAD_ON
#define IF_GC_THREAD_ON(v) v
#define IF_GC_THREAD_ON_VAR(v) v
#else
#define IF_GC_THREAD_ON(v) 0
#define IF_GC_THREAD_ON_VAR(v) 0
#endif
#define UPDATE_ALL_ENTITIES 1

/*
single default
avgTicks = 27.473518
minTicks = 8
maxTicks = 16880
numAbove16 = 68445
numAbove33 = 49091
lua avg = 2.913245
lua max = 12.805262

all default
avgTicks = 31.327093
minTicks = 8
maxTicks = 59657
numAbove16 = 70702
numAbove33 = 49330
lua avg = 2.530447
lua max = 10.649895

single threaded
avgTicks = 44.683224
minTicks = 8
maxTicks = 59766
numAbove16 = 21496
numAbove33 = 7559
lua avg = 2.718985
lua max = 62.073116

all threaded
avgTicks = 30.165352
minTicks = 8
maxTicks = 58712
numAbove16 = 49316
numAbove33 = 27787
lua avg = 1.649091
lua max = 7.857541
 */

struct GCThreadData {
    SDL_mutex *mutex;
    lua_State *state;
    volatile bool quit;
};

struct V2 {
    float x, y;
};

struct Color {
    float r, g, b;
};

struct Entity {
    V2 p;
    V2 dp;
    Color color;
    Uint32 framesToBeActive;
};

V2 operator*(V2 v, float s) {
    V2 ret = {v.x*s, v.y*s};
    return ret;
}

V2 operator*(float s, V2 v) {
    return v*s;
}

V2 operator/(V2 v, float s) {
    V2 ret = {v.x/s, v.y/s};
    return ret;
}

V2 operator-(V2 a, V2 b) {
    V2 ret = {a.x - b.x, a.y - b.y};
    return ret;
}

V2 operator-(V2 v) {
    V2 ret = {-v.x, -v.y};
    return ret;
}

V2 operator+(V2 a, V2 b) {
    V2 ret = {a.x + b.x, a.y + b.y};
    return ret;
}

float inner(V2 a, V2 b) {
    return a.x*b.x+a.y*b.y;
}

float lengthSquared(V2 v) {
    return inner(v, v);
}

float length(V2 v) {
    return sqrtf(lengthSquared(v));
}

V2 normalize(V2 v) {
    return v/length(v);
}

float colorDistance(Color a, Color b) {
    Color delta = {a.r - b.r, a.g - b.g, a.b - b.b};
    return sqrtf(delta.r*delta.r+delta.g*delta.g+delta.b*delta.b);
}

void renderEntity(SDL_Renderer *renderer, Entity *entity) {
    //printf("%f, %f, %f\n", entity->color.r, entity->color.g, entity->color.b);
    SDL_SetRenderDrawColor(renderer,
                           (Uint8)(entity->color.r*255),
                           (Uint8)(entity->color.g*255),
                           (Uint8)(entity->color.b*255), 255);
    SDL_Rect r = {};
    r.h = 4;
    r.w = 4;
    r.x = entity->p.x;
    r.y = entity->p.y;
    SDL_RenderFillRect(renderer, &r);
}

int gcThreadFunc(void *data) {
    GCThreadData *gcThreadData = (GCThreadData *)data;
    printf("first\n");
    while (!gcThreadData->quit) {
        SDL_LockMutex(gcThreadData->mutex);
        if (!gcThreadData->quit) {
            //printf("trying to collect\n");
            //lua_gc(gcThreadData->state, LUA_GCSETPAUSE, 200);
            //lua_gc(gcThreadData->state, LUA_GCSETSTEPMUL, 200);
            lua_gc(gcThreadData->state, LUA_GCSTEP, 5);
            //lua_gc(gcThreadData->state, LUA_GCSETPAUSE, 50*1024);
            //lua_gc(gcThreadData->state, LUA_GCSETSTEPMUL, 500);
        }
        SDL_UnlockMutex(gcThreadData->mutex);
        SDL_Delay(0);
    }
    printf("gcc end\n");
    return 0;
}

int cTest(lua_State *state) {
    char *pToStr = (char *)lua_touserdata(state, 1);
    printf("Testing, sent \"%s\"\n", pToStr);
    lua_pushstring(state, "Hello!!");
    return 1;
}

void registerFunction(lua_State *state, lua_CFunction func, const char *name) {
    lua_pushcfunction(state, func);
    lua_setglobal(state, name);
}

int lGetEntity(lua_State *state) {
    lua_getfield(state, 1, "entities");
    Entity *entities = (Entity *)lua_touserdata(state, -1);
    int index = lua_tointeger(state, 2);
    lua_pushlightuserdata(state, entities + (index - 1));
    return 1;
}

int lGetDP(lua_State *state) {
    Entity *entity = (Entity *)lua_touserdata(state, 1);
    lua_createtable(state, 0, 2);
    lua_pushnumber(state, entity->dp.x);
    lua_setfield(state, -2, "x");
    lua_pushnumber(state, entity->dp.y);
    lua_setfield(state, -2, "y");
    return 1;
}

int lSetDP(lua_State *state) {
    Entity *entity = (Entity *)lua_touserdata(state, 1);
    lua_getfield(state, 2, "x");
    entity->dp.x = lua_tonumber(state, -1);
    lua_getfield(state, 2, "y");
    entity->dp.y = lua_tonumber(state, -1);
    return 0;
}


int lGetP(lua_State *state) {
    Entity *entity = (Entity *)lua_touserdata(state, 1);
    lua_createtable(state, 0, 2);
    lua_pushnumber(state, entity->p.x);
    lua_setfield(state, -2, "x");
    lua_pushnumber(state, entity->p.y);
    lua_setfield(state, -2, "y");
    return 1;
}

int lSetP(lua_State *state) {
    Entity *entity = (Entity *)lua_touserdata(state, 1);
    lua_getfield(state, 2, "x"); //x at index 3
    entity->p.x = lua_tonumber(state, -1);
    lua_getfield(state, 2, "y"); //x at index 4
    entity->p.y = lua_tonumber(state, -1);
    return 0;
}

int lSetColor(lua_State *state) {
    Entity *entity = (Entity *)lua_touserdata(state, 1);
    lua_getfield(state, 2, "r"); //x at index 3
    lua_getfield(state, 2, "g"); //x at index 4
    lua_getfield(state, 2, "b"); //x at index 4
    entity->color.r = lua_tonumber(state, 3);
    entity->color.g = lua_tonumber(state, 4);
    entity->color.b = lua_tonumber(state, 5);
    return 0;
}

int lGetLength(lua_State *state) {
    lua_getfield(state, 1, "length");
    return 1;
}



#if UPDATE_ALL_ENTITIES
#include "updateAllEntities.cpp"
#else
#include "updateSingleEntity.cpp"
#endif

int main(int argc, char* argv[]) {
    SDL_Init(SDL_INIT_VIDEO|SDL_INIT_AUDIO);
    // create new Lua state
    
    Entity entities[300];
    for (int i = 0; i < arrayLength(entities); ++i) {
        entities[i].framesToBeActive = 1;
    }
    
    SDL_Window *window;
    SDL_Renderer *renderer;
    SDL_CreateWindowAndRenderer(200, 200, 0, &window, &renderer);
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);
    SDL_RenderPresent(renderer);
    
    lua_State *lua_state;
    lua_state = luaL_newstate();
    
    // load Lua libraries
    luaL_openlibs(lua_state);
    //static const luaL_Reg lualibs[] = {{ "base", luaopen_base },
    //{ NULL, NULL}};
    // 
    //const luaL_Reg *lib = lualibs;
    //for(; lib->func != NULL; lib++) {
    //lib->func(lua_state);
    //lua_settop(lua_state, 0);
    //}

    Uint32 oldTicks = SDL_GetTicks();
    SDL_mutex *gcMutex = SDL_CreateMutex();
    GCThreadData gcThreadData;
    gcThreadData.mutex = gcMutex;
    gcThreadData.state = lua_state;
    gcThreadData.quit = false;
    IF_GC_THREAD_ON(lua_gc(lua_state, LUA_GCSTOP, 0));
    //IF_GC_THREAD_ON(lua_gc(lua_state, LUA_GCSETSTEPMUL, 1600));
    SDL_Thread *gcThread = IF_GC_THREAD_ON_VAR(SDL_CreateThread(gcThreadFunc, "gc thread", &gcThreadData));
    float avgTicks = 0;
    Uint32 maxTicks = 0;
    Uint32 minTicks = 1000000;
    Uint32 numAbove16 = 0;
    Uint32 numAbove33 = 0;
    float avgLuaTime = 0;
    float maxLuaTime = 0;
    luaSetup(lua_state, gcMutex);
    SDL_LockMutex(gcMutex);
    lua_getglobal(lua_state, "setup");
    lua_createtable(lua_state, 0, 2);
    lua_pushlightuserdata(lua_state, entities);
    lua_setfield(lua_state, -2, "entities");
    lua_pushinteger(lua_state, arrayLength(entities));
    lua_setfield(lua_state, -2, "length");
    if (lua_pcall(lua_state, 1, 0, 0)) {
        printf("ERROR! %s\n", lua_tostring(lua_state, -1));
    }
    lua_getglobal(lua_state, "luaTest");
    char *pToStr = "testing testing";
    lua_pushlightuserdata(lua_state, pToStr);
    if (lua_pcall(lua_state, 1, 0, 0)) {
        printf("ERROR! %s\n", lua_tostring(lua_state, -1));
    }
    SDL_UnlockMutex(gcMutex);
    Uint32 ticksSpentWaitingForLock = 0;
    int numRuns = 60*60*30;
    for (int i = 0; i < numRuns; i++) {
        Uint64 tookPerf = updateAndRender(entities, arrayLength(entities),
                                          renderer, lua_state, gcMutex);
        avgLuaTime += tookPerf;
        if (maxLuaTime < tookPerf) {
            maxLuaTime = tookPerf;
        }
        SDL_RenderPresent(renderer);
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);
        Uint32 currTicks = SDL_GetTicks() - oldTicks;
        avgTicks += currTicks;
        if (currTicks > maxTicks) {
            maxTicks = currTicks;
        }
        if (currTicks < minTicks) {
            minTicks = currTicks;
        }
        if (currTicks < 16) {
            SDL_Delay(16 - currTicks);
        }
        if (currTicks >= 16) {
            numAbove16++;
        }
        if (currTicks >= 33) {
            numAbove33++;
        }
        oldTicks = SDL_GetTicks();
    }
    avgTicks /= numRuns;
    avgLuaTime /= numRuns;
    printf("avgTicks = %f\nminTicks = %u\nmaxTicks = %u\n"
           "numAbove16 = %u\nnumAbove33 = %u\n"
           "lua avg = %f\nlua max = %f\n",
           avgTicks, minTicks, maxTicks, numAbove16, numAbove33,
           (avgLuaTime/SDL_GetPerformanceFrequency())*1000,
           (maxLuaTime/SDL_GetPerformanceFrequency())*1000);
    
    // close the Lua state
    SDL_LockMutex(gcMutex);
    gcThreadData.quit = true;
    lua_close(lua_state);
    SDL_UnlockMutex(gcMutex);
    int ret;
    SDL_Delay(10);
    SDL_WaitThread(gcThread, &ret);
    SDL_Quit();
    printf("end\n");
    return 0;
}
