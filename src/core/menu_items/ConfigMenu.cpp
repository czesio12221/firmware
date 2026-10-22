#include "ConfigMenu.h"
#include "../mykeyboard.h"
#include "core/display.h"
#include "core/i2c_finder.h"
#include "core/main_menu.h"
#include "core/settings.h"
#include "core/utils.h"
#include "core/wifi/wifi_common.h"
#ifdef HAS_RGB_LED
#include "core/led_control.h"
#endif

/*********************************************************************
**  Function: optionsMenu
**  Main Config menu entry point
**********************************************************************/
void ConfigMenu::optionsMenu() {
    returnToMenu = false;
    while (true) {
        // Check if we need to exit to Main Menu (e.g., DevMode disabled)
        if (returnToMenu) {
            returnToMenu = false; // Reset flag
            return;
        }

        std::vector<Option> localOptions = {
            {"Display & UI",  [this]() { displayUIMenu(); }},
#ifdef HAS_RGB_LED
            {"LED USTAWIENIA",    [this]() { ledMenu(); }      },
#endif
#if !defined(LITE_VERSION) && (defined(BUZZ_PIN) || defined(HAS_NS4168_SPKR))
            {"Audio USTAWIENIA",  [this]() { audioMenu(); }    },
#endif
            {"System ustawienia", [this]() { systemMenu(); }   },
            {"ZASILANIE wył",         [this]() { powerMenu(); }    },
        };

#if !defined(LITE_VERSION)
        if (!appStoreInstalled()) {
            localOptions.push_back({"instalowanie aplikacji", []() { installAppStoreJS(); }});
        }
#endif

        if (bruceConfig.devMode) {
            localOptions.push_back({"TRYB PROGRAMISTY", [this]() { devMenu(); }});
        }

        localOptions.push_back({"informacje o urzadeniu", showDeviceInfo});
        localOptions.push_back({"MeNU GLOWNE", []() {}});

        int selected = loopOptions(localOptions, MENU_TYPE_SUBMENU, "ustawienia");

        // Exit to Main Menu only if user pressed Back
        if (selected == -1 || selected == localOptions.size() - 1) { return; }
        // Otherwise rebuild Config menu after submenu returns
    }
}

/*********************************************************************
**  Function: displayUIMenu
**  Display & UI configuration submenu with auto-rebuild
**********************************************************************/
void ConfigMenu::displayUIMenu() {
    while (true) {
        std::vector<Option> localOptions = {
            {"JASNOSC",  [this]() { setBrightnessMenu(); }               },
            {"wygaszacz ekranu",    [this]() { setDimmerTimeMenu(); }               },
            {"OBRUCi EKRAN", [this]() { lambdaHelper(gsetRotation, true)(); }},
            {"kolor systemu",    [this]() { setUIColor(); }                      },
            {"MOTYW",    [this]() { setTheme(); }                        },
            {"WSTECZ",        []() {}                                         },
        };

        int selected = loopOptions(localOptions, MENU_TYPE_SUBMENU, "Ekran iwyglad");

        // Exit only if user pressed Back or ESC
        if (selected == -1 || selected == localOptions.size() - 1) { return; }
        // Otherwise loop continues and menu rebuilds
    }
}

