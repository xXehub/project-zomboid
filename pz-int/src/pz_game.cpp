// ============================================================================
// pz_game.cpp - JNI bridge between pz-int and the Project Zomboid JVM.
//
// ProjectZomboid64.exe hosts a Java 25 JVM (jvm.dll is loaded in-process).
// The bridge grabs that JVM via JNI_GetCreatedJavaVMs, attaches its render
// thread, and implements every function declared in pz_game.h.
//
// All JNI signatures below were verified with javap against
// C:\games\Project Zomboid\Project Zomboid\projectzomboid.jar (Build 42,
// Java 25 classes, class file version 69).
//
// Threading: every call runs on the render thread that attached itself
// once at startup. PZ's game loop is single-threaded on the main thread;
// reading entity positions from another attached thread is the same
// pattern external tools use via JMX and has been stable here, but any
// concurrent mutation (AddItem etc.) carries a small risk. Mutating calls
// are therefore fire-and-forget single calls, never loops.
// ============================================================================

#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include <Windows.h>
#include <jni.h>

#include <algorithm>
#include <array>
#include <atomic>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <mutex>
#include <string>
#include <vector>

#include "pz_game.h"
#include "projection.h"
#include "feature_policy.h"

// ============================================================================
// logging (mirrors main_entry.cpp pzlog, separate copy for the bridge)
// ============================================================================

namespace pzlog2 {

    inline FILE* g_file = nullptr;

    inline void open()
    {
        if (g_file) return;
        wchar_t temp[MAX_PATH]{};
        const auto len = ::GetTempPathW(MAX_PATH, temp);
        const std::wstring base = (len > 0 && len < MAX_PATH)
            ? std::wstring(temp) : std::wstring(L".\\");
        ::_wfopen_s(&g_file, (base + L"pzint.log").c_str(), L"at");
    }

    inline void log(const char* fmt, ...)
    {
        char buf[1024];
        va_list ap;
        va_start(ap, fmt);
        ::_vsnprintf_s(buf, sizeof(buf), _TRUNCATE, fmt, ap);
        va_end(ap);

        SYSTEMTIME st{};
        ::GetLocalTime(&st);

        char line[1280];
        ::_snprintf_s(line, sizeof(line), _TRUNCATE,
            "[%02u:%02u:%02u.%03u] [bridge] %s\n",
            st.wHour, st.wMinute, st.wSecond, st.wMilliseconds, buf);

        ::OutputDebugStringA(line);
        open();
        if (g_file) { std::fputs(line, g_file); std::fflush(g_file); }
    }

    // One-shot errors: log a given message only once per process.
    inline void once(const char* key, const char* fmt, ...)
    {
        static std::mutex m;
        static std::vector<std::string> seen;
        std::lock_guard<std::mutex> lk(m);
        for (const auto& s : seen) if (s == key) return;
        seen.push_back(key);

        va_list ap;
        va_start(ap, fmt);
        char buf[1024];
        ::_vsnprintf_s(buf, sizeof(buf), _TRUNCATE, fmt, ap);
        va_end(ap);
        log("%s", buf);
    }

} // namespace pzlog2

// ============================================================================
// JNI plumbing
// ============================================================================

namespace pzj {

    // The game's JVM. Resolved lazily from jvm.dll which ProjectZomboid64
    // loads into its own process.
    static JavaVM* g_vm = nullptr;
    static JNIEnv* g_env = nullptr;       // render-thread env (attached)
    static bool    g_attach_failed = false;
    static bool    g_attached_here = false;

    // Class/method cache. Filled once after the first successful attach.
    struct classes {
        jclass isoPlayer{};
        jclass isoZombie{};
        jclass isoGameCharacter{};
        jclass isoMovingObject{};
        jclass isoWorld{};
        jclass isoCell{};
        jclass isoGridSquare{};
        jclass isoCamera{};
        jclass gameClient{};
        jclass climateManager{};
        jclass climateFloat{};
        jclass scriptManager{};
        jclass itemScript{};
        jclass inventoryItem{};
        jclass handWeapon{};
        jclass itemFactory{};
        jclass bodyDamage{};
        jclass bodyPart{};
        jclass bodyPartType{};
        jclass stats{};
        jclass characterStat{};
        jclass systemDisabler{};
        jclass core{};
        // vehicle / animal / ground-item ESP
        jclass vehicleManager{};
        jclass baseVehicle{};
        jclass isoAnimal{};
        jclass worldInventoryObject{};
        jclass isoUtils{};
        jclass playerCheats{};
        jclass cheatType{};
        jclass enumSet{};
        jclass perkFactory{};
        jclass worldMapVisited{};
    } g_cls;

    struct methods {
        // IsoPlayer / IsoGameCharacter
        jmethodID player_getInstance{};
        jmethodID player_getPlayerNum{};
        jmethodID char_getX{};
        jmethodID char_getY{};
        jmethodID char_getZ{};
        jmethodID char_getSquare{};
        jmethodID char_getName{};
        jmethodID char_getHealth{};
        jmethodID char_getBodyDamage{};
        jmethodID char_getStats{};
        jmethodID char_setHealth{};
        jmethodID char_getMaxWeight{};
        jmethodID char_setMaxWeight{};
        jmethodID char_getPrimaryHandItem{};
        jmethodID handweapon_getMaxAmmo{};
        jfieldID char_invincible{};
        jfieldID char_maxWeightBase{};
        jmethodID char_isFallOnFront{};
        jmethodID char_isSitOnGround{};
        jmethodID char_isKnockedDown{};
        jmethodID zombie_isCrawling{};
        jmethodID zombie_isSitAgainstWall{};
        jmethodID char_setForwardDirection{};
        jmethodID player_isAiming{};
        jfieldID perkfactory_list{};
        jmethodID perkfactory_getName{};
        jmethodID char_getPerkLevel{};
        jmethodID char_setPerkLevelDebug{};
        jmethodID weapon_getHitChance{};
        jmethodID weapon_setHitChance{};
        jmethodID weapon_getRecoilDelay{};
        jmethodID weapon_setRecoilDelay{};
        jmethodID weapon_getAimingTime{};
        jmethodID weapon_setAimingTime{};
        jmethodID weapon_getProjectileSpread{};
        jmethodID weapon_setProjectileSpread{};
        jmethodID weapon_getCriticalChance{};
        jmethodID weapon_setCriticalChance{};
        jmethodID weapon_getMinDamage{};
        jmethodID weapon_setMinDamage{};
        jmethodID weapon_getMaxDamage{};
        jmethodID weapon_setMaxDamage{};
        jmethodID char_getCheats{};
        jfieldID playercheats_cheats{};
        jmethodID enumset_add{};
        jmethodID enumset_remove{};
        jmethodID enumset_contains{};
        jfieldID cheat_god{};
        jfieldID cheat_invisible{};
        jfieldID cheat_noclip{};
        jfieldID cheat_carry{};
        jfieldID cheat_endurance{};
        jfieldID cheat_instant_actions{};
        jfieldID cheat_always_day{};
        jfieldID cheat_ammo{};
        jmethodID worldmap_getInstance{};
        jmethodID worldmap_getMinX{};
        jmethodID worldmap_getMinY{};
        jfieldID worldmap_maxX{};
        jfieldID worldmap_maxY{};
        jmethodID worldmap_setKnownInCells{};
        jmethodID worldmap_setVisitedInCells{};
        jfieldID core_debug{};
        jfieldID player_accessLevel{};
        jfieldID stat_fatigue{};
        jfieldID stat_boredom{};
        jfieldID stat_panic{};
        jfieldID stat_wetness{};
        jfieldID stat_endurance{};

        // IsoZombie / IsoWorld / IsoCell
        jmethodID zombie_getTarget{};
        jmethodID zombie_setTarget{};
        jfieldID world_instance{};
        jfieldID world_currentCell{};
        jmethodID cell_getZombieList{};

        // GameClient
        jfieldID gameclient_instance{};
        jfieldID gameclient_client{};
        jmethodID gameclient_getPlayers{};

        // IsoCamera / Core
        jmethodID cam_getOffX{};
        jmethodID cam_getOffY{};
        jmethodID core_getInstance{};
        jmethodID core_getScreenWidth{};
        jmethodID core_getScreenHeight{};
        jfieldID core_tileScale{};
        jmethodID core_getZoom{};

        // ClimateManager / ClimateFloat
        jmethodID climate_getInstance{};
        jmethodID climate_getFloat{};
        jfieldID climate_desaturationMember{};
        jfieldID climate_globalLightIntensityMember{};
        jfieldID climate_nightStrengthMember{};
        jfieldID climate_ambientMember{};
        jfieldID climate_viewDistanceMember{};
        jfieldID climate_dayLightStrengthMember{};
        jfieldID climate_override{};
        jfieldID climate_interpolate{};
        jfieldID climate_isOverride{};
        // ClimateFloat proper method API (Build 42)
        jmethodID climatefloat_setOverride{};      // (FF)V - value, interpolation
        jmethodID climatefloat_setEnableOverride{}; // (Z)V
        jmethodID climatefloat_setFinalValue{};     // (F)V

        jfieldID climate_isOverrideValue{};
        jfieldID climate_finalValue{};

        // Stats / CharacterStat / SystemDisabler
        jmethodID stats_set{};
        jfieldID stat_hunger{};
        jfieldID stat_thirst{};
        jfieldID system_zombiesDontAttack{};

        // ScriptManager / item scripts
        jfieldID scriptman_instance{};
        jmethodID scriptman_getAllItems{};
        jmethodID item_getFullName{};
        jmethodID item_getDisplayName{};

        // InventoryItem / factory
        jmethodID inv_setCurrentAmmoCount{};
        jmethodID inv_setCondition{};
        jmethodID factory_CreateItem_str_f{};
        jmethodID factory_CreateItem_str{};

        // BodyDamage
        jmethodID bodydamage_RestoreToFullHealth{};
        jmethodID bodydamage_setOverallBodyHealth{};


        // BodyDamage - disease/infection clearing
        jmethodID bodydamage_setInfected{};
        jmethodID bodydamage_setIsFakeInfected{};
        jmethodID bodydamage_setInfectionLevel{};
        jmethodID bodydamage_setInfectionTime{};
        jmethodID bodydamage_setHasACold{};
        jmethodID bodydamage_setColdStrength{};
        jmethodID bodydamage_setCatchACold{};
        jmethodID bodydamage_getBodyParts{};

        // BodyPart - per-limb wound clearing
        jmethodID bodypart_SetInfected{};
        jmethodID bodypart_SetFakeInfected{};
        jmethodID bodypart_setBleeding{};
        jmethodID bodypart_setDeepWounded{};
        jmethodID bodypart_setScratched{};
        jmethodID bodypart_setInfectedWound{};
        jmethodID bodypart_setWoundInfectionLevel{};
        jmethodID bodypart_setHaveGlass{};
        jmethodID bodypart_setHaveBullet{};
        jmethodID bodypart_setNeedBurnWash{};
        jmethodID bodypart_setBurnTime{};

        // CharacterStat - disease stats
        jfieldID stat_sickness{};
        jfieldID stat_pain{};
        jfieldID stat_food_sickness{};
        jfieldID stat_poison{};
        jfieldID stat_zombie_fever{};
        jfieldID stat_stress{};
        jfieldID stat_unhappiness{};

        // IsoUtils - use game's own projection (guaranteed correct at any zoom)
        jmethodID isoutils_XToScreenExact{};
        jmethodID isoutils_YToScreenExact{};
        // java.util.List
        jmethodID list_size{};
        jmethodID list_get{};

        // Vehicle ESP
        jfieldID vehiclemanager_instance{};
        jmethodID vehiclemanager_getVehicles{};
        jmethodID vehicle_getScriptName{};
        jmethodID vehicle_getEngineCondition{};
        jmethodID vehicle_getCurrentSpeedKmHour{};

        // Cell / square entity and world-item access
        jmethodID world_getCell{};
        jmethodID cell_getAnimals{};
        jmethodID square_getRadius{};
        jmethodID square_getWorldObjects{};
        jmethodID square_addWorldItem_str{};
        jmethodID square_addWorldItem_obj{};

        // Animal ESP
        jmethodID animal_getAnimalType{};
        jmethodID animal_getHealth{};

        // Ground item ESP
        jmethodID worldinvobj_getItem{};
        jmethodID worldinvobj_getWorldPosX{};
        jmethodID worldinvobj_getWorldPosY{};
        jmethodID worldinvobj_getWorldPosZ{};
        jmethodID invitem_getDisplayName{};
    } g_m;

    // Resolve the List methods once from the ArrayList the game hands us.
    inline bool ensure_list_methods()
    {
        if (g_m.list_size && g_m.list_get) return true;
        jclass list_cls = g_env->FindClass("java/util/List");
        if (!list_cls) { g_env->ExceptionClear(); return false; }
        g_m.list_size = g_env->GetMethodID(list_cls, "size", "()I");
        g_m.list_get  = g_env->GetMethodID(list_cls, "get", "(I)Ljava/lang/Object;");
        g_env->DeleteLocalRef(list_cls);
        if (g_env->ExceptionCheck()) { g_env->ExceptionClear(); g_m.list_size = g_m.list_get = nullptr; return false; }
        return g_m.list_size && g_m.list_get;
    }

    static bool g_resolved = false;
    static jobject g_class_loader = nullptr;
    static jmethodID g_load_class = nullptr;

    // ---- helpers -------------------------------------------------------------

    inline JNIEnv* env() { return g_env; }

    inline bool jstr_to_utf8(jstring s, char* out, std::size_t cap)
    {
        if (!s || !out || cap == 0) return false;
        const char* utf = g_env->GetStringUTFChars(s, nullptr);
        if (!utf) { out[0] = '\0'; return false; }
        ::strncpy_s(out, cap, utf, _TRUNCATE);
        g_env->ReleaseStringUTFChars(s, utf);
        return true;
    }

    inline jstring utf8_to_jstr(const char* s)
    {
        return g_env->NewStringUTF(s ? s : "");
    }

    // Native-attached threads have no defining Java class, so FindClass only
    // sees bootstrap classes. Load game classes through the thread context
    // loader and retain global refs for the lifetime of the bridge.
    inline jclass fc(const char* name)
    {
        if (!g_class_loader || !g_load_class || !name) return nullptr;

        std::string binary_name{name};
        std::replace(binary_name.begin(), binary_name.end(), '/', '.');
        const auto java_name = g_env->NewStringUTF(binary_name.c_str());
        if (!java_name) return nullptr;
        const auto local = static_cast<jclass>(
            g_env->CallObjectMethod(g_class_loader, g_load_class, java_name));
        g_env->DeleteLocalRef(java_name);
        if (g_env->ExceptionCheck() || !local) {
            g_env->ExceptionClear();
            pzlog2::once(name, "class loader failed: %s", name);
            return nullptr;
        }
        const auto global = static_cast<jclass>(g_env->NewGlobalRef(local));
        g_env->DeleteLocalRef(local);
        return global;
    }


    inline jmethodID sm(jclass c, const char* name, const char* sig)
    {
        if (!c) return nullptr;
        jmethodID m = g_env->GetStaticMethodID(c, name, sig);
        if (!m) {
            pzlog2::once(name, "GetStaticMethodID failed: %s %s", name, sig);
            g_env->ExceptionClear();
        }
        return m;
    }

    inline jmethodID gm(jclass c, const char* name, const char* sig)
    {
        if (!c) return nullptr;
        jmethodID m = g_env->GetMethodID(c, name, sig);
        if (!m) {
            pzlog2::once(name, "GetMethodID failed: %s %s", name, sig);
            g_env->ExceptionClear();
        }
        return m;
    }

    inline jfieldID sf(jclass c, const char* name, const char* sig)
    {
        if (!c) return nullptr;
        jfieldID f = g_env->GetStaticFieldID(c, name, sig);
        if (!f) {
            pzlog2::once(name, "GetStaticFieldID failed: %s %s", name, sig);
            g_env->ExceptionClear();
        }
        return f;
    }

    inline jfieldID if_(jclass c, const char* name, const char* sig)
    {
        if (!c) return nullptr;
        jfieldID f = g_env->GetFieldID(c, name, sig);
        if (!f) {
            pzlog2::once(name, "GetFieldID failed: %s %s", name, sig);
            g_env->ExceptionClear();
        }
        return f;
    }

    // ---- init ------------------------------------------------------------------

