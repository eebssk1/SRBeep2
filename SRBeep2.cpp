/***********************************
Orig: A Docile Sloth adocilesloth@gmail.com
Now: EBK21 chkd13303@gmail.com
************************************/

#include <obs-module.h>
#include <obs-frontend-api.h>
#include <thread>
#include <atomic>
#include <sstream>
#include <mutex>

#define likely(x)   __builtin_expect(!!(x), 1)
#define unlikely(x) __builtin_expect(!!(x), 0)

#include <SDL3/SDL.h>
#include <SDL3/SDL_thread.h>
#include <SDL3_mixer/SDL_mixer.h>


#define EXIT_WITH_ERROR(x) do { \
        blog(LOG_ERROR, "Failed to play audio! At %s:%s", __FILE__, x); \
        blog(LOG_ERROR, SDL_GetError()); \
        goto exit; \
} while(0)

std::mutex audioMutex;
std::thread st_stt_Thread, st_sto_Thread, rc_stt_Thread, rc_sto_Thread, bf_stt_Thread, bf_sto_Thread, ps_stt_Thread, ps_sto_Thread;
std::atomic_int queue = 0;

MIX_Mixer* sdlmixer = NULL;
thread_local MIX_Audio* audio = NULL;
thread_local MIX_Track* track = NULL;

OBS_DECLARE_MODULE()

extern "C" void obs_module_unload(void) {
        if (st_stt_Thread.joinable()) {
                st_stt_Thread.join();
        }
        if (st_sto_Thread.joinable()) {
                st_sto_Thread.join();
        }
        if (rc_stt_Thread.joinable()) {
                rc_stt_Thread.join();
        }
        if (rc_sto_Thread.joinable()) {
                rc_sto_Thread.join();
        }
        if (bf_stt_Thread.joinable()) {
                bf_stt_Thread.join();
        }
        if (bf_sto_Thread.joinable()) {
                bf_sto_Thread.join();
        }
        if (ps_stt_Thread.joinable()) {
                ps_stt_Thread.join();
        }
        if (ps_sto_Thread.joinable()) {
                ps_sto_Thread.join();
        }
        MIX_Quit();
        SDL_Quit();
        return;
}

extern "C" const char * obs_module_author(void) {
        return "EBK21";
}

extern "C" const char * obs_module_name(void) {
        return "Stream/Recording Start/Stop Beeps";
}

extern "C" const char * obs_module_description(void) {
        return "Adds audio sound when streaming/recording/buffer starts/stops or when recording is paused/unpaused.";
}

void play_clip(const char * filepath) {
        bool sound = false;
        ++queue;
        audioMutex.lock();
        if (likely(queue == 1)) {
                sdlmixer = MIX_CreateMixerDevice(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, NULL);
                if (unlikely(NULL == sdlmixer)) {
                    audioMutex.unlock();
                    EXIT_WITH_ERROR(__LINE__);
                }
        }
        audioMutex.unlock();
        audio = MIX_LoadAudio(NULL, filepath, false);
        if (unlikely(NULL == audio))
                EXIT_WITH_ERROR(__LINE__);

        track = MIX_CreateTrack(sdlmixer);
        if (unlikely(!track))
                EXIT_WITH_ERROR(__LINE__);

        if (unlikely(!MIX_SetTrackAudio(track, audio)))
                EXIT_WITH_ERROR(__LINE__);

        sound = MIX_PlayTrack(track, 0);

        if (unlikely(!sound))
                EXIT_WITH_ERROR(__LINE__);

        while (MIX_TrackPlaying(track) && sound) {
                SDL_Delay(60 + 30 * (queue - 1));
        }

exit:
        if(likely(NULL != track)) MIX_DestroyTrack(track);
        if(likely(NULL != audio)) MIX_DestroyAudio(audio);
        --queue;
        if (queue == 0 && NULL != sdlmixer) MIX_DestroyMixer(sdlmixer);
        return;
}

std::string clean_path(std::string audio_path) {
        std::string cleaned_path;
        //If relative path then the first 2 chars should be ".."
        if (audio_path.find("..") != std::string::npos) {
                size_t pos = audio_path.find("..");
                cleaned_path = audio_path.substr(pos);
        }
        //If absolute path, Windows will start with a capital, Linux/Mac will start with "/"
        else {
                #ifdef _WIN32
                while (islower(audio_path[0]) && audio_path.length() > 0) {
                        audio_path = audio_path.substr(1);
                }
                #else
                while (audio_path.substr(0, 1) != "/" && audio_path.length() > 0) {
                        audio_path = audio_path.substr(1);
                }
                #endif
                cleaned_path = audio_path;
        }
        return cleaned_path;
}

void play_sound(std::string file_name) {
        const char * obs_data_path = obs_get_module_data_path(obs_current_module());
        std::stringstream audio_path;
        std::string true_path;

        audio_path << obs_data_path;
        audio_path << file_name;
        true_path = clean_path(audio_path.str());
        play_clip(true_path.c_str());
        audio_path.str("");

        return;
}

void obsstudio_srbeep_frontend_event_callback(enum obs_frontend_event event, void * private_data) {
        if (event == OBS_FRONTEND_EVENT_STREAMING_STARTED) {
                if (st_stt_Thread.joinable()) {
                        st_stt_Thread.join();
                }
                st_stt_Thread = std::thread(play_sound, "/stream_start_sound.mp3");
        } else if (event == OBS_FRONTEND_EVENT_RECORDING_STARTED) {
                if (rc_stt_Thread.joinable()) {
                        rc_stt_Thread.join();
                }
                rc_stt_Thread = std::thread(play_sound, "/record_start_sound.mp3");
        } else if (event == OBS_FRONTEND_EVENT_REPLAY_BUFFER_STARTED) {
                if (bf_stt_Thread.joinable()) {
                        bf_stt_Thread.join();
                }
                bf_stt_Thread = std::thread(play_sound, "/buffer_start_sound.mp3");
        } else if (event == OBS_FRONTEND_EVENT_RECORDING_PAUSED) {
                if (ps_stt_Thread.joinable()) {
                        ps_stt_Thread.join();
                }
                ps_stt_Thread = std::thread(play_sound, "/pause_start_sound.mp3");
        } else if (event == OBS_FRONTEND_EVENT_STREAMING_STOPPED) {
                if (st_sto_Thread.joinable()) {
                        st_sto_Thread.join();
                }
                st_sto_Thread = std::thread(play_sound, "/stream_stop_sound.mp3");
        } else if (event == OBS_FRONTEND_EVENT_RECORDING_STOPPED) {
                if (rc_sto_Thread.joinable()) {
                        rc_sto_Thread.join();
                }
                rc_sto_Thread = std::thread(play_sound, "/record_stop_sound.mp3");
        } else if (event == OBS_FRONTEND_EVENT_REPLAY_BUFFER_STOPPED) {
                if (bf_sto_Thread.joinable()) {
                        bf_sto_Thread.join();
                }
                bf_sto_Thread = std::thread(play_sound, "/buffer_stop_sound.mp3");
        } else if (event == OBS_FRONTEND_EVENT_RECORDING_UNPAUSED) {
                if (ps_stt_Thread.joinable()) {
                        ps_stt_Thread.join();
                }
                ps_stt_Thread = std::thread(play_sound, "/pause_stop_sound.mp3");
        }
}

extern "C" bool obs_module_load(void) {
        SDL_Init(0);
        MIX_Init();
        obs_frontend_add_event_callback(obsstudio_srbeep_frontend_event_callback, 0);
        return true;
}
