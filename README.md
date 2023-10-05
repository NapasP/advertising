# Advertising
This MetaMod:Source plugin for CS2 add the advertising

## Installation
1.	Install [Metamod:Source](https://www.sourcemm.net/downloads.php/?branch=master)
2.	Grab the [latest release package](https://github.com/NapasP/advertising/releases)
3.	Extract the package and upload it to your gameserver
4.	Use `meta list` to check if the plugin works

## Build instructions
Just as any other [AMBuild](https://wiki.alliedmods.net/AMBuild) project:
1. [Install AMBuild](https://wiki.alliedmods.net/AMBuild#Installation)
2. Download [CS2 SDK](https://github.com/alliedmodders/hl2sdk/tree/cs2)
3. Download [Metamod:Source](https://github.com/alliedmodders/metamod-source)
4. `mkdir build && cd build`
5. `python ../configure.py --enable-optimize --plugin-name=advertising --plugin-alias=advertising --sdks=cs2 --targets=x86_64 --mms_path=??? --hl2sdk-root=???`
6. `ambuild`