/*********************************************************************
**  Function: ledMenu
**  LED configuration submenu with auto-rebuild for toggles
**********************************************************************/
#ifdef HAS_RGB_LED
void ConfigMenu::ledMenu() {
    while (true) {
        std::vector<Option> localOptions = {
            {"LED Color",
             [this]() {
                 beginLed();
                 setLedColorConfig();
             }                                                                            },
            {"LED Effect",
             [this]() {
                 beginLed();
                 setLedEffectConfig();
             }                                                                            },
            {"LED Brightness",
             [this]() {
                 beginLed();
                 setLedBrightnessConfig();
             }                                                                            },
            {String("LED Blink: ") + (bruceConfig.ledBlinkEnabled ? "ON" : "OFF"),
             [this]() {
                 // Toggle LED blink setting
                 bruceConfig.ledBlinkEnabled = !bruceConfig.ledBlinkEnabled;
                 bruceConfig.saveFile();
             }                                                                            },
            {"WSTECZ",                                                               []() {}},
        };

        int selected = loopOptions(localOptions, MENU_TYPE_SUBMENU, "LED ustawienia");

        // Exit only if user pressed Back or ESC
        if (selected == -1 || selected == localOptions.size() - 1) { return; }
        // Menu rebuilds to update toggle label
    }
}
#endif
/*********************************************************************
**  Function: audioMenu
**  Audio configuration submenu with auto-rebuild for toggles
**********************************************************************/
void ConfigMenu::audioMenu() {
    while (true) {
        std::vector<Option> localOptions = {
#if !defined(LITE_VERSION)
#if defined(BUZZ_PIN) || defined(HAS_NS4168_SPKR)

            {String("Sound: ") + (bruceConfig.soundEnabled ? "ON" : "OFF"),
                                                             [this]() {
                 // Toggle sound setting
                 bruceConfig.soundEnabled = !bruceConfig.soundEnabled;
                 bruceConfig.saveFile();
             }                                                                                                                                            },
#if defined(HAS_NS4168_SPKR)
            {"GLOSNOSC",                                                [this]() { setSoundVolume(); }},
#endif  // BUZZ_PIN || HAS_NS4168_SPKR
#endif  //  HAS_NS4168_SPKR
#endif  //  LITE_VERSION
            {"WSTECZ",                                                        []() {}                       },
        };

        int selected = loopOptions(localOptions, MENU_TYPE_SUBMENU, "ustawienia audio");

        // Exit only if user pressed Back or ESC
        if (selected == -1 || selected == localOptions.size() - 1) { return; }
        // Menu rebuilds to update toggle label
    }
}

/*********************************************************************
**  Function: systemMenu
**  System configuration submenu with auto-rebuild for toggles
**********************************************************************/
void ConfigMenu::systemMenu() {
    while (true) {
        std::vector<Option> localOptions = {
            {String("InstaBoot: ") + (bruceConfig.instantBoot ? "ON" : "OFF"),
             [this]() {
                 // Toggle InstaBoot setting
                 bruceConfig.instantBoot = !bruceConfig.instantBoot;
                 bruceConfig.saveFile();
             }                                                                                                           },
            {String("wifi przy starcie") + (bruceConfig.wifiAtStartup ? "ON" : "OFF"),
             [this]() {
                 // Toggle WiFi at startup setting
                 bruceConfig.wifiAtStartup = !bruceConfig.wifiAtStartup;
                 bruceConfig.saveFile();
             }                                                                                                           },
            {"APLIKACJA STARTOWA",                                                         [this]() { setStartupApp(); }        },
            {"UKR>POKARZ APK",                                                      [this]() { mainMenu.hideAppsMenu(); }},
            {"ZEGAREK",                                                               [this]() { setClock(); }             },
            {String("jezyk klawiatury") + bruceConfig.keyboardLang,              [this]() { setKeyboardLanguage(); }  },
            {"ZAWANSOWANE",                                                            [this]() { advancedMenu(); }         },
            {"WSTECZ",                                                                []() {}                              },
        };

        int selected = loopOptions(localOptions, MENU_TYPE_SUBMENU, "USTAWIENIA SYSTEMU");

        // Exit only if user pressed Back or ESC
        if (selected == -1 || selected == localOptions.size() - 1) { return; }
        // Menu rebuilds to update toggle labels
    }
}

