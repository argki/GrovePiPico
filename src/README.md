# src

## server

- Raspberry Pi Pico へ書き込むファームウェア。
- `main.py`以外は https://files.seeedstudio.com/wiki/Grove_Shield_for_Pi_Pico_V1.0/Libraries.rar より入手

## client

- C++で実装されたサンプル。
- https://github.com/DexterInd/GrovePi/tree/4a30c93a9900f88281680afd6bf1096e9bc5c359/Software/Cpp がベース。
- `make` でリポジトリ直下の `bin` ディレクトリにバイナリが出力される。
- 実行時は事前に環境変数 `GROVEPI_SERIAL` にパスを登録しておく必要あり。
  - 未指定の場合 `/dev/tty.usbmodem*` `/dev/ttyACM*` `/dev/ttyUSB*` を自動的に探索。
