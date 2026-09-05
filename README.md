# kb-1 ZMK firmware

[kb-1](https://github.com/beryu/kb-1)用のZMKファームウェアです。

## 対応ハードウェア

- Seeed Studio XIAO nRF52840 Plus
- 左右分割、49キー
- PAW3222トラックボール
- 単四電池と基板上の5V昇圧回路

XIAO nRF52840 Sense Plusではなく、XIAO nRF52840 Plusを使用してください。

## ファームウェア

GitHub Actionsの成果物には次のUF2が含まれます。

- `kb_1_left_central.uf2`: 左側にトラックボールを取り付ける場合
- `kb_1_right_peripheral.uf2`: 左側Centralと組み合わせる右側
- `kb_1_right_central.uf2`: 右側にトラックボールを取り付ける場合
- `kb_1_left_peripheral.uf2`: 右側Centralと組み合わせる左側
- `settings_reset.uf2`: Bluetooth設定のリセット用

トラックボールを取り付けた側に`_central`、反対側に対応する`_peripheral`を書き込んでください。

## 書き込み

1. キーボードから電池を外します。
2. 電源スイッチをONにしてXIAOをUSB接続します。
3. 電源スイッチを`ON → OFF → ON`と素早く2回操作し、UF2ブートローダーに入ります。
4. マウントされたドライブへ対応するUF2をコピーします。

キーマップは[keymap-editor](https://nickcoutsos.github.io/keymap-editor/)または[ZMK Studio](https://zmk.studio/)で編集できます。

## ビルド

`build.yaml`を使用してGitHub ActionsまたはZMK CLIでビルドします。ボードターゲットは`xiao_ble//zmk`です。

## PCBとの対応

GPIO割り当てはkb-1のKiCad基板データから生成された回路図を正としています。D14/P0.09は`COL0`として使用するため、devicetreeでNFCからGPIOへ切り替えています。

| 信号 | XIAO | nRF52840 GPIO |
| --- | --- | --- |
| VBAT_ADC | D0 | P0.02 / AIN0 |
| ROW0 | D1 | P0.03 |
| COL2 | D2 | P0.28 |
| COL4 | D3 | P0.29 |
| ROW3 | D4 | P0.04 |
| COL6 | D5 | P0.05 |
| センサー電源 | D6 | P1.11 |
| CS | D7 | P1.12 |
| SCLK | D8 | P1.13 |
| MOTION | D9 | P1.14 |
| SDIO | D10 | P1.15 |
| ROW1 | D11 | P0.15 |
| COL1 | D12 | P0.19 |
| COL3 | D13 | P1.01 |
| COL0 | D14 | P0.09 |
| ROW4 | D17 | P1.03 |
| ROW2 | D18 | P1.05 |
| COL5 | D19 | P1.07 |

現時点ではGitHub Actionsによるビルド検証までを対象としています。キー位置、分割接続、トラックボール、電池残量表示は実機での確認が必要です。