    // Try to grab the JVM created by the game and retain a usable game class
    // loader. Safe to call repeatedly while the game is still booting.
    static bool ensure_attached()
    {
        if (g_env && g_class_loader && g_load_class) return true;
        if (g_attach_failed) return false;

        if (!g_vm) {
            const HMODULE jvm = ::GetModuleHandleW(L"jvm.dll");
            if (!jvm) return false;

            using get_created_vms_fn = jint(JNICALL*)(JavaVM**, jsize, jsize*);
            const auto get_created_vms = reinterpret_cast<get_created_vms_fn>(
                reinterpret_cast<void*>(::GetProcAddress(
                    jvm, "JNI_GetCreatedJavaVMs")));
            if (!get_created_vms) {
                g_attach_failed = true;
                pzlog2::log("JNI_GetCreatedJavaVMs not found in jvm.dll");
                return false;
            }

            JavaVM* vms[2]{};
            jsize count = 0;
            if (get_created_vms(vms, 2, &count) != JNI_OK ||
                count < 1 || !vms[0]) {
                pzlog2::once("novm", "no created Java VM found (n=%d)",
                    static_cast<int>(count));
                return false;
            }
            g_vm = vms[0];
            pzlog2::log("JavaVM acquired %p", static_cast<void*>(g_vm));
        }

        JNIEnv* env = nullptr;
        const auto get_env = g_vm->GetEnv(
            reinterpret_cast<void**>(&env), JNI_VERSION_1_6);
        if (get_env == JNI_EDETACHED) {
            JavaVMAttachArgs args{ JNI_VERSION_1_6, nullptr, nullptr };
            const auto attach = g_vm->AttachCurrentThreadAsDaemon(
                reinterpret_cast<void**>(&env), &args);
            if (attach != JNI_OK || !env) {
                pzlog2::once("attachfail",
                    "AttachCurrentThreadAsDaemon failed (%d)",
                    static_cast<int>(attach));
                g_attach_failed = true;
                return false;
            }
            g_attached_here = true;
        } else if (get_env != JNI_OK || !env) {
            pzlog2::once("getenvfail", "JavaVM::GetEnv failed (%d)",
                static_cast<int>(get_env));
            g_attach_failed = true;
            return false;
        }
        g_env = env;

        const auto thread_class = g_env->FindClass("java/lang/Thread");
        const auto loader_class = g_env->FindClass("java/lang/ClassLoader");
        if (!thread_class || !loader_class) {
            g_env->ExceptionClear();
            if (thread_class) g_env->DeleteLocalRef(thread_class);
            if (loader_class) g_env->DeleteLocalRef(loader_class);
            pzlog2::once("bootstraploader",
                "bootstrap class loader classes unavailable; retrying");
            return false;
        }
        const auto current_thread = g_env->GetStaticMethodID(
            thread_class, "currentThread", "()Ljava/lang/Thread;");
        const auto get_context_loader = g_env->GetMethodID(
            thread_class, "getContextClassLoader", "()Ljava/lang/ClassLoader;");
        g_load_class = g_env->GetMethodID(
            loader_class, "loadClass", "(Ljava/lang/String;)Ljava/lang/Class;");
        const auto thread = current_thread
            ? g_env->CallStaticObjectMethod(thread_class, current_thread) : nullptr;
        auto loader = thread && get_context_loader
            ? g_env->CallObjectMethod(thread, get_context_loader) : nullptr;
        if (!loader) {
            g_env->ExceptionClear();
            const auto get_system_loader = g_env->GetStaticMethodID(
                loader_class, "getSystemClassLoader", "()Ljava/lang/ClassLoader;");
            loader = get_system_loader
                ? g_env->CallStaticObjectMethod(loader_class, get_system_loader)
                : nullptr;
        }
        if (g_env->ExceptionCheck() || !loader || !g_load_class) {
            g_env->ExceptionClear();
            if (thread) g_env->DeleteLocalRef(thread);
            if (loader) g_env->DeleteLocalRef(loader);
            g_env->DeleteLocalRef(loader_class);
            g_env->DeleteLocalRef(thread_class);
            g_load_class = nullptr;
            pzlog2::once("gameloader",
                "game class loader unavailable; retrying");
            return false;
        }
        g_class_loader = g_env->NewGlobalRef(loader);
        if (thread) g_env->DeleteLocalRef(thread);
        g_env->DeleteLocalRef(loader);
        g_env->DeleteLocalRef(loader_class);
        g_env->DeleteLocalRef(thread_class);
        if (!g_class_loader) {
            g_attach_failed = true;
            return false;
        }

        pzlog2::log("render thread JVM env and game class loader acquired");
        return true;
    }

    struct jframe {
        bool ok{ false };

        explicit jframe(jint capacity = 512)
            : ok(g_env && g_env->PushLocalFrame(capacity) == JNI_OK) {}

        ~jframe()
        {
            if (ok) g_env->PopLocalFrame(nullptr);
        }

        jframe(const jframe&) = delete;
        jframe& operator=(const jframe&) = delete;
    };

    static bool ensure_resolved()
    {
        if (g_resolved) return true;
        if (!ensure_attached()) return false;

        auto& c = g_cls;
        auto& m = g_m;

        c.isoPlayer = fc("zombie/characters/IsoPlayer");
        c.isoZombie = fc("zombie/characters/IsoZombie");
        c.isoGameCharacter = fc("zombie/characters/IsoGameCharacter");
        c.isoMovingObject = fc("zombie/iso/IsoMovingObject");
        c.isoWorld = fc("zombie/iso/IsoWorld");
        c.isoCell = fc("zombie/iso/IsoCell");
        c.isoGridSquare = fc("zombie/iso/IsoGridSquare");
        c.isoCamera = fc("zombie/iso/IsoCamera");
        c.gameClient = fc("zombie/network/GameClient");
        c.climateManager = fc("zombie/iso/weather/ClimateManager");
        c.climateFloat = fc("zombie/iso/weather/ClimateManager$ClimateFloat");
        c.scriptManager = fc("zombie/scripting/ScriptManager");
        c.itemScript = fc("zombie/scripting/objects/Item");
        c.inventoryItem = fc("zombie/inventory/InventoryItem");
        c.handWeapon = fc("zombie/inventory/types/HandWeapon");
        c.itemFactory = fc("zombie/inventory/InventoryItemFactory");
        c.bodyDamage = fc("zombie/characters/BodyDamage/BodyDamage");
        c.stats = fc("zombie/characters/Stats");
        c.characterStat = fc("zombie/characters/CharacterStat");
        c.systemDisabler = fc("zombie/SystemDisabler");
        c.core = fc("zombie/core/Core");

        c.bodyPart = fc("zombie/characters/BodyDamage/BodyPart");
        c.bodyPartType = fc("zombie/characters/BodyDamage/BodyPartType");
        c.isoUtils = fc("zombie/iso/IsoUtils");

        // Vehicle / animal / ground-item ESP classes
        c.vehicleManager = fc("zombie/vehicles/VehicleManager");
        c.baseVehicle = fc("zombie/vehicles/BaseVehicle");
        c.isoAnimal = fc("zombie/characters/animals/IsoAnimal");
        c.worldInventoryObject = fc("zombie/iso/objects/IsoWorldInventoryObject");
        c.playerCheats = fc("zombie/characters/PlayerCheats");
        c.cheatType = fc("zombie/characters/CheatType");
        c.enumSet = fc("java/util/EnumSet");
        c.perkFactory = fc("zombie/characters/skills/PerkFactory");
        c.worldMapVisited = fc("zombie/worldMap/WorldMapVisited");
        m.player_getInstance = sm(c.isoPlayer, "getInstance",
            "()Lzombie/characters/IsoPlayer;");
        m.player_getPlayerNum = gm(c.isoPlayer, "getPlayerNum", "()I");
        m.char_getX = gm(c.isoGameCharacter, "getX", "()F");
        m.char_getY = gm(c.isoGameCharacter, "getY", "()F");
        m.char_getZ = gm(c.isoGameCharacter, "getZ", "()F");
        m.char_getSquare = gm(c.isoMovingObject, "getSquare",
            "()Lzombie/iso/IsoGridSquare;");
        m.char_getName = gm(c.isoPlayer, "getDisplayName",
            "()Ljava/lang/String;");
        m.char_getHealth = gm(c.isoGameCharacter, "getHealth", "()F");
        m.char_getBodyDamage = gm(c.isoGameCharacter, "getBodyDamage",
            "()Lzombie/characters/BodyDamage/BodyDamage;");
        m.char_getStats = gm(c.isoGameCharacter, "getStats",
            "()Lzombie/characters/Stats;");
        m.char_setHealth = gm(c.isoGameCharacter, "setHealth", "(F)V");
        m.char_getMaxWeight = gm(c.isoGameCharacter, "getMaxWeight", "()I");
        m.char_setMaxWeight = gm(c.isoGameCharacter, "setMaxWeight", "(I)V");
        m.char_maxWeightBase = if_(c.isoGameCharacter, "maxWeightBase", "I");
        m.char_getPrimaryHandItem = gm(c.isoGameCharacter, "getPrimaryHandItem",
            "()Lzombie/inventory/InventoryItem;");
        m.handweapon_getMaxAmmo = gm(c.handWeapon, "getMaxAmmo", "()I");
        m.char_invincible = if_(c.isoGameCharacter, "invincible", "Z");
        m.char_isFallOnFront = gm(c.isoGameCharacter, "isFallOnFront", "()Z");
        m.char_isSitOnGround = gm(c.isoGameCharacter, "isSitOnGround", "()Z");
        m.char_isKnockedDown = gm(c.isoGameCharacter, "isKnockedDown", "()Z");
        m.zombie_isCrawling = gm(c.isoZombie, "isCrawling", "()Z");
        m.zombie_isSitAgainstWall = gm(c.isoZombie, "isSitAgainstWall", "()Z");
        m.char_setForwardDirection = gm(c.isoGameCharacter,
            "setForwardDirection", "(FF)V");
        m.player_isAiming = gm(c.isoPlayer, "isAiming", "()Z");
        m.perkfactory_list = sf(c.perkFactory, "PerkList", "Ljava/util/ArrayList;");
        m.perkfactory_getName = sm(c.perkFactory, "getPerkName",
            "(Lzombie/characters/skills/PerkFactory$Perk;)Ljava/lang/String;");
        m.char_getPerkLevel = gm(c.isoGameCharacter, "getPerkLevel",
            "(Lzombie/characters/skills/PerkFactory$Perk;)I");
        m.char_setPerkLevelDebug = gm(c.isoGameCharacter, "setPerkLevelDebug",
            "(Lzombie/characters/skills/PerkFactory$Perk;I)V");

        m.weapon_getHitChance = gm(c.handWeapon, "getHitChance", "()I");
        m.weapon_setHitChance = gm(c.handWeapon, "setHitChance", "(I)V");
        m.weapon_getRecoilDelay = gm(c.handWeapon, "getRecoilDelay", "()I");
        m.weapon_setRecoilDelay = gm(c.handWeapon, "setRecoilDelay", "(I)V");
        m.weapon_getAimingTime = gm(c.handWeapon, "getAimingTime", "()I");
        m.weapon_setAimingTime = gm(c.handWeapon, "setAimingTime", "(I)V");
        m.weapon_getProjectileSpread = gm(c.handWeapon,
            "getProjectileSpread", "()F");
        m.weapon_setProjectileSpread = gm(c.handWeapon,
            "setProjectileSpread", "(F)V");
        m.weapon_getCriticalChance = gm(c.handWeapon,
            "getCriticalChance", "()F");
        m.weapon_setCriticalChance = gm(c.handWeapon,
            "setCriticalChance", "(F)V");
        m.weapon_getMinDamage = gm(c.handWeapon, "getMinDamage", "()F");
        m.weapon_setMinDamage = gm(c.handWeapon, "setMinDamage", "(F)V");
        m.weapon_getMaxDamage = gm(c.handWeapon, "getMaxDamage", "()F");
        m.weapon_setMaxDamage = gm(c.handWeapon, "setMaxDamage", "(F)V");

        m.char_getCheats = gm(c.isoGameCharacter, "getCheats",
            "()Lzombie/characters/PlayerCheats;");
        m.playercheats_cheats = if_(c.playerCheats, "cheats",
            "Ljava/util/EnumSet;");
        m.enumset_add = gm(c.enumSet, "add", "(Ljava/lang/Object;)Z");
        m.enumset_remove = gm(c.enumSet, "remove", "(Ljava/lang/Object;)Z");
        m.enumset_contains = gm(c.enumSet, "contains", "(Ljava/lang/Object;)Z");
        constexpr auto cheat_type_sig = "Lzombie/characters/CheatType;";
        m.cheat_god = sf(c.cheatType, "GOD_MODE", cheat_type_sig);
        m.cheat_invisible = sf(c.cheatType, "INVISIBLE", cheat_type_sig);
        m.cheat_noclip = sf(c.cheatType, "NO_CLIP", cheat_type_sig);
        m.cheat_carry = sf(c.cheatType, "UNLIMITED_CARRY", cheat_type_sig);
        m.cheat_endurance = sf(c.cheatType, "UNLIMITED_ENDURANCE", cheat_type_sig);
        m.cheat_instant_actions = sf(c.cheatType,
            "TIMED_ACTION_INSTANT", cheat_type_sig);
        m.cheat_always_day = sf(c.cheatType, "ALWAYS_DAY", cheat_type_sig);
        m.cheat_ammo = sf(c.cheatType, "UNLIMITED_AMMO", cheat_type_sig);
        m.worldmap_getInstance = sm(c.worldMapVisited, "getInstance",
            "()Lzombie/worldMap/WorldMapVisited;");
        m.worldmap_getMinX = gm(c.worldMapVisited, "getMinX", "()I");
        m.worldmap_getMinY = gm(c.worldMapVisited, "getMinY", "()I");
        m.worldmap_maxX = if_(c.worldMapVisited, "maxX", "I");
        m.worldmap_maxY = if_(c.worldMapVisited, "maxY", "I");
        m.worldmap_setKnownInCells = gm(c.worldMapVisited,
            "setKnownInCells", "(IIII)V");
        m.worldmap_setVisitedInCells = gm(c.worldMapVisited,
            "setVisitedInCells", "(IIII)V");
        m.core_debug = sf(c.core, "debug", "Z");
        m.player_accessLevel = if_(c.isoPlayer, "accessLevel",
            "Ljava/lang/String;");
        m.stat_fatigue = sf(c.characterStat, "FATIGUE",
            "Lzombie/characters/CharacterStat;");
        m.stat_boredom = sf(c.characterStat, "BOREDOM",
            "Lzombie/characters/CharacterStat;");
        m.stat_panic = sf(c.characterStat, "PANIC",
            "Lzombie/characters/CharacterStat;");
        m.stat_wetness = sf(c.characterStat, "WETNESS",
            "Lzombie/characters/CharacterStat;");
        m.stat_endurance = sf(c.characterStat, "ENDURANCE",
            "Lzombie/characters/CharacterStat;");

        m.zombie_getTarget = gm(c.isoZombie, "getTarget",
            "()Lzombie/iso/IsoMovingObject;");
        m.zombie_setTarget = gm(c.isoZombie, "setTarget",
            "(Lzombie/iso/IsoMovingObject;)V");
        m.world_instance = sf(c.isoWorld, "instance", "Lzombie/iso/IsoWorld;");
        m.world_currentCell = if_(c.isoWorld, "currentCell", "Lzombie/iso/IsoCell;");
        m.cell_getZombieList = gm(c.isoCell, "getZombieList",
            "()Ljava/util/ArrayList;");

        m.gameclient_instance = sf(c.gameClient, "instance",
            "Lzombie/network/GameClient;");
        m.gameclient_client = sf(c.gameClient, "client", "Z");
        m.gameclient_getPlayers = gm(c.gameClient, "getPlayers",
            "()Ljava/util/ArrayList;");

        m.cam_getOffX = sm(c.isoCamera, "getOffX", "(I)F");
        m.cam_getOffY = sm(c.isoCamera, "getOffY", "(I)F");
        m.core_getInstance = sm(c.core, "getInstance", "()Lzombie/core/Core;");
        m.core_getScreenWidth = gm(c.core, "getScreenWidth", "()I");
        m.core_getScreenHeight = gm(c.core, "getScreenHeight", "()I");
        m.core_tileScale = sf(c.core, "tileScale", "I");

        constexpr auto climate_float_sig =
            "Lzombie/iso/weather/ClimateManager$ClimateFloat;";
        m.core_getZoom = gm(c.core, "getZoom", "(I)F");
        m.climate_getInstance = sm(c.climateManager, "getInstance",
            "()Lzombie/iso/weather/ClimateManager;");
        m.climate_getFloat = gm(c.climateManager, "getClimateFloat",
            "(I)Lzombie/iso/weather/ClimateManager$ClimateFloat;");
        m.climate_desaturationMember = if_(c.climateManager,
            "desaturation", climate_float_sig);
        m.climate_globalLightIntensityMember = if_(c.climateManager,
            "globalLightIntensity", climate_float_sig);
        m.climate_nightStrengthMember = if_(c.climateManager,
            "nightStrength", climate_float_sig);
        m.climate_ambientMember = if_(c.climateManager,
            "ambient", climate_float_sig);
        m.climate_viewDistanceMember = if_(c.climateManager,
            "viewDistance", climate_float_sig);
        m.climate_dayLightStrengthMember = if_(c.climateManager,
            "dayLightStrength", climate_float_sig);
        m.climate_override = if_(c.climateFloat, "override", "F");
        m.climate_interpolate = if_(c.climateFloat, "interpolate", "F");
        m.climate_isOverride = if_(c.climateFloat, "isOverride", "Z");
        m.climate_isOverrideValue = if_(c.climateFloat,
            "isOverrideValue", "Z");

        // ClimateFloat proper method API (Build 42 uses these, not raw fields)
        m.climatefloat_setOverride = gm(c.climateFloat, "setOverride", "(FF)V");
        m.climatefloat_setEnableOverride = gm(c.climateFloat, "setEnableOverride", "(Z)V");
        m.climatefloat_setFinalValue = gm(c.climateFloat, "setFinalValue", "(F)V");
        m.climate_finalValue = if_(c.climateFloat, "finalValue", "F");

        m.stats_set = gm(c.stats, "set",
            "(Lzombie/characters/CharacterStat;F)Z");
        m.stat_hunger = sf(c.characterStat, "HUNGER",
            "Lzombie/characters/CharacterStat;");
        m.stat_thirst = sf(c.characterStat, "THIRST",
            "Lzombie/characters/CharacterStat;");
        m.system_zombiesDontAttack = sf(c.systemDisabler,
            "zombiesDontAttack", "Z");

        m.scriptman_instance = sf(c.scriptManager, "instance",
            "Lzombie/scripting/ScriptManager;");
        m.scriptman_getAllItems = gm(c.scriptManager, "getAllItems",
            "()Ljava/util/ArrayList;");
        m.item_getFullName = gm(c.itemScript, "getFullName",
            "()Ljava/lang/String;");
        m.item_getDisplayName = gm(c.itemScript, "getDisplayName",
            "()Ljava/lang/String;");

        m.inv_setCurrentAmmoCount = gm(c.inventoryItem,
            "setCurrentAmmoCount", "(I)V");
        m.inv_setCondition = gm(c.inventoryItem, "setCondition", "(I)V");
        m.factory_CreateItem_str_f = sm(c.itemFactory, "CreateItem",
            "(Ljava/lang/String;F)Lzombie/inventory/InventoryItem;");
        m.factory_CreateItem_str = sm(c.itemFactory, "CreateItem",
            "(Ljava/lang/String;)Lzombie/inventory/InventoryItem;");

        m.bodydamage_RestoreToFullHealth = gm(c.bodyDamage,
            "RestoreToFullHealth", "()V");
        m.bodydamage_setOverallBodyHealth = gm(c.bodyDamage,
            "setOverallBodyHealth", "(F)V");


        // BodyDamage disease/infection methods
        m.bodydamage_setInfected = gm(c.bodyDamage, "setInfected", "(Z)V");
        m.bodydamage_setIsFakeInfected = gm(c.bodyDamage, "setIsFakeInfected", "(Z)V");
        m.bodydamage_setInfectionLevel = gm(c.bodyDamage, "setInfectionGrowthRate", "(F)V");
        m.bodydamage_setInfectionTime = gm(c.bodyDamage, "setInfectionTime", "(F)V");
        m.bodydamage_setHasACold = gm(c.bodyDamage, "setHasACold", "(Z)V");
        m.bodydamage_setColdStrength = gm(c.bodyDamage, "setColdStrength", "(F)V");
        m.bodydamage_setCatchACold = gm(c.bodyDamage, "setCatchACold", "(F)V");
        m.bodydamage_getBodyParts = gm(c.bodyDamage, "getBodyParts",
            "()Ljava/util/ArrayList;");


        // IsoUtils projection (static methods)
        m.isoutils_XToScreenExact = sm(c.isoUtils, "XToScreenExact", "(FFFI)F");
        m.isoutils_YToScreenExact = sm(c.isoUtils, "YToScreenExact", "(FFFI)F");
        // BodyPart wound methods
        m.bodypart_SetInfected = gm(c.bodyPart, "SetInfected", "(Z)V");
        m.bodypart_SetFakeInfected = gm(c.bodyPart, "SetFakeInfected", "(Z)V");
        m.bodypart_setBleeding = gm(c.bodyPart, "setBleeding", "(Z)V");
        m.bodypart_setDeepWounded = gm(c.bodyPart, "setDeepWounded", "(Z)V");
        m.bodypart_setScratched = gm(c.bodyPart, "setScratched", "(ZZ)V");
        m.bodypart_setInfectedWound = gm(c.bodyPart, "setInfectedWound", "(Z)V");
        m.bodypart_setWoundInfectionLevel = gm(c.bodyPart, "setWoundInfectionLevel", "(F)V");
        m.bodypart_setHaveGlass = gm(c.bodyPart, "setHaveGlass", "(Z)V");
        m.bodypart_setHaveBullet = gm(c.bodyPart, "setHaveBullet", "(ZI)V");
        m.bodypart_setNeedBurnWash = gm(c.bodyPart, "setNeedBurnWash", "(Z)V");
        m.bodypart_setBurnTime = gm(c.bodyPart, "setBurnTime", "(F)V");

        // Vehicle ESP methods
        m.vehiclemanager_instance = sf(c.vehicleManager, "instance",
            "Lzombie/vehicles/VehicleManager;");
        m.vehiclemanager_getVehicles = gm(c.vehicleManager, "getVehicles",
            "()Ljava/util/ArrayList;");
        m.vehicle_getScriptName = gm(c.baseVehicle, "getScriptName",
            "()Ljava/lang/String;");
        m.vehicle_getEngineCondition = gm(c.baseVehicle, "getEngineCondition", "()I");
        m.vehicle_getCurrentSpeedKmHour = gm(c.baseVehicle, "getCurrentSpeedKmHour", "()F");

        // Cell / square entity and world-item methods
        m.world_getCell = gm(c.isoWorld, "getCell", "()Lzombie/iso/IsoCell;");
        m.cell_getAnimals = gm(c.isoCell, "getAnimals", "()Ljava/util/List;");
        m.square_getRadius = gm(c.isoGridSquare, "getRadius", "(I)Ljava/util/List;");
        m.square_getWorldObjects = gm(c.isoGridSquare, "getWorldObjects",
            "()Ljava/util/ArrayList;");
        m.square_addWorldItem_str = gm(c.isoGridSquare, "AddWorldInventoryItem",
            "(Ljava/lang/String;FFFZZ)Lzombie/inventory/InventoryItem;");
        m.square_addWorldItem_obj = gm(c.isoGridSquare, "AddWorldInventoryItem",
            "(Lzombie/inventory/InventoryItem;FFFZZ)Lzombie/inventory/InventoryItem;");

        // Animal ESP methods
        m.animal_getAnimalType = gm(c.isoAnimal, "getAnimalType",
            "()Ljava/lang/String;");
        m.animal_getHealth = gm(c.isoAnimal, "getHealth", "()F");

        // Ground item ESP methods
        m.worldinvobj_getItem = gm(c.worldInventoryObject, "getItem",
            "()Lzombie/inventory/InventoryItem;");
        m.worldinvobj_getWorldPosX = gm(c.worldInventoryObject, "getWorldPosX", "()F");
        m.worldinvobj_getWorldPosY = gm(c.worldInventoryObject, "getWorldPosY", "()F");
        m.worldinvobj_getWorldPosZ = gm(c.worldInventoryObject, "getWorldPosZ", "()F");
        m.invitem_getDisplayName = gm(c.inventoryItem, "getDisplayName",
            "()Ljava/lang/String;");

        // CharacterStat disease fields
        constexpr auto cs_sig = "Lzombie/characters/CharacterStat;";
        m.stat_sickness = sf(c.characterStat, "SICKNESS", cs_sig);
        m.stat_pain = sf(c.characterStat, "PAIN", cs_sig);
        m.stat_food_sickness = sf(c.characterStat, "FOOD_SICKNESS", cs_sig);
        m.stat_poison = sf(c.characterStat, "POISON", cs_sig);
        m.stat_zombie_fever = sf(c.characterStat, "ZOMBIE_FEVER", cs_sig);
        m.stat_stress = sf(c.characterStat, "STRESS", cs_sig);
        m.stat_unhappiness = sf(c.characterStat, "UNHAPPINESS", cs_sig);
        if (g_env->ExceptionCheck()) g_env->ExceptionClear();
        const bool required_ok = c.isoPlayer && c.isoGameCharacter &&
            c.isoWorld && c.isoCell && c.isoGridSquare && c.isoCamera && c.core &&
            m.player_getInstance && m.char_getX && m.char_getY && m.char_getZ &&
            m.char_getSquare && m.world_instance && m.world_getCell &&
            m.cell_getZombieList && m.cam_getOffX && m.cam_getOffY &&
            m.core_getInstance && m.core_getScreenWidth &&
            m.core_getScreenHeight && m.core_tileScale;
        if (!required_ok) {
            pzlog2::once("resolvefail",
                "required Build 42 JNI members missing");
            return false;
        }

        g_resolved = true;
        pzlog2::log("Build 42 JNI members resolved");
        return true;
    }
} // namespace pzj