/*********************************************************************
**  Function: advancedMenu
**  Advanced settings submenu (nested under System Config)
**********************************************************************/
void ConfigMenu::advancedMenu() {
    while (true) {
        std::vector<Option> localOptions = {
            {"USTAWIENIA PINOW", [this]() { pinsMenu(); }           },
#if !defined(LITE_VERSION)
            {"Toggle BLE API",  [this]() { enableBLEAPI(); }       },
            {"BadUSB/BLE",      [this]() { setBadUSBBLEMenu(); }   },
#endif
            {"NAZWA Bt",
             [this]() {
                 String name = keyboard(bruceConfigPins.bleName, 30, "NAZWA URZDZENIA BT");
                 if (name.length() > 0 && name != "\x1B") bruceConfigPins.setBleName(name);
             }                                                     },
            {"DANE SIECI WIFI",   [this]() { setNetworkCredsMenu(); }},
            {"RESET FABRYCZNY",
             []() {
                 // Confirmation dialog for destructive action
                 drawMainBorder(true);
                 int8_t choice = displayMessage(
                     "Are you sure you want\nto Factory Reset?\nAll data will be lost!",
                     "No",
                     nullptr,
                     "Yes",
                     TFT_RED
                 );

                 if (choice == 1) {
                     // User confirmed - perform factory reset
                     bruceConfigPins.factoryReset();
                     bruceConfig.factoryReset(); // Restarts ESP
                 }
                 // If cancelled, loop continues and menu rebuilds
             }                                                     },
            {"Back",            []() {}                            },
        };

        int selected = loopOptions(localOptions, MENU_TYPE_SUBMENU, "Advanced");

        // Exit to System Config menu
        if (selected == -1 || selected == localOptions.size() - 1) { return; }
        // Menu rebuilds after each action
    }
}
/*********************************************************************
**  Function: powerMenu
**  Power management submenu with auto-rebuild
**********************************************************************/
void ConfigMenu::powerMenu() {
    while (true) {
        std::vector<Option> localOptions = {
            {"GŁEMBOKI SEN ", goToDeepSleep          },
            {"USPIJ ",      setSleepMode           },
            {"Restart",    []() { ESP.restart(); }},
            {"WYLACZ",
             []() {
                 // Confirmation dialog for power off
                 drawMainBorder(true);
                 int8_t choice = displayMessage("Power Off Device?", "No", nullptr, "Yes", TFT_RED);

                 if (choice == 1) { powerOff(); }
             }                                    },
            {"Back",       []() {}                },
        };

        int selected = loopOptions(localOptions, MENU_TYPE_SUBMENU, "Power Menu");

        // Exit to Config menu
        if (selected == -1 || selected == localOptions.size() - 1) { return; }
        // Menu rebuilds after each action
    }
}

/*********************************************************************
**  Function: devMenu
**  Developer mode menu for advanced hardware configuration
**********************************************************************/
void ConfigMenu::devMenu() {
    while (true) {
        std::vector<Option> localOptions = {
            {"PORT SZEREGOWY USB",  [this]() { switchToUSBSerial(); }          },
            {"PORT SZERGOWU UARD", [this]() { switchToUARTSerial(); }         },
            {"WYLACZ TRYB PROGRAMISTY", [this]() { bruceConfig.setDevMode(false); }},
            {"WSTECZ",            []() {}                                    },
        };

        int selected = loopOptions(localOptions, MENU_TYPE_SUBMENU, "TRYB PROGRAMISTY");

        // Check if "Disable DevMode" was pressed (second-to-last option)
        if (selected == localOptions.size() - 2) {
            returnToMenu = true; // Signal to exit all Config menus
            return;
        }

        // Exit to Config menu on Back or ESC
        if (selected == -1 || selected == localOptions.size() - 1) { return; }
        // Menu rebuilds after each action
    }
}

