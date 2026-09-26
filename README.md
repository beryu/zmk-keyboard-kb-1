# kb-1 ZMK firmware

[kb-1](https://github.com/beryu/kb-1)用のZMKファームウェアです。現行の基板は左右を12極ケーブルで接続し、右側のSeeed Studio XIAO nRF52840 Plus 1個で全48キー（左26キー、右22キー）とPAW3222トラックボールを制御します。右側のUSB-Cから給電し、PCへのキー・ポインター入力はBluetoothで送信します。左側はコントローラーを持たない受動基板です。

XIAO nRF52840 Sense Plusではなく、XIAO nRF52840 Plusを使用してください。

## ファームウェア

GitHub Actionsの成果物には次のUF2が含まれます。

- `kb_1.uf2`: 右側のXIAO用。左右のキーとトラックボールを制御
- `settings_reset.uf2`: 右側XIAOのBluetooth設定をリセット

左側へ書き込むファームウェアはありません。

## 接続と書き込み

1. 電源を外した状態で、左右の基板を対応する12極ケーブルで接続します。現行基板のコネクターとケーブル仕様は[基板設計リポジトリ](https://github.com/beryu/torabo-tsuki-om/blob/master/pcb/ffsd-migration.md)を参照してください。
2. 右側XIAOのUSB-Cを電源に接続します。USBは給電とUF2書き込みに使用し、ZMKのUSBキーボード出力は無効です。
3. 書き込む場合は右側XIAOのリセットを素早く2回押してUF2ブートローダーに入り、マウントされたドライブへ`kb_1.uf2`をコピーします。
4. PCから`kb-1`にBluetooth接続します。ZMK StudioもBluetooth経由で使用できます。

キーマップは[keymap-editor](https://nickcoutsos.github.io/keymap-editor/)または[ZMK Studio](https://zmk.studio/)で編集できます。基板の変更に合わせて、右側の内側3段目のキーは配列から削除されています。Bluetoothペアリング解除の`BT_CLR`はレイヤー2の右上キーに配置しています。

## ビルド

`build.yaml`を使用してGitHub ActionsまたはZMK CLIでビルドします。ZMK v0.3でのボードターゲットは`seeeduino_xiao_ble`、シールドは`kb_1`です。ZMKとPAW3222ドライバは、再現可能なビルドのため`config/west.yml`でタグまたはコミットSHAへ固定しています。

## PCBとの対応

GPIO割り当ては[現行のKiCad回路図](https://github.com/beryu/torabo-tsuki-om)の右側XIAOから採っています。左右間は行・列・GNDを直接配線し、ZMKの無線分割機能は使用しません。D14/P0.09とD15/P0.10をNFCからGPIOへ切り替えています。

| 信号 | XIAO | nRF52840 GPIO |
| --- | --- | --- |
| LROW0 / LROW1 / LROW2 / LROW3 | D0 / D1 / D6 / D15 | P0.02 / P0.03 / P1.11 / P0.10 |
| RROW0 / RROW1 / RROW2 / RROW3 | D11 / D18 / D4 / D17 | P0.15 / P1.05 / P0.04 / P1.03 |
| COL0 / COL1 / COL2 / COL3 | D14 / D12 / D2 / D13 | P0.09 / P0.19 / P0.28 / P1.01 |
| COL4 / COL5 / COL6 | D3 / D19 / D5 | P0.29 / P1.07 / P0.05 |
| CS / SCLK / MOTION / SDIO | D7 / D8 / D9 / D10 | P1.12 / P1.13 / P1.14 / P1.15 |

現時点ではビルド検証までを対象としています。キー位置、左右ケーブルの導通、トラックボール、Bluetooth接続は実機で確認してください。
