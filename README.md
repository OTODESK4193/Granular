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
- **タブUI**: MAIN / ARP / MOD / FX / CONFIG（ドット波形は常時表示、下部のみ切り替え）
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
- **Start/End**: ノブ＋波形上のマーカードラッグでサンプル範囲を調整。
  グレインは領域端で約3msのフェード処理を行い、End→Startの折り返しで生じる
  波形の不連続（クリック・突発ノイズ）を防止
- **プレイヘッド表示**: 発音中の実再生位置をミント色ラインで波形上に表示
  （ピンク線=Position起点、ミント線=動くプレイヘッド、白線=Position/Start/Endの変調後位置）
- **MOTION**: プレイヘッドの動きを Forward / Backward / Ping-Pong / Random から選択。
  S-SYNCオンで「Start-End領域を指定音価（1拍〜8小節）でスキャン」するDAWテンポ追随。
  グレインは領域内をラップ再生（Positionが常に発火起点として尊重される）
- **マスターフィルター（TPT SVF）**: HPF/LPFはTPT（トポロジー保存）ステートバリアブル
  フィルタで、カットオフをサンプル単位でスムージング（変調してもクリック/ザラつき無し）。
  12dB/oct（1段）/ 24dB/oct（2段）を Filter Slope で切替
- **マスターリミッター**: ブリックウォール、Ceiling（-6〜0dB）＋ Release可変。
  DC/サブソニックはFXフィードバックのDCブロッカーで除去、レイテンシは0報告
- **Dry/Wet**: FX全体のドライ/ウェットをMasterノブで調整（MOD対象）
- **Dryプレビュー**: ヘッダーの PLAY DRY で取り込んだ原音を試聴（再押しで停止）
- **UI**: 5タブ構成 MAIN / ARP / MOD / FX / CONFIG。
  MAIN=グレイン・ピッチ・ADSRエンベロープ・POSITION/START/END・MOTION・マスター
  （HPF/LPF/GAIN/DRY-WET）・LIMITER(CEILING)、
  ARP=アルペジエイター＋QUANTIZE(Key/Scale)＋TEXTURE(WINDOW/TONE/P-SYNC/REV)、
  MOD=下記モジュレーションマトリクス、
  FX=5スロットFXチェーン、
  CONFIG=MATERIAL解析モード・Limiter RELEASE・POLY（同時発音数1-24）・
  Filter Slope・Color Theme（10種）。
  全ノブ右クリックで数値入力、トグルはLED点灯式、
  ヘッダーにロゴ隣の解析HUD（4行）＋ PLAY DRY / LOAD / ANALYZE / DELETE / RESET
  （破壊的操作はY/N確認付き）
- **カラーテーマ（10種）**: Midnight / Sakura / Ocean / Forest / Sunset / Mono /
  Neon / Vaporwave / Amber / Arctic。CONFIGで切替（完全反映はUI再オープン時）