// ============================================================================
// pz:: implementation
// ============================================================================

namespace pz {

    using pzj::g_env;
    using pzj::g_cls;
    using pzj::g_m;

    // Cached screen size + zoom + camera offset, refreshed per frame.
    struct frame_ctx {
        int screen_w{ 0 };
        int screen_h{ 0 };
        int player_idx{ 0 };
        int tile_scale{ 1 };
        float cam_off_x{ 0.0f };
        float cam_off_y{ 0.0f };
        float zoom{ 1.0f };
        bool valid{ false };
    };
    static frame_ctx g_frame_ctx;

    float projection_zoom() noexcept
    {
        return g_frame_ctx.valid ? g_frame_ctx.zoom : 0.0f;
    }

    // True once a full world (IsoPlayer.getInstance() non-null) was seen.
    static bool g_seen_world = false;

    bool game_ready()
    {
        if (!pzj::ensure_resolved()) return false;

        // Cheap readiness: local player instance exists.
        pzj::jframe fr;
        if (!fr.ok) return false;

        const auto player = static_cast<jobject>(
            g_env->CallStaticObjectMethod(g_cls.isoPlayer, g_m.player_getInstance));
        if (g_env->ExceptionCheck()) { g_env->ExceptionClear(); return false; }
        if (!player) return false;

        g_seen_world = true;
        return true;
    }

    bool local_player_pos(float& x, float& y, float& z)
    {
        if (!pzj::ensure_resolved()) return false;
        pzj::jframe fr;
        if (!fr.ok) return false;

        const auto player = static_cast<jobject>(
            g_env->CallStaticObjectMethod(g_cls.isoPlayer, g_m.player_getInstance));
        if (g_env->ExceptionCheck()) { g_env->ExceptionClear(); return false; }
        if (!player) return false;

        x = g_env->CallFloatMethod(player, g_m.char_getX);
        y = g_env->CallFloatMethod(player, g_m.char_getY);
        z = g_env->CallFloatMethod(player, g_m.char_getZ);
        if (g_env->ExceptionCheck()) { g_env->ExceptionClear(); return false; }
        return true;
    }

    // Refresh the per-frame projection context (zoom, camera offset,
    // screen size). Must run before projecting entities.
    static bool refresh_frame_ctx()
    {
        auto& c = g_frame_ctx;
        c.valid = false;

        const auto core = static_cast<jobject>(
            g_env->CallStaticObjectMethod(g_cls.core, g_m.core_getInstance));
        if (g_env->ExceptionCheck() || !core) {
            g_env->ExceptionClear();
            return false;
        }

        c.screen_w = g_env->CallIntMethod(core, g_m.core_getScreenWidth);
        c.screen_h = g_env->CallIntMethod(core, g_m.core_getScreenHeight);
        c.tile_scale = g_env->GetStaticIntField(g_cls.core, g_m.core_tileScale);
        if (g_env->ExceptionCheck()) {
            g_env->ExceptionClear();
            return false;
        }

        const auto player = static_cast<jobject>(
            g_env->CallStaticObjectMethod(g_cls.isoPlayer, g_m.player_getInstance));
        if (g_env->ExceptionCheck() || !player) {
            g_env->ExceptionClear();
            return false;
        }
        c.player_idx = g_m.player_getPlayerNum
            ? g_env->CallIntMethod(player, g_m.player_getPlayerNum) : 0;
        if (g_env->ExceptionCheck()) {
            g_env->ExceptionClear();
            c.player_idx = 0;
        }

        c.cam_off_x = g_env->CallStaticFloatMethod(
            g_cls.isoCamera, g_m.cam_getOffX, c.player_idx);
        c.cam_off_y = g_env->CallStaticFloatMethod(
            g_cls.isoCamera, g_m.cam_getOffY, c.player_idx);
        if (g_env->ExceptionCheck()) {
            g_env->ExceptionClear();
            return false;
        }

        c.zoom = g_m.core_getZoom
            ? g_env->CallFloatMethod(core, g_m.core_getZoom, c.player_idx)
            : 1.0f;
        if (g_env->ExceptionCheck() || !std::isfinite(c.zoom) || c.zoom <= 0.0f) {
            g_env->ExceptionClear();
            return false;
        }

        c.valid = c.screen_w > 0 && c.screen_h > 0 && c.tile_scale > 0;
        return c.valid;
    }

    // Project one world position to overlay pixels.
    static void project(float wx, float wy, float wz, float& sx, float& sy,
        bool& on_screen)
    {
        const auto& c = g_frame_ctx;
        sx = sy = 0.0f;
        on_screen = false;
        if (!c.valid) return;
        const float scale = static_cast<float>(c.tile_scale);
        const auto point = projection::from_exact(
            (wx - wy) * 32.0f * scale - c.cam_off_x,
            (wx + wy) * 16.0f * scale - wz * 96.0f * scale - c.cam_off_y,
            c.zoom);
        sx = point.x;
        sy = point.y;
        on_screen = sx > -64.0f && sy > -64.0f &&
            sx < static_cast<float>(c.screen_w) + 64.0f &&
            sy < static_cast<float>(c.screen_h) + 64.0f;
    }

    [[nodiscard]] static float max_projection_distance(const entity_type type) noexcept
    {
        switch (type) {
        case entity_type::vehicle: return 320.0f;
        case entity_type::item: return 120.0f;
        default: return 220.0f;
        }

    }

    // Fill one entity row from a character object (zombie or player).
    static void fill_entity(jobject chr, float lx, float ly, entity_type type,
        entity& e)
    {
        e.type = type;
        e.is_local = false;
        e.wx = g_env->CallFloatMethod(chr, g_m.char_getX);
        e.wy = g_env->CallFloatMethod(chr, g_m.char_getY);
        e.wz = g_env->CallFloatMethod(chr, g_m.char_getZ);
        if (g_env->ExceptionCheck()) {
            g_env->ExceptionClear();
            e.wx = e.wy = e.wz = 0.0f;
        }

        const float dx = e.wx - lx;
        const float dy = e.wy - ly;
        e.dist = std::sqrt(dx * dx + dy * dy);
        if (e.dist > max_projection_distance(type)) {
            e.health = 0.0f;
            e.on_screen = false;
            return;
        }

        if (type == entity_type::zombie) {
            ::strncpy_s(e.name, sizeof(e.name), "Zombie", _TRUNCATE);
        } else {
            const auto name = static_cast<jstring>(
                g_env->CallObjectMethod(chr, g_m.char_getName));
            if (g_env->ExceptionCheck() || !name ||
                !pzj::jstr_to_utf8(name, e.name, sizeof(e.name))) {
                g_env->ExceptionClear();
                ::strncpy_s(e.name, sizeof(e.name), "Player", _TRUNCATE);
            }
        }

        e.health = g_m.char_getHealth
            ? g_env->CallFloatMethod(chr, g_m.char_getHealth) : 0.0f;
        if (g_env->ExceptionCheck()) {
            g_env->ExceptionClear();
            e.health = 0.0f;
        }

        const auto read_pose_flag = [&](jmethodID method) {
            if (!method) return false;
            const jboolean value = g_env->CallBooleanMethod(chr, method);
            if (!g_env->ExceptionCheck()) return value == JNI_TRUE;
            g_env->ExceptionClear();
            return false;
        };
        const bool crawling = type == entity_type::zombie &&
            read_pose_flag(g_m.zombie_isCrawling);
        const bool sitting = read_pose_flag(g_m.char_isSitOnGround) ||
            (type == entity_type::zombie &&
                read_pose_flag(g_m.zombie_isSitAgainstWall));
        const bool on_floor = read_pose_flag(g_m.char_isKnockedDown) ||
            read_pose_flag(g_m.char_isFallOnFront);
        e.pose = crawling ? projection::character_pose::crawling :
            sitting ? projection::character_pose::sitting :
            on_floor ? projection::character_pose::floor :
            projection::character_pose::standing;

        project(e.wx, e.wy, e.wz, e.sx, e.sy, e.on_screen);
    }

    // Fill one entity row from a BaseVehicle.
    static void fill_vehicle(jobject veh, float lx, float ly, entity& e)
    {
        e.type = entity_type::vehicle;
        e.is_local = false;
        // BaseVehicle inherits getX/getY/getZ from IsoMovingObject
        e.wx = g_env->CallFloatMethod(veh, g_m.char_getX);
        e.wy = g_env->CallFloatMethod(veh, g_m.char_getY);
        e.wz = g_env->CallFloatMethod(veh, g_m.char_getZ);
        if (g_env->ExceptionCheck()) {
            g_env->ExceptionClear();
            e.wx = e.wy = e.wz = 0.0f;
        }
        const float dx = e.wx - lx;
        const float dy = e.wy - ly;
        e.dist = std::sqrt(dx * dx + dy * dy);
        if (e.dist > max_projection_distance(e.type)) {
            e.health = 0.0f;
            e.on_screen = false;
            ::strncpy_s(e.name, sizeof(e.name), "Vehicle", _TRUNCATE);
            return;
        }

        if (g_m.vehicle_getScriptName) {
            const auto sn = static_cast<jstring>(
                g_env->CallObjectMethod(veh, g_m.vehicle_getScriptName));
            if (!g_env->ExceptionCheck() && sn)
                pzj::jstr_to_utf8(sn, e.name, sizeof(e.name));
            else {
                g_env->ExceptionClear();
                ::strncpy_s(e.name, sizeof(e.name), "Vehicle", _TRUNCATE);
            }
        } else {
            ::strncpy_s(e.name, sizeof(e.name), "Vehicle", _TRUNCATE);
        }
        e.health = g_m.vehicle_getEngineCondition
            ? static_cast<float>(g_env->CallIntMethod(veh, g_m.vehicle_getEngineCondition))
            : 0.0f;
        if (g_env->ExceptionCheck()) { g_env->ExceptionClear(); e.health = 0.0f; }
        project(e.wx, e.wy, e.wz, e.sx, e.sy, e.on_screen);
    }

    // Fill one entity row from an IsoAnimal.
    static void fill_animal(jobject ani, float lx, float ly, entity& e)
    {
        e.type = entity_type::animal;
        e.is_local = false;
        e.wx = g_env->CallFloatMethod(ani, g_m.char_getX);
        e.wy = g_env->CallFloatMethod(ani, g_m.char_getY);
        e.wz = g_env->CallFloatMethod(ani, g_m.char_getZ);
        if (g_env->ExceptionCheck()) {
            g_env->ExceptionClear();
            e.wx = e.wy = e.wz = 0.0f;
        }
        const float dx = e.wx - lx;
        const float dy = e.wy - ly;
        e.dist = std::sqrt(dx * dx + dy * dy);
        if (e.dist > max_projection_distance(e.type)) {
            e.health = 0.0f;
            e.on_screen = false;
            ::strncpy_s(e.name, sizeof(e.name), "Animal", _TRUNCATE);
            return;
        }

        if (g_m.animal_getAnimalType) {
            const auto at = static_cast<jstring>(
                g_env->CallObjectMethod(ani, g_m.animal_getAnimalType));
            if (!g_env->ExceptionCheck() && at)
                pzj::jstr_to_utf8(at, e.name, sizeof(e.name));
            else {
                g_env->ExceptionClear();
                ::strncpy_s(e.name, sizeof(e.name), "Animal", _TRUNCATE);
            }
        } else {
            ::strncpy_s(e.name, sizeof(e.name), "Animal", _TRUNCATE);
        }
        e.health = g_m.animal_getHealth
            ? g_env->CallFloatMethod(ani, g_m.animal_getHealth) * 100.0f
            : 0.0f;
        if (g_env->ExceptionCheck()) { g_env->ExceptionClear(); e.health = 0.0f; }
        project(e.wx, e.wy, e.wz, e.sx, e.sy, e.on_screen);
    }

