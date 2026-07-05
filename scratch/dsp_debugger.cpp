#include <JuceHeader.h>
#include "DSP/GranularEngine.h"
#include "DSP/Arpeggiator.h"
#include "DSP/FxChain.h"
#include "DSP/ModMatrix.h"
#include <iostream>
#include <random>
#include <cmath>
#include <cstdlib>
#include <csignal>

// シグナルハンドラ（クラッシュ時に原因を標準出力に書き出す）
void signalHandler(int signum)
{
    std::cout << "\n[CRASH DETECTED] Signal: " << signum << std::endl;
    if (signum == SIGABRT) {
        std::cout << "SIGABRT: Assertion failure or abort() called." << std::endl;
    } else if (signum == SIGSEGV) {
        std::cout << "SIGSEGV: Invalid memory access." << std::endl;
    } else if (signum == SIGFPE) {
        std::cout << "SIGFPE: Floating point exception (e.g. division by zero)." << std::endl;
    }
    std::cout << std::flush;
    std::exit(signum);
}

// シミュレータ用のヘルパークラス
class DspDebugger
{
public:
    DspDebugger()
    {
        double sampleRate = 44100.0;
        int maxBlockSize = 512;

        // 1. ダミーのソースオーディオバッファを作成 (1秒間のサイン波)
        sourceBuffer.setSize(2, (int)sampleRate);
        float* srcL = sourceBuffer.getWritePointer(0);
        float* srcR = sourceBuffer.getWritePointer(1);
        for (int i = 0; i < sourceBuffer.getNumSamples(); ++i)
        {
            float val = std::sin(2.0f * juce::MathConstants<float>::pi * 220.0f * (float)i / (float)sampleRate);
            srcL[i] = val;
            srcR[i] = val;
        }

        // 2. DSPオブジェクトのセットアップ
        synth.prepareToPlay(sampleRate, maxBlockSize);
        arp.prepare(sampleRate);
        fxChain.prepare(sampleRate);
        masterLimiter.prepare(sampleRate);

        // 3. テストバッファ
        testBuffer.setSize(2, maxBlockSize);
    }

