import subprocess
import os

# print(os.getcwd())

msgLen = '1000'

commandList = []
commandList.append("./Initiator --msgLength " + msgLen)
commandList.append("./Initiator --pubName Dummy --msgLength " + msgLen)
commandList.append("./Initiator --pubName Dummy --msgLength " + msgLen)
commandList.append("./Initiator --pubName Dummy --msgLength " + msgLen)
commandList.append("./Initiator --pubName Dummy --msgLength " + msgLen)


for command in commandList:
    subprocess.Popen(command.split(), cwd="build/examples")


input("Press Enter to shutdown...\n")
os.system("pkill -f Echoer")
os.system("pkill -f Initiator")
