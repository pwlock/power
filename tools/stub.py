import subprocess
import sys 

krnl = sys.argv[1]
output = sys.argv[2]

LD = "clang"
OUTPUT_NAME = f"{output}lib__r-kernelapi.so"

args = f"tmp.c -ffreestanding -nostdlib -shared -o {OUTPUT_NAME} "

nmout = subprocess.run(("nm", "-n", "-j", krnl), capture_output=True, text=True).stdout
spl = nmout.splitlines()

def execute_linker():
    global args
    subprocess.run((LD, *args.split(" ")))

def emplace_to_args(idx):
    global args, spl
    args += f"-Wl,--defsym={spl[idx]}=__r_unreacheable "

with open("tmp.c", "w") as file:
    file.write("/* This is a simple, MECHANICALLY GENERATED," 
                "program created by stub.py. DO. NOT. EDIT. */\n")
    file.write("void __r_unreacheable() {  }")

print("Linking kernelapi...")
for i in range(0, len(spl), 1):
    emplace_to_args(i)

args.strip()
execute_linker()
print("\033[92mDone!\033[0m")