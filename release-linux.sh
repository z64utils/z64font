mkdir -p bin/release

gcc -o bin/release/z64font-linux -DZ64FONT_GUI `./common.sh` `wowlib/deps/wow_gui_x11.sh`

