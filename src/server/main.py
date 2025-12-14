"""Raspberry Pi Pico + Grove Shield 向けの I/O ファームウェア。

USB シリアル(CDC-ACM)経由でテキストベースのプロトコルを受け取り、
**GrovePi C++ API と同名のコマンド** (pinMode/digitalRead/digitalWrite/analogRead など)
を解釈して、Pico 上の GPIO / ADC / I2C デバイスを制御する。

プロトコルの詳細は docs/protocol.md を参照。
"""

import sys
import time
from machine import ADC, Pin, I2C, PWM, time_pulse_us
import dht

# GrovePi アナログピン番号(0/1/2) → Pico ADC へのマッピング
ANALOG_PINS = {
    0: ADC(0),  # A0 -> GP26
    1: ADC(1),  # A1 -> GP27
    2: ADC(2),  # A2 -> GP28
}

# GrovePi デジタルピン番号(16/18/20) → Pico GPIO へのマッピング
# C++ 側からは Grove shield のシルク表記 (D16/D18/D20) をそのまま指定する想定。
DIGITAL_PINS = {
    16: Pin(16, Pin.OUT, value=0),  # D16 -> GP16
    18: Pin(18, Pin.OUT, value=0),  # D18 -> GP18
    20: Pin(20, Pin.OUT, value=0),  # D20 -> GP20
}

# I2C Grove ポート: LCD(JHD1313M3) 用のテキスト表示チャネル
I2C_BUSES = {
    # Grove Shield for Pi Pico v1.0 の I2C ポート配線
    # I2C0: SCL -> GP9, SDA -> GP8
    "i2c0": I2C(0, scl=Pin(9), sda=Pin(8), freq=400_000),
    # I2C1: SCL -> GP7, SDA -> GP6
    "i2c1": I2C(1, scl=Pin(7), sda=Pin(6), freq=400_000),
}

try:
    # Grove 16x2 LCD(JHD1313M3, RGB バックライト) 用 MicroPython ドライバ
    # 別途 lcd1602.py を Pico 本体に配置しておくこと。
    # https://wiki.seeedstudio.com/Grove-Starter-Kit-for-Raspberry-Pi-Pico/#docusaurus_skipToContent_fallback
    # https://files.seeedstudio.com/wiki/Grove-16x2_LCD--White_on_Blue/lcd1602.py
    #
    # RGB バックライト付きモデルなので、LCD1602_RGB を利用する。
    from lcd1602 import LCD1602_RGB as LCD1602
except ImportError:
    LCD1602 = None  # ドライバ未配置の場合は LCD 機能を無効化

_LCD_CACHE = {}


def _get_lcd(key):
    """指定された I2C チャネルに対応する LCD インスタンスを取得する。

    必要に応じて遅延初期化を行う。
    """
    k = key.lower()

    if k in _LCD_CACHE:
        return _LCD_CACHE[k]

    if LCD1602 is None:
        raise RuntimeError("LCD_DRIVER_NOT_AVAILABLE")

    if k not in I2C_BUSES:
        raise KeyError("UNKNOWN_LCD_CHANNEL")

    i2c = I2C_BUSES[k]
    lcd = LCD1602(i2c, 2, 16)
    _LCD_CACHE[k] = lcd
    return lcd


def write_lcd(name, raw_text):
    """JHD1313M3 LCD に文字列を表示する。

    16x2 の LCD を想定し、先頭 16 文字を 1 行目、
    17〜32 文字目を 2 行目に表示する。33 文字目以降は切り捨て。

    """
    lcd = _get_lcd(name)

    if not isinstance(raw_text, str):
        raw_text = _to_str(raw_text)

    text = raw_text.strip()

    # 改行はスペースに置き換え、全体で 32 文字に制限
    text = text.replace("\r", " ").replace("\n", " ")
    text = text[:32]

    line1 = text[:16]
    line2 = text[16:32]

    # Seeed のサンプルと同じイメージで、clear() + home() のみを使って
    # 行頭から書き込む（スペース塗りつぶしは行わない）
    try:
        lcd.clear()
    except Exception:
        # clear が失敗しても最低限 home から書き込む
        pass

    lcd.home()
    lcd.print(line1)
    if line2:
        lcd.setCursor(0, 1)
        lcd.print(line2)

# 本体オンボード LED
LED = Pin("LED", Pin.OUT, value=0)

def _to_str(s):
    """bytes/str を str に揃えるヘルパー。

    Args:
        s: 変換対象の値。bytes または str を想定。

    Returns:
        str 型に変換された文字列。
    """
    if isinstance(s, str):
        return s
    try:
        return s.decode("utf-8")
    except Exception:
        return str(s)

