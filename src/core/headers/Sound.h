#pragma once
#include <Common.h>

#define ALSA_PCM_NEW_HW_PARAMS_API
#include <alsa/asoundlib.h>

void AlsaLogCall(const char* function, const char* file, i32 line, i32 err) {
	if(err < 0) {
		std::cerr << err << " Alsa error: " << snd_strerror(err) << " " << function << " file: " << file << " line: " << line << std::endl;
		__builtin_trap();
	}
}

struct SoundAsset {
    i16* buffer;
    u32 frameCount;
    u32 sampleRate;
    u32 instanceCount;
};
struct SoundInstance {
    SoundAsset* asset;
    u32 currentFrame;
    f32 volume;
    bool loop;
};

struct SoundTable {
    SoundAsset asset;
    SoundInstance* activeSounds = nullptr;
    u32 count = 0;
    u32 cap = 0;
};

struct SoundSystemState {
	snd_pcm_t* handle;
	snd_pcm_hw_params_t* params;
	snd_pcm_uframes_t frames;
	snd_pcm_uframes_t hwBufferSizeFrames;
	u32 bufferSize;
    u32 sampleRate;
    u32 channels;
    u32 periodTime;
    u32 periodSize;
	i16* buffer;
};




void InitSound(SoundSystemState& sound) {
	
    i32 ret;
    i32 dir;
    char** hints;
    char** n;
    char* name;
    char* desc;
    char* ioid;

    ALSA_CALL(snd_pcm_open(&sound.handle, "default", SND_PCM_STREAM_PLAYBACK, SND_PCM_NONBLOCK));
	snd_pcm_hw_params_alloca(&sound.params);
	ALSA_CALL(snd_pcm_hw_params_any(sound.handle, sound.params));
    
	ALSA_CALL(snd_pcm_hw_params_set_access(sound.handle, sound.params, SND_PCM_ACCESS_RW_INTERLEAVED));
	ALSA_CALL(snd_pcm_hw_params_set_format(sound.handle, sound.params, SND_PCM_FORMAT_S16_LE));
	ALSA_CALL(snd_pcm_hw_params_set_channels(sound.handle, sound.params, 2));
	sound.sampleRate = 48000;
	ALSA_CALL(snd_pcm_hw_params_set_rate_near(sound.handle, sound.params,&sound.sampleRate, &dir));
    
	sound.frames = 32;
	ALSA_CALL(snd_pcm_hw_params_set_period_size_near(sound.handle, sound.params, &sound.frames, &dir));
	ALSA_CALL(snd_pcm_hw_params(sound.handle, sound.params));
    ALSA_CALL(snd_pcm_hw_params_get_buffer_size(sound.params , &sound.hwBufferSizeFrames ));
	ALSA_CALL(snd_pcm_hw_params_get_period_size(sound.params, &sound.frames,&dir));
    sound.periodSize = sound.frames;
	sound.bufferSize = sound.frames * 4; /* 2 bytes/sample, 2 channels */
	sound.buffer = (i16*)aligned_alloc(16 , sound.bufferSize);
    MemSet(sound.buffer , 0 , sound.bufferSize );

	ALSA_CALL(snd_pcm_hw_params_get_period_time(sound.params, &sound.periodTime, &dir));
}


void PushSound(SoundTable& table, SoundInstance instance) {

    if(table.count+1 > table.cap ) {
        table.cap++;
        table.cap *= 2;
        auto tmp = (SoundInstance*)malloc( table.cap * sizeof(SoundInstance) );

        for(u32 i = 0; i < table.count ; i++) {
            tmp[i] = table.activeSounds[i];
        }

        free(table.activeSounds);
        table.activeSounds = tmp;
    }
    
    instance.asset->instanceCount++;
    table.activeSounds[table.count++] = instance;
}


void FreeSoundAsset(SoundAsset* asset) {
    free(asset->buffer);
    asset->buffer = nullptr;
    asset->frameCount = 0;
    asset->sampleRate = 0;
    asset->instanceCount = 0;
}