    // Fill one entity row from an IsoWorldInventoryObject.
    static void fill_ground_item(jobject obj, float lx, float ly, entity& e)
    {
        e.type = entity_type::item;
        e.is_local = false;
        e.health = 0.0f;
        if (g_m.worldinvobj_getWorldPosX && g_m.worldinvobj_getWorldPosY &&
            g_m.worldinvobj_getWorldPosZ) {
            e.wx = g_env->CallFloatMethod(obj, g_m.worldinvobj_getWorldPosX);
            e.wy = g_env->CallFloatMethod(obj, g_m.worldinvobj_getWorldPosY);
            e.wz = g_env->CallFloatMethod(obj, g_m.worldinvobj_getWorldPosZ);
        } else {
            e.wx = g_env->CallFloatMethod(obj, g_m.char_getX);
            e.wy = g_env->CallFloatMethod(obj, g_m.char_getY);
            e.wz = g_env->CallFloatMethod(obj, g_m.char_getZ);
        }
        if (g_env->ExceptionCheck()) {
            g_env->ExceptionClear();
            e.wx = e.wy = e.wz = 0.0f;
        }
        bool named = false;
        if (g_m.worldinvobj_getItem && g_m.invitem_getDisplayName) {
            const auto itm = static_cast<jobject>(
                g_env->CallObjectMethod(obj, g_m.worldinvobj_getItem));
            if (!g_env->ExceptionCheck() && itm) {
                const auto dn = static_cast<jstring>(
                    g_env->CallObjectMethod(itm, g_m.invitem_getDisplayName));
                if (!g_env->ExceptionCheck() && dn) {
                    pzj::jstr_to_utf8(dn, e.name, sizeof(e.name));
                    named = true;
                } else {
                    g_env->ExceptionClear();
                }
                g_env->DeleteLocalRef(itm);
            } else {
                g_env->ExceptionClear();
            }
        }
        if (!named) ::strncpy_s(e.name, sizeof(e.name), "Item", _TRUNCATE);
        const float dx = e.wx - lx;
        const float dy = e.wy - ly;
        e.dist = std::sqrt(dx * dx + dy * dy);
        project(e.wx, e.wy, e.wz, e.sx, e.sy, e.on_screen);
    }

    static void carry_render_positions(
        std::vector<entity>& fresh, const std::vector<entity>& previous)
    {
        std::vector<std::uint8_t> matched(previous.size(), 0);
        for (auto& next : fresh) {
            std::size_t best = previous.size();
            float best_distance_sq = 4.0f;
            for (std::size_t i = 0; i < previous.size(); ++i) {
                if (matched[i] || previous[i].type != next.type) continue;
                const float dx = previous[i].wx - next.wx;
                const float dy = previous[i].wy - next.wy;
                const float dz = previous[i].wz - next.wz;
                const float distance_sq = dx * dx + dy * dy + dz * dz;
                if (distance_sq < best_distance_sq) {
                    best_distance_sq = distance_sq;
                    best = i;
                }
            }

            if (best != previous.size() && previous[best].render_position_valid) {
                matched[best] = 1;
                next.render_wx = previous[best].render_wx;
                next.render_wy = previous[best].render_wy;
                next.render_wz = previous[best].render_wz;
                next.render_position_valid = true;
            } else {
                next.render_wx = next.wx;
                next.render_wy = next.wy;
                next.render_wz = next.wz;
                next.render_position_valid = true;
            }
        }
    }

    void collect_entities(std::vector<entity>& out)
    {
        if (!pzj::ensure_resolved()) {
            out.clear();
            return;
        }

        pzj::jframe frame{ 1024 };
        if (!frame.ok || !refresh_frame_ctx()) {
            for (auto& entry : out) entry.on_screen = false;
            return;
        }
        using clock = std::chrono::steady_clock;
        static auto next_refresh = clock::time_point{};
        static auto next_item_refresh = clock::time_point{};
        static std::vector<entity> ground_items;
        constexpr auto refresh_interval = std::chrono::milliseconds{ 33 };
        constexpr auto item_refresh_interval = std::chrono::milliseconds{ 500 };
        constexpr jint max_zombies{ 512 };
        constexpr jint max_vehicles{ 64 };
        constexpr jint max_animals{ 64 };
        constexpr std::size_t max_ground_items{ 128 };
        constexpr std::size_t max_item_candidates{ 512 };
        constexpr jint item_scan_diameter{ 61 }; // getRadius halves this to 30 tiles.
        const auto now = clock::now();

        if (now >= next_refresh) {
            std::vector<entity> fresh;
            fresh.reserve(512);

            const auto player = static_cast<jobject>(
                g_env->CallStaticObjectMethod(g_cls.isoPlayer, g_m.player_getInstance));
            if (g_env->ExceptionCheck() || !player) {
                g_env->ExceptionClear();
                ground_items.clear();
                next_item_refresh = {};
                out.clear();
                return;
            }

            const float lx = g_env->CallFloatMethod(player, g_m.char_getX);
            const float ly = g_env->CallFloatMethod(player, g_m.char_getY);
            if (g_env->ExceptionCheck()) {
                g_env->ExceptionClear();
                return;
            }

            if (!pzj::ensure_list_methods()) return;
            const auto world = static_cast<jobject>(
                g_env->GetStaticObjectField(g_cls.isoWorld, g_m.world_instance));
            if (!g_env->ExceptionCheck() && world) {
                auto cell = static_cast<jobject>(
                    g_env->CallObjectMethod(world, g_m.world_getCell));
                if (g_env->ExceptionCheck() || !cell) {
                    g_env->ExceptionClear();
                    cell = static_cast<jobject>(
                        g_env->GetObjectField(world, g_m.world_currentCell));
                    if (g_env->ExceptionCheck()) {
                        g_env->ExceptionClear();
                        cell = nullptr;
                    }
                }

                if (cell) {
                    // IsoCell owns the complete loaded zombie list. The old
                    // IsoWorld.zombieWithModel source is only a transient
                    // renderer list and is frequently empty outside its pass.
                    const auto zombies = static_cast<jobject>(
                        g_env->CallObjectMethod(cell, g_m.cell_getZombieList));
                    if (!g_env->ExceptionCheck() && zombies) {
                        const jint size = g_env->CallIntMethod(zombies, g_m.list_size);
                        const jint count = std::clamp(size, jint{ 0 }, max_zombies);
                        for (jint i = 0; i < count; ++i) {
                            const auto zombie = static_cast<jobject>(
                                g_env->CallObjectMethod(zombies, g_m.list_get, i));
                            if (g_env->ExceptionCheck() || !zombie) {
                                g_env->ExceptionClear();
                                continue;
                            }
                            entity entry;
                            fill_entity(zombie, lx, ly, entity_type::zombie, entry);
                            fresh.push_back(entry);
                            g_env->DeleteLocalRef(zombie);
                        }
                    } else {
                        g_env->ExceptionClear();
                    }

                    if (g_m.cell_getAnimals) {
                        const auto animals = static_cast<jobject>(
                            g_env->CallObjectMethod(cell, g_m.cell_getAnimals));
                        if (!g_env->ExceptionCheck() && animals) {
                            const jint size = g_env->CallIntMethod(animals, g_m.list_size);
                            const jint count = std::clamp(size, jint{ 0 }, max_animals);
                            for (jint i = 0; i < count; ++i) {
                                const auto animal = static_cast<jobject>(
                                    g_env->CallObjectMethod(animals, g_m.list_get, i));
                                if (g_env->ExceptionCheck() || !animal) {
                                    g_env->ExceptionClear();
                                    continue;
                                }
                                entity entry;
                                fill_animal(animal, lx, ly, entry);
                                fresh.push_back(entry);
                                g_env->DeleteLocalRef(animal);
                            }
                        } else {
                            g_env->ExceptionClear();
                        }
                    }

                    // Static ground items are not guaranteed to exist in
                    // IsoCell.processWorldItems. Walk loaded squares near the
                    // player at a lower cadence and keep the nearest entries.
                    if (now >= next_item_refresh) {
                        std::vector<entity> scanned_items;
                        scanned_items.reserve(max_ground_items);

                        if (g_m.char_getSquare && g_m.square_getRadius &&
                            g_m.square_getWorldObjects) {
                            const auto player_square = static_cast<jobject>(
                                g_env->CallObjectMethod(player, g_m.char_getSquare));
                            if (!g_env->ExceptionCheck() && player_square) {
                                const auto squares = static_cast<jobject>(
                                    g_env->CallObjectMethod(player_square,
                                        g_m.square_getRadius, item_scan_diameter));
                                if (!g_env->ExceptionCheck() && squares) {
                                    const jint square_count = g_env->CallIntMethod(
                                        squares, g_m.list_size);
                                    if (!g_env->ExceptionCheck()) {
                                        for (jint i = 0; i < square_count &&
                                            scanned_items.size() < max_item_candidates; ++i) {
                                            const auto square = static_cast<jobject>(
                                                g_env->CallObjectMethod(squares,
                                                    g_m.list_get, i));
                                            if (g_env->ExceptionCheck() || !square) {
                                                g_env->ExceptionClear();
                                                continue;
                                            }
                                            const auto items = static_cast<jobject>(
                                                g_env->CallObjectMethod(square,
                                                    g_m.square_getWorldObjects));
                                            if (!g_env->ExceptionCheck() && items) {
                                                const jint item_count = g_env->CallIntMethod(
                                                    items, g_m.list_size);
                                                if (!g_env->ExceptionCheck()) {
                                                    for (jint j = 0; j < item_count &&
                                                        scanned_items.size() < max_item_candidates; ++j) {
                                                        const auto item = static_cast<jobject>(
                                                            g_env->CallObjectMethod(items,
                                                                g_m.list_get, j));
                                                        if (g_env->ExceptionCheck() || !item) {
                                                            g_env->ExceptionClear();
                                                            continue;
                                                        }
                                                        entity entry;
                                                        fill_ground_item(item, lx, ly, entry);
                                                        scanned_items.push_back(entry);
                                                        g_env->DeleteLocalRef(item);
                                                    }
                                                } else {
                                                    g_env->ExceptionClear();
                                                }
                                                g_env->DeleteLocalRef(items);
                                            } else {
                                                g_env->ExceptionClear();
                                            }
                                            g_env->DeleteLocalRef(square);
                                        }
                                    } else {
                                        g_env->ExceptionClear();
                                    }
                                    g_env->DeleteLocalRef(squares);
                                } else {
                                    g_env->ExceptionClear();
                                }
                                g_env->DeleteLocalRef(player_square);
                            } else {
                                g_env->ExceptionClear();
                            }
                        }

                        std::sort(scanned_items.begin(), scanned_items.end(),
                            [](const entity& lhs, const entity& rhs) {
                                return lhs.dist < rhs.dist;
                            });
                        if (scanned_items.size() > max_ground_items)
                            scanned_items.resize(max_ground_items);
                        ground_items.swap(scanned_items);
                        next_item_refresh = now + item_refresh_interval;
                    }
                } else {
                    ground_items.clear();
                    next_item_refresh = {};
                }

                // VehicleManager already exposes the authoritative loaded list.
                if (g_cls.vehicleManager && g_m.vehiclemanager_instance &&
                    g_m.vehiclemanager_getVehicles) {
                    const auto manager = static_cast<jobject>(
                        g_env->GetStaticObjectField(g_cls.vehicleManager,
                            g_m.vehiclemanager_instance));
                    if (!g_env->ExceptionCheck() && manager) {
                        const auto vehicles = static_cast<jobject>(
                            g_env->CallObjectMethod(manager,
                                g_m.vehiclemanager_getVehicles));
                        if (!g_env->ExceptionCheck() && vehicles) {
                            const jint size = g_env->CallIntMethod(vehicles, g_m.list_size);
                            const jint count = std::clamp(size, jint{ 0 }, max_vehicles);
                            for (jint i = 0; i < count; ++i) {
                                const auto vehicle = static_cast<jobject>(
                                    g_env->CallObjectMethod(vehicles, g_m.list_get, i));
                                if (g_env->ExceptionCheck() || !vehicle) {
                                    g_env->ExceptionClear();
                                    continue;
                                }
                                entity entry;
                                fill_vehicle(vehicle, lx, ly, entry);
                                fresh.push_back(entry);
                                g_env->DeleteLocalRef(vehicle);
                            }
                        } else {
                            g_env->ExceptionClear();
                        }
                    } else {
                        g_env->ExceptionClear();
                    }
                }
            } else {
                g_env->ExceptionClear();
                ground_items.clear();
                next_item_refresh = {};
            }

            // GameClient.getPlayers() is rebuilt from IDToPlayerMap and is the
            // authoritative remote-player collection. Zero is expected in SP.
            if (g_cls.gameClient && g_m.gameclient_instance &&
                g_m.gameclient_getPlayers) {
                const auto client = static_cast<jobject>(
                    g_env->GetStaticObjectField(g_cls.gameClient,
                        g_m.gameclient_instance));
                if (!g_env->ExceptionCheck() && client) {
                    const auto players = static_cast<jobject>(
                        g_env->CallObjectMethod(client, g_m.gameclient_getPlayers));
                    if (!g_env->ExceptionCheck() && players) {
                        const jint size = g_env->CallIntMethod(players, g_m.list_size);
                        const jint count = std::clamp(size, jint{ 0 }, jint{ 64 });
                        for (jint i = 0; i < count; ++i) {
                            const auto other = static_cast<jobject>(
                                g_env->CallObjectMethod(players, g_m.list_get, i));
                            if (g_env->ExceptionCheck() || !other) {
                                g_env->ExceptionClear();
                                continue;
                            }
                            if (!g_env->IsSameObject(other, player)) {
                                entity entry;
                                fill_entity(other, lx, ly, entity_type::player, entry);
                                fresh.push_back(entry);
                            }
                            g_env->DeleteLocalRef(other);
                        }
                    } else {
                        g_env->ExceptionClear();
                    }
                } else {
                    g_env->ExceptionClear();
                }
            }

            fresh.insert(fresh.end(), ground_items.begin(), ground_items.end());
            carry_render_positions(fresh, out);
            out.swap(fresh);
            next_refresh = now + refresh_interval;
        }

        // Camera follows the player every frame. Reproject the cached world
        // positions natively while 30 Hz JNI snapshots are interpolated.
        static auto previous_frame = clock::now();
        const auto frame_now = clock::now();
        const float delta_seconds = std::clamp(
            std::chrono::duration<float>(frame_now - previous_frame).count(),
            0.0f, 0.05f);
        previous_frame = frame_now;

        // Smooth only entity movement in world space. Camera and zoom remain
        // exact because projection still runs from the current frame context.
        for (auto& entry : out) {
            if (!entry.render_position_valid) {
                entry.render_wx = entry.wx;
                entry.render_wy = entry.wy;
                entry.render_wz = entry.wz;
                entry.render_position_valid = true;
            } else {
                entry.render_wx = projection::smooth_toward(
                    entry.render_wx, entry.wx, delta_seconds);
                entry.render_wy = projection::smooth_toward(
                    entry.render_wy, entry.wy, delta_seconds);
                entry.render_wz = projection::smooth_toward(
                    entry.render_wz, entry.wz, delta_seconds);
            }
            project(entry.render_wx, entry.render_wy, entry.render_wz,
                entry.sx, entry.sy, entry.on_screen);
        }
    }

    // ---- item database ------------------------------------------------------

    // The flattened database is built once on first use and kept as
    // std::strings (converted from JNI local refs into globals is not
    // needed; we copy to native memory immediately).
    struct item_entry {
        std::string full_type;
        std::string display;
    };
    static std::vector<item_entry> g_items;
    static jobject g_item_source = nullptr;
    static jint g_item_cursor = 0;
    static jint g_item_total = 0;
    static bool g_items_built = false;

    static bool begin_item_db()
    {
        if (g_item_source) return true;
        if (!g_cls.scriptManager || !g_m.scriptman_instance ||
            !g_m.scriptman_getAllItems || !g_m.item_getFullName ||
            !g_m.item_getDisplayName || !pzj::ensure_list_methods()) return false;

        const auto manager = static_cast<jobject>(
            g_env->GetStaticObjectField(g_cls.scriptManager, g_m.scriptman_instance));
        if (g_env->ExceptionCheck() || !manager) {
            g_env->ExceptionClear();
            return false;
        }
        const auto items = static_cast<jobject>(
            g_env->CallObjectMethod(manager, g_m.scriptman_getAllItems));
        if (g_env->ExceptionCheck() || !items) {
            g_env->ExceptionClear();
            return false;
        }

        g_item_total = g_env->CallIntMethod(items, g_m.list_size);
        if (g_env->ExceptionCheck() || g_item_total <= 0) {
            g_env->ExceptionClear();
            return false;
        }
        g_item_source = g_env->NewGlobalRef(items);
        if (!g_item_source) return false;
        g_items.clear();
        g_items.reserve(static_cast<std::size_t>(g_item_total));
        g_item_cursor = 0;
        pzlog2::log("item db loading: %d definitions", g_item_total);
        return true;
    }

    static void build_item_db_batch()
    {
        if (g_items_built || !begin_item_db()) return;
        constexpr jint batch_size = 64;
        const jint end = std::min(g_item_cursor + batch_size, g_item_total);

        for (; g_item_cursor < end; ++g_item_cursor) {
            const auto script = static_cast<jobject>(
                g_env->CallObjectMethod(g_item_source, g_m.list_get, g_item_cursor));
            if (g_env->ExceptionCheck() || !script) {
                g_env->ExceptionClear();
                continue;
            }
            const auto full_name = static_cast<jstring>(
                g_env->CallObjectMethod(script, g_m.item_getFullName));
            const auto display_name = static_cast<jstring>(
                g_env->CallObjectMethod(script, g_m.item_getDisplayName));
            if (g_env->ExceptionCheck()) {
                g_env->ExceptionClear();
                g_env->DeleteLocalRef(script);
                continue;
            }

            char full[256]{};
            char display[256]{};
            if (full_name) pzj::jstr_to_utf8(full_name, full, sizeof(full));
            if (display_name)
                pzj::jstr_to_utf8(display_name, display, sizeof(display));
            if (full[0]) {
                item_entry entry;
                entry.full_type = full;
                entry.display = display[0] ? display : full;
                g_items.push_back(std::move(entry));
            }

            if (display_name) g_env->DeleteLocalRef(display_name);
            if (full_name) g_env->DeleteLocalRef(full_name);
            g_env->DeleteLocalRef(script);
        }

        if (g_item_cursor >= g_item_total) {
            std::sort(g_items.begin(), g_items.end(),
                [](const item_entry& lhs, const item_entry& rhs) {
                    return lhs.display == rhs.display
                        ? lhs.full_type < rhs.full_type
                        : lhs.display < rhs.display;
                });
            g_items.erase(std::unique(g_items.begin(), g_items.end(),
                [](const item_entry& lhs, const item_entry& rhs) {
                    return lhs.full_type == rhs.full_type;
                }), g_items.end());
            g_env->DeleteGlobalRef(g_item_source);
            g_item_source = nullptr;
            g_items_built = true;
            pzlog2::log("item db built: %d entries",
                static_cast<int>(g_items.size()));
        }
    }