    void runSimulation(int numBlocks)
    {
        std::mt19937 gen(54321); // 固定シードで再現性を確保
        std::uniform_real_distribution<float> dis01(0.0f, 1.0f);
        std::uniform_real_distribution<float> disMinus1To1(-1.0f, 1.0f);
        std::uniform_int_distribution<int> disMidi(36, 84);
        std::uniform_int_distribution<int> disFXType(0, 5);

        double sampleRate = 44100.0;

        std::cout << "Starting simulation loop of " << numBlocks << " blocks..." << std::endl;

        for (int block = 0; block < numBlocks; ++block)
        {
            testBuffer.clear();

            // 1. MIDIバッファの構築 (ランダムにノートオン・ノートオフを追加してボイススチールを誘発)
            midiMessages.clear();
            if (dis01(gen) < 0.3f)
            {
                int note = disMidi(gen);
                float vel = dis01(gen);
                midiMessages.addEvent(juce::MidiMessage::noteOn(1, note, vel), gen() % 512);
            }
            if (dis01(gen) < 0.2f)
            {
                int note = disMidi(gen);
                midiMessages.addEvent(juce::MidiMessage::noteOff(1, note), gen() % 512);
            }

            // 2. ARPパラメータのランダム急変 (全ノブシミュレーション)
            ArpParams arpP;
            arpP.enable = dis01(gen) > 0.3f; // 頻繁にON/OFF
            arpP.pattern = gen() % 13;
            arpP.octaves = 1 + (gen() % 4);
            arpP.sync = dis01(gen) > 0.5f;
            arpP.rateSync = gen() % 13;
            arpP.rateFree = 0.5f + dis01(gen) * 29.5f;
            arpP.durMode = gen() % 5;
            arpP.gate = 0.05f + dis01(gen) * 0.95f; // Gateノブの激しい変動
            arpP.latch = dis01(gen) > 0.8f;
            arpP.key = gen() % 13;
            arpP.scale = gen() % 5;
            arpP.bpm = 60.0 + dis01(gen) * 120.0;

            arp.process(midiMessages, 512, arpP);

            // 3. グラニュラーエンジンパラメータのランダム急変 (全ノブシミュレーション)
            GranularParams p;
            p.attack = 0.0005f + dis01(gen) * 2.0f;
            p.decay = 0.001f + dis01(gen) * 2.0f;
            p.sustain = dis01(gen);
            p.release = 0.001f + dis01(gen) * 2.0f;
            p.grainSize = 0.02f + dis01(gen) * 0.48f;
            p.density = dis01(gen);
            p.position = dis01(gen);
            p.positionSpray = dis01(gen);
            p.pitchJitter = dis01(gen) * 2400.0f;
            p.panSpread = dis01(gen);
            p.speed = dis01(gen) * 2.0f;
            p.playMode = gen() % 4;
            p.speedSync = dis01(gen) > 0.5f;
            p.speedSyncDiv = gen() % 6;
            p.reverseGrain = dis01(gen) > 0.5f;
            p.windowShape = gen() % 4;
            p.octave = -3 + (gen() % 7);
            p.pitchSt = -24 + (gen() % 49);
            p.fineTune = -100.0f + dis01(gen) * 200.0f;
            p.pitchMod = disMinus1To1(gen) * 24.0f;
            p.outGainDb = -36.0f + dis01(gen) * 48.0f;
            p.pitchSync = dis01(gen) > 0.5f;
            p.tone = dis01(gen);
            p.masterHPF = 20.0f + dis01(gen) * 1980.0f;
            p.masterLPF = 200.0f + dis01(gen) * 19800.0f;
            p.filterSlope = gen() % 2;
            p.poly = 1 + (gen() % 24); // 同時発音数の急変

            int effS = (int)(dis01(gen) * 10000.0f);
            int effE = effS + 2000 + (int)(dis01(gen) * 20000.0f);

            // 4. エンジン処理実行
            synth.processBlock(testBuffer, midiMessages, &sourceBuffer, effS, effE, p);

            // 5. FXパラメータのランダム急変 (全ノブシミュレーション)
            FxChain::Params fxP;
            for (int s = 0; s < FxChain::kNumSlots; ++s)
            {
                fxP.type[s] = disFXType(gen);
                fxP.amount[s] = dis01(gen); // 0〜1のフル変調
            }
            fxP.bpm = arpP.bpm;
            fxP.satAlgo = gen() % 10;
            fxP.satDrive = 1.0f + dis01(gen) * 11.0f;
            fxP.satPreHz = 20.0f + dis01(gen) * 1980.0f;
            fxP.satTrimDb = -12.0f + dis01(gen) * 24.0f;
            fxP.choRate = 0.05f + dis01(gen) * 3.95f;
            fxP.choDepth = dis01(gen);
            fxP.choWidth = dis01(gen);
            fxP.dlyTime = gen() % 10;
            fxP.dlyFeedback = dis01(gen) * 0.95f;
            fxP.dlyDuck = dis01(gen);
            fxP.dlyDamp = dis01(gen);
            fxP.frzSize = 20.0f + dis01(gen) * 980.0f;
            fxP.frzFeedback = dis01(gen) * 0.99f;
            fxP.frzDamp = dis01(gen);
            fxP.revDecay = dis01(gen);
            fxP.revShimmer = dis01(gen);
            fxP.revDamp = dis01(gen);
            fxP.revMod = dis01(gen);

            // 6. FX処理実行
            fxChain.process(testBuffer, fxP);

            // 7. リミッター
            masterLimiter.setReleaseMs(50.0f);
            float ceilingLin = juce::Decibels::decibelsToGain(-0.1f);
            float* dL = testBuffer.getWritePointer(0);
            float* dR = testBuffer.getWritePointer(1);
            for (int i = 0; i < 512; ++i)
            {
                masterLimiter.process(dL[i], dR[i], ceilingLin);
            }

            // 8. 異常信号の検知 (NaN, Inf, もしくはリミッターをすり抜けた極端な振幅の監視)
            for (int ch = 0; ch < 2; ++ch)
            {
                const float* ptr = testBuffer.getReadPointer(ch);
                for (int i = 0; i < 512; ++i)
                {
                    float val = ptr[i];
                    if (std::isnan(val) || std::isinf(val) || std::abs(val) > 2.0f)
                    {
                        std::cout << "\n--- [ERROR DETECTED] Block: " << block << " Sample: " << i << " Channel: " << ch << " Val: " << val << " ---" << std::endl;
                        std::cout << "Engine parameters:" << std::endl;
                        std::cout << "  Tone: " << p.tone << " | PitchSync: " << p.pitchSync << " | Density: " << p.density << std::endl;
                        std::cout << "  Position: " << p.position << " | Spray: " << p.positionSpray << std::endl;
                        std::cout << "  Polyphony: " << p.poly << " | OutGain: " << p.outGainDb << std::endl;
                        std::cout << "FX parameters:" << std::endl;
                        for (int s = 0; s < FxChain::kNumSlots; ++s)
                        {
                            std::cout << "  Slot " << s << " -> Type: " << fxP.type[s] << " Amount: " << fxP.amount[s] << std::endl;
                        }
                        std::cout << "  Delay Feedback: " << fxP.dlyFeedback << std::endl;
                        std::cout << "  Freeze Feedback: " << fxP.frzFeedback << " Size: " << fxP.frzSize << std::endl;
                        std::cout << "  Reverb Decay: " << fxP.revDecay << " Shimmer: " << fxP.revShimmer << std::endl;
                        
                        std::cout << "Exiting with error code..." << std::endl;
                        std::cout << std::flush;
                        std::exit(1);
                    }
                }
            }
        }
        std::cout << "SUCCESS: " << numBlocks << " blocks processed with zero anomalies." << std::endl;
        std::cout << std::flush;
    }

private:
    juce::AudioBuffer<float> sourceBuffer;
    juce::AudioBuffer<float> testBuffer;
    juce::MidiBuffer midiMessages;

    GranularSynth synth;
    Arpeggiator arp;
    FxChain fxChain;
    BrickLimiter masterLimiter;
};

int main(int argc, char* argv[])
{
    // 標準出力をノンバッファリング（即時書き出し）に設定
    std::cout << std::unitbuf;
    std::cerr << std::unitbuf;

    // クラッシュ用シグナルハンドラの登録
    std::signal(SIGABRT, signalHandler);
    std::signal(SIGSEGV, signalHandler);
    std::signal(SIGFPE,  signalHandler);

    std::cout << "Starting DSP debugging simulation (Plain main with Signal Handler)..." << std::endl;
    std::cout << std::flush;

    try
    {
        DspDebugger debugger;
        debugger.runSimulation(50000);
    }
    catch (const std::exception& e)
    {
        std::cout << "EXCEPTION CAUGHT: " << e.what() << std::endl;
        std::cout << std::flush;
        return 1;
    }
    catch (...)
    {
        std::cout << "UNKNOWN EXCEPTION CAUGHT" << std::endl;
        std::cout << std::flush;
        return 1;
    }

    return 0;
}
