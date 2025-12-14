# GrovePiPico

Raspberry Pi Pico と Grove シールドを用いて、USB シリアル経由で Grove モジュールとのデータの送受信を行う。  
`src/server`内の `*.py` を Raspberry Pi Pico に書き込み、ホスト側から関数呼び出し風のテキストプロトコルで制御することを想定。
