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
- **プレイヘッド表示**: 発音中の実再生位置をミント色ラインで波形上に表示
  （ピンク線=Position起点、ミント線=動くプレイヘッド）
- **MOTION**: プレイヘッドの動きを Forward / Backward / Ping-Pong / Random から選択。
  S-SYNCオンで「Start-End領域を指定音価（1拍〜8小節）でスキャン」するDAWテンポ追随。
  グレインは領域内をラップ再生（Positionが常に発火起点として尊重される）
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
2. [x] **MODタブ**: LFO×4（テンポ同期1/1〜1/32、Sine/Tri/Saw/Sqr/S&H/Chaos）＋
       ループ可能ADエンベロープ×2 ＋ Macro×4 ＋ MIDIソース（Velocity/Note/ModWheel/
       ノート毎Random）→ **16スロットマトリクス** → 主要パラメータ＋FX Amount×5。
       Amountは±バイポーラ（ダブルクリックで0）。
       **Pitchへの変調は量子化前に加算されるため、暴れてもKey/Scaleに乗る**
3. [x] **FXタブ**: **5スロット式**（FX数と一致）— **ADAA Saturation（アルゴリズム10種・NextGenKick2移植）**/
       Ensemble Chorus / Tape Delay（テンポ同期＋ダッキング）/ Freeze /
       Shimmer Reverb（16ch FDN＋オクターブシフター）。
       **適用順はスロットヘッダーのD&Dで並べ替え可能**。チェーンはLimiter前段に挿入。
       **スロット選択で下部に詳細パラメータが出現**:
       Sat=ALGO/DRIVE/PRE-HPF/TRIM、Chorus=RATE/DEPTH/WIDTH、
       Delay=TIME(1/2〜1/16T)/FEEDBACK/DUCK/DAMP、Freeze=SIZE/FEEDBACK/DAMP、
       Reverb=DECAY/SHIMMER/DAMP/MOD
4. [ ] **Configタブ**: カラーテーマ変更、同時発音数、Limiter Ceiling、解析ロジック設定
5. [ ] グレインループのSIMD化（グレイン主導ブロック処理への再設計、2〜3倍高速化見込み）
6. [ ] プリセットブラウザ、リサイズGUI、MPE/ピッチベンド