/*********************************************************************
**  Function: pinsMenu
**  Developer mode menu for advanced hardware configuration
**********************************************************************/
void ConfigMenu::pinsMenu() {
    while (true) {
        std::vector<Option> localOptions = {
            {"I2C WYSZUKIWARKA",     [this]() { find_i2c_addresses(); }                      },
            {"CC1101 PINY",    [this]() { setSPIPinsMenu(bruceConfigPins.CC1101_bus); }},
            {"NRF24 PINY",    [this]() { setSPIPinsMenu(bruceConfigPins.NRF24_bus); } },
#if !defined(LITE_VERSION)
            {"LoRa PINY",      [this]() { setSPIPinsMenu(bruceConfigPins.LoRa_bus); }  },
            {"PINY RFID", [this]() { setSPIPinsMenu(bruceConfigPins.ST25R_bus); } },
            {"INTERNET KABLOWY",     [this]() { setSPIPinsMenu(bruceConfigPins.W5500_bus); } },
#endif
            {"SDCard PINY",    [this]() { setSPIPinsMenu(bruceConfigPins.SDCARD_bus); }},
            {"I2C PINY",       [this]() { setI2CPinsMenu(bruceConfigPins.i2c_bus); }   },
            {"UART PINY",      [this]() { setUARTPinsMenu(bruceConfigPins.uart_bus); } },
            {"GPS PINY",       [this]() { setUARTPinsMenu(bruceConfigPins.gps_bus); }  },
            //{"Serial use USB",  [this]() { switchToUSBSerial(); }                       },
            //{"Serial use UART", [this]() { switchToUARTSerial(); }                      },
            {"Back",           []() {}                                                 },
        };

        int selected = loopOptions(localOptions, MENU_TYPE_SUBMENU, "Pins Setup");

        // Exit to Config menu on Back or ESC
        if (selected == -1 || selected == localOptions.size() - 1) { return; }
        // Menu rebuilds after each action
    }
}

/*********************************************************************
**  Function: switchToUSBSerial
**  Switch serial output to USB Serial
**********************************************************************/
void ConfigMenu::switchToUSBSerial() {
    USBserial.setSerialOutput(&Serial);
    Serial1.end();
}

/*********************************************************************
**  Function: switchToUARTSerial
**  Switch serial output to UART (handles pin conflicts)
**********************************************************************/
void ConfigMenu::switchToUARTSerial() {
    // Check and resolve SD card pin conflicts
    if (bruceConfigPins.SDCARD_bus.checkConflict(bruceConfigPins.uart_bus.rx) ||
        bruceConfigPins.SDCARD_bus.checkConflict(bruceConfigPins.uart_bus.tx)) {
        sdcardSPI.end();
    }

    // Check and resolve CC1101/NRF24 pin conflicts
    if (bruceConfigPins.CC1101_bus.checkConflict(bruceConfigPins.uart_bus.rx) ||
        bruceConfigPins.CC1101_bus.checkConflict(bruceConfigPins.uart_bus.tx) ||
        bruceConfigPins.NRF24_bus.checkConflict(bruceConfigPins.uart_bus.rx) ||
        bruceConfigPins.NRF24_bus.checkConflict(bruceConfigPins.uart_bus.tx)) {
        AUX_SPI.end();
    }

    // Configure UART pins and switch serial output
    pinMode(bruceConfigPins.uart_bus.rx, INPUT);
    pinMode(bruceConfigPins.uart_bus.tx, OUTPUT);
    Serial1.begin(115200, SERIAL_8N1, bruceConfigPins.uart_bus.rx, bruceConfigPins.uart_bus.tx);
    USBserial.setSerialOutput(&Serial1);
}
/*********************************************************************
**  Function: drawIcon
**  Draw config gear icon
**********************************************************************/
void ConfigMenu::drawIcon(float scale) {
    clearIconArea();
    int radius = scale * 9;

    // Draw 6 gear teeth segments
    for (int i = 0; i < 6; i++) {
        tft.drawArc(
            iconCenterX,
            iconCenterY,
            3.5 * radius,
            2 * radius,
            15 + 60 * i,
            45 + 60 * i,
            bruceConfig.priColor,
            bruceConfig.bgColor,
            true
        );
    }

    // Draw inner circle
    tft.drawArc(
        iconCenterX,
        iconCenterY,
        2.5 * radius,
        radius,
        0,
        360,
        bruceConfig.priColor,
        bruceConfig.bgColor,
        false
    );
}
