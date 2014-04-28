IRKit Device
===

IRKit device and [IRKit SDK](https://github.com/irkit/ios-sdk) lets you control your home electronics from your iOS devices.
IRKit device has a Infrared LED and receiver, and a WiFi module inside.
Internet connected devices can make IRKit send IR signals for you.

## Get IRKit Device

Buy on [amazon.co.jp](http://www.amazon.co.jp/gp/product/B00H91KK26)

## Schematics

Basically, it's a [Arduino Leonardo](http://arduino.cc/en/Main/ArduinoBoardLeonardo) fork with following modifications:

* added a [WiFi module](http://www.gainspan.com/gs1011mips) ( [GainSpan](http://www.gainspan.com/home) 's WiFi module ).
* added Infrared Receiver, Infrared LED, some LEDs, and a microphone.

<img src="https://raw.github.com/irkit/device/master/hardware/schematic.png" />

## Firmware

It's basically a normal Arduino sketch.

See [IRKit.ino](https://github.com/irkit/device/blob/master/firmware/src/IRKit/IRKit.ino)

## Board

### Top

<img src="https://raw.github.com/irkit/device/master/hardware/top.png" />

### Bottom

<img src="https://raw.github.com/irkit/device/master/hardware/bottom.png" />

## Case

See [Case](https://github.com/irkit/device/tree/master/case)

## Extend it!

Following Arduino pins are pulled out for you.

* D2,D3,D4,D5,D12,TXLED
* A0,A1,A2,A3

See [IRKit.sch, eagle schematic file](https://github.com/irkit/device/blob/master/hardware/IRKit.sch) 's CN2

<a id="firmware-update-en"></a>
## How to update firmware

You can use [Arduino IDE](http://arduino.cc/en/Main/Software) to write IRKit's firmware.  
Follow this procedure (for MacOS).

1. Download, install and run [Arduino IDE](http://arduino.cc/en/Main/Software)
1. Connect IRKit using USB Micro-B cable to Mac.
1. Navigate to Arduino IDE Menu and select Tools -> Board -> "Arduino Leonardo".
1. Navigate to Arduino IDE Menu and select Tools -> Serial Port -> "/dev/tty.usbmodemXXXX".

   XXXX part should differ on your environment.

1. Download latest zip file from [IRKit device tags](https://github.com/irkit/device/releases).
1. Unzip it and open `firmware/src/IRKit/` directory.
1. Rename `version.template` to `version.c` .

   FYI. Product version is embedded here in official builds.

1. Double click `IRKit.ino` in the same directory.
1. Navigate to Arduino IDE Menu and click File -> Upload, to write into IRKit.
1. All Done!

<a id="logging-en"></a>
## How to get logs

IRKit logs what it's doing to USB serial, and you can use [Arduino IDE](http://arduino.cc/en/Main/Software) to see it.  
Follow this procedure (for MacOS).

1. Download, install and run [Arduino IDE](http://arduino.cc/en/Main/Software)
1. Connect IRKit using USB Micro-B cable to Mac.
1. Navigate to Arduino IDE Menu and select Tools -> Serial Port -> "/dev/tty.usbmodemXXXX".

   XXXX part should differ on your environment.

1. Navigate to Arduino IDE Menu and select Tools -> Serial Monitor.
1. Change new window's right-bottom settings to "No line ending" and "115200 baud", and close the window.
1. Reconnect IRKit and immediately start Serial Monitor again.

   This is to collect logs as soon as IRKit startups.

1. Copy and send logs to IRKit developer, thanks!

<a id="firmware-update"></a>
## ファームウェアアップデート方法

IRKitのファームウェアは、 [Arduino IDE](http://arduino.cc/en/Main/Software) を使って書き込むことができます。

出荷時のファームウェアバージョンによって、どのファームウェアバージョンまでアップデートできるかが変わります。  
以下の手順で現在のバージョンを確認し、対応するバージョンを使用してください。

| 現在のバージョン | 対応する最新のバージョン |
| --- | --- |
| 1.3.3以下 | [1.3.3](https://github.com/irkit/device/releases/tag/v1.3.3) |
| 1.3.4以上 | [最新](https://github.com/irkit/device/releases/latest) |

[IRKitシンプルリモコン](https://itunes.apple.com/us/app/irkit-simple-remote/id778790928?l=ja&ls=1&mt=8) を起動し、左上のボタンから設定画面を開くと、以下の画像の赤の四角のようにファームウェアバージョンを表示します。

<a href="https://www.flickr.com/photos/99687464@N00/14023119426" title="IRKit-firmware-version"><img src="https://farm6.staticflickr.com/5285/14023119426_aff8549745.jpg" width="282" height="500" alt="IRKit-firmware-version"></a>

以下がファームウェアのアップデート方法です(MacOS編)。

1. [Arduino IDE](http://arduino.cc/en/Main/Software) をダウンロードしてインストール後、起動します。
1. IRKitをUSB Micro-Bケーブルを使ってMacに接続します。
1. Arduino IDEのメニューから ツール -> マイコンボード -> "Arduino Leonardo" を選びます。
1. Arduino IDEのメニューから ツール -> シリアルポート -> "/dev/tty.usbmodemXXXX" を選びます。

   環境によってXXXXの部分やその前後が異なる場合があります。

1. ファームウェアをダウンロードし、zipファイルを解凍します。
1. 解凍したディレクトリ内にある `firmware/src/IRKit/` ディレクトリを開きます。
1. 同ディレクトリ内の `version.template` を `version.c` にリネームします。  

   参考) 製品版ではIRKitのバージョン情報をここに埋め込んでいます。

1. 同ディレクトリ内の `IRKit.ino` をダブルクリックして Arduino IDE で開きます。
1. Arduino IDEのメニューから ファイル -> マイコンボードに書き込む を押してIRKitに書き込みます。
1. 完了！

<a id="logging"></a>
## ログ取得方法

IRKitは、USB Micro-Bを使いパソコンまたはMacに接続すると、動作ログを取得することができます。  
以下の手順でログを取得します(MacOS編)。

1. [Arduino IDE](http://arduino.cc/en/Main/Software) をダウンロードしてインストール後、起動します。
1. IRKitをUSB Micro-Bケーブルを使ってMacに接続します。
1. Arduino IDEのメニューから ツール -> シリアルポート -> "/dev/tty.usbmodemXXXX" を選びます。

   環境によってXXXXの部分やその前後が異なる場合があります。

1. Arduino IDEのメニューから ツール -> シリアルモニタ を選びます。
1. 新しく開いたウィンドウの右下の設定を "改行なし" "115200baud" に変更し、一度ウィンドウを閉じます。
1. IRKitのUSB Micro-Bケーブルを外し、再度接続した直後に、上記同様に シリアルモニタ を開きます。

   IRKit起動直後からのログを取得するためです。

1. ウィンドウに表示される文字列をコピーして開発者へメールしてください。ありがとうございます！

## License

Firmware is licensed under GPLv2 or later.  
See details in [firmware/LICENSE](https://github.com/irkit/device/blob/master/firmware/LICENSE).

Schematics are licensed under Creative Commons Attribution Share-Alike license.  
See details in [hardware/LICENSE](https://github.com/irkit/device/blob/master/hardware/LICENSE).

Case is licensed under Creative Commons Attribution NonCommercial Share-Alike license.  
See details in [case/LICENSE](https://github.com/irkit/device/blob/master/case/LICENSE).

