# Granular — 24-voice Scale-Quantized Granular Synth (OTODESK)

パステル・ドット波形GUIと「変調後グレインピッチ量子化」を核にしたグラニュラーシンセ。
JUCE 8.0系 / VST3 + Standalone / C++20。

## 特徴

- **24音ポリフォニー** × ボイスあたり最大**100グレイン**（Quanta 2同等・ボイススチーリング付き）
  - Density指数カーブ（オーバーラップ 0.25〜64）、1/√overlap音量正規化
  - 全ボイス合計1200グレインのCPUバジェットで暴走防止
- **音程感強調（ノイズ系素材を演奏可能な音程に）**
  - **SYNC**: グレイン発火間隔をノート周波数に同期（pitch-synchronous granular）。
    発火の周期性がピッチとして知覚され、ホワイトノイズでも音程になる
  - **TONE**: ノート同調コムレゾネーター（Karplus-Strong系）。弦的な倍音構造を付与
- **タブUI**: MAIN / MOD / FX（ドット波形は常時表示、下部のみ切り替え）
- **グレインピッチ量子化 (QUANTIZE)**
  ノート + オクターブ/セミ/ファイン + ランダムジッターの**変調後の最終ピッチ**を
  Key + Scale の構成音へスナップ。Key は手動 (C〜B) と Auto（素材解析結果）両対応。
  スケール16種（Major / Minor / ペンタ / チャーチモード / Harmonic Minor / Whole Tone / Quartal 等）
- **高音質化**: グレイン読み出しは4点Hermite補間（Tessellatorの最近傍から改良）
- **決定論的グレインスケジューラ**: Densityをオーバーラップ量として扱い、
  低密度でもテクスチャが安定（±25%のゆらぎ付き）
- **Root Note Tracking**: 補正はセント偏差のみ（音質ほぼ無劣化）。
  検出ノートをルートとして記憶し、そのノートを弾くと原音の高さで再生。
  表示はAbleton基準（C3 = MIDI 60）
- **高精度ピッチ解析**: 正規化自己相関＋オクターブエラーガード＋3窓中央値。
  ファイル名Keyヒント（"Pad-C4" "Bass(D)" "Am_pluck" "Ebmaj"等）でKeyを確定し
  解析は高さの特定に専念
- **PadEnvelope**: Attackはべき乗カーブ最大10秒（ゆっくり立ち上がるPAD）〜
  最短0.5ms（歯切れのよい発音）。Decay/Releaseは指数カーブ
- **Start/End**: ノブ＋波形上のマーカードラッグでサンプル範囲を調整
- **マスターリミッター**: ブリックウォール、Ceiling -0.1dBデフォルト
- **UI**: 5タブ構成 MAIN / ARP / MOD / FX / CONFIG。
  MAIN=グレイン・ピッチ・エンベロープ・POSITION/START/END・マスター、
  ARP=アルペジエイター＋QUANTIZE(Key/Scale)＋TEXTURE(WINDOW/TONE/P-SYNC/REV)、
  CONFIG=MATERIAL解析モード・Limiter CEILING。
  全ノブ右クリックで数値入力、トグルはLED点灯式、
  ヘッダーに LOAD / ANALYZE / DELETE / RESET（破壊的操作はY/N確認付き）
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

## ロードマップ（Quanta 2超えに向けて / 優先順）

1. [x] **ARP**: アルペジエイター13種（Up/Down/Up-Down/Down-Up/Up-Down Incl/
       Converge/Diverge/Pedal Low/Pedal High/As Played/Random/Random Walk/Chord）、
       TempoSync（付点・三連対応）/Free(Hz)、Duration（Gate%/音符）、Latch、
       Octaveレンジ1-4、出力ノートはQuantize Key/Scaleへスナップ
2. [ ] **MODタブ**: LFO×4（テンポ同期、Sine/Tri/Saw/Sqr/S&H/Chaos）＋ ループ可能Env×2
       ＋ 16スロットマトリクス（Velocity/Note/ModWheel/Random/Macro×4）
       ※量子化は変調後に行われるため、Pitchへの変調もすべてスケールに乗る
3. [ ] **FXタブ**: 4スロット式 — Shimmer Reverb / Tape Delay / Ensemble Chorus /
       Freeze・Smear。**適用順はD&Dで並べ替え可能**
4. [ ] **Configタブ**: カラーテーマ変更、同時発音数、Limiter Ceiling、解析ロジック設定
5. [ ] グレインループのSIMD化（グレイン主導ブロック処理への再設計、2〜3倍高速化見込み）
6. [ ] プリセットブラウザ、リサイズGUI、MPE/ピッチベンド
