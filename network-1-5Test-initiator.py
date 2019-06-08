import subprocess
import os

commandList = []
commandList.append("./Initiator --subCount 5 --msgLength 10000")

for command in commandList:
    subprocess.Popen(command.split(), cwd="build/examples")


input("Press Enter to shutdown...\n")
os.system("pkill -f Echoer")
os.system("pkill -f Initiator")