    int item_db_count()
    {
        if (!pzj::ensure_resolved()) return 0;
        if (!g_items_built) {
            pzj::jframe frame{ 192 };
            if (frame.ok) build_item_db_batch();
        }
        return static_cast<int>(g_items.size());
    }

    const char* item_db_get(int idx, const char** out_display)
    {
        if (idx < 0 || idx >= static_cast<int>(g_items.size())) {
            if (out_display) *out_display = nullptr;
            return nullptr;
        }
        if (out_display) *out_display = g_items[idx].display.c_str();
        return g_items[idx].full_type.c_str();
    }

    // ---- spawning -----------------------------------------------------------
    static char g_last_spawn_msg[128] = "none";

    static bool can_spawn_world_item()
    {
        if (!g_cls.gameClient || !g_m.gameclient_client) {
            ::strcpy_s(g_last_spawn_msg, "spawn refused: network mode unavailable");
            return false;
        }
        const jboolean client = g_env->GetStaticBooleanField(
            g_cls.gameClient, g_m.gameclient_client);
        if (g_env->ExceptionCheck()) {
            g_env->ExceptionClear();
            ::strcpy_s(g_last_spawn_msg, "spawn refused: network mode lookup failed");
            return false;
        }
        // Installed Build 42 only transmits these world-item overloads from
        // GameServer. A client-side object would be an unsynchronized pickup.
        if (client == JNI_TRUE) {
            ::strcpy_s(g_last_spawn_msg,
                "spawn refused: multiplayer requires server-side spawning");
            return false;
        }
        return true;
    }

    bool spawn_item(const char* full_type)
    {
        ::strcpy_s(g_last_spawn_msg, "ground spawn failed");
        if (!full_type || !full_type[0]) return false;
        if (!pzj::ensure_resolved() || !g_m.char_getSquare ||
            !g_m.square_addWorldItem_str) return false;

        pzj::jframe frame;
        if (!frame.ok) return false;
        if (!can_spawn_world_item()) return false;

        const auto player = static_cast<jobject>(
            g_env->CallStaticObjectMethod(g_cls.isoPlayer, g_m.player_getInstance));
        if (g_env->ExceptionCheck() || !player) {
            g_env->ExceptionClear();
            return false;
        }
        const auto square = static_cast<jobject>(
            g_env->CallObjectMethod(player, g_m.char_getSquare));
        if (g_env->ExceptionCheck() || !square) {
            g_env->ExceptionClear();
            return false;
        }

        const auto type = pzj::utf8_to_jstr(full_type);
        if (!type) return false;
        const auto added = static_cast<jobject>(g_env->CallObjectMethod(
            square, g_m.square_addWorldItem_str, type,
            0.5f, 0.5f, 0.0f, JNI_TRUE, JNI_TRUE));
        if (g_env->ExceptionCheck()) {
            g_env->ExceptionClear();
            return false;
        }
        const bool ok = added != nullptr;

        _snprintf_s(g_last_spawn_msg, sizeof(g_last_spawn_msg), _TRUNCATE,
            "ground spawn %s -> %s", full_type, ok ? "created locally" : "FAIL");
        pzlog2::log("%s", g_last_spawn_msg);
        return ok;
    }

    bool spawn_item_custom(const char* full_type, int condition, int ammo)
    {
        ::strcpy_s(g_last_spawn_msg, "custom ground spawn failed");
        if (!full_type || !full_type[0]) return false;
        if (!pzj::ensure_resolved() || !g_m.char_getSquare ||
            !g_m.square_addWorldItem_obj) return false;

        pzj::jframe frame;
        if (!frame.ok) return false;
        if (!can_spawn_world_item()) return false;

        const auto type = pzj::utf8_to_jstr(full_type);
        if (!type) return false;

        jobject item = nullptr;
        if (g_m.factory_CreateItem_str_f) {
            const float condition_factor = condition < 0
                ? 1.0f : static_cast<float>(condition) / 100.0f;
            item = static_cast<jobject>(g_env->CallStaticObjectMethod(
                g_cls.itemFactory, g_m.factory_CreateItem_str_f, type,
                condition_factor));
            if (g_env->ExceptionCheck()) {
                g_env->ExceptionClear();
                item = nullptr;
            }
        }
        if (!item && g_m.factory_CreateItem_str) {
            item = static_cast<jobject>(g_env->CallStaticObjectMethod(
                g_cls.itemFactory, g_m.factory_CreateItem_str, type));
            if (g_env->ExceptionCheck()) {
                g_env->ExceptionClear();
                item = nullptr;
            }
        }
        if (!item) return false;

        if (condition >= 0 && g_m.inv_setCondition) {
            g_env->CallVoidMethod(item, g_m.inv_setCondition,
                static_cast<jint>(condition));
            if (g_env->ExceptionCheck()) {
                g_env->ExceptionClear();
                return false;
            }
        }

        if (ammo >= 0 && g_cls.handWeapon &&
            g_env->IsInstanceOf(item, g_cls.handWeapon) &&
            g_m.inv_setCurrentAmmoCount) {
            g_env->CallVoidMethod(item, g_m.inv_setCurrentAmmoCount,
                static_cast<jint>(ammo));
            if (g_env->ExceptionCheck()) {
                g_env->ExceptionClear();
                return false;
            }
        }

        const auto player = static_cast<jobject>(
            g_env->CallStaticObjectMethod(g_cls.isoPlayer, g_m.player_getInstance));
        if (g_env->ExceptionCheck() || !player) {
            g_env->ExceptionClear();
            return false;
        }
        const auto square = static_cast<jobject>(
            g_env->CallObjectMethod(player, g_m.char_getSquare));
        if (g_env->ExceptionCheck() || !square) {
            g_env->ExceptionClear();
            return false;
        }

        const auto added = static_cast<jobject>(g_env->CallObjectMethod(
            square, g_m.square_addWorldItem_obj, item,
            0.5f, 0.5f, 0.0f, JNI_TRUE, JNI_TRUE));
        if (g_env->ExceptionCheck() || !added) {
            g_env->ExceptionClear();
            return false;
        }

        _snprintf_s(g_last_spawn_msg, sizeof(g_last_spawn_msg), _TRUNCATE,
            "custom ground %s c=%d a=%d -> ok", full_type, condition, ammo);
        pzlog2::log("%s", g_last_spawn_msg);
        return true;
    }

    static bool get_player_cheat(jobject player, jfieldID cheat_field,
        bool& enabled)
    {
        if (!player || !cheat_field || !g_m.char_getCheats ||
            !g_m.playercheats_cheats || !g_m.enumset_contains ||
            !g_cls.cheatType) {
            return false;
        }

        const auto cheats = static_cast<jobject>(
            g_env->CallObjectMethod(player, g_m.char_getCheats));
        if (g_env->ExceptionCheck() || !cheats) {
            if (g_env->ExceptionCheck()) g_env->ExceptionClear();
            return false;
        }
        const auto set = static_cast<jobject>(
            g_env->GetObjectField(cheats, g_m.playercheats_cheats));
        const auto type = g_env->GetStaticObjectField(g_cls.cheatType, cheat_field);
        if (g_env->ExceptionCheck() || !set || !type) {
            if (g_env->ExceptionCheck()) g_env->ExceptionClear();
            if (type) g_env->DeleteLocalRef(type);
            if (set) g_env->DeleteLocalRef(set);
            g_env->DeleteLocalRef(cheats);
            return false;
        }

        const jboolean value = g_env->CallBooleanMethod(
            set, g_m.enumset_contains, type);
        const bool success = !g_env->ExceptionCheck();
        if (!success) g_env->ExceptionClear();
        if (success) enabled = value == JNI_TRUE;
        g_env->DeleteLocalRef(type);
        g_env->DeleteLocalRef(set);
        g_env->DeleteLocalRef(cheats);
        return success;
    }

    static bool set_player_cheat(jobject player, jfieldID cheat_field,
        const bool enabled)
    {
        if (!player || !cheat_field || !g_m.char_getCheats ||
            !g_m.playercheats_cheats || !g_m.enumset_add ||
            !g_m.enumset_remove || !g_cls.cheatType) {
            return false;
        }

        const auto cheats = static_cast<jobject>(
            g_env->CallObjectMethod(player, g_m.char_getCheats));
        if (g_env->ExceptionCheck() || !cheats) {
            if (g_env->ExceptionCheck()) g_env->ExceptionClear();
            return false;
        }
        const auto set = static_cast<jobject>(
            g_env->GetObjectField(cheats, g_m.playercheats_cheats));
        const auto type = g_env->GetStaticObjectField(g_cls.cheatType, cheat_field);
        if (g_env->ExceptionCheck() || !set || !type) {
            if (g_env->ExceptionCheck()) g_env->ExceptionClear();
            if (type) g_env->DeleteLocalRef(type);
            if (set) g_env->DeleteLocalRef(set);
            g_env->DeleteLocalRef(cheats);
            return false;
        }

        g_env->CallBooleanMethod(set,
            enabled ? g_m.enumset_add : g_m.enumset_remove, type);
        bool readback{};
        const bool success = !g_env->ExceptionCheck() &&
            get_player_cheat(player, cheat_field, readback) &&
            readback == enabled;
        if (g_env->ExceptionCheck()) g_env->ExceptionClear();
        g_env->DeleteLocalRef(type);
        g_env->DeleteLocalRef(set);
        g_env->DeleteLocalRef(cheats);
        return success;
    }

    // ---- debug ---------------------------------------------------------------

    debug_info get_debug_info()
    {
        debug_info d{};
        d.jni_env_valid = (pzj::g_env != nullptr);
        d.class_loader_valid = (pzj::g_class_loader != nullptr);
        d.resolved = pzj::g_resolved;
        d.screen_w = g_frame_ctx.screen_w;
        d.screen_h = g_frame_ctx.screen_h;
        d.tile_scale = g_frame_ctx.tile_scale;
        d.player_idx = g_frame_ctx.player_idx;
        d.cam_off_x = g_frame_ctx.cam_off_x;
        d.cam_off_y = g_frame_ctx.cam_off_y;
        d.frame_ctx_valid = g_frame_ctx.valid;
        d.isoutils_available = (g_m.isoutils_XToScreenExact != nullptr);
        d.climate_method_api = (g_m.climatefloat_setEnableOverride != nullptr &&
            g_m.climatefloat_setOverride != nullptr);
        d.climate_fields_ok = (g_m.climate_desaturationMember != nullptr &&
            g_m.climate_getInstance != nullptr);
        d.world_spawn_api_available = g_m.square_addWorldItem_str &&
            g_m.square_addWorldItem_obj;
        d.zoom = 0.0f;
        if (d.resolved && g_m.core_getZoom && g_cls.core) {
            const auto core = static_cast<jobject>(
                g_env->CallStaticObjectMethod(g_cls.core, g_m.core_getInstance));
            if (!g_env->ExceptionCheck() && core) {
                d.zoom = g_env->CallFloatMethod(core, g_m.core_getZoom,
                    static_cast<jint>(g_frame_ctx.player_idx));
                if (g_env->ExceptionCheck()) { g_env->ExceptionClear(); d.zoom = 0; }
            } else { g_env->ExceptionClear(); }
        }
        ::strncpy_s(d.last_spawn_result, sizeof(d.last_spawn_result),
            g_last_spawn_msg, _TRUNCATE);
        return d;
    }

    void dump_deep_debug(const std::vector<entity>& entities)
    {
        wchar_t temp[MAX_PATH]{};
        ::GetTempPathW(MAX_PATH, temp);
        std::wstring path = std::wstring(temp) + L"pzint_dump.txt";
        FILE* f = nullptr;
        ::_wfopen_s(&f, path.c_str(), L"wt");
        if (!f) return;

        auto w = [&](const char* fmt, ...) {
            va_list ap; va_start(ap, fmt);
            char buf[2048]; ::_vsnprintf_s(buf, sizeof(buf), _TRUNCATE, fmt, ap);
            va_end(ap);
            std::fputs(buf, f); std::fputs("\n", f);
        };

        w("=== pz-int DEEP DEBUG DUMP ===");
        w("timestamp: render frame");
        w("");

        // ---- JNI state ----
        w("=== JNI STATE ===");
        w("g_env: %p", pzj::g_env);
        w("g_vm: %p", pzj::g_vm);
        w("g_class_loader: %p", pzj::g_class_loader);
        w("g_load_class: %p", pzj::g_load_class);
        w("g_resolved: %d", pzj::g_resolved ? 1 : 0);
        w("g_attach_failed: %d", pzj::g_attach_failed ? 1 : 0);
        w("");

        // ---- All class resolution ----
        w("=== CLASSES (null = FAILED) ===");
        #define CLS(name) w("  " #name ": %p", g_cls.name)
        CLS(isoPlayer); CLS(isoZombie); CLS(isoGameCharacter); CLS(isoMovingObject);
        CLS(isoWorld); CLS(isoCell); CLS(isoGridSquare); CLS(isoCamera); CLS(gameClient);
        CLS(climateManager); CLS(climateFloat); CLS(scriptManager); CLS(itemScript);
        CLS(inventoryItem); CLS(handWeapon); CLS(itemFactory);
        CLS(bodyDamage); CLS(bodyPart); CLS(bodyPartType); CLS(stats);
        CLS(characterStat); CLS(systemDisabler); CLS(core);
        CLS(vehicleManager); CLS(baseVehicle); CLS(isoAnimal);
        CLS(worldInventoryObject); CLS(isoUtils);
        #undef CLS
        w("");

        // ---- All method resolution ----
        w("=== METHODS (null = FAILED) ===");
        #define MTD(name) w("  " #name ": %p", (void*)g_m.name)
        MTD(player_getInstance); MTD(player_getPlayerNum);
        MTD(char_getX); MTD(char_getY); MTD(char_getZ); MTD(char_getSquare);
        MTD(char_getName); MTD(char_getHealth);
        MTD(char_setHealth); MTD(char_getMaxWeight); MTD(char_setMaxWeight);
        MTD(char_invincible); MTD(char_getBodyDamage); MTD(char_getStats);
        MTD(cam_getOffX); MTD(cam_getOffY); MTD(core_getInstance);
        MTD(core_getScreenWidth); MTD(core_getScreenHeight); MTD(core_tileScale);
        MTD(core_getZoom);
        w("  -- climate --");
        MTD(climate_getInstance); MTD(climate_getFloat);
        MTD(climate_desaturationMember); MTD(climate_globalLightIntensityMember);
        MTD(climate_nightStrengthMember); MTD(climate_ambientMember);
        MTD(climate_viewDistanceMember); MTD(climate_dayLightStrengthMember);
        MTD(climate_override); MTD(climate_interpolate);
        MTD(climate_isOverride); MTD(climate_isOverrideValue); MTD(climate_finalValue);
        MTD(climatefloat_setOverride); MTD(climatefloat_setEnableOverride);
        MTD(climatefloat_setFinalValue);
        w("  -- world item spawn --");
        MTD(square_addWorldItem_str); MTD(square_addWorldItem_obj);
        w("  -- isoutils --");
        MTD(isoutils_XToScreenExact); MTD(isoutils_YToScreenExact);
        w("  -- stats --");
        MTD(stats_set); MTD(stat_hunger); MTD(stat_thirst);
        MTD(stat_sickness); MTD(stat_pain); MTD(system_zombiesDontAttack);
        w("  -- vehicle --");
        MTD(vehiclemanager_instance); MTD(vehiclemanager_getVehicles);
        MTD(vehicle_getScriptName);
        w("  -- animal / ground item --");
        MTD(animal_getAnimalType); MTD(cell_getZombieList);
        MTD(square_getRadius); MTD(square_getWorldObjects);
        MTD(worldinvobj_getItem); MTD(worldinvobj_getWorldPosX);
        MTD(invitem_getDisplayName);
        #undef MTD
        w("");

        // ---- Live climate state ----
        w("=== LIVE CLIMATE STATE ===");
        if (pzj::g_resolved && g_m.climate_getInstance && g_cls.climateManager) {
            pzj::jframe frame{ 64 };
            if (frame.ok) {
                const auto mgr = static_cast<jobject>(
                    g_env->CallStaticObjectMethod(g_cls.climateManager, g_m.climate_getInstance));
                if (!g_env->ExceptionCheck() && mgr) {
                    const char* names[] = {"desaturation","globalLightIntensity","nightStrength","ambient","viewDistance","dayLightStrength"};
                    const jfieldID flds[] = {
                        g_m.climate_desaturationMember, g_m.climate_globalLightIntensityMember,
                        g_m.climate_nightStrengthMember, g_m.climate_ambientMember,
                        g_m.climate_viewDistanceMember, g_m.climate_dayLightStrengthMember
                    };
                    for (int i = 0; i < 6; ++i) {
                        if (!flds[i]) { w("  %s: FIELD_NULL", names[i]); continue; }
                        const auto cf = static_cast<jobject>(g_env->GetObjectField(mgr, flds[i]));
                        if (!cf || g_env->ExceptionCheck()) {
                            g_env->ExceptionClear();
                            w("  %s: GET_FAILED", names[i]);
                            continue;
                        }
                        float ov = g_env->GetFloatField(cf, g_m.climate_override);
                        float fv = g_env->GetFloatField(cf, g_m.climate_finalValue);
                        float ip = g_env->GetFloatField(cf, g_m.climate_interpolate);
                        jboolean io = g_env->GetBooleanField(cf, g_m.climate_isOverride);
                        if (g_env->ExceptionCheck()) g_env->ExceptionClear();
                        w("  %s: override=%.3f final=%.3f interp=%.3f isOverride=%d",
                            names[i], ov, fv, ip, io ? 1 : 0);

                        // TEST: try setting override via method API right now
                        if (g_m.climatefloat_setEnableOverride && g_m.climatefloat_setOverride) {
                            g_env->CallVoidMethod(cf, g_m.climatefloat_setOverride, 1.0f, 1.0f);
                            g_env->CallVoidMethod(cf, g_m.climatefloat_setEnableOverride, JNI_TRUE);
                            bool exc = g_env->ExceptionCheck();
                            if (exc) g_env->ExceptionClear();
                            // Read back
                            float ov2 = g_env->GetFloatField(cf, g_m.climate_override);
                            jboolean io2 = g_env->GetBooleanField(cf, g_m.climate_isOverride);
                            if (g_env->ExceptionCheck()) g_env->ExceptionClear();
                            w("    -> TEST setOverride(1.0,1.0)+setEnableOverride(true): exc=%d override=%.3f isOverride=%d",
                                exc?1:0, ov2, io2?1:0);
                            // Restore original
                            g_env->CallVoidMethod(cf, g_m.climatefloat_setOverride, ov, ip);
                            g_env->CallVoidMethod(cf, g_m.climatefloat_setEnableOverride, io);
                            if (g_env->ExceptionCheck()) g_env->ExceptionClear();
                        }
                        g_env->DeleteLocalRef(cf);
                    }
                    g_env->DeleteLocalRef(mgr);
                } else {
                    g_env->ExceptionClear();
                    w("  ClimateManager.getInstance() FAILED");
                }
            } else { w("  JFrame allocation failed"); }
        } else { w("  NOT RESOLVED"); }
        w("");

        // ---- Frame context ----
        w("=== FRAME CONTEXT ===");
        w("valid=%d screen=%dx%d tileScale=%d playerIdx=%d",
            g_frame_ctx.valid?1:0, g_frame_ctx.screen_w, g_frame_ctx.screen_h,
            g_frame_ctx.tile_scale, g_frame_ctx.player_idx);
        w("camOff=(%.1f, %.1f)", g_frame_ctx.cam_off_x, g_frame_ctx.cam_off_y);
        w("");
        // ---- Live feature readback (verify actual in-game effect) ----
        w("=== FEATURE READBACK ===");
        if (pzj::g_resolved) {
            const jboolean core_debug = (g_cls.core && g_m.core_debug)
                ? g_env->GetStaticBooleanField(g_cls.core, g_m.core_debug)
                : JNI_FALSE;
            if (g_env->ExceptionCheck()) g_env->ExceptionClear();
            w("Core.debug=%d (PlayerCheats.isSet gate)", core_debug ? 1 : 0);
            const auto player = static_cast<jobject>(g_env->CallStaticObjectMethod(
                g_cls.isoPlayer, g_m.player_getInstance));
            if (!g_env->ExceptionCheck() && player) {
                const auto report = [&](const char* name, jfieldID field) {
                    bool member = false;
                    const bool ok = get_player_cheat(player, field, member);
                    w("  cheat %s: member=%d read_ok=%d", name,
                        member ? 1 : 0, ok ? 1 : 0);
                };
                report("NO_CLIP", g_m.cheat_noclip);
                report("INVISIBLE", g_m.cheat_invisible);
                report("UNLIMITED_ENDURANCE", g_m.cheat_endurance);
                report("UNLIMITED_CARRY", g_m.cheat_carry);
                report("TIMED_ACTION_INSTANT", g_m.cheat_instant_actions);
                if (g_m.char_getMaxWeight) {
                    const jint mw = g_env->CallIntMethod(player,
                        g_m.char_getMaxWeight);
                    if (g_env->ExceptionCheck()) g_env->ExceptionClear();
                    const jint mwb = g_m.char_maxWeightBase
                        ? g_env->GetIntField(player, g_m.char_maxWeightBase) : -1;
                    if (g_env->ExceptionCheck()) g_env->ExceptionClear();
                    w("  maxWeight=%d maxWeightBase=%d", mw, mwb);
                }
                g_env->DeleteLocalRef(player);
            } else {
                g_env->ExceptionClear();
                w("  player unavailable");
            }
        } else { w("  NOT RESOLVED"); }
        w("");

        // ---- All entities ----
        w("=== ALL ENTITIES (%d) ===", static_cast<int>(entities.size()));
        for (int i = 0; i < static_cast<int>(entities.size()); ++i) {
            const auto& e = entities[i];
            const char* tname = "?";
            switch(e.type){
            case entity_type::zombie: tname="zombie"; break;
            case entity_type::player: tname="player"; break;
            case entity_type::vehicle: tname="vehicle"; break;
            case entity_type::animal: tname="animal"; break;
            case entity_type::item: tname="item"; break;
            }
            w("  [%d] type=%s name='%s' wpos=(%.2f,%.2f,%.2f) spos=(%.1f,%.1f) on_screen=%d dist=%.1f hp=%.1f local=%d",
                i, tname, e.name, e.wx, e.wy, e.wz, e.sx, e.sy, e.on_screen?1:0, e.dist, e.health, e.is_local?1:0);
        }
        w("");

        w("=== DUMP COMPLETE ===");
        w("File: %%TEMP%%\\pzint_dump.txt");
        std::fclose(f);
        pzlog2::log("deep debug dump written to %%TEMP%%\\pzint_dump.txt");
    }

