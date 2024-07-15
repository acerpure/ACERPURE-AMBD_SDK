# ACERPURE-AMBD_SDK

## 編譯說明

- 由於上游的原專案 [ambd_sdk](https://github.com/ambiot/ambd_sdk) 是在 Windows 和在 Cygwin 之間開發，因此執行 Make 編譯時可能會遇到 LF 與 CRLF 換行的問題
  - 此外該專案也有很多檔案 LF 與 CRLF 混用，因此會有點麻煩
- 然後部分 Makefile 又有使用 `$(ABS_ROOTDIR)/code_analyze.py`，而 `code_analyze.py` 也會因為該問題造成無法執行
- 因此會需要根據系統為 Windows(CRLF) 與非 Windows(LF) 去調整部分檔案
  - [project/realtek_amebaD_va0_example/GCC-RELEASE/project_hp/asdk/Makefile](project/realtek_amebaD_va0_example/GCC-RELEASE/project_hp/asdk/Makefile)
  - [project/realtek_amebaD_va0_example/GCC-RELEASE/project_hp/asdk/code_analyze.py](project/realtek_amebaD_va0_example/GCC-RELEASE/project_hp/asdk/code_analyze.py)

## 待移植

- 魔改版 wifi_simple_config
  - component/common/api/wifi/wifi_simple_config.c

## 修改說明

- flash_util
  - 根據 BW16 處理各個 SECTOR 的 flash 記憶體位置
  - 使原本用 IAR EWARM 編譯之部分給改為用純 gcc 與 Makefile 做編譯
    - 參考 project/realtek_amebaD_va0_example/GCC-RELEASE/project_hp/asdk/make/utilities/Makefile
- acerpure_uart_atcmd
  - 修掉 extern var 的問題，以傳遞變數
  - 把跟 amazon-freertos 有關的部分給拆出去 (不相容)
  - 使原本用 IAR EWARM 編譯之部分給改為用純 gcc 與 Makefile 做編譯
    - 參考 project/realtek_amebaD_va0_example/GCC-RELEASE/project_hp/asdk/make/utilities_example/Makefile

## 測試

- 初始化 Wifi 設定
  - 修改 [component/common/application/acer/inc/startup/acer_init.h](component/common/application/acer/inc/startup/acer_init.h) 之內容並編譯即可

- 使用 mdns 以方便測試
  - mdns 服務名稱: ameba.local
- 使用 httpd 以方便測試，以下為各個 route path
  - <http://ameba.local/appversion>
    - [GET] response sample

      ```json
      {
        "type": "set-device",
        "field": "commfw",
        "value": "v1.1.0"
      }
      ```

  - <http://ameba.local/mainfw>
    - [GET] response sample

      ```json
      {
        "type": "set-device",
        "field": "mainfw",
        "value": ""
      }
      ```

  - <http://ameba.local/devicestate>
    - [GET] response sample

      ```json
      {}
      ```

  - <http://ameba.local/devicemonitor>

    - [GET] response sample

      ```json
      {
        "type": "save-monitor",
        "data": {
          "PM2.5": "0",
          "PM1.0": "0",
          "GAS": "0",
          "CO2": "0"
        }
      }
      ```

  - <http://ameba.local/action>

    - [POST] request json data, 可以只送單一個值, 如只設定 `'.Acerpure-0.Power = "On"'`

      ```json
      {
        "Acerpure-0":{
          "FilterHealthAlertInterval": "11",
          "FilterHealth5AlertInterval": "11",
          "FilterHealth10AlertInterval": "11",
          "FilterHealth20AlertInterval": "11",
          "FilterInstallAlertInterval": "11",
          "AQIAlertInterval": "11",
          "GASAlertInterval": "11",
          "CO2AlertInterval": "11",
          "OTAUrl": "",
          "OTAState": "Stop",
          "MyFavorIcon": "Off",
          "PkType": "",
          "Power": "On",
          "AirPurifierSpeed": "Smart",
          "AirCirculatorSpeed": "1",
          "AirPurifierRotate": "On",
          "AirCirculatorRotate": "On",
          "AirDetectMode": "PM2.5",
          "KidMode": "On",
          "SleepMode": "On",
          "ShutdownTimer": "1",
          "UV": "On"
        }
      }
      ```

## Build in container

- 本專案需要搭配 vscode 與 Docker 等容器化技術才能使用，請參考 [vscode devcontainer](https://code.visualstudio.com/docs/devcontainers/containers#_quick-start-open-an-existing-folder-in-a-container) 以開啟本專案的開發環境
- **因為建置過程中會使用 x86 指令集，所以如果是 Apple Mac silicon 之電腦的話，必須為 Docker-Desktop 或是 Orb-Stack 開啟 Rosetta 才能正常建置**
- 該 container 已準備好編譯帶有 matter fw 的環境。此外，該 container image 尚未精簡，故需要 60GB 的存儲空間。

<!-- 
### Get Ameba SDK & Matter SDK

```sh
# 因為整個 repo 實在太大惹，網路爛的話有可能會使部分內容 pull 不成功，並導致建置失敗，故把建議把 Log 打開來確認
export GIT_TRACE_PACKET=1
export GIT_TRACE=1
export GIT_CURL_VERBOSE=1

# 需要配對指定的 commit 才能動，別直接 pull master 或 tag，否則可能會建構失敗。可參考 release 頁面 https://github.com/ambiot/ambd_matter/releases

# 僅 clone tag v1.3-release 的 ambd_matter
git clone -b v1.3-release --single-branch https://github.com/ambiot/ambd_matter.git

# clone connectedhomeip 並切到前面 ambd_matter v1.3-release 所指定的 commit 70d9a61475d31686f0fde8e7b56f352a0f59b299
git clone --single-branch https://github.com/project-chip/connectedhomeip.git && bash -c "cd connectedhomeip && git reset --hard 70d9a61475d31686f0fde8e7b56f352a0f59b299"

# **<一定要做這一步驟。因爲該 repo 架構複雜，所以需使用額外的程式去搞相依>** 然後，指定使用 connectedhomeip 專案魔改的 git submodule 之 ameba 所需要之部分
# Checking out: nlassert, nlio, nlunit-test, mbedtls, qrcode, pigweed, openthread, nanopb, freertos, third_party/pybind11/repo, third_party/jsoncpp/repo, editline, third_party/boringssl/repo/src
/workspaces/matter/connectedhomeip/scripts/checkout_submodules.py --allow-changing-global-git-config --shallow --platform ameba
```

### Make project_lp

```sh
cd /workspaces/matter/ambd_matter/project/realtek_amebaD_va0_example/GCC-RELEASE/project_lp/
/workspaces/matter/connectedhomeip/scripts/run_in_build_env.sh make all
```

### Make Matter Libraries and project_hp

```sh
. /workspaces/matter/connectedhomeip/scripts/activate.sh
cd /workspaces/matter/ambd_matter/project/realtek_amebaD_va0_example/GCC-RELEASE/project_hp/
make -C asdk all_clusters
make all
```

## AmebaIOT Image Tools for MAC OSX

由於非 Linux 發行版的 Docker 容器其實都是在特殊的虛擬主機中執行，而且[無法直通 usb 裝置](https://docs.docker.com/desktop/faqs/general/#can-i-pass-through-a-usb-device-to-a-container)，故得於容器外的主機(即本機)上操作。

以下是在容器之外操作的（因為 docker 碰不到 usb 裝置），故請另外在本專案目錄開一個終端機去操作

```bash
# 建立有著要被燒錄的 km0_boot_all.bin, km4_boot_all.bin, km0_km4_image2.bin 
# 以及燒錄工具暫存檔之資料夾，並進入建立的 .upload 資料夾
mkdir .upload |: && cd .upload

# 燒錄工具初始化
../ambd_matter/tools/AmebaD/Image_Tool_MacOS/MacOS_v11/Ameba_1-10_MP_ImageTool_MacOS11 -set chip AmebaD
../ambd_matter/tools/AmebaD/Image_Tool_MacOS/MacOS_v11/Ameba_1-10_MP_ImageTool_MacOS11 -scan device
../ambd_matter/tools/AmebaD/Image_Tool_MacOS/MacOS_v11/Ameba_1-10_MP_ImageTool_MacOS11 -set baudrate 1500000

# 在 make 完前面的 project_lp 與 project_hp 後，複製以下檔案到 .upload 資料夾
cp ../ambd_matter/project/realtek_amebaD_va0_example/GCC-RELEASE/project_lp/asdk/image/km0_boot_all.bin .
cp ../ambd_matter/project/realtek_amebaD_va0_example/GCC-RELEASE/project_hp/asdk/image/{km4_boot_all.bin,km0_km4_image2.bin} .

# 定址每個 .bin 之 offset，然後合併為一個 Image_All.bin
../ambd_matter/tools/AmebaD/Image_Tool_MacOS/MacOS_v11/Ameba_1-10_MP_ImageTool_MacOS11 -combine km0_boot_all.bin 0x0000 km4_boot_all.bin 0x4000 km0_km4_image2.bin 0x6000
../ambd_matter/tools/AmebaD/Image_Tool_MacOS/MacOS_v11/Ameba_1-10_MP_ImageTool_MacOS11 -set image Image_All.bin

# 設置 Image 地址，然後確認
../ambd_matter/tools/AmebaD/Image_Tool_MacOS/MacOS_v11/Ameba_1-10_MP_ImageTool_MacOS11 -set address 0x08000000
../ambd_matter/tools/AmebaD/Image_Tool_MacOS/MacOS_v11/Ameba_1-10_MP_ImageTool_MacOS11 -show

# 燒錄至板子中
../ambd_matter/tools/AmebaD/Image_Tool_MacOS/MacOS_v11/Ameba_1-10_MP_ImageTool_MacOS11 -download

# 看 serial console
# 使用 `control` + `a` 進到操作模式後，再按 `k` 即可終止 console (下次燒錄前必須終止)
screen /dev/tty.wchusbserial130 115200
```
-->
