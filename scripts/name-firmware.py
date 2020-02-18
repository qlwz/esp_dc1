Import('env')
import os
import shutil

OUTPUT_DIR = "output{}".format(os.path.sep)

def bin_map_copy(source, target, env):
    variant = str(target[0]).split(os.path.sep)[2]
    #print(variant)
    #print(str(target[0]))
    
    o = str(target[0]).replace("firmware.bin", "")
    dirs = os.listdir(o)
    for file in dirs:
        p = os.path.join(o, file, "esp_framework\\Rtc.cpp.o")
        if os.path.isfile(p):
            os.remove(p)
    
    # check if output directories exist and create if necessary
    if not os.path.isdir(OUTPUT_DIR):
        os.mkdir(OUTPUT_DIR)

    # create string with location and file names based on variant
    bin_file = "{}{}{}.bin".format(OUTPUT_DIR, os.path.sep, variant)

    if os.path.isfile(bin_file):
        os.remove(bin_file)

    # copy firmware.bin to firmware/<variant>.bin
    shutil.copy(str(target[0]), bin_file)
    

env.AddPostAction("$BUILD_DIR/${PROGNAME}.bin", [bin_map_copy])