    // ---- toggles --------------------------------------------------------------

    struct climate_state {
        bool saved{ false };
        jboolean is_override{ JNI_FALSE };
        jboolean is_override_value{ JNI_FALSE };
        float override_value{ 0.0f };
        float interpolate{ 0.0f };
        float final_value{ 0.0f };
    };

    static std::array<climate_state, 6> g_climate_state{};

    static bool climate_state_saved()
    {
        return std::any_of(g_climate_state.begin(), g_climate_state.end(),
            [](const climate_state& state) { return state.saved; });
    }
    static bool g_zombie_state_saved = false;
    static jboolean g_zombie_state = JNI_FALSE;
    static jobject g_survival_player = nullptr;
    static bool g_invincible_saved = false;
    static jboolean g_invincible_state = JNI_FALSE;
    static bool g_debug_saved = false;
    static jboolean g_debug_state = JNI_FALSE;
    static bool g_invisible_applied = false;
    static bool g_noclip_applied = false;
    static bool g_endurance_applied = false;
    static bool g_instant_actions_applied = false;
    static bool g_carry_applied = false;
    static bool g_ammo_applied = false;
    // Unlimited carry / anti overload force the maxWeight pair directly.
    static bool g_maxweight_saved = false;
    static jint g_maxweight_value = 0;
    static jint g_maxweightbase_value = 0;

    struct weapon_state {
        jobject weapon{};
        jint hit_chance{};
        jint recoil_delay{};
        jint aiming_time{};
        float projectile_spread{};
        float critical_chance{};
        float min_damage{};
        float max_damage{};
    };

    static weapon_state g_weapon_state{};
    static survival_features g_last_features{};

    static bool same_features(const survival_features& lhs,
        const survival_features& rhs)
    {
        return lhs.full_bright == rhs.full_bright &&
            lhs.night_vision == rhs.night_vision &&
            lhs.zombie_ignore == rhs.zombie_ignore &&
            lhs.god_mode == rhs.god_mode &&
            lhs.anti_hunger == rhs.anti_hunger &&
            lhs.anti_overload == rhs.anti_overload &&
            lhs.anti_thirst == rhs.anti_thirst &&
            lhs.auto_heal == rhs.auto_heal &&
            lhs.infinite_ammo == rhs.infinite_ammo &&
            lhs.unlimited_endurance == rhs.unlimited_endurance &&
            lhs.instant_actions == rhs.instant_actions &&
            lhs.aim_assist == rhs.aim_assist &&
            lhs.aim_assist_max_dist == rhs.aim_assist_max_dist &&
            lhs.perfect_accuracy == rhs.perfect_accuracy &&
            lhs.one_hit == rhs.one_hit &&
            lhs.anti_fatigue == rhs.anti_fatigue &&
            lhs.all_needs == rhs.all_needs &&
            lhs.invisible == rhs.invisible &&
            lhs.noclip == rhs.noclip &&
            lhs.unlimited_carry == rhs.unlimited_carry &&
            lhs.no_reload == rhs.no_reload &&
            lhs.debug_bypass == rhs.debug_bypass;
    }

    // Cure all diseases, infections, wounds, and negative stats on the player.
    static void apply_auto_heal(jobject player)
    {
        if (!g_m.char_getBodyDamage) return;
        const auto bd = static_cast<jobject>(
            g_env->CallObjectMethod(player, g_m.char_getBodyDamage));
        if (g_env->ExceptionCheck() || !bd) { g_env->ExceptionClear(); return; }

        // 1. Full health restore
        if (g_m.bodydamage_RestoreToFullHealth)
            g_env->CallVoidMethod(bd, g_m.bodydamage_RestoreToFullHealth);

        // 2. Clear zombie infection
        if (g_m.bodydamage_setInfected)
            g_env->CallVoidMethod(bd, g_m.bodydamage_setInfected, JNI_FALSE);
        if (g_m.bodydamage_setIsFakeInfected)
            g_env->CallVoidMethod(bd, g_m.bodydamage_setIsFakeInfected, JNI_FALSE);
        if (g_m.bodydamage_setInfectionLevel)
            g_env->CallVoidMethod(bd, g_m.bodydamage_setInfectionLevel, 0.0f);
        if (g_m.bodydamage_setInfectionTime)
            g_env->CallVoidMethod(bd, g_m.bodydamage_setInfectionTime, -1.0f);

        // 3. Clear cold/flu
        if (g_m.bodydamage_setHasACold)
            g_env->CallVoidMethod(bd, g_m.bodydamage_setHasACold, JNI_FALSE);
        if (g_m.bodydamage_setColdStrength)
            g_env->CallVoidMethod(bd, g_m.bodydamage_setColdStrength, 0.0f);
        if (g_m.bodydamage_setCatchACold)
            g_env->CallVoidMethod(bd, g_m.bodydamage_setCatchACold, 0.0f);

        if (g_env->ExceptionCheck()) g_env->ExceptionClear();

        // 4. Clear every body part wound/infection
        if (g_m.bodydamage_getBodyParts && pzj::ensure_list_methods()) {
            const auto parts = static_cast<jobject>(
                g_env->CallObjectMethod(bd, g_m.bodydamage_getBodyParts));
            if (!g_env->ExceptionCheck() && parts) {
                const jint count = g_env->CallIntMethod(parts, g_m.list_size);
                for (jint i = 0; i < count && i < 64; ++i) {
                    const auto part = g_env->CallObjectMethod(parts, g_m.list_get, i);
                    if (!part || g_env->ExceptionCheck()) {
                        g_env->ExceptionClear(); continue;
                    }
                    if (g_m.bodypart_SetInfected)
                        g_env->CallVoidMethod(part, g_m.bodypart_SetInfected, JNI_FALSE);
                    if (g_m.bodypart_SetFakeInfected)
                        g_env->CallVoidMethod(part, g_m.bodypart_SetFakeInfected, JNI_FALSE);
                    if (g_m.bodypart_setBleeding)
                        g_env->CallVoidMethod(part, g_m.bodypart_setBleeding, JNI_FALSE);
                    if (g_m.bodypart_setDeepWounded)
                        g_env->CallVoidMethod(part, g_m.bodypart_setDeepWounded, JNI_FALSE);
                    if (g_m.bodypart_setScratched)
                        g_env->CallVoidMethod(part, g_m.bodypart_setScratched, JNI_FALSE, JNI_FALSE);
                    if (g_m.bodypart_setInfectedWound)
                        g_env->CallVoidMethod(part, g_m.bodypart_setInfectedWound, JNI_FALSE);
                    if (g_m.bodypart_setWoundInfectionLevel)
                        g_env->CallVoidMethod(part, g_m.bodypart_setWoundInfectionLevel, 0.0f);
                    if (g_m.bodypart_setHaveGlass)
                        g_env->CallVoidMethod(part, g_m.bodypart_setHaveGlass, JNI_FALSE);
                    if (g_m.bodypart_setHaveBullet)
                        g_env->CallVoidMethod(part, g_m.bodypart_setHaveBullet, JNI_FALSE, 0);
                    if (g_m.bodypart_setNeedBurnWash)
                        g_env->CallVoidMethod(part, g_m.bodypart_setNeedBurnWash, JNI_FALSE);
                    if (g_m.bodypart_setBurnTime)
                        g_env->CallVoidMethod(part, g_m.bodypart_setBurnTime, 0.0f);
                    if (g_env->ExceptionCheck()) g_env->ExceptionClear();
                    g_env->DeleteLocalRef(part);
                }
                g_env->DeleteLocalRef(parts);
            }
            if (g_env->ExceptionCheck()) g_env->ExceptionClear();
        }

        // 5. Clear disease stats (sickness, pain, food poisoning, poison, zombie fever, stress, unhappiness)
        if (g_m.char_getStats && g_m.stats_set) {
            const auto stats = static_cast<jobject>(
                g_env->CallObjectMethod(player, g_m.char_getStats));
            if (!g_env->ExceptionCheck() && stats) {
                const jfieldID disease_stats[] = {
                    g_m.stat_sickness, g_m.stat_pain, g_m.stat_food_sickness,
                    g_m.stat_poison, g_m.stat_zombie_fever, g_m.stat_stress,
                    g_m.stat_unhappiness
                };
                for (const auto sf : disease_stats) {
                    if (!sf) continue;
                    const auto stat_obj = g_env->GetStaticObjectField(
                        g_cls.characterStat, sf);
                    if (stat_obj) {
                        g_env->CallBooleanMethod(stats, g_m.stats_set,
                            stat_obj, 0.0f);
                    }
                }
                if (g_env->ExceptionCheck()) g_env->ExceptionClear();
                g_env->DeleteLocalRef(stats);
            }
        }

        if (g_m.char_setHealth)
            g_env->CallVoidMethod(player, g_m.char_setHealth, 100.0f);
        if (g_env->ExceptionCheck()) g_env->ExceptionClear();
        g_env->DeleteLocalRef(bd);
    }

    static bool apply_climate_override(bool enabled,
        const std::array<float, 6>& forced_values = { 0.0f, 1.0f, 0.0f, 1.0f, 1.0f, 1.0f })
    {
        if (!enabled && !climate_state_saved()) return true;
        if (!g_m.climate_getInstance) return false;

        const auto manager = static_cast<jobject>(
            g_env->CallStaticObjectMethod(g_cls.climateManager,
                g_m.climate_getInstance));
        if (g_env->ExceptionCheck() || !manager) {
            g_env->ExceptionClear();
            return false;
        }

        const std::array<jfieldID, 6> members{
            g_m.climate_desaturationMember,
            g_m.climate_globalLightIntensityMember,
            g_m.climate_nightStrengthMember,
            g_m.climate_ambientMember,
            g_m.climate_viewDistanceMember,
            g_m.climate_dayLightStrengthMember
        };

        // Use Build 42 method API: setEnableOverride + setOverride(value, interp)
        // These methods set internal state that raw field writes miss, preventing
        // the game's climate tick from overwriting our values.
        const bool has_method_api = g_m.climatefloat_setEnableOverride &&
            g_m.climatefloat_setOverride;

        bool success = true;
        for (std::size_t i = 0; i < members.size(); ++i) {
            auto& state = g_climate_state[i];
            if (!members[i]) {
                if (enabled || state.saved) success = false;
                continue;
            }

            const auto value = static_cast<jobject>(
                g_env->GetObjectField(manager, members[i]));
            if (g_env->ExceptionCheck() || !value) {
                g_env->ExceptionClear();
                success = false;
                continue;
            }

            if (enabled) {
                if (!state.saved) {
                    // Save original state for restoration
                    state.is_override = g_env->GetBooleanField(
                        value, g_m.climate_isOverride);
                    state.override_value = g_env->GetFloatField(
                        value, g_m.climate_override);
                    state.interpolate = g_env->GetFloatField(
                        value, g_m.climate_interpolate);
                    state.final_value = g_env->GetFloatField(
                        value, g_m.climate_finalValue);
                    if (g_env->ExceptionCheck()) {
                        g_env->ExceptionClear();
                        success = false;
                        g_env->DeleteLocalRef(value);
                        continue;
                    }
                    state.is_override_value = g_env->GetBooleanField(
                        value, g_m.climate_isOverrideValue);
                    if (g_env->ExceptionCheck()) g_env->ExceptionClear();
                    state.saved = true;
                }

                // Apply override using method API (preferred) or raw fields (fallback)
                if (has_method_api) {
                    g_env->CallVoidMethod(value, g_m.climatefloat_setOverride,
                        forced_values[i], 1.0f);
                    g_env->CallVoidMethod(value, g_m.climatefloat_setEnableOverride,
                        JNI_TRUE);
                    if (g_m.climatefloat_setFinalValue)
                        g_env->CallVoidMethod(value, g_m.climatefloat_setFinalValue,
                            forced_values[i]);
                } else {
                    g_env->SetFloatField(value, g_m.climate_override, forced_values[i]);
                    g_env->SetFloatField(value, g_m.climate_interpolate, 1.0f);
                    g_env->SetBooleanField(value, g_m.climate_isOverride, JNI_TRUE);
                    g_env->SetBooleanField(value, g_m.climate_isOverrideValue, JNI_FALSE);
                    g_env->SetFloatField(value, g_m.climate_finalValue, forced_values[i]);
                }
                if (g_env->ExceptionCheck()) { g_env->ExceptionClear(); success = false; }
            } else if (state.saved) {
                // Restore original values
                if (has_method_api) {
                    g_env->CallVoidMethod(value, g_m.climatefloat_setOverride,
                        state.override_value, state.interpolate);
                    g_env->CallVoidMethod(value, g_m.climatefloat_setEnableOverride,
                        state.is_override);
                    if (g_m.climatefloat_setFinalValue)
                        g_env->CallVoidMethod(value, g_m.climatefloat_setFinalValue,
                            state.final_value);
                } else {
                    g_env->SetFloatField(value, g_m.climate_override, state.override_value);
                    g_env->SetFloatField(value, g_m.climate_interpolate, state.interpolate);
                    g_env->SetBooleanField(value, g_m.climate_isOverride, state.is_override);
                    g_env->SetBooleanField(value, g_m.climate_isOverrideValue, state.is_override_value);
                    g_env->SetFloatField(value, g_m.climate_finalValue, state.final_value);
                }
                if (g_env->ExceptionCheck()) {
                    g_env->ExceptionClear();
                    success = false;
                } else {
                    state.saved = false;
                }
            }
            g_env->DeleteLocalRef(value);
        }

        g_env->DeleteLocalRef(manager);
        return success && (enabled || !climate_state_saved());
    }

