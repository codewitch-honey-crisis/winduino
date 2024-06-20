@rmdir lib /S /Q
@mkdir lib
@cd lib
@git clone https://github.com/codewitch-honey-crisis/htcw_winduino htcw_winduino
@git clone https://github.com/codewitch-honey-crisis/htcw_tft_io htcw_tft_io
@git clone https://github.com/codewitch-honey-crisis/htcw_st7789 htcw_st7789
@git clone https://github.com/codewitch-honey-crisis/htcw_ft6236 htcw_ft6236
@git clone https://github.com/codewitch-honey-crisis/uix htcw_uix
@.\htcw_uix\fetch_deps.cmd
@cd ..
