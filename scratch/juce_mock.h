#pragma once
#include <algorithm>
#include <cmath>
#include <vector>
#include <string>
#include <array>
#include <iostream>
#include <initializer_list>

#define JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ClassName)

namespace juce
{
    // MathConstants は struct テンプレートにする
    template <typename T>
    struct MathConstants
    {
        static constexpr T pi = T(3.14159265358979323846);
        static constexpr T twoPi = T(6.28318530717958647692);
        static constexpr T halfPi = T(1.57079632679489661923);
    };

    template <typename T>
    inline T jlimit(T min, T max, T val) { return (val < min) ? min : ((val > max) ? max : val); }

    // 2変数および3変数以上の jmax
    template <typename T>
    inline T jmax(T a, T b) { return std::max(a, b); }
    
    template <typename T, typename... Args>
    inline T jmax(T a, T b, Args... args) { return std::max(a, jmax(b, args...)); }

    template <typename T>
    inline T jmin(T a, T b) { return std::min(a, b); }
    
    template <typename T, typename... Args>
    inline T jmin(T a, T b, Args... args) { return std::min(a, jmin(b, args...)); }

    inline int roundToInt(double x) { return (int)std::round(x); }
    inline int roundToInt(float x) { return (int)std::round(x); }

    struct String {
        std::string s;
        String() = default;
        String(const char* c) : s(c) {}
        String(const std::string& str) : s(str) {}
        bool operator==(const String& other) const { return s == other.s; }
        const char* toRawUTF8() const { return s.c_str(); }
    };
    struct StringArray {
        std::vector<String> arr;
        StringArray() = default;
        StringArray(std::initializer_list<String> list) : arr(list) {}
        void add(const String& str) { arr.push_back(str); }
        int size() const { return (int)arr.size(); }
        const String& operator[](int idx) const { return arr[idx]; }
        static StringArray fromTokens(const String&, const char*) { return StringArray(); }
    };

    namespace Decibels
    {
        inline float decibelsToGain(float db) { return std::pow(10.0f, db * 0.05f); }
        inline float gainToDecibels(float gain) { return 20.0f * std::log10(gain); }
    }

    struct Random
    {
        float nextFloat() { return (float)rand() / (float)RAND_MAX; }
        int nextInt(int maxVal) { return maxVal > 0 ? rand() % maxVal : 0; }
        bool nextBool() { return rand() % 2 == 0; }
    };

    // MidiMessage & MidiBuffer のモック
    struct MidiMessage
    {
        int noteNumber = 60;
        float velocity = 0.8f;
        int state = 0; // 1=On, 2=Off
        
        static MidiMessage noteOn(int, int note, float vel) {
            MidiMessage m; m.noteNumber = note; m.velocity = vel; m.state = 1; return m;
        }
        static MidiMessage noteOff(int, int note) {
            MidiMessage m; m.noteNumber = note; m.state = 2; return m;
        }
        bool isNoteOn() const { return state == 1; }
        bool isNoteOff() const { return state == 2; }
        bool isAllNotesOff() const { return false; }
        bool isAllSoundOff() const { return false; }
        int getNoteNumber() const { return noteNumber; }
        float getFloatVelocity() const { return velocity; }
    };

    struct MidiBuffer
    {
        struct Element {
            MidiMessage message;
            int samplePosition;
        };
        std::vector<Element> events;
        void clear() { events.clear(); }
        void addEvent(const MidiMessage& msg, int pos) { events.push_back({msg, pos}); }
        
        struct Iterator {
            const MidiBuffer& buf;
            size_t idx = 0;
            bool operator!=(const Iterator& other) const { return idx != other.idx; }
            void operator++() { ++idx; }
            Element operator*() const { return buf.events[idx]; }
        };
        Iterator begin() const { return Iterator{*this, 0}; }
        Iterator end() const { return Iterator{*this, events.size()}; }
    };

    template <typename T>
    class AudioBuffer
    {
        std::vector<std::vector<T>> data;
        int numChannels = 0;
        int numSamples = 0;
    public:
        AudioBuffer() = default;
        AudioBuffer(int channels, int samples) { setSize(channels, samples); }
        void setSize(int channels, int samples, bool keep = false, bool clear = false, bool avoid = false)
        {
            numChannels = channels;
            numSamples = samples;
            data.assign(channels, std::vector<T>(samples, 0));
        }
        int getNumChannels() const { return numChannels; }
        int getNumSamples() const { return numSamples; }
        const T* getReadPointer(int ch) const { return data[ch].data(); }
        T* getWritePointer(int ch) { return data[ch].data(); }
        T getSample(int ch, int sample) const { return data[ch][sample]; }
        void setSample(int ch, int sample, T val) { data[ch][sample] = val; }
        void clear() { for(auto& v : data) std::fill(v.begin(), v.end(), 0); }
        void copyFrom(int destCh, int destStart, const AudioBuffer& src, int srcCh, int srcStart, int num)
        {
            std::copy(src.data[srcCh].begin() + srcStart, src.data[srcCh].begin() + srcStart + num, data[destCh].begin() + destStart);
        }
        void addFrom(int destCh, int destStart, const AudioBuffer& src, int srcCh, int srcStart, int num, float gain = 1.0f)
        {
            for(int i=0; i<num; ++i)
                data[destCh][destStart + i] += src.data[srcCh][srcStart + i] * gain;
        }
        void applyGain(int ch, int start, int num, float gain)
        {
            for(int i=0; i<num; ++i) data[ch][start + i] *= gain;
        }
    };
};