    static bool restore_player_state()
    {
        if (!g_survival_player) {
            return !g_invincible_saved && !g_maxweight_saved &&
                !g_noclip_applied && !g_invisible_applied &&
                !g_endurance_applied && !g_instant_actions_applied &&
                !g_carry_applied && !g_ammo_applied;
        }

        bool success = true;
        if (g_invincible_saved) {
            if (!g_m.char_invincible) {
                success = false;
            } else {
                g_env->SetBooleanField(g_survival_player, g_m.char_invincible,
                    g_invincible_state);
                if (g_env->ExceptionCheck()) {
                    g_env->ExceptionClear();
                    success = false;
                } else {
                    g_invincible_saved = false;
                }
            }
        }
        if (g_maxweight_saved) {
            if (!g_m.char_setMaxWeight) {
                success = false;
            } else {
                g_env->CallVoidMethod(g_survival_player, g_m.char_setMaxWeight,
                    g_maxweight_value);
                if (g_m.char_maxWeightBase)
                    g_env->SetIntField(g_survival_player, g_m.char_maxWeightBase,
                        g_maxweightbase_value);
                if (g_env->ExceptionCheck()) {
                    g_env->ExceptionClear();
                    success = false;
                } else {
                    g_maxweight_saved = false;
                }
            }
        }
        const auto clear_cheat = [&](jfieldID field, bool& applied) {
            if (!applied) return;
            if (set_player_cheat(g_survival_player, field, false)) applied = false;
            else success = false;
        };
        clear_cheat(g_m.cheat_noclip, g_noclip_applied);
        clear_cheat(g_m.cheat_invisible, g_invisible_applied);
        clear_cheat(g_m.cheat_endurance, g_endurance_applied);
        clear_cheat(g_m.cheat_instant_actions, g_instant_actions_applied);
        clear_cheat(g_m.cheat_carry, g_carry_applied);
        clear_cheat(g_m.cheat_ammo, g_ammo_applied);

        const bool restored = !g_invincible_saved && !g_maxweight_saved &&
            !g_noclip_applied && !g_invisible_applied &&
            !g_endurance_applied && !g_instant_actions_applied &&
            !g_carry_applied && !g_ammo_applied;
        if (restored) {
            g_env->DeleteGlobalRef(g_survival_player);
            g_survival_player = nullptr;
        }
        return success && restored;
    }

    [[nodiscard]] static bool weapon_features_enabled(
        const survival_features& features) noexcept
    {
        return features.perfect_accuracy || features.one_hit;
    }

    static bool restore_weapon_state()
    {
        if (!g_weapon_state.weapon) return true;
        if (!g_m.weapon_setHitChance || !g_m.weapon_setRecoilDelay ||
            !g_m.weapon_setAimingTime || !g_m.weapon_setProjectileSpread ||
            !g_m.weapon_setCriticalChance || !g_m.weapon_setMinDamage ||
            !g_m.weapon_setMaxDamage) {
            return false;
        }

        g_env->CallVoidMethod(g_weapon_state.weapon, g_m.weapon_setHitChance,
            g_weapon_state.hit_chance);
        g_env->CallVoidMethod(g_weapon_state.weapon, g_m.weapon_setRecoilDelay,
            g_weapon_state.recoil_delay);
        g_env->CallVoidMethod(g_weapon_state.weapon, g_m.weapon_setAimingTime,
            g_weapon_state.aiming_time);
        g_env->CallVoidMethod(g_weapon_state.weapon,
            g_m.weapon_setProjectileSpread, g_weapon_state.projectile_spread);
        g_env->CallVoidMethod(g_weapon_state.weapon,
            g_m.weapon_setCriticalChance, g_weapon_state.critical_chance);
        g_env->CallVoidMethod(g_weapon_state.weapon, g_m.weapon_setMinDamage,
            g_weapon_state.min_damage);
        g_env->CallVoidMethod(g_weapon_state.weapon, g_m.weapon_setMaxDamage,
            g_weapon_state.max_damage);
        if (g_env->ExceptionCheck()) {
            g_env->ExceptionClear();
            return false;
        }

        g_env->DeleteGlobalRef(g_weapon_state.weapon);
        g_weapon_state = {};
        return true;
    }

    static bool apply_weapon_features(jobject player,
        const survival_features& features)
    {
        const bool enabled = weapon_features_enabled(features);
        if (!enabled) return restore_weapon_state();
        if (!player || !g_m.char_getPrimaryHandItem || !g_cls.handWeapon)
            return false;

        const auto held = static_cast<jobject>(
            g_env->CallObjectMethod(player, g_m.char_getPrimaryHandItem));
        if (g_env->ExceptionCheck() || !held ||
            g_env->IsInstanceOf(held, g_cls.handWeapon) != JNI_TRUE) {
            g_env->ExceptionClear();
            if (held) g_env->DeleteLocalRef(held);
            return restore_weapon_state();
        }

        if (g_weapon_state.weapon &&
            !g_env->IsSameObject(held, g_weapon_state.weapon) &&
            !restore_weapon_state()) {
            g_env->DeleteLocalRef(held);
            return false;
        }

        if (!g_weapon_state.weapon) {
            if (!g_m.weapon_getHitChance || !g_m.weapon_getRecoilDelay ||
                !g_m.weapon_getAimingTime || !g_m.weapon_getProjectileSpread ||
                !g_m.weapon_getCriticalChance || !g_m.weapon_getMinDamage ||
                !g_m.weapon_getMaxDamage) {
                g_env->DeleteLocalRef(held);
                return false;
            }
            weapon_state saved{};
            saved.hit_chance = g_env->CallIntMethod(held,
                g_m.weapon_getHitChance);
            saved.recoil_delay = g_env->CallIntMethod(held,
                g_m.weapon_getRecoilDelay);
            saved.aiming_time = g_env->CallIntMethod(held,
                g_m.weapon_getAimingTime);
            saved.projectile_spread = g_env->CallFloatMethod(held,
                g_m.weapon_getProjectileSpread);
            saved.critical_chance = g_env->CallFloatMethod(held,
                g_m.weapon_getCriticalChance);
            saved.min_damage = g_env->CallFloatMethod(held,
                g_m.weapon_getMinDamage);
            saved.max_damage = g_env->CallFloatMethod(held,
                g_m.weapon_getMaxDamage);
            if (g_env->ExceptionCheck()) {
                g_env->ExceptionClear();
                g_env->DeleteLocalRef(held);
                return false;
            }
            saved.weapon = g_env->NewGlobalRef(held);
            if (g_env->ExceptionCheck() || !saved.weapon) {
                g_env->ExceptionClear();
                g_env->DeleteLocalRef(held);
                return false;
            }
            g_weapon_state = saved;
        }

        g_env->CallVoidMethod(held, g_m.weapon_setHitChance,
            features.perfect_accuracy ? jint{ 100 } : g_weapon_state.hit_chance);
        g_env->CallVoidMethod(held, g_m.weapon_setRecoilDelay,
            features.perfect_accuracy ? jint{ 0 } : g_weapon_state.recoil_delay);
        g_env->CallVoidMethod(held, g_m.weapon_setAimingTime,
            features.perfect_accuracy ? jint{ 0 } : g_weapon_state.aiming_time);
        g_env->CallVoidMethod(held, g_m.weapon_setProjectileSpread,
            features.perfect_accuracy ? 0.0f : g_weapon_state.projectile_spread);
        g_env->CallVoidMethod(held, g_m.weapon_setMinDamage,
            features.one_hit ? 1000.0f : g_weapon_state.min_damage);
        g_env->CallVoidMethod(held, g_m.weapon_setMaxDamage,
            features.one_hit ? 1000.0f : g_weapon_state.max_damage);
        const bool ok = !g_env->ExceptionCheck();
        if (!ok) g_env->ExceptionClear();
        g_env->DeleteLocalRef(held);
        return ok;
    }

    static void apply_aim_assist(const survival_features& features,
        const std::vector<entity>& entities)
    {
        if (!features.aim_assist || features.aim_assist_max_dist <= 0.0f ||
            !g_m.player_isAiming || !g_m.char_setForwardDirection) {
            return;
        }

        const auto player = static_cast<jobject>(
            g_env->CallStaticObjectMethod(g_cls.isoPlayer, g_m.player_getInstance));
        if (g_env->ExceptionCheck() || !player) {
            g_env->ExceptionClear();
            return;
        }
        const jboolean aiming = g_env->CallBooleanMethod(player,
            g_m.player_isAiming);
        if (g_env->ExceptionCheck() || aiming != JNI_TRUE) {
            g_env->ExceptionClear();
            g_env->DeleteLocalRef(player);
            return;
        }

        const entity* target = nullptr;
        for (const auto& candidate : entities) {
            if (candidate.type != entity_type::zombie ||
                candidate.health <= 0.0f || !std::isfinite(candidate.dist) ||
                candidate.dist > features.aim_assist_max_dist) {
                continue;
            }
            if (!target || candidate.dist < target->dist) target = &candidate;
        }
        if (target) {
            const float x = g_env->CallFloatMethod(player, g_m.char_getX);
            const float y = g_env->CallFloatMethod(player, g_m.char_getY);
            if (!g_env->ExceptionCheck()) {
                const auto direction = aim_direction_to(
                    x, y, target->wx, target->wy);
                if (direction.valid) {
                    g_env->CallVoidMethod(player, g_m.char_setForwardDirection,
                        direction.x, direction.y);
                }
            }
        }
        if (g_env->ExceptionCheck()) g_env->ExceptionClear();
        g_env->DeleteLocalRef(player);
    }

    static bool set_current_weapon_ammo_to_max()
    {
        if (!g_m.char_getPrimaryHandItem || !g_cls.handWeapon ||
            !g_m.handweapon_getMaxAmmo || !g_m.inv_setCurrentAmmoCount) {
            return false;
        }

        const auto player = static_cast<jobject>(
            g_env->CallStaticObjectMethod(g_cls.isoPlayer, g_m.player_getInstance));
        if (g_env->ExceptionCheck() || !player) {
            g_env->ExceptionClear();
            return false;
        }

        const auto held = static_cast<jobject>(
            g_env->CallObjectMethod(player, g_m.char_getPrimaryHandItem));
        if (g_env->ExceptionCheck() || !held) {
            g_env->ExceptionClear();
            g_env->DeleteLocalRef(player);
            return false;
        }

        const bool is_weapon = g_env->IsInstanceOf(held, g_cls.handWeapon) == JNI_TRUE;
        if (g_env->ExceptionCheck() || !is_weapon) {
            g_env->ExceptionClear();
            g_env->DeleteLocalRef(held);
            g_env->DeleteLocalRef(player);
            return false;
        }

        const jint max_ammo = g_env->CallIntMethod(held, g_m.handweapon_getMaxAmmo);
        if (g_env->ExceptionCheck() || max_ammo <= 0) {
            g_env->ExceptionClear();
            g_env->DeleteLocalRef(held);
            g_env->DeleteLocalRef(player);
            return false;
        }

        g_env->CallVoidMethod(held, g_m.inv_setCurrentAmmoCount, max_ammo);
        const bool ok = !g_env->ExceptionCheck();
        if (!ok) g_env->ExceptionClear();
        g_env->DeleteLocalRef(held);
        g_env->DeleteLocalRef(player);
        return ok;
    }

    void apply_survival_features(const survival_features& features,
        const std::vector<entity>& entities)
    {
        if (!pzj::ensure_resolved()) return;

        pzj::jframe frame{ 64 };
        if (!frame.ok) return;

        // Climate override must run EVERY frame (no throttle) because the
        // game server overwrites climate values every tick. If we only set
        // them at 10Hz the server wins in between and the effect flickers.
        const bool want_climate = features.full_bright ||
            (features.night_vision && !features.full_bright);
        const bool had_climate = g_last_features.full_bright ||
            g_last_features.night_vision;
        if (want_climate || had_climate) {
            if (features.full_bright) {
                static_cast<void>(apply_climate_override(true));
            } else if (features.night_vision) {
                constexpr std::array<float, 6> nv{ 0.0f, 0.85f, 0.2f, 0.7f, 0.9f, 0.5f };
                static_cast<void>(apply_climate_override(true, nv));
            } else {
                static_cast<void>(apply_climate_override(false));
            }
        }

        apply_aim_assist(features, entities);

        // Throttle everything else to 10Hz (100ms)
        using clock = std::chrono::steady_clock;
        static auto next_hold = clock::time_point{};
        const auto now = clock::now();
        const bool changed = !same_features(features, g_last_features);
        if (!changed && now < next_hold) return;

        if (features.zombie_ignore && !g_zombie_state_saved &&
            g_m.system_zombiesDontAttack) {
            g_zombie_state = g_env->GetStaticBooleanField(
                g_cls.systemDisabler, g_m.system_zombiesDontAttack);
            if (g_env->ExceptionCheck()) {
                g_env->ExceptionClear();
            } else {
                g_zombie_state_saved = true;
            }
        }
        if (features.zombie_ignore && g_zombie_state_saved) {
            g_env->SetStaticBooleanField(g_cls.systemDisabler,
                g_m.system_zombiesDontAttack, JNI_TRUE);
            if (g_env->ExceptionCheck()) g_env->ExceptionClear();
        } else if (!features.zombie_ignore && g_zombie_state_saved) {
            g_env->SetStaticBooleanField(g_cls.systemDisabler,
                g_m.system_zombiesDontAttack, g_zombie_state);
            if (g_env->ExceptionCheck()) {
                g_env->ExceptionClear();
            } else {
                g_zombie_state_saved = false;
            }
        }

        const bool wants_cheatset = features.noclip || features.invisible ||
            features.unlimited_endurance || features.instant_actions ||
            features.unlimited_carry || features.no_reload;
        const bool cheatset_active = g_noclip_applied || g_invisible_applied ||
            g_endurance_applied || g_instant_actions_applied || g_carry_applied ||
            g_ammo_applied;
        const bool wants_weight = features.anti_overload;
        const bool needs_player = features.god_mode || features.anti_hunger ||
            wants_weight || features.anti_thirst || features.auto_heal ||
            features.infinite_ammo || features.anti_fatigue || features.all_needs ||
            wants_cheatset || cheatset_active ||
            weapon_features_enabled(features) || g_weapon_state.weapon ||
            g_invincible_saved || g_maxweight_saved;
        jobject player = nullptr;
        if (needs_player) {
            player = g_env->CallStaticObjectMethod(
                g_cls.isoPlayer, g_m.player_getInstance);
            if (g_env->ExceptionCheck()) {
                g_env->ExceptionClear();
                player = nullptr;
            }
        }

        if (player && g_survival_player &&
            !g_env->IsSameObject(player, g_survival_player) &&
            !restore_player_state()) {
            return;
        }
        const bool holds_player_state = features.god_mode || wants_weight ||
            wants_cheatset;
        if (player && !g_survival_player && holds_player_state) {
            g_survival_player = g_env->NewGlobalRef(player);
            if (g_env->ExceptionCheck()) {
                g_env->ExceptionClear();
                g_survival_player = nullptr;
            }
        }

        // --- God mode: invincible field (ungated) + health/heal hold ---
        if (player && features.god_mode) {
            if (g_survival_player && !g_invincible_saved && g_m.char_invincible) {
                g_invincible_state = g_env->GetBooleanField(
                    g_survival_player, g_m.char_invincible);
                if (g_env->ExceptionCheck()) g_env->ExceptionClear();
                else g_invincible_saved = true;
            }
            if (g_survival_player && g_invincible_saved && g_m.char_invincible) {
                g_env->SetBooleanField(g_survival_player,
                    g_m.char_invincible, JNI_TRUE);
                if (g_env->ExceptionCheck()) g_env->ExceptionClear();
            }
            if (g_m.char_setHealth)
                g_env->CallVoidMethod(player, g_m.char_setHealth, 100.0f);
            if (g_m.char_getBodyDamage && g_m.bodydamage_RestoreToFullHealth) {
                const auto damage = static_cast<jobject>(
                    g_env->CallObjectMethod(player, g_m.char_getBodyDamage));
                if (!g_env->ExceptionCheck() && damage) {
                    g_env->CallVoidMethod(damage,
                        g_m.bodydamage_RestoreToFullHealth);
                    g_env->DeleteLocalRef(damage);
                }
            }
            if (g_env->ExceptionCheck()) g_env->ExceptionClear();
        } else if (!features.god_mode && g_survival_player &&
            g_invincible_saved && g_m.char_invincible) {
            g_env->SetBooleanField(g_survival_player, g_m.char_invincible,
                g_invincible_state);
            if (g_m.char_setHealth)
                g_env->CallVoidMethod(g_survival_player, g_m.char_setHealth, 100.0f);
            if (g_env->ExceptionCheck()) g_env->ExceptionClear();
            else g_invincible_saved = false;
        }

        // --- Unlimited carry / Anti overload: force maxWeight + maxWeightBase
        //     directly (ungated). BodyDamage recomputes maxWeight from
        //     maxWeightBase, so both must be forced to defeat the recompute. ---
        if (player && wants_weight && g_survival_player &&
            g_m.char_getMaxWeight && g_m.char_setMaxWeight) {
            if (!g_maxweight_saved) {
                g_maxweight_value = g_env->CallIntMethod(
                    g_survival_player, g_m.char_getMaxWeight);
                g_maxweightbase_value = g_m.char_maxWeightBase
                    ? g_env->GetIntField(g_survival_player, g_m.char_maxWeightBase)
                    : 0;
                if (g_env->ExceptionCheck()) g_env->ExceptionClear();
                else g_maxweight_saved = true;
            }
            const jint huge = static_cast<jint>(
                feature_policy::anti_overload_weight());
            g_env->CallVoidMethod(g_survival_player, g_m.char_setMaxWeight, huge);
            if (g_m.char_maxWeightBase)
                g_env->SetIntField(g_survival_player, g_m.char_maxWeightBase, huge);
            if (g_env->ExceptionCheck()) g_env->ExceptionClear();
        } else if (!wants_weight && g_survival_player && g_maxweight_saved &&
            g_m.char_setMaxWeight) {
            g_env->CallVoidMethod(g_survival_player, g_m.char_setMaxWeight,
                g_maxweight_value);
            if (g_m.char_maxWeightBase)
                g_env->SetIntField(g_survival_player, g_m.char_maxWeightBase,
                    g_maxweightbase_value);
            if (g_env->ExceptionCheck()) g_env->ExceptionClear();
            else g_maxweight_saved = false;
        }

        const bool needs_active = features.anti_hunger || features.anti_thirst ||
            features.anti_fatigue || features.all_needs;
        if (player && needs_active && g_m.char_getStats && g_m.stats_set) {
            const auto stats = static_cast<jobject>(
                g_env->CallObjectMethod(player, g_m.char_getStats));
            if (!g_env->ExceptionCheck() && stats) {
                const auto force = [&](jfieldID stat_field, float value) {
                    if (!stat_field) return;
                    const auto stat = g_env->GetStaticObjectField(
                        g_cls.characterStat, stat_field);
                    if (stat) {
                        g_env->CallBooleanMethod(stats, g_m.stats_set, stat, value);
                        g_env->DeleteLocalRef(stat);
                    }
                };
                if (features.anti_hunger || features.all_needs)
                    force(g_m.stat_hunger, 0.0f);
                if (features.anti_thirst || features.all_needs)
                    force(g_m.stat_thirst, 0.0f);
                if (features.anti_fatigue || features.all_needs) {
                    force(g_m.stat_fatigue, 0.0f);
                    force(g_m.stat_endurance, 1.0f);
                }
                if (features.all_needs) {
                    force(g_m.stat_boredom, 0.0f);
                    force(g_m.stat_unhappiness, 0.0f);
                    force(g_m.stat_stress, 0.0f);
                    force(g_m.stat_panic, 0.0f);
                    force(g_m.stat_sickness, 0.0f);
                    force(g_m.stat_wetness, 0.0f);
                }
                if (g_env->ExceptionCheck()) g_env->ExceptionClear();
                g_env->DeleteLocalRef(stats);
            } else {
                g_env->ExceptionClear();
            }
        }

        if (features.infinite_ammo)
            static_cast<void>(set_current_weapon_ammo_to_max());

        // Auto heal: cure all diseases, infections, wounds every tick
        if (player && features.auto_heal)
            apply_auto_heal(player);

        static_cast<void>(apply_weapon_features(player, features));

        // --- PlayerCheats family (NO_CLIP / INVISIBLE / UNLIMITED_ENDURANCE /
        //     TIMED_ACTION_INSTANT / UNLIMITED_CARRY / UNLIMITED_AMMO). Their
        //     read path is gated by isCheatAllowed() = Core.debug || client ||
        //     server. We ONLY set the backing EnumSet membership here; the
        //     explicit Debug bypass toggle is what forces Core.debug so they
        //     take effect. Individual cheats never touch Core.debug. ---
        if (features.debug_bypass && g_cls.core && g_m.core_debug) {
            if (!g_debug_saved) {
                g_debug_state = g_env->GetStaticBooleanField(
                    g_cls.core, g_m.core_debug);
                if (g_env->ExceptionCheck()) g_env->ExceptionClear();
                else g_debug_saved = true;
            }
            g_env->SetStaticBooleanField(g_cls.core, g_m.core_debug, JNI_TRUE);
            if (g_env->ExceptionCheck()) g_env->ExceptionClear();
        }

        if (player) {
            const auto drive_cheat = [&](jfieldID field, bool want, bool& applied) {
                if (!field) return;
                if (want) {
                    if (set_player_cheat(player, field, true)) applied = true;
                } else if (applied) {
                    if (set_player_cheat(player, field, false)) applied = false;
                }
            };
            drive_cheat(g_m.cheat_noclip, features.noclip, g_noclip_applied);
            drive_cheat(g_m.cheat_invisible, features.invisible,
                g_invisible_applied);
            drive_cheat(g_m.cheat_endurance, features.unlimited_endurance,
                g_endurance_applied);
            drive_cheat(g_m.cheat_instant_actions, features.instant_actions,
                g_instant_actions_applied);
            drive_cheat(g_m.cheat_carry, features.unlimited_carry,
                g_carry_applied);
            drive_cheat(g_m.cheat_ammo, features.no_reload, g_ammo_applied);
        }

        // Core.debug is driven ONLY by the explicit Debug bypass toggle, never
        // by individual cheats. The cheats above merely set their EnumSet flag;
        // they take effect once Debug bypass forces Core.debug (isCheatAllowed).
        const bool still_needs_debug = features.debug_bypass;
        if (!still_needs_debug && g_debug_saved && g_cls.core && g_m.core_debug) {
            g_env->SetStaticBooleanField(g_cls.core, g_m.core_debug, g_debug_state);
            if (g_env->ExceptionCheck()) g_env->ExceptionClear();
            else g_debug_saved = false;
        }

        if (g_survival_player && !g_invincible_saved && !g_maxweight_saved &&
            !g_noclip_applied && !g_invisible_applied &&
            !g_endurance_applied && !g_instant_actions_applied &&
            !g_carry_applied && !g_ammo_applied) {
            g_env->DeleteGlobalRef(g_survival_player);
            g_survival_player = nullptr;
        }
        if (g_env->ExceptionCheck()) g_env->ExceptionClear();
        g_last_features = features;
        next_hold = now + std::chrono::milliseconds{ 100 };
    }

