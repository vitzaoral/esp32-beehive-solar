# Zkouška doma: teploměr Xiaomi + brána ESP32

Větev `brana-xiaomi`. Zkouší se na **ESP32 ze šuplíku**, ne na desce u včel.

## 0. Než začneš (jednou)

1. **Blynk, nové zařízení:** v konzoli Blynk založ zařízení „Brána zkouška“ ze šablony **Vcely baterky** a zkopíruj jeho Auth Token.
   Nepoužívej token z desky u včel: beeSys čte solár z Blynku a zkušební deska bez INA3221 by mu poslala nuly a spustila poplachy baterie.
2. **Blynk, nové datastreamy v šabloně** (stávajících se to nedotkne):
   - V58 „BLE perioda min“: Integer 0–1440, výchozí 5
   - V59 „BLE stav“: String
   - V60 „BLE 1 teplota“: Double −40–85 °C, V61 „BLE 1 vlhkost“: Double 0–100 %, V62 „BLE 1 baterie“: Integer 0–100 %
3. **beeSys:** příjem z brány je ve větvi `brana-xiaomi` repa beeSys. Do zkoušky ho nasadí Claude, až to schválíš.
   Do `deploy/.env` přijde `BLE_GATEWAY_TOKEN=` (vygenerovat `openssl rand -hex 24`), stejná hodnota jde do `settings.cpp`.
   Bez nasazení ukáže Blynk V59 „beeSys 404“. Zbytek zkoušky funguje i tak.

## 1. Teploměr: nahrát pvvx z telefonu

> **Xiaomi Monitor 3 Mini (MJWSD06MMC)** se nahrává stejně: firmware `MJ6_vNN.bin` přes TelinkMiFlasher po přihlášení klíči z Mi Home (pvvx issue #636).
> Vysílá stejné formáty, brána ho zná bez úprav. Má baterii CR2450. Pár lidí ho při nahrávání zablokovalo, oprava pak jde jen kabelem přes USB-UART (pvvx issue #809).


1. Vyndej a vrať baterii. Na iPhonu nainstaluj prohlížeč **Bluefy** (Safari neumí Web Bluetooth). Jde to i v Chrome na Macu.
2. Otevři https://pvvx.github.io/ATC_MiThermometer/TelinkMiFlasher.html, dej **Connect** a vyber `LYWSD03MMC`.
3. Podívej se na **Hardware version** a **Software version**:
   - **Firmware 2.1.1_0159 a novější** (nové kusy, HW B1.5/B1.6) se nejdřív musí přihlásit:
     teploměr přidej do aplikace **Mi Home**. Na Macu pusť [Xiaomi-cloud-tokens-extractor](https://github.com/PiotrMachowski/Xiaomi-cloud-tokens-extractor) a opiš „Device known id“, „Mi Token“ a „Mi Bind Key“.
     Ve flasheru je vyplň, dej **Login** a u HW B1.5/B1.6 nahraj nejdřív nabízený `Original_OTA_…`. Displej pak nesvítí, to je v pořádku. Znovu se připoj a pokračuj krokem 4.
     Postup je popsaný v https://github.com/pvvx/ATC_MiThermometer/issues/602.
   - Starší firmware: rovnou krok 4.
4. Dej tlačítko **Custom Firmware ver 5.7** (nebo novější) a **Start Flashing**, trvá to asi minutu.
5. Po restartu se znovu připoj a nastav:
   - Advertising type **BTHome** (výchozí) nebo **Custom**. Brána umí obojí i ATC1441.
   - **Bez šifrování**: bindkey prázdný, nic „encrypted“.
   - Ostatní nech ve výchozím stavu (vysílá po 2,5 s). Dej **Send Config**.
6. Opiš **MAC** (`A4:C1:38:…`), je nahoře ve flasheru.

## 2. ESP32 ze šuplíku: nahrát firmware kabelem z Macu

```bash
cd ~/Developer/IoT/esp32-beehive-solar
git fetch origin && git checkout brana-xiaomi && git pull
```

- `src/settings.cpp` (není v gitu): doplň řádek `const char *beesysToken = "…";`. Vyplň Wi-Fi doma a **token zařízení Brána zkouška**.
- `src/ble_config.h`: místo `A4:C1:38:00:00:00` dej MAC teploměru.

```bash
ls /dev/cu.*                       # najdi port: cu.usbserial-…, cu.SLAB_USBtoUART nebo cu.wchusbserial…
pio test -e native                 # 13 testů parseru, mají projít
pio run -e esp32dev -t upload -t monitor --upload-port /dev/cu.usbserial-XXXX
```

Když nahrávání hlásí „Failed to connect“, drž při startu nahrávání na desce tlačítko **BOOT**.
Monitor ukončíš Ctrl+C. Znovu ho pustíš: `pio device monitor -b 115200`.

## 3. Co máš vidět

- **Sériová linka** po startu: `BLE sken … cidel 1/1` a řádek s teplotou, vlhkostí, baterií a RSSI. Pak `beeSys: POST 200`.
  Dokud MAC v `ble_config.h` nesedí, uvidíš `NEZNAME cidlo A4:C1:38:…`, tu MAC si zkopíruj.
- **Blynk** (zařízení Brána zkouška): V60–V62 hodnoty z teploměru, V59 `BLE 1/1 | beeSys 200 | OTA ok`.
  Poslouchá se při startu a pak každých 5 minut (každé 5. probuzení).
- **beeSys** (sys.zaoral.cz, karta Úly): nové čidlo „Zkouska Xiaomi“ se signálem v dBm a baterií, v grafu teplot vlastní čára.
  Přesun k úlu = v adminu změnit `location` na `hive_X`.

## 4. Co vyzkoušet (asi 2 týdny)

| Zkouška | Jak | Má se stát |
|---|---|---|
| Výpadek brány | Odpoj ESP32 na **6 hodin**, pak zapoj. | Po 30–40 min přijde push „Teploměr u včel mlčí“. Do minuty po zapojení přijdou data a push „zase hlásí“. Teploměr nic nepotřebuje. |
| Mráz | Teploměr dej na 2–3 h do **mrazáku**. | Mrazák stíní Bluetooth, mezera v datech je normální. Po vyndání se musí sám ozvat. Zkontroluj napětí baterie (pokles v mrazu) a ve flasheru GraphMemo nejnižší teplotu. |
| Dosah | Teploměr dej do zavřeného hrnce s poklicí (jako plechová střecha úlu) a postupně dál od brány. | U včel je nejdál 10 m. RSSI nad −85 dBm je v pořádku, pod −90 dBm začne vypadávat. |
| Výdrž | Nech běžet. | Sleduj baterii (mV) v beeSysu, jestli neklesá rychleji než pár mV týdně. |
| Vypnutí na dálku | V Blynku nastav V58 = 0, pak zpět 5. | BLE se přestane poslouchat od dalšího probuzení. |

## 5. Pak (zatím nedělat)

- Nahrání do desky u včel přes Blynk.Air (`firmware.bin`), jen po schválení.
- Po OTA má V59 jednou ukázat `OTA ceka` a další minutu `OTA ok`. To znamená, že bootloader umí návrat ke staré verzi.
  Když `ceka` neuvidíš, deska u včel návrat neumí a chrání ji jen opatrnost.
