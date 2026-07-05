#include "JuceHeader.h"
#include "DSP/FxChain.h"
#include "DSP/GranularEngine.h" // BrickLimiter のためにインクルード
#include <iostream>
#include <random>
#include <cmath>
#include <cstdlib>

class FxChainDebugger
{
public:
    FxChainDebugger()
    {
        double sampleRate = 44100.0;
        fxChain.prepare(sampleRate);
        testBuffer.setSize(2, 512);
    }

    void runSimulation(int numBlocks)
    {
        std::mt19937 gen(12345);
        std::uniform_real_distribution<float> dis01(0.0f, 1.0f);
        std::uniform_real_distribution<float> disMinus1To1(-1.0f, 1.0f);
        std::uniform_int_distribution<int> disFXType(0, 5);

        std::cout << "Starting Pure C++ FX Simulation of " << numBlocks << " blocks..." << std::endl;

        for (int block = 0; block < numBlocks; ++block)
        {
            // 1. テスト用の入力信号を充填 (サイン波 + ランダムスパイク)
            float* dL = testBuffer.getWritePointer(0);
            float* dR = testBuffer.getWritePointer(1);
            for (int i = 0; i < 512; ++i)
            {
                float val = std::sin(2.0f * 3.14159265f * 440.0f * (float)(block * 512 + i) / 44100.0f) * 0.5f;
                if (dis01(gen) < 0.01f) val += disMinus1To1(gen) * 0.8f;
                dL[i] = val;
                dR[i] = val;
            }

            // 2. FX全パラメータ（全ノブ）のランダム急変
            FxChain::Params fxP;
            for (int s = 0; s < FxChain::kNumSlots; ++s)
            {
                fxP.type[s] = disFXType(gen);
                fxP.amount[s] = dis01(gen); // 0〜1のフル変調
            }
            fxP.bpm = 60.0 + dis01(gen) * 180.0;
            fxP.satAlgo = gen() % 10;
            fxP.satDrive = 1.0f + dis01(gen) * 11.0f;
            fxP.satPreHz = 20.0f + dis01(gen) * 1980.0f;
            fxP.satTrimDb = -12.0f + dis01(gen) * 24.0f;
            fxP.choRate = 0.05f + dis01(gen) * 3.95f;
            fxP.choDepth = dis01(gen);
            fxP.choWidth = dis01(gen);
            fxP.dlyTime = gen() % 10;
            fxP.dlyFeedback = dis01(gen) * 0.99f; // 最大フィードバック
            fxP.dlyDuck = dis01(gen);
            fxP.dlyDamp = dis01(gen);
            fxP.frzSize = 20.0f + dis01(gen) * 980.0f;
            fxP.frzFeedback = dis01(gen) * 0.99f;
            fxP.frzDamp = dis01(gen);
            fxP.revDecay = dis01(gen);
            fxP.revShimmer = dis01(gen);
            fxP.revDamp = dis01(gen);
            fxP.revMod = dis01(gen);

            // 3. FX処理実行
            fxChain.process(testBuffer, fxP);

            // 4. 異常値の検知
            for (int ch = 0; ch < 2; ++ch)
            {
                const float* ptr = testBuffer.getReadPointer(ch);
                for (int i = 0; i < 512; ++i)
                {
                    float val = ptr[i];
                    if (std::isnan(val) || std::isinf(val) || std::abs(val) > 5.0f)
                    {
                        std::cout << "\n--- [ERROR DETECTED] Block: " << block << " Sample: " << i << " Channel: " << ch << " Val: " << val << " ---" << std::endl;
                        std::cout << "FX parameters at failure:" << std::endl;
                        for (int s = 0; s < FxChain::kNumSlots; ++s)
                        {
                            std::cout << "  Slot " << s << " -> Type: " << fxP.type[s] << " Amount: " << fxP.amount[s] << std::endl;
                        }
                        std::cout << "  Delay Feedback: " << fxP.dlyFeedback << " | Damp: " << fxP.dlyDamp << std::endl;
                        std::cout << "  Freeze Feedback: " << fxP.frzFeedback << " Size: " << fxP.frzSize << std::endl;
                        std::cout << "  Reverb Decay: " << fxP.revDecay << " Shimmer: " << fxP.revShimmer << " Damp: " << fxP.revDamp << std::endl;
                        std::cout << "Exiting debug tool (normal exit)..." << std::endl;
                        std::cout << std::flush;
                        std::exit(0); // 正常終了コード0で終わらせることで確実にStdoutを取得する
                    }
                }
            }
        }

        std::cout << "SUCCESS: " << numBlocks << " blocks processed with ZERO anomalies in FX Chain." << std::endl;
        std::cout << std::flush;
    }

private:
    juce::AudioBuffer<float> testBuffer;
    FxChain fxChain;
};

int main()
{
    // 出力バッファリングを無効化
    std::cout << std::unitbuf;

    std::cout << "Initializing Pure C++ FX Debugger..." << std::endl;
    std::cout << std::flush;
    try
    {
        FxChainDebugger debugger;
        debugger.runSimulation(100000); // 10万ブロック (約20分間相当)
    }
    catch (const std::exception& e)
    {
        std::cout << "Exception: " << e.what() << std::endl;
        std::cout << std::flush;
    }
    catch (...)
    {
        std::cout << "Unknown Exception." << std::endl;
        std::cout << std::flush;
    }
    return 0;
}