    void refill_ammo()
    {
        if (!pzj::ensure_resolved()) return;

        pzj::jframe fr;
        if (!fr.ok) return;

        if (set_current_weapon_ammo_to_max())
            pzlog2::log("refill_ammo -> max");
    }

    void grant_admin()
    {
        if (!pzj::ensure_resolved()) return;

        pzj::jframe fr{ 16 };
        if (!fr.ok || !g_m.player_accessLevel) return;

        const auto player = static_cast<jobject>(
            g_env->CallStaticObjectMethod(g_cls.isoPlayer, g_m.player_getInstance));
        if (g_env->ExceptionCheck() || !player) {
            g_env->ExceptionClear();
            return;
        }

        const auto admin = g_env->NewStringUTF("admin");
        if (admin) {
            g_env->SetObjectField(player, g_m.player_accessLevel, admin);
            g_env->DeleteLocalRef(admin);
        }
        if (g_env->ExceptionCheck()) g_env->ExceptionClear();
        g_env->DeleteLocalRef(player);
        pzlog2::log("grant_admin -> accessLevel=admin");
    }

    void reveal_map()
    {
        if (!pzj::ensure_resolved()) return;
        pzj::jframe fr{ 16 };
        if (!fr.ok || !g_cls.worldMapVisited || !g_m.worldmap_getInstance ||
            !g_m.worldmap_getMinX || !g_m.worldmap_getMinY ||
            !g_m.worldmap_maxX || !g_m.worldmap_maxY ||
            !g_m.worldmap_setKnownInCells || !g_m.worldmap_setVisitedInCells) {
            return;
        }
        const auto visited = static_cast<jobject>(g_env->CallStaticObjectMethod(
            g_cls.worldMapVisited, g_m.worldmap_getInstance));
        if (g_env->ExceptionCheck() || !visited) {
            g_env->ExceptionClear();
            return;
        }
        const jint min_x = g_env->CallIntMethod(visited, g_m.worldmap_getMinX);
        const jint min_y = g_env->CallIntMethod(visited, g_m.worldmap_getMinY);
        const jint max_x = g_env->GetIntField(visited, g_m.worldmap_maxX);
        const jint max_y = g_env->GetIntField(visited, g_m.worldmap_maxY);
        if (g_env->ExceptionCheck()) {
            g_env->ExceptionClear();
            g_env->DeleteLocalRef(visited);
            return;
        }
        g_env->CallVoidMethod(visited, g_m.worldmap_setKnownInCells,
            min_x, min_y, max_x, max_y);
        g_env->CallVoidMethod(visited, g_m.worldmap_setVisitedInCells,
            min_x, min_y, max_x, max_y);
        if (g_env->ExceptionCheck()) g_env->ExceptionClear();
        g_env->DeleteLocalRef(visited);
        pzlog2::log("reveal_map -> cells [%d,%d]..[%d,%d]",
            static_cast<int>(min_x), static_cast<int>(min_y),
            static_cast<int>(max_x), static_cast<int>(max_y));
    }

    // ---- skill / perk editor ------------------------------------------------
    static std::vector<std::string> g_perk_names;
    static std::vector<jobject> g_perks; // global refs into PerkFactory.PerkList

    static bool build_perks()
    {
        if (!g_perks.empty()) return true;
        if (!g_cls.perkFactory || !g_m.perkfactory_list ||
            !g_m.perkfactory_getName || !pzj::ensure_list_methods()) {
            return false;
        }
        const auto list = static_cast<jobject>(g_env->GetStaticObjectField(
            g_cls.perkFactory, g_m.perkfactory_list));
        if (g_env->ExceptionCheck() || !list) {
            if (g_env->ExceptionCheck()) g_env->ExceptionClear();
            return false;
        }
        const jint n = g_env->CallIntMethod(list, g_m.list_size);
        if (g_env->ExceptionCheck() || n <= 0) {
            g_env->ExceptionClear();
            g_env->DeleteLocalRef(list);
            return false;
        }
        for (jint i = 0; i < n; ++i) {
            const auto perk = static_cast<jobject>(
                g_env->CallObjectMethod(list, g_m.list_get, i));
            if (g_env->ExceptionCheck() || !perk) {
                g_env->ExceptionClear();
                continue;
            }
            char name[64]{};
            const auto jname = static_cast<jstring>(g_env->CallStaticObjectMethod(
                g_cls.perkFactory, g_m.perkfactory_getName, perk));
            const bool named = !g_env->ExceptionCheck() && jname &&
                pzj::jstr_to_utf8(jname, name, sizeof(name)) && name[0];
            if (g_env->ExceptionCheck()) g_env->ExceptionClear();
            if (jname) g_env->DeleteLocalRef(jname);
            if (!named || ::strcmp(name, "None") == 0) {
                g_env->DeleteLocalRef(perk);
                continue;
            }
            const auto global = g_env->NewGlobalRef(perk);
            g_env->DeleteLocalRef(perk);
            if (!global) { g_env->ExceptionClear(); continue; }
            g_perks.push_back(global);
            g_perk_names.emplace_back(name);
        }
        g_env->DeleteLocalRef(list);
        return !g_perks.empty();
    }

    int perk_count()
    {
        if (!pzj::ensure_resolved()) return 0;
        pzj::jframe fr{ 16 };
        if (!fr.ok) return 0;
        return build_perks() ? static_cast<int>(g_perks.size()) : 0;
    }

    const char* perk_name(int idx)
    {
        if (idx < 0 || idx >= static_cast<int>(g_perk_names.size())) return "";
        return g_perk_names[static_cast<std::size_t>(idx)].c_str();
    }

    int perk_level(int idx)
    {
        if (!pzj::ensure_resolved()) return 0;
        pzj::jframe fr{ 16 };
        if (!fr.ok || !build_perks() || !g_m.char_getPerkLevel ||
            idx < 0 || idx >= static_cast<int>(g_perks.size())) {
            return 0;
        }
        const auto player = static_cast<jobject>(g_env->CallStaticObjectMethod(
            g_cls.isoPlayer, g_m.player_getInstance));
        if (g_env->ExceptionCheck() || !player) {
            g_env->ExceptionClear();
            return 0;
        }
        const jint level = g_env->CallIntMethod(player, g_m.char_getPerkLevel,
            g_perks[static_cast<std::size_t>(idx)]);
        if (g_env->ExceptionCheck()) { g_env->ExceptionClear(); }
        g_env->DeleteLocalRef(player);
        return static_cast<int>(level);
    }

    void set_perk_level(int idx, int level)
    {
        if (!pzj::ensure_resolved()) return;
        pzj::jframe fr{ 16 };
        if (!fr.ok || !build_perks() || !g_m.char_setPerkLevelDebug ||
            idx < 0 || idx >= static_cast<int>(g_perks.size())) {
            return;
        }
        const jint clamped = static_cast<jint>(std::clamp(level, 0, 10));
        const auto player = static_cast<jobject>(g_env->CallStaticObjectMethod(
            g_cls.isoPlayer, g_m.player_getInstance));
        if (g_env->ExceptionCheck() || !player) {
            g_env->ExceptionClear();
            return;
        }
        g_env->CallVoidMethod(player, g_m.char_setPerkLevelDebug,
            g_perks[static_cast<std::size_t>(idx)], clamped);
        if (g_env->ExceptionCheck()) g_env->ExceptionClear();
        g_env->DeleteLocalRef(player);
    }

    bool shutdown()
    {
        if (!g_env) return true;

        if (pzj::g_resolved) {
            pzj::jframe frame{ 64 };
            if (!frame.ok) return false;

            bool restored = apply_climate_override(false);
            if (g_zombie_state_saved) {
                if (!g_m.system_zombiesDontAttack) {
                    restored = false;
                } else {
                    g_env->SetStaticBooleanField(g_cls.systemDisabler,
                        g_m.system_zombiesDontAttack, g_zombie_state);
                    if (g_env->ExceptionCheck()) {
                        g_env->ExceptionClear();
                        restored = false;
                    } else {
                        g_zombie_state_saved = false;
                    }
                }
            }
            if (!restore_player_state()) restored = false;
            if (!restore_weapon_state()) restored = false;
            if (g_invisible_applied || g_noclip_applied) {
                const auto player = static_cast<jobject>(
                    g_env->CallStaticObjectMethod(g_cls.isoPlayer,
                        g_m.player_getInstance));
                if (!g_env->ExceptionCheck() && player) {
                    set_player_cheat(player, g_m.cheat_invisible, false);
                    set_player_cheat(player, g_m.cheat_noclip, false);
                    g_env->DeleteLocalRef(player);
                } else {
                    g_env->ExceptionClear();
                }
                g_invisible_applied = false;
                g_noclip_applied = false;
            }
            if (g_debug_saved && g_m.core_debug && g_cls.core) {
                g_env->SetStaticBooleanField(g_cls.core, g_m.core_debug,
                    g_debug_state);
                if (g_env->ExceptionCheck()) {
                    g_env->ExceptionClear();
                    restored = false;
                } else {
                    g_debug_saved = false;
                }
            }
            if (g_env->ExceptionCheck()) {
                g_env->ExceptionClear();
                restored = false;
            }
            if (!restored || climate_state_saved() || g_zombie_state_saved ||
                g_invincible_saved || g_maxweight_saved || g_noclip_applied ||
                g_invisible_applied || g_endurance_applied ||
                g_instant_actions_applied || g_carry_applied || g_ammo_applied ||
                g_debug_saved ||
                g_weapon_state.weapon) {
                pzlog2::log("feature restoration incomplete; retrying shutdown");
                return false;
            }
        } else if (climate_state_saved() || g_zombie_state_saved ||
            g_invincible_saved || g_maxweight_saved || g_noclip_applied ||
            g_invisible_applied || g_endurance_applied ||
            g_instant_actions_applied || g_carry_applied || g_ammo_applied ||
            g_debug_saved ||
            g_weapon_state.weapon) {
            return false;
        }

        if (g_item_source) {
            g_env->DeleteGlobalRef(g_item_source);
            g_item_source = nullptr;
        }
        g_items.clear();
        g_item_cursor = 0;
        g_item_total = 0;
        g_items_built = false;
        for (auto* const perk : g_perks) {
            if (perk) g_env->DeleteGlobalRef(perk);
        }
        g_perks.clear();
        g_perk_names.clear();
        g_last_features = {};
        g_climate_state = {};
        g_zombie_state = JNI_FALSE;
        g_invincible_state = JNI_FALSE;
        g_debug_state = JNI_FALSE;
        g_invisible_applied = false;
        g_noclip_applied = false;
        g_endurance_applied = false;
        g_instant_actions_applied = false;
        g_carry_applied = false;
        g_ammo_applied = false;
        g_maxweight_saved = false;
        g_maxweight_value = 0;
        g_maxweightbase_value = 0;
        g_weapon_state = {};
        g_frame_ctx = {};
        g_seen_world = false;

        const std::array<jclass*, 34> classes{
            &g_cls.isoPlayer,
            &g_cls.isoZombie,
            &g_cls.isoGameCharacter,
            &g_cls.isoMovingObject,
            &g_cls.isoWorld,
            &g_cls.isoCell,
            &g_cls.isoGridSquare,
            &g_cls.isoCamera,
            &g_cls.gameClient,
            &g_cls.climateManager,
            &g_cls.climateFloat,
            &g_cls.scriptManager,
            &g_cls.itemScript,
            &g_cls.inventoryItem,
            &g_cls.handWeapon,
            &g_cls.itemFactory,
            &g_cls.bodyDamage,
            &g_cls.bodyPart,
            &g_cls.bodyPartType,
            &g_cls.stats,
            &g_cls.characterStat,
            &g_cls.systemDisabler,
            &g_cls.core,
            &g_cls.vehicleManager,
            &g_cls.baseVehicle,
            &g_cls.isoAnimal,
            &g_cls.worldInventoryObject,
            &g_cls.isoUtils,
            &g_cls.playerCheats,
            &g_cls.cheatType,
            &g_cls.enumSet,
            &g_cls.perkFactory,
            &g_cls.worldMapVisited,
            nullptr
        };
        for (auto* const cls : classes) {
            if (cls && *cls) {
                g_env->DeleteGlobalRef(*cls);
                *cls = nullptr;
            }
        }
        if (pzj::g_class_loader) {
            g_env->DeleteGlobalRef(pzj::g_class_loader);
            pzj::g_class_loader = nullptr;
        }
        pzj::g_load_class = nullptr;
        pzj::g_resolved = false;
        g_m = {};

        if (pzj::g_attached_here && pzj::g_vm) {
            const jint detach_result = pzj::g_vm->DetachCurrentThread();
            if (detach_result != JNI_OK) {
                pzlog2::log("DetachCurrentThread failed (%d)",
                    static_cast<int>(detach_result));
                return false;
            }
        }
        pzj::g_attached_here = false;
        g_env = nullptr;
        pzj::g_vm = nullptr;
        pzlog2::log("JNI bridge and feature state fully shut down");
        return true;
    }

} // namespace pz
