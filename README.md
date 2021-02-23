# Xilinx SDK Projects for EasyExcel2USB
## Build Instructions
1. Launch Xilinx SDK from a clean workspace.
2. Import current directory as a Git repository in SDK.
3. Right-click on each BSP project and select "Re-generate BSP Sources".
4. Build all the application projects as normal.
5. Right-click on hw_platform and select "Change Hardware Platform Specification".
6. In the popped window, select the .hdf file exported by the HDL project.
7. Once the .hdf file changed, SDK will automatically guide you to update the hdf file from the external reference specified in previous step.
## Download Instructions
1. Once a new bootloader elf file is built, Vivado will mark the bitstream as an out-of-date one. Just rerun "Generate Bitstream" to obtain a new bitstream containing the latest bootloader program.
2. To program the application into flash, from tool-bar in SDK, launch 'Xilinx -> Program Flash'. In the popped window, choose the QSPI flash on board and select "lwip_server.elf" as Image File. Pay attention to the offset address, which must identical to the BASEADDR value written in eb-config.h or blconfig.h (default: 0xC00000). When download is finished, recycle the power or send 'Boot form Configuration Device' in Vivado to load the new application program into DDR memory.
## Required SDK Version
* Tested on SDK 2019.1 and 2018.3
