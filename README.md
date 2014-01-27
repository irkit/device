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

<a id="firmware-update"></a>
## ファームウェアアップデート方法

IRKitのファームウェアは、 [Arduino IDE](http://arduino.cc/en/Main/Software) を使って書き込むことができます。  
以下の手順で書き込みできます(MacOS編)。

1. [Arduino IDE](http://arduino.cc/en/Main/Software) をダウンロードしてインストール後、起動します。
1. IRKitをUSB Micro-Bケーブルを使ってMacに接続します。
1. Arduino IDEのメニューから ツール -> マイコンボード -> "Arduino Leonardo" を選びます。
1. Arduino IDEのメニューから ツール -> シリアルポート -> "/dev/tty.usbmodemXXXX" を選びます。

   環境によってXXXXの部分やその前後が異なる場合があります。

1. [IRKit device tag一覧](https://github.com/irkit/device/releases) から最新のzipを選択しダウンロードし、解凍します。
1. 解凍したディレクトリ内にある `firmware/src/IRKit/` ディレクトリを開きます。
1. 同ディレクトリ内の `version.template` を `version.c` にリネームします。  

   参考) 製品版ではIRKitのバージョン情報をここに埋め込んでいます。

1. 同ディレクトリ内の `IRKit.ino` をダブルクリックして Arduino IDE で開きます。
1. Arduino IDEのメニューから ファイル -> マイコンボードに書き込む を押してIRKitに書き込みます。
1. 完了！

## License

Firmware is licensed under GPLv2 or later.  
See details in [firmware/LICENSE](https://github.com/irkit/device/blob/master/firmware/LICENSE).

Schematics are licensed under Creative Commons Attribution Share-Alike license.  
See details in [hardware/LICENSE](https://github.com/irkit/device/blob/master/hardware/LICENSE).

Case is licensed under Creative Commons Attribution NonCommercial Share-Alike license.  
See details in [case/LICENSE](https://github.com/irkit/device/blob/master/case/LICENSE).

