#include "audio.h"

Audio::Audio()
{
    // Engine
    ma_result result = ma_engine_init(NULL, &engine);
    if (result != MA_SUCCESS)
        throw std::runtime_error("Failed to initialise audio");

    ma_device* device = ma_engine_get_device(&engine);

    // Audio manager; re-sample, etc. at load-time as opposed to live
    ma_resource_manager_config config;
    config = ma_resource_manager_config_init();
    config.decodedFormat = device->playback.format;
    config.decodedChannels = device->playback.channels;
    config.decodedSampleRate = device->sampleRate;

    result = ma_resource_manager_init(&config, &resource_manager);
    if (result != MA_SUCCESS)
    {
        ma_device_uninit(device);
        throw std::runtime_error("Failed to initialise audio resource manager");
    }

    for (size_t i = 0; i < AUDIO_NAMES.size(); i++)
    {
        const ID id = (ID)i;
        const std::string path = AUDIO_ROOT + AUDIO_NAMES[i].first;
        const bool looping = AUDIO_NAMES[i].second;

        u32 flags = MA_RESOURCE_MANAGER_DATA_SOURCE_FLAG_DECODE |
            (looping ? MA_RESOURCE_MANAGER_DATA_SOURCE_FLAG_LOOPING : 0);

        ma_resource_manager_data_source* data_source =
            (ma_resource_manager_data_source*)malloc(sizeof(ma_resource_manager_data_source));

        result = ma_resource_manager_data_source_init(
            &resource_manager,
            path.c_str(),
            flags,
            NULL,
            data_source
        );

        if (result != MA_SUCCESS)
            throw std::runtime_error(
                "Failed to load audio file " + std::string(AUDIO_NAMES[i].first)
            );

        cache_data_sources[id] = data_source;

        // Looping sounds (e.g. ambience) get one persistent ma_sound; don't duplicate
        if (looping)
        {
            ma_sound* sound = (ma_sound*)malloc(sizeof(ma_sound));
            result = ma_sound_init_from_data_source(
                &engine,
                data_source,
                MA_SOUND_FLAG_DECODE | MA_SOUND_FLAG_LOOPING,
                NULL,
                sound
            );

            if (result != MA_SUCCESS)
                throw std::runtime_error(
                    "Failed to process audio file " + std::string(AUDIO_NAMES[i].first)
                );

            looping_data_sources[id] = data_source;
            looping_sounds[id] = sound;
        }
    }

    ma_sound_set_volume(looping_sounds[ID::AMBIENCE], 0.2f);
}

void Audio::create_instance(const ID id, ma_resource_manager_data_source*& out_data_source, ma_sound*& out_sound, const bool looping)
{
    const std::string path = AUDIO_ROOT + AUDIO_NAMES[(size_t)id].first;

    u32 flags = MA_RESOURCE_MANAGER_DATA_SOURCE_FLAG_DECODE |
        (looping ? MA_RESOURCE_MANAGER_DATA_SOURCE_FLAG_LOOPING : 0);

    out_data_source = (ma_resource_manager_data_source*)malloc(sizeof(ma_resource_manager_data_source));

    ma_result result = ma_resource_manager_data_source_init(
        &resource_manager,
        path.c_str(),
        flags,
        NULL,
        out_data_source
    );

    if (result != MA_SUCCESS)
    {
        free(out_data_source);
        out_data_source = nullptr;
        out_sound = nullptr;
        return;
    }

    flags = MA_SOUND_FLAG_DECODE | (looping ? MA_SOUND_FLAG_LOOPING : 0);

    out_sound = (ma_sound*)malloc(sizeof(ma_sound));
    result = ma_sound_init_from_data_source(
        &engine,
        out_data_source,
        flags,
        NULL,
        out_sound
    );

    if (result != MA_SUCCESS)
    {
        ma_resource_manager_data_source_uninit(out_data_source);
        free(out_data_source);
        free(out_sound);
        out_data_source = nullptr;
        out_sound = nullptr;
        return;
    }
}

void Audio::play(const ID id, const float pitch)
{
    const bool looping = AUDIO_NAMES[(size_t)id].second;

    if (looping)
    {
        // Single persistent instance - just (re)start it
        ma_sound_seek_to_pcm_frame(looping_sounds[id], 0);
        ma_sound_start(looping_sounds[id]);
        return;
    }

    ma_resource_manager_data_source* data_source = nullptr;
    ma_sound* sound = nullptr;
    create_instance(id, data_source, sound, false);

    if (!sound)
        return;

    if (pitch != 1.0f)
        ma_sound_set_pitch(sound, pitch);

    ma_sound_start(sound);
    active_sounds.push_back({ data_source, sound });
}

void Audio::update()
{
    for (size_t i = 0; i < active_sounds.size();)
    {
        if (ma_sound_at_end(active_sounds[i].sound))
        {
            ma_sound_uninit(active_sounds[i].sound);
            free(active_sounds[i].sound);

            ma_resource_manager_data_source_uninit(active_sounds[i].data_source);
            free(active_sounds[i].data_source);

            active_sounds[i] = active_sounds.back();
            active_sounds.pop_back();
        }
        else
        {
            i++;
        }
    }
}

Audio::~Audio()
{
    for (auto& active : active_sounds)
    {
        ma_sound_uninit(active.sound);
        free(active.sound);

        ma_resource_manager_data_source_uninit(active.data_source);
        free(active.data_source);
    }

    for (auto& sound : looping_sounds)
    {
        ma_sound_uninit(sound.second);
        free(sound.second);
    }

    // looping_data_sources entries are the same pointers as the
    // corresponding cache_data_sources entries, so they're freed once below
    for (auto& source : cache_data_sources)
    {
        ma_resource_manager_data_source_uninit(source.second);
        free(source.second);
    }

    ma_resource_manager_uninit(&resource_manager);
    ma_engine_uninit(&engine);
}