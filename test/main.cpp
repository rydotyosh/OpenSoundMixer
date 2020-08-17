#define NOMINMAX
#include <algorithm>
#include <chrono>
#include <functional>
#include <string>
#include <thread>
#include <vector>

#include <OpenSoundMixer.h>

#ifdef WIN32
#include <Windows.h>
#include <direct.h>
#else
#include <sys/types.h>
#include <unistd.h>
#endif

inline void Sleep(int32_t ms) {
    std::chrono::milliseconds d(ms);
    std::this_thread::sleep_for(d);
}

template <class T>
void SafeAddRef(T& t) {
    if (t != NULL) {
        t->AddRef();
    }
}

template <class T>
void SafeRelease(T& t) {
    if (t != NULL) {
        t->Release();
        t = NULL;
    }
}

std::string GetDirectoryName(const std::string& path) {
    const std::string::size_type pos = std::max<int32_t>(path.find_last_of('/'), path.find_last_of('\\'));
    return (pos == std::string::npos) ? std::string() : path.substr(0, pos + 1);
}

std::string GetExecutingDirectory() {
    char buf[260];

#ifdef _WIN32
    int len = GetModuleFileNameA(NULL, buf, 260);
    if (len <= 0) return "";
#else

    char temp[32];
    sprintf(temp, "/proc/%d/exe", getpid());
    int bytes = std::min((int)readlink(temp, buf, 260), 260 - 1);
    if (bytes >= 0) buf[bytes] = '\0';
#endif

    return GetDirectoryName(buf);
}

void SetCurrentDir(const char* path) {
#ifdef _WIN32
    _chdir(path);
#else
    chdir(path);
#endif
}

int main(int argc, char** argv) {
    osm::SetLogger([](osm::LogType logType, const char* message) { printf("%s\n", message); });

    SetCurrentDir(GetExecutingDirectory().c_str());

    osm::Sound* staticSound = nullptr;
    osm::Sound* streamSound = nullptr;

    auto manager = osm::Manager::Create();

    if (manager->Initialize()) {
        printf("Scceed in initializing manager.\n");
    } else {
        printf("Failed to initialize manager.\n");
        manager->Release();
        return 0;
    }

    {
        FILE* fp = nullptr;

#if _WIN32
        fopen_s(&fp, "../se1.wav", "rb");
#else
        fp = fopen("se1.wav", "rb");
#endif
        if (fp == nullptr) return 0;

        fseek(fp, 0, SEEK_END);
        auto size = ftell(fp);
        fseek(fp, 0, SEEK_SET);

        std::vector<uint8_t> data;
        data.resize(size);
        fread(data.data(), size, 1, fp);
        fclose(fp);

        staticSound = manager->CreateSound(data.data(), data.size(), true);
    }

    {
        FILE* fp = nullptr;
#if _WIN32
        fopen_s(&fp, "../bgm1.ogg", "rb");
#else
        fp = fopen("bgm1.ogg", "rb");
#endif
        if (fp == nullptr) return 0;

        fseek(fp, 0, SEEK_END);
        auto size = ftell(fp);
        fseek(fp, 0, SEEK_SET);

        std::vector<uint8_t> data;
        data.resize(size);
        fread(data.data(), size, 1, fp);
        fclose(fp);

        streamSound = manager->CreateSound(data.data(), data.size(), true);
    }

    printf("Loaded resources.\n");

    auto id1 = manager->Play(streamSound);
    manager->FadeIn(id1, 3);
    Sleep(1000);
    auto id2 = manager->Play(staticSound);

    
    // Seeking Test
    //================================================================================
    Sleep(1000);
    manager->SetPlaybackPosition(id1, 0);
    //================================================================================
    

    // Procedural Test
    //================================================================================
    Sleep(1000);
    struct ob {
        int m_count=0;
        void proc(osm::Sample* samples, int count) { 
            for (int i = 0; i < count; ++i) {
                double v = 0;
                int n = 44100 / 6;
                double beat = m_count % n / (double)n;
                int tick = m_count / n % 4;
                double vels[] = {0.8, 0.4, 1.0, 0.4};
                v += (std::rand() / (double)RAND_MAX - 0.5) * 5000.0 * pow(1 - beat, 2) * vels[tick];
                double beat2 = (m_count + n * 2) % (n * 4) / (double)(n * 4);
                v += pow(sin(m_count * 0.005), 0.2) * 10000.0 * pow(1 - beat2, 2);
                samples[i].Left = v;
                samples[i].Right = v;
                m_count++;
            }
        }
        static void func(void* x, osm::Sample* samples, int count) {
            ((ob*)x)->proc(samples,count);
        }
    } obj;
    osm::Sound* synth = manager->CreateProceduralSound({&obj, &ob::func});
    manager->Play(synth);
    //================================================================================

    // Fast Fourier Transform Test
    //================================================================================
    Sleep(1000);
    std::vector<float> spectrums(32768);
    manager->GetSpectrum(id1, spectrums, osm::FFTWindow::Rectangular);
    for(int i = 0; i < 32768; ++i)
        printf("spectrums[%05d] = %f\n", i, spectrums[i]);
    //================================================================================
    

    while (manager->IsPlaying(id1) || manager->IsPlaying(id2)) {
        // current playback position
        // printf("percent %f\n", manager->GetPlaybackPercent(id1));
        Sleep(1);
    }

    /*
    int32_t time = 0;
    while (true)
    {
            if (time % 500 == 0)
            {
                    manager->Play(streamSound);
            }
            Sleep_(1);
            time++;
    }
    */

    manager->Finalize();
    staticSound->Release();
    streamSound->Release();
    manager->Release();

	osm::SetLogger(nullptr);

    return 0;
}
