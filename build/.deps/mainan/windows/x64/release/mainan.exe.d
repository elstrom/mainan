{
    files = {
        [[build\.objs\mainan\windows\x64\release\gpu_mind.cu.obj]],
        [[build\.objs\mainan\windows\x64\release\main.cpp.obj]],
        [[build\.objs\mainan\windows\x64\release\rules\cuda\devlink\mainan_gpucode.cu.obj]]
    },
    values = {
        [[C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\Tools\MSVC\14.44.35207\bin\HostX64\x64\link.exe]],
        {
            "-nologo",
            "-dynamicbase",
            "-nxcompat",
            "-machine:x64",
            [[-libpath:C:\Program Files\NVIDIA GPU Computing Toolkit\CUDA\v13.1\lib\x64]],
            "cudart.lib",
            "gdi32.lib",
            "user32.lib",
            "ws2_32.lib",
            "shell32.lib",
            "cudadevrt.lib"
        }
    }
}