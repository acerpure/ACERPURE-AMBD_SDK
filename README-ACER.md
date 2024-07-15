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

    - [POST] request json data

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