- **プリセットブラウザ**: ヘッダーの BROWSE で下部に展開（BassSynth風）。
  ファクトリー80種（8カテゴリ×サブカテゴリ、オーソドックス〜奇抜まで）＋ユーザー保存。
  カテゴリ/検索/★お気に入り/右クリック削除（確認付き）/▲▼・キーボード↑↓で切替。
  ヘッダーの **RANDOM** でノブ・MOD（3〜10本）・FXをランダマイズ。
  ユーザープリセットは **ドキュメント/OTODESK/Granular/Presets/** に保存（Sampleはパス参照）
- **安全設計**: FX前の安全ソフトクリップ・TONEコム/リバーブFDN/ディレイの飽和＆DCブロッカー・
  グレイン境界フェード・Positionのサンプル単位グライドで、突発的な大音量やクリックを防止
- **パステル・ドット波形GUI**:
  波形を小さなドットの縦積みで表現。**読み込んだ音の周波数を解析して着色**
  （低域=暖色 / 高域=寒色、明暗コントラスト付き。色はカラーテーマに連動）。
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
   ├─ PluginProcessor.h/.cpp     … APVTS / サンプル下処理スレッド / FX Dry-Wet / Dryプレビュー
   ├─ PluginEditor.h/.cpp        … レイアウト / HUD / タブ / テーマ適用
   ├─ DSP/
   │  ├─ GranularEngine.h/.cpp   … 24ボイス・グレインエンジン / TPT SVF / BrickLimiter
   │  ├─ ModMatrix.h             … LFO/ADSR/Macro → 16スロットマトリクス
   │  ├─ Arpeggiator.h/.cpp      … アルペジエイター（13パターン）
   │  ├─ FxChain.h               … 5スロットFX（Sat/Chorus/Delay/Freeze/Reverb）
   │  ├─ ScaleQuantizer.h        … Key/Scale量子化（変調後適用）
   │  ├─ PitchAnalyzer.h/.cpp    … Omni-Analyzer（Tessellator移植）
   │  └─ GrainVisualizerData.h   … 音声→GUI ロックフリーイベント
   ├─ GUI/
   │  ├─ ColorPalette.h          … 10カラーテーマ / スペクトルカラー
   │  ├─ ArcDial.h/.cpp          … アークダイアル LookAndFeel（変調範囲アーク）
   │  ├─ MainPanel / ModPanel / FxPanel / ArpPanel / ConfigPanel … 各タブUI
   │  ├─ PresetBrowser.h        … プリセットブラウザ（Factory/User/Favorites）
   │  ├─ ValueKnob.h / GlowToggle.h … 数値入力ノブ / LEDトグル
   │  └─ DotWaveformDisplay.h/.cpp … ドット波形＋グレイン可視化＋周波数カラー
   ├─ Presets/
   │  └─ PresetData.h           … ファクトリープリセット80種（パラメータ上書き方式）
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
2. [x] **MODタブ**: ソース段は LFO / Env / MACRO のサブタブ構成。
       LFO×4（テンポ同期1/1〜1/32、Sine/Tri/Saw/Sqr/S&H/Chaos）、
       **ADSRエンベロープ×3**（Loopトグルで A-D 循環の複雑LFOにもなる）、Macro×4、
       MIDIソース（Velocity/Note/ModWheel/ノート毎Random）。
       → **16スロットマトリクス（Page1/Page2）** → 行き先は主要パラメータ＋
       Sample Start/End＋Arp Octaves/Rate/Gate＋Tone＋FX Dry/Wet＋FX Amount×5。
       各行 Amountは±バイポーラ（ダブルクリックで0）、**Uni/Bipolarトグル**で極性選択。
       宛先ノブには**変調範囲を白アークで表示**、波形にも Position/Spray/Start/End の
       変調をリアルタイム反映。
       **Pitchへの変調は量子化前に加算されるため、暴れてもKey/Scaleに乗る**。
       Macroは「1ノブ→複数行き先」のハブ（DAWオートメーション/MIDIマッピング可）
3. [x] **FXタブ**: **5スロット式**（FX数と一致）— **ADAA Saturation（アルゴリズム10種・NextGenKick2移植）**/
       Ensemble Chorus / Tape Delay（テンポ同期＋ダッキング）/ Freeze /
       Shimmer Reverb（16ch FDN＋オクターブシフター）。
       **適用順はスロットヘッダーのD&Dで並べ替え可能**。チェーンはLimiter前段に挿入。
       **スロット選択で下部に詳細パラメータが出現**:
       Sat=ALGO/DRIVE/PRE-HPF/TRIM、Chorus=RATE/DEPTH/WIDTH、
       Delay=TIME(1/2〜1/16T)/FEEDBACK/DUCK/DAMP、Freeze=SIZE/FEEDBACK/DAMP、
       Reverb=DECAY/SHIMMER/DAMP/MOD
4. [x] **Configタブ**: カラーテーマ（10種）、同時発音数（POLY 1-24）、Filter Slope、
       Limiter Release、MATERIAL解析モード（Ceilingは Main へ移設）
5. [x] **プリセットブラウザ**: ファクトリー80（8カテゴリ×サブ）＋ユーザー保存（ドキュメント）、
       お気に入り・検索・削除・▲▼/キーボード切替、RANDOMマクロ
6. [ ] グレインループのSIMD化（グレイン主導ブロック処理への再設計、2〜3倍高速化見込み）
7. [ ] リサイズGUI、MPE/ピッチベンド
8. [ ] スペクトルカラーのFFTベース高精度化、Limiterルックアヘッド
