
copy Output\SampleBox.hex .\Bin\SampleBox.hex
copy "..\..\SmartBox_hw1.7 - boot\MDK-ARM\Output\SmartBox-Boot.hex" ".\Bin\SampleBox-Boot.hex"

::功能:hex转bin 生成远程升级固件
srec_cat .\Bin\SampleBox.hex -Intel -offset -0x8020000 -o .\Bin\SampleBox.bin -Binary
srec_cat .\Bin\SampleBox-Boot.hex -Intel -offset -0x8000000 -o .\Bin\SampleBox-Boot.bin -Binary

::功能:合并bin 生成离线烧写固件
srec_cat .\Bin\SampleBox-Boot.bin -binary -fill 0xFF 0x0000 0x20000 .\Bin\SampleBox.bin -binary -offset 0x20000 -o .\Bin\SampleBox-Merge.bin -Binary
::srec_cat Bin\SampleBox.bin -binary -offset 0x20000 -o Bin\SampleBox-Merge.bin -Binary

::功能:合并hex 用于KEIL下载
srec_cat Bin\SampleBox-Boot.hex -Intel Bin\SampleBox.hex -Intel -o Bin\SampleBox-Merge.hex -Intel

copy Bin\SampleBox-Merge.hex .\output\SampleBox-Merge.hex

exit
