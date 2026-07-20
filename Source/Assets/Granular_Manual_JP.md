# GRANULAR 簡易マニュアル（日本語）

[English](Granular_Manual_EN.md) | **日本語**

---

## 目次

1. [まずは音を出す](#1-まずは音を出す)
2. [画面の構成](#2-画面の構成)
3. [MAIN タブ](#3-main-タブ)
4. [ARP タブ](#4-arp-タブ)
5. [MOD タブ](#5-mod-タブモジュレーション)
6. [FX タブ](#6-fx-タブ)
7. [CONFIG タブ](#7-config-タブ)
8. [プリセット](#8-プリセット)
9. [使いこなしのヒント](#9-使いこなしのヒント)

---

## 1. まずは音を出す

**OTODESK / 24ボイス スケール量子化グラニュラーシンセ（VST3 / Standalone）**

読み込んだ音を「粒（グレイン）」に分解して再構築するグラニュラーシンセです。ノイズや環境音でも音程を持った楽器として演奏でき、変調・アルペジオ・FX・80種のプリセットを備えています。

### インストール

1. `Granular.vst3` を以下にコピーします。
   ```
   C:\Program Files\Common Files\VST3\
   ```
2. DAW でプラグインを再スキャンします。

### 最初の一歩

1. 音源（WAV / AIFF / MP3 / FLAC / OGG）を**波形エリアにドラッグ＆ドロップ**、またはヘッダーの **LOAD** ボタンから読み込みます。
2. 読み込むと自動でピッチ解析・正規化が行われ、**元の音程を保ったまま鍵盤で演奏**できます（C3＝原音の高さ）。
3. MIDIキーボードを弾くと発音します。

> **PLAY DRY**：取り込んだ原音をそのまま試聴します（もう一度押すと停止）。

---

## 2. 画面の構成

上部に常時**ドット波形**（読み込んだ音の周波数を解析し、低域＝暖色／高域＝寒色で表示。発音中は粒がリアルタイムに舞います）。
その下に **5つのタブ**があり、下部の表示が切り替わります。

| タブ | 内容 |
|------|------|
| **MAIN** | グレイン・ピッチ・エンベロープ（ADSR）・POSITION/START/END・MOTION・マスター |
| **ARP** | アルペジエイター＋量子化（Key/Scale）＋テクスチャ |
| **MOD** | モジュレーション（LFO / Env → 16スロットマトリクス） |
| **FX** | 5スロットのエフェクトチェーン |
| **CONFIG** | 解析モード・リミッター・同時発音数・フィルタ・カラーテーマ |

> ノブは**右クリックで数値を直接入力**できます。破壊的な操作（DELETE / RESET など）は確認が出ます。

---

## 3. MAIN タブ

- **GRAIN**：SIZE（粒の長さ）/ DENSITY（密度）/ SPRAY（位置のばらつき）/ JITTER（ピッチの揺れ）/ PAN / SPEED
- **PITCH**：OCTAVE / SEMI / FINE
- **ENVELOPE**：A / D / S / R（音量の時間変化）
- **SAMPLE**：POSITION（再生位置）/ START / END（使用範囲）。波形上のマーカーをドラッグしても調整できます
- **MOTION**：再生位置の動き方（Forward / Backward / Ping-Pong / Random）。**S-SYNC** で領域をテンポ同期スキャン

> **SPEED / SCAN / S-SYNC の関係**
> Forward / Backward / Ping-Pong では、マーカーが Start から End へ移動する**速さ**を SPEED が決めます。S-SYNC をオンにすると SPEED は無視され、代わりに「Start から End まで渡りきるのに SCAN の音価ぶんかける」動きになります（例: 120BPM で 8 Bars なら16秒かけて1往き）。素材の長さに関係なく所要時間が決まるので、テンポに貼り付いたゆっくりした変化が作れます。
> Random だけはマーカーが移動せず、**一定間隔でランダムな位置へ飛びます**。この飛ぶ間隔を SPEED が決めます（SPEED=1 で毎秒約30回、下げるほどゆっくり、**0 でフリーズ**して1点に貼り付き）。S-SYNC をオンにすれば SCAN の音価ごとにジャンプするので、拍にピタッと合ったスタッターになります。
> 次のジャンプまでに発火した粒はすべて同じ場所から出るため、DENSITY を上げたまま SPEED を下げると「密な粒の塊がガコッと場所を変える」グリッチ感が出ます。
- **MASTER**：HPF / LPF / GAIN / DRY-WET（FX全体の混ぜ具合）
- **LIMITER**：CEILING（出力の天井）

---

## 4. ARP タブ

鍵盤で押さえた和音を自動でばらして演奏します。パターン13種、テンポ同期（付点・三連）／フリー（Hz）、Gate（音の長さ）、Octaveレンジ、Latch（押さえ続けなくても保持）。出力ノートは **Key / Scale** にスナップします。

---

## 5. MOD タブ（モジュレーション）

左が**ソース**（サブタブ切替）:

- **LFO ×4**：波形6種（Sine / Tri / Saw / Sqr / S&H / Chaos）。SYNC でテンポ同期
- **Env ×3**：ADSR。LOOP をオンにすると A-D を繰り返す複雑なLFOにもなります
- **MIDIソース**：Velocity / Note / Mod Wheel / Random（ノート毎のS&H）

右が **16スロットのマトリクス**（Page 1 / Page 2）:

- 各行で **Source（源）× Amount（量）→ Destination（行き先）** を設定
- **Amount** はダブルクリックで0。**UNI/BI** トグルで片方向／両方向を選択
- 変調される宛先ノブには**白いアーク**で変調範囲が表示され、波形上の Position / Spray / Start / End にも反映されます

**1ソース→複数行き先**：例えば「Source=LFO 1, Dest=Position, Amt=+0.5」と「Source=LFO 1, Dest=Spray, Amt=-0.5」の2行を作ると、1つのLFOで Position を右・Spray を左に同時に動かせます。同じソースは最大16行まで割り当て可能です。

---

## 6. FX タブ

5スロットに以下を自由に配置（ヘッダーのドラッグで並べ替え可）:

Saturation / Chorus / Tape Delay / Freeze / Shimmer Reverb。スロットを選ぶと下部に詳細パラメータが出ます。FX全体の混ぜ具合は MAIN の **DRY-WET** で調整します。

---

## 7. CONFIG タブ

- **MATERIAL**：解析モード（Auto / Percussive / Melodic / Vocal）。変更後、ヘッダーの **ANALYZE** で再解析
- **LIMITER RELEASE**：リミッターのリリース
- **POLY**：同時発音数（1〜24）
- **FILTER SLOPE**：マスターフィルタの傾き（12 / 24 dB/oct）
- **COLOR THEME**：10種のカラーテーマ（完全に反映するにはプラグイン画面を開き直してください）

---

## 8. プリセット

ヘッダーの **BROWSE** で下部にブラウザが開きます。

- **ファクトリー80種**：8カテゴリ（Pads & Ambient / Textures / Rhythmic / Bass / Leads & Keys / Cinematic / Glitch & IDM / Experimental）、オーソドックス〜奇抜まで
- 左でカテゴリ選択、上の検索で絞り込み
- 行を**クリックで即ロード**、**▲▼ボタン**や**キーボード↑↓**で順送り
- **右クリック**：お気に入り追加/解除、ユーザープリセットの削除（確認あり）
- ★ が付いたものは Favorites カテゴリにまとまります

**RANDOM**（ヘッダー・BROWSEの左）：ノブ・MOD（3〜10本）・FXをランダマイズ。思わぬ音の発見に。

**保存**：ブラウザ上部に名前を入れて **SAVE**。ユーザープリセットは
`ドキュメント / OTODESK / Granular / Presets /` に `.xml` で保存されます。

> **サンプルについて**：プリセットには音声そのものは含まれず、**サンプルのファイルパスを記録**します。元のファイルが残っていれば、読み込み時に自動で再ロードされます。

---

## 9. 使いこなしのヒント

- **音程感を出したい**：TONE を上げる、または P-SYNC（Pitch Sync）をオン。ノイズ系素材でも音階になります
- **動く音にしたい**：MOD で LFO → Position（波形上を粒が舞います）、または Env(Loop) → Size/Density
- **リズムを作る**：ARP か、LFO(SYNC) → Density/Position
- **広がりを出す**：PAN を上げ、Shimmer Reverb を薄く
- 大きな音量になりがちな設定でも、内部の多層セーフティ（ソフトクリップ／リミッター）で保護されます

### 音が途切れる・重い

- CONFIG → **POLY** を下げる（24 → 12 など）
- **DENSITY** を下げる（グレイン数が減ります）
- 使っていない FX スロットを `None` にする

### カラーテーマを変えても色が変わりきらない

- ノブのアクセント色は起動時にキャッシュされるためです。プラグイン画面を閉じて開き直すと完全に反映されます

---

## ライセンス

本ソフトウェアは **AGPLv3** で配布されています。

タイムストレッチには **Signalsmith Stretch**（MIT ライセンス）を使用しています。

- Signalsmith Stretch — Copyright © 2022 Geraint Luff / Signalsmith Audio Ltd.
- Signalsmith Linear — Copyright © 2025 Signalsmith Audio

同じ表記はプラグイン内の **CONFIG** タブ（ANALYSIS セクションの下）にも表示されます。

---

**開発:** @kijyoumusic (OTODESK)
[X (Twitter)](https://x.com/kijyoumusic) | [公式サイト](https://otodesk4193.github.io/OTODESK_SITE/)
