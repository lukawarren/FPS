#pragma once
#include "common.h"

class Audio
{
public:
    Audio();
    ~Audio();

    enum class ID
    {
        AMBIENCE = 0,
        STEPS_BEGIN = 1,
        STEPS_FINAL = 24,
        SHOOT,
        LIGHT
    };

    // (path, loops) pairs
    static inline constexpr std::array<std::pair<const char*, bool>, 27> AUDIO_NAMES =
    {{
        { "wind woosh loop.mp3", true },
        { "metal_steps_01.wav", false },
        { "metal_steps_02.wav", false },
        { "metal_steps_03.wav", false },
        { "metal_steps_04.wav", false },
        { "metal_steps_05.wav", false },
        { "metal_steps_06.wav", false },
        { "metal_steps_07.wav", false },
        { "metal_steps_08.wav", false },
        { "metal_steps_09.wav", false },
        { "metal_steps_10.wav", false },
        { "metal_steps_11.wav", false },
        { "metal_steps_12.wav", false },
        { "metal_steps_13.wav", false },
        { "metal_steps_14.wav", false },
        { "metal_steps_15.wav", false },
        { "metal_steps_16.wav", false },
        { "metal_steps_17.wav", false },
        { "metal_steps_18.wav", false },
        { "metal_steps_19.wav", false },
        { "metal_steps_20.wav", false },
        { "metal_steps_21.wav", false },
        { "metal_steps_22.wav", false },
        { "metal_steps_23.wav", false },
        { "metal_steps_24.wav", false },
        { "shot_01.mp3", false },
        { "Sci-Fi Drone.wav", true }
    }};

    void play(const ID id, const float pitch = 1.0f);

    void play_3d(
        const ID id,
        const ma_vec3f position,
        const float pitch = 1.0f,
        const float min_distance = 1.0f,
        const float max_distance = 500.0f
    );

    void set_listener(
        const ma_vec3f position,
        const ma_vec3f direction,
        const ma_vec3f up = { 0.0f, 1.0f, 0.0f }
    );

    void update();

private:
    struct ActiveSound
    {
        ma_resource_manager_data_source* data_source;
        ma_sound* sound;
    };

    ma_engine engine;
    ma_resource_manager resource_manager;

    std::unordered_map<ID, ma_resource_manager_data_source*> cache_data_sources;
    std::unordered_map<ID, ma_resource_manager_data_source*> looping_data_sources;
    std::unordered_map<ID, ma_sound*> looping_sounds;
    std::vector<ActiveSound> active_sounds;

    void create_instance(
        const ID id,
        ma_resource_manager_data_source*& out_data_source,
        ma_sound*& out_sound,
        const bool looping,
        const bool spatial
    );
};