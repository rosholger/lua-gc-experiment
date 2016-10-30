void luaSetup(lua_State *lua_state, SDL_mutex *gcMutex) {
    SDL_LockMutex(gcMutex);
    registerFunction(lua_state, cTest, "cTest");
    registerFunction(lua_state, lGetEntity, "getEntity");
    registerFunction(lua_state, lGetP, "getP");
    registerFunction(lua_state, lSetP, "setP");
    registerFunction(lua_state, lGetDP, "getDP");
    registerFunction(lua_state, lSetDP, "setDP");
    registerFunction(lua_state, lSetColor, "setColor");
    registerFunction(lua_state, lGetLength, "getLength");
    luaL_dofile(lua_state, "common.lua");
    luaL_dofile(lua_state, "updateAllEntities.lua");
    SDL_UnlockMutex(gcMutex);
}

void luaUpdate(lua_State *state, SDL_mutex *gcMutex,
               Entity *entities, int entitiesLength) {
    IF_GC_THREAD_ON(SDL_LockMutex(gcMutex));
    lua_getglobal(state, "update");
    lua_createtable(state, 0, 2);
    lua_pushlightuserdata(state, entities);
    lua_setfield(state, -2, "entities");
    lua_pushinteger(state, entitiesLength);
    lua_setfield(state, -2, "length");
    if (lua_pcall(state, 1, 0, 0)) {
        printf("ERROR! %s\n", lua_tostring(state, -1));
    }
    //printf("amount of memory = %d\n", lua_gc(state, LUA_GCCOUNT, 0));
    IF_GC_THREAD_ON(SDL_UnlockMutex(gcMutex));
}

Uint64 updateAndRender(Entity *entities, int entitiesLength,
                       SDL_Renderer *renderer, lua_State *lua_state,
                       SDL_mutex *gcMutex) {
    Uint64 currPerf = SDL_GetPerformanceCounter();
    luaUpdate(lua_state, gcMutex, entities, entitiesLength);
    Uint64 tookPerf = SDL_GetPerformanceCounter() - currPerf;
    for (int i = 0; i < entitiesLength; ++i) {
        if (entities[i].framesToBeActive) {
            Entity *e = entities + i;
            for (int j = i+1; j < entitiesLength; ++j) {
                Entity *o = entities + j;
                if (o->framesToBeActive) {
                    V2 d = e->p - o->p;
                    float posDist = length(d);
                    if (posDist != 0) {
                        float colorDiff = colorDistance(e->color, o->color);
                        float colorDiffAffect = ((colorDiff/sqrtf(3))*2)-1;
                        V2 dpEffect =
                            (d/(posDist*posDist))*(colorDiffAffect*0.1);
                        e->dp = e->dp + dpEffect;
                        o->dp = o->dp - dpEffect;
                    }
                }
            }
            renderEntity(renderer, e);
        }
    }
    return tookPerf;
}