def read_line():
    """stdin から 1 行読み取り、末尾の改行を除いて返す関数。

    機械対機械(C/C++ クライアント)通信を前提とし、
    **受信した文字のエコーバックは一切行わない**。

    Returns:
        読み取った 1 行（末尾の改行を除去した文字列）。
        EOF かつバッファが空の場合は None。
    """
    line = sys.stdin.readline()
    if not line:
        return None
    # CRLF / LF をまとめて扱い、前後の空白も削除
    return _to_str(line).strip()

def send_number(n):
    """数値を 10 進数文字列 + 改行で送信する。

    Args:
        n: 送信する数値。int 変換可能な値を想定。
    """
    try:
        s = str(int(n))
    except Exception:
        s = "0"
    sys.stdout.write(s + "\n")

def send_error():
    """エラー時の共通レスポンスを送信する。

    Returns:
        なし。
    """
    sys.stdout.write("error\n")


def send_two_floats(a, b):
    """浮動小数点 2 つを \"a b\\n\" 形式で送信する。"""
    try:
        s = "{} {}".format(float(a), float(b))
    except Exception:
        s = "0 0"
    sys.stdout.write(s + "\n")

def _parse_int(token):
    """int 変換のヘルパー。失敗したら None を返す。"""
    try:
        return int(token)
    except Exception:
        return None


_PWM_CACHE = {}
_DHT_CACHE = {}
_DHT_LAST = {}


def pinMode(pin_no, mode):
    """pinMode(pin, mode)

    Args:
        pin_no: 設定対象のピン番号。デジタルピンは 16/18/20、アナログピンは 0/1/2。
        mode: 文字列 \"INPUT\"/\"OUTPUT\"（大文字小文字は問わない）、またはそれに相当する値。
    """
    m = str(mode).lower()
    # アナログピンは何もしない (成功扱い)
    if pin_no in ANALOG_PINS:
        return

    pin = DIGITAL_PINS.get(pin_no)
    if pin is None:
        raise KeyError("UNKNOWN_DIGITAL_PIN")

    if m in ("in", "input"):
        pin.init(mode=Pin.IN)
    elif m in ("out", "output"):
        pin.init(mode=Pin.OUT)
    else:
        raise ValueError("UNKNOWN_MODE")


def digitalWrite(pin_no, value):
    """digitalWrite(pin, value)

    Args:
        pin_no: 出力対象のデジタルピン番号 (16/18/20)。
        value: 文字列 \"HIGH\"/\"LOW\"（大文字小文字は問わない）、または真偽値に変換可能な値。
    """
    pin = DIGITAL_PINS.get(pin_no)
    if pin is None:
        raise KeyError("UNKNOWN_DIGITAL_PIN")

    token = str(value).lower()
    if token == "high":
        v = 1
    elif token == "low":
        v = 0
    else:
        raise ValueError("UNKNOWN_LEVEL")

    try:
        pin.init(mode=Pin.OUT, value=v)
    except Exception:
        pin.value(v)


def digitalRead(pin_no):
    """digitalRead(pin) -> 0/1

    Args:
        pin_no: 入力対象のデジタルピン番号 (16/18/20)。

    Returns:
        0 または 1 の整数。
    """
    pin = DIGITAL_PINS.get(pin_no)
    if pin is None:
        raise KeyError("UNKNOWN_DIGITAL_PIN")

    try:
        pin.init(mode=Pin.IN)
    except Exception:
        pass
    v = pin.value()
    return 1 if v else 0


def analogRead(pin_no):
    """analogRead(pin) -> 0〜65535

    Args:
        pin_no: アナログピン番号 (0/1/2)。

    Returns:
        0〜65535 の整数値 (`ADC.read_u16()` の生値)。
    """
    adc = ANALOG_PINS.get(pin_no)
    if adc is None:
        raise KeyError("UNKNOWN_ANALOG_PIN")
    return adc.read_u16()


def analogWrite(pin_no, value):
    """analogWrite(pin, value[0-255])

    Args:
        pin_no: PWM 出力対象のデジタルピン番号 (16/18/20)。
        value: 0〜255 の整数。0 で OFF, 255 で最大デューティ。
    """
    pin = DIGITAL_PINS.get(pin_no)
    if pin is None:
        raise KeyError("UNKNOWN_DIGITAL_PIN")

    pwm = _PWM_CACHE.get(pin_no)
    if pwm is None:
        pwm = PWM(pin)
        pwm.freq(1000)  # 1kHz 程度の PWM
        _PWM_CACHE[pin_no] = pwm

    v = int(value)
    if v < 0:
        v = 0
    if v > 255:
        v = 255

    duty = int(v * 257)  # 0–255 -> 0–65535 へ線形変換
    pwm.duty_u16(duty)


