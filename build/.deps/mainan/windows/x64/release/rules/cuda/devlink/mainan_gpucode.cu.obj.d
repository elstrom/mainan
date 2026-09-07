{
    files = {
        [[build\.objs\mainan\windows\x64\release\gpu_mind.cu.obj]]
    },
    values = {
        [[C:\Program Files\NVIDIA GPU Computing Toolkit\CUDA\v13.3\bin\nvcc]],
        {
            [[-LC:\Program Files\NVIDIA GPU Computing Toolkit\CUDA\v13.3\lib\x64]],
            "-lcudart",
            "-lgdi32",
            "-luser32",
            "-lws2_32",
            "-lshell32",
            "-lcudadevrt",
            "-m64",
            "-dlink"
        }
    }
}