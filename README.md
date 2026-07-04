# Granular — 24-voice Scale-Quantized Granular Synth (OTODESK)

パステル・ドット波形GUIと「変調後グレインピッチ量子化」を核にしたグラニュラーシンセ。
JUCE 8.0系 / VST3 + Standalone / C++20。

## 特徴

- **24音ポリフォニー** × ボイスあたり最大32グレイン（ボイススチーリング付き）
- **グレインピッチ量子化 (QUANTIZE)**
  ノート + オクターブ/セミ/ファイン + ランダムジッターの**変調後の最終ピッチ**を
  Key + Scale の構成音へスナップ。Key は手動 (C〜B) と Auto（素材解析結果）両対応。
  スケール16種（Major / Minor / ペンタ / チャーチモード / Harmonic Minor / Whole Tone / Quartal 等）
- **高音質化**: グレイン読み出しは4点Hermite補間（Tessellatorの最近傍から改良）
- **決定論的グレインスケジューラ**: Densityをオーバーラップ量として扱い、
  低密度でもテクスチャが安定（±25%のゆらぎ付き）
- **サンプル下処理**: 読み込み時に Omni-Analyzer（Tessellator移植）でピッチ解析し、
  **Signalsmith Stretch** でC3へ高品質ピッチ補正 → -6dB正規化 → 無音トリム
- **パステル・ドット波形GUI**:
  波形を小さなドットの縦積みで表現（mint→blue→lavender→pinkのグラデーション）。
  グレイン発火をリアルタイムにドットで可視化（ピッチ→高さ・色、時間減衰）。
  波形クリック/ドラッグで Position 操作、サンプルはD&DまたはLOADボタン。

## ビルド (Windows / Visual Studio 2022)

1. JUCE 8.0系を `C:/JUCE` に配置（別の場所なら CMake の `JUCE_PATH` を変更）
2. Visual Studio で「フォルダーを開く」→ このフォルダを選択（CMakeSettings.json を自動認識）
3. 構成 `x64-Release` を選び、ビルド
4. `COPY_PLUGIN_AFTER_BUILD TRUE` なので VST3 は自動で
   `C:\Program Files\Common Files\VST3` にコピーされます（Ableton Liveで再スキャン）

## フォルダ構成

```
Granular/
├─ CMakeLists.txt
├─ CMakeSettings.json
└─ Source/
   ├─ PluginProcessor.h/.cpp     … APVTS / サンプル下処理スレッド
   ├─ PluginEditor.h/.cpp        … レイアウト / HUD
   ├─ DSP/
   │  ├─ GranularEngine.h/.cpp   … 24ボイス・グレインエンジン
   │  ├─ ScaleQuantizer.h        … Key/Scale量子化（変調後適用）
   │  ├─ PitchAnalyzer.h/.cpp    … Omni-Analyzer（Tessellator移植）
   │  └─ GrainVisualizerData.h   … 音声→GUI ロックフリーイベント
   ├─ GUI/
   │  ├─ ColorPalette.h          … パステルテーマ
   │  ├─ ArcDial.h/.cpp          … アークダイアル LookAndFeel
   │  └─ DotWaveformDisplay.h/.cpp … ドット波形＋グレイン可視化
   └─ third_party/
      └─ signalsmith-stretch/    … MITライセンス（下記参照）
```

## ライセンスについて

- **Signalsmith Stretch / Signalsmith Linear** (MIT License, Signalsmith Audio Ltd.)
  無料配布・ソース非公開・将来の有料化すべて可。
  配布時はドキュメント等に著作権表示を含めてください
  （`Source/third_party/signalsmith-stretch/LICENSE-*.txt` を同梱すれば十分）。
- RubberBand は使用していないため、GPL/商用ライセンスの制約はありません。

## ロードマップ（Quanta 2超えに向けて）

- [ ] LFO ×2〜4 + モジュレーションマトリクス（量子化は変調後なので相性◎）
- [ ] FXスロット（Chorus / Delay / Reverb / Freeze — Tessellatorから移植可能）
- [ ] グレインごとのフィルター、ステレオソース対応の強化
- [ ] プリセットブラウザ、リサイズ対応GUI
- [ ] MPE / ピッチベンド対応