def ultrasonicRead(pin_no):
    """ultrasonicRead(pin) -> distance[cm]

    Args:
        pin_no: 超音波距離センサーを接続したデジタルピン番号 (16/18/20)。

    Returns:
        距離[cm] を表す整数。
    """
    pin = DIGITAL_PINS.get(pin_no)
    if pin is None:
        raise KeyError("UNKNOWN_DIGITAL_PIN")

    # トリガーパルス送信
    pin.init(mode=Pin.OUT)
    pin.value(0)
    time.sleep_us(2)
    pin.value(1)
    time.sleep_us(10)
    pin.value(0)

    # エコー計測
    pin.init(mode=Pin.IN)
    try:
        duration = time_pulse_us(pin, 1, 30000)  # 30ms タイムアウト
    except Exception:
        raise RuntimeError("PULSE_TIMEOUT")

    if duration <= 0:
        raise RuntimeError("PULSE_ERROR")

    distance_cm = duration / 58.0
    return int(distance_cm + 0.5)


def setText(bus, text):
    """setText(bus, text)

    Args:
        bus: I2C バス番号。0/1, \"i2c0\"/\"i2c1\" のいずれか。
        text: LCD に表示する文字列。最大 32 文字（16x2）。
    """
    bus_token = str(bus).lower()
    if bus_token in ("0", "i2c0"):
        key = "i2c0"
    elif bus_token in ("1", "i2c1"):
        key = "i2c1"
    else:
        raise ValueError("UNKNOWN_I2C_BUS")
    write_lcd(key, text)


def setRGB(bus, r, g, b):
    """setRGB(bus, r, g, b)

    Args:
        bus: I2C バス番号。0/1, \"i2c0\"/\"i2c1\" のいずれか。
        r: 赤成分 (0〜255)。
        g: 緑成分 (0〜255)。
        b: 青成分 (0〜255)。
    """
    bus_token = str(bus).lower()
    if bus_token in ("0", "i2c0"):
        key = "i2c0"
    elif bus_token in ("1", "i2c1"):
        key = "i2c1"
    else:
        raise ValueError("UNKNOWN_I2C_BUS")

    lcd = _get_lcd(key)
    if hasattr(lcd, "set_rgb"):
        lcd.set_rgb(int(r), int(g), int(b))


def dhtRead(pin_no, module_type):
    """dhtRead(pin, module_type) -> (temp, hum)

    Args:
        pin_no: DHT センサーを接続したピン番号 (デジタルピン番号)。
        module_type: 0 = BLUE モジュール(DHT11), 1 = WHITE モジュール(DHT22)。

    Returns:
        (temp, hum): 温度[℃], 湿度[%] のタプル。
    """
    # DHT ライブラリは Pin 番号から新しい Pin インスタンスを受け取る想定なので、
    # DIGITAL_PINS ではなくピン番号から直接生成する。
    pin = Pin(pin_no, Pin.IN)

    if module_type == 0:
        sensor = dht.DHT11(pin)
    elif module_type == 1:
        sensor = dht.DHT22(pin)
    else:
        raise ValueError("UNKNOWN_DHT_MODULE_TYPE")

    key = (pin_no, module_type)

    try:
        # DHT11 / DHT22 系は、データシート上も「連続して高速に測定するとエラーになりうる」
        # という性質を持つため、ここで measure() が例外を投げることがある。
        # センサー仕様起因の一時的なエラーでホスト側アプリが落ちないよう、
        # 正常に取得できた値をキャッシュしておき、失敗時はその値を返す。
        sensor.measure()
        temp = sensor.temperature()
        hum = sensor.humidity()
        _DHT_CACHE[key] = (float(temp), float(hum))
    except Exception:
        # 計測エラー時は新規計測は諦め、前回値があればそれを返す。
        if key in _DHT_CACHE:
            return _DHT_CACHE[key]
        # 一度も成功していない場合だけ、上位にエラーとして伝える。
        raise

    return _DHT_CACHE[key]

def _parse_call(line):
    """\"func(arg1, arg2, ...)\" 形式の 1 行をパースする。

    Returns:
        (name, args_str) または (None, None)。
    """
    if not line:
        return None, None

    s = line.strip()
    if not s:
        return None, None

    l = s.find("(")
    r = s.rfind(")")
    if l <= 0 or r <= l:
        return None, None

    name = s[:l].strip()
    args_str = s[l + 1 : r].strip()
    return name, args_str


def _split_args(args_str, maxsplit=-1):
    """カンマ区切りの引数文字列を分割してトリムする."""
    if not args_str:
        return []
    if maxsplit == 1:
        parts = args_str.split(",", 1)
    else:
        parts = args_str.split(",")
    return [p.strip() for p in parts if p.strip() != ""]


