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

extern "C" {
        #include <SDL.h>
        #include <SDL_thread.h>
        #include <SDL_mixer.h>
};

std::mutex audioMutex;
std::thread st_stt_Thread, st_sto_Thread, rc_stt_Thread, rc_sto_Thread, bf_stt_Thread, bf_sto_Thread, ps_stt_Thread, ps_sto_Thread, ;
std::atomic_int queue = 0;

OBS_DECLARE_MODULE()

void obs_module_unload(void) {
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
        SDL_Quit();
        return;
}

const char * obs_module_author(void) {
        return "EBK21";
}

const char * obs_module_name(void) {
        return "Stream/Recording Start/Stop Beeps";
}

const char * obs_module_description(void) {
        return "Adds audio sound when streaming/recording/buffer starts/stops or when recording is paused/unpaused.";
}

void play_clip(const char * filepath) {
        thread_local Mix_Music * music;
        ++queue;
        audioMutex.lock();
        if (queue == 1) {
                if (Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 1, 1024) != 0) {
                        audioMutex.unlock();
                        --queue;
                        return;
                }
        }
        audioMutex.unlock();
        if (!(music = Mix_LoadMUS(filepath))) {
                --queue;
                return;
        }
        Mix_PlayMusic(music, 0);
        while (Mix_PlayingMusic() == 1) {
                SDL_Delay(100 + 20 * (queue - 1));
        }
        Mix_FreeMusic(music);
        --queue;
        if (queue == 0) Mix_CloseAudio();
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

bool obs_module_load(void) {
        obs_frontend_add_event_callback(obsstudio_srbeep_frontend_event_callback, 0);
        return true;
}
