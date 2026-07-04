# OpenHantek6022 遠端控制（CLI / MCP）

OpenHantek 內建一個 SCPI 風格的 TCP 控制伺服器（僅監聽 localhost），
讓命令列工具與 LLM（透過 MCP）可以調整檔位、觸發、擷取截圖與讀取量測值。

## 啟動

```sh
OpenHantek --server 5025          # 真實硬體
OpenHantek --demoMode --server 5025   # 無硬體展示模式
```

也可以在 GUI 內操作：選單「**遠端 → 啟用 MCP/SCPI 伺服器**」（可設定連接埠、檢視指令記錄），
狀態列右下角會顯示伺服器狀態：`MCP ● 127.0.0.1:5025`（綠色 = 監聽中）。
此設定會被記住，下次啟動自動生效。

## 協定（TCP, line-based）

每行一個指令，回覆 `OK [payload]` 或 `ERR <message>`。

| 指令 | 說明 |
|---|---|
| `*IDN?` | 識別：`OpenHantek6022,<版本>,<裝置>` |
| `RUN` / `STOP` | 開始／停止擷取 |
| `SINGLE` | 單次觸發（觸發一次後停止） |
| `AUTOSET` | 自動調整增益、時基、偏移與觸發（削頂時自動迭代） |
| `FORCETRIGGER` | 強制重新擷取（Roll 模式刷新） |
| `CH<n>:ENABLE ON\|OFF` | 開關通道（n = 1..2） |
| `CH<n>:GAIN <V/div>` | 垂直增益，自動貼齊 20mV..10V 檔位 |
| `CH<n>:COUPLING AC\|DC` | 耦合（AC 需硬體改裝） |
| `CH<n>:PROBE <attn>` | 探棒衰減 1..1000 |
| `CH<n>:INVERT ON\|OFF` | 反轉 |
| `CH<n>:OFFSET <div>` | 垂直位置 （-4..4 格） |
| `TIMEBASE <s/div>` / `TIMEBASE?` | 時基 |
| `SAMPLERATE <S/s>` / `SAMPLERATE?` | 採樣率（貼齊硬體檔位） |
| `TRIGGER:MODE AUTO\|NORMAL\|SINGLE\|ROLL` | 觸發模式 |
| `TRIGGER:SOURCE <1\|2>` | 觸發源 |
| `TRIGGER:SLOPE POS\|NEG\|BOTH` | 觸發沿 |
| `TRIGGER:LEVEL <V>` | 觸發準位 |
| `CALFREQ <Hz>` | 校正方波輸出頻率 |
| `MEASURE?` | JSON：各通道 Vpp/Vmax/Vmin/DC/RMS/頻率/是否削頂 |
| `CONFIG?` | JSON：目前完整設定 |
| `SCREENSHOT [path]` | 截圖存成 PNG，回傳絕對路徑 |
| `FONTSIZE <6..24>` | 即時調整 UI 字體大小 |

## CLI：`ohctl`

```sh
./ohctl idn
./ohctl ch 1 --gain 0.5 --coupling DC --enable on
./ohctl timebase 0.001
./ohctl trigger --mode NORMAL --source 1 --slope POS --level 1.2
./ohctl autoset
./ohctl measure          # JSON 輸出
./ohctl screenshot /tmp/scope.png
./ohctl raw "CH2:OFFSET -1.5"
```

## MCP Server（給 LLM 用）

需求：`pip install mcp`（Python 3.10+）

工具：`identify, run, stop, single, autoset, set_channel, set_timebase,
set_trigger, set_calibration_frequency, measure, get_config, screenshot`
（`screenshot` 直接回傳影像內容，LLM 可以「看」到波形）

環境變數：`OPENHANTEK_HOST`（預設 127.0.0.1）、`OPENHANTEK_PORT`（預設 5025）。

### Claude Code 配置

方法一：CLI 一行註冊（目前專案）

```sh
claude mcp add openhantek -- python3 /ABS/PATH/OpenHantek6022/mcp_server/openhantek_mcp.py
# 全域（所有專案）：
claude mcp add --scope user openhantek -- python3 /ABS/PATH/OpenHantek6022/mcp_server/openhantek_mcp.py
```

方法二：專案根目錄 `.mcp.json`（可進版控與團隊共用）

```json
{
  "mcpServers": {
    "openhantek": {
      "command": "python3",
      "args": ["/ABS/PATH/OpenHantek6022/mcp_server/openhantek_mcp.py"],
      "env": { "OPENHANTEK_PORT": "5025" }
    }
  }
}
```

驗證：`claude mcp list` 應出現 `openhantek`；對話中直接說
「幫我 autoset 然後截圖看波形」即可。

### opencode 配置

在專案或家目錄的 `opencode.json`（`~/.config/opencode/opencode.json`）加入：

```json
{
  "$schema": "https://opencode.ai/config.json",
  "mcp": {
    "openhantek": {
      "type": "local",
      "command": ["python3", "/ABS/PATH/OpenHantek6022/mcp_server/openhantek_mcp.py"],
      "enabled": true,
      "environment": { "OPENHANTEK_PORT": "5025" }
    }
  }
}
```

### 使用前提

1. 先啟動 OpenHantek 並開啟伺服器（`--server 5025` 或選單「遠端」）
2. `pip install mcp`（建議 venv：`python3 -m venv .venv && .venv/bin/pip install mcp`，
   並把配置中的 `python3` 換成 `.venv/bin/python3`）