def handle_command(line):
    """1 行のテキストコマンドを解釈して実行する。"""
    name, args_str = _parse_call(line)
    if not name:
        send_error()
        return

    cmd = name.lower()

    # --- GrovePi C++ API 対応コマンド ---

    # pinMode(pin, mode)
    if cmd == "pinmode":
        parts = _split_args(args_str)
        if len(parts) != 2:
            send_error()
            return

        pin_no = _parse_int(parts[0])
        if pin_no is None:
            send_error()
            return
        mode = parts[1]

        try:
            pinMode(pin_no, mode)
            sys.stdout.write("\n")
        except Exception:
            send_error()
        return

    # digitalWrite(pin, value)
    if cmd == "digitalwrite":
        parts = _split_args(args_str)
        if len(parts) != 2:
            send_error()
            return

        pin_no = _parse_int(parts[0])
        if pin_no is None:
            send_error()
            return

        try:
            digitalWrite(pin_no, parts[1])
            sys.stdout.write("\n")
        except Exception:
            send_error()
        return

    # digitalRead(pin)
    if cmd == "digitalread":
        parts = _split_args(args_str)
        if len(parts) != 1:
            send_error()
            return

        pin_no = _parse_int(parts[0])
        if pin_no is None:
            send_error()
            return

        try:
            v = digitalRead(pin_no)
            send_number(v)
        except Exception:
            send_error()
        return

    # analogRead(pin)
    if cmd == "analogread":
        parts = _split_args(args_str)
        if len(parts) != 1:
            send_error()
            return

        pin_no = _parse_int(parts[0])
        if pin_no is None:
            send_error()
            return

        try:
            v = analogRead(pin_no)
            send_number(v)
        except Exception:
            send_error()
        return

    # analogWrite(pin, value)
    if cmd == "analogwrite":
        parts = _split_args(args_str)
        if len(parts) != 2:
            send_error()
            return

        pin_no = _parse_int(parts[0])
        value = _parse_int(parts[1])
        if pin_no is None or value is None:
            send_error()
            return

        try:
            analogWrite(pin_no, value)
            sys.stdout.write("\n")
        except Exception:
            send_error()
        return

    # ultrasonicRead(pin)
    if cmd == "ultrasonicread":
        parts = _split_args(args_str)
        if len(parts) != 1:
            send_error()
            return

        pin_no = _parse_int(parts[0])
        if pin_no is None:
            send_error()
            return

        try:
            distance = ultrasonicRead(pin_no)
            send_number(distance)
        except Exception:
            send_error()
        return

    # --- LCD 表示系の拡張コマンド ---

    # setText(bus, text...)
    if cmd == "settext":
        # 第1引数: bus, 第2引数: 以降すべてテキストとして扱う
        parts = _split_args(args_str, maxsplit=1)
        if len(parts) != 2:
            send_error()
            return

        bus_token = parts[0].lower()
        if bus_token in ("0", "i2c0"):
            key = "i2c0"
        elif bus_token in ("1", "i2c1"):
            key = "i2c1"
        else:
            send_error()
            return

        text = parts[1]
        try:
            setText(bus_token, text)
            sys.stdout.write("\n")
        except Exception:
            send_error()
        return

    # setRGB(bus, r, g, b)
    if cmd == "setrgb":
        parts = _split_args(args_str)
        if len(parts) != 4:
            send_error()
            return

        bus_token = parts[0].lower()
        if bus_token in ("0", "i2c0"):
            key = "i2c0"
        elif bus_token in ("1", "i2c1"):
            key = "i2c1"
        else:
            send_error()
            return

        r = _parse_int(parts[1])
        g = _parse_int(parts[2])
        b = _parse_int(parts[3])
        if r is None or g is None or b is None:
            send_error()
            return

        try:
            setRGB(bus_token, r, g, b)
            sys.stdout.write("\n")
        except Exception:
            send_error()
        return

    # --- DHT 温湿度センサー (Pico 専用拡張) ---

    # dhtRead(pin, module_type)
    if cmd == "dhtread":
        parts = _split_args(args_str)
        if len(parts) != 2:
            send_error()
            return

        pin_no = _parse_int(parts[0])
        module_type = _parse_int(parts[1])
        if pin_no is None or module_type is None:
            send_error()
            return

        try:
            temp, hum = dhtRead(pin_no, module_type)
            send_two_floats(temp, hum)
        except Exception:
            send_error()
        return

    send_error()

def main():
    """標準入力からのコマンドを無限ループで処理するエントリポイント。"""
    while True:
        line = read_line()
        if line is None:
            continue
        LED.value(1)
        try:
            handle_command(line)
        finally:
            LED.value(0)

if __name__ == "__main__":
    main()