void FWide(SoundSystemState& sound, SoundTable& table) {

    for(u32 i = 0; i < sound.frames; i+=2) {

        f32 dst0 = 0;
        f32 dst1 = 0;
        f32 dst2 = 0;
        f32 dst3 = 0;

        for(u32 k = 0; k < table.count ; k++) {

            if( table.activeSounds[k].currentFrame + 2 >= table.activeSounds[k].asset->frameCount ) {
                if( table.activeSounds[k].loop ) {
                    table.activeSounds[k].currentFrame = 0;
                }
                else {
                    table.activeSounds[k].asset->instanceCount--;
                    if( table.activeSounds[k].asset->instanceCount == 0 ) {
                        FreeSoundAsset(table.activeSounds[k].asset);
                    }
                    table.activeSounds[k] = table.activeSounds[--table.count];
                    if(table.count > 0) {
                        dst0 += table.activeSounds[k].asset->buffer[ (table.activeSounds[k].currentFrame << 1) + 0 ] * table.activeSounds[k].volume;
                        dst1 += table.activeSounds[k].asset->buffer[ (table.activeSounds[k].currentFrame << 1) + 1 ] * table.activeSounds[k].volume;
                        dst2 += table.activeSounds[k].asset->buffer[ (table.activeSounds[k].currentFrame << 1) + 2 ] * table.activeSounds[k].volume;
                        dst3 += table.activeSounds[k].asset->buffer[ (table.activeSounds[k].currentFrame << 1) + 3 ] * table.activeSounds[k].volume;
                        table.activeSounds[k].currentFrame += 2;
                    }
                }
            }
            else {
                dst0 += table.activeSounds[k].asset->buffer[ (table.activeSounds[k].currentFrame << 1) + 0 ] * table.activeSounds[k].volume;
                dst1 += table.activeSounds[k].asset->buffer[ (table.activeSounds[k].currentFrame << 1) + 1 ] * table.activeSounds[k].volume;
                dst2 += table.activeSounds[k].asset->buffer[ (table.activeSounds[k].currentFrame << 1) + 2 ] * table.activeSounds[k].volume;
                dst3 += table.activeSounds[k].asset->buffer[ (table.activeSounds[k].currentFrame << 1) + 3 ] * table.activeSounds[k].volume;
                table.activeSounds[k].currentFrame += 2;
            }
        }

        sound.buffer[i * 2 + 0] = dst0 / table.count;
        sound.buffer[i * 2 + 1] = dst1 / table.count;
        sound.buffer[i * 2 + 2] = dst2 / table.count;
        sound.buffer[i * 2 + 3] = dst3 / table.count;
    }
}

void PlaySounds(SoundSystemState& sound, SoundTable& table) {

    i32 ret = snd_pcm_writei(sound.handle, sound.buffer , sound.frames);

    if(ret != -EAGAIN ) {
        FWide(sound, table);
    }

    if (ret == -EPIPE) {
        std::cout << "underrun" << std::endl;
        snd_pcm_prepare(sound.handle);
    }
    else if (ret < 0) {
        snd_strerror(ret);
    }  
    else if (ret != (int)sound.frames) {
        std::cout << "short write: " << sound.frames << std::endl;
    }
}

void LoadSoundAsset(const char* filePath , SoundAsset* asset) {

    auto file = fopen(filePath, "r" );
    fseek(file , 0 ,SEEK_END );
    u32 size = ftell(file);
    asset->frameCount = (size >> 2) + ((size >> 2) & 1); // frame == 2 byte(R) + 2 byte(L)
    asset->buffer = (i16*)aligned_alloc( 16 , asset->frameCount*4 ); // malloc
    asset->sampleRate = 48000;
    asset->instanceCount = 0;

    fseek(file , 44 , SEEK_SET);
    fread(asset->buffer , size , 1 , file);
    asset->buffer[ (asset->frameCount << 1) - 1] = asset->buffer[ (asset->frameCount << 1) - 2] * 0.1;
    asset->buffer[ (asset->frameCount << 1) - 0] = asset->buffer[ (asset->frameCount << 1) - 1] * 0.1;
    fclose(file);
}