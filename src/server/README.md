# server

## 必要なもの

- Raspberry Pi Pico
- Pi Pico v1.0 用 Grove シールド
- USB Type-A - micro USB ケーブル
- VS Code
- VS Code の拡張機能「Raspberry Pi Pico」


## Raspberry Pi Pico への書き込み手順

- VS Codeで フォルダーを開く から本ディレクトリを開く。 
- 初回のみ
  - Pico 上の BOOTSEL ボタンを押しながら USB ケーブルで PC に接続。
  - 右下に、Found aconnected Pico in BOOTSEL mode.(省略)Do you want to flash it now?と表示されたら Yesをクリックし、指示に従ってflashする。
- 左下のステータスが「Pico Connected」になったら、エクスプローラーの`*.py`を右クリックし「Upload file to Pico」

## 動作確認

- VS Code左下のステータスが「Pico Connected」の状態では、`main.py`でのシリアル通信が正常に動作しないため、「Pico Disconnected」の状態になった後USBを抜き差ししてから動作確認する必要あり。
  - 接続方法は後述
- Grove - LCD RGB Backlight（V4.0）に文字を表示するには、5V/3V3 切替スイッチをVCC=5Vにする必要あり。


```bash
pinMode(16, OUTPUT)     # D16 を出力に設定
digitalWrite(16, HIGH)  # D16 を HIGH に
digitalWrite(16, LOW)   # D16 を LOW に

analogRead(0)           # A0 のアナログ値を読む
65535                   # A0 のアナログ値(レスポンスの例)

setText(1, Hello)       # I2C1 の LCD に \"Hello\" を表示
setRGB(1, 0, 255, 0)    # I2C1 の LCD バックライトを緑に
```

## 各OSでの接続方法

### macOS

```bash
# デバイス確認
ls /dev/tty.usbmodem*

# picocom をインストール(初回のみ)
brew install picocom

# 接続
picocom --echo /dev/tty.usbmodem21201
```

終了するには `Ctrl`+`A` → `Ctrl`+`X`。

### Ubuntu

```bash
# デバイス確認
ls /dev/ttyACM*

# picocom をインストール(初回のみ)
sudo apt update
sudo apt install -y picocom

# 接続
picocom --echo /dev/ttyACM0
```

### Windows

- Tera Term を起動し、「シリアル」を選んで `COM` ポートを選択し、接続。
- 設定 → 端末 を開き、「ローカルエコー」にチェックを入れる。